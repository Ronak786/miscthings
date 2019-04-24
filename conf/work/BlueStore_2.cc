// vim: ts=8 sw=2 smarttab
/*
 * Ceph - scalable distributed file system
 *
 * Copyright (C) 2014 Red Hat
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "include/cpp-btree/btree_set.h"

#include "BlueStore.h"
#include "os/kv.h"
#include "include/compat.h"
#include "include/intarith.h"
#include "include/stringify.h"
#include "common/errno.h"
#include "common/safe_io.h"
#include "Allocator.h"
#include "FreelistManager.h"
#include "BlueFS.h"
#include "BlueRocksEnv.h"
#include "auth/Crypto.h"
#include "common/EventTrace.h"

#define dout_context cct
#define dout_subsys ceph_subsys_bluestore

//using bid_t = decltype(BlueStore::Blob::id);

// bluestore_cache_onode
MEMPOOL_DECLARE_FACTORY(BlueStore::Onode, bluestore_onode,
			      bluestore_cache_onode);

// bluestore_cache_other
MEMPOOL_DECLARE_FACTORY(BlueStore::Buffer, bluestore_buffer,
			      bluestore_cache_other);
MEMPOOL_DECLARE_FACTORY(BlueStore::Extent, bluestore_extent,
			      bluestore_cache_other);
MEMPOOL_DECLARE_FACTORY(BlueStore::Blob, bluestore_blob,
			      bluestore_cache_other);
MEMPOOL_DECLARE_FACTORY(BlueStore::SharedBlob, bluestore_shared_blob,
			      bluestore_cache_other);

// bluestore_txc
MEMPOOL_DECLARE_FACTORY(BlueStore::TransContext, bluestore_transcontext, bluestore_txc);


// kv store prefixes
const string PREFIX_SUPER = "S";   // field -> value
const string PREFIX_STAT = "T";    // field -> value(int64 array)
const string PREFIX_COLL = "C";    // collection name -> cnode_t
const string PREFIX_OBJ = "O";     // object name -> onode_t
const string PREFIX_OMAP = "M";    // u64 + keyname -> value
const string PREFIX_DEFERRED = "L";  // id -> deferred_transaction_t
const string PREFIX_ALLOC = "B";   // u64 offset -> u64 length (freelist)
const string PREFIX_SHARED_BLOB = "X"; // u64 offset -> shared_blob_t

// write a label in the first block.  always use this size.  note that
// bluefs makes a matching assumption about the location of its
// superblock (always the second block of the device).
#define BDEV_LABEL_BLOCK_SIZE  4096

// reserve: label (4k) + bluefs super (4k), which means we start at 8k.
#define SUPER_RESERVED  8192

#define OBJECT_MAX_SIZE 0xffffffff // 32 bits


/*
 * extent map blob encoding
 *
 * we use the low bits of the blobid field to indicate some common scenarios
 * and spanning vs local ids.  See ExtentMap::{encode,decode}_some().
 */
#define BLOBID_FLAG_CONTIGUOUS 0x1  // this extent starts at end of previous
#define BLOBID_FLAG_ZEROOFFSET 0x2  // blob_offset is 0
#define BLOBID_FLAG_SAMELENGTH 0x4  // length matches previous extent
#define BLOBID_FLAG_SPANNING   0x8  // has spanning blob id
#define BLOBID_SHIFT_BITS        4

/*
 * object name key structure
 *
 * encoded u8: shard + 2^7 (so that it sorts properly)
 * encoded u64: poolid + 2^63 (so that it sorts properly)
 * encoded u32: hash (bit reversed)
 *
 * escaped string: namespace
 *
 * escaped string: key or object name
 * 1 char: '<', '=', or '>'.  if =, then object key == object name, and
 *         we are done.  otherwise, we are followed by the object name.
 * escaped string: object name (unless '=' above)
 *
 * encoded u64: snap
 * encoded u64: generation
 * 'o'
 */
#define ONODE_KEY_SUFFIX 'o'

/*
 * extent shard key
 *
 * object prefix key
 * u32
 * 'x'
 */
#define EXTENT_SHARD_KEY_SUFFIX 'x'

/*
 * string encoding in the key
 *
 * The key string needs to lexicographically sort the same way that
 * ghobject_t does.  We do this by escaping anything <= to '#' with #
 * plus a 2 digit hex string, and anything >= '~' with ~ plus the two
 * hex digits.
 *
 * We use ! as a terminator for strings; this works because it is < #
 * and will get escaped if it is present in the string.
 *
 */
template<typename S>
static void append_escaped(const string &in, S *out)
{
  char hexbyte[in.length() * 3 + 1];
  char* ptr = &hexbyte[0];
  for (string::const_iterator i = in.begin(); i != in.end(); ++i) {
    if (*i <= '#') {
      *ptr++ = '#';
      *ptr++ = "0123456789abcdef"[(*i >> 4) & 0x0f];
      *ptr++ = "0123456789abcdef"[*i & 0x0f];
    } else if (*i >= '~') {
      *ptr++ = '~';
      *ptr++ = "0123456789abcdef"[(*i >> 4) & 0x0f];
      *ptr++ = "0123456789abcdef"[*i & 0x0f];
    } else {
      *ptr++  = *i;
    }
  }
  *ptr++ = '!';
  out->append(hexbyte, ptr - &hexbyte[0]);
}

inline unsigned h2i(char c)
{
  if ((c >= '0') && (c <= '9')) {
    return c - 0x30;
  } else if ((c >= 'a') && (c <= 'f')) {
    return c - 'a' + 10;
  } else if ((c >= 'A') && (c <= 'F')) {
    return c - 'A' + 10;
  } else {
    return 256; // make it always larger than 255
  }
}

static int decode_escaped(const char *p, string *out)
{
  char buff[256];
  char* ptr = &buff[0];
  char* max = &buff[252];
  const char *orig_p = p;
  while (*p && *p != '!') {
    if (*p == '#' || *p == '~') {
      unsigned hex = 0;
      p++;
      hex = h2i(*p++) << 4;
      if (hex > 255) {
        return -EINVAL;
      }
      hex |= h2i(*p++);
      if (hex > 255) {
        return -EINVAL;
      }
      *ptr++ = hex;
    } else {
      *ptr++ = *p++;
    }
    if (ptr > max) {
       out->append(buff, ptr-buff);
       ptr = &buff[0];
    }
  }
  if (ptr != buff) {
     out->append(buff, ptr-buff);
  }
  return p - orig_p;
}

// some things we encode in binary (as le32 or le64); print the
// resulting key strings nicely
template<typename S>
static string pretty_binary_string(const S& in)
{
  char buf[10];
  string out;
  out.reserve(in.length() * 3);
  enum { NONE, HEX, STRING } mode = NONE;
  unsigned from = 0, i;
  for (i=0; i < in.length(); ++i) {
    if ((in[i] < 32 || (unsigned char)in[i] > 126) ||
	(mode == HEX && in.length() - i >= 4 &&
	 ((in[i] < 32 || (unsigned char)in[i] > 126) ||
	  (in[i+1] < 32 || (unsigned char)in[i+1] > 126) ||
	  (in[i+2] < 32 || (unsigned char)in[i+2] > 126) ||
	  (in[i+3] < 32 || (unsigned char)in[i+3] > 126)))) {
      if (mode == STRING) {
	out.append(in.c_str() + from, i - from);
	out.push_back('\'');
      }
      if (mode != HEX) {
	out.append("0x");
	mode = HEX;
      }
      if (in.length() - i >= 4) {
	// print a whole u32 at once
	snprintf(buf, sizeof(buf), "%08x",
		 (uint32_t)(((unsigned char)in[i] << 24) |
			    ((unsigned char)in[i+1] << 16) |
			    ((unsigned char)in[i+2] << 8) |
			    ((unsigned char)in[i+3] << 0)));
	i += 3;
      } else {
	snprintf(buf, sizeof(buf), "%02x", (int)(unsigned char)in[i]);
      }
      out.append(buf);
    } else {
      if (mode != STRING) {
	out.push_back('\'');
	mode = STRING;
	from = i;
      }
    }
  }
  if (mode == STRING) {
    out.append(in.c_str() + from, i - from);
    out.push_back('\'');
  }
  return out;
}

template<typename T>
static void _key_encode_shard(shard_id_t shard, T *key)
{
  key->push_back((char)((uint8_t)shard.id + (uint8_t)0x80));
}

static const char *_key_decode_shard(const char *key, shard_id_t *pshard)
{
  pshard->id = (uint8_t)*key - (uint8_t)0x80;
  return key + 1;
}

static void get_coll_key_range(const coll_t& cid, int bits,
			       string *temp_start, string *temp_end,
			       string *start, string *end)
{
  temp_start->clear();
  temp_end->clear();
  start->clear();
  end->clear();

  spg_t pgid;
  if (cid.is_pg(&pgid)) {
    _key_encode_shard(pgid.shard, start);
    *temp_start = *start;

    _key_encode_u64(pgid.pool() + 0x8000000000000000ull, start);
    _key_encode_u64((-2ll - pgid.pool()) + 0x8000000000000000ull, temp_start);

    *end = *start;
    *temp_end = *temp_start;

    uint32_t reverse_hash = hobject_t::_reverse_bits(pgid.ps());
    _key_encode_u32(reverse_hash, start);
    _key_encode_u32(reverse_hash, temp_start);

    uint64_t end_hash = reverse_hash  + (1ull << (32 - bits));
    if (end_hash > 0xffffffffull)
      end_hash = 0xffffffffull;

    _key_encode_u32(end_hash, end);
    _key_encode_u32(end_hash, temp_end);
  } else {
    _key_encode_shard(shard_id_t::NO_SHARD, start);
    _key_encode_u64(-1ull + 0x8000000000000000ull, start);
    *end = *start;
    _key_encode_u32(0, start);
    _key_encode_u32(0xffffffff, end);

    // no separate temp section
    *temp_start = *end;
    *temp_end = *end;
  }
}

static void get_shared_blob_key(uint64_t sbid, string *key)
{
  key->clear();
  _key_encode_u64(sbid, key);
}

static int get_key_shared_blob(const string& key, uint64_t *sbid)
{
  const char *p = key.c_str();
  if (key.length() < sizeof(uint64_t))
    return -1;
  _key_decode_u64(p, sbid);
  return 0;
}

template<typename S>
static int get_key_object(const S& key, ghobject_t *oid)
{
  int r;
  const char *p = key.c_str();

  if (key.length() < 1 + 8 + 4)
    return -1;
  p = _key_decode_shard(p, &oid->shard_id);

  uint64_t pool;
  p = _key_decode_u64(p, &pool);
  oid->hobj.pool = pool - 0x8000000000000000ull;

  unsigned hash;
  p = _key_decode_u32(p, &hash);

  oid->hobj.set_bitwise_key_u32(hash);

  r = decode_escaped(p, &oid->hobj.nspace);
  if (r < 0)
    return -2;
  p += r + 1;

  string k;
  r = decode_escaped(p, &k);
  if (r < 0)
    return -3;
  p += r + 1;
  if (*p == '=') {
    // no key
    ++p;
    oid->hobj.oid.name = k;
  } else if (*p == '<' || *p == '>') {
    // key + name
    ++p;
    r = decode_escaped(p, &oid->hobj.oid.name);
    if (r < 0)
      return -5;
    p += r + 1;
    oid->hobj.set_key(k);
  } else {
    // malformed
    return -6;
  }

  p = _key_decode_u64(p, &oid->hobj.snap.val);
  p = _key_decode_u64(p, &oid->generation);

  if (*p != ONODE_KEY_SUFFIX) {
    return -7;
  }
  p++;
  if (*p) {
    // if we get something other than a null terminator here,
    // something goes wrong.
    return -8;
  }

  return 0;
}

template<typename S>
static void get_object_key(CephContext *cct, const ghobject_t& oid, S *key)
{
  key->clear();

  size_t max_len = 1 + 8 + 4 +
                  (oid.hobj.nspace.length() * 3 + 1) +
                  (oid.hobj.get_key().length() * 3 + 1) +
                   1 + // for '<', '=', or '>'
                  (oid.hobj.oid.name.length() * 3 + 1) +
                   8 + 8 + 1;
  key->reserve(max_len);

  _key_encode_shard(oid.shard_id, key);
  _key_encode_u64(oid.hobj.pool + 0x8000000000000000ull, key);
  _key_encode_u32(oid.hobj.get_bitwise_key_u32(), key);

  append_escaped(oid.hobj.nspace, key);

  if (oid.hobj.get_key().length()) {
    // is a key... could be < = or >.
    append_escaped(oid.hobj.get_key(), key);
    // (ASCII chars < = and > sort in that order, yay)
    int r = oid.hobj.get_key().compare(oid.hobj.oid.name);
    if (r) {
      key->append(r > 0 ? ">" : "<");
      append_escaped(oid.hobj.oid.name, key);
    } else {
      // same as no key
      key->append("=");
    }
  } else {
    // no key
    append_escaped(oid.hobj.oid.name, key);
    key->append("=");
  }

  _key_encode_u64(oid.hobj.snap, key);
  _key_encode_u64(oid.generation, key);

  key->push_back(ONODE_KEY_SUFFIX);

  // sanity check
  if (true) {
    ghobject_t t;
    int r = get_key_object(*key, &t);
    if (r || t != oid) {
      derr << "  r " << r << dendl;
      derr << "key " << pretty_binary_string(*key) << dendl;
      derr << "oid " << oid << dendl;
      derr << "  t " << t << dendl;
      assert(r == 0 && t == oid);
    }
  }
}


// extent shard keys are the onode key, plus a u32, plus 'x'.  the trailing
// char lets us quickly test whether it is a shard key without decoding any
// of the prefix bytes.
template<typename S>
static void get_extent_shard_key(const S& onode_key, uint32_t offset,
				 string *key)
{
  key->clear();
  key->reserve(onode_key.length() + 4 + 1);
  key->append(onode_key.c_str(), onode_key.size());
  _key_encode_u32(offset, key);
  key->push_back(EXTENT_SHARD_KEY_SUFFIX);
}

static void rewrite_extent_shard_key(uint32_t offset, string *key)
{
  assert(key->size() > sizeof(uint32_t) + 1);
  assert(*key->rbegin() == EXTENT_SHARD_KEY_SUFFIX);
  _key_encode_u32(offset, key->size() - sizeof(uint32_t) - 1, key);
}

template<typename S>
static void generate_extent_shard_key_and_apply(
  const S& onode_key,
  uint32_t offset,
  string *key,
  std::function<void(const string& final_key)> apply)
{
  if (key->empty()) { // make full key
    assert(!onode_key.empty());
    get_extent_shard_key(onode_key, offset, key);
  } else {
    rewrite_extent_shard_key(offset, key);
  }
  apply(*key);
}

int get_key_extent_shard(const string& key, string *onode_key, uint32_t *offset)
{
  assert(key.size() > sizeof(uint32_t) + 1);
  assert(*key.rbegin() == EXTENT_SHARD_KEY_SUFFIX);
  int okey_len = key.size() - sizeof(uint32_t) - 1;
  *onode_key = key.substr(0, okey_len);
  const char *p = key.data() + okey_len;
  _key_decode_u32(p, offset);
  return 0;
}

static bool is_extent_shard_key(const string& key)
{
  return *key.rbegin() == EXTENT_SHARD_KEY_SUFFIX;
}

// '-' < '.' < '~'
static void get_omap_header(uint64_t id, string *out)
{
  _key_encode_u64(id, out);
  out->push_back('-');
}

// hmm, I don't think there's any need to escape the user key since we
// have a clean prefix.
static void get_omap_key(uint64_t id, const string& key, string *out)
{
  _key_encode_u64(id, out);
  out->push_back('.');
  out->append(key);
}

static void rewrite_omap_key(uint64_t id, string old, string *out)
{
  _key_encode_u64(id, out);
  out->append(old.c_str() + out->length(), old.size() - out->length());
}

static void decode_omap_key(const string& key, string *user_key)
{
  *user_key = key.substr(sizeof(uint64_t) + 1);
}

static void get_omap_tail(uint64_t id, string *out)
{
  _key_encode_u64(id, out);
  out->push_back('~');
}

static void get_deferred_key(uint64_t seq, string *out)
{
  _key_encode_u64(seq, out);
}


// merge operators

struct Int64ArrayMergeOperator : public KeyValueDB::MergeOperator {
  void merge_nonexistent(
    const char *rdata, size_t rlen, std::string *new_value) override {
    *new_value = std::string(rdata, rlen);
  }
  void merge(
    const char *ldata, size_t llen,
    const char *rdata, size_t rlen,
    std::string *new_value) override {
    assert(llen == rlen);
    assert((rlen % 8) == 0);
    new_value->resize(rlen);
    const __le64* lv = (const __le64*)ldata;
    const __le64* rv = (const __le64*)rdata;
    __le64* nv = &(__le64&)new_value->at(0);
    for (size_t i = 0; i < rlen >> 3; ++i) {
      nv[i] = lv[i] + rv[i];
    }
  }
  // We use each operator name and each prefix to construct the
  // overall RocksDB operator name for consistency check at open time.
  string name() const override {
    return "int64_array";
  }
};

// Buffer

static ostream& operator<<(ostream& out, const BlueStore::Buffer& b)
{
  out << "buffer(" << &b << " space " << b.space << " 0x" << std::hex
      << b.offset << "~" << b.length << std::dec
      << " " << BlueStore::Buffer::get_state_name(b.state);
  if (b.flags)
    out << " " << BlueStore::Buffer::get_flag_name(b.flags);
  return out << ")";
}
// =====================================

#undef dout_prefix
#define dout_prefix *_dout << "bluestore(" << path << ") "


static void aio_cb(void *priv, void *priv2)
{
  BlueStore *store = static_cast<BlueStore*>(priv);
  BlueStore::AioContext *c = static_cast<BlueStore::AioContext*>(priv2);
  c->aio_finish(store);
}

BlueStore::BlueStore(CephContext *cct, const string& path)
  : ObjectStore(cct, path),
    throttle_bytes(cct, "bluestore_throttle_bytes",
		   cct->_conf->bluestore_throttle_bytes),
    throttle_deferred_bytes(cct, "bluestore_throttle_deferred_bytes",
		       cct->_conf->bluestore_throttle_bytes +
		       cct->_conf->bluestore_throttle_deferred_bytes),
    deferred_finisher(cct, "defered_finisher", "dfin"),
    kv_sync_thread(this),
    kv_finalize_thread(this),
    mempool_thread(this)
{
  _init_logger();
  cct->_conf->add_observer(this);
  set_cache_shards(1);
}

BlueStore::BlueStore(CephContext *cct,
  const string& path,
  uint64_t _min_alloc_size)
  : ObjectStore(cct, path),
    throttle_bytes(cct, "bluestore_throttle_bytes",
		   cct->_conf->bluestore_throttle_bytes),
    throttle_deferred_bytes(cct, "bluestore_throttle_deferred_bytes",
		       cct->_conf->bluestore_throttle_bytes +
		       cct->_conf->bluestore_throttle_deferred_bytes),
    deferred_finisher(cct, "defered_finisher", "dfin"),
    kv_sync_thread(this),
    kv_finalize_thread(this),
    min_alloc_size(_min_alloc_size),
    min_alloc_size_order(ctz(_min_alloc_size)),
    mempool_thread(this)
{
  _init_logger();
  cct->_conf->add_observer(this);
  set_cache_shards(1);
}

BlueStore::~BlueStore()
{
  for (auto f : finishers) {
    delete f;
  }
  finishers.clear();

  cct->_conf->remove_observer(this);
  _shutdown_logger();
  assert(!mounted);
  assert(db == NULL);
  assert(bluefs == NULL);
  assert(fsid_fd < 0);
  assert(path_fd < 0);
  for (auto i : cache_shards) {
    delete i;
  }
  cache_shards.clear();
}

const char **BlueStore::get_tracked_conf_keys() const
{
  static const char* KEYS[] = {
    "bluestore_csum_type",
    "bluestore_compression_mode",
    "bluestore_compression_algorithm",
    "bluestore_compression_min_blob_size",
    "bluestore_compression_min_blob_size_ssd",
    "bluestore_compression_min_blob_size_hdd",
    "bluestore_compression_max_blob_size",
    "bluestore_compression_max_blob_size_ssd",
    "bluestore_compression_max_blob_size_hdd",
    "bluestore_compression_required_ratio",
    "bluestore_max_alloc_size",
    "bluestore_prefer_deferred_size",
    "bluestore_prefer_deferred_size_hdd",
    "bluestore_prefer_deferred_size_ssd",
    "bluestore_deferred_batch_ops",
    "bluestore_deferred_batch_ops_hdd",
    "bluestore_deferred_batch_ops_ssd",
    "bluestore_throttle_bytes",
    "bluestore_throttle_deferred_bytes",
    "bluestore_throttle_cost_per_io_hdd",
    "bluestore_throttle_cost_per_io_ssd",
    "bluestore_throttle_cost_per_io",
    "bluestore_max_blob_size",
    "bluestore_max_blob_size_ssd",
    "bluestore_max_blob_size_hdd",
    NULL
  };
  return KEYS;
}

void BlueStore::handle_conf_change(const struct md_config_t *conf,
				   const std::set<std::string> &changed)
{
  if (changed.count("bluestore_csum_type")) {
    _set_csum();
  }
  if (changed.count("bluestore_compression_mode") ||
      changed.count("bluestore_compression_algorithm") ||
      changed.count("bluestore_compression_min_blob_size") ||
      changed.count("bluestore_compression_max_blob_size")) {
    if (bdev) {
      _set_compression();
    }
  }
  if (changed.count("bluestore_max_blob_size") ||
      changed.count("bluestore_max_blob_size_ssd") ||
      changed.count("bluestore_max_blob_size_hdd")) {
    if (bdev) {
      // only after startup
      _set_blob_size();
    }
  }
  if (changed.count("bluestore_prefer_deferred_size") ||
      changed.count("bluestore_prefer_deferred_size_hdd") ||
      changed.count("bluestore_prefer_deferred_size_ssd") ||
      changed.count("bluestore_max_alloc_size") ||
      changed.count("bluestore_deferred_batch_ops") ||
      changed.count("bluestore_deferred_batch_ops_hdd") ||
      changed.count("bluestore_deferred_batch_ops_ssd")) {
    if (bdev) {
      // only after startup
      _set_alloc_sizes();
    }
  }
  if (changed.count("bluestore_throttle_cost_per_io") ||
      changed.count("bluestore_throttle_cost_per_io_hdd") ||
      changed.count("bluestore_throttle_cost_per_io_ssd")) {
    if (bdev) {
      _set_throttle_params();
    }
  }
  if (changed.count("bluestore_throttle_bytes")) {
    throttle_bytes.reset_max(conf->bluestore_throttle_bytes);
    throttle_deferred_bytes.reset_max(
      conf->bluestore_throttle_bytes + conf->bluestore_throttle_deferred_bytes);
  }
  if (changed.count("bluestore_throttle_deferred_bytes")) {
    throttle_deferred_bytes.reset_max(
      conf->bluestore_throttle_bytes + conf->bluestore_throttle_deferred_bytes);
  }
}

void BlueStore::_set_compression()
{
  auto m = Compressor::get_comp_mode_type(cct->_conf->bluestore_compression_mode);
  if (m) {
    comp_mode = *m;
  } else {
    derr << __func__ << " unrecognized value '"
         << cct->_conf->bluestore_compression_mode
         << "' for bluestore_compression_mode, reverting to 'none'"
         << dendl;
    comp_mode = Compressor::COMP_NONE;
  }

  compressor = nullptr;

  if (comp_mode == Compressor::COMP_NONE) {
    dout(10) << __func__ << " compression mode set to 'none', "
             << "ignore other compression setttings" << dendl;
    return;
  }

  if (cct->_conf->bluestore_compression_min_blob_size) {
    comp_min_blob_size = cct->_conf->bluestore_compression_min_blob_size;
  } else {
    assert(bdev);
    if (bdev->is_rotational()) {
      comp_min_blob_size = cct->_conf->bluestore_compression_min_blob_size_hdd;
    } else {
      comp_min_blob_size = cct->_conf->bluestore_compression_min_blob_size_ssd;
    }
  }

  if (cct->_conf->bluestore_compression_max_blob_size) {
    comp_max_blob_size = cct->_conf->bluestore_compression_max_blob_size;
  } else {
    assert(bdev);
    if (bdev->is_rotational()) {
      comp_max_blob_size = cct->_conf->bluestore_compression_max_blob_size_hdd;
    } else {
      comp_max_blob_size = cct->_conf->bluestore_compression_max_blob_size_ssd;
    }
  }

  auto& alg_name = cct->_conf->bluestore_compression_algorithm;
  if (!alg_name.empty()) {
    compressor = Compressor::create(cct, alg_name);
    if (!compressor) {
      derr << __func__ << " unable to initialize " << alg_name.c_str() << " compressor"
           << dendl;
    }
  }
 
  dout(10) << __func__ << " mode " << Compressor::get_comp_mode_name(comp_mode)
	   << " alg " << (compressor ? compressor->get_type_name() : "(none)")
	   << dendl;
}

void BlueStore::_set_csum()
{
  csum_type = Checksummer::CSUM_NONE;
  int t = Checksummer::get_csum_string_type(cct->_conf->bluestore_csum_type);
  if (t > Checksummer::CSUM_NONE)
    csum_type = t;

  dout(10) << __func__ << " csum_type "
	   << Checksummer::get_csum_type_string(csum_type)
	   << dendl;
}

void BlueStore::_set_throttle_params()
{
  if (cct->_conf->bluestore_throttle_cost_per_io) {
    throttle_cost_per_io = cct->_conf->bluestore_throttle_cost_per_io;
  } else {
    assert(bdev);
    if (bdev->is_rotational()) {
      throttle_cost_per_io = cct->_conf->bluestore_throttle_cost_per_io_hdd;
    } else {
      throttle_cost_per_io = cct->_conf->bluestore_throttle_cost_per_io_ssd;
    }
  }

  dout(10) << __func__ << " throttle_cost_per_io " << throttle_cost_per_io
	   << dendl;
}
void BlueStore::_set_blob_size()
{
  if (cct->_conf->bluestore_max_blob_size) {
    max_blob_size = cct->_conf->bluestore_max_blob_size;
  } else {
    assert(bdev);
    if (bdev->is_rotational()) {
      max_blob_size = cct->_conf->bluestore_max_blob_size_hdd;
    } else {
      max_blob_size = cct->_conf->bluestore_max_blob_size_ssd;
    }
  }
  dout(10) << __func__ << " max_blob_size 0x" << std::hex << max_blob_size
           << std::dec << dendl;
}

int BlueStore::_set_cache_sizes()
{
  assert(bdev);
  if (cct->_conf->bluestore_cache_size) {
    cache_size = cct->_conf->bluestore_cache_size;
  } else {
    // choose global cache size based on backend type
    if (bdev->is_rotational()) {
      cache_size = cct->_conf->bluestore_cache_size_hdd;
    } else {
      cache_size = cct->_conf->bluestore_cache_size_ssd;
    }
  }
  cache_meta_ratio = cct->_conf->bluestore_cache_meta_ratio;
  cache_kv_ratio = cct->_conf->bluestore_cache_kv_ratio;

  double cache_kv_max = cct->_conf->bluestore_cache_kv_max;
  double cache_kv_max_ratio = 0;

  // if cache_kv_max is negative, disable it
  if (cache_size > 0 && cache_kv_max >= 0) {
    cache_kv_max_ratio = (double) cache_kv_max / (double) cache_size;
    if (cache_kv_max_ratio < 1.0 && cache_kv_max_ratio < cache_kv_ratio) {
      dout(1) << __func__ << " max " << cache_kv_max_ratio
            << " < ratio " << cache_kv_ratio
            << dendl;
      cache_meta_ratio = cache_meta_ratio + cache_kv_ratio - cache_kv_max_ratio;
      cache_kv_ratio = cache_kv_max_ratio;
    }
  }  

  cache_data_ratio =
    (double)1.0 - (double)cache_meta_ratio - (double)cache_kv_ratio;

  if (cache_meta_ratio < 0 || cache_meta_ratio > 1.0) {
    derr << __func__ << " bluestore_cache_meta_ratio (" << cache_meta_ratio
	 << ") must be in range [0,1.0]" << dendl;
    return -EINVAL;
  }
  if (cache_kv_ratio < 0 || cache_kv_ratio > 1.0) {
    derr << __func__ << " bluestore_cache_kv_ratio (" << cache_kv_ratio
	 << ") must be in range [0,1.0]" << dendl;
    return -EINVAL;
  }
  if (cache_meta_ratio + cache_kv_ratio > 1.0) {
    derr << __func__ << " bluestore_cache_meta_ratio (" << cache_meta_ratio
	 << ") + bluestore_cache_kv_ratio (" << cache_kv_ratio
	 << ") = " << cache_meta_ratio + cache_kv_ratio << "; must be <= 1.0"
	 << dendl;
    return -EINVAL;
  }
  if (cache_data_ratio < 0) {
    // deal with floating point imprecision
    cache_data_ratio = 0;
  }
  dout(1) << __func__ << " cache_size " << cache_size
          << " meta " << cache_meta_ratio
	  << " kv " << cache_kv_ratio
	  << " data " << cache_data_ratio
	  << dendl;
  return 0;
}

int BlueStore::write_meta(const std::string& key, const std::string& value)
{
  bluestore_bdev_label_t label;
  string p = path + "/block";
  int r = _read_bdev_label(cct, p, &label);
  if (r < 0) {
    return ObjectStore::write_meta(key, value);
  }
  label.meta[key] = value;
  r = _write_bdev_label(cct, p, label);
  assert(r == 0);
  return ObjectStore::write_meta(key, value);
}

int BlueStore::read_meta(const std::string& key, std::string *value)
{
  bluestore_bdev_label_t label;
  string p = path + "/block";
  int r = _read_bdev_label(cct, p, &label);
  if (r < 0) {
    return ObjectStore::read_meta(key, value);
  }
  auto i = label.meta.find(key);
  if (i == label.meta.end()) {
    return ObjectStore::read_meta(key, value);
  }
  *value = i->second;
  return 0;
}

void BlueStore::_init_logger()
{
  PerfCountersBuilder b(cct, "bluestore",
                        l_bluestore_first, l_bluestore_last);
  b.add_time_avg(l_bluestore_kv_flush_lat, "kv_flush_lat",
		 "Average kv_thread flush latency",
		 "fl_l", PerfCountersBuilder::PRIO_INTERESTING);
  b.add_time_avg(l_bluestore_kv_commit_lat, "kv_commit_lat",
		 "Average kv_thread commit latency");
  b.add_time_avg(l_bluestore_kv_lat, "kv_lat",
		 "Average kv_thread sync latency",
		 "k_l", PerfCountersBuilder::PRIO_INTERESTING);
  b.add_time_avg(l_bluestore_state_prepare_lat, "state_prepare_lat",
    "Average prepare state latency");
  b.add_time_avg(l_bluestore_state_aio_wait_lat, "state_aio_wait_lat",
		 "Average aio_wait state latency",
		 "io_l", PerfCountersBuilder::PRIO_INTERESTING);
  b.add_time_avg(l_bluestore_state_io_done_lat, "state_io_done_lat",
    "Average io_done state latency");
  b.add_time_avg(l_bluestore_state_kv_queued_lat, "state_kv_queued_lat",
    "Average kv_queued state latency");
  b.add_time_avg(l_bluestore_state_kv_committing_lat, "state_kv_commiting_lat",
    "Average kv_commiting state latency");
  b.add_time_avg(l_bluestore_state_kv_done_lat, "state_kv_done_lat",
    "Average kv_done state latency");
  b.add_time_avg(l_bluestore_state_deferred_queued_lat, "state_deferred_queued_lat",
    "Average deferred_queued state latency");
  b.add_time_avg(l_bluestore_state_deferred_aio_wait_lat, "state_deferred_aio_wait_lat",
    "Average aio_wait state latency");
  b.add_time_avg(l_bluestore_state_deferred_cleanup_lat, "state_deferred_cleanup_lat",
    "Average cleanup state latency");
  b.add_time_avg(l_bluestore_state_finishing_lat, "state_finishing_lat",
    "Average finishing state latency");
  b.add_time_avg(l_bluestore_state_done_lat, "state_done_lat",
    "Average done state latency");
  b.add_time_avg(l_bluestore_throttle_lat, "throttle_lat",
		 "Average submit throttle latency",
		 "th_l", PerfCountersBuilder::PRIO_CRITICAL);
  b.add_time_avg(l_bluestore_submit_lat, "submit_lat",
		 "Average submit latency",
		 "s_l", PerfCountersBuilder::PRIO_CRITICAL);
  b.add_time_avg(l_bluestore_commit_lat, "commit_lat",
		 "Average commit latency",
		 "c_l", PerfCountersBuilder::PRIO_CRITICAL);
  b.add_time_avg(l_bluestore_read_lat, "read_lat",
		 "Average read latency",
		 "r_l", PerfCountersBuilder::PRIO_CRITICAL);
  b.add_time_avg(l_bluestore_read_onode_meta_lat, "read_onode_meta_lat",
    "Average read onode metadata latency");
  b.add_time_avg(l_bluestore_read_wait_aio_lat, "read_wait_aio_lat",
    "Average read latency");
  b.add_time_avg(l_bluestore_compress_lat, "compress_lat",
    "Average compress latency");
  b.add_time_avg(l_bluestore_decompress_lat, "decompress_lat",
    "Average decompress latency");
  b.add_time_avg(l_bluestore_csum_lat, "csum_lat",
    "Average checksum latency");
  b.add_u64_counter(l_bluestore_compress_success_count, "compress_success_count",
    "Sum for beneficial compress ops");
  b.add_u64_counter(l_bluestore_compress_rejected_count, "compress_rejected_count",
    "Sum for compress ops rejected due to low net gain of space");
  b.add_u64_counter(l_bluestore_write_pad_bytes, "write_pad_bytes",
    "Sum for write-op padded bytes");
  b.add_u64_counter(l_bluestore_deferred_write_ops, "deferred_write_ops",
		    "Sum for deferred write op");
  b.add_u64_counter(l_bluestore_deferred_write_bytes, "deferred_write_bytes",
		    "Sum for deferred write bytes", "def");
  b.add_u64_counter(l_bluestore_write_penalty_read_ops, "write_penalty_read_ops",
		    "Sum for write penalty read ops");
  b.add_u64(l_bluestore_allocated, "bluestore_allocated",
    "Sum for allocated bytes");
  b.add_u64(l_bluestore_stored, "bluestore_stored",
    "Sum for stored bytes");
  b.add_u64(l_bluestore_compressed, "bluestore_compressed",
    "Sum for stored compressed bytes");
  b.add_u64(l_bluestore_compressed_allocated, "bluestore_compressed_allocated",
    "Sum for bytes allocated for compressed data");
  b.add_u64(l_bluestore_compressed_original, "bluestore_compressed_original",
    "Sum for original bytes that were compressed");

  b.add_u64(l_bluestore_onodes, "bluestore_onodes",
	    "Number of onodes in cache");
  b.add_u64_counter(l_bluestore_onode_hits, "bluestore_onode_hits",
		    "Sum for onode-lookups hit in the cache");
  b.add_u64_counter(l_bluestore_onode_misses, "bluestore_onode_misses",
		    "Sum for onode-lookups missed in the cache");
  b.add_u64_counter(l_bluestore_onode_shard_hits, "bluestore_onode_shard_hits",
		    "Sum for onode-shard lookups hit in the cache");
  b.add_u64_counter(l_bluestore_onode_shard_misses,
		    "bluestore_onode_shard_misses",
		    "Sum for onode-shard lookups missed in the cache");
  b.add_u64(l_bluestore_extents, "bluestore_extents",
	    "Number of extents in cache");
  b.add_u64(l_bluestore_blobs, "bluestore_blobs",
	    "Number of blobs in cache");
  b.add_u64(l_bluestore_buffers, "bluestore_buffers",
	    "Number of buffers in cache");
  b.add_u64(l_bluestore_buffer_bytes, "bluestore_buffer_bytes",
	    "Number of buffer bytes in cache");
  b.add_u64(l_bluestore_buffer_hit_bytes, "bluestore_buffer_hit_bytes",
    "Sum for bytes of read hit in the cache");
  b.add_u64(l_bluestore_buffer_miss_bytes, "bluestore_buffer_miss_bytes",
    "Sum for bytes of read missed in the cache");

  b.add_u64_counter(l_bluestore_write_big, "bluestore_write_big",
		    "Large aligned writes into fresh blobs");
  b.add_u64_counter(l_bluestore_write_big_bytes, "bluestore_write_big_bytes",
		    "Large aligned writes into fresh blobs (bytes)");
  b.add_u64_counter(l_bluestore_write_big_blobs, "bluestore_write_big_blobs",
		    "Large aligned writes into fresh blobs (blobs)");
  b.add_u64_counter(l_bluestore_write_small, "bluestore_write_small",
		    "Small writes into existing or sparse small blobs");
  b.add_u64_counter(l_bluestore_write_small_bytes, "bluestore_write_small_bytes",
		    "Small writes into existing or sparse small blobs (bytes)");
  b.add_u64_counter(l_bluestore_write_small_unused,
		    "bluestore_write_small_unused",
		    "Small writes into unused portion of existing blob");
  b.add_u64_counter(l_bluestore_write_small_deferred,
		    "bluestore_write_small_deferred",
		    "Small overwrites using deferred");
  b.add_u64_counter(l_bluestore_write_small_pre_read,
		    "bluestore_write_small_pre_read",
		    "Small writes that required we read some data (possibly "
		    "cached) to fill out the block");
  b.add_u64_counter(l_bluestore_write_small_new, "bluestore_write_small_new",
		    "Small write into new (sparse) blob");

  b.add_u64_counter(l_bluestore_txc, "bluestore_txc", "Transactions committed");
  b.add_u64_counter(l_bluestore_onode_reshard, "bluestore_onode_reshard",
		    "Onode extent map reshard events");
  b.add_u64_counter(l_bluestore_blob_split, "bluestore_blob_split",
		    "Sum for blob splitting due to resharding");
  b.add_u64_counter(l_bluestore_extent_compress, "bluestore_extent_compress",
		    "Sum for extents that have been removed due to compression");
  b.add_u64_counter(l_bluestore_gc_merged, "bluestore_gc_merged",
		    "Sum for extents that have been merged due to garbage "
		    "collection");
  b.add_u64_counter(l_bluestore_read_eio, "bluestore_read_eio",
                    "Read EIO errors propagated to high level callers");
  logger = b.create_perf_counters();
  cct->get_perfcounters_collection()->add(logger);
}

int BlueStore::_reload_logger()
{
  struct store_statfs_t store_statfs;

  int r = statfs(&store_statfs);
  if(r >= 0) {
    logger->set(l_bluestore_allocated, store_statfs.allocated);
    logger->set(l_bluestore_stored, store_statfs.stored);
    logger->set(l_bluestore_compressed, store_statfs.compressed);
    logger->set(l_bluestore_compressed_allocated, store_statfs.compressed_allocated);
    logger->set(l_bluestore_compressed_original, store_statfs.compressed_original);
  }
  return r;
}

void BlueStore::_shutdown_logger()
{
  cct->get_perfcounters_collection()->remove(logger);
  delete logger;
}

int BlueStore::get_block_device_fsid(CephContext* cct, const string& path,
				     uuid_d *fsid)
{
  bluestore_bdev_label_t label;
  int r = _read_bdev_label(cct, path, &label);
  if (r < 0)
    return r;
  *fsid = label.osd_uuid;
  return 0;
}

int BlueStore::_open_path()
{
  // sanity check(s)
  if (cct->_conf->get_val<uint64_t>("osd_max_object_size") >=
      4*1024*1024*1024ull) {
    derr << __func__ << " osd_max_object_size >= 4GB; BlueStore has hard limit of 4GB." << dendl;
    return -EINVAL;
  }
  assert(path_fd < 0);
  path_fd = TEMP_FAILURE_RETRY(::open(path.c_str(), O_DIRECTORY));
  if (path_fd < 0) {
    int r = -errno;
    derr << __func__ << " unable to open " << path << ": " << cpp_strerror(r)
	 << dendl;
    return r;
  }
  return 0;
}

void BlueStore::_close_path()
{
  VOID_TEMP_FAILURE_RETRY(::close(path_fd));
  path_fd = -1;
}

int BlueStore::_write_bdev_label(CephContext *cct,
				 string path, bluestore_bdev_label_t label)
{
  dout(10) << __func__ << " path " << path << " label " << label << dendl;
  bufferlist bl;
  ::encode(label, bl);
  uint32_t crc = bl.crc32c(-1);
  ::encode(crc, bl);
  assert(bl.length() <= BDEV_LABEL_BLOCK_SIZE);
  bufferptr z(BDEV_LABEL_BLOCK_SIZE - bl.length());
  z.zero();
  bl.append(std::move(z));

  int fd = TEMP_FAILURE_RETRY(::open(path.c_str(), O_WRONLY));
  if (fd < 0) {
    fd = -errno;
    derr << __func__ << " failed to open " << path << ": " << cpp_strerror(fd)
	 << dendl;
    return fd;
  }
  int r = bl.write_fd(fd);
  if (r < 0) {
    derr << __func__ << " failed to write to " << path
	 << ": " << cpp_strerror(r) << dendl;
  }
  r = ::fsync(fd);
  if (r < 0) {
    derr << __func__ << " failed to fsync " << path
	 << ": " << cpp_strerror(r) << dendl;
  }
  VOID_TEMP_FAILURE_RETRY(::close(fd));
  return r;
}

int BlueStore::_read_bdev_label(CephContext* cct, string path,
				bluestore_bdev_label_t *label)
{
  dout(10) << __func__ << dendl;
  int fd = TEMP_FAILURE_RETRY(::open(path.c_str(), O_RDONLY));
  if (fd < 0) {
    fd = -errno;
    derr << __func__ << " failed to open " << path << ": " << cpp_strerror(fd)
	 << dendl;
    return fd;
  }
  bufferlist bl;
  int r = bl.read_fd(fd, BDEV_LABEL_BLOCK_SIZE);
  VOID_TEMP_FAILURE_RETRY(::close(fd));
  if (r < 0) {
    derr << __func__ << " failed to read from " << path
	 << ": " << cpp_strerror(r) << dendl;
    return r;
  }

  uint32_t crc, expected_crc;
  bufferlist::iterator p = bl.begin();
  try {
    ::decode(*label, p);
    bufferlist t;
    t.substr_of(bl, 0, p.get_off());
    crc = t.crc32c(-1);
    ::decode(expected_crc, p);
  }
  catch (buffer::error& e) {
    dout(2) << __func__ << " unable to decode label at offset " << p.get_off()
	 << ": " << e.what()
	 << dendl;
    return -ENOENT;
  }
  if (crc != expected_crc) {
    derr << __func__ << " bad crc on label, expected " << expected_crc
	 << " != actual " << crc << dendl;
    return -EIO;
  }
  dout(10) << __func__ << " got " << *label << dendl;
  return 0;
}

int BlueStore::_check_or_set_bdev_label(
  string path, uint64_t size, string desc, bool create)
{
  bluestore_bdev_label_t label;
  if (create) {
    label.osd_uuid = fsid;
    label.size = size;
    label.btime = ceph_clock_now();
    label.description = desc;
    int r = _write_bdev_label(cct, path, label);
    if (r < 0)
      return r;
  } else {
    int r = _read_bdev_label(cct, path, &label);
    if (r < 0)
      return r;
    if (cct->_conf->bluestore_debug_permit_any_bdev_label) {
      dout(20) << __func__ << " bdev " << path << " fsid " << label.osd_uuid
	   << " and fsid " << fsid << " check bypassed" << dendl;
    }
    else if (label.osd_uuid != fsid) {
      derr << __func__ << " bdev " << path << " fsid " << label.osd_uuid
	   << " does not match our fsid " << fsid << dendl;
      return -EIO;
    }
  }
  return 0;
}

void BlueStore::_set_alloc_sizes(void)
{
  max_alloc_size = cct->_conf->bluestore_max_alloc_size;

  if (cct->_conf->bluestore_prefer_deferred_size) {
    prefer_deferred_size = cct->_conf->bluestore_prefer_deferred_size;
  } else {
    assert(bdev);
    if (bdev->is_rotational()) {
      prefer_deferred_size = cct->_conf->bluestore_prefer_deferred_size_hdd;
    } else {
      prefer_deferred_size = cct->_conf->bluestore_prefer_deferred_size_ssd;
    }
  }

  if (cct->_conf->bluestore_deferred_batch_ops) {
    deferred_batch_ops = cct->_conf->bluestore_deferred_batch_ops;
  } else {
    assert(bdev);
    if (bdev->is_rotational()) {
      deferred_batch_ops = cct->_conf->bluestore_deferred_batch_ops_hdd;
    } else {
      deferred_batch_ops = cct->_conf->bluestore_deferred_batch_ops_ssd;
    }
  }

  dout(10) << __func__ << " min_alloc_size 0x" << std::hex << min_alloc_size
	   << std::dec << " order " << min_alloc_size_order
	   << " max_alloc_size 0x" << std::hex << max_alloc_size
	   << " prefer_deferred_size 0x" << prefer_deferred_size
	   << std::dec
	   << " deferred_batch_ops " << deferred_batch_ops
	   << dendl;
}

int BlueStore::_open_bdev(bool create)
{
  assert(bdev == NULL);
  string p = path + "/block";
  bdev = BlockDevice::create(cct, p, aio_cb, static_cast<void*>(this));
  int r = bdev->open(p);
  if (r < 0)
    goto fail;

  if (bdev->supported_bdev_label()) {
    r = _check_or_set_bdev_label(p, bdev->get_size(), "main", create);
    if (r < 0)
      goto fail_close;
  }

  // initialize global block parameters
  block_size = bdev->get_block_size();
  block_mask = ~(block_size - 1);
  block_size_order = ctz(block_size);
  assert(block_size == 1u << block_size_order);
  // and set cache_size based on device type
  r = _set_cache_sizes();
  if (r < 0) {
    goto fail_close;
  }
  return 0;

 fail_close:
  bdev->close();
 fail:
  delete bdev;
  bdev = NULL;
  return r;
}

void BlueStore::_close_bdev()
{
  assert(bdev);
  bdev->close();
  delete bdev;
  bdev = NULL;
}

int BlueStore::_open_fm(bool create)
{
  assert(fm == NULL);
  fm = FreelistManager::create(cct, freelist_type, db, PREFIX_ALLOC);

  if (create) {
    // initialize freespace
    dout(20) << __func__ << " initializing freespace" << dendl;
    KeyValueDB::Transaction t = db->get_transaction();
    {
      bufferlist bl;
      bl.append(freelist_type);
      t->set(PREFIX_SUPER, "freelist_type", bl);
    }
    // being able to allocate in units less than bdev block size 
    // seems to be a bad idea.
    assert( cct->_conf->bdev_block_size <= (int64_t)min_alloc_size);
    fm->create(bdev->get_size(), (int64_t)min_alloc_size, t);

    // allocate superblock reserved space.  note that we do not mark
    // bluefs space as allocated in the freelist; we instead rely on
    // bluefs_extents.
    uint64_t reserved = ROUND_UP_TO(MAX(SUPER_RESERVED, min_alloc_size),
				    min_alloc_size);
    fm->allocate(0, reserved, t);

    if (cct->_conf->bluestore_bluefs) {
      assert(bluefs_extents.num_intervals() == 1);
      interval_set<uint64_t>::iterator p = bluefs_extents.begin();
      reserved = ROUND_UP_TO(p.get_start() + p.get_len(), min_alloc_size);
      dout(20) << __func__ << " reserved 0x" << std::hex << reserved << std::dec
	       << " for bluefs" << dendl;
      bufferlist bl;
      ::encode(bluefs_extents, bl);
      t->set(PREFIX_SUPER, "bluefs_extents", bl);
      dout(20) << __func__ << " bluefs_extents 0x" << std::hex << bluefs_extents
	       << std::dec << dendl;
    }

    if (cct->_conf->bluestore_debug_prefill > 0) {
      uint64_t end = bdev->get_size() - reserved;
      dout(1) << __func__ << " pre-fragmenting freespace, using "
	      << cct->_conf->bluestore_debug_prefill << " with max free extent "
	      << cct->_conf->bluestore_debug_prefragment_max << dendl;
      uint64_t start = P2ROUNDUP(reserved, min_alloc_size);
      uint64_t max_b = cct->_conf->bluestore_debug_prefragment_max / min_alloc_size;
      float r = cct->_conf->bluestore_debug_prefill;
      r /= 1.0 - r;
      bool stop = false;

      while (!stop && start < end) {
	uint64_t l = (rand() % max_b + 1) * min_alloc_size;
	if (start + l > end) {
	  l = end - start;
          l = P2ALIGN(l, min_alloc_size);
        }
        assert(start + l <= end);

	uint64_t u = 1 + (uint64_t)(r * (double)l);
	u = P2ROUNDUP(u, min_alloc_size);
        if (start + l + u > end) {
          u = end - (start + l);
          // trim to align so we don't overflow again
          u = P2ALIGN(u, min_alloc_size);
          stop = true;
        }
        assert(start + l + u <= end);

	dout(20) << "  free 0x" << std::hex << start << "~" << l
		 << " use 0x" << u << std::dec << dendl;

        if (u == 0) {
          // break if u has been trimmed to nothing
          break;
        }

	fm->allocate(start + l, u, t);
	start += l + u;
      }
    }
    db->submit_transaction_sync(t);
  }

  int r = fm->init(bdev->get_size());
  if (r < 0) {
    derr << __func__ << " freelist init failed: " << cpp_strerror(r) << dendl;
    delete fm;
    fm = NULL;
    return r;
  }
  return 0;
}

void BlueStore::_close_fm()
{
  dout(10) << __func__ << dendl;
  assert(fm);
  fm->shutdown();
  delete fm;
  fm = NULL;
}

int BlueStore::_open_alloc()
{
  assert(alloc == NULL);
  assert(bdev->get_size());
  alloc = Allocator::create(cct, cct->_conf->bluestore_allocator,
                            bdev->get_size(),
                            min_alloc_size);
  if (!alloc) {
    lderr(cct) << __func__ << " Allocator::unknown alloc type "
               << cct->_conf->bluestore_allocator
               << dendl;
    return -EINVAL;
  }

  uint64_t num = 0, bytes = 0;

  dout(1) << __func__ << " opening allocation metadata" << dendl;
  // initialize from freelist
  fm->enumerate_reset();
  uint64_t offset, length;
  while (fm->enumerate_next(&offset, &length)) {
    alloc->init_add_free(offset, length);
    ++num;
    bytes += length;
  }
  fm->enumerate_reset();
  dout(1) << __func__ << " loaded " << pretty_si_t(bytes)
	  << " in " << num << " extents"
	  << dendl;

  // also mark bluefs space as allocated
  for (auto e = bluefs_extents.begin(); e != bluefs_extents.end(); ++e) {
    alloc->init_rm_free(e.get_start(), e.get_len());
  }
  dout(10) << __func__ << " marked bluefs_extents 0x" << std::hex
	   << bluefs_extents << std::dec << " as allocated" << dendl;

  return 0;
}

void BlueStore::_close_alloc()
{
  assert(alloc);
  alloc->shutdown();
  delete alloc;
  alloc = NULL;
}

int BlueStore::_open_fsid(bool create)
{
  assert(fsid_fd < 0);
  int flags = O_RDWR;
  if (create)
    flags |= O_CREAT;
  fsid_fd = ::openat(path_fd, "fsid", flags, 0644);
  if (fsid_fd < 0) {
    int err = -errno;
    derr << __func__ << " " << cpp_strerror(err) << dendl;
    return err;
  }
  return 0;
}

int BlueStore::_read_fsid(uuid_d *uuid)
{
  char fsid_str[40];
  memset(fsid_str, 0, sizeof(fsid_str));
  int ret = safe_read(fsid_fd, fsid_str, sizeof(fsid_str));
  if (ret < 0) {
    derr << __func__ << " failed: " << cpp_strerror(ret) << dendl;
    return ret;
  }
  if (ret > 36)
    fsid_str[36] = 0;
  else
    fsid_str[ret] = 0;
  if (!uuid->parse(fsid_str)) {
    derr << __func__ << " unparsable uuid " << fsid_str << dendl;
    return -EINVAL;
  }
  return 0;
}

int BlueStore::_write_fsid()
{
  int r = ::ftruncate(fsid_fd, 0);
  if (r < 0) {
    r = -errno;
    derr << __func__ << " fsid truncate failed: " << cpp_strerror(r) << dendl;
    return r;
  }
  string str = stringify(fsid) + "\n";
  r = safe_write(fsid_fd, str.c_str(), str.length());
  if (r < 0) {
    derr << __func__ << " fsid write failed: " << cpp_strerror(r) << dendl;
    return r;
  }
  r = ::fsync(fsid_fd);
  if (r < 0) {
    r = -errno;
    derr << __func__ << " fsid fsync failed: " << cpp_strerror(r) << dendl;
    return r;
  }
  return 0;
}

void BlueStore::_close_fsid()
{
  VOID_TEMP_FAILURE_RETRY(::close(fsid_fd));
  fsid_fd = -1;
}

int BlueStore::_lock_fsid()
{
  struct flock l;
  memset(&l, 0, sizeof(l));
  l.l_type = F_WRLCK;
  l.l_whence = SEEK_SET;
  int r = ::fcntl(fsid_fd, F_SETLK, &l);
  if (r < 0) {
    int err = errno;
    derr << __func__ << " failed to lock " << path << "/fsid"
	 << " (is another ceph-osd still running?)"
	 << cpp_strerror(err) << dendl;
    return -err;
  }
  return 0;
}

bool BlueStore::is_rotational()
{
  if (bdev) {
    return bdev->is_rotational();
  }

  bool rotational = true;
  int r = _open_path();
  if (r < 0)
    goto out;
  r = _open_fsid(false);
  if (r < 0)
    goto out_path;
  r = _read_fsid(&fsid);
  if (r < 0)
    goto out_fsid;
  r = _lock_fsid();
  if (r < 0)
    goto out_fsid;
  r = _open_bdev(false);
  if (r < 0)
    goto out_fsid;
  rotational = bdev->is_rotational();
  _close_bdev();
 out_fsid:
  _close_fsid();
 out_path:
  _close_path();
  out:
  return rotational;
}

bool BlueStore::is_journal_rotational()
{
  if (!bluefs) {
    dout(5) << __func__ << " bluefs disabled, default to store media type"
            << dendl;
    return is_rotational();
  }
  dout(10) << __func__ << " " << (int)bluefs->wal_is_rotational() << dendl;
  return bluefs->wal_is_rotational();
}

bool BlueStore::test_mount_in_use()
{
  // most error conditions mean the mount is not in use (e.g., because
  // it doesn't exist).  only if we fail to lock do we conclude it is
  // in use.
  bool ret = false;
  int r = _open_path();
  if (r < 0)
    return false;
  r = _open_fsid(false);
  if (r < 0)
    goto out_path;
  r = _lock_fsid();
  if (r < 0)
    ret = true; // if we can't lock, it is in use
  _close_fsid();
 out_path:
  _close_path();
  return ret;
}

int BlueStore::_open_db(bool create)
{
  int r;
  assert(!db);
  string fn = path + "/db";
  string options;
  stringstream err;
  ceph::shared_ptr<Int64ArrayMergeOperator> merge_op(new Int64ArrayMergeOperator);

  string kv_backend;
  if (create) {
    kv_backend = cct->_conf->bluestore_kvbackend;
  } else {
    r = read_meta("kv_backend", &kv_backend);
    if (r < 0) {
      derr << __func__ << " unable to read 'kv_backend' meta" << dendl;
      return -EIO;
    }
  }
  dout(10) << __func__ << " kv_backend = " << kv_backend << dendl;

  bool do_bluefs;
  if (create) {
    do_bluefs = cct->_conf->bluestore_bluefs;
  } else {
    string s;
    r = read_meta("bluefs", &s);
    if (r < 0) {
      derr << __func__ << " unable to read 'bluefs' meta" << dendl;
      return -EIO;
    }
    if (s == "1") {
      do_bluefs = true;
    } else if (s == "0") {
      do_bluefs = false;
    } else {
      derr << __func__ << " bluefs = " << s << " : not 0 or 1, aborting"
	   << dendl;
      return -EIO;
    }
  }
  dout(10) << __func__ << " do_bluefs = " << do_bluefs << dendl;

  rocksdb::Env *env = NULL;
  if (do_bluefs) {
    dout(10) << __func__ << " initializing bluefs" << dendl;
    if (kv_backend != "rocksdb") {
      derr << " backend must be rocksdb to use bluefs" << dendl;
      return -EINVAL;
    }
    bluefs = new BlueFS(cct);

    string bfn;
    struct stat st;

    bfn = path + "/block.db";
    if (::stat(bfn.c_str(), &st) == 0) {
      r = bluefs->add_block_device(BlueFS::BDEV_DB, bfn);
      if (r < 0) {
        derr << __func__ << " add block device(" << bfn << ") returned: " 
             << cpp_strerror(r) << dendl;
        goto free_bluefs;
      }

      if (bluefs->bdev_support_label(BlueFS::BDEV_DB)) {
        r = _check_or_set_bdev_label(
	  bfn,
	  bluefs->get_block_device_size(BlueFS::BDEV_DB),
          "bluefs db", create);
        if (r < 0) {
          derr << __func__
	       << " check block device(" << bfn << ") label returned: "
               << cpp_strerror(r) << dendl;
          goto free_bluefs;
        }
      }
      if (create) {
	bluefs->add_block_extent(
	  BlueFS::BDEV_DB,
	  SUPER_RESERVED,
	  bluefs->get_block_device_size(BlueFS::BDEV_DB) - SUPER_RESERVED);
      }
      bluefs_shared_bdev = BlueFS::BDEV_SLOW;
      bluefs_single_shared_device = false;
    } else {
      r = -errno;
      if (::lstat(bfn.c_str(), &st) == -1) {
	r = 0;
	bluefs_shared_bdev = BlueFS::BDEV_DB;
      } else {
	derr << __func__ << " " << bfn << " symlink exists but target unusable: "
	     << cpp_strerror(r) << dendl;
	goto free_bluefs;
      }
    }

    // shared device
    bfn = path + "/block";
    r = bluefs->add_block_device(bluefs_shared_bdev, bfn);
    if (r < 0) {
      derr << __func__ << " add block device(" << bfn << ") returned: " 
	   << cpp_strerror(r) << dendl;
      goto free_bluefs;
    }
    if (create) {
      // note: we always leave the first SUPER_RESERVED (8k) of the device unused
      uint64_t initial =
	bdev->get_size() * (cct->_conf->bluestore_bluefs_min_ratio +
			    cct->_conf->bluestore_bluefs_gift_ratio);
      initial = MAX(initial, cct->_conf->bluestore_bluefs_min);
      if (cct->_conf->bluefs_alloc_size % min_alloc_size) {
	derr << __func__ << " bluefs_alloc_size 0x" << std::hex
	     << cct->_conf->bluefs_alloc_size << " is not a multiple of "
	     << "min_alloc_size 0x" << min_alloc_size << std::dec << dendl;
	r = -EINVAL;
	goto free_bluefs;
      }
      // align to bluefs's alloc_size
      initial = P2ROUNDUP(initial, cct->_conf->bluefs_alloc_size);
      // put bluefs in the middle of the device in case it is an HDD
      uint64_t start = P2ALIGN((bdev->get_size() - initial) / 2,
			       cct->_conf->bluefs_alloc_size);
      bluefs->add_block_extent(bluefs_shared_bdev, start, initial);
      bluefs_extents.insert(start, initial);
    }

    bfn = path + "/block.wal";
    if (::stat(bfn.c_str(), &st) == 0) {
      r = bluefs->add_block_device(BlueFS::BDEV_WAL, bfn);
      if (r < 0) {
        derr << __func__ << " add block device(" << bfn << ") returned: " 
	     << cpp_strerror(r) << dendl;
        goto free_bluefs;			
      }

      if (bluefs->bdev_support_label(BlueFS::BDEV_WAL)) {
        r = _check_or_set_bdev_label(
	  bfn,
	  bluefs->get_block_device_size(BlueFS::BDEV_WAL),
          "bluefs wal", create);
        if (r < 0) {
          derr << __func__ << " check block device(" << bfn
               << ") label returned: " << cpp_strerror(r) << dendl;
          goto free_bluefs;
        }
      }

      if (create) {
	bluefs->add_block_extent(
	  BlueFS::BDEV_WAL, BDEV_LABEL_BLOCK_SIZE,
	  bluefs->get_block_device_size(BlueFS::BDEV_WAL) -
	   BDEV_LABEL_BLOCK_SIZE);
      }
      cct->_conf->set_val("rocksdb_separate_wal_dir", "true");
      bluefs_single_shared_device = false;
    } else {
      r = -errno;
      if (::lstat(bfn.c_str(), &st) == -1) {
	r = 0;
	cct->_conf->set_val("rocksdb_separate_wal_dir", "false");
      } else {
	derr << __func__ << " " << bfn << " symlink exists but target unusable: "
	     << cpp_strerror(r) << dendl;
	goto free_bluefs;
      }
    }

    if (create) {
      bluefs->mkfs(fsid);
    }
    r = bluefs->mount();
    if (r < 0) {
      derr << __func__ << " failed bluefs mount: " << cpp_strerror(r) << dendl;
      goto free_bluefs;
    }
    if (cct->_conf->bluestore_bluefs_env_mirror) {
      rocksdb::Env *a = new BlueRocksEnv(bluefs);
      rocksdb::Env *b = rocksdb::Env::Default();
      if (create) {
	string cmd = "rm -rf " + path + "/db " +
	  path + "/db.slow " +
	  path + "/db.wal";
	int r = system(cmd.c_str());
	(void)r;
      }
      env = new rocksdb::EnvMirror(b, a, false, true);
    } else {
      env = new BlueRocksEnv(bluefs);

      // simplify the dir names, too, as "seen" by rocksdb
      fn = "db";
    }

    if (bluefs_shared_bdev == BlueFS::BDEV_SLOW) {
      // we have both block.db and block; tell rocksdb!
      // note: the second (last) size value doesn't really matter
      ostringstream db_paths;
      uint64_t db_size = bluefs->get_block_device_size(BlueFS::BDEV_DB);
      uint64_t slow_size = bluefs->get_block_device_size(BlueFS::BDEV_SLOW);
      db_paths << fn << ","
               << (uint64_t)(db_size * 95 / 100) << " "
               << fn + ".slow" << ","
               << (uint64_t)(slow_size * 95 / 100);
      cct->_conf->set_val("rocksdb_db_paths", db_paths.str(), false);
      dout(10) << __func__ << " set rocksdb_db_paths to "
	       << cct->_conf->get_val<std::string>("rocksdb_db_paths") << dendl;
    }

    if (create) {
      env->CreateDir(fn);
      if (cct->_conf->rocksdb_separate_wal_dir)
	env->CreateDir(fn + ".wal");
      if (cct->_conf->get_val<std::string>("rocksdb_db_paths").length())
	env->CreateDir(fn + ".slow");
    }
  } else if (create) {
    int r = ::mkdir(fn.c_str(), 0755);
    if (r < 0)
      r = -errno;
    if (r < 0 && r != -EEXIST) {
      derr << __func__ << " failed to create " << fn << ": " << cpp_strerror(r)
	   << dendl;
      return r;
    }

    // wal_dir, too!
    if (cct->_conf->rocksdb_separate_wal_dir) {
      string walfn = path + "/db.wal";
      r = ::mkdir(walfn.c_str(), 0755);
      if (r < 0)
	r = -errno;
      if (r < 0 && r != -EEXIST) {
	derr << __func__ << " failed to create " << walfn
	  << ": " << cpp_strerror(r)
	  << dendl;
	return r;
      }
    }
  }

  db = KeyValueDB::create(cct,
			  kv_backend,
			  fn,
			  static_cast<void*>(env));
  if (!db) {
    derr << __func__ << " error creating db" << dendl;
    if (bluefs) {
      bluefs->umount();
      delete bluefs;
      bluefs = NULL;
    }
    // delete env manually here since we can't depend on db to do this
    // under this case
    delete env;
    env = NULL;
    return -EIO;
  }

  FreelistManager::setup_merge_operators(db);
  db->set_merge_operator(PREFIX_STAT, merge_op);

  db->set_cache_size(cache_size * cache_kv_ratio);

  if (kv_backend == "rocksdb")
    options = cct->_conf->bluestore_rocksdb_options;
  db->init(options);
  if (create)
    r = db->create_and_open(err);
  else
    r = db->open(err);
  if (r) {
    derr << __func__ << " erroring opening db: " << err.str() << dendl;
    if (bluefs) {
      bluefs->umount();
      delete bluefs;
      bluefs = NULL;
    }
    delete db;
    db = NULL;
    return -EIO;
  }
  dout(1) << __func__ << " opened " << kv_backend
	  << " path " << fn << " options " << options << dendl;
  return 0;

free_bluefs:
  assert(bluefs);
  delete bluefs;
  bluefs = NULL;
  return r;
}

void BlueStore::_close_db()
{
  assert(db);
  delete db;
  db = NULL;
  if (bluefs) {
    bluefs->umount();
    delete bluefs;
    bluefs = NULL;
  }
}

int BlueStore::_reconcile_bluefs_freespace()
{
  dout(10) << __func__ << dendl;
  interval_set<uint64_t> bset;
  int r = bluefs->get_block_extents(bluefs_shared_bdev, &bset);
  assert(r == 0);
  if (bset == bluefs_extents) {
    dout(10) << __func__ << " we agree bluefs has 0x" << std::hex << bset
	     << std::dec << dendl;
    return 0;
  }
  dout(10) << __func__ << " bluefs says 0x" << std::hex << bset << std::dec
	   << dendl;
  dout(10) << __func__ << " super says  0x" << std::hex << bluefs_extents
	   << std::dec << dendl;

  interval_set<uint64_t> overlap;
  overlap.intersection_of(bset, bluefs_extents);

  bset.subtract(overlap);
  if (!bset.empty()) {
    derr << __func__ << " bluefs extra 0x" << std::hex << bset << std::dec
	 << dendl;
    return -EIO;
  }

  interval_set<uint64_t> super_extra;
  super_extra = bluefs_extents;
  super_extra.subtract(overlap);
  if (!super_extra.empty()) {
    // This is normal: it can happen if we commit to give extents to
    // bluefs and we crash before bluefs commits that it owns them.
    dout(10) << __func__ << " super extra " << super_extra << dendl;
    for (interval_set<uint64_t>::iterator p = super_extra.begin();
	 p != super_extra.end();
	 ++p) {
      bluefs->add_block_extent(bluefs_shared_bdev, p.get_start(), p.get_len());
    }
  }

  return 0;
}

int BlueStore::_balance_bluefs_freespace(PExtentVector *extents)
{
  int ret = 0;
  assert(bluefs);

  vector<pair<uint64_t,uint64_t>> bluefs_usage;  // <free, total> ...
  bluefs->get_usage(&bluefs_usage);
  assert(bluefs_usage.size() > bluefs_shared_bdev);

  // fixme: look at primary bdev only for now
  uint64_t bluefs_free = bluefs_usage[bluefs_shared_bdev].first;
  uint64_t bluefs_total = bluefs_usage[bluefs_shared_bdev].second;
  float bluefs_free_ratio = (float)bluefs_free / (float)bluefs_total;

  uint64_t my_free = alloc->get_free();
  uint64_t total = bdev->get_size();
  float my_free_ratio = (float)my_free / (float)total;

  uint64_t total_free = bluefs_free + my_free;

  float bluefs_ratio = (float)bluefs_free / (float)total_free;

  dout(10) << __func__
	   << " bluefs " << pretty_si_t(bluefs_free)
	   << " free (" << bluefs_free_ratio
	   << ") bluestore " << pretty_si_t(my_free)
	   << " free (" << my_free_ratio
	   << "), bluefs_ratio " << bluefs_ratio
	   << dendl;

  uint64_t gift = 0;
  uint64_t reclaim = 0;
  if (bluefs_ratio < cct->_conf->bluestore_bluefs_min_ratio) {
    gift = cct->_conf->bluestore_bluefs_gift_ratio * total_free;
    dout(10) << __func__ << " bluefs_ratio " << bluefs_ratio
	     << " < min_ratio " << cct->_conf->bluestore_bluefs_min_ratio
	     << ", should gift " << pretty_si_t(gift) << dendl;
  } else if (bluefs_ratio > cct->_conf->bluestore_bluefs_max_ratio) {
    reclaim = cct->_conf->bluestore_bluefs_reclaim_ratio * total_free;
    if (bluefs_total - reclaim < cct->_conf->bluestore_bluefs_min)
      reclaim = bluefs_total - cct->_conf->bluestore_bluefs_min;
    dout(10) << __func__ << " bluefs_ratio " << bluefs_ratio
	     << " > max_ratio " << cct->_conf->bluestore_bluefs_max_ratio
	     << ", should reclaim " << pretty_si_t(reclaim) << dendl;
  }

  // don't take over too much of the freespace
  uint64_t free_cap = cct->_conf->bluestore_bluefs_max_ratio * total_free;
  if (bluefs_total < cct->_conf->bluestore_bluefs_min &&
      cct->_conf->bluestore_bluefs_min < free_cap) {
    uint64_t g = cct->_conf->bluestore_bluefs_min - bluefs_total;
    dout(10) << __func__ << " bluefs_total " << bluefs_total
	     << " < min " << cct->_conf->bluestore_bluefs_min
	     << ", should gift " << pretty_si_t(g) << dendl;
    if (g > gift)
      gift = g;
    reclaim = 0;
  }
  uint64_t min_free = cct->_conf->get_val<uint64_t>("bluestore_bluefs_min_free");
  if (bluefs_free < min_free &&
      min_free < free_cap) {
    uint64_t g = min_free - bluefs_free;
    dout(10) << __func__ << " bluefs_free " << bluefs_total
	     << " < min " << min_free
	     << ", should gift " << pretty_si_t(g) << dendl;
    if (g > gift)
      gift = g;
    reclaim = 0;
  }

  if (gift) {
    // round up to alloc size
    gift = P2ROUNDUP(gift, cct->_conf->bluefs_alloc_size);

    // hard cap to fit into 32 bits
    gift = MIN(gift, 1ull<<31);
    dout(10) << __func__ << " gifting " << gift
	     << " (" << pretty_si_t(gift) << ")" << dendl;

    // fixme: just do one allocation to start...
    int r = alloc->reserve(gift);
    assert(r == 0);

    AllocExtentVector exts;
    int64_t alloc_len = alloc->allocate(gift, cct->_conf->bluefs_alloc_size,
					0, 0, &exts);

    if (alloc_len <= 0) {
      dout(1) << __func__ << " no allocate on 0x" << std::hex << gift
              << " min_alloc_size 0x" << min_alloc_size << std::dec << dendl;
      alloc->unreserve(gift);
      alloc->dump();
      return 0;
    } else if (alloc_len < (int64_t)gift) {
      dout(1) << __func__ << " insufficient allocate on 0x" << std::hex << gift
              << " min_alloc_size 0x" << min_alloc_size 
	      << " allocated 0x" << alloc_len
	      << std::dec << dendl;
      alloc->unreserve(gift - alloc_len);
      alloc->dump();
    }
    for (auto& p : exts) {
      bluestore_pextent_t e = bluestore_pextent_t(p);
      dout(1) << __func__ << " gifting " << e << " to bluefs" << dendl;
      extents->push_back(e);
    }
    gift = 0;

    ret = 1;
  }

  // reclaim from bluefs?
  if (reclaim) {
    // round up to alloc size
    reclaim = P2ROUNDUP(reclaim, cct->_conf->bluefs_alloc_size);

    // hard cap to fit into 32 bits
    reclaim = MIN(reclaim, 1ull<<31);
    dout(10) << __func__ << " reclaiming " << reclaim
	     << " (" << pretty_si_t(reclaim) << ")" << dendl;

    while (reclaim > 0) {
      // NOTE: this will block and do IO.
      AllocExtentVector extents;
      int r = bluefs->reclaim_blocks(bluefs_shared_bdev, reclaim,
				     &extents);
      if (r < 0) {
	derr << __func__ << " failed to reclaim space from bluefs"
	     << dendl;
	break;
      }
      for (auto e : extents) {
	bluefs_extents.erase(e.offset, e.length);
	bluefs_extents_reclaiming.insert(e.offset, e.length);
	reclaim -= e.length;
      }
    }

    ret = 1;
  }

  return ret;
}

void BlueStore::_commit_bluefs_freespace(
  const PExtentVector& bluefs_gift_extents)
{
  dout(10) << __func__ << dendl;
  for (auto& p : bluefs_gift_extents) {
    bluefs->add_block_extent(bluefs_shared_bdev, p.offset, p.length);
  }
}

int BlueStore::_open_collections(int *errors)
{
  dout(10) << __func__ << dendl;
  assert(coll_map.empty());
  KeyValueDB::Iterator it = db->get_iterator(PREFIX_COLL);
  for (it->upper_bound(string());
       it->valid();
       it->next()) {
    coll_t cid;
    if (cid.parse(it->key())) {
      CollectionRef c(
	new Collection(
	  this,
	  cache_shards[cid.hash_to_shard(cache_shards.size())],
	  cid));
      bufferlist bl = it->value();
      bufferlist::iterator p = bl.begin();
      try {
        ::decode(c->cnode, p);
      } catch (buffer::error& e) {
        derr << __func__ << " failed to decode cnode, key:"
             << pretty_binary_string(it->key()) << dendl;
        return -EIO;
      }   
      dout(20) << __func__ << " opened " << cid << " " << c
	       << " " << c->cnode << dendl;
      coll_map[cid] = c;
    } else {
      derr << __func__ << " unrecognized collection " << it->key() << dendl;
      if (errors)
	(*errors)++;
    }
  }
  return 0;
}

void BlueStore::_open_statfs()
{
  bufferlist bl;
  int r = db->get(PREFIX_STAT, "bluestore_statfs", &bl);
  if (r >= 0) {
    if (size_t(bl.length()) >= sizeof(vstatfs.values)) {
      auto it = bl.begin();
      vstatfs.decode(it);
    } else {
      dout(10) << __func__ << " store_statfs is corrupt, using empty" << dendl;
    }
  }
  else {
    dout(10) << __func__ << " store_statfs missed, using empty" << dendl;
  }
}

int BlueStore::_setup_block_symlink_or_file(
  string name,
  string epath,
  uint64_t size,
  bool create)
{
  dout(20) << __func__ << " name " << name << " path " << epath
	   << " size " << size << " create=" << (int)create << dendl;
  int r = 0;
  int flags = O_RDWR;
  if (create)
    flags |= O_CREAT;
  if (epath.length()) {
    r = ::symlinkat(epath.c_str(), path_fd, name.c_str());
    if (r < 0) {
      r = -errno;
      derr << __func__ << " failed to create " << name << " symlink to "
           << epath << ": " << cpp_strerror(r) << dendl;
      return r;
    }

    if (!epath.compare(0, strlen(SPDK_PREFIX), SPDK_PREFIX)) {
      int fd = ::openat(path_fd, epath.c_str(), flags, 0644);
      if (fd < 0) {
	r = -errno;
	derr << __func__ << " failed to open " << epath << " file: "
	     << cpp_strerror(r) << dendl;
	return r;
      }
      string serial_number = epath.substr(strlen(SPDK_PREFIX));
      r = ::write(fd, serial_number.c_str(), serial_number.size());
      assert(r == (int)serial_number.size());
      dout(1) << __func__ << " created " << name << " symlink to "
              << epath << dendl;
      VOID_TEMP_FAILURE_RETRY(::close(fd));
    }
  }
  if (size) {
    int fd = ::openat(path_fd, name.c_str(), flags, 0644);
    if (fd >= 0) {
      // block file is present
      struct stat st;
      int r = ::fstat(fd, &st);
      if (r == 0 &&
	  S_ISREG(st.st_mode) &&   // if it is a regular file
	  st.st_size == 0) {       // and is 0 bytes
	r = ::ftruncate(fd, size);
	if (r < 0) {
	  r = -errno;
	  derr << __func__ << " failed to resize " << name << " file to "
	       << size << ": " << cpp_strerror(r) << dendl;
	  VOID_TEMP_FAILURE_RETRY(::close(fd));
	  return r;
	}

	if (cct->_conf->bluestore_block_preallocate_file) {
          r = ::ceph_posix_fallocate(fd, 0, size);
          if (r > 0) {
	    derr << __func__ << " failed to prefallocate " << name << " file to "
	      << size << ": " << cpp_strerror(r) << dendl;
	    VOID_TEMP_FAILURE_RETRY(::close(fd));
	    return -r;
	  }
	}
	dout(1) << __func__ << " resized " << name << " file to "
		<< pretty_si_t(size) << "B" << dendl;
      }
      VOID_TEMP_FAILURE_RETRY(::close(fd));
    } else {
      int r = -errno;
      if (r != -ENOENT) {
	derr << __func__ << " failed to open " << name << " file: "
	     << cpp_strerror(r) << dendl;
	return r;
      }
    }
  }
  return 0;
}

int BlueStore::mkfs()
{
  dout(1) << __func__ << " path " << path << dendl;
  int r;
  uuid_d old_fsid;

  {
    string done;
    r = read_meta("mkfs_done", &done);
    if (r == 0) {
      dout(1) << __func__ << " already created" << dendl;
      if (cct->_conf->bluestore_fsck_on_mkfs) {
        r = fsck(cct->_conf->bluestore_fsck_on_mkfs_deep);
        if (r < 0) {
          derr << __func__ << " fsck found fatal error: " << cpp_strerror(r)
               << dendl;
          return r;
        }
        if (r > 0) {
          derr << __func__ << " fsck found " << r << " errors" << dendl;
          r = -EIO;
        }
      }
      return r; // idempotent
    }
  }

  {
    string type;
    r = read_meta("type", &type);
    if (r == 0) {
      if (type != "bluestore") {
	derr << __func__ << " expected bluestore, but type is " << type << dendl;
	return -EIO;
      }
    } else {
      r = write_meta("type", "bluestore");
      if (r < 0)
        return r;
    }
  }

  freelist_type = "bitmap";

  r = _open_path();
  if (r < 0)
    return r;

  r = _open_fsid(true);
  if (r < 0)
    goto out_path_fd;

  r = _lock_fsid();
  if (r < 0)
    goto out_close_fsid;

  r = _read_fsid(&old_fsid);
  if (r < 0 || old_fsid.is_zero()) {
    if (fsid.is_zero()) {
      fsid.generate_random();
      dout(1) << __func__ << " generated fsid " << fsid << dendl;
    } else {
      dout(1) << __func__ << " using provided fsid " << fsid << dendl;
    }
    // we'll write it later.
  } else {
    if (!fsid.is_zero() && fsid != old_fsid) {
      derr << __func__ << " on-disk fsid " << old_fsid
	   << " != provided " << fsid << dendl;
      r = -EINVAL;
      goto out_close_fsid;
    }
    fsid = old_fsid;
  }

  r = _setup_block_symlink_or_file("block", cct->_conf->bluestore_block_path,
				   cct->_conf->bluestore_block_size,
				   cct->_conf->bluestore_block_create);
  if (r < 0)
    goto out_close_fsid;
  if (cct->_conf->bluestore_bluefs) {
    r = _setup_block_symlink_or_file("block.wal", cct->_conf->bluestore_block_wal_path,
	cct->_conf->bluestore_block_wal_size,
	cct->_conf->bluestore_block_wal_create);
    if (r < 0)
      goto out_close_fsid;
    r = _setup_block_symlink_or_file("block.db", cct->_conf->bluestore_block_db_path,
	cct->_conf->bluestore_block_db_size,
	cct->_conf->bluestore_block_db_create);
    if (r < 0)
      goto out_close_fsid;
  }

  r = _open_bdev(true);
  if (r < 0)
    goto out_close_fsid;

  // choose min_alloc_size
  if (cct->_conf->bluestore_min_alloc_size) {
    min_alloc_size = cct->_conf->bluestore_min_alloc_size;
  } else {
    assert(bdev);
    if (bdev->is_rotational()) {
      min_alloc_size = cct->_conf->bluestore_min_alloc_size_hdd;
    } else {
      min_alloc_size = cct->_conf->bluestore_min_alloc_size_ssd;
    }
  }

  // make sure min_alloc_size is power of 2 aligned.
  if (!ISP2(min_alloc_size)) {
    derr << __func__ << " min_alloc_size 0x"
	 << std::hex << min_alloc_size << std::dec
	 << " is not power of 2 aligned!"
	 << dendl;
    r = -EINVAL;
    goto out_close_bdev;
  }

  r = _open_db(true);
  if (r < 0)
    goto out_close_bdev;

  r = _open_fm(true);
  if (r < 0)
    goto out_close_db;

  {
    KeyValueDB::Transaction t = db->get_transaction();
    {
      bufferlist bl;
      ::encode((uint64_t)0, bl);
      t->set(PREFIX_SUPER, "nid_max", bl);
      t->set(PREFIX_SUPER, "blobid_max", bl);
    }

    {
      bufferlist bl;
      ::encode((uint64_t)min_alloc_size, bl);
      t->set(PREFIX_SUPER, "min_alloc_size", bl);
    }

    ondisk_format = latest_ondisk_format;
    _prepare_ondisk_format_super(t);
    db->submit_transaction_sync(t);
  }


  r = write_meta("kv_backend", cct->_conf->bluestore_kvbackend);
  if (r < 0)
    goto out_close_fm;

  r = write_meta("bluefs", stringify(bluefs ? 1 : 0));
  if (r < 0)
    goto out_close_fm;

  if (fsid != old_fsid) {
    r = _write_fsid();
    if (r < 0) {
      derr << __func__ << " error writing fsid: " << cpp_strerror(r) << dendl;
      goto out_close_fm;
    }
  }

 out_close_fm:
  _close_fm();
 out_close_db:
  _close_db();
 out_close_bdev:
  _close_bdev();
 out_close_fsid:
  _close_fsid();
 out_path_fd:
  _close_path();

  if (r == 0 &&
      cct->_conf->bluestore_fsck_on_mkfs) {
    int rc = fsck(cct->_conf->bluestore_fsck_on_mkfs_deep);
    if (rc < 0)
      return rc;
    if (rc > 0) {
      derr << __func__ << " fsck found " << rc << " errors" << dendl;
      r = -EIO;
    }
  }

  if (r == 0) {
    // indicate success by writing the 'mkfs_done' file
    r = write_meta("mkfs_done", "yes");
  }

  if (r < 0) {
    derr << __func__ << " failed, " << cpp_strerror(r) << dendl;
  } else {
    dout(0) << __func__ << " success" << dendl;
  }
  return r;
}

void BlueStore::set_cache_shards(unsigned num)
{
  dout(10) << __func__ << " " << num << dendl;
  size_t old = cache_shards.size();
  assert(num >= old);
  cache_shards.resize(num);
  for (unsigned i = old; i < num; ++i) {
    cache_shards[i] = Cache::create(cct, cct->_conf->bluestore_cache_type,
				    logger);
  }
}

int BlueStore::_mount(bool kv_only)
{
  dout(1) << __func__ << " path " << path << dendl;

  _kv_only = kv_only;

  {
    string type;
    int r = read_meta("type", &type);
    if (r < 0) {
      derr << __func__ << " failed to load os-type: " << cpp_strerror(r)
	   << dendl;
      return r;
    }

    if (type != "bluestore") {
      derr << __func__ << " expected bluestore, but type is " << type << dendl;
      return -EIO;
    }
  }

  if (cct->_conf->bluestore_fsck_on_mount) {
    int rc = fsck(cct->_conf->bluestore_fsck_on_mount_deep);
    if (rc < 0)
      return rc;
    if (rc > 0) {
      derr << __func__ << " fsck found " << rc << " errors" << dendl;
      return -EIO;
    }
  }

  int r = _open_path();
  if (r < 0)
    return r;
  r = _open_fsid(false);
  if (r < 0)
    goto out_path;

  r = _read_fsid(&fsid);
  if (r < 0)
    goto out_fsid;

  r = _lock_fsid();
  if (r < 0)
    goto out_fsid;

  r = _open_bdev(false);
  if (r < 0)
    goto out_fsid;

  r = _open_db(false);
  if (r < 0)
    goto out_bdev;

  if (kv_only)
    return 0;

  r = _open_super_meta();
  if (r < 0)
    goto out_db;

  r = _open_fm(false);
  if (r < 0)
    goto out_db;

  r = _open_alloc();
  if (r < 0)
    goto out_fm;

  r = _open_collections();
  if (r < 0)
    goto out_alloc;

  r = _reload_logger();
  if (r < 0)
    goto out_coll;

  if (bluefs) {
    r = _reconcile_bluefs_freespace();
    if (r < 0)
      goto out_coll;
  }

  _kv_start();

  r = _deferred_replay();
  if (r < 0)
    goto out_stop;

  mempool_thread.init();

  mounted = true;
  return 0;

 out_stop:
  _kv_stop();
 out_coll:
  _flush_cache();
 out_alloc:
  _close_alloc();
 out_fm:
  _close_fm();
 out_db:
  _close_db();
 out_bdev:
  _close_bdev();
 out_fsid:
  _close_fsid();
 out_path:
  _close_path();
  return r;
}

int BlueStore::umount()
{
  assert(_kv_only || mounted);
  dout(1) << __func__ << dendl;

  _osr_drain_all();
  _osr_unregister_all();

  mounted = false;
  if (!_kv_only) {
    mempool_thread.shutdown();
    dout(20) << __func__ << " stopping kv thread" << dendl;
    _kv_stop();
    _flush_cache();
    dout(20) << __func__ << " closing" << dendl;

    _close_alloc();
    _close_fm();
  }
  _close_db();
  _close_bdev();
  _close_fsid();
  _close_path();

  if (cct->_conf->bluestore_fsck_on_umount) {
    int rc = fsck(cct->_conf->bluestore_fsck_on_umount_deep);
    if (rc < 0)
      return rc;
    if (rc > 0) {
      derr << __func__ << " fsck found " << rc << " errors" << dendl;
      return -EIO;
    }
  }
  return 0;
}

static void apply(uint64_t off,
                  uint64_t len,
                  uint64_t granularity,
                  BlueStore::mempool_dynamic_bitset &bitset,
                  std::function<void(uint64_t,
				     BlueStore::mempool_dynamic_bitset &)> f) {
  auto end = ROUND_UP_TO(off + len, granularity);
  while (off < end) {
    uint64_t pos = off / granularity;
    f(pos, bitset);
    off += granularity;
  }
}

int BlueStore::_fsck_check_extents(
  const ghobject_t& oid,
  const PExtentVector& extents,
  bool compressed,
  mempool_dynamic_bitset &used_blocks,
  uint64_t granularity,
  store_statfs_t& expected_statfs)
{
  dout(30) << __func__ << " oid " << oid << " extents " << extents << dendl;
  int errors = 0;
  for (auto e : extents) {
    if (!e.is_valid())
      continue;
    expected_statfs.allocated += e.length;
    if (compressed) {
      expected_statfs.compressed_allocated += e.length;
    }
    bool already = false;
    apply(
      e.offset, e.length, granularity, used_blocks,
      [&](uint64_t pos, mempool_dynamic_bitset &bs) {
	assert(pos < bs.size());
	if (bs.test(pos))
	  already = true;
	else
	  bs.set(pos);
      });
    if (already) {
      derr << " " << oid << " extent " << e
	   << " or a subset is already allocated" << dendl;
      ++errors;
    }
    if (e.end() > bdev->get_size()) {
      derr << " " << oid << " extent " << e
	   << " past end of block device" << dendl;
      ++errors;
    }
  }
  return errors;
}

int BlueStore::_fsck(bool deep, bool repair)
{
  dout(1) << __func__
	  << (repair ? " fsck" : " repair")
	  << (deep ? " (deep)" : " (shallow)") << " start" << dendl;
  int errors = 0;
  int repaired = 0;

  typedef btree::btree_set<
    uint64_t,std::less<uint64_t>,
    mempool::bluestore_fsck::pool_allocator<uint64_t>> uint64_t_btree_t;
  uint64_t_btree_t used_nids;
  uint64_t_btree_t used_omap_head;
  uint64_t_btree_t used_sbids;

  mempool_dynamic_bitset used_blocks;
  KeyValueDB::Iterator it;
  store_statfs_t expected_statfs, actual_statfs;
  struct sb_info_t {
    list<ghobject_t> oids;
    SharedBlobRef sb;
    bluestore_extent_ref_map_t ref_map;
    bool compressed;
  };
  mempool::bluestore_fsck::map<uint64_t,sb_info_t> sb_info;

  uint64_t num_objects = 0;
  uint64_t num_extents = 0;
  uint64_t num_blobs = 0;
  uint64_t num_spanning_blobs = 0;
  uint64_t num_shared_blobs = 0;
  uint64_t num_sharded_objects = 0;
  uint64_t num_object_shards = 0;

  utime_t start = ceph_clock_now();

  int r = _open_path();
  if (r < 0)
    return r;
  r = _open_fsid(false);
  if (r < 0)
    goto out_path;

  r = _read_fsid(&fsid);
  if (r < 0)
    goto out_fsid;

  r = _lock_fsid();
  if (r < 0)
    goto out_fsid;

  r = _open_bdev(false);
  if (r < 0)
    goto out_fsid;

  r = _open_db(false);
  if (r < 0)
    goto out_bdev;

  r = _open_super_meta();
  if (r < 0)
    goto out_db;

  r = _open_fm(false);
  if (r < 0)
    goto out_db;

  r = _open_alloc();
  if (r < 0)
    goto out_fm;

  r = _open_collections(&errors);
  if (r < 0)
    goto out_alloc;

  mempool_thread.init();

  // we need finishers and kv_{sync,finalize}_thread *just* for replay
  _kv_start();
  r = _deferred_replay();
  _kv_stop();
  if (r < 0)
    goto out_scan;

  used_blocks.resize(fm->get_alloc_units());
  apply(
    0, MAX(min_alloc_size, SUPER_RESERVED), fm->get_alloc_size(), used_blocks,
    [&](uint64_t pos, mempool_dynamic_bitset &bs) {
	assert(pos < bs.size());
      bs.set(pos);
    }
  );

  if (bluefs) {
    for (auto e = bluefs_extents.begin(); e != bluefs_extents.end(); ++e) {
      apply(
        e.get_start(), e.get_len(), fm->get_alloc_size(), used_blocks,
        [&](uint64_t pos, mempool_dynamic_bitset &bs) {
	assert(pos < bs.size());
          bs.set(pos);
        }
	);
    }
    r = bluefs->fsck();
    if (r < 0) {
      goto out_scan;
    }
    if (r > 0)
      errors += r;
  }

  // get expected statfs; fill unaffected fields to be able to compare
  // structs
  statfs(&actual_statfs);
  expected_statfs.total = actual_statfs.total;
  expected_statfs.available = actual_statfs.available;

  // walk PREFIX_OBJ
  dout(1) << __func__ << " walking object keyspace" << dendl;
  it = db->get_iterator(PREFIX_OBJ);
  if (it) {
    CollectionRef c;
    spg_t pgid;
    mempool::bluestore_fsck::list<string> expecting_shards;
    for (it->lower_bound(string()); it->valid(); it->next()) {
      if (g_conf->bluestore_debug_fsck_abort) {
	goto out_scan;
      }
      dout(30) << " key " << pretty_binary_string(it->key()) << dendl;
      if (is_extent_shard_key(it->key())) {
	while (!expecting_shards.empty() &&
	       expecting_shards.front() < it->key()) {
	  derr << "fsck error: missing shard key "
	       << pretty_binary_string(expecting_shards.front())
	       << dendl;
	  ++errors;
	  expecting_shards.pop_front();
	}
	if (!expecting_shards.empty() &&
	    expecting_shards.front() == it->key()) {
	  // all good
	  expecting_shards.pop_front();
	  continue;
	}

        uint32_t offset;
        string okey;
        get_key_extent_shard(it->key(), &okey, &offset);
        derr << "fsck error: stray shard 0x" << std::hex << offset
	     << std::dec << dendl;
        if (expecting_shards.empty()) {
          derr << "fsck error: " << pretty_binary_string(it->key())
               << " is unexpected" << dendl;
          ++errors;
          continue;
        }
	while (expecting_shards.front() > it->key()) {
	  derr << "fsck error:   saw " << pretty_binary_string(it->key())
	       << dendl;
	  derr << "fsck error:   exp "
	       << pretty_binary_string(expecting_shards.front()) << dendl;
	  ++errors;
	  expecting_shards.pop_front();
	  if (expecting_shards.empty()) {
	    break;
	  }
	}
	continue;
      }

      ghobject_t oid;
      int r = get_key_object(it->key(), &oid);
      if (r < 0) {
        derr << "fsck error: bad object key "
             << pretty_binary_string(it->key()) << dendl;
	++errors;
	continue;
      }
      if (!c ||
	  oid.shard_id != pgid.shard ||
	  oid.hobj.pool != (int64_t)pgid.pool() ||
	  !c->contains(oid)) {
	c = nullptr;
	for (ceph::unordered_map<coll_t, CollectionRef>::iterator p =
	       coll_map.begin();
	     p != coll_map.end();
	     ++p) {
	  if (p->second->contains(oid)) {
	    c = p->second;
	    break;
	  }
	}
	if (!c) {
          derr << "fsck error: stray object " << oid
               << " not owned by any collection" << dendl;
	  ++errors;
	  continue;
	}
	c->cid.is_pg(&pgid);
	dout(20) << __func__ << "  collection " << c->cid << " " << c->cnode
		 << dendl;
      }

      if (!expecting_shards.empty()) {
	for (auto &k : expecting_shards) {
	  derr << "fsck error: missing shard key "
	       << pretty_binary_string(k) << dendl;
	}
	++errors;
	expecting_shards.clear();
      }

      dout(10) << __func__ << "  " << oid << dendl;
      RWLock::RLocker l(c->lock);
      OnodeRef o = c->get_onode(oid, false);
      if (o->onode.nid) {
	if (o->onode.nid > nid_max) {
	  derr << "fsck error: " << oid << " nid " << o->onode.nid
	       << " > nid_max " << nid_max << dendl;
	  ++errors;
	}
	if (used_nids.count(o->onode.nid)) {
	  derr << "fsck error: " << oid << " nid " << o->onode.nid
	       << " already in use" << dendl;
	  ++errors;
	  continue; // go for next object
	}
	used_nids.insert(o->onode.nid);
      }
      ++num_objects;
      num_spanning_blobs += o->extent_map.spanning_blob_map.size();
      o->extent_map.fault_range(db, 0, OBJECT_MAX_SIZE);
      _dump_onode(o, 30);
      // shards
      if (!o->extent_map.shards.empty()) {
	++num_sharded_objects;
	num_object_shards += o->extent_map.shards.size();
      }
      for (auto& s : o->extent_map.shards) {
	dout(20) << __func__ << "    shard " << *s.shard_info << dendl;
	expecting_shards.push_back(string());
	get_extent_shard_key(o->key, s.shard_info->offset,
			     &expecting_shards.back());
	if (s.shard_info->offset >= o->onode.size) {
	  derr << "fsck error: " << oid << " shard 0x" << std::hex
	       << s.shard_info->offset << " past EOF at 0x" << o->onode.size
	       << std::dec << dendl;
	  ++errors;
	}
      }
      // lextents
      map<BlobRef,bluestore_blob_t::unused_t> referenced;
      uint64_t pos = 0;
      mempool::bluestore_fsck::map<BlobRef,
				   bluestore_blob_use_tracker_t> ref_map;
      for (auto& l : o->extent_map.extent_map) {
	dout(20) << __func__ << "    " << l << dendl;
	if (l.logical_offset < pos) {
	  derr << "fsck error: " << oid << " lextent at 0x"
	       << std::hex << l.logical_offset
	       << " overlaps with the previous, which ends at 0x" << pos
	       << std::dec << dendl;
	  ++errors;
	}
	if (o->extent_map.spans_shard(l.logical_offset, l.length)) {
	  derr << "fsck error: " << oid << " lextent at 0x"
	       << std::hex << l.logical_offset << "~" << l.length
	       << " spans a shard boundary"
	       << std::dec << dendl;
	  ++errors;
	}
	pos = l.logical_offset + l.length;
	expected_statfs.stored += l.length;
	assert(l.blob);
	const bluestore_blob_t& blob = l.blob->get_blob();

        auto& ref = ref_map[l.blob];
        if (ref.is_empty()) {
          uint32_t min_release_size = blob.get_release_size(min_alloc_size);
          uint32_t l = blob.get_logical_length();
          ref.init(l, min_release_size);
        }
	ref.get(
	  l.blob_offset, 
	  l.length);
	++num_extents;
	if (blob.has_unused()) {
	  auto p = referenced.find(l.blob);
	  bluestore_blob_t::unused_t *pu;
	  if (p == referenced.end()) {
	    pu = &referenced[l.blob];
	  } else {
	    pu = &p->second;
	  }
	  uint64_t blob_len = blob.get_logical_length();
	  assert((blob_len % (sizeof(*pu)*8)) == 0);
	  assert(l.blob_offset + l.length <= blob_len);
	  uint64_t chunk_size = blob_len / (sizeof(*pu)*8);
	  uint64_t start = l.blob_offset / chunk_size;
	  uint64_t end =
	    ROUND_UP_TO(l.blob_offset + l.length, chunk_size) / chunk_size;
	  for (auto i = start; i < end; ++i) {
	    (*pu) |= (1u << i);
	  }
	}
      }
      for (auto &i : referenced) {
	dout(20) << __func__ << "  referenced 0x" << std::hex << i.second
		 << std::dec << " for " << *i.first << dendl;
	const bluestore_blob_t& blob = i.first->get_blob();
	if (i.second & blob.unused) {
	  derr << "fsck error: " << oid << " blob claims unused 0x"
	       << std::hex << blob.unused
	       << " but extents reference 0x" << i.second
	       << " on blob " << *i.first << dendl;
	  ++errors;
	}
	if (blob.has_csum()) {
	  uint64_t blob_len = blob.get_logical_length();
	  uint64_t unused_chunk_size = blob_len / (sizeof(blob.unused)*8);
	  unsigned csum_count = blob.get_csum_count();
	  unsigned csum_chunk_size = blob.get_csum_chunk_size();
	  for (unsigned p = 0; p < csum_count; ++p) {
	    unsigned pos = p * csum_chunk_size;
	    unsigned firstbit = pos / unused_chunk_size;    // [firstbit,lastbit]
	    unsigned lastbit = (pos + csum_chunk_size - 1) / unused_chunk_size;
	    unsigned mask = 1u << firstbit;
	    for (unsigned b = firstbit + 1; b <= lastbit; ++b) {
	      mask |= 1u << b;
	    }
	    if ((blob.unused & mask) == mask) {
	      // this csum chunk region is marked unused
	      if (blob.get_csum_item(p) != 0) {
		derr << "fsck error: " << oid
		     << " blob claims csum chunk 0x" << std::hex << pos
		     << "~" << csum_chunk_size
		     << " is unused (mask 0x" << mask << " of unused 0x"
		     << blob.unused << ") but csum is non-zero 0x"
		     << blob.get_csum_item(p) << std::dec << " on blob "
		     << *i.first << dendl;
		++errors;
	      }
	    }
	  }
	}
      }
      for (auto &i : ref_map) {
	++num_blobs;
	const bluestore_blob_t& blob = i.first->get_blob();
	bool equal = i.first->get_blob_use_tracker().equal(i.second);
	if (!equal) {
	  derr << "fsck error: " << oid << " blob " << *i.first
	       << " doesn't match expected ref_map " << i.second << dendl;
	  ++errors;
	}
	if (blob.is_compressed()) {
	  expected_statfs.compressed += blob.get_compressed_payload_length();
	  expected_statfs.compressed_original += 
	    i.first->get_referenced_bytes();
	}
	if (blob.is_shared()) {
	  if (i.first->shared_blob->get_sbid() > blobid_max) {
	    derr << "fsck error: " << oid << " blob " << blob
		 << " sbid " << i.first->shared_blob->get_sbid() << " > blobid_max "
		 << blobid_max << dendl;
	    ++errors;
	  } else if (i.first->shared_blob->get_sbid() == 0) {
            derr << "fsck error: " << oid << " blob " << blob
                 << " marked as shared but has uninitialized sbid"
                 << dendl;
            ++errors;
          }
	  sb_info_t& sbi = sb_info[i.first->shared_blob->get_sbid()];
	  sbi.sb = i.first->shared_blob;
	  sbi.oids.push_back(oid);
	  sbi.compressed = blob.is_compressed();
	  for (auto e : blob.get_extents()) {
	    if (e.is_valid()) {
	      sbi.ref_map.get(e.offset, e.length);
	    }
	  }
	} else {
	  errors += _fsck_check_extents(oid, blob.get_extents(),
					blob.is_compressed(),
					used_blocks,
					fm->get_alloc_size(),
					expected_statfs);
        }
      }
      if (deep) {
	bufferlist bl;
	int r = _do_read(c.get(), o, 0, o->onode.size, bl, 0);
	if (r < 0) {
	  ++errors;
	  derr << "fsck error: " << oid << " error during read: "
	       << cpp_strerror(r) << dendl;
	}
      }
      // omap
      if (o->onode.has_omap()) {
	if (used_omap_head.count(o->onode.nid)) {
	  derr << "fsck error: " << oid << " omap_head " << o->onode.nid
	       << " already in use" << dendl;
	  ++errors;
	} else {
	  used_omap_head.insert(o->onode.nid);
	}
      }
    }
  }
  dout(1) << __func__ << " checking shared_blobs" << dendl;
  it = db->get_iterator(PREFIX_SHARED_BLOB);
  if (it) {
    for (it->lower_bound(string()); it->valid(); it->next()) {
      string key = it->key();
      uint64_t sbid;
      if (get_key_shared_blob(key, &sbid)) {
	derr << "fsck error: bad key '" << key
	     << "' in shared blob namespace" << dendl;
	++errors;
	continue;
      }
      auto p = sb_info.find(sbid);
      if (p == sb_info.end()) {
	derr << "fsck error: found stray shared blob data for sbid 0x"
	     << std::hex << sbid << std::dec << dendl;
	++errors;
      } else {
	++num_shared_blobs;
	sb_info_t& sbi = p->second;
	bluestore_shared_blob_t shared_blob(sbid);
	bufferlist bl = it->value();
	bufferlist::iterator blp = bl.begin();
	::decode(shared_blob, blp);
	dout(20) << __func__ << "  " << *sbi.sb << " " << shared_blob << dendl;
	if (shared_blob.ref_map != sbi.ref_map) {
	  derr << "fsck error: shared blob 0x" << std::hex << sbid
	       << std::dec << " ref_map " << shared_blob.ref_map
	       << " != expected " << sbi.ref_map << dendl;
	  ++errors;
	}
	PExtentVector extents;
	for (auto &r : shared_blob.ref_map.ref_map) {
	  extents.emplace_back(bluestore_pextent_t(r.first, r.second.length));
	}
	errors += _fsck_check_extents(p->second.oids.front(),
				      extents,
				      p->second.compressed,
				      used_blocks,
				      fm->get_alloc_size(),
				      expected_statfs);
	sb_info.erase(p);
      }
    }
  }
  for (auto &p : sb_info) {
    derr << "fsck error: shared_blob 0x" << p.first
	 << " key is missing (" << *p.second.sb << ")" << dendl;
    ++errors;
  }
  if (!(actual_statfs == expected_statfs)) {
    derr << "fsck error: actual " << actual_statfs
	 << " != expected " << expected_statfs << dendl;
    ++errors;
  }

  dout(1) << __func__ << " checking for stray omap data" << dendl;
  it = db->get_iterator(PREFIX_OMAP);
  if (it) {
    for (it->lower_bound(string()); it->valid(); it->next()) {
      uint64_t omap_head;
      _key_decode_u64(it->key().c_str(), &omap_head);
      if (used_omap_head.count(omap_head) == 0) {
	derr << "fsck error: found stray omap data on omap_head "
	     << omap_head << dendl;
	++errors;
      }
    }
  }

  dout(1) << __func__ << " checking deferred events" << dendl;
  it = db->get_iterator(PREFIX_DEFERRED);
  if (it) {
    for (it->lower_bound(string()); it->valid(); it->next()) {
      bufferlist bl = it->value();
      bufferlist::iterator p = bl.begin();
      bluestore_deferred_transaction_t wt;
      try {
	::decode(wt, p);
      } catch (buffer::error& e) {
	derr << "fsck error: failed to decode deferred txn "
	     << pretty_binary_string(it->key()) << dendl;
	r = -EIO;
        goto out_scan;
      }
      dout(20) << __func__ << "  deferred " << wt.seq
	       << " ops " << wt.ops.size()
	       << " released 0x" << std::hex << wt.released << std::dec << dendl;
      for (auto e = wt.released.begin(); e != wt.released.end(); ++e) {
        apply(
          e.get_start(), e.get_len(), fm->get_alloc_size(), used_blocks,
          [&](uint64_t pos, mempool_dynamic_bitset &bs) {
	assert(pos < bs.size());
            bs.set(pos);
          }
        );
      }
    }
  }

  dout(1) << __func__ << " checking freelist vs allocated" << dendl;
  {
    // remove bluefs_extents from used set since the freelist doesn't
    // know they are allocated.
    for (auto e = bluefs_extents.begin(); e != bluefs_extents.end(); ++e) {
      apply(
        e.get_start(), e.get_len(), fm->get_alloc_size(), used_blocks,
        [&](uint64_t pos, mempool_dynamic_bitset &bs) {
	assert(pos < bs.size());
	  bs.reset(pos);
        }
      );
    }
    fm->enumerate_reset();
    uint64_t offset, length;
    while (fm->enumerate_next(&offset, &length)) {
      bool intersects = false;
      apply(
        offset, length, fm->get_alloc_size(), used_blocks,
        [&](uint64_t pos, mempool_dynamic_bitset &bs) {
	  assert(pos < bs.size());
          if (bs.test(pos)) {
            intersects = true;
          } else {
	    bs.set(pos);
          }
        }
      );
      if (intersects) {
	if (offset == SUPER_RESERVED &&
	    length == min_alloc_size - SUPER_RESERVED) {
	  // this is due to the change just after luminous to min_alloc_size
	  // granularity allocations, and our baked in assumption at the top
	  // of _fsck that 0~ROUND_UP_TO(SUPER_RESERVED,min_alloc_size) is used
	  // (vs luminous's ROUND_UP_TO(SUPER_RESERVED,block_size)).  harmless,
	  // since we will never allocate this region below min_alloc_size.
	  dout(10) << __func__ << " ignoring free extent between SUPER_RESERVED"
		   << " and min_alloc_size, 0x" << std::hex << offset << "~"
		   << length << dendl;
	} else {
	  derr << "fsck error: free extent 0x" << std::hex << offset
	       << "~" << length << std::dec
	       << " intersects allocated blocks" << dendl;
	  ++errors;
	}
      }
    }
    fm->enumerate_reset();
    size_t count = used_blocks.count();
    if (used_blocks.size() != count) {
      assert(used_blocks.size() > count);
      ++errors;
      used_blocks.flip();
      size_t start = used_blocks.find_first();
      while (start != decltype(used_blocks)::npos) {
	size_t cur = start;
	while (true) {
	  size_t next = used_blocks.find_next(cur);
	  if (next != cur + 1) {
	    derr << "fsck error: leaked extent 0x" << std::hex
		 << ((uint64_t)start * fm->get_alloc_size()) << "~"
		 << ((cur + 1 - start) * fm->get_alloc_size()) << std::dec
		 << dendl;
	    start = next;
	    break;
	  }
	  cur = next;
	}
      }
      used_blocks.flip();
    }
  }

 out_scan:
  mempool_thread.shutdown();
  _flush_cache();
 out_alloc:
  _close_alloc();
 out_fm:
  _close_fm();
 out_db:
  it.reset();  // before db is closed
  _close_db();
 out_bdev:
  _close_bdev();
 out_fsid:
  _close_fsid();
 out_path:
  _close_path();

  // fatal errors take precedence
  if (r < 0)
    return r;

  dout(2) << __func__ << " " << num_objects << " objects, "
	  << num_sharded_objects << " of them sharded.  "
	  << dendl;
  dout(2) << __func__ << " " << num_extents << " extents to "
	  << num_blobs << " blobs, "
	  << num_spanning_blobs << " spanning, "
	  << num_shared_blobs << " shared."
	  << dendl;

  utime_t duration = ceph_clock_now() - start;
  dout(1) << __func__ << " finish with " << errors << " errors, " << repaired
	  << " repaired, " << (errors - repaired) << " remaining in "
	  << duration << " seconds" << dendl;
  return errors - repaired;
}

void BlueStore::collect_metadata(map<string,string> *pm)
{
  dout(10) << __func__ << dendl;
  bdev->collect_metadata("bluestore_bdev_", pm);
  if (bluefs) {
    (*pm)["bluefs"] = "1";
    (*pm)["bluefs_single_shared_device"] = stringify((int)bluefs_single_shared_device);
    bluefs->collect_metadata(pm);
  } else {
    (*pm)["bluefs"] = "0";
  }
}

int BlueStore::statfs(struct store_statfs_t *buf)
{
  buf->reset();
  buf->total = bdev->get_size();
  buf->available = alloc->get_free();

  if (bluefs) {
    // part of our shared device is "free" according to BlueFS, but we
    // can't touch bluestore_bluefs_min of it.
    int64_t shared_available = std::min(
      bluefs->get_free(bluefs_shared_bdev),
      bluefs->get_total(bluefs_shared_bdev) - cct->_conf->bluestore_bluefs_min);
    if (shared_available > 0) {
      buf->available += shared_available;
    }
  }

  {
    std::lock_guard<std::mutex> l(vstatfs_lock);
    
    buf->allocated = vstatfs.allocated();
    buf->stored = vstatfs.stored();
    buf->compressed = vstatfs.compressed();
    buf->compressed_original = vstatfs.compressed_original();
    buf->compressed_allocated = vstatfs.compressed_allocated();
  }

  dout(20) << __func__ << *buf << dendl;
  return 0;
}

// ---------------
// cache

BlueStore::CollectionRef BlueStore::_get_collection(const coll_t& cid)
{
  RWLock::RLocker l(coll_lock);
  ceph::unordered_map<coll_t,CollectionRef>::iterator cp = coll_map.find(cid);
  if (cp == coll_map.end())
    return CollectionRef();
  return cp->second;
}

void BlueStore::_queue_reap_collection(CollectionRef& c)
{
  dout(10) << __func__ << " " << c << " " << c->cid << dendl;
  // _reap_collections and this in the same thread,
  // so no need a lock.
  removed_collections.push_back(c);
}

void BlueStore::_reap_collections()
{

  list<CollectionRef> removed_colls;
  {
    // _queue_reap_collection and this in the same thread.
    // So no need a lock.
    if (!removed_collections.empty())
      removed_colls.swap(removed_collections);
    else
      return;
  }

  list<CollectionRef>::iterator p = removed_colls.begin();
  while (p != removed_colls.end()) {
    CollectionRef c = *p;
    dout(10) << __func__ << " " << c << " " << c->cid << dendl;
    if (c->onode_map.map_any([&](OnodeRef o) {
	  assert(!o->exists);
	  if (o->flushing_count.load()) {
	    dout(10) << __func__ << " " << c << " " << c->cid << " " << o->oid
		     << " flush_txns " << o->flushing_count << dendl;
	    return true;
	  }
	  return false;
	})) {
      ++p;
      continue;
    }
    c->onode_map.clear();
    p = removed_colls.erase(p);
    dout(10) << __func__ << " " << c << " " << c->cid << " done" << dendl;
  }
  if (removed_colls.empty()) {
    dout(10) << __func__ << " all reaped" << dendl;
  } else {
    removed_collections.splice(removed_collections.begin(), removed_colls);
  }
}

void BlueStore::_update_cache_logger()
{
  uint64_t num_onodes = 0;
  uint64_t num_extents = 0;
  uint64_t num_blobs = 0;
  uint64_t num_buffers = 0;
  uint64_t num_buffer_bytes = 0;
  for (auto c : cache_shards) {
    c->add_stats(&num_onodes, &num_extents, &num_blobs,
		 &num_buffers, &num_buffer_bytes);
  }
  logger->set(l_bluestore_onodes, num_onodes);
  logger->set(l_bluestore_extents, num_extents);
  logger->set(l_bluestore_blobs, num_blobs);
  logger->set(l_bluestore_buffers, num_buffers);
  logger->set(l_bluestore_buffer_bytes, num_buffer_bytes);
}

// ---------------
// read operations

ObjectStore::CollectionHandle BlueStore::open_collection(const coll_t& cid)
{
  return _get_collection(cid);
}

bool BlueStore::exists(const coll_t& cid, const ghobject_t& oid)
{
  CollectionHandle c = _get_collection(cid);
  if (!c)
    return false;
  return exists(c, oid);
}

bool BlueStore::exists(CollectionHandle &c_, const ghobject_t& oid)
{
  Collection *c = static_cast<Collection *>(c_.get());
  dout(10) << __func__ << " " << c->cid << " " << oid << dendl;
  if (!c->exists)
    return false;

  bool r = true;

  {
    RWLock::RLocker l(c->lock);
    OnodeRef o = c->get_onode(oid, false);
    if (!o || !o->exists)
      r = false;
  }

  return r;
}

int BlueStore::stat(
    const coll_t& cid,
    const ghobject_t& oid,
    struct stat *st,
    bool allow_eio)
{
  CollectionHandle c = _get_collection(cid);
  if (!c)
    return -ENOENT;
  return stat(c, oid, st, allow_eio);
}

int BlueStore::stat(
  CollectionHandle &c_,
  const ghobject_t& oid,
  struct stat *st,
  bool allow_eio)
{
  Collection *c = static_cast<Collection *>(c_.get());
  if (!c->exists)
    return -ENOENT;
  dout(10) << __func__ << " " << c->get_cid() << " " << oid << dendl;

  {
    RWLock::RLocker l(c->lock);
    OnodeRef o = c->get_onode(oid, false);
    if (!o || !o->exists)
      return -ENOENT;
    st->st_size = o->onode.size;
    st->st_blksize = 4096;
    st->st_blocks = (st->st_size + st->st_blksize - 1) / st->st_blksize;
    st->st_nlink = 1;
  }

  int r = 0;
  if (_debug_mdata_eio(oid)) {
    r = -EIO;
    derr << __func__ << " " << c->cid << " " << oid << " INJECT EIO" << dendl;
  }
  return r;
}
int BlueStore::set_collection_opts(
  const coll_t& cid,
  const pool_opts_t& opts)
{
  CollectionHandle ch = _get_collection(cid);
  if (!ch)
    return -ENOENT;
  Collection *c = static_cast<Collection *>(ch.get());
  dout(15) << __func__ << " " << cid << " options " << opts << dendl;
  if (!c->exists)
    return -ENOENT;
  RWLock::WLocker l(c->lock);
  c->pool_opts = opts;
  return 0;
}

int BlueStore::read(
  const coll_t& cid,
  const ghobject_t& oid,
  uint64_t offset,
  size_t length,
  bufferlist& bl,
  uint32_t op_flags)
{
  CollectionHandle c = _get_collection(cid);
  if (!c)
    return -ENOENT;
  return read(c, oid, offset, length, bl, op_flags);
}

int BlueStore::read(
  CollectionHandle &c_,
  const ghobject_t& oid,
  uint64_t offset,
  size_t length,
  bufferlist& bl,
  uint32_t op_flags)
{
  utime_t start = ceph_clock_now();
  Collection *c = static_cast<Collection *>(c_.get());
  const coll_t &cid = c->get_cid();
  dout(15) << __func__ << " " << cid << " " << oid
	   << " 0x" << std::hex << offset << "~" << length << std::dec
	   << dendl;
  if (!c->exists)
    return -ENOENT;

  bl.clear();
  int r;
  {
    RWLock::RLocker l(c->lock);
    utime_t start1 = ceph_clock_now();
    OnodeRef o = c->get_onode(oid, false);
    logger->tinc(l_bluestore_read_onode_meta_lat, ceph_clock_now() - start1);
    if (!o || !o->exists) {
      r = -ENOENT;
      goto out;
    }

    if (offset == length && offset == 0)
      length = o->onode.size;

    r = _do_read(c, o, offset, length, bl, op_flags);
    if (r == -EIO) {
      logger->inc(l_bluestore_read_eio);
    }
  }

 out:
  if (r >= 0 && _debug_data_eio(oid)) {
    r = -EIO;
    derr << __func__ << " " << c->cid << " " << oid << " INJECT EIO" << dendl;
  } else if (cct->_conf->bluestore_debug_random_read_err &&
    (rand() % (int)(cct->_conf->bluestore_debug_random_read_err * 100.0)) == 0) {
    dout(0) << __func__ << ": inject random EIO" << dendl;
    r = -EIO;
  }
  dout(10) << __func__ << " " << cid << " " << oid
	   << " 0x" << std::hex << offset << "~" << length << std::dec
	   << " = " << r << dendl;
  logger->tinc(l_bluestore_read_lat, ceph_clock_now() - start);
  return r;
}

// --------------------------------------------------------
// intermediate data structures used while reading
struct region_t {
  uint64_t logical_offset;
  uint64_t blob_xoffset;   //region offset within the blob
  uint64_t length;
  bufferlist bl;

  // used later in read process
  uint64_t front = 0;
  uint64_t r_off = 0;

  region_t(uint64_t offset, uint64_t b_offs, uint64_t len)
    : logical_offset(offset),
    blob_xoffset(b_offs),
    length(len){}
  region_t(const region_t& from)
    : logical_offset(from.logical_offset),
    blob_xoffset(from.blob_xoffset),
    length(from.length){}

  friend ostream& operator<<(ostream& out, const region_t& r) {
    return out << "0x" << std::hex << r.logical_offset << ":"
      << r.blob_xoffset << "~" << r.length << std::dec;
  }
};

typedef list<region_t> regions2read_t;
typedef map<BlueStore::BlobRef, regions2read_t> blobs2read_t;

int BlueStore::_do_read(
  Collection *c,
  OnodeRef o,
  uint64_t offset,
  size_t length,
  bufferlist& bl,
  uint32_t op_flags)
{
  FUNCTRACE();
  int r = 0;

  dout(20) << __func__ << " 0x" << std::hex << offset << "~" << length
           << " size 0x" << o->onode.size << " (" << std::dec
           << o->onode.size << ")" << dendl;
  bl.clear();

  if (offset >= o->onode.size) {
    return r;
  }

  // generally, don't buffer anything, unless the client explicitly requests
  // it.
  bool buffered = false;
  if (op_flags & CEPH_OSD_OP_FLAG_FADVISE_WILLNEED) {
    dout(20) << __func__ << " will do buffered read" << dendl;
    buffered = true;
  } else if (cct->_conf->bluestore_default_buffered_read &&
	     (op_flags & (CEPH_OSD_OP_FLAG_FADVISE_DONTNEED |
			  CEPH_OSD_OP_FLAG_FADVISE_NOCACHE)) == 0) {
    dout(20) << __func__ << " defaulting to buffered read" << dendl;
    buffered = true;
  }

  if (offset + length > o->onode.size) {
    length = o->onode.size - offset;
  }

  utime_t start = ceph_clock_now();
  o->extent_map.fault_range(db, offset, length);
  logger->tinc(l_bluestore_read_onode_meta_lat, ceph_clock_now() - start);
  _dump_onode(o);

  ready_regions_t ready_regions;

  // build blob-wise list to of stuff read (that isn't cached)
  blobs2read_t blobs2read;
  unsigned left = length;
  uint64_t pos = offset;
  unsigned num_regions = 0;
  auto lp = o->extent_map.seek_lextent(offset);
  while (left > 0 && lp != o->extent_map.extent_map.end()) {
    if (pos < lp->logical_offset) {
      unsigned hole = lp->logical_offset - pos;
      if (hole >= left) {
	break;
      }
      dout(30) << __func__ << "  hole 0x" << std::hex << pos << "~" << hole
	       << std::dec << dendl;
      pos += hole;
      left -= hole;
    }
    BlobRef& bptr = lp->blob;
    unsigned l_off = pos - lp->logical_offset;
    unsigned b_off = l_off + lp->blob_offset;
    unsigned b_len = std::min(left, lp->length - l_off);

    ready_regions_t cache_res;
    interval_set<uint32_t> cache_interval;
    bptr->shared_blob->bc.read(
      bptr->shared_blob->get_cache(), b_off, b_len, cache_res, cache_interval);
    dout(20) << __func__ << "  blob " << *bptr << std::hex
	     << " need 0x" << b_off << "~" << b_len
	     << " cache has 0x" << cache_interval
	     << std::dec << dendl;

    auto pc = cache_res.begin();
    while (b_len > 0) {
      unsigned l;
      if (pc != cache_res.end() &&
	  pc->first == b_off) {
	l = pc->second.length();
	ready_regions[pos].claim(pc->second);
	dout(30) << __func__ << "    use cache 0x" << std::hex << pos << ": 0x"
		 << b_off << "~" << l << std::dec << dendl;
	++pc;
      } else {
	l = b_len;
	if (pc != cache_res.end()) {
	  assert(pc->first > b_off);
	  l = pc->first - b_off;
	}
	dout(30) << __func__ << "    will read 0x" << std::hex << pos << ": 0x"
		 << b_off << "~" << l << std::dec << dendl;
	blobs2read[bptr].emplace_back(region_t(pos, b_off, l));
	++num_regions;
      }
      pos += l;
      b_off += l;
      left -= l;
      b_len -= l;
    }
    ++lp;
  }

  // read raw blob data.  use aio if we have >1 blobs to read.
  start = ceph_clock_now(); // for the sake of simplicity 
                                    // measure the whole block below.
                                    // The error isn't that much...
  vector<bufferlist> compressed_blob_bls;
  IOContext ioc(cct, NULL, true); // allow EIO
  for (auto& p : blobs2read) {
    const BlobRef& bptr = p.first;
    dout(20) << __func__ << "  blob " << *bptr << std::hex
	     << " need " << p.second << std::dec << dendl;
    if (bptr->get_blob().is_compressed()) {
      // read the whole thing
      if (compressed_blob_bls.empty()) {
	// ensure we avoid any reallocation on subsequent blobs
	compressed_blob_bls.reserve(blobs2read.size());
      }
      compressed_blob_bls.push_back(bufferlist());
      bufferlist& bl = compressed_blob_bls.back();
      r = bptr->get_blob().map(
	0, bptr->get_blob().get_ondisk_length(),
	[&](uint64_t offset, uint64_t length) {
	  int r;
	  // use aio if there are more regions to read than those in this blob
	  if (num_regions > p.second.size()) {
	    r = bdev->aio_read(offset, length, &bl, &ioc);
	  } else {
	    r = bdev->read(offset, length, &bl, &ioc, false);
	  }
	  if (r < 0)
            return r;
          return 0;
	});
      if (r < 0) {
        derr << __func__ << " bdev-read failed: " << cpp_strerror(r) << dendl;
        if (r == -EIO) {
          // propagate EIO to caller
          return r;
        }
        assert(r == 0);
      }
    } else {
      // read the pieces
      for (auto& reg : p.second) {
	// determine how much of the blob to read
	uint64_t chunk_size = bptr->get_blob().get_chunk_size(block_size);
	reg.r_off = reg.blob_xoffset;
	uint64_t r_len = reg.length;
	reg.front = reg.r_off % chunk_size;
	if (reg.front) {
	  reg.r_off -= reg.front;
	  r_len += reg.front;
	}
	unsigned tail = r_len % chunk_size;
	if (tail) {
	  r_len += chunk_size - tail;
	}
	dout(20) << __func__ << "    region 0x" << std::hex
		 << reg.logical_offset
		 << ": 0x" << reg.blob_xoffset << "~" << reg.length
		 << " reading 0x" << reg.r_off << "~" << r_len << std::dec
		 << dendl;

	// read it
	r = bptr->get_blob().map(
	  reg.r_off, r_len,
	  [&](uint64_t offset, uint64_t length) {
	    int r;
	    // use aio if there is more than one region to read
	    if (num_regions > 1) {
	      r = bdev->aio_read(offset, length, &reg.bl, &ioc);
	    } else {
	      r = bdev->read(offset, length, &reg.bl, &ioc, false);
	    }
	    if (r < 0)
              return r;
            return 0;
	  });
        if (r < 0) {
          derr << __func__ << " bdev-read failed: " << cpp_strerror(r)
               << dendl;
          if (r == -EIO) {
            // propagate EIO to caller
            return r;
          }
          assert(r == 0);
        }
	assert(reg.bl.length() == r_len);
      }
    }
  }
  if (ioc.has_pending_aios()) {
    bdev->aio_submit(&ioc);
    dout(20) << __func__ << " waiting for aio" << dendl;
    ioc.aio_wait();
    r = ioc.get_return_value();
    if (r < 0) {
      assert(r == -EIO); // no other errors allowed
      return -EIO;
    }
  }
  logger->tinc(l_bluestore_read_wait_aio_lat, ceph_clock_now() - start);

  // enumerate and decompress desired blobs
  auto p = compressed_blob_bls.begin();
  blobs2read_t::iterator b2r_it = blobs2read.begin();
  while (b2r_it != blobs2read.end()) {
    const BlobRef& bptr = b2r_it->first;
    dout(20) << __func__ << "  blob " << *bptr << std::hex
	     << " need 0x" << b2r_it->second << std::dec << dendl;
    if (bptr->get_blob().is_compressed()) {
      assert(p != compressed_blob_bls.end());
      bufferlist& compressed_bl = *p++;
      if (_verify_csum(o, &bptr->get_blob(), 0, compressed_bl,
		       b2r_it->second.front().logical_offset) < 0) {
	return -EIO;
      }
      bufferlist raw_bl;
      r = _decompress(compressed_bl, &raw_bl);
      if (r < 0)
	return r;
      if (buffered) {
	bptr->shared_blob->bc.did_read(bptr->shared_blob->get_cache(), 0,
				       raw_bl);
      }
      for (auto& i : b2r_it->second) {
	ready_regions[i.logical_offset].substr_of(
	  raw_bl, i.blob_xoffset, i.length);
      }
    } else {
      for (auto& reg : b2r_it->second) {
	if (_verify_csum(o, &bptr->get_blob(), reg.r_off, reg.bl,
			 reg.logical_offset) < 0) {
	  return -EIO;
	}
	if (buffered) {
	  bptr->shared_blob->bc.did_read(bptr->shared_blob->get_cache(),
					 reg.r_off, reg.bl);
	}

	// prune and keep result
	ready_regions[reg.logical_offset].substr_of(
	  reg.bl, reg.front, reg.length);
      }
    }
    ++b2r_it;
  }

  // generate a resulting buffer
  auto pr = ready_regions.begin();
  auto pr_end = ready_regions.end();
  pos = 0;
  while (pos < length) {
    if (pr != pr_end && pr->first == pos + offset) {
      dout(30) << __func__ << " assemble 0x" << std::hex << pos
	       << ": data from 0x" << pr->first << "~" << pr->second.length()
	       << std::dec << dendl;
      pos += pr->second.length();
      bl.claim_append(pr->second);
      ++pr;
    } else {
      uint64_t l = length - pos;
      if (pr != pr_end) {
        assert(pr->first > pos + offset);
	l = pr->first - (pos + offset);
      }
      dout(30) << __func__ << " assemble 0x" << std::hex << pos
	       << ": zeros for 0x" << (pos + offset) << "~" << l
	       << std::dec << dendl;
      bl.append_zero(l);
      pos += l;
    }
  }
  assert(bl.length() == length);
  assert(pos == length);
  assert(pr == pr_end);
  r = bl.length();
  return r;
}

int BlueStore::_verify_csum(OnodeRef& o,
			    const bluestore_blob_t* blob, uint64_t blob_xoffset,
			    const bufferlist& bl,
			    uint64_t logical_offset) const
{
  int bad;
  uint64_t bad_csum;
  utime_t start = ceph_clock_now();
  int r = blob->verify_csum(blob_xoffset, bl, &bad, &bad_csum);
  if (r < 0) {
    if (r == -1) {
      PExtentVector pex;
      blob->map(
	bad,
	blob->get_csum_chunk_size(),
	[&](uint64_t offset, uint64_t length) {
	  pex.emplace_back(bluestore_pextent_t(offset, length));
          return 0;
	});
      derr << __func__ << " bad "
	   << Checksummer::get_csum_type_string(blob->csum_type)
	   << "/0x" << std::hex << blob->get_csum_chunk_size()
	   << " checksum at blob offset 0x" << bad
	   << ", got 0x" << bad_csum << ", expected 0x"
	   << blob->get_csum_item(bad / blob->get_csum_chunk_size()) << std::dec
	   << ", device location " << pex
	   << ", logical extent 0x" << std::hex
	   << (logical_offset + bad - blob_xoffset) << "~"
	   << blob->get_csum_chunk_size() << std::dec
	   << ", object " << o->oid
	   << dendl;
    } else {
      derr << __func__ << " failed with exit code: " << cpp_strerror(r) << dendl;
    }
  }
  logger->tinc(l_bluestore_csum_lat, ceph_clock_now() - start);
  return r;
}

int BlueStore::_decompress(bufferlist& source, bufferlist* result)
{
  int r = 0;
  utime_t start = ceph_clock_now();
  bufferlist::iterator i = source.begin();
  bluestore_compression_header_t chdr;
  ::decode(chdr, i);
  int alg = int(chdr.type);
  CompressorRef cp = compressor;
  if (!cp || (int)cp->get_type() != alg) {
    cp = Compressor::create(cct, alg);
  }

  if (!cp.get()) {
    // if compressor isn't available - error, because cannot return
    // decompressed data?
    derr << __func__ << " can't load decompressor " << alg << dendl;
    r = -EIO;
  } else {
    r = cp->decompress(i, chdr.length, *result);
    if (r < 0) {
      derr << __func__ << " decompression failed with exit code " << r << dendl;
      r = -EIO;
    }
  }
  logger->tinc(l_bluestore_decompress_lat, ceph_clock_now() - start);
  return r;
}

// this stores fiemap into interval_set, other variations
// use it internally
int BlueStore::_fiemap(
  CollectionHandle &c_,
  const ghobject_t& oid,
  uint64_t offset,
  size_t length,
  interval_set<uint64_t>& destset)
{
  Collection *c = static_cast<Collection *>(c_.get());
  if (!c->exists)
    return -ENOENT;
  {
    RWLock::RLocker l(c->lock);

    OnodeRef o = c->get_onode(oid, false);
    if (!o || !o->exists) {
      return -ENOENT;
    }
    _dump_onode(o);

    dout(20) << __func__ << " 0x" << std::hex << offset << "~" << length
	     << " size 0x" << o->onode.size << std::dec << dendl;

    boost::intrusive::set<Extent>::iterator ep, eend;
    if (offset >= o->onode.size)
      goto out;

    if (offset + length > o->onode.size) {
      length = o->onode.size - offset;
    }

    o->extent_map.fault_range(db, offset, length);
    eend = o->extent_map.extent_map.end();
    ep = o->extent_map.seek_lextent(offset);
    while (length > 0) {
      dout(20) << __func__ << " offset " << offset << dendl;
      if (ep != eend && ep->logical_offset + ep->length <= offset) {
        ++ep;
        continue;
      }

      uint64_t x_len = length;
      if (ep != eend && ep->logical_offset <= offset) {
        uint64_t x_off = offset - ep->logical_offset;
        x_len = MIN(x_len, ep->length - x_off);
        dout(30) << __func__ << " lextent 0x" << std::hex << offset << "~"
	         << x_len << std::dec << " blob " << ep->blob << dendl;
        destset.insert(offset, x_len);
        length -= x_len;
        offset += x_len;
        if (x_off + x_len == ep->length)
	  ++ep;
        continue;
      }
      if (ep != eend &&
	  ep->logical_offset > offset &&
	  ep->logical_offset - offset < x_len) {
        x_len = ep->logical_offset - offset;
      }
      offset += x_len;
      length -= x_len;
    }
  }

 out:
  dout(20) << __func__ << " 0x" << std::hex << offset << "~" << length
	   << " size = 0x(" << destset << ")" << std::dec << dendl;
  return 0;
}

int BlueStore::fiemap(
  const coll_t& cid,
  const ghobject_t& oid,
  uint64_t offset,
  size_t len,
  bufferlist& bl)
{
  CollectionHandle c = _get_collection(cid);
  if (!c)
    return -ENOENT;
  return fiemap(c, oid, offset, len, bl);
}

int BlueStore::fiemap(
  CollectionHandle &c_,
  const ghobject_t& oid,
  uint64_t offset,
  size_t length,
  bufferlist& bl)
{
  interval_set<uint64_t> m;
  int r = _fiemap(c_, oid, offset, length, m);
  if (r >= 0) {
    ::encode(m, bl);
  }
  return r;
}

int BlueStore::fiemap(
  const coll_t& cid,
  const ghobject_t& oid,
  uint64_t offset,
  size_t len,
  map<uint64_t, uint64_t>& destmap)
{
  CollectionHandle c = _get_collection(cid);
  if (!c)
    return -ENOENT;
  return fiemap(c, oid, offset, len, destmap);
}

int BlueStore::fiemap(
  CollectionHandle &c_,
  const ghobject_t& oid,
  uint64_t offset,
  size_t length,
  map<uint64_t, uint64_t>& destmap)
{
  interval_set<uint64_t> m;
  int r = _fiemap(c_, oid, offset, length, m);
  if (r >= 0) {
    m.move_into(destmap);
  }
  return r;
}

int BlueStore::getattr(
  const coll_t& cid,
  const ghobject_t& oid,
  const char *name,
  bufferptr& value)
{
  CollectionHandle c = _get_collection(cid);
  if (!c)
    return -ENOENT;
  return getattr(c, oid, name, value);
}

int BlueStore::getattr(
  CollectionHandle &c_,
  const ghobject_t& oid,
  const char *name,
  bufferptr& value)
{
  Collection *c = static_cast<Collection *>(c_.get());
  dout(15) << __func__ << " " << c->cid << " " << oid << " " << name << dendl;
  if (!c->exists)
    return -ENOENT;

  int r;
  {
    RWLock::RLocker l(c->lock);
    mempool::bluestore_cache_other::string k(name);

    OnodeRef o = c->get_onode(oid, false);
    if (!o || !o->exists) {
      r = -ENOENT;
      goto out;
    }

    if (!o->onode.attrs.count(k)) {
      r = -ENODATA;
      goto out;
    }
    value = o->onode.attrs[k];
    r = 0;
  }
 out:
  if (r == 0 && _debug_mdata_eio(oid)) {
    r = -EIO;
    derr << __func__ << " " << c->cid << " " << oid << " INJECT EIO" << dendl;
  }
  dout(10) << __func__ << " " << c->cid << " " << oid << " " << name
	   << " = " << r << dendl;
  return r;
}


int BlueStore::getattrs(
  const coll_t& cid,
  const ghobject_t& oid,
  map<string,bufferptr>& aset)
{
  CollectionHandle c = _get_collection(cid);
  if (!c)
    return -ENOENT;
  return getattrs(c, oid, aset);
}

int BlueStore::getattrs(
  CollectionHandle &c_,
  const ghobject_t& oid,
  map<string,bufferptr>& aset)
{
  Collection *c = static_cast<Collection *>(c_.get());
  dout(15) << __func__ << " " << c->cid << " " << oid << dendl;
  if (!c->exists)
    return -ENOENT;

  int r;
  {
    RWLock::RLocker l(c->lock);

    OnodeRef o = c->get_onode(oid, false);
    if (!o || !o->exists) {
      r = -ENOENT;
      goto out;
    }
    for (auto& i : o->onode.attrs) {
      aset.emplace(i.first.c_str(), i.second);
    }
    r = 0;
  }

 out:
  if (r == 0 && _debug_mdata_eio(oid)) {
    r = -EIO;
    derr << __func__ << " " << c->cid << " " << oid << " INJECT EIO" << dendl;
  }
  dout(10) << __func__ << " " << c->cid << " " << oid
	   << " = " << r << dendl;
  return r;
}

int BlueStore::list_collections(vector<coll_t>& ls)
{
  RWLock::RLocker l(coll_lock);
  for (ceph::unordered_map<coll_t, CollectionRef>::iterator p = coll_map.begin();
       p != coll_map.end();
       ++p)
    ls.push_back(p->first);
  return 0;
}

bool BlueStore::collection_exists(const coll_t& c)
{
  RWLock::RLocker l(coll_lock);
  return coll_map.count(c);
}

int BlueStore::collection_empty(const coll_t& cid, bool *empty)
{
  dout(15) << __func__ << " " << cid << dendl;
  vector<ghobject_t> ls;
  ghobject_t next;
  int r = collection_list(cid, ghobject_t(), ghobject_t::get_max(), 1,
			  &ls, &next);
  if (r < 0) {
    derr << __func__ << " collection_list returned: " << cpp_strerror(r)
         << dendl;
    return r;
  }
  *empty = ls.empty();
  dout(10) << __func__ << " " << cid << " = " << (int)(*empty) << dendl;
  return 0;
}

int BlueStore::collection_bits(const coll_t& cid)
{
  dout(15) << __func__ << " " << cid << dendl;
  CollectionRef c = _get_collection(cid);
  if (!c)
    return -ENOENT;
  RWLock::RLocker l(c->lock);
  dout(10) << __func__ << " " << cid << " = " << c->cnode.bits << dendl;
  return c->cnode.bits;
}

int BlueStore::collection_list(
  const coll_t& cid, const ghobject_t& start, const ghobject_t& end, int max,
  vector<ghobject_t> *ls, ghobject_t *pnext)
{
  CollectionHandle c = _get_collection(cid);
  if (!c)
    return -ENOENT;
  return collection_list(c, start, end, max, ls, pnext);
}

int BlueStore::collection_list(
  CollectionHandle &c_, const ghobject_t& start, const ghobject_t& end, int max,
  vector<ghobject_t> *ls, ghobject_t *pnext)
{
  Collection *c = static_cast<Collection *>(c_.get());
  dout(15) << __func__ << " " << c->cid
           << " start " << start << " end " << end << " max " << max << dendl;
  int r;
  {
    RWLock::RLocker l(c->lock);
    r = _collection_list(c, start, end, max, ls, pnext);
  }

  dout(10) << __func__ << " " << c->cid
    << " start " << start << " end " << end << " max " << max
    << " = " << r << ", ls.size() = " << ls->size()
    << ", next = " << (pnext ? *pnext : ghobject_t())  << dendl;
  return r;
}

int BlueStore::_collection_list(
  Collection *c, const ghobject_t& start, const ghobject_t& end, int max,
  vector<ghobject_t> *ls, ghobject_t *pnext)
{

  if (!c->exists)
    return -ENOENT;

  int r = 0;
  ghobject_t static_next;
  KeyValueDB::Iterator it;
  string temp_start_key, temp_end_key;
  string start_key, end_key;
  bool set_next = false;
  string pend;
  bool temp;

  if (!pnext)
    pnext = &static_next;

  if (start == ghobject_t::get_max() ||
    start.hobj.is_max()) {
    goto out;
  }
  get_coll_key_range(c->cid, c->cnode.bits, &temp_start_key, &temp_end_key,
    &start_key, &end_key);
  dout(20) << __func__
    << " range " << pretty_binary_string(temp_start_key)
    << " to " << pretty_binary_string(temp_end_key)
    << " and " << pretty_binary_string(start_key)
    << " to " << pretty_binary_string(end_key)
    << " start " << start << dendl;
  it = db->get_iterator(PREFIX_OBJ);
  if (start == ghobject_t() ||
    start.hobj == hobject_t() ||
    start == c->cid.get_min_hobj()) {
    it->upper_bound(temp_start_key);
    temp = true;
  } else {
    string k;
    get_object_key(cct, start, &k);
    if (start.hobj.is_temp()) {
      temp = true;
      assert(k >= temp_start_key && k < temp_end_key);
    } else {
      temp = false;
      assert(k >= start_key && k < end_key);
    }
    dout(20) << " start from " << pretty_binary_string(k)
      << " temp=" << (int)temp << dendl;
    it->lower_bound(k);
  }
  if (end.hobj.is_max()) {
    pend = temp ? temp_end_key : end_key;
  } else {
    get_object_key(cct, end, &end_key);
    if (end.hobj.is_temp()) {
      if (temp)
	pend = end_key;
      else
	goto out;
    } else {
      pend = temp ? temp_end_key : end_key;
    }
  }
  dout(20) << __func__ << " pend " << pretty_binary_string(pend) << dendl;
  while (true) {
    if (!it->valid() || it->key() >= pend) {
      if (!it->valid())
	dout(20) << __func__ << " iterator not valid (end of db?)" << dendl;
      else
	dout(20) << __func__ << " key " << pretty_binary_string(it->key())
	         << " >= " << end << dendl;
      if (temp) {
	if (end.hobj.is_temp()) {
	  break;
	}
	dout(30) << __func__ << " switch to non-temp namespace" << dendl;
	temp = false;
	it->upper_bound(start_key);
	pend = end_key;
	dout(30) << __func__ << " pend " << pretty_binary_string(pend) << dendl;
	continue;
      }
      break;
    }
    dout(30) << __func__ << " key " << pretty_binary_string(it->key()) << dendl;
    if (is_extent_shard_key(it->key())) {
      it->next();
      continue;
    }
    ghobject_t oid;
    int r = get_key_object(it->key(), &oid);
    assert(r == 0);
    dout(20) << __func__ << " oid " << oid << " end " << end << dendl;
    if (ls->size() >= (unsigned)max) {
      dout(20) << __func__ << " reached max " << max << dendl;
      *pnext = oid;
      set_next = true;
      break;
    }
    ls->push_back(oid);
    it->next();
  }
out:
  if (!set_next) {
    *pnext = ghobject_t::get_max();
  }

  return r;
}

int BlueStore::omap_get(
  const coll_t& cid,                ///< [in] Collection containing oid
  const ghobject_t &oid,   ///< [in] Object containing omap
  bufferlist *header,      ///< [out] omap header
  map<string, bufferlist> *out /// < [out] Key to value map
  )
{
  CollectionHandle c = _get_collection(cid);
  if (!c)
    return -ENOENT;
  return omap_get(c, oid, header, out);
}

int BlueStore::omap_get(
  CollectionHandle &c_,    ///< [in] Collection containing oid
  const ghobject_t &oid,   ///< [in] Object containing omap
  bufferlist *header,      ///< [out] omap header
  map<string, bufferlist> *out /// < [out] Key to value map
  )
{
  Collection *c = static_cast<Collection *>(c_.get());
  dout(15) << __func__ << " " << c->get_cid() << " oid " << oid << dendl;
  if (!c->exists)
    return -ENOENT;
  RWLock::RLocker l(c->lock);
  int r = 0;
  OnodeRef o = c->get_onode(oid, false);
  if (!o || !o->exists) {
    r = -ENOENT;
    goto out;
  }
  if (!o->onode.has_omap())
    goto out;
  o->flush();
  {
    KeyValueDB::Iterator it = db->get_iterator(PREFIX_OMAP);
    string head, tail;
    get_omap_header(o->onode.nid, &head);
    get_omap_tail(o->onode.nid, &tail);
    it->lower_bound(head);
    while (it->valid()) {
      if (it->key() == head) {
	dout(30) << __func__ << "  got header" << dendl;
	*header = it->value();
      } else if (it->key() >= tail) {
	dout(30) << __func__ << "  reached tail" << dendl;
	break;
      } else {
	string user_key;
	decode_omap_key(it->key(), &user_key);
	dout(30) << __func__ << "  got " << pretty_binary_string(it->key())
		 << " -> " << user_key << dendl;
	(*out)[user_key] = it->value();
      }
      it->next();
    }
  }
 out:
  dout(10) << __func__ << " " << c->get_cid() << " oid " << oid << " = " << r
	   << dendl;
  return r;
}

int BlueStore::omap_get_header(
  const coll_t& cid,                ///< [in] Collection containing oid
  const ghobject_t &oid,   ///< [in] Object containing omap
  bufferlist *header,      ///< [out] omap header
  bool allow_eio ///< [in] don't assert on eio
  )
{
  CollectionHandle c = _get_collection(cid);
  if (!c)
    return -ENOENT;
  return omap_get_header(c, oid, header, allow_eio);
}

int BlueStore::omap_get_header(
  CollectionHandle &c_,                ///< [in] Collection containing oid
  const ghobject_t &oid,   ///< [in] Object containing omap
  bufferlist *header,      ///< [out] omap header
  bool allow_eio ///< [in] don't assert on eio
  )
{
  Collection *c = static_cast<Collection *>(c_.get());
  dout(15) << __func__ << " " << c->get_cid() << " oid " << oid << dendl;
  if (!c->exists)
    return -ENOENT;
  RWLock::RLocker l(c->lock);
  int r = 0;
  OnodeRef o = c->get_onode(oid, false);
  if (!o || !o->exists) {
    r = -ENOENT;
    goto out;
  }
  if (!o->onode.has_omap())
    goto out;
  o->flush();
  {
    string head;
    get_omap_header(o->onode.nid, &head);
    if (db->get(PREFIX_OMAP, head, header) >= 0) {
      dout(30) << __func__ << "  got header" << dendl;
    } else {
      dout(30) << __func__ << "  no header" << dendl;
    }
  }
 out:
  dout(10) << __func__ << " " << c->get_cid() << " oid " << oid << " = " << r
	   << dendl;
  return r;
}

int BlueStore::omap_get_keys(
  const coll_t& cid,              ///< [in] Collection containing oid
  const ghobject_t &oid, ///< [in] Object containing omap
  set<string> *keys      ///< [out] Keys defined on oid
  )
{
  CollectionHandle c = _get_collection(cid);
  if (!c)
    return -ENOENT;
  return omap_get_keys(c, oid, keys);
}

int BlueStore::omap_get_keys(
  CollectionHandle &c_,              ///< [in] Collection containing oid
  const ghobject_t &oid, ///< [in] Object containing omap
  set<string> *keys      ///< [out] Keys defined on oid
  )
{
  Collection *c = static_cast<Collection *>(c_.get());
  dout(15) << __func__ << " " << c->get_cid() << " oid " << oid << dendl;
  if (!c->exists)
    return -ENOENT;
  RWLock::RLocker l(c->lock);
  int r = 0;
  OnodeRef o = c->get_onode(oid, false);
  if (!o || !o->exists) {
    r = -ENOENT;
    goto out;
  }
  if (!o->onode.has_omap())
    goto out;
  o->flush();
  {
    KeyValueDB::Iterator it = db->get_iterator(PREFIX_OMAP);
    string head, tail;
    get_omap_key(o->onode.nid, string(), &head);
    get_omap_tail(o->onode.nid, &tail);
    it->lower_bound(head);
    while (it->valid()) {
      if (it->key() >= tail) {
	dout(30) << __func__ << "  reached tail" << dendl;
	break;
      }
      string user_key;
      decode_omap_key(it->key(), &user_key);
      dout(30) << __func__ << "  got " << pretty_binary_string(it->key())
	       << " -> " << user_key << dendl;
      keys->insert(user_key);
      it->next();
    }
  }
 out:
  dout(10) << __func__ << " " << c->get_cid() << " oid " << oid << " = " << r
	   << dendl;
  return r;
}

int BlueStore::omap_get_values(
  const coll_t& cid,                    ///< [in] Collection containing oid
  const ghobject_t &oid,       ///< [in] Object containing omap
  const set<string> &keys,     ///< [in] Keys to get
  map<string, bufferlist> *out ///< [out] Returned keys and values
  )
{
  CollectionHandle c = _get_collection(cid);
  if (!c)
    return -ENOENT;
  return omap_get_values(c, oid, keys, out);
}

int BlueStore::omap_get_values(
  CollectionHandle &c_,        ///< [in] Collection containing oid
  const ghobject_t &oid,       ///< [in] Object containing omap
  const set<string> &keys,     ///< [in] Keys to get
  map<string, bufferlist> *out ///< [out] Returned keys and values
  )
{
  Collection *c = static_cast<Collection *>(c_.get());
  dout(15) << __func__ << " " << c->get_cid() << " oid " << oid << dendl;
  if (!c->exists)
    return -ENOENT;
  RWLock::RLocker l(c->lock);
  int r = 0;
  string final_key;
  OnodeRef o = c->get_onode(oid, false);
  if (!o || !o->exists) {
    r = -ENOENT;
    goto out;
  }
  if (!o->onode.has_omap())
    goto out;
  o->flush();
  _key_encode_u64(o->onode.nid, &final_key);
  final_key.push_back('.');
  for (set<string>::const_iterator p = keys.begin(); p != keys.end(); ++p) {
    final_key.resize(9); // keep prefix
    final_key += *p;
    bufferlist val;
    if (db->get(PREFIX_OMAP, final_key, &val) >= 0) {
      dout(30) << __func__ << "  got " << pretty_binary_string(final_key)
	       << " -> " << *p << dendl;
      out->insert(make_pair(*p, val));
    }
  }
 out:
  dout(10) << __func__ << " " << c->get_cid() << " oid " << oid << " = " << r
	   << dendl;
  return r;
}

int BlueStore::omap_check_keys(
  const coll_t& cid,                ///< [in] Collection containing oid
  const ghobject_t &oid,   ///< [in] Object containing omap
  const set<string> &keys, ///< [in] Keys to check
  set<string> *out         ///< [out] Subset of keys defined on oid
  )
{
  CollectionHandle c = _get_collection(cid);
  if (!c)
    return -ENOENT;
  return omap_check_keys(c, oid, keys, out);
}

int BlueStore::omap_check_keys(
  CollectionHandle &c_,    ///< [in] Collection containing oid
  const ghobject_t &oid,   ///< [in] Object containing omap
  const set<string> &keys, ///< [in] Keys to check
  set<string> *out         ///< [out] Subset of keys defined on oid
  )
{
  Collection *c = static_cast<Collection *>(c_.get());
  dout(15) << __func__ << " " << c->get_cid() << " oid " << oid << dendl;
  if (!c->exists)
    return -ENOENT;
  RWLock::RLocker l(c->lock);
  int r = 0;
  string final_key;
  OnodeRef o = c->get_onode(oid, false);
  if (!o || !o->exists) {
    r = -ENOENT;
    goto out;
  }
  if (!o->onode.has_omap())
    goto out;
  o->flush();
  _key_encode_u64(o->onode.nid, &final_key);
  final_key.push_back('.');
  for (set<string>::const_iterator p = keys.begin(); p != keys.end(); ++p) {
    final_key.resize(9); // keep prefix
    final_key += *p;
    bufferlist val;
    if (db->get(PREFIX_OMAP, final_key, &val) >= 0) {
      dout(30) << __func__ << "  have " << pretty_binary_string(final_key)
	       << " -> " << *p << dendl;
      out->insert(*p);
    } else {
      dout(30) << __func__ << "  miss " << pretty_binary_string(final_key)
	       << " -> " << *p << dendl;
    }
  }
 out:
  dout(10) << __func__ << " " << c->get_cid() << " oid " << oid << " = " << r
	   << dendl;
  return r;
}

ObjectMap::ObjectMapIterator BlueStore::get_omap_iterator(
  const coll_t& cid,              ///< [in] collection
  const ghobject_t &oid  ///< [in] object
  )
{
  CollectionHandle c = _get_collection(cid);
  if (!c) {
    dout(10) << __func__ << " " << cid << "doesn't exist" <<dendl;
    return ObjectMap::ObjectMapIterator();
  }
  return get_omap_iterator(c, oid);
}

ObjectMap::ObjectMapIterator BlueStore::get_omap_iterator(
  CollectionHandle &c_,              ///< [in] collection
  const ghobject_t &oid  ///< [in] object
  )
{
  Collection *c = static_cast<Collection *>(c_.get());
  dout(10) << __func__ << " " << c->get_cid() << " " << oid << dendl;
  if (!c->exists) {
    return ObjectMap::ObjectMapIterator();
  }
  RWLock::RLocker l(c->lock);
  OnodeRef o = c->get_onode(oid, false);
  if (!o || !o->exists) {
    dout(10) << __func__ << " " << oid << "doesn't exist" <<dendl;
    return ObjectMap::ObjectMapIterator();
  }
  o->flush();
  dout(10) << __func__ << " has_omap = " << (int)o->onode.has_omap() <<dendl;
  KeyValueDB::Iterator it = db->get_iterator(PREFIX_OMAP);
  return ObjectMap::ObjectMapIterator(new OmapIteratorImpl(c, o, it));
}

// -----------------
// write helpers

void BlueStore::_prepare_ondisk_format_super(KeyValueDB::Transaction& t)
{
  dout(10) << __func__ << " ondisk_format " << ondisk_format
	   << " min_compat_ondisk_format " << min_compat_ondisk_format
	   << dendl;
  assert(ondisk_format == latest_ondisk_format);
  {
    bufferlist bl;
    ::encode(ondisk_format, bl);
    t->set(PREFIX_SUPER, "ondisk_format", bl);
  }
  {
    bufferlist bl;
    ::encode(min_compat_ondisk_format, bl);
    t->set(PREFIX_SUPER, "min_compat_ondisk_format", bl);
  }
}

int BlueStore::_open_super_meta()
{
  // nid
  {
    nid_max = 0;
    bufferlist bl;
    db->get(PREFIX_SUPER, "nid_max", &bl);
    bufferlist::iterator p = bl.begin();
    try {
      uint64_t v;
      ::decode(v, p);
      nid_max = v;
    } catch (buffer::error& e) {
      derr << __func__ << " unable to read nid_max" << dendl;
      return -EIO;
    }
    dout(10) << __func__ << " old nid_max " << nid_max << dendl;
    nid_last = nid_max.load();
  }

  // blobid
  {
    blobid_max = 0;
    bufferlist bl;
    db->get(PREFIX_SUPER, "blobid_max", &bl);
    bufferlist::iterator p = bl.begin();
    try {
      uint64_t v;
      ::decode(v, p);
      blobid_max = v;
    } catch (buffer::error& e) {
      derr << __func__ << " unable to read blobid_max" << dendl;
      return -EIO;
    }
    dout(10) << __func__ << " old blobid_max " << blobid_max << dendl;
    blobid_last = blobid_max.load();
  }

  // freelist
  {
    bufferlist bl;
    db->get(PREFIX_SUPER, "freelist_type", &bl);
    if (bl.length()) {
      freelist_type = std::string(bl.c_str(), bl.length());
      dout(10) << __func__ << " freelist_type " << freelist_type << dendl;
    } else {
      assert("Not Support extent freelist manager" == 0);
    }
  }

  // bluefs alloc
  if (cct->_conf->bluestore_bluefs) {
    bluefs_extents.clear();
    bufferlist bl;
    db->get(PREFIX_SUPER, "bluefs_extents", &bl);
    bufferlist::iterator p = bl.begin();
    try {
      ::decode(bluefs_extents, p);
    }
    catch (buffer::error& e) {
      derr << __func__ << " unable to read bluefs_extents" << dendl;
      return -EIO;
    }
    dout(10) << __func__ << " bluefs_extents 0x" << std::hex << bluefs_extents
	     << std::dec << dendl;
  }

  // ondisk format
  int32_t compat_ondisk_format = 0;
  {
    bufferlist bl;
    int r = db->get(PREFIX_SUPER, "ondisk_format", &bl);
    if (r < 0) {
      // base case: kraken bluestore is v1 and readable by v1
      dout(20) << __func__ << " missing ondisk_format; assuming kraken"
	       << dendl;
      ondisk_format = 1;
      compat_ondisk_format = 1;
    } else {
      auto p = bl.begin();
      try {
	::decode(ondisk_format, p);
      } catch (buffer::error& e) {
	derr << __func__ << " unable to read ondisk_format" << dendl;
	return -EIO;
      }
      bl.clear();
      {
	r = db->get(PREFIX_SUPER, "min_compat_ondisk_format", &bl);
	assert(!r);
	auto p = bl.begin();
	try {
	  ::decode(compat_ondisk_format, p);
	} catch (buffer::error& e) {
	  derr << __func__ << " unable to read compat_ondisk_format" << dendl;
	  return -EIO;
	}
      }
    }
    dout(10) << __func__ << " ondisk_format " << ondisk_format
	     << " compat_ondisk_format " << compat_ondisk_format
	     << dendl;
  }

  if (latest_ondisk_format < compat_ondisk_format) {
    derr << __func__ << " compat_ondisk_format is "
	 << compat_ondisk_format << " but we only understand version "
	 << latest_ondisk_format << dendl;
    return -EPERM;
  }
  if (ondisk_format < latest_ondisk_format) {
    int r = _upgrade_super();
    if (r < 0) {
      return r;
    }
  }

  {
    bufferlist bl;
    db->get(PREFIX_SUPER, "min_alloc_size", &bl);
    auto p = bl.begin();
    try {
      uint64_t val;
      ::decode(val, p);
      min_alloc_size = val;
      min_alloc_size_order = ctz(val);
      assert(min_alloc_size == 1u << min_alloc_size_order);
    } catch (buffer::error& e) {
      derr << __func__ << " unable to read min_alloc_size" << dendl;
      return -EIO;
    }
    dout(10) << __func__ << " min_alloc_size 0x" << std::hex << min_alloc_size
	     << std::dec << dendl;
  }
  _open_statfs();
  _set_alloc_sizes();
  _set_throttle_params();

  _set_csum();
  _set_compression();
  _set_blob_size();

  return 0;
}

int BlueStore::_upgrade_super()
{
  dout(1) << __func__ << " from " << ondisk_format << ", latest "
	  << latest_ondisk_format << dendl;
  assert(ondisk_format > 0);
  assert(ondisk_format < latest_ondisk_format);

  if (ondisk_format == 1) {
    // changes:
    // - super: added ondisk_format
    // - super: added min_readable_ondisk_format
    // - super: added min_compat_ondisk_format
    // - super: added min_alloc_size
    // - super: removed min_min_alloc_size
    KeyValueDB::Transaction t = db->get_transaction();
    {
      bufferlist bl;
      db->get(PREFIX_SUPER, "min_min_alloc_size", &bl);
      auto p = bl.begin();
      try {
	uint64_t val;
	::decode(val, p);
	min_alloc_size = val;
      } catch (buffer::error& e) {
	derr << __func__ << " failed to read min_min_alloc_size" << dendl;
	return -EIO;
      }
      t->set(PREFIX_SUPER, "min_alloc_size", bl);
      t->rmkey(PREFIX_SUPER, "min_min_alloc_size");
    }
    ondisk_format = 2;
    _prepare_ondisk_format_super(t);
    int r = db->submit_transaction_sync(t);
    assert(r == 0);
  }

  // done
  dout(1) << __func__ << " done" << dendl;
  return 0;
}

void BlueStore::_assign_nid(TransContext *txc, OnodeRef o)
{
  if (o->onode.nid) {
    assert(o->exists);
    return;
  }
  uint64_t nid = ++nid_last;
  dout(20) << __func__ << " " << nid << dendl;
  o->onode.nid = nid;
  txc->last_nid = nid;
  o->exists = true;
}

uint64_t BlueStore::_assign_blobid(TransContext *txc)
{
  uint64_t bid = ++blobid_last;
  dout(20) << __func__ << " " << bid << dendl;
  txc->last_blobid = bid;
  return bid;
}

void BlueStore::get_db_statistics(Formatter *f)
{
  db->get_statistics(f);
}

BlueStore::TransContext *BlueStore::_txc_create(OpSequencer *osr)
{
  TransContext *txc = new TransContext(cct, osr);
  txc->t = db->get_transaction();
  osr->queue_new(txc);
  dout(20) << __func__ << " osr " << osr << " = " << txc
	   << " seq " << txc->seq << dendl;
  return txc;
}

void BlueStore::_txc_calc_cost(TransContext *txc)
{
  // this is about the simplest model for transaction cost you can
  // imagine.  there is some fixed overhead cost by saying there is a
  // minimum of one "io".  and then we have some cost per "io" that is
  // a configurable (with different hdd and ssd defaults), and add
  // that to the bytes value.
  int ios = 1;  // one "io" for the kv commit
  for (auto& p : txc->ioc.pending_aios) {
    ios += p.iov.size();
  }
  auto cost = throttle_cost_per_io.load();
  txc->cost = ios * cost + txc->bytes;
  dout(10) << __func__ << " " << txc << " cost " << txc->cost << " ("
	   << ios << " ios * " << cost << " + " << txc->bytes
	   << " bytes)" << dendl;
}

void BlueStore::_txc_update_store_statfs(TransContext *txc)
{
  if (txc->statfs_delta.is_empty())
    return;

  logger->inc(l_bluestore_allocated, txc->statfs_delta.allocated());
  logger->inc(l_bluestore_stored, txc->statfs_delta.stored());
  logger->inc(l_bluestore_compressed, txc->statfs_delta.compressed());
  logger->inc(l_bluestore_compressed_allocated, txc->statfs_delta.compressed_allocated());
  logger->inc(l_bluestore_compressed_original, txc->statfs_delta.compressed_original());

  {
    std::lock_guard<std::mutex> l(vstatfs_lock);
    vstatfs += txc->statfs_delta;
  }

  bufferlist bl;
  txc->statfs_delta.encode(bl);

  txc->t->merge(PREFIX_STAT, "bluestore_statfs", bl);
  txc->statfs_delta.reset();
}

void BlueStore::_txc_state_proc(TransContext *txc)
{
  while (true) {
    dout(10) << __func__ << " txc " << txc
	     << " " << txc->get_state_name() << dendl;
    switch (txc->state) {
    case TransContext::STATE_PREPARE:
      txc->log_state_latency(logger, l_bluestore_state_prepare_lat);
      if (txc->ioc.has_pending_aios()) {
	txc->state = TransContext::STATE_AIO_WAIT;
	txc->had_ios = true;
	_txc_aio_submit(txc);
	return;
      }
      // ** fall-thru **

    case TransContext::STATE_AIO_WAIT:
      txc->log_state_latency(logger, l_bluestore_state_aio_wait_lat);
      _txc_finish_io(txc);  // may trigger blocked txc's too
      return;

    case TransContext::STATE_IO_DONE:
      //assert(txc->osr->qlock.is_locked());  // see _txc_finish_io
      if (txc->had_ios) {
	++txc->osr->txc_with_unstable_io;
      }
      txc->log_state_latency(logger, l_bluestore_state_io_done_lat);
      txc->state = TransContext::STATE_KV_QUEUED;
      if (cct->_conf->bluestore_sync_submit_transaction) {
	if (txc->last_nid >= nid_max ||
	    txc->last_blobid >= blobid_max) {
	  dout(20) << __func__
		   << " last_{nid,blobid} exceeds max, submit via kv thread"
		   << dendl;
	} else if (txc->osr->kv_committing_serially) {
	  dout(20) << __func__ << " prior txc submitted via kv thread, us too"
		   << dendl;
	  // note: this is starvation-prone.  once we have a txc in a busy
	  // sequencer that is committing serially it is possible to keep
	  // submitting new transactions fast enough that we get stuck doing
	  // so.  the alternative is to block here... fixme?
	} else if (txc->osr->txc_with_unstable_io) {
	  dout(20) << __func__ << " prior txc(s) with unstable ios "
		   << txc->osr->txc_with_unstable_io.load() << dendl;
	} else if (cct->_conf->bluestore_debug_randomize_serial_transaction &&
		   rand() % cct->_conf->bluestore_debug_randomize_serial_transaction
		   == 0) {
	  dout(20) << __func__ << " DEBUG randomly forcing submit via kv thread"
		   << dendl;
	} else {
	  txc->state = TransContext::STATE_KV_SUBMITTED;
	  int r = cct->_conf->bluestore_debug_omit_kv_commit ? 0 : db->submit_transaction(txc->t);
	  assert(r == 0);
	  _txc_applied_kv(txc);
	}
      }
      {
	std::lock_guard<std::mutex> l(kv_lock);
	kv_queue.push_back(txc);
	kv_cond.notify_one();
	if (txc->state != TransContext::STATE_KV_SUBMITTED) {
	  kv_queue_unsubmitted.push_back(txc);
	  ++txc->osr->kv_committing_serially;
	}
	if (txc->had_ios)
	  kv_ios++;
	kv_throttle_costs += txc->cost;
      }
      return;
    case TransContext::STATE_KV_SUBMITTED:
      txc->log_state_latency(logger, l_bluestore_state_kv_committing_lat);
      txc->state = TransContext::STATE_KV_DONE;
      _txc_committed_kv(txc);
      // ** fall-thru **

    case TransContext::STATE_KV_DONE:
      txc->log_state_latency(logger, l_bluestore_state_kv_done_lat);
      if (txc->deferred_txn) {
	txc->state = TransContext::STATE_DEFERRED_QUEUED;
	_deferred_queue(txc);
	return;
      }
      txc->state = TransContext::STATE_FINISHING;
      break;

    case TransContext::STATE_DEFERRED_CLEANUP:
      txc->log_state_latency(logger, l_bluestore_state_deferred_cleanup_lat);
      txc->state = TransContext::STATE_FINISHING;
      // ** fall-thru **

    case TransContext::STATE_FINISHING:
      txc->log_state_latency(logger, l_bluestore_state_finishing_lat);
      _txc_finish(txc);
      return;

    default:
      derr << __func__ << " unexpected txc " << txc
	   << " state " << txc->get_state_name() << dendl;
      assert(0 == "unexpected txc state");
      return;
    }
  }
}

void BlueStore::_txc_finish_io(TransContext *txc)
{
  dout(20) << __func__ << " " << txc << dendl;

  /*
   * we need to preserve the order of kv transactions,
   * even though aio will complete in any order.
   */

  OpSequencer *osr = txc->osr.get();
  std::lock_guard<std::mutex> l(osr->qlock);
  txc->state = TransContext::STATE_IO_DONE;

  // release aio contexts (including pinned buffers).
  txc->ioc.running_aios.clear();

  OpSequencer::q_list_t::iterator p = osr->q.iterator_to(*txc);
  while (p != osr->q.begin()) {
    --p;
    if (p->state < TransContext::STATE_IO_DONE) {
      dout(20) << __func__ << " " << txc << " blocked by " << &*p << " "
	       << p->get_state_name() << dendl;
      return;
    }
    if (p->state > TransContext::STATE_IO_DONE) {
      ++p;
      break;
    }
  }
  do {
    _txc_state_proc(&*p++);
  } while (p != osr->q.end() &&
	   p->state == TransContext::STATE_IO_DONE);

  if (osr->kv_submitted_waiters &&
      osr->_is_all_kv_submitted()) {
    osr->qcond.notify_all();
  }
}

void BlueStore::_txc_write_nodes(TransContext *txc, KeyValueDB::Transaction t)
{
  dout(20) << __func__ << " txc " << txc
	   << " onodes " << txc->onodes
	   << " shared_blobs " << txc->shared_blobs
	   << dendl;

  // finalize onodes
  for (auto o : txc->onodes) {
    // finalize extent_map shards
    o->extent_map.update(t, false);
    if (o->extent_map.needs_reshard()) {
      o->extent_map.reshard(db, t);
      o->extent_map.update(t, true);
      if (o->extent_map.needs_reshard()) {
	dout(20) << __func__ << " warning: still wants reshard, check options?"
		 << dendl;
	o->extent_map.clear_needs_reshard();
      }
      logger->inc(l_bluestore_onode_reshard);
    }

    // bound encode
    size_t bound = 0;
    denc(o->onode, bound);
    o->extent_map.bound_encode_spanning_blobs(bound);
    if (o->onode.extent_map_shards.empty()) {
      denc(o->extent_map.inline_bl, bound);
    }

    // encode
    bufferlist bl;
    unsigned onode_part, blob_part, extent_part;
    {
      auto p = bl.get_contiguous_appender(bound, true);
      denc(o->onode, p);
      onode_part = p.get_logical_offset();
      o->extent_map.encode_spanning_blobs(p);
      blob_part = p.get_logical_offset() - onode_part;
      if (o->onode.extent_map_shards.empty()) {
	denc(o->extent_map.inline_bl, p);
      }
      extent_part = p.get_logical_offset() - onode_part - blob_part;
    }

    dout(20) << "  onode " << o->oid << " is " << bl.length()
	     << " (" << onode_part << " bytes onode + "
	     << blob_part << " bytes spanning blobs + "
	     << extent_part << " bytes inline extents)"
	     << dendl;
    t->set(PREFIX_OBJ, o->key.c_str(), o->key.size(), bl);
    o->flushing_count++;
  }

  // objects we modified but didn't affect the onode
  auto p = txc->modified_objects.begin();
  while (p != txc->modified_objects.end()) {
    if (txc->onodes.count(*p) == 0) {
      (*p)->flushing_count++;
      ++p;
    } else {
      // remove dups with onodes list to avoid problems in _txc_finish
      p = txc->modified_objects.erase(p);
    }
  }

  // finalize shared_blobs
  for (auto sb : txc->shared_blobs) {
    string key;
    auto sbid = sb->get_sbid();
    get_shared_blob_key(sbid, &key);
    if (sb->persistent->empty()) {
      dout(20) << "  shared_blob 0x" << std::hex << sbid << std::dec
	       << " is empty" << dendl;
      t->rmkey(PREFIX_SHARED_BLOB, key);
    } else {
      bufferlist bl;
      ::encode(*(sb->persistent), bl);
      dout(20) << "  shared_blob 0x" << std::hex << sbid << std::dec
	       << " is " << bl.length() << " " << *sb << dendl;
      t->set(PREFIX_SHARED_BLOB, key, bl);
    }
  }
}

void BlueStore::BSPerfTracker::update_from_perfcounters(
  PerfCounters &logger)
{
  os_commit_latency.consume_next(
    logger.get_tavg_ms(
      l_bluestore_commit_lat));
  os_apply_latency.consume_next(
    logger.get_tavg_ms(
      l_bluestore_commit_lat));
}

void BlueStore::_txc_finalize_kv(TransContext *txc, KeyValueDB::Transaction t)
{
  dout(20) << __func__ << " txc " << txc << std::hex
	   << " allocated 0x" << txc->allocated
	   << " released 0x" << txc->released
	   << std::dec << dendl;

  // We have to handle the case where we allocate *and* deallocate the
  // same region in this transaction.  The freelist doesn't like that.
  // (Actually, the only thing that cares is the BitmapFreelistManager
  // debug check. But that's important.)
  interval_set<uint64_t> tmp_allocated, tmp_released;
  interval_set<uint64_t> *pallocated = &txc->allocated;
  interval_set<uint64_t> *preleased = &txc->released;
  if (!txc->allocated.empty() && !txc->released.empty()) {
    interval_set<uint64_t> overlap;
    overlap.intersection_of(txc->allocated, txc->released);
    if (!overlap.empty()) {
      tmp_allocated = txc->allocated;
      tmp_allocated.subtract(overlap);
      tmp_released = txc->released;
      tmp_released.subtract(overlap);
      dout(20) << __func__ << "  overlap 0x" << std::hex << overlap
	       << ", new allocated 0x" << tmp_allocated
	       << " released 0x" << tmp_released << std::dec
	       << dendl;
      pallocated = &tmp_allocated;
      preleased = &tmp_released;
    }
  }

  // update freelist with non-overlap sets
  for (interval_set<uint64_t>::iterator p = pallocated->begin();
       p != pallocated->end();
       ++p) {
    fm->allocate(p.get_start(), p.get_len(), t);
  }
  for (interval_set<uint64_t>::iterator p = preleased->begin();
       p != preleased->end();
       ++p) {
    dout(20) << __func__ << " release 0x" << std::hex << p.get_start()
	     << "~" << p.get_len() << std::dec << dendl;
    fm->release(p.get_start(), p.get_len(), t);
  }

  _txc_update_store_statfs(txc);
}

void BlueStore::_txc_applied_kv(TransContext *txc)
{
  for (auto ls : { &txc->onodes, &txc->modified_objects }) {
    for (auto& o : *ls) {
      dout(20) << __func__ << " onode " << o << " had " << o->flushing_count
	       << dendl;
      if (--o->flushing_count == 0) {
        std::lock_guard<std::mutex> l(o->flush_lock);
	o->flush_cond.notify_all();
      }
    }
  }
}

void BlueStore::_txc_committed_kv(TransContext *txc)
{
  dout(20) << __func__ << " txc " << txc << dendl;

  // warning: we're calling onreadable_sync inside the sequencer lock
  if (txc->onreadable_sync) {
    txc->onreadable_sync->complete(0);
    txc->onreadable_sync = NULL;
  }
  unsigned n = txc->osr->parent->shard_hint.hash_to_shard(m_finisher_num);
  if (txc->oncommit) {
    logger->tinc(l_bluestore_commit_lat, ceph_clock_now() - txc->start);
    finishers[n]->queue(txc->oncommit);
    txc->oncommit = NULL;
  }
  if (txc->onreadable) {
    finishers[n]->queue(txc->onreadable);
    txc->onreadable = NULL;
  }

  if (!txc->oncommits.empty()) {
    finishers[n]->queue(txc->oncommits);
  }
}

void BlueStore::_txc_finish(TransContext *txc)
{
  dout(20) << __func__ << " " << txc << " onodes " << txc->onodes << dendl;
  assert(txc->state == TransContext::STATE_FINISHING);

  for (auto& sb : txc->shared_blobs_written) {
    sb->bc.finish_write(sb->get_cache(), txc->seq);
  }
  txc->shared_blobs_written.clear();

  while (!txc->removed_collections.empty()) {
    _queue_reap_collection(txc->removed_collections.front());
    txc->removed_collections.pop_front();
  }

  OpSequencerRef osr = txc->osr;
  bool empty = false;
  bool submit_deferred = false;
  OpSequencer::q_list_t releasing_txc;
  {
    std::lock_guard<std::mutex> l(osr->qlock);
    txc->state = TransContext::STATE_DONE;
    bool notify = false;
    while (!osr->q.empty()) {
      TransContext *txc = &osr->q.front();
      dout(20) << __func__ << "  txc " << txc << " " << txc->get_state_name()
	       << dendl;
      if (txc->state != TransContext::STATE_DONE) {
	if (txc->state == TransContext::STATE_PREPARE &&
	  deferred_aggressive) {
	  // for _osr_drain_preceding()
          notify = true;
	}
	if (txc->state == TransContext::STATE_DEFERRED_QUEUED &&
	    osr->q.size() > g_conf->bluestore_max_deferred_txc) {
	  submit_deferred = true;
	}
        break;
      }

      osr->q.pop_front();
      releasing_txc.push_back(*txc);
      notify = true;
    }
    if (notify) {
      osr->qcond.notify_all();
    }
    if (osr->q.empty()) {
      dout(20) << __func__ << " osr " << osr << " q now empty" << dendl;
      empty = true;
    }
  }
  while (!releasing_txc.empty()) {
    // release to allocator only after all preceding txc's have also
    // finished any deferred writes that potentially land in these
    // blocks
    auto txc = &releasing_txc.front();
    _txc_release_alloc(txc);
    releasing_txc.pop_front();
    txc->log_state_latency(logger, l_bluestore_state_done_lat);
    delete txc;
  }

  if (submit_deferred) {
    // we're pinning memory; flush!  we could be more fine-grained here but
    // i'm not sure it's worth the bother.
    deferred_try_submit();
  }

  if (empty && osr->zombie) {
    dout(10) << __func__ << " reaping empty zombie osr " << osr << dendl;
    osr->_unregister();
  }
}

void BlueStore::_txc_release_alloc(TransContext *txc)
{
  // update allocator with full released set
  if (!cct->_conf->bluestore_debug_no_reuse_blocks) {
    dout(10) << __func__ << " " << txc << " " << std::hex
             << txc->released << std::dec << dendl;
    for (interval_set<uint64_t>::iterator p = txc->released.begin();
	 p != txc->released.end();
	 ++p) {
      alloc->release(p.get_start(), p.get_len());
    }
  }

  txc->allocated.clear();
  txc->released.clear();
}

void BlueStore::_osr_drain_preceding(TransContext *txc)
{
  OpSequencer *osr = txc->osr.get();
  dout(10) << __func__ << " " << txc << " osr " << osr << dendl;
  ++deferred_aggressive; // FIXME: maybe osr-local aggressive flag?
  {
    // submit anything pending
    deferred_lock.lock();
    if (osr->deferred_pending) {
      _deferred_submit_unlock(osr);
    } else {
      deferred_lock.unlock();
    }
  }
  {
    // wake up any previously finished deferred events
    std::lock_guard<std::mutex> l(kv_lock);
    kv_cond.notify_one();
  }
  osr->drain_preceding(txc);
  --deferred_aggressive;
  dout(10) << __func__ << " " << osr << " done" << dendl;
}

void BlueStore::_osr_drain_all()
{
  dout(10) << __func__ << dendl;

  set<OpSequencerRef> s;
  {
    std::lock_guard<std::mutex> l(osr_lock);
    s = osr_set;
  }
  dout(20) << __func__ << " osr_set " << s << dendl;

  ++deferred_aggressive;
  {
    // submit anything pending
    deferred_try_submit();
  }
  {
    // wake up any previously finished deferred events
    std::lock_guard<std::mutex> l(kv_lock);
    kv_cond.notify_one();
  }
  {
    std::lock_guard<std::mutex> l(kv_finalize_lock);
    kv_finalize_cond.notify_one();
  }
  for (auto osr : s) {
    dout(20) << __func__ << " drain " << osr << dendl;
    osr->drain();
  }
  --deferred_aggressive;

  dout(10) << __func__ << " done" << dendl;
}

void BlueStore::_osr_unregister_all()
{
  set<OpSequencerRef> s;
  {
    std::lock_guard<std::mutex> l(osr_lock);
    s = osr_set;
  }
  dout(10) << __func__ << " " << s << dendl;
  for (auto osr : s) {
    osr->_unregister();

    if (!osr->zombie) {
      // break link from Sequencer to us so that this OpSequencer
      // instance can die with this mount/umount cycle.  note that
      // we assume umount() will not race against ~Sequencer.
      assert(osr->parent);
      osr->parent->p.reset();
    }
  }
  // nobody should be creating sequencers during umount either.
  {
    std::lock_guard<std::mutex> l(osr_lock);
    assert(osr_set.empty());
  }
}

void BlueStore::_kv_start()
{
  dout(10) << __func__ << dendl;

  if (cct->_conf->bluestore_shard_finishers) {
    if (cct->_conf->osd_op_num_shards) {
      m_finisher_num = cct->_conf->osd_op_num_shards;
    } else {
      assert(bdev);
      if (bdev->is_rotational()) {
        m_finisher_num = cct->_conf->osd_op_num_shards_hdd;
      } else {
        m_finisher_num = cct->_conf->osd_op_num_shards_ssd;
      }
    }
  }

  assert(m_finisher_num != 0);

  for (int i = 0; i < m_finisher_num; ++i) {
    ostringstream oss;
    oss << "finisher-" << i;
    Finisher *f = new Finisher(cct, oss.str(), "finisher");
    finishers.push_back(f);
  }

  deferred_finisher.start();
  for (auto f : finishers) {
    f->start();
  }
  kv_sync_thread.create("bstore_kv_sync");
  kv_finalize_thread.create("bstore_kv_final");
}

void BlueStore::_kv_stop()
{
  dout(10) << __func__ << dendl;
  {
    std::unique_lock<std::mutex> l(kv_lock);
    while (!kv_sync_started) {
      kv_cond.wait(l);
    }
    kv_stop = true;
    kv_cond.notify_all();
  }
  {
    std::unique_lock<std::mutex> l(kv_finalize_lock);
    while (!kv_finalize_started) {
      kv_finalize_cond.wait(l);
    }
    kv_finalize_stop = true;
    kv_finalize_cond.notify_all();
  }
  kv_sync_thread.join();
  kv_finalize_thread.join();
  assert(removed_collections.empty());
  {
    std::lock_guard<std::mutex> l(kv_lock);
    kv_stop = false;
  }
  {
    std::lock_guard<std::mutex> l(kv_finalize_lock);
    kv_finalize_stop = false;
  }
  dout(10) << __func__ << " stopping finishers" << dendl;
  deferred_finisher.wait_for_empty();
  deferred_finisher.stop();
  for (auto f : finishers) {
    f->wait_for_empty();
    f->stop();
  }
  dout(10) << __func__ << " stopped" << dendl;
}

void BlueStore::_kv_sync_thread()
{
  dout(10) << __func__ << " start" << dendl;
  std::unique_lock<std::mutex> l(kv_lock);
  assert(!kv_sync_started);
  kv_sync_started = true;
  kv_cond.notify_all();
  while (true) {
    assert(kv_committing.empty());
    if (kv_queue.empty() &&
	((deferred_done_queue.empty() && deferred_stable_queue.empty()) ||
	 !deferred_aggressive)) {
      if (kv_stop)
	break;
      dout(20) << __func__ << " sleep" << dendl;
      kv_cond.wait(l);
      dout(20) << __func__ << " wake" << dendl;
    } else {
      deque<TransContext*> kv_submitting;
      deque<DeferredBatch*> deferred_done, deferred_stable;
      uint64_t aios = 0, costs = 0;

      dout(20) << __func__ << " committing " << kv_queue.size()
	       << " submitting " << kv_queue_unsubmitted.size()
	       << " deferred done " << deferred_done_queue.size()
	       << " stable " << deferred_stable_queue.size()
	       << dendl;
      kv_committing.swap(kv_queue);
      kv_submitting.swap(kv_queue_unsubmitted);
      deferred_done.swap(deferred_done_queue);
      deferred_stable.swap(deferred_stable_queue);
      aios = kv_ios;
      costs = kv_throttle_costs;
      kv_ios = 0;
      kv_throttle_costs = 0;
      utime_t start = ceph_clock_now();
      l.unlock();

      dout(30) << __func__ << " committing " << kv_committing << dendl;
      dout(30) << __func__ << " submitting " << kv_submitting << dendl;
      dout(30) << __func__ << " deferred_done " << deferred_done << dendl;
      dout(30) << __func__ << " deferred_stable " << deferred_stable << dendl;

      bool force_flush = false;
      // if bluefs is sharing the same device as data (only), then we
      // can rely on the bluefs commit to flush the device and make
      // deferred aios stable.  that means that if we do have done deferred
      // txcs AND we are not on a single device, we need to force a flush.
      if (bluefs_single_shared_device && bluefs) {
	if (aios) {
	  force_flush = true;
	} else if (kv_committing.empty() && kv_submitting.empty() &&
		   deferred_stable.empty()) {
	  force_flush = true;  // there's nothing else to commit!
	} else if (deferred_aggressive) {
	  force_flush = true;
	}
      } else
	force_flush = true;

      if (force_flush) {
	dout(20) << __func__ << " num_aios=" << aios
		 << " force_flush=" << (int)force_flush
		 << ", flushing, deferred done->stable" << dendl;
	// flush/barrier on block device
	bdev->flush();

	// if we flush then deferred done are now deferred stable
	deferred_stable.insert(deferred_stable.end(), deferred_done.begin(),
			       deferred_done.end());
	deferred_done.clear();
      }
      utime_t after_flush = ceph_clock_now();

      // we will use one final transaction to force a sync
      KeyValueDB::Transaction synct = db->get_transaction();

      // increase {nid,blobid}_max?  note that this covers both the
      // case where we are approaching the max and the case we passed
      // it.  in either case, we increase the max in the earlier txn
      // we submit.
      uint64_t new_nid_max = 0, new_blobid_max = 0;
      if (nid_last + cct->_conf->bluestore_nid_prealloc/2 > nid_max) {
	KeyValueDB::Transaction t =
	  kv_submitting.empty() ? synct : kv_submitting.front()->t;
	new_nid_max = nid_last + cct->_conf->bluestore_nid_prealloc;
	bufferlist bl;
	::encode(new_nid_max, bl);
	t->set(PREFIX_SUPER, "nid_max", bl);
	dout(10) << __func__ << " new_nid_max " << new_nid_max << dendl;
      }
      if (blobid_last + cct->_conf->bluestore_blobid_prealloc/2 > blobid_max) {
	KeyValueDB::Transaction t =
	  kv_submitting.empty() ? synct : kv_submitting.front()->t;
	new_blobid_max = blobid_last + cct->_conf->bluestore_blobid_prealloc;
	bufferlist bl;
	::encode(new_blobid_max, bl);
	t->set(PREFIX_SUPER, "blobid_max", bl);
	dout(10) << __func__ << " new_blobid_max " << new_blobid_max << dendl;
      }

      for (auto txc : kv_committing) {
	if (txc->state == TransContext::STATE_KV_QUEUED) {
	  txc->log_state_latency(logger, l_bluestore_state_kv_queued_lat);
	  int r = cct->_conf->bluestore_debug_omit_kv_commit ? 0 : db->submit_transaction(txc->t);
	  assert(r == 0);
	  _txc_applied_kv(txc);
	  --txc->osr->kv_committing_serially;
	  txc->state = TransContext::STATE_KV_SUBMITTED;
	  if (txc->osr->kv_submitted_waiters) {
	    std::lock_guard<std::mutex> l(txc->osr->qlock);
	    if (txc->osr->_is_all_kv_submitted()) {
	      txc->osr->qcond.notify_all();
	    }
	  }

	} else {
	  assert(txc->state == TransContext::STATE_KV_SUBMITTED);
	  txc->log_state_latency(logger, l_bluestore_state_kv_queued_lat);
	}
	if (txc->had_ios) {
	  --txc->osr->txc_with_unstable_io;
	}
      }

      // release throttle *before* we commit.  this allows new ops
      // to be prepared and enter pipeline while we are waiting on
      // the kv commit sync/flush.  then hopefully on the next
      // iteration there will already be ops awake.  otherwise, we
      // end up going to sleep, and then wake up when the very first
      // transaction is ready for commit.
      throttle_bytes.put(costs);

      PExtentVector bluefs_gift_extents;
      if (bluefs &&
	  after_flush - bluefs_last_balance >
	  cct->_conf->bluestore_bluefs_balance_interval) {
	bluefs_last_balance = after_flush;
	int r = _balance_bluefs_freespace(&bluefs_gift_extents);
	assert(r >= 0);
	if (r > 0) {
	  for (auto& p : bluefs_gift_extents) {
	    bluefs_extents.insert(p.offset, p.length);
	  }
	  bufferlist bl;
	  ::encode(bluefs_extents, bl);
	  dout(10) << __func__ << " bluefs_extents now 0x" << std::hex
		   << bluefs_extents << std::dec << dendl;
	  synct->set(PREFIX_SUPER, "bluefs_extents", bl);
	}
      }

      // cleanup sync deferred keys
      for (auto b : deferred_stable) {
	for (auto& txc : b->txcs) {
	  bluestore_deferred_transaction_t& wt = *txc.deferred_txn;
	  if (!wt.released.empty()) {
	    // kraken replay compat only
	    txc.released = wt.released;
	    dout(10) << __func__ << " deferred txn has released "
		     << txc.released
		     << " (we just upgraded from kraken) on " << &txc << dendl;
	    _txc_finalize_kv(&txc, synct);
	  }
	  // cleanup the deferred
	  string key;
	  get_deferred_key(wt.seq, &key);
	  synct->rm_single_key(PREFIX_DEFERRED, key);
	}
      }

      // submit synct synchronously (block and wait for it to commit)
      int r = cct->_conf->bluestore_debug_omit_kv_commit ? 0 : db->submit_transaction_sync(synct);
      assert(r == 0);

      if (new_nid_max) {
	nid_max = new_nid_max;
	dout(10) << __func__ << " nid_max now " << nid_max << dendl;
      }
      if (new_blobid_max) {
	blobid_max = new_blobid_max;
	dout(10) << __func__ << " blobid_max now " << blobid_max << dendl;
      }

      {
	utime_t finish = ceph_clock_now();
	utime_t dur_flush = after_flush - start;
	utime_t dur_kv = finish - after_flush;
	utime_t dur = finish - start;
	dout(20) << __func__ << " committed " << kv_committing.size()
	  << " cleaned " << deferred_stable.size()
	  << " in " << dur
	  << " (" << dur_flush << " flush + " << dur_kv << " kv commit)"
	  << dendl;
	logger->tinc(l_bluestore_kv_flush_lat, dur_flush);
	logger->tinc(l_bluestore_kv_commit_lat, dur_kv);
	logger->tinc(l_bluestore_kv_lat, dur);
      }

      if (bluefs) {
	if (!bluefs_gift_extents.empty()) {
	  _commit_bluefs_freespace(bluefs_gift_extents);
	}
	for (auto p = bluefs_extents_reclaiming.begin();
	     p != bluefs_extents_reclaiming.end();
	     ++p) {
	  dout(20) << __func__ << " releasing old bluefs 0x" << std::hex
		   << p.get_start() << "~" << p.get_len() << std::dec
		   << dendl;
	  alloc->release(p.get_start(), p.get_len());
	}
	bluefs_extents_reclaiming.clear();
      }

      {
	std::unique_lock<std::mutex> m(kv_finalize_lock);
	if (kv_committing_to_finalize.empty()) {
	  kv_committing_to_finalize.swap(kv_committing);
	} else {
	  kv_committing_to_finalize.insert(
	    kv_committing_to_finalize.end(),
	    kv_committing.begin(),
	    kv_committing.end());
	  kv_committing.clear();
	}
	if (deferred_stable_to_finalize.empty()) {
	  deferred_stable_to_finalize.swap(deferred_stable);
	} else {
	  deferred_stable_to_finalize.insert(
	    deferred_stable_to_finalize.end(),
	    deferred_stable.begin(),
	    deferred_stable.end());
          deferred_stable.clear();
	}
	kv_finalize_cond.notify_one();
      }

      l.lock();
      // previously deferred "done" are now "stable" by virtue of this
      // commit cycle.
      deferred_stable_queue.swap(deferred_done);
    }
  }
  dout(10) << __func__ << " finish" << dendl;
  kv_sync_started = false;
}

void BlueStore::_kv_finalize_thread()
{
  deque<TransContext*> kv_committed;
  deque<DeferredBatch*> deferred_stable;
  dout(10) << __func__ << " start" << dendl;
  std::unique_lock<std::mutex> l(kv_finalize_lock);
  assert(!kv_finalize_started);
  kv_finalize_started = true;
  kv_finalize_cond.notify_all();
  while (true) {
    assert(kv_committed.empty());
    assert(deferred_stable.empty());
    if (kv_committing_to_finalize.empty() &&
	deferred_stable_to_finalize.empty()) {
      if (kv_finalize_stop)
	break;
      dout(20) << __func__ << " sleep" << dendl;
      kv_finalize_cond.wait(l);
      dout(20) << __func__ << " wake" << dendl;
    } else {
      kv_committed.swap(kv_committing_to_finalize);
      deferred_stable.swap(deferred_stable_to_finalize);
      l.unlock();
      dout(20) << __func__ << " kv_committed " << kv_committed << dendl;
      dout(20) << __func__ << " deferred_stable " << deferred_stable << dendl;

      while (!kv_committed.empty()) {
	TransContext *txc = kv_committed.front();
	assert(txc->state == TransContext::STATE_KV_SUBMITTED);
	_txc_state_proc(txc);
	kv_committed.pop_front();
      }

      for (auto b : deferred_stable) {
	auto p = b->txcs.begin();
	while (p != b->txcs.end()) {
	  TransContext *txc = &*p;
	  p = b->txcs.erase(p); // unlink here because
	  _txc_state_proc(txc); // this may destroy txc
	}
	delete b;
      }
      deferred_stable.clear();

      if (!deferred_aggressive) {
	if (deferred_queue_size >= deferred_batch_ops.load() ||
	    throttle_deferred_bytes.past_midpoint()) {
	  deferred_try_submit();
	}
      }

      // this is as good a place as any ...
      _reap_collections();

      l.lock();
    }
  }
  dout(10) << __func__ << " finish" << dendl;
  kv_finalize_started = false;
}

bluestore_deferred_op_t *BlueStore::_get_deferred_op(
  TransContext *txc, OnodeRef o)
{
  if (!txc->deferred_txn) {
    txc->deferred_txn = new bluestore_deferred_transaction_t;
  }
  txc->deferred_txn->ops.push_back(bluestore_deferred_op_t());
  return &txc->deferred_txn->ops.back();
}

void BlueStore::_deferred_queue(TransContext *txc)
{
  dout(20) << __func__ << " txc " << txc << " osr " << txc->osr << dendl;
  deferred_lock.lock();
  if (!txc->osr->deferred_pending &&
      !txc->osr->deferred_running) {
    deferred_queue.push_back(*txc->osr);
  }
  if (!txc->osr->deferred_pending) {
    txc->osr->deferred_pending = new DeferredBatch(cct, txc->osr.get());
  }
  ++deferred_queue_size;
  txc->osr->deferred_pending->txcs.push_back(*txc);
  bluestore_deferred_transaction_t& wt = *txc->deferred_txn;
  for (auto opi = wt.ops.begin(); opi != wt.ops.end(); ++opi) {
    const auto& op = *opi;
    assert(op.op == bluestore_deferred_op_t::OP_WRITE);
    bufferlist::const_iterator p = op.data.begin();
    for (auto e : op.extents) {
      txc->osr->deferred_pending->prepare_write(
	cct, wt.seq, e.offset, e.length, p);
    }
  }
  if (deferred_aggressive &&
      !txc->osr->deferred_running) {
    _deferred_submit_unlock(txc->osr.get());
  } else {
    deferred_lock.unlock();
  }
}

void BlueStore::deferred_try_submit()
{
  dout(20) << __func__ << " " << deferred_queue.size() << " osrs, "
	   << deferred_queue_size << " txcs" << dendl;
  std::lock_guard<std::mutex> l(deferred_lock);
  vector<OpSequencerRef> osrs;
  osrs.reserve(deferred_queue.size());
  for (auto& osr : deferred_queue) {
    osrs.push_back(&osr);
  }
  for (auto& osr : osrs) {
    if (osr->deferred_pending) {
      if (!osr->deferred_running) {
	_deferred_submit_unlock(osr.get());
	deferred_lock.lock();
      } else {
	dout(20) << __func__ << "  osr " << osr << " already has running"
		 << dendl;
      }
    } else {
      dout(20) << __func__ << "  osr " << osr << " has no pending" << dendl;
    }
  }
}

void BlueStore::_deferred_submit_unlock(OpSequencer *osr)
{
  dout(10) << __func__ << " osr " << osr
	   << " " << osr->deferred_pending->iomap.size() << " ios pending "
	   << dendl;
  assert(osr->deferred_pending);
  assert(!osr->deferred_running);

  auto b = osr->deferred_pending;
  deferred_queue_size -= b->seq_bytes.size();
  assert(deferred_queue_size >= 0);

  osr->deferred_running = osr->deferred_pending;
  osr->deferred_pending = nullptr;

  uint64_t start = 0, pos = 0;
  bufferlist bl;
  auto i = b->iomap.begin();
  while (true) {
    if (i == b->iomap.end() || i->first != pos) {
      if (bl.length()) {
	dout(20) << __func__ << " write 0x" << std::hex
		 << start << "~" << bl.length()
		 << " crc " << bl.crc32c(-1) << std::dec << dendl;
	if (!g_conf->bluestore_debug_omit_block_device_write) {
	  logger->inc(l_bluestore_deferred_write_ops);
	  logger->inc(l_bluestore_deferred_write_bytes, bl.length());
	  int r = bdev->aio_write(start, bl, &b->ioc, false);
	  assert(r == 0);
	}
      }
      if (i == b->iomap.end()) {
	break;
      }
      start = 0;
      pos = i->first;
      bl.clear();
    }
    dout(20) << __func__ << "   seq " << i->second.seq << " 0x"
	     << std::hex << pos << "~" << i->second.bl.length() << std::dec
	     << dendl;
    if (!bl.length()) {
      start = pos;
    }
    pos += i->second.bl.length();
    bl.claim_append(i->second.bl);
    ++i;
  }

  deferred_lock.unlock();
  bdev->aio_submit(&b->ioc);
}

struct C_DeferredTrySubmit : public Context {
  BlueStore *store;
  C_DeferredTrySubmit(BlueStore *s) : store(s) {}
  void finish(int r) {
    store->deferred_try_submit();
  }
};

void BlueStore::_deferred_aio_finish(OpSequencer *osr)
{
  dout(10) << __func__ << " osr " << osr << dendl;
  assert(osr->deferred_running);
  DeferredBatch *b = osr->deferred_running;

  {
    std::lock_guard<std::mutex> l(deferred_lock);
    assert(osr->deferred_running == b);
    osr->deferred_running = nullptr;
    if (!osr->deferred_pending) {
      dout(20) << __func__ << " dequeueing" << dendl;
      auto q = deferred_queue.iterator_to(*osr);
      deferred_queue.erase(q);
    } else if (deferred_aggressive) {
      dout(20) << __func__ << " queuing async deferred_try_submit" << dendl;
      deferred_finisher.queue(new C_DeferredTrySubmit(this));
    } else {
      dout(20) << __func__ << " leaving queued, more pending" << dendl;
    }
  }

  {
    uint64_t costs = 0;
    std::lock_guard<std::mutex> l2(osr->qlock);
    for (auto& i : b->txcs) {
      TransContext *txc = &i;
      txc->state = TransContext::STATE_DEFERRED_CLEANUP;
      costs += txc->cost;
    }
    osr->qcond.notify_all();
    throttle_deferred_bytes.put(costs);
    std::lock_guard<std::mutex> l(kv_lock);
    deferred_done_queue.emplace_back(b);
  }

  // in the normal case, do not bother waking up the kv thread; it will
  // catch us on the next commit anyway.
  if (deferred_aggressive) {
    std::lock_guard<std::mutex> l(kv_lock);
    kv_cond.notify_one();
  }
}

int BlueStore::_deferred_replay()
{
  dout(10) << __func__ << " start" << dendl;
  OpSequencerRef osr = new OpSequencer(cct, this);
  int count = 0;
  int r = 0;
  KeyValueDB::Iterator it = db->get_iterator(PREFIX_DEFERRED);
  for (it->lower_bound(string()); it->valid(); it->next(), ++count) {
    dout(20) << __func__ << " replay " << pretty_binary_string(it->key())
	     << dendl;
    bluestore_deferred_transaction_t *deferred_txn =
      new bluestore_deferred_transaction_t;
    bufferlist bl = it->value();
    bufferlist::iterator p = bl.begin();
    try {
      ::decode(*deferred_txn, p);
    } catch (buffer::error& e) {
      derr << __func__ << " failed to decode deferred txn "
	   << pretty_binary_string(it->key()) << dendl;
      delete deferred_txn;
      r = -EIO;
      goto out;
    }
    TransContext *txc = _txc_create(osr.get());
    txc->deferred_txn = deferred_txn;
    txc->state = TransContext::STATE_KV_DONE;
    _txc_state_proc(txc);
  }
 out:
  dout(20) << __func__ << " draining osr" << dendl;
  _osr_drain_all();
  osr->discard();
  dout(10) << __func__ << " completed " << count << " events" << dendl;
  return r;
}

// ---------------------------
// transactions

int BlueStore::queue_transactions(
    Sequencer *posr,
    vector<Transaction>& tls,
    TrackedOpRef op,
    ThreadPool::TPHandle *handle)
{
  FUNCTRACE();
  Context *onreadable;
  Context *ondisk;
  Context *onreadable_sync;
  ObjectStore::Transaction::collect_contexts(
    tls, &onreadable, &ondisk, &onreadable_sync);

  if (cct->_conf->objectstore_blackhole) {
    dout(0) << __func__ << " objectstore_blackhole = TRUE, dropping transaction"
	    << dendl;
    delete ondisk;
    delete onreadable;
    delete onreadable_sync;
    return 0;
  }
  utime_t start = ceph_clock_now();
  // set up the sequencer
  OpSequencer *osr;
  assert(posr);
  if (posr->p) {
    osr = static_cast<OpSequencer *>(posr->p.get());
    dout(10) << __func__ << " existing " << osr << " " << *osr << dendl;
  } else {
    osr = new OpSequencer(cct, this);
    osr->parent = posr;
    posr->p = osr;
    dout(10) << __func__ << " new " << osr << " " << *osr << dendl;
  }

  // prepare
  TransContext *txc = _txc_create(osr);
  txc->onreadable = onreadable;
  txc->onreadable_sync = onreadable_sync;
  txc->oncommit = ondisk;

  for (vector<Transaction>::iterator p = tls.begin(); p != tls.end(); ++p) {
    (*p).set_osr(osr);
    txc->bytes += (*p).get_num_bytes();
    _txc_add_transaction(txc, &(*p));
  }
  _txc_calc_cost(txc);

  _txc_write_nodes(txc, txc->t);

  // journal deferred items
  if (txc->deferred_txn) {
    txc->deferred_txn->seq = ++deferred_seq;
    bufferlist bl;
    ::encode(*txc->deferred_txn, bl);
    string key;
    get_deferred_key(txc->deferred_txn->seq, &key);
    txc->t->set(PREFIX_DEFERRED, key, bl);
  }

  _txc_finalize_kv(txc, txc->t);
  if (handle)
    handle->suspend_tp_timeout();

  utime_t tstart = ceph_clock_now();
  throttle_bytes.get(txc->cost);
  if (txc->deferred_txn) {
    // ensure we do not block here because of deferred writes
    if (!throttle_deferred_bytes.get_or_fail(txc->cost)) {
      dout(10) << __func__ << " failed get throttle_deferred_bytes, aggressive"
	       << dendl;
      ++deferred_aggressive;
      deferred_try_submit();
      {
	// wake up any previously finished deferred events
	std::lock_guard<std::mutex> l(kv_lock);
	kv_cond.notify_one();
      }
      throttle_deferred_bytes.get(txc->cost);
      --deferred_aggressive;
   }
  }
  utime_t tend = ceph_clock_now();

  if (handle)
    handle->reset_tp_timeout();

  logger->inc(l_bluestore_txc);

  // execute (start)
  _txc_state_proc(txc);

  logger->tinc(l_bluestore_submit_lat, ceph_clock_now() - start);
  logger->tinc(l_bluestore_throttle_lat, tend - tstart);
  return 0;
}

void BlueStore::_txc_aio_submit(TransContext *txc)
{
  dout(10) << __func__ << " txc " << txc << dendl;
  bdev->aio_submit(&txc->ioc);
}

void BlueStore::_txc_add_transaction(TransContext *txc, Transaction *t)
{
  Transaction::iterator i = t->begin();

  _dump_transaction(t);

  vector<CollectionRef> cvec(i.colls.size());
  unsigned j = 0;
  for (vector<coll_t>::iterator p = i.colls.begin(); p != i.colls.end();
       ++p, ++j) {
    cvec[j] = _get_collection(*p);
  }
  vector<OnodeRef> ovec(i.objects.size());

  for (int pos = 0; i.have_op(); ++pos) {
    Transaction::Op *op = i.decode_op();
    int r = 0;

    // no coll or obj
    if (op->op == Transaction::OP_NOP)
      continue;

    // collection operations
    CollectionRef &c = cvec[op->cid];
    switch (op->op) {
    case Transaction::OP_RMCOLL:
      {
        const coll_t &cid = i.get_cid(op->cid);
	r = _remove_collection(txc, cid, &c);
	if (!r)
	  continue;
      }
      break;

    case Transaction::OP_MKCOLL:
      {
	assert(!c);
	const coll_t &cid = i.get_cid(op->cid);
	r = _create_collection(txc, cid, op->split_bits, &c);
	if (!r)
	  continue;
      }
      break;

    case Transaction::OP_SPLIT_COLLECTION:
      assert(0 == "deprecated");
      break;

    case Transaction::OP_SPLIT_COLLECTION2:
      {
        uint32_t bits = op->split_bits;
        uint32_t rem = op->split_rem;
	r = _split_collection(txc, c, cvec[op->dest_cid], bits, rem);
	if (!r)
	  continue;
      }
      break;

    case Transaction::OP_COLL_HINT:
      {
        uint32_t type = op->hint_type;
        bufferlist hint;
        i.decode_bl(hint);
        bufferlist::iterator hiter = hint.begin();
        if (type == Transaction::COLL_HINT_EXPECTED_NUM_OBJECTS) {
          uint32_t pg_num;
          uint64_t num_objs;
          ::decode(pg_num, hiter);
          ::decode(num_objs, hiter);
          dout(10) << __func__ << " collection hint objects is a no-op, "
		   << " pg_num " << pg_num << " num_objects " << num_objs
		   << dendl;
        } else {
          // Ignore the hint
          dout(10) << __func__ << " unknown collection hint " << type << dendl;
        }
	continue;
      }
      break;

    case Transaction::OP_COLL_SETATTR:
      r = -EOPNOTSUPP;
      break;

    case Transaction::OP_COLL_RMATTR:
      r = -EOPNOTSUPP;
      break;

    case Transaction::OP_COLL_RENAME:
      assert(0 == "not implemented");
      break;
    }
    if (r < 0) {
      derr << __func__ << " error " << cpp_strerror(r)
           << " not handled on operation " << op->op
           << " (op " << pos << ", counting from 0)" << dendl;
      _dump_transaction(t, 0);
      assert(0 == "unexpected error");
    }

    // these operations implicity create the object
    bool create = false;
    if (op->op == Transaction::OP_TOUCH ||
	op->op == Transaction::OP_WRITE ||
	op->op == Transaction::OP_ZERO) {
      create = true;
    }

    // object operations
    RWLock::WLocker l(c->lock);
    OnodeRef &o = ovec[op->oid];
    if (!o) {
      ghobject_t oid = i.get_oid(op->oid);
      o = c->get_onode(oid, create);
    }
    if (!create && (!o || !o->exists)) {
      dout(10) << __func__ << " op " << op->op << " got ENOENT on "
	       << i.get_oid(op->oid) << dendl;
      r = -ENOENT;
      goto endop;
    }

    switch (op->op) {
    case Transaction::OP_TOUCH:
      r = _touch(txc, c, o);
      break;

    case Transaction::OP_WRITE:
      {
        uint64_t off = op->off;
        uint64_t len = op->len;
	uint32_t fadvise_flags = i.get_fadvise_flags();
        bufferlist bl;
        i.decode_bl(bl);
	r = _write(txc, c, o, off, len, bl, fadvise_flags);
      }
      break;

    case Transaction::OP_ZERO:
      {
        uint64_t off = op->off;
        uint64_t len = op->len;
	r = _zero(txc, c, o, off, len);
      }
      break;

    case Transaction::OP_TRIMCACHE:
      {
        // deprecated, no-op
      }
      break;

    case Transaction::OP_TRUNCATE:
      {
        uint64_t off = op->off;
	r = _truncate(txc, c, o, off);
      }
      break;

    case Transaction::OP_REMOVE:
      {
	r = _remove(txc, c, o);
      }
      break;

    case Transaction::OP_SETATTR:
      {
        string name = i.decode_string();
        bufferptr bp;
        i.decode_bp(bp);
	r = _setattr(txc, c, o, name, bp);
      }
      break;

    case Transaction::OP_SETATTRS:
      {
        map<string, bufferptr> aset;
        i.decode_attrset(aset);
	r = _setattrs(txc, c, o, aset);
      }
      break;

    case Transaction::OP_RMATTR:
      {
	string name = i.decode_string();
	r = _rmattr(txc, c, o, name);
      }
      break;

    case Transaction::OP_RMATTRS:
      {
	r = _rmattrs(txc, c, o);
      }
      break;

    case Transaction::OP_CLONE:
      {
	OnodeRef& no = ovec[op->dest_oid];
	if (!no) {
          const ghobject_t& noid = i.get_oid(op->dest_oid);
	  no = c->get_onode(noid, true);
	}
	r = _clone(txc, c, o, no);
      }
      break;

    case Transaction::OP_CLONERANGE:
      assert(0 == "deprecated");
      break;

    case Transaction::OP_CLONERANGE2:
      {
	OnodeRef& no = ovec[op->dest_oid];
	if (!no) {
	  const ghobject_t& noid = i.get_oid(op->dest_oid);
	  no = c->get_onode(noid, true);
	}
        uint64_t srcoff = op->off;
        uint64_t len = op->len;
        uint64_t dstoff = op->dest_off;
	r = _clone_range(txc, c, o, no, srcoff, len, dstoff);
      }
      break;

    case Transaction::OP_COLL_ADD:
      assert(0 == "not implemented");
      break;

    case Transaction::OP_COLL_REMOVE:
      assert(0 == "not implemented");
      break;

    case Transaction::OP_COLL_MOVE:
      assert(0 == "deprecated");
      break;

    case Transaction::OP_COLL_MOVE_RENAME:
    case Transaction::OP_TRY_RENAME:
      {
	assert(op->cid == op->dest_cid);
	const ghobject_t& noid = i.get_oid(op->dest_oid);
	OnodeRef& no = ovec[op->dest_oid];
	if (!no) {
	  no = c->get_onode(noid, false);
	}
	r = _rename(txc, c, o, no, noid);
      }
      break;

    case Transaction::OP_OMAP_CLEAR:
      {
	r = _omap_clear(txc, c, o);
      }
      break;
    case Transaction::OP_OMAP_SETKEYS:
      {
	bufferlist aset_bl;
        i.decode_attrset_bl(&aset_bl);
	r = _omap_setkeys(txc, c, o, aset_bl);
      }
      break;
    case Transaction::OP_OMAP_RMKEYS:
      {
	bufferlist keys_bl;
        i.decode_keyset_bl(&keys_bl);
	r = _omap_rmkeys(txc, c, o, keys_bl);
      }
      break;
    case Transaction::OP_OMAP_RMKEYRANGE:
      {
        string first, last;
        first = i.decode_string();
        last = i.decode_string();
	r = _omap_rmkey_range(txc, c, o, first, last);
      }
      break;
    case Transaction::OP_OMAP_SETHEADER:
      {
        bufferlist bl;
        i.decode_bl(bl);
	r = _omap_setheader(txc, c, o, bl);
      }
      break;

    case Transaction::OP_SETALLOCHINT:
      {
	r = _set_alloc_hint(txc, c, o,
			    op->expected_object_size,
			    op->expected_write_size,
			    op->alloc_hint_flags);
      }
      break;

    default:
      derr << __func__ << "bad op " << op->op << dendl;
      ceph_abort();
    }

  endop:
    if (r < 0) {
      bool ok = false;

      if (r == -ENOENT && !(op->op == Transaction::OP_CLONERANGE ||
			    op->op == Transaction::OP_CLONE ||
			    op->op == Transaction::OP_CLONERANGE2 ||
			    op->op == Transaction::OP_COLL_ADD ||
			    op->op == Transaction::OP_SETATTR ||
			    op->op == Transaction::OP_SETATTRS ||
			    op->op == Transaction::OP_RMATTR ||
			    op->op == Transaction::OP_OMAP_SETKEYS ||
			    op->op == Transaction::OP_OMAP_RMKEYS ||
			    op->op == Transaction::OP_OMAP_RMKEYRANGE ||
			    op->op == Transaction::OP_OMAP_SETHEADER))
	// -ENOENT is usually okay
	ok = true;
      if (r == -ENODATA)
	ok = true;

      if (!ok) {
	const char *msg = "unexpected error code";

	if (r == -ENOENT && (op->op == Transaction::OP_CLONERANGE ||
			     op->op == Transaction::OP_CLONE ||
			     op->op == Transaction::OP_CLONERANGE2))
	  msg = "ENOENT on clone suggests osd bug";

	if (r == -ENOSPC)
	  // For now, if we hit _any_ ENOSPC, crash, before we do any damage
	  // by partially applying transactions.
	  msg = "ENOSPC from bluestore, misconfigured cluster";

	if (r == -ENOTEMPTY) {
	  msg = "ENOTEMPTY suggests garbage data in osd data dir";
	}

        derr << __func__ << " error " << cpp_strerror(r)
             << " not handled on operation " << op->op
             << " (op " << pos << ", counting from 0)"
             << dendl;
        derr << msg << dendl;
        _dump_transaction(t, 0);
	assert(0 == "unexpected error");
      }
    }
  }
}



// -----------------
// write operations

int BlueStore::_touch(TransContext *txc,
		      CollectionRef& c,
		      OnodeRef &o)
{
  dout(15) << __func__ << " " << c->cid << " " << o->oid << dendl;
  int r = 0;
  _assign_nid(txc, o);
  txc->write_onode(o);
  dout(10) << __func__ << " " << c->cid << " " << o->oid << " = " << r << dendl;
  return r;
}

void BlueStore::_dump_onode(const OnodeRef& o, int log_level)
{
  if (!cct->_conf->subsys.should_gather(ceph_subsys_bluestore, log_level))
    return;
  dout(log_level) << __func__ << " " << o << " " << o->oid
		  << " nid " << o->onode.nid
		  << " size 0x" << std::hex << o->onode.size
		  << " (" << std::dec << o->onode.size << ")"
		  << " expected_object_size " << o->onode.expected_object_size
		  << " expected_write_size " << o->onode.expected_write_size
		  << " in " << o->onode.extent_map_shards.size() << " shards"
		  << ", " << o->extent_map.spanning_blob_map.size()
		  << " spanning blobs"
		  << dendl;
  for (auto p = o->onode.attrs.begin();
       p != o->onode.attrs.end();
       ++p) {
    dout(log_level) << __func__ << "  attr " << p->first
		    << " len " << p->second.length() << dendl;
  }
  _dump_extent_map(o->extent_map, log_level);
}

void BlueStore::_dump_extent_map(ExtentMap &em, int log_level)
{
  uint64_t pos = 0;
  for (auto& s : em.shards) {
    dout(log_level) << __func__ << "  shard " << *s.shard_info
		    << (s.loaded ? " (loaded)" : "")
		    << (s.dirty ? " (dirty)" : "")
		    << dendl;
  }
  for (auto& e : em.extent_map) {
    dout(log_level) << __func__ << "  " << e << dendl;
    assert(e.logical_offset >= pos);
    pos = e.logical_offset + e.length;
    const bluestore_blob_t& blob = e.blob->get_blob();
    if (blob.has_csum()) {
      vector<uint64_t> v;
      unsigned n = blob.get_csum_count();
      for (unsigned i = 0; i < n; ++i)
	v.push_back(blob.get_csum_item(i));
      dout(log_level) << __func__ << "      csum: " << std::hex << v << std::dec
		      << dendl;
    }
    std::lock_guard<std::recursive_mutex> l(e.blob->shared_blob->get_cache()->lock);
    for (auto& i : e.blob->shared_blob->bc.buffer_map) {
      dout(log_level) << __func__ << "       0x" << std::hex << i.first
		      << "~" << i.second->length << std::dec
		      << " " << *i.second << dendl;
    }
  }
}

void BlueStore::_dump_transaction(Transaction *t, int log_level)
{
  dout(log_level) << " transaction dump:\n";
  JSONFormatter f(true);
  f.open_object_section("transaction");
  t->dump(&f);
  f.close_section();
  f.flush(*_dout);
  *_dout << dendl;
}

void BlueStore::_pad_zeros(
  bufferlist *bl, uint64_t *offset,
  uint64_t chunk_size)
{
  auto length = bl->length();
  dout(30) << __func__ << " 0x" << std::hex << *offset << "~" << length
	   << " chunk_size 0x" << chunk_size << std::dec << dendl;
  dout(40) << "before:\n";
  bl->hexdump(*_dout);
  *_dout << dendl;
  // front
  size_t front_pad = *offset % chunk_size;
  size_t back_pad = 0;
  size_t pad_count = 0;
  if (front_pad) {
    size_t front_copy = MIN(chunk_size - front_pad, length);
    bufferptr z = buffer::create_page_aligned(chunk_size);
    z.zero(0, front_pad, false);
    pad_count += front_pad;
    bl->copy(0, front_copy, z.c_str() + front_pad);
    if (front_copy + front_pad < chunk_size) {
      back_pad = chunk_size - (length + front_pad);
      z.zero(front_pad + length, back_pad, false);
      pad_count += back_pad;
    }
    bufferlist old, t;
    old.swap(*bl);
    t.substr_of(old, front_copy, length - front_copy);
    bl->append(z);
    bl->claim_append(t);
    *offset -= front_pad;
    length += pad_count;
  }

  // back
  uint64_t end = *offset + length;
  unsigned back_copy = end % chunk_size;
  if (back_copy) {
    assert(back_pad == 0);
    back_pad = chunk_size - back_copy;
    assert(back_copy <= length);
    bufferptr tail(chunk_size);
    bl->copy(length - back_copy, back_copy, tail.c_str());
    tail.zero(back_copy, back_pad, false);
    bufferlist old;
    old.swap(*bl);
    bl->substr_of(old, 0, length - back_copy);
    bl->append(tail);
    length += back_pad;
    pad_count += back_pad;
  }
  dout(20) << __func__ << " pad 0x" << std::hex << front_pad << " + 0x"
	   << back_pad << " on front/back, now 0x" << *offset << "~"
	   << length << std::dec << dendl;
  dout(40) << "after:\n";
  bl->hexdump(*_dout);
  *_dout << dendl;
  if (pad_count)
    logger->inc(l_bluestore_write_pad_bytes, pad_count);
  assert(bl->length() == length);
}

void BlueStore::_do_write_small(
    TransContext *txc,
    CollectionRef &c,
    OnodeRef o,
    uint64_t offset, uint64_t length,
    bufferlist::iterator& blp,
    WriteContext *wctx)
{
  dout(10) << __func__ << " 0x" << std::hex << offset << "~" << length
	   << std::dec << dendl;
  assert(length < min_alloc_size);
  uint64_t end_offs = offset + length;

  logger->inc(l_bluestore_write_small);
  logger->inc(l_bluestore_write_small_bytes, length);

  bufferlist bl;
  blp.copy(length, bl);

  // Look for an existing mutable blob we can use.
  auto begin = o->extent_map.extent_map.begin();
  auto end = o->extent_map.extent_map.end();
  auto ep = o->extent_map.seek_lextent(offset);
  if (ep != begin) {
    --ep;
    if (ep->blob_end() <= offset) {
      ++ep;
    }
  }
  auto prev_ep = ep;
  if (prev_ep != begin) {
    --prev_ep;
  } else {
    prev_ep = end; // to avoid this extent check as it's a duplicate
  }

  auto max_bsize = MAX(wctx->target_blob_size, min_alloc_size);
  auto min_off = offset >= max_bsize ? offset - max_bsize : 0;
  uint32_t alloc_len = min_alloc_size;
  auto offset0 = P2ALIGN(offset, alloc_len);

  bool any_change;

  // search suitable extent in both forward and reverse direction in
  // [offset - target_max_blob_size, offset + target_max_blob_size] range
  // then check if blob can be reused via can_reuse_blob func or apply
  // direct/deferred write (the latter for extents including or higher
  // than 'offset' only).
  do {
    any_change = false;

    if (ep != end && ep->logical_offset < offset + max_bsize) {
      BlobRef b = ep->blob;
      auto bstart = ep->blob_start();
      dout(20) << __func__ << " considering " << *b
	       << " bstart 0x" << std::hex << bstart << std::dec << dendl;
      if (bstart >= end_offs) {
	dout(20) << __func__ << " ignoring distant " << *b << dendl;
      } else if (!b->get_blob().is_mutable()) {
	dout(20) << __func__ << " ignoring immutable " << *b << dendl;
      } else if (ep->logical_offset % min_alloc_size !=
		  ep->blob_offset % min_alloc_size) {
	dout(20) << __func__ << " ignoring offset-skewed " << *b << dendl;
      } else {
	uint64_t chunk_size = b->get_blob().get_chunk_size(block_size);
	// can we pad our head/tail out with zeros?
	uint64_t head_pad, tail_pad;
	head_pad = P2PHASE(offset, chunk_size);
	tail_pad = P2NPHASE(end_offs, chunk_size);
	if (head_pad || tail_pad) {
	  o->extent_map.fault_range(db, offset - head_pad,
				    end_offs - offset + head_pad + tail_pad);
	}
	if (head_pad &&
	    o->extent_map.has_any_lextents(offset - head_pad, chunk_size)) {
	  head_pad = 0;
	}
	if (tail_pad && o->extent_map.has_any_lextents(end_offs, tail_pad)) {
	  tail_pad = 0;
	}

	uint64_t b_off = offset - head_pad - bstart;
	uint64_t b_len = length + head_pad + tail_pad;

	// direct write into unused blocks of an existing mutable blob?
	if ((b_off % chunk_size == 0 && b_len % chunk_size == 0) &&
	    b->get_blob().get_ondisk_length() >= b_off + b_len &&
	    b->get_blob().is_unused(b_off, b_len) &&
	    b->get_blob().is_allocated(b_off, b_len)) {
	  _apply_padding(head_pad, tail_pad, bl);

	  dout(20) << __func__ << "  write to unused 0x" << std::hex
		   << b_off << "~" << b_len
		   << " pad 0x" << head_pad << " + 0x" << tail_pad
		   << std::dec << " of mutable " << *b << dendl;
	  _buffer_cache_write(txc, b, b_off, bl,
			      wctx->buffered ? 0 : Buffer::FLAG_NOCACHE);

	  if (!g_conf->bluestore_debug_omit_block_device_write) {
	    if (b_len <= prefer_deferred_size) {
	      dout(20) << __func__ << " deferring small 0x" << std::hex
		       << b_len << std::dec << " unused write via deferred" << dendl;
	      bluestore_deferred_op_t *op = _get_deferred_op(txc, o);
	      op->op = bluestore_deferred_op_t::OP_WRITE;
	      b->get_blob().map(
		b_off, b_len,
		[&](uint64_t offset, uint64_t length) {
		  op->extents.emplace_back(bluestore_pextent_t(offset, length));
		  return 0;
		});
	      op->data = bl;
	    } else {
	      b->get_blob().map_bl(
		b_off, bl,
		[&](uint64_t offset, bufferlist& t) {
		  bdev->aio_write(offset, t,
				  &txc->ioc, wctx->buffered);
		});
	    }
	  }
	  b->dirty_blob().calc_csum(b_off, bl);
	  dout(20) << __func__ << "  lex old " << *ep << dendl;
	  Extent *le = o->extent_map.set_lextent(c, offset, b_off + head_pad, length,
						 b,
						 &wctx->old_extents);
	  b->dirty_blob().mark_used(le->blob_offset, le->length);
	  txc->statfs_delta.stored() += le->length;
	  dout(20) << __func__ << "  lex " << *le << dendl;
	  logger->inc(l_bluestore_write_small_unused);
	  return;
	}
	// read some data to fill out the chunk?
	uint64_t head_read = P2PHASE(b_off, chunk_size);
	uint64_t tail_read = P2NPHASE(b_off + b_len, chunk_size);
	if ((head_read || tail_read) &&
	    (b->get_blob().get_ondisk_length() >= b_off + b_len + tail_read) &&
	    head_read + tail_read < min_alloc_size) {
	  b_off -= head_read;
	  b_len += head_read + tail_read;

	} else {
	  head_read = tail_read = 0;
	}

	// chunk-aligned deferred overwrite?
	if (b->get_blob().get_ondisk_length() >= b_off + b_len &&
	    b_off % chunk_size == 0 &&
	    b_len % chunk_size == 0 &&
	    b->get_blob().is_allocated(b_off, b_len)) {

	  _apply_padding(head_pad, tail_pad, bl);

	  dout(20) << __func__ << "  reading head 0x" << std::hex << head_read
		   << " and tail 0x" << tail_read << std::dec << dendl;
	  if (head_read) {
	    bufferlist head_bl;
	    int r = _do_read(c.get(), o, offset - head_pad - head_read, head_read,
			     head_bl, 0);
	    assert(r >= 0 && r <= (int)head_read);
	    size_t zlen = head_read - r;
	    if (zlen) {
	      head_bl.append_zero(zlen);
	      logger->inc(l_bluestore_write_pad_bytes, zlen);
	    }
	    bl.claim_prepend(head_bl);
	    logger->inc(l_bluestore_write_penalty_read_ops);
	  }
	  if (tail_read) {
	    bufferlist tail_bl;
	    int r = _do_read(c.get(), o, offset + length + tail_pad, tail_read,
			     tail_bl, 0);
	    assert(r >= 0 && r <= (int)tail_read);
	    size_t zlen = tail_read - r;
	    if (zlen) {
	      tail_bl.append_zero(zlen);
	      logger->inc(l_bluestore_write_pad_bytes, zlen);
	    }
	    bl.claim_append(tail_bl);
	    logger->inc(l_bluestore_write_penalty_read_ops);
	  }
	  logger->inc(l_bluestore_write_small_pre_read);

	  bluestore_deferred_op_t *op = _get_deferred_op(txc, o);
	  op->op = bluestore_deferred_op_t::OP_WRITE;
	  _buffer_cache_write(txc, b, b_off, bl,
			      wctx->buffered ? 0 : Buffer::FLAG_NOCACHE);

	  int r = b->get_blob().map(
	    b_off, b_len,
	    [&](uint64_t offset, uint64_t length) {
	      op->extents.emplace_back(bluestore_pextent_t(offset, length));
	      return 0;
	    });
	  assert(r == 0);
	  if (b->get_blob().csum_type) {
	    b->dirty_blob().calc_csum(b_off, bl);
	  }
	  op->data.claim(bl);
	  dout(20) << __func__ << "  deferred write 0x" << std::hex << b_off << "~"
		   << b_len << std::dec << " of mutable " << *b
		   << " at " << op->extents << dendl;
	  Extent *le = o->extent_map.set_lextent(c, offset, offset - bstart, length,
						 b, &wctx->old_extents);
	  b->dirty_blob().mark_used(le->blob_offset, le->length);
	  txc->statfs_delta.stored() += le->length;
	  dout(20) << __func__ << "  lex " << *le << dendl;
	  logger->inc(l_bluestore_write_small_deferred);
	  return;
	}
	// try to reuse blob if we can
	if (b->can_reuse_blob(min_alloc_size,
			      max_bsize,
			      offset0 - bstart,
			      &alloc_len)) {
	  assert(alloc_len == min_alloc_size); // expecting data always
					       // fit into reused blob
	  // Need to check for pending writes desiring to
	  // reuse the same pextent. The rationale is that during GC two chunks
	  // from garbage blobs(compressed?) can share logical space within the same
	  // AU. That's in turn might be caused by unaligned len in clone_range2.
	  // Hence the second write will fail in an attempt to reuse blob at
	  // do_alloc_write().
	  if (!wctx->has_conflict(b,
				  offset0,
				  offset0 + alloc_len, 
				  min_alloc_size)) {

	    // we can't reuse pad_head/pad_tail since they might be truncated 
	    // due to existent extents
	    uint64_t b_off = offset - bstart;
	    uint64_t b_off0 = b_off;
	    _pad_zeros(&bl, &b_off0, chunk_size);

	    dout(20) << __func__ << " reuse blob " << *b << std::hex
		     << " (0x" << b_off0 << "~" << bl.length() << ")"
		     << " (0x" << b_off << "~" << length << ")"
		     << std::dec << dendl;

	    o->extent_map.punch_hole(c, offset, length, &wctx->old_extents);
	    wctx->write(offset, b, alloc_len, b_off0, bl, b_off, length,
			false, false);
	    logger->inc(l_bluestore_write_small_unused);
	    return;
	  }
	}
      }
      ++ep;
      any_change = true;
    } // if (ep != end && ep->logical_offset < offset + max_bsize)

    // check extent for reuse in reverse order
    if (prev_ep != end && prev_ep->logical_offset >= min_off) {
      BlobRef b = prev_ep->blob;
      auto bstart = prev_ep->blob_start();
      dout(20) << __func__ << " considering " << *b
	       << " bstart 0x" << std::hex << bstart << std::dec << dendl;
      if (b->can_reuse_blob(min_alloc_size,
			    max_bsize,
                            offset0 - bstart,
                            &alloc_len)) {
	assert(alloc_len == min_alloc_size); // expecting data always
					     // fit into reused blob
	// Need to check for pending writes desiring to
	// reuse the same pextent. The rationale is that during GC two chunks
	// from garbage blobs(compressed?) can share logical space within the same
	// AU. That's in turn might be caused by unaligned len in clone_range2.
	// Hence the second write will fail in an attempt to reuse blob at
	// do_alloc_write().
	if (!wctx->has_conflict(b,
				offset0,
				offset0 + alloc_len, 
				min_alloc_size)) {

	  uint64_t chunk_size = b->get_blob().get_chunk_size(block_size);
	  uint64_t b_off = offset - bstart;
	  uint64_t b_off0 = b_off;
	  _pad_zeros(&bl, &b_off0, chunk_size);

	  dout(20) << __func__ << " reuse blob " << *b << std::hex
		    << " (0x" << b_off0 << "~" << bl.length() << ")"
		    << " (0x" << b_off << "~" << length << ")"
		    << std::dec << dendl;

	  o->extent_map.punch_hole(c, offset, length, &wctx->old_extents);
	  wctx->write(offset, b, alloc_len, b_off0, bl, b_off, length,
		      false, false);
	  logger->inc(l_bluestore_write_small_unused);
	  return;
	}
      } 
      if (prev_ep != begin) {
	--prev_ep;
	any_change = true;
      } else {
	prev_ep = end; // to avoid useless first extent re-check
      }
    } // if (prev_ep != end && prev_ep->logical_offset >= min_off) 
  } while (any_change);

  // new blob.
  
  BlobRef b = c->new_blob();
  uint64_t b_off = P2PHASE(offset, alloc_len);
  uint64_t b_off0 = b_off;
  _pad_zeros(&bl, &b_off0, block_size);
  o->extent_map.punch_hole(c, offset, length, &wctx->old_extents);
  wctx->write(offset, b, alloc_len, b_off0, bl, b_off, length, true, true);
  logger->inc(l_bluestore_write_small_new);

  return;
}

void BlueStore::_do_write_big(
    TransContext *txc,
    CollectionRef &c,
    OnodeRef o,
    uint64_t offset, uint64_t length,
    bufferlist::iterator& blp,
    WriteContext *wctx)
{
  dout(10) << __func__ << " 0x" << std::hex << offset << "~" << length
	   << " target_blob_size 0x" << wctx->target_blob_size << std::dec
	   << " compress " << (int)wctx->compress
	   << dendl;
  logger->inc(l_bluestore_write_big);
  logger->inc(l_bluestore_write_big_bytes, length);
  o->extent_map.punch_hole(c, offset, length, &wctx->old_extents);
  auto max_bsize = MAX(wctx->target_blob_size, min_alloc_size);
  while (length > 0) {
    bool new_blob = false;
    uint32_t l = MIN(max_bsize, length);
    BlobRef b;
    uint32_t b_off = 0;

    //attempting to reuse existing blob
    if (!wctx->compress) {
      // look for an existing mutable blob we can reuse
      auto begin = o->extent_map.extent_map.begin();
      auto end = o->extent_map.extent_map.end();
      auto ep = o->extent_map.seek_lextent(offset);
      auto prev_ep = ep;
      if (prev_ep != begin) {
        --prev_ep;
      } else {
        prev_ep = end; // to avoid this extent check as it's a duplicate
      }
      auto min_off = offset >= max_bsize ? offset - max_bsize : 0;
      // search suitable extent in both forward and reverse direction in
      // [offset - target_max_blob_size, offset + target_max_blob_size] range
      // then check if blob can be reused via can_reuse_blob func.
      bool any_change;
      do {
	any_change = false;
	if (ep != end && ep->logical_offset < offset + max_bsize) {
	  if (offset >= ep->blob_start() &&
              ep->blob->can_reuse_blob(min_alloc_size, max_bsize,
	                               offset - ep->blob_start(),
	                               &l)) {
	    b = ep->blob;
	    b_off = offset - ep->blob_start();
            prev_ep = end; // to avoid check below
	    dout(20) << __func__ << " reuse blob " << *b << std::hex
		     << " (0x" << b_off << "~" << l << ")" << std::dec << dendl;
	  } else {
	    ++ep;
	    any_change = true;
	  }
	}

	if (prev_ep != end && prev_ep->logical_offset >= min_off) {
	  if (prev_ep->blob->can_reuse_blob(min_alloc_size, max_bsize,
                                    	    offset - prev_ep->blob_start(),
                                    	    &l)) {
	    b = prev_ep->blob;
	    b_off = offset - prev_ep->blob_start();
	    dout(20) << __func__ << " reuse blob " << *b << std::hex
		     << " (0x" << b_off << "~" << l << ")" << std::dec << dendl;
	  } else if (prev_ep != begin) {
	    --prev_ep;
	    any_change = true;
	  } else {
	    prev_ep = end; // to avoid useless first extent re-check
	  }
	}
      } while (b == nullptr && any_change);
    }
    if (b == nullptr) {
      b = c->new_blob();
      b_off = 0;
      new_blob = true;
    }

    bufferlist t;
    blp.copy(l, t);
    wctx->write(offset, b, l, b_off, t, b_off, l, false, new_blob);
    offset += l;
    length -= l;
    logger->inc(l_bluestore_write_big_blobs);
  }
}

int BlueStore::_do_alloc_write(
  TransContext *txc,
  CollectionRef coll,
  OnodeRef o,
  WriteContext *wctx)
{
  dout(20) << __func__ << " txc " << txc
	   << " " << wctx->writes.size() << " blobs"
	   << dendl;
  if (wctx->writes.empty()) {
    return 0;
  }

  CompressorRef c;
  double crr = 0;
  if (wctx->compress) {
    c = select_option(
      "compression_algorithm",
      compressor,
      [&]() {
        string val;
        if (coll->pool_opts.get(pool_opts_t::COMPRESSION_ALGORITHM, &val)) {
          CompressorRef cp = compressor;
          if (!cp || cp->get_type_name() != val) {
            cp = Compressor::create(cct, val);
          }
          return boost::optional<CompressorRef>(cp);
        }
        return boost::optional<CompressorRef>();
      }
    );

    crr = select_option(
      "compression_required_ratio",
      cct->_conf->bluestore_compression_required_ratio,
      [&]() {
        double val;
        if (coll->pool_opts.get(pool_opts_t::COMPRESSION_REQUIRED_RATIO, &val)) {
          return boost::optional<double>(val);
        }
        return boost::optional<double>();
      }
    );
  }

  // checksum
  int csum = csum_type.load();
  csum = select_option(
    "csum_type",
    csum,
    [&]() {
      int val;
      if (coll->pool_opts.get(pool_opts_t::CSUM_TYPE, &val)) {
        return  boost::optional<int>(val);
      }
      return boost::optional<int>();
    }
  );

  // compress (as needed) and calc needed space
  uint64_t need = 0;
  auto max_bsize = MAX(wctx->target_blob_size, min_alloc_size);
  for (auto& wi : wctx->writes) {
    if (c && wi.blob_length > min_alloc_size) {
      utime_t start = ceph_clock_now();

      // compress
      assert(wi.b_off == 0);
      assert(wi.blob_length == wi.bl.length());

      // FIXME: memory alignment here is bad
      bufferlist t;
      int r = c->compress(wi.bl, t);
      assert(r == 0);

      bluestore_compression_header_t chdr;
      chdr.type = c->get_type();
      chdr.length = t.length();
      ::encode(chdr, wi.compressed_bl);
      wi.compressed_bl.claim_append(t);

      wi.compressed_len = wi.compressed_bl.length();
      uint64_t newlen = P2ROUNDUP(wi.compressed_len, min_alloc_size);
      uint64_t want_len_raw = wi.blob_length * crr;
      uint64_t want_len = P2ROUNDUP(want_len_raw, min_alloc_size);
      if (newlen <= want_len && newlen < wi.blob_length) {
	// Cool. We compressed at least as much as we were hoping to.
	// pad out to min_alloc_size
	wi.compressed_bl.append_zero(newlen - wi.compressed_len);
	logger->inc(l_bluestore_write_pad_bytes, newlen - wi.compressed_len);
	dout(20) << __func__ << std::hex << "  compressed 0x" << wi.blob_length
		 << " -> 0x" << wi.compressed_len << " => 0x" << newlen
		 << " with " << c->get_type()
		 << std::dec << dendl;
	txc->statfs_delta.compressed() += wi.compressed_len;
	txc->statfs_delta.compressed_original() += wi.blob_length;
	txc->statfs_delta.compressed_allocated() += newlen;
	logger->inc(l_bluestore_compress_success_count);
	wi.compressed = true;
	need += newlen;
      } else {
	dout(20) << __func__ << std::hex << "  0x" << wi.blob_length
		 << " compressed to 0x" << wi.compressed_len << " -> 0x" << newlen
		 << " with " << c->get_type()
		 << ", which is more than required 0x" << want_len_raw
		 << " -> 0x" << want_len
		 << ", leaving uncompressed"
		 << std::dec << dendl;
	logger->inc(l_bluestore_compress_rejected_count);
	need += wi.blob_length;
      }
      logger->tinc(l_bluestore_compress_lat,
		   ceph_clock_now() - start);
    } else {
      need += wi.blob_length;
    }
  }
  int r = alloc->reserve(need);
  if (r < 0) {
    derr << __func__ << " failed to reserve 0x" << std::hex << need << std::dec
	 << dendl;
    return r;
  }
  AllocExtentVector prealloc;
  prealloc.reserve(2 * wctx->writes.size());;
  int prealloc_left = 0;
  prealloc_left = alloc->allocate(
    need, min_alloc_size, need,
    0, &prealloc);
  assert(prealloc_left == (int64_t)need);
  dout(20) << __func__ << " prealloc " << prealloc << dendl;
  auto prealloc_pos = prealloc.begin();

  for (auto& wi : wctx->writes) {
    BlobRef b = wi.b;
    bluestore_blob_t& dblob = b->dirty_blob();
    uint64_t b_off = wi.b_off;
    bufferlist *l = &wi.bl;
    uint64_t final_length = wi.blob_length;
    uint64_t csum_length = wi.blob_length;
    unsigned csum_order = block_size_order;
    if (wi.compressed) {
      final_length = wi.compressed_bl.length();
      csum_length = final_length;
      csum_order = ctz(csum_length);
      l = &wi.compressed_bl;
      dblob.set_compressed(wi.blob_length, wi.compressed_len);
    } else if (wi.new_blob) {
      // initialize newly created blob only
      assert(dblob.is_mutable());
      if (l->length() != wi.blob_length) {
        // hrm, maybe we could do better here, but let's not bother.
        dout(20) << __func__ << " forcing csum_order to block_size_order "
                << block_size_order << dendl;
	csum_order = block_size_order;
      } else {
        csum_order = std::min(wctx->csum_order, ctz(l->length()));
      }
      // try to align blob with max_blob_size to improve
      // its reuse ratio, e.g. in case of reverse write
      uint32_t suggested_boff =
       (wi.logical_offset - (wi.b_off0 - wi.b_off)) % max_bsize;
      if ((suggested_boff % (1 << csum_order)) == 0 &&
           suggested_boff + final_length <= max_bsize &&
           suggested_boff > b_off) {
        dout(20) << __func__ << " forcing blob_offset to 0x"
                 << std::hex << suggested_boff << std::dec << dendl;
        assert(suggested_boff >= b_off);
        csum_length += suggested_boff - b_off;
        b_off = suggested_boff;
      }
      if (csum != Checksummer::CSUM_NONE) {
        dout(20) << __func__ << " initialize csum setting for new blob " << *b
                 << " csum_type " << Checksummer::get_csum_type_string(csum)
                 << " csum_order " << csum_order
                 << " csum_length 0x" << std::hex << csum_length << std::dec
                 << dendl;
        dblob.init_csum(csum, csum_order, csum_length);
      }
    }

    AllocExtentVector extents;
    int64_t left = final_length;
    while (left > 0) {
      assert(prealloc_left > 0);
      if (prealloc_pos->length <= left) {
	prealloc_left -= prealloc_pos->length;
	left -= prealloc_pos->length;
	txc->statfs_delta.allocated() += prealloc_pos->length;
	extents.push_back(*prealloc_pos);
	++prealloc_pos;
      } else {
	extents.emplace_back(prealloc_pos->offset, left);
	prealloc_pos->offset += left;
	prealloc_pos->length -= left;
	prealloc_left -= left;
	txc->statfs_delta.allocated() += left;
	left = 0;
	break;
      }
    }
    for (auto& p : extents) {
      txc->allocated.insert(p.offset, p.length);
    }
    dblob.allocated(P2ALIGN(b_off, min_alloc_size), final_length, extents);

    dout(20) << __func__ << " blob " << *b << dendl;
    if (dblob.has_csum()) {
      dblob.calc_csum(b_off, *l);
    }

    if (wi.mark_unused) {
      auto b_end = b_off + wi.bl.length();
      if (b_off) {
        dblob.add_unused(0, b_off);
      }
      if (b_end < wi.blob_length) {
        dblob.add_unused(b_end, wi.blob_length - b_end);
      }
    }

    Extent *le = o->extent_map.set_lextent(coll, wi.logical_offset,
                                           b_off + (wi.b_off0 - wi.b_off),
                                           wi.length0,
                                           wi.b,
                                           nullptr);
    wi.b->dirty_blob().mark_used(le->blob_offset, le->length);
    txc->statfs_delta.stored() += le->length;
    dout(20) << __func__ << "  lex " << *le << dendl;
    _buffer_cache_write(txc, wi.b, b_off, wi.bl,
                        wctx->buffered ? 0 : Buffer::FLAG_NOCACHE);

    // queue io
    if (!g_conf->bluestore_debug_omit_block_device_write) {
      if (l->length() <= prefer_deferred_size.load()) {
	dout(20) << __func__ << " deferring small 0x" << std::hex
		 << l->length() << std::dec << " write via deferred" << dendl;
	bluestore_deferred_op_t *op = _get_deferred_op(txc, o);
	op->op = bluestore_deferred_op_t::OP_WRITE;
	int r = b->get_blob().map(
	  b_off, l->length(),
	  [&](uint64_t offset, uint64_t length) {
	    op->extents.emplace_back(bluestore_pextent_t(offset, length));
	    return 0;
	  });
        assert(r == 0);
	op->data = *l;
      } else {
	b->get_blob().map_bl(
	  b_off, *l,
	  [&](uint64_t offset, bufferlist& t) {
	    bdev->aio_write(offset, t, &txc->ioc, false);
	  });
      }
    }
  }
  assert(prealloc_pos == prealloc.end());
  assert(prealloc_left == 0);
  return 0;
}

void BlueStore::_wctx_finish(
  TransContext *txc,
  CollectionRef& c,
  OnodeRef o,
  WriteContext *wctx,
  set<SharedBlob*> *maybe_unshared_blobs)
{
  auto oep = wctx->old_extents.begin();
  while (oep != wctx->old_extents.end()) {
    auto &lo = *oep;
    oep = wctx->old_extents.erase(oep);
    dout(20) << __func__ << " lex_old " << lo.e << dendl;
    BlobRef b = lo.e.blob;
    const bluestore_blob_t& blob = b->get_blob();
    if (blob.is_compressed()) {
      if (lo.blob_empty) {
	txc->statfs_delta.compressed() -= blob.get_compressed_payload_length();
      }
      txc->statfs_delta.compressed_original() -= lo.e.length;
    }
    auto& r = lo.r;
    txc->statfs_delta.stored() -= lo.e.length;
    if (!r.empty()) {
      dout(20) << __func__ << "  blob release " << r << dendl;
      if (blob.is_shared()) {
	PExtentVector final;
        c->load_shared_blob(b->shared_blob);
	for (auto e : r) {
	  b->shared_blob->put_ref(
	    e.offset, e.length, &final,
	    b->is_referenced() ? nullptr : maybe_unshared_blobs);
	}
	dout(20) << __func__ << "  shared_blob release " << final
		 << " from " << *b->shared_blob << dendl;
	txc->write_shared_blob(b->shared_blob);
	r.clear();
	r.swap(final);
      }
    }
    // we can't invalidate our logical extents as we drop them because
    // other lextents (either in our onode or others) may still
    // reference them.  but we can throw out anything that is no
    // longer allocated.  Note that this will leave behind edge bits
    // that are no longer referenced but not deallocated (until they
    // age out of the cache naturally).
    b->discard_unallocated(c.get());
    for (auto e : r) {
      dout(20) << __func__ << "  release " << e << dendl;
      txc->released.insert(e.offset, e.length);
      txc->statfs_delta.allocated() -= e.length;
      if (blob.is_compressed()) {
        txc->statfs_delta.compressed_allocated() -= e.length;
      }
    }
    delete &lo;
    if (b->is_spanning() && !b->is_referenced()) {
      dout(20) << __func__ << "  spanning_blob_map removing empty " << *b
	       << dendl;
      o->extent_map.spanning_blob_map.erase(b->id);
    }
  }
}

void BlueStore::_do_write_data(
  TransContext *txc,
  CollectionRef& c,
  OnodeRef o,
  uint64_t offset,
  uint64_t length,
  bufferlist& bl,
  WriteContext *wctx)
{
  uint64_t end = offset + length;
  bufferlist::iterator p = bl.begin();

  if (offset / min_alloc_size == (end - 1) / min_alloc_size &&
      (length != min_alloc_size)) {
    // we fall within the same block
    _do_write_small(txc, c, o, offset, length, p, wctx);
  } else {
    uint64_t head_offset, head_length;
    uint64_t middle_offset, middle_length;
    uint64_t tail_offset, tail_length;

    head_offset = offset;
    head_length = P2NPHASE(offset, min_alloc_size);

    tail_offset = P2ALIGN(end, min_alloc_size);
    tail_length = P2PHASE(end, min_alloc_size);

    middle_offset = head_offset + head_length;
    middle_length = length - head_length - tail_length;

    if (head_length) {
      _do_write_small(txc, c, o, head_offset, head_length, p, wctx);
    }

    if (middle_length) {
      _do_write_big(txc, c, o, middle_offset, middle_length, p, wctx);
    }

    if (tail_length) {
      _do_write_small(txc, c, o, tail_offset, tail_length, p, wctx);
    }
  }
}

void BlueStore::_choose_write_options(
   CollectionRef& c,
   OnodeRef o,
   uint32_t fadvise_flags,
   WriteContext *wctx)
{
  if (fadvise_flags & CEPH_OSD_OP_FLAG_FADVISE_WILLNEED) {
    dout(20) << __func__ << " will do buffered write" << dendl;
    wctx->buffered = true;
  } else if (cct->_conf->bluestore_default_buffered_write &&
	     (fadvise_flags & (CEPH_OSD_OP_FLAG_FADVISE_DONTNEED |
			       CEPH_OSD_OP_FLAG_FADVISE_NOCACHE)) == 0) {
    dout(20) << __func__ << " defaulting to buffered write" << dendl;
    wctx->buffered = true;
  }

  // apply basic csum block size
  wctx->csum_order = block_size_order;

  // compression parameters
  unsigned alloc_hints = o->onode.alloc_hint_flags;
  auto cm = select_option(
    "compression_mode",
    comp_mode.load(),
    [&]() {
      string val;
      if(c->pool_opts.get(pool_opts_t::COMPRESSION_MODE, &val)) {
	return boost::optional<Compressor::CompressionMode>(
	  Compressor::get_comp_mode_type(val));
      }
      return boost::optional<Compressor::CompressionMode>();
    }
  );

  wctx->compress = (cm != Compressor::COMP_NONE) &&
    ((cm == Compressor::COMP_FORCE) ||
     (cm == Compressor::COMP_AGGRESSIVE &&
      (alloc_hints & CEPH_OSD_ALLOC_HINT_FLAG_INCOMPRESSIBLE) == 0) ||
     (cm == Compressor::COMP_PASSIVE &&
      (alloc_hints & CEPH_OSD_ALLOC_HINT_FLAG_COMPRESSIBLE)));

  if ((alloc_hints & CEPH_OSD_ALLOC_HINT_FLAG_SEQUENTIAL_READ) &&
      (alloc_hints & CEPH_OSD_ALLOC_HINT_FLAG_RANDOM_READ) == 0 &&
      (alloc_hints & (CEPH_OSD_ALLOC_HINT_FLAG_IMMUTABLE |
                      CEPH_OSD_ALLOC_HINT_FLAG_APPEND_ONLY)) &&
      (alloc_hints & CEPH_OSD_ALLOC_HINT_FLAG_RANDOM_WRITE) == 0) {

    dout(20) << __func__ << " will prefer large blob and csum sizes" << dendl;

    if (o->onode.expected_write_size) {
      wctx->csum_order = std::max(min_alloc_size_order,
			          (uint8_t)ctz(o->onode.expected_write_size));
    } else {
      wctx->csum_order = min_alloc_size_order;
    }

    if (wctx->compress) {
      wctx->target_blob_size = select_option(
        "compression_max_blob_size",
        comp_max_blob_size.load(),
        [&]() {
          int val;
          if(c->pool_opts.get(pool_opts_t::COMPRESSION_MAX_BLOB_SIZE, &val)) {
   	    return boost::optional<uint64_t>((uint64_t)val);
          }
          return boost::optional<uint64_t>();
        }
      );
    }
  } else {
    if (wctx->compress) {
      wctx->target_blob_size = select_option(
        "compression_min_blob_size",
        comp_min_blob_size.load(),
        [&]() {
          int val;
          if(c->pool_opts.get(pool_opts_t::COMPRESSION_MIN_BLOB_SIZE, &val)) {
   	    return boost::optional<uint64_t>((uint64_t)val);
          }
          return boost::optional<uint64_t>();
        }
      );
    }
  }

  uint64_t max_bsize = max_blob_size.load();
  if (wctx->target_blob_size == 0 || wctx->target_blob_size > max_bsize) {
    wctx->target_blob_size = max_bsize;
  }

  // set the min blob size floor at 2x the min_alloc_size, or else we
  // won't be able to allocate a smaller extent for the compressed
  // data.
  if (wctx->compress &&
      wctx->target_blob_size < min_alloc_size * 2) {
    wctx->target_blob_size = min_alloc_size * 2;
  }

  dout(20) << __func__ << " prefer csum_order " << wctx->csum_order
           << " target_blob_size 0x" << std::hex << wctx->target_blob_size
           << std::dec << dendl;
}

int BlueStore::_do_gc(
  TransContext *txc,
  CollectionRef& c,
  OnodeRef o,
  const GarbageCollector& gc,
  const WriteContext& wctx,
  uint64_t *dirty_start,
  uint64_t *dirty_end)
{
  auto& extents_to_collect = gc.get_extents_to_collect();

  WriteContext wctx_gc;
  wctx_gc.fork(wctx); // make a clone for garbage collection

  for (auto it = extents_to_collect.begin();
       it != extents_to_collect.end();
       ++it) {
    bufferlist bl;
    int r = _do_read(c.get(), o, it->offset, it->length, bl, 0);
    assert(r == (int)it->length);

    o->extent_map.fault_range(db, it->offset, it->length);
    _do_write_data(txc, c, o, it->offset, it->length, bl, &wctx_gc);
    logger->inc(l_bluestore_gc_merged, it->length);

    if (*dirty_start > it->offset) {
      *dirty_start = it->offset;
    }

    if (*dirty_end < it->offset + it->length) {
      *dirty_end = it->offset + it->length;
    }
  }

  dout(30) << __func__ << " alloc write" << dendl;
  int r = _do_alloc_write(txc, c, o, &wctx_gc);
  if (r < 0) {
    derr << __func__ << " _do_alloc_write failed with " << cpp_strerror(r)
         << dendl;
    return r;
  }

  _wctx_finish(txc, c, o, &wctx_gc);
  return 0;
}

int BlueStore::_do_write(
  TransContext *txc,
  CollectionRef& c,
  OnodeRef o,
  uint64_t offset,
  uint64_t length,
  bufferlist& bl,
  uint32_t fadvise_flags)
{
  int r = 0;

  dout(20) << __func__
	   << " " << o->oid
	   << " 0x" << std::hex << offset << "~" << length
	   << " - have 0x" << o->onode.size
	   << " (" << std::dec << o->onode.size << ")"
	   << " bytes"
	   << " fadvise_flags 0x" << std::hex << fadvise_flags << std::dec
	   << dendl;
  _dump_onode(o);

  if (length == 0) {
    return 0;
  }

  uint64_t end = offset + length;

  GarbageCollector gc(c->store->cct);
  int64_t benefit;
  auto dirty_start = offset;
  auto dirty_end = end;

  WriteContext wctx;
  _choose_write_options(c, o, fadvise_flags, &wctx);
  o->extent_map.fault_range(db, offset, length);
  _do_write_data(txc, c, o, offset, length, bl, &wctx);
  r = _do_alloc_write(txc, c, o, &wctx);
  if (r < 0) {
    derr << __func__ << " _do_alloc_write failed with " << cpp_strerror(r)
	 << dendl;
    goto out;
  }

  // NB: _wctx_finish() will empty old_extents
  // so we must do gc estimation before that
  benefit = gc.estimate(offset,
                        length,
		        o->extent_map,
			wctx.old_extents,
			min_alloc_size);

  _wctx_finish(txc, c, o, &wctx);
  if (end > o->onode.size) {
    dout(20) << __func__ << " extending size to 0x" << std::hex << end
             << std::dec << dendl;
    o->onode.size = end;
  }

  if (benefit >= g_conf->bluestore_gc_enable_total_threshold) {
    if (!gc.get_extents_to_collect().empty()) {
      dout(20) << __func__ << " perform garbage collection, "
               << "expected benefit = " << benefit << " AUs" << dendl;
      r = _do_gc(txc, c, o, gc, wctx, &dirty_start, &dirty_end);
      if (r < 0) {
        derr << __func__ << " _do_gc failed with " << cpp_strerror(r)
             << dendl;
        goto out;
      }
    }
  }

  o->extent_map.compress_extent_map(dirty_start, dirty_end - dirty_start);
  o->extent_map.dirty_range(dirty_start, dirty_end - dirty_start);

  r = 0;

 out:
  return r;
}

int BlueStore::_write(TransContext *txc,
		      CollectionRef& c,
		      OnodeRef& o,
		      uint64_t offset, size_t length,
		      bufferlist& bl,
		      uint32_t fadvise_flags)
{
  dout(15) << __func__ << " " << c->cid << " " << o->oid
	   << " 0x" << std::hex << offset << "~" << length << std::dec
	   << dendl;
  int r = 0;
  if (offset + length >= OBJECT_MAX_SIZE) {
    r = -E2BIG;
  } else {
    _assign_nid(txc, o);
    r = _do_write(txc, c, o, offset, length, bl, fadvise_flags);
    txc->write_onode(o);
  }
  dout(10) << __func__ << " " << c->cid << " " << o->oid
	   << " 0x" << std::hex << offset << "~" << length << std::dec
	   << " = " << r << dendl;
  return r;
}

int BlueStore::_zero(TransContext *txc,
		     CollectionRef& c,
		     OnodeRef& o,
		     uint64_t offset, size_t length)
{
  dout(15) << __func__ << " " << c->cid << " " << o->oid
	   << " 0x" << std::hex << offset << "~" << length << std::dec
	   << dendl;
  int r = 0;
  if (offset + length >= OBJECT_MAX_SIZE) {
    r = -E2BIG;
  } else {
    _assign_nid(txc, o);
    r = _do_zero(txc, c, o, offset, length);
  }
  dout(10) << __func__ << " " << c->cid << " " << o->oid
	   << " 0x" << std::hex << offset << "~" << length << std::dec
	   << " = " << r << dendl;
  return r;
}

int BlueStore::_do_zero(TransContext *txc,
			CollectionRef& c,
			OnodeRef& o,
			uint64_t offset, size_t length)
{
  dout(15) << __func__ << " " << c->cid << " " << o->oid
	   << " 0x" << std::hex << offset << "~" << length << std::dec
	   << dendl;
  int r = 0;

  _dump_onode(o);

  WriteContext wctx;
  o->extent_map.fault_range(db, offset, length);
  o->extent_map.punch_hole(c, offset, length, &wctx.old_extents);
  o->extent_map.dirty_range(offset, length);
  _wctx_finish(txc, c, o, &wctx);

  if (length > 0 && offset + length > o->onode.size) {
    o->onode.size = offset + length;
    dout(20) << __func__ << " extending size to " << offset + length
	     << dendl;
  }
  txc->write_onode(o);

  dout(10) << __func__ << " " << c->cid << " " << o->oid
	   << " 0x" << std::hex << offset << "~" << length << std::dec
	   << " = " << r << dendl;
  return r;
}

void BlueStore::_do_truncate(
  TransContext *txc, CollectionRef& c, OnodeRef o, uint64_t offset,
  set<SharedBlob*> *maybe_unshared_blobs)
{
  dout(15) << __func__ << " " << c->cid << " " << o->oid
	   << " 0x" << std::hex << offset << std::dec << dendl;

  _dump_onode(o, 30);

  if (offset == o->onode.size)
    return;

  if (offset < o->onode.size) {
    WriteContext wctx;
    uint64_t length = o->onode.size - offset;
    o->extent_map.fault_range(db, offset, length);
    o->extent_map.punch_hole(c, offset, length, &wctx.old_extents);
    o->extent_map.dirty_range(offset, length);
    _wctx_finish(txc, c, o, &wctx, maybe_unshared_blobs);

    // if we have shards past EOF, ask for a reshard
    if (!o->onode.extent_map_shards.empty() &&
	o->onode.extent_map_shards.back().offset >= offset) {
      dout(10) << __func__ << "  request reshard past EOF" << dendl;
      if (offset) {
	o->extent_map.request_reshard(offset - 1, offset + length);
      } else {
	o->extent_map.request_reshard(0, length);
      }
    }
  }

  o->onode.size = offset;

  txc->write_onode(o);
}

int BlueStore::_truncate(TransContext *txc,
			 CollectionRef& c,
			 OnodeRef& o,
			 uint64_t offset)
{
  dout(15) << __func__ << " " << c->cid << " " << o->oid
	   << " 0x" << std::hex << offset << std::dec
	   << dendl;
  int r = 0;
  if (offset >= OBJECT_MAX_SIZE) {
    r = -E2BIG;
  } else {
    _do_truncate(txc, c, o, offset);
  }
  dout(10) << __func__ << " " << c->cid << " " << o->oid
	   << " 0x" << std::hex << offset << std::dec
	   << " = " << r << dendl;
  return r;
}

int BlueStore::_do_remove(
  TransContext *txc,
  CollectionRef& c,
  OnodeRef o)
{
  set<SharedBlob*> maybe_unshared_blobs;
  bool is_gen = !o->oid.is_no_gen();
  _do_truncate(txc, c, o, 0, is_gen ? &maybe_unshared_blobs : nullptr);
  if (o->onode.has_omap()) {
    o->flush();
    _do_omap_clear(txc, o->onode.nid);
  }
  o->exists = false;
  string key;
  for (auto &s : o->extent_map.shards) {
    dout(20) << __func__ << "  removing shard 0x" << std::hex
	     << s.shard_info->offset << std::dec << dendl;
    generate_extent_shard_key_and_apply(o->key, s.shard_info->offset, &key,
      [&](const string& final_key) {
        txc->t->rmkey(PREFIX_OBJ, final_key);
      }
    );
  }
  txc->t->rmkey(PREFIX_OBJ, o->key.c_str(), o->key.size());
  txc->removed(o);
  o->extent_map.clear();
  o->onode = bluestore_onode_t();
  _debug_obj_on_delete(o->oid);

  if (!is_gen || maybe_unshared_blobs.empty()) {
    return 0;
  }

  // see if we can unshare blobs still referenced by the head
  dout(10) << __func__ << " gen and maybe_unshared_blobs "
	   << maybe_unshared_blobs << dendl;
  ghobject_t nogen = o->oid;
  nogen.generation = ghobject_t::NO_GEN;
  OnodeRef h = c->onode_map.lookup(nogen);

  if (!h || !h->exists) {
    return 0;
  }

  dout(20) << __func__ << " checking for unshareable blobs on " << h
	   << " " << h->oid << dendl;
  map<SharedBlob*,bluestore_extent_ref_map_t> expect;
  for (auto& e : h->extent_map.extent_map) {
    const bluestore_blob_t& b = e.blob->get_blob();
    SharedBlob *sb = e.blob->shared_blob.get();
    if (b.is_shared() &&
	sb->loaded &&
	maybe_unshared_blobs.count(sb)) {
      if (b.is_compressed()) {
	expect[sb].get(0, b.get_ondisk_length());
      } else {
	b.map(e.blob_offset, e.length, [&](uint64_t off, uint64_t len) {
	    expect[sb].get(off, len);
	    return 0;
	  });
      }
    }
  }

  vector<SharedBlob*> unshared_blobs;
  unshared_blobs.reserve(maybe_unshared_blobs.size());
  for (auto& p : expect) {
    dout(20) << " ? " << *p.first << " vs " << p.second << dendl;
    if (p.first->persistent->ref_map == p.second) {
      SharedBlob *sb = p.first;
      dout(20) << __func__ << "  unsharing " << *sb << dendl;
      unshared_blobs.push_back(sb);
      txc->unshare_blob(sb);
      uint64_t sbid = c->make_blob_unshared(sb);
      string key;
      get_shared_blob_key(sbid, &key);
      txc->t->rmkey(PREFIX_SHARED_BLOB, key);
    }
  }

  if (unshared_blobs.empty()) {
    return 0;
  }

  for (auto& e : h->extent_map.extent_map) {
    const bluestore_blob_t& b = e.blob->get_blob();
    SharedBlob *sb = e.blob->shared_blob.get();
    if (b.is_shared() &&
        std::find(unshared_blobs.begin(), unshared_blobs.end(),
                  sb) != unshared_blobs.end()) {
      dout(20) << __func__ << "  unsharing " << e << dendl;
      bluestore_blob_t& blob = e.blob->dirty_blob();
      blob.clear_flag(bluestore_blob_t::FLAG_SHARED);
      h->extent_map.dirty_range(e.logical_offset, 1);
    }
  }
  txc->write_onode(h);

  return 0;
}

int BlueStore::_remove(TransContext *txc,
		       CollectionRef& c,
		       OnodeRef &o)
{
  dout(15) << __func__ << " " << c->cid << " " << o->oid << dendl;
  int r = _do_remove(txc, c, o);
  dout(10) << __func__ << " " << c->cid << " " << o->oid << " = " << r << dendl;
  return r;
}

int BlueStore::_setattr(TransContext *txc,
			CollectionRef& c,
			OnodeRef& o,
			const string& name,
			bufferptr& val)
{
  dout(15) << __func__ << " " << c->cid << " " << o->oid
	   << " " << name << " (" << val.length() << " bytes)"
	   << dendl;
  int r = 0;
  if (val.is_partial()) {
    auto& b = o->onode.attrs[name.c_str()] = bufferptr(val.c_str(),
						       val.length());
    b.reassign_to_mempool(mempool::mempool_bluestore_cache_other);
  } else {
    auto& b = o->onode.attrs[name.c_str()] = val;
    b.reassign_to_mempool(mempool::mempool_bluestore_cache_other);
  }
  txc->write_onode(o);
  dout(10) << __func__ << " " << c->cid << " " << o->oid
	   << " " << name << " (" << val.length() << " bytes)"
	   << " = " << r << dendl;
  return r;
}

int BlueStore::_setattrs(TransContext *txc,
			 CollectionRef& c,
			 OnodeRef& o,
			 const map<string,bufferptr>& aset)
{
  dout(15) << __func__ << " " << c->cid << " " << o->oid
	   << " " << aset.size() << " keys"
	   << dendl;
  int r = 0;
  for (map<string,bufferptr>::const_iterator p = aset.begin();
       p != aset.end(); ++p) {
    if (p->second.is_partial()) {
      auto& b = o->onode.attrs[p->first.c_str()] =
	bufferptr(p->second.c_str(), p->second.length());
      b.reassign_to_mempool(mempool::mempool_bluestore_cache_other);
    } else {
      auto& b = o->onode.attrs[p->first.c_str()] = p->second;
      b.reassign_to_mempool(mempool::mempool_bluestore_cache_other);
    }
  }
  txc->write_onode(o);
  dout(10) << __func__ << " " << c->cid << " " << o->oid
	   << " " << aset.size() << " keys"
	   << " = " << r << dendl;
  return r;
}


int BlueStore::_rmattr(TransContext *txc,
		       CollectionRef& c,
		       OnodeRef& o,
		       const string& name)
{
  dout(15) << __func__ << " " << c->cid << " " << o->oid
	   << " " << name << dendl;
  int r = 0;
  auto it = o->onode.attrs.find(name.c_str());
  if (it == o->onode.attrs.end())
    goto out;

  o->onode.attrs.erase(it);
  txc->write_onode(o);

 out:
  dout(10) << __func__ << " " << c->cid << " " << o->oid
	   << " " << name << " = " << r << dendl;
  return r;
}

int BlueStore::_rmattrs(TransContext *txc,
			CollectionRef& c,
			OnodeRef& o)
{
  dout(15) << __func__ << " " << c->cid << " " << o->oid << dendl;
  int r = 0;

  if (o->onode.attrs.empty())
    goto out;

  o->onode.attrs.clear();
  txc->write_onode(o);

 out:
  dout(10) << __func__ << " " << c->cid << " " << o->oid << " = " << r << dendl;
  return r;
}

void BlueStore::_do_omap_clear(TransContext *txc, uint64_t id)
{
  KeyValueDB::Iterator it = db->get_iterator(PREFIX_OMAP);
  string prefix, tail;
  get_omap_header(id, &prefix);
  get_omap_tail(id, &tail);
  it->lower_bound(prefix);
  while (it->valid()) {
    if (it->key() >= tail) {
      dout(30) << __func__ << "  stop at " << pretty_binary_string(tail)
	       << dendl;
      break;
    }
    txc->t->rmkey(PREFIX_OMAP, it->key());
    dout(30) << __func__ << "  rm " << pretty_binary_string(it->key()) << dendl;
    it->next();
  }
}

int BlueStore::_omap_clear(TransContext *txc,
			   CollectionRef& c,
			   OnodeRef& o)
{
  dout(15) << __func__ << " " << c->cid << " " << o->oid << dendl;
  int r = 0;
  if (o->onode.has_omap()) {
    o->flush();
    _do_omap_clear(txc, o->onode.nid);
    o->onode.clear_omap_flag();
    txc->write_onode(o);
  }
  dout(10) << __func__ << " " << c->cid << " " << o->oid << " = " << r << dendl;
  return r;
}

int BlueStore::_omap_setkeys(TransContext *txc,
			     CollectionRef& c,
			     OnodeRef& o,
			     bufferlist &bl)
{
  dout(15) << __func__ << " " << c->cid << " " << o->oid << dendl;
  int r;
  bufferlist::iterator p = bl.begin();
  __u32 num;
  if (!o->onode.has_omap()) {
    o->onode.set_omap_flag();
    txc->write_onode(o);
  } else {
    txc->note_modified_object(o);
  }
  string final_key;
  _key_encode_u64(o->onode.nid, &final_key);
  final_key.push_back('.');
  ::decode(num, p);
  while (num--) {
    string key;
    bufferlist value;
    ::decode(key, p);
    ::decode(value, p);
    final_key.resize(9); // keep prefix
    final_key += key;
    dout(30) << __func__ << "  " << pretty_binary_string(final_key)
	     << " <- " << key << dendl;
    txc->t->set(PREFIX_OMAP, final_key, value);
  }
  r = 0;
  dout(10) << __func__ << " " << c->cid << " " << o->oid << " = " << r << dendl;
  return r;
}

int BlueStore::_omap_setheader(TransContext *txc,
			       CollectionRef& c,
			       OnodeRef &o,
			       bufferlist& bl)
{
  dout(15) << __func__ << " " << c->cid << " " << o->oid << dendl;
  int r;
  string key;
  if (!o->onode.has_omap()) {
    o->onode.set_omap_flag();
    txc->write_onode(o);
  } else {
    txc->note_modified_object(o);
  }
  get_omap_header(o->onode.nid, &key);
  txc->t->set(PREFIX_OMAP, key, bl);
  r = 0;
  dout(10) << __func__ << " " << c->cid << " " << o->oid << " = " << r << dendl;
  return r;
}

int BlueStore::_omap_rmkeys(TransContext *txc,
			    CollectionRef& c,
			    OnodeRef& o,
			    bufferlist& bl)
{
  dout(15) << __func__ << " " << c->cid << " " << o->oid << dendl;
  int r = 0;
  bufferlist::iterator p = bl.begin();
  __u32 num;
  string final_key;

  if (!o->onode.has_omap()) {
    goto out;
  }
  _key_encode_u64(o->onode.nid, &final_key);
  final_key.push_back('.');
  ::decode(num, p);
  while (num--) {
    string key;
    ::decode(key, p);
    final_key.resize(9); // keep prefix
    final_key += key;
    dout(30) << __func__ << "  rm " << pretty_binary_string(final_key)
	     << " <- " << key << dendl;
    txc->t->rmkey(PREFIX_OMAP, final_key);
  }
  txc->note_modified_object(o);

 out:
  dout(10) << __func__ << " " << c->cid << " " << o->oid << " = " << r << dendl;
  return r;
}

int BlueStore::_omap_rmkey_range(TransContext *txc,
				 CollectionRef& c,
				 OnodeRef& o,
				 const string& first, const string& last)
{
  dout(15) << __func__ << " " << c->cid << " " << o->oid << dendl;
  KeyValueDB::Iterator it;
  string key_first, key_last;
  int r = 0;
  if (!o->onode.has_omap()) {
    goto out;
  }
  o->flush();
  it = db->get_iterator(PREFIX_OMAP);
  get_omap_key(o->onode.nid, first, &key_first);
  get_omap_key(o->onode.nid, last, &key_last);
  it->lower_bound(key_first);
  while (it->valid()) {
    if (it->key() >= key_last) {
      dout(30) << __func__ << "  stop at " << pretty_binary_string(key_last)
	       << dendl;
      break;
    }
    txc->t->rmkey(PREFIX_OMAP, it->key());
    dout(30) << __func__ << "  rm " << pretty_binary_string(it->key()) << dendl;
    it->next();
  }
  txc->note_modified_object(o);

 out:
  dout(10) << __func__ << " " << c->cid << " " << o->oid << " = " << r << dendl;
  return r;
}

int BlueStore::_set_alloc_hint(
  TransContext *txc,
  CollectionRef& c,
  OnodeRef& o,
  uint64_t expected_object_size,
  uint64_t expected_write_size,
  uint32_t flags)
{
  dout(15) << __func__ << " " << c->cid << " " << o->oid
	   << " object_size " << expected_object_size
	   << " write_size " << expected_write_size
	   << " flags " << ceph_osd_alloc_hint_flag_string(flags)
	   << dendl;
  int r = 0;
  o->onode.expected_object_size = expected_object_size;
  o->onode.expected_write_size = expected_write_size;
  o->onode.alloc_hint_flags = flags;
  txc->write_onode(o);
  dout(10) << __func__ << " " << c->cid << " " << o->oid
	   << " object_size " << expected_object_size
	   << " write_size " << expected_write_size
	   << " flags " << ceph_osd_alloc_hint_flag_string(flags)
	   << " = " << r << dendl;
  return r;
}

int BlueStore::_clone(TransContext *txc,
		      CollectionRef& c,
		      OnodeRef& oldo,
		      OnodeRef& newo)
{
  dout(15) << __func__ << " " << c->cid << " " << oldo->oid << " -> "
	   << newo->oid << dendl;
  int r = 0;
  if (oldo->oid.hobj.get_hash() != newo->oid.hobj.get_hash()) {
    derr << __func__ << " mismatched hash on " << oldo->oid
	 << " and " << newo->oid << dendl;
    return -EINVAL;
  }

  _assign_nid(txc, newo);

  // clone data
  oldo->flush();
  _do_truncate(txc, c, newo, 0);
  if (cct->_conf->bluestore_clone_cow) {
    _do_clone_range(txc, c, oldo, newo, 0, oldo->onode.size, 0);
  } else {
    bufferlist bl;
    r = _do_read(c.get(), oldo, 0, oldo->onode.size, bl, 0);
    if (r < 0)
      goto out;
    r = _do_write(txc, c, newo, 0, oldo->onode.size, bl, 0);
    if (r < 0)
      goto out;
  }

  // clone attrs
  newo->onode.attrs = oldo->onode.attrs;

  // clone omap
  if (newo->onode.has_omap()) {
    dout(20) << __func__ << " clearing old omap data" << dendl;
    newo->flush();
    _do_omap_clear(txc, newo->onode.nid);
  }
  if (oldo->onode.has_omap()) {
    dout(20) << __func__ << " copying omap data" << dendl;
    if (!newo->onode.has_omap()) {
      newo->onode.set_omap_flag();
    }
    KeyValueDB::Iterator it = db->get_iterator(PREFIX_OMAP);
    string head, tail;
    get_omap_header(oldo->onode.nid, &head);
    get_omap_tail(oldo->onode.nid, &tail);
    it->lower_bound(head);
    while (it->valid()) {
      if (it->key() >= tail) {
	dout(30) << __func__ << "  reached tail" << dendl;
	break;
      } else {
	dout(30) << __func__ << "  got header/data "
		 << pretty_binary_string(it->key()) << dendl;
        string key;
	rewrite_omap_key(newo->onode.nid, it->key(), &key);
	txc->t->set(PREFIX_OMAP, key, it->value());
      }
      it->next();
    }
  } else {
    newo->onode.clear_omap_flag();
  }

  txc->write_onode(newo);
  r = 0;

 out:
  dout(10) << __func__ << " " << c->cid << " " << oldo->oid << " -> "
	   << newo->oid << " = " << r << dendl;
  return r;
}

int BlueStore::_do_clone_range(
  TransContext *txc,
  CollectionRef& c,
  OnodeRef& oldo,
  OnodeRef& newo,
  uint64_t srcoff,
  uint64_t length,
  uint64_t dstoff)
{
  dout(15) << __func__ << " " << c->cid << " " << oldo->oid << " -> "
	   << newo->oid
	   << " 0x" << std::hex << srcoff << "~" << length << " -> "
	   << " 0x" << dstoff << "~" << length << std::dec << dendl;
  oldo->extent_map.fault_range(db, srcoff, length);
  newo->extent_map.fault_range(db, dstoff, length);
  _dump_onode(oldo);
  _dump_onode(newo);

  // hmm, this could go into an ExtentMap::dup() method.
  vector<BlobRef> id_to_blob(oldo->extent_map.extent_map.size());
  for (auto &e : oldo->extent_map.extent_map) {
    e.blob->last_encoded_id = -1;
  }
  int n = 0;
  uint64_t end = srcoff + length;
  uint32_t dirty_range_begin = 0;
  uint32_t dirty_range_end = 0;
  bool src_dirty = false;
  for (auto ep = oldo->extent_map.seek_lextent(srcoff);
       ep != oldo->extent_map.extent_map.end();
       ++ep) {
    auto& e = *ep;
    if (e.logical_offset >= end) {
      break;
    }
    dout(20) << __func__ << "  src " << e << dendl;
    BlobRef cb;
    bool blob_duped = true;
    if (e.blob->last_encoded_id >= 0) {
      // blob is already duped
      cb = id_to_blob[e.blob->last_encoded_id];
      blob_duped = false;
    } else {
      // dup the blob
      const bluestore_blob_t& blob = e.blob->get_blob();
      // make sure it is shared
      if (!blob.is_shared()) {
	c->make_blob_shared(_assign_blobid(txc), e.blob);
	if (!src_dirty) {
	  src_dirty = true;
          dirty_range_begin = e.logical_offset;
        }
        assert(e.logical_end() > 0);
        // -1 to exclude next potential shard
        dirty_range_end = e.logical_end() - 1;
      } else {
	c->load_shared_blob(e.blob->shared_blob);
      }
      cb = new Blob();
      e.blob->last_encoded_id = n;
      id_to_blob[n] = cb;
      e.blob->dup(*cb);
      // bump the extent refs on the copied blob's extents
      for (auto p : blob.get_extents()) {
	if (p.is_valid()) {
	  e.blob->shared_blob->get_ref(p.offset, p.length);
	}
      }
      txc->write_shared_blob(e.blob->shared_blob);
      dout(20) << __func__ << "    new " << *cb << dendl;
    }
    // dup extent
    int skip_front, skip_back;
    if (e.logical_offset < srcoff) {
      skip_front = srcoff - e.logical_offset;
    } else {
      skip_front = 0;
    }
    if (e.logical_end() > end) {
      skip_back = e.logical_end() - end;
    } else {
      skip_back = 0;
    }
    Extent *ne = new Extent(e.logical_offset + skip_front + dstoff - srcoff,
			    e.blob_offset + skip_front,
			    e.length - skip_front - skip_back, cb);
    newo->extent_map.extent_map.insert(*ne);
    ne->blob->get_ref(c.get(), ne->blob_offset, ne->length);
    // fixme: we may leave parts of new blob unreferenced that could
    // be freed (relative to the shared_blob).
    txc->statfs_delta.stored() += ne->length;
    if (e.blob->get_blob().is_compressed()) {
      txc->statfs_delta.compressed_original() += ne->length;
      if (blob_duped){
        txc->statfs_delta.compressed() +=
          cb->get_blob().get_compressed_payload_length();
      }
    }
    dout(20) << __func__ << "  dst " << *ne << dendl;
    ++n;
  }
  if (src_dirty) {
    oldo->extent_map.dirty_range(dirty_range_begin,
      dirty_range_end - dirty_range_begin);
    txc->write_onode(oldo);
  }
  txc->write_onode(newo);

  if (dstoff + length > newo->onode.size) {
    newo->onode.size = dstoff + length;
  }
  newo->extent_map.dirty_range(dstoff, length);
  _dump_onode(oldo);
  _dump_onode(newo);
  return 0;
}

int BlueStore::_clone_range(TransContext *txc,
			    CollectionRef& c,
			    OnodeRef& oldo,
			    OnodeRef& newo,
			    uint64_t srcoff, uint64_t length, uint64_t dstoff)
{
  dout(15) << __func__ << " " << c->cid << " " << oldo->oid << " -> "
	   << newo->oid << " from 0x" << std::hex << srcoff << "~" << length
	   << " to offset 0x" << dstoff << std::dec << dendl;
  int r = 0;

  if (srcoff + length >= OBJECT_MAX_SIZE ||
      dstoff + length >= OBJECT_MAX_SIZE) {
    r = -E2BIG;
    goto out;
  }
  if (srcoff + length > oldo->onode.size) {
    r = -EINVAL;
    goto out;
  }

  _assign_nid(txc, newo);

  if (length > 0) {
    if (cct->_conf->bluestore_clone_cow) {
      _do_zero(txc, c, newo, dstoff, length);
      _do_clone_range(txc, c, oldo, newo, srcoff, length, dstoff);
    } else {
      bufferlist bl;
      r = _do_read(c.get(), oldo, srcoff, length, bl, 0);
      if (r < 0)
	goto out;
      r = _do_write(txc, c, newo, dstoff, bl.length(), bl, 0);
      if (r < 0)
	goto out;
    }
  }

  txc->write_onode(newo);
  r = 0;

 out:
  dout(10) << __func__ << " " << c->cid << " " << oldo->oid << " -> "
	   << newo->oid << " from 0x" << std::hex << srcoff << "~" << length
	   << " to offset 0x" << dstoff << std::dec
	   << " = " << r << dendl;
  return r;
}

int BlueStore::_rename(TransContext *txc,
		       CollectionRef& c,
		       OnodeRef& oldo,
		       OnodeRef& newo,
		       const ghobject_t& new_oid)
{
  dout(15) << __func__ << " " << c->cid << " " << oldo->oid << " -> "
	   << new_oid << dendl;
  int r;
  ghobject_t old_oid = oldo->oid;
  mempool::bluestore_cache_other::string new_okey;

  if (newo) {
    if (newo->exists) {
      r = -EEXIST;
      goto out;
    }
    assert(txc->onodes.count(newo) == 0);
  }

  txc->t->rmkey(PREFIX_OBJ, oldo->key.c_str(), oldo->key.size());

  // rewrite shards
  {
    oldo->extent_map.fault_range(db, 0, oldo->onode.size);
    get_object_key(cct, new_oid, &new_okey);
    string key;
    for (auto &s : oldo->extent_map.shards) {
      generate_extent_shard_key_and_apply(oldo->key, s.shard_info->offset, &key,
        [&](const string& final_key) {
          txc->t->rmkey(PREFIX_OBJ, final_key);
        }
      );
      s.dirty = true;
    }
  }

  newo = oldo;
  txc->write_onode(newo);

  // this adjusts oldo->{oid,key}, and reset oldo to a fresh empty
  // Onode in the old slot
  c->onode_map.rename(oldo, old_oid, new_oid, new_okey);
  r = 0;

 out:
  dout(10) << __func__ << " " << c->cid << " " << old_oid << " -> "
	   << new_oid << " = " << r << dendl;
  return r;
}

// collections

int BlueStore::_create_collection(
  TransContext *txc,
  const coll_t &cid,
  unsigned bits,
  CollectionRef *c)
{
  dout(15) << __func__ << " " << cid << " bits " << bits << dendl;
  int r;
  bufferlist bl;

  {
    RWLock::WLocker l(coll_lock);
    if (*c) {
      r = -EEXIST;
      goto out;
    }
    c->reset(
      new Collection(
	this,
	cache_shards[cid.hash_to_shard(cache_shards.size())],
	cid));
    (*c)->cnode.bits = bits;
    coll_map[cid] = *c;
  }
  ::encode((*c)->cnode, bl);
  txc->t->set(PREFIX_COLL, stringify(cid), bl);
  r = 0;

 out:
  dout(10) << __func__ << " " << cid << " bits " << bits << " = " << r << dendl;
  return r;
}

int BlueStore::_remove_collection(TransContext *txc, const coll_t &cid,
				  CollectionRef *c)
{
  dout(15) << __func__ << " " << cid << dendl;
  int r;

  {
    RWLock::WLocker l(coll_lock);
    if (!*c) {
      r = -ENOENT;
      goto out;
    }
    size_t nonexistent_count = 0;
    assert((*c)->exists);
    if ((*c)->onode_map.map_any([&](OnodeRef o) {
        if (o->exists) {
          dout(10) << __func__ << " " << o->oid << " " << o
                   << " exists in onode_map" << dendl;
          return true;
        }
        ++nonexistent_count;
        return false;
        })) {
      r = -ENOTEMPTY;
      goto out;
    }

    vector<ghobject_t> ls;
    ghobject_t next;
    // Enumerate onodes in db, up to nonexistent_count + 1
    // then check if all of them are marked as non-existent.
    // Bypass the check if returned number is greater than nonexistent_count
    r = _collection_list(c->get(), ghobject_t(), ghobject_t::get_max(),
                         nonexistent_count + 1, &ls, &next);
    if (r >= 0) {
      bool exists = false; //ls.size() > nonexistent_count;
      for (auto it = ls.begin(); !exists && it < ls.end(); ++it) {
        dout(10) << __func__ << " oid " << *it << dendl;
        auto onode = (*c)->onode_map.lookup(*it);
        exists = !onode || onode->exists;
        if (exists) {
          dout(10) << __func__ << " " << *it
                   << " exists in db" << dendl;
        }
      }
      if (!exists) {
        coll_map.erase(cid);
        txc->removed_collections.push_back(*c);
        (*c)->exists = false;
        c->reset();
        txc->t->rmkey(PREFIX_COLL, stringify(cid));
        r = 0;
      } else {
        dout(10) << __func__ << " " << cid
                 << " is non-empty" << dendl;
        r = -ENOTEMPTY;
      }
    }
  }

 out:
  dout(10) << __func__ << " " << cid << " = " << r << dendl;
  return r;
}

int BlueStore::_split_collection(TransContext *txc,
				CollectionRef& c,
				CollectionRef& d,
				unsigned bits, int rem)
{
  dout(15) << __func__ << " " << c->cid << " to " << d->cid << " "
	   << " bits " << bits << dendl;
  RWLock::WLocker l(c->lock);
  RWLock::WLocker l2(d->lock);
  int r;

  // flush all previous deferred writes on this sequencer.  this is a bit
  // heavyweight, but we need to make sure all deferred writes complete
  // before we split as the new collection's sequencer may need to order
  // this after those writes, and we don't bother with the complexity of
  // moving those TransContexts over to the new osr.
  _osr_drain_preceding(txc);

  // move any cached items (onodes and referenced shared blobs) that will
  // belong to the child collection post-split.  leave everything else behind.
  // this may include things that don't strictly belong to the now-smaller
  // parent split, but the OSD will always send us a split for every new
  // child.

  spg_t pgid, dest_pgid;
  bool is_pg = c->cid.is_pg(&pgid);
  assert(is_pg);
  is_pg = d->cid.is_pg(&dest_pgid);
  assert(is_pg);

  // the destination should initially be empty.
  assert(d->onode_map.empty());
  assert(d->shared_blob_set.empty());
  assert(d->cnode.bits == bits);

  c->split_cache(d.get());

  // adjust bits.  note that this will be redundant for all but the first
  // split call for this parent (first child).
  c->cnode.bits = bits;
  assert(d->cnode.bits == bits);
  r = 0;

  bufferlist bl;
  ::encode(c->cnode, bl);
  txc->t->set(PREFIX_COLL, stringify(c->cid), bl);

  dout(10) << __func__ << " " << c->cid << " to " << d->cid << " "
	   << " bits " << bits << " = " << r << dendl;
  return r;
}

// DB key value Histogram
#define KEY_SLAB 32
#define VALUE_SLAB 64

const string prefix_onode = "o";
const string prefix_onode_shard = "x";
const string prefix_other = "Z";

int BlueStore::DBHistogram::get_key_slab(size_t sz)
{
  return (sz/KEY_SLAB);
}

string BlueStore::DBHistogram::get_key_slab_to_range(int slab)
{
  int lower_bound = slab * KEY_SLAB;
  int upper_bound = (slab + 1) * KEY_SLAB;
  string ret = "[" + stringify(lower_bound) + "," + stringify(upper_bound) + ")";
  return ret;
}

int BlueStore::DBHistogram::get_value_slab(size_t sz)
{
  return (sz/VALUE_SLAB);
}

string BlueStore::DBHistogram::get_value_slab_to_range(int slab)
{
  int lower_bound = slab * VALUE_SLAB;
  int upper_bound = (slab + 1) * VALUE_SLAB;
  string ret = "[" + stringify(lower_bound) + "," + stringify(upper_bound) + ")";
  return ret;
}

void BlueStore::DBHistogram::update_hist_entry(map<string, map<int, struct key_dist> > &key_hist,
                      const string &prefix, size_t key_size, size_t value_size)
{
    uint32_t key_slab = get_key_slab(key_size);
    uint32_t value_slab = get_value_slab(value_size);
    key_hist[prefix][key_slab].count++;
    key_hist[prefix][key_slab].max_len = MAX(key_size, key_hist[prefix][key_slab].max_len);
    key_hist[prefix][key_slab].val_map[value_slab].count++;
    key_hist[prefix][key_slab].val_map[value_slab].max_len =
                  MAX(value_size, key_hist[prefix][key_slab].val_map[value_slab].max_len);
}

void BlueStore::DBHistogram::dump(Formatter *f)
{
  f->open_object_section("rocksdb_value_distribution");
  for (auto i : value_hist) {
    f->dump_unsigned(get_value_slab_to_range(i.first).data(), i.second);
  }
  f->close_section();

  f->open_object_section("rocksdb_key_value_histogram");
  for (auto i : key_hist) {
    f->dump_string("prefix", i.first);
    f->open_object_section("key_hist");
    for ( auto k : i.second) {
      f->dump_unsigned(get_key_slab_to_range(k.first).data(), k.second.count);
      f->dump_unsigned("max_len", k.second.max_len);
      f->open_object_section("value_hist");
      for ( auto j : k.second.val_map) {
	f->dump_unsigned(get_value_slab_to_range(j.first).data(), j.second.count);
	f->dump_unsigned("max_len", j.second.max_len);
      }
      f->close_section();
    }
    f->close_section();
  }
  f->close_section();
}

//Itrerates through the db and collects the stats
void BlueStore::generate_db_histogram(Formatter *f)
{
  //globals
  uint64_t num_onodes = 0;
  uint64_t num_shards = 0;
  uint64_t num_super = 0;
  uint64_t num_coll = 0;
  uint64_t num_omap = 0;
  uint64_t num_deferred = 0;
  uint64_t num_alloc = 0;
  uint64_t num_stat = 0;
  uint64_t num_others = 0;
  uint64_t num_shared_shards = 0;
  size_t max_key_size =0, max_value_size = 0;
  uint64_t total_key_size = 0, total_value_size = 0;
  size_t key_size = 0, value_size = 0;
  DBHistogram hist;

  utime_t start = ceph_clock_now();

  KeyValueDB::WholeSpaceIterator iter = db->get_iterator();
  iter->seek_to_first();
  while (iter->valid()) {
    dout(30) << __func__ << " Key: " << iter->key() << dendl;
    key_size = iter->key_size();
    value_size = iter->value_size();
    hist.value_hist[hist.get_value_slab(value_size)]++;
    max_key_size = MAX(max_key_size, key_size);
    max_value_size = MAX(max_value_size, value_size);
    total_key_size += key_size;
    total_value_size += value_size;

    pair<string,string> key(iter->raw_key());

    if (key.first == PREFIX_SUPER) {
	hist.update_hist_entry(hist.key_hist, PREFIX_SUPER, key_size, value_size);
	num_super++;
    } else if (key.first == PREFIX_STAT) {
	hist.update_hist_entry(hist.key_hist, PREFIX_STAT, key_size, value_size);
	num_stat++;
    } else if (key.first == PREFIX_COLL) {
	hist.update_hist_entry(hist.key_hist, PREFIX_COLL, key_size, value_size);
	num_coll++;
    } else if (key.first == PREFIX_OBJ) {
      if (key.second.back() == ONODE_KEY_SUFFIX) {
	hist.update_hist_entry(hist.key_hist, prefix_onode, key_size, value_size);
	num_onodes++;
      } else {
	hist.update_hist_entry(hist.key_hist, prefix_onode_shard, key_size, value_size);
	num_shards++;
      }
    } else if (key.first == PREFIX_OMAP) {
	hist.update_hist_entry(hist.key_hist, PREFIX_OMAP, key_size, value_size);
	num_omap++;
    } else if (key.first == PREFIX_DEFERRED) {
	hist.update_hist_entry(hist.key_hist, PREFIX_DEFERRED, key_size, value_size);
	num_deferred++;
    } else if (key.first == PREFIX_ALLOC || key.first == "b" ) {
	hist.update_hist_entry(hist.key_hist, PREFIX_ALLOC, key_size, value_size);
	num_alloc++;
    } else if (key.first == PREFIX_SHARED_BLOB) {
	hist.update_hist_entry(hist.key_hist, PREFIX_SHARED_BLOB, key_size, value_size);
	num_shared_shards++;
    } else {
	hist.update_hist_entry(hist.key_hist, prefix_other, key_size, value_size);
	num_others++;
    }
    iter->next();
  }

  utime_t duration = ceph_clock_now() - start;
  f->open_object_section("rocksdb_key_value_stats");
  f->dump_unsigned("num_onodes", num_onodes);
  f->dump_unsigned("num_shards", num_shards);
  f->dump_unsigned("num_super", num_super);
  f->dump_unsigned("num_coll", num_coll);
  f->dump_unsigned("num_omap", num_omap);
  f->dump_unsigned("num_deferred", num_deferred);
  f->dump_unsigned("num_alloc", num_alloc);
  f->dump_unsigned("num_stat", num_stat);
  f->dump_unsigned("num_shared_shards", num_shared_shards);
  f->dump_unsigned("num_others", num_others);
  f->dump_unsigned("max_key_size", max_key_size);
  f->dump_unsigned("max_value_size", max_value_size);
  f->dump_unsigned("total_key_size", total_key_size);
  f->dump_unsigned("total_value_size", total_value_size);
  f->close_section();

  hist.dump(f);

  dout(20) << __func__ << " finished in " << duration << " seconds" << dendl;

}

void BlueStore::_flush_cache()
{
  dout(10) << __func__ << dendl;
  for (auto i : cache_shards) {
    i->trim_all();
    assert(i->empty());
  }
  for (auto& p : coll_map) {
    if (!p.second->onode_map.empty()) {
      derr << __func__ << "stray onodes on " << p.first << dendl;
      p.second->onode_map.dump(cct, 0);
    }
    if (!p.second->shared_blob_set.empty()) {
      derr << __func__ << " stray shared blobs on " << p.first << dendl;
      p.second->shared_blob_set.dump(cct, 0);
    }
    assert(p.second->onode_map.empty());
    assert(p.second->shared_blob_set.empty());
  }
  coll_map.clear();
}

// For external caller.
// We use a best-effort policy instead, e.g.,
// we don't care if there are still some pinned onodes/data in the cache
// after this command is completed.
void BlueStore::flush_cache()
{
  dout(10) << __func__ << dendl;
  for (auto i : cache_shards) {
    i->trim_all();
  }
}

void BlueStore::_apply_padding(uint64_t head_pad,
			       uint64_t tail_pad,
			       bufferlist& padded)
{
  if (head_pad) {
    padded.prepend_zero(head_pad);
  }
  if (tail_pad) {
    padded.append_zero(tail_pad);
  }
  if (head_pad || tail_pad) {
    dout(20) << __func__ << "  can pad head 0x" << std::hex << head_pad
	      << " tail 0x" << tail_pad << std::dec << dendl;
    logger->inc(l_bluestore_write_pad_bytes, head_pad + tail_pad);
  }
}

// ===========================================
