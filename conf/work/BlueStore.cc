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

using bid_t = decltype(BlueStore::Blob::id);

// bluestore_cache_onode
MEMPOOL_DEFINE_OBJECT_FACTORY(BlueStore::Onode, bluestore_onode,
			      bluestore_cache_onode);

// bluestore_cache_other
MEMPOOL_DEFINE_OBJECT_FACTORY(BlueStore::Buffer, bluestore_buffer,
			      bluestore_cache_other);
MEMPOOL_DEFINE_OBJECT_FACTORY(BlueStore::Extent, bluestore_extent,
			      bluestore_cache_other);
MEMPOOL_DEFINE_OBJECT_FACTORY(BlueStore::Blob, bluestore_blob,
			      bluestore_cache_other);
MEMPOOL_DEFINE_OBJECT_FACTORY(BlueStore::SharedBlob, bluestore_shared_blob,
			      bluestore_cache_other);

// bluestore_txc
MEMPOOL_DEFINE_OBJECT_FACTORY(BlueStore::TransContext, bluestore_transcontext,
			      bluestore_txc);


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

// Garbage Collector

void BlueStore::GarbageCollector::process_protrusive_extents(
  const BlueStore::ExtentMap& extent_map, 
  uint64_t start_offset,
  uint64_t end_offset,
  uint64_t start_touch_offset,
  uint64_t end_touch_offset,
  uint64_t min_alloc_size)
{
  assert(start_offset <= start_touch_offset && end_offset>= end_touch_offset);

  uint64_t lookup_start_offset = P2ALIGN(start_offset, min_alloc_size);
  uint64_t lookup_end_offset = ROUND_UP_TO(end_offset, min_alloc_size);

  dout(30) << __func__ << " (hex): [" << std::hex
           << lookup_start_offset << ", " << lookup_end_offset 
           << ")" << std::dec << dendl;

  for (auto it = extent_map.seek_lextent(lookup_start_offset);
       it != extent_map.extent_map.end() &&
         it->logical_offset < lookup_end_offset;
       ++it) {
    uint64_t alloc_unit_start = it->logical_offset / min_alloc_size;
    uint64_t alloc_unit_end = (it->logical_end() - 1) / min_alloc_size;

    dout(30) << __func__ << " " << *it
             << "alloc_units: " << alloc_unit_start << ".." << alloc_unit_end
             << dendl;

    Blob* b = it->blob.get();

    if (it->logical_offset >=start_touch_offset &&
        it->logical_end() <= end_touch_offset) {
      // Process extents within the range affected by 
      // the current write request.
      // Need to take into account if existing extents
      // can be merged with them (uncompressed case)
      if (!b->get_blob().is_compressed()) {
        if (blob_info_counted && used_alloc_unit == alloc_unit_start) {
	  --blob_info_counted->expected_allocations; // don't need to allocate
                                                     // new AU for compressed
                                                     // data since another
                                                     // collocated uncompressed
                                                     // blob already exists
          dout(30) << __func__  << " --expected:"
                   << alloc_unit_start << dendl;
        }
        used_alloc_unit = alloc_unit_end;
        blob_info_counted =  nullptr;
      }
    } else if (b->get_blob().is_compressed()) {

      // additionally we take compressed blobs that were not impacted
      // by the write into account too
      BlobInfo& bi =
        affected_blobs.emplace(
          b, BlobInfo(b->get_referenced_bytes())).first->second;

      int adjust =
       (used_alloc_unit && used_alloc_unit == alloc_unit_start) ? 0 : 1;
      bi.expected_allocations += alloc_unit_end - alloc_unit_start + adjust;
      dout(30) << __func__  << " expected_allocations=" 
               << bi.expected_allocations << " end_au:"
               << alloc_unit_end << dendl;

      blob_info_counted =  &bi;
      used_alloc_unit = alloc_unit_end;

      assert(it->length <= bi.referenced_bytes);
       bi.referenced_bytes -= it->length;
      dout(30) << __func__ << " affected_blob:" << *b
               << " unref 0x" << std::hex << it->length
               << " referenced = 0x" << bi.referenced_bytes
               << std::dec << dendl;
      // NOTE: we can't move specific blob to resulting GC list here
      // when reference counter == 0 since subsequent extents might
      // decrement its expected_allocation. 
      // Hence need to enumerate all the extents first.
      if (!bi.collect_candidate) {
        bi.first_lextent = it;
        bi.collect_candidate = true;
      }
      bi.last_lextent = it;
    } else {
      if (blob_info_counted && used_alloc_unit == alloc_unit_start) {
        // don't need to allocate new AU for compressed data since another
        // collocated uncompressed blob already exists
    	--blob_info_counted->expected_allocations;
        dout(30) << __func__  << " --expected_allocations:"
		 << alloc_unit_start << dendl;
      }
      used_alloc_unit = alloc_unit_end;
      blob_info_counted = nullptr;
    }
  }

  for (auto b_it = affected_blobs.begin();
       b_it != affected_blobs.end();
       ++b_it) {
    Blob* b = b_it->first;
    BlobInfo& bi = b_it->second;
    if (bi.referenced_bytes == 0) {
      uint64_t len_on_disk = b_it->first->get_blob().get_ondisk_length();
      int64_t blob_expected_for_release =
        ROUND_UP_TO(len_on_disk, min_alloc_size) / min_alloc_size;

      dout(30) << __func__ << " " << *(b_it->first)
               << " expected4release=" << blob_expected_for_release
               << " expected_allocations=" << bi.expected_allocations
               << dendl;
      int64_t benefit = blob_expected_for_release - bi.expected_allocations;
      if (benefit >= g_conf->bluestore_gc_enable_blob_threshold) {
        if (bi.collect_candidate) {
          auto it = bi.first_lextent;
          bool bExit = false;
          do {
            if (it->blob.get() == b) {
              extents_to_collect.emplace_back(it->logical_offset, it->length);
            }
            bExit = it == bi.last_lextent;
            ++it;
          } while (!bExit);
        }
        expected_for_release += blob_expected_for_release;
        expected_allocations += bi.expected_allocations;
      }
    }
  }
}

int64_t BlueStore::GarbageCollector::estimate(
  uint64_t start_offset,
  uint64_t length,
  const BlueStore::ExtentMap& extent_map,
  const BlueStore::old_extent_map_t& old_extents,
  uint64_t min_alloc_size)
{

  affected_blobs.clear();
  extents_to_collect.clear();
  used_alloc_unit = boost::optional<uint64_t >();
  blob_info_counted = nullptr;

  gc_start_offset = start_offset;
  gc_end_offset = start_offset + length;

  uint64_t end_offset = start_offset + length;

  for (auto it = old_extents.begin(); it != old_extents.end(); ++it) {
    Blob* b = it->e.blob.get();
    if (b->get_blob().is_compressed()) {

      // update gc_start_offset/gc_end_offset if needed
      gc_start_offset = min(gc_start_offset, (uint64_t)it->e.blob_start());
      gc_end_offset = max(gc_end_offset, (uint64_t)it->e.blob_end());

      auto o = it->e.logical_offset;
      auto l = it->e.length;

      uint64_t ref_bytes = b->get_referenced_bytes();
      // micro optimization to bypass blobs that have no more references
      if (ref_bytes != 0) {
        dout(30) << __func__ << " affected_blob:" << *b
                 << " unref 0x" << std::hex << o << "~" << l
                 << std::dec << dendl;
	affected_blobs.emplace(b, BlobInfo(ref_bytes));
      }
    }
  }
  dout(30) << __func__ << " gc range(hex): [" << std::hex
           << gc_start_offset << ", " << gc_end_offset 
           << ")" << std::dec << dendl;

  // enumerate preceeding extents to check if they reference affected blobs
  if (gc_start_offset < start_offset || gc_end_offset > end_offset) {
    process_protrusive_extents(extent_map,
                               gc_start_offset,
			       gc_end_offset,
			       start_offset,
			       end_offset,
			       min_alloc_size);
  }
  return expected_for_release - expected_allocations;
}

// Cache

BlueStore::Cache *BlueStore::Cache::create(CephContext* cct, string type,
					   PerfCounters *logger)
{
  Cache *c = nullptr;

  if (type == "lru")
    c = new LRUCache(cct);
  else if (type == "2q")
    c = new TwoQCache(cct);
  else
    assert(0 == "unrecognized cache type");

  c->logger = logger;
  return c;
}

void BlueStore::Cache::trim_all()
{
  std::lock_guard<std::recursive_mutex> l(lock);
  _trim(0, 0);
}

void BlueStore::Cache::trim(
  uint64_t target_bytes,
  float target_meta_ratio,
  float target_data_ratio,
  float bytes_per_onode)
{
  std::lock_guard<std::recursive_mutex> l(lock);
  uint64_t current_meta = _get_num_onodes() * bytes_per_onode;
  uint64_t current_buffer = _get_buffer_bytes();
  uint64_t current = current_meta + current_buffer;

  uint64_t target_meta = target_bytes * target_meta_ratio;
  uint64_t target_buffer = target_bytes * target_data_ratio;

  // correct for overflow or float imprecision
  target_meta = min(target_bytes, target_meta);
  target_buffer = min(target_bytes - target_meta, target_buffer);

  if (current <= target_bytes) {
    dout(10) << __func__
	     << " shard target " << pretty_si_t(target_bytes)
	     << " meta/data ratios " << target_meta_ratio
	     << " + " << target_data_ratio << " ("
	     << pretty_si_t(target_meta) << " + "
	     << pretty_si_t(target_buffer) << "), "
	     << " current " << pretty_si_t(current) << " ("
	     << pretty_si_t(current_meta) << " + "
	     << pretty_si_t(current_buffer) << ")"
	     << dendl;
    return;
  }

  uint64_t need_to_free = current - target_bytes;
  uint64_t free_buffer = 0;
  uint64_t free_meta = 0;
  if (current_buffer > target_buffer) {
    free_buffer = current_buffer - target_buffer;
    if (free_buffer > need_to_free) {
      free_buffer = need_to_free;
    }
  }
  free_meta = need_to_free - free_buffer;

  // start bounds at what we have now
  uint64_t max_buffer = current_buffer - free_buffer;
  uint64_t max_meta = current_meta - free_meta;
  uint64_t max_onodes = max_meta / bytes_per_onode;

  dout(10) << __func__
	   << " shard target " << pretty_si_t(target_bytes)
	   << " ratio " << target_meta_ratio << " ("
	   << pretty_si_t(target_meta) << " + "
	   << pretty_si_t(target_buffer) << "), "
	   << " current " << pretty_si_t(current) << " ("
	   << pretty_si_t(current_meta) << " + "
	   << pretty_si_t(current_buffer) << "),"
	   << " need_to_free " << pretty_si_t(need_to_free) << " ("
	   << pretty_si_t(free_meta) << " + "
	   << pretty_si_t(free_buffer) << ")"
	   << " -> max " << max_onodes << " onodes + "
	   << max_buffer << " buffer"
	   << dendl;
  _trim(max_onodes, max_buffer);
}


// LRUCache
#undef dout_prefix
#define dout_prefix *_dout << "bluestore.LRUCache(" << this << ") "

void BlueStore::LRUCache::_touch_onode(OnodeRef& o)
{
  auto p = onode_lru.iterator_to(*o);
  onode_lru.erase(p);
  onode_lru.push_front(*o);
}

void BlueStore::LRUCache::_trim(uint64_t onode_max, uint64_t buffer_max)
{
  dout(20) << __func__ << " onodes " << onode_lru.size() << " / " << onode_max
	   << " buffers " << buffer_size << " / " << buffer_max
	   << dendl;

  _audit("trim start");

  // buffers
  while (buffer_size > buffer_max) {
    auto i = buffer_lru.rbegin();
    if (i == buffer_lru.rend()) {
      // stop if buffer_lru is now empty
      break;
    }

    Buffer *b = &*i;
    assert(b->is_clean());
    dout(20) << __func__ << " rm " << *b << dendl;
    b->space->_rm_buffer(this, b);
  }

  // onodes
  int num = onode_lru.size() - onode_max;
  if (num <= 0)
    return; // don't even try

  auto p = onode_lru.end();
  assert(p != onode_lru.begin());
  --p;
  int skipped = 0;
  int max_skipped = g_conf->bluestore_cache_trim_max_skip_pinned;
  while (num > 0) {
    Onode *o = &*p;
    int refs = o->nref.load();
    if (refs > 1) {
      dout(20) << __func__ << "  " << o->oid << " has " << refs
	       << " refs, skipping" << dendl;
      if (++skipped >= max_skipped) {
        dout(20) << __func__ << " maximum skip pinned reached; stopping with "
                 << num << " left to trim" << dendl;
        break;
      }

      if (p == onode_lru.begin()) {
        break;
      } else {
        p--;
        num--;
        continue;
      }
    }
    dout(30) << __func__ << "  rm " << o->oid << dendl;
    if (p != onode_lru.begin()) {
      onode_lru.erase(p--);
    } else {
      onode_lru.erase(p);
      assert(num == 1);
    }
    o->get();  // paranoia
    o->c->onode_map.remove(o->oid);
    o->put();
    --num;
  }
}

#ifdef DEBUG_CACHE
void BlueStore::LRUCache::_audit(const char *when)
{
  dout(10) << __func__ << " " << when << " start" << dendl;
  uint64_t s = 0;
  for (auto i = buffer_lru.begin(); i != buffer_lru.end(); ++i) {
    s += i->length;
  }
  if (s != buffer_size) {
    derr << __func__ << " buffer_size " << buffer_size << " actual " << s
	 << dendl;
    for (auto i = buffer_lru.begin(); i != buffer_lru.end(); ++i) {
      derr << __func__ << " " << *i << dendl;
    }
    assert(s == buffer_size);
  }
  dout(20) << __func__ << " " << when << " buffer_size " << buffer_size
	   << " ok" << dendl;
}
#endif

// TwoQCache
#undef dout_prefix
#define dout_prefix *_dout << "bluestore.2QCache(" << this << ") "


void BlueStore::TwoQCache::_touch_onode(OnodeRef& o)
{
  auto p = onode_lru.iterator_to(*o);
  onode_lru.erase(p);
  onode_lru.push_front(*o);
}

void BlueStore::TwoQCache::_add_buffer(Buffer *b, int level, Buffer *near)
{
  dout(20) << __func__ << " level " << level << " near " << near
	   << " on " << *b
	   << " which has cache_private " << b->cache_private << dendl;
  if (near) {
    b->cache_private = near->cache_private;
    switch (b->cache_private) {
    case BUFFER_WARM_IN:
      buffer_warm_in.insert(buffer_warm_in.iterator_to(*near), *b);
      break;
    case BUFFER_WARM_OUT:
      assert(b->is_empty());
      buffer_warm_out.insert(buffer_warm_out.iterator_to(*near), *b);
      break;
    case BUFFER_HOT:
      buffer_hot.insert(buffer_hot.iterator_to(*near), *b);
      break;
    default:
      assert(0 == "bad cache_private");
    }
  } else if (b->cache_private == BUFFER_NEW) {
    b->cache_private = BUFFER_WARM_IN;
    if (level > 0) {
      buffer_warm_in.push_front(*b);
    } else {
      // take caller hint to start at the back of the warm queue
      buffer_warm_in.push_back(*b);
    }
  } else {
    // we got a hint from discard
    switch (b->cache_private) {
    case BUFFER_WARM_IN:
      // stay in warm_in.  move to front, even though 2Q doesn't actually
      // do this.
      dout(20) << __func__ << " move to front of warm " << *b << dendl;
      buffer_warm_in.push_front(*b);
      break;
    case BUFFER_WARM_OUT:
      b->cache_private = BUFFER_HOT;
      // move to hot.  fall-thru
    case BUFFER_HOT:
      dout(20) << __func__ << " move to front of hot " << *b << dendl;
      buffer_hot.push_front(*b);
      break;
    default:
      assert(0 == "bad cache_private");
    }
  }
  if (!b->is_empty()) {
    buffer_bytes += b->length;
    buffer_list_bytes[b->cache_private] += b->length;
  }
}

void BlueStore::TwoQCache::_rm_buffer(Buffer *b)
{
  dout(20) << __func__ << " " << *b << dendl;
 if (!b->is_empty()) {
    assert(buffer_bytes >= b->length);
    buffer_bytes -= b->length;
    assert(buffer_list_bytes[b->cache_private] >= b->length);
    buffer_list_bytes[b->cache_private] -= b->length;
  }
  switch (b->cache_private) {
  case BUFFER_WARM_IN:
    buffer_warm_in.erase(buffer_warm_in.iterator_to(*b));
    break;
  case BUFFER_WARM_OUT:
    buffer_warm_out.erase(buffer_warm_out.iterator_to(*b));
    break;
  case BUFFER_HOT:
    buffer_hot.erase(buffer_hot.iterator_to(*b));
    break;
  default:
    assert(0 == "bad cache_private");
  }
}

void BlueStore::TwoQCache::_move_buffer(Cache *srcc, Buffer *b)
{
  TwoQCache *src = static_cast<TwoQCache*>(srcc);
  src->_rm_buffer(b);

  // preserve which list we're on (even if we can't preserve the order!)
  switch (b->cache_private) {
  case BUFFER_WARM_IN:
    assert(!b->is_empty());
    buffer_warm_in.push_back(*b);
    break;
  case BUFFER_WARM_OUT:
    assert(b->is_empty());
    buffer_warm_out.push_back(*b);
    break;
  case BUFFER_HOT:
    assert(!b->is_empty());
    buffer_hot.push_back(*b);
    break;
  default:
    assert(0 == "bad cache_private");
  }
  if (!b->is_empty()) {
    buffer_bytes += b->length;
    buffer_list_bytes[b->cache_private] += b->length;
  }
}

void BlueStore::TwoQCache::_adjust_buffer_size(Buffer *b, int64_t delta)
{
  dout(20) << __func__ << " delta " << delta << " on " << *b << dendl;
  if (!b->is_empty()) {
    assert((int64_t)buffer_bytes + delta >= 0);
    buffer_bytes += delta;
    assert((int64_t)buffer_list_bytes[b->cache_private] + delta >= 0);
    buffer_list_bytes[b->cache_private] += delta;
  }
}

void BlueStore::TwoQCache::_trim(uint64_t onode_max, uint64_t buffer_max)
{
  dout(20) << __func__ << " onodes " << onode_lru.size() << " / " << onode_max
	   << " buffers " << buffer_bytes << " / " << buffer_max
	   << dendl;

  _audit("trim start");

  // buffers
  if (buffer_bytes > buffer_max) {
    uint64_t kin = buffer_max * cct->_conf->bluestore_2q_cache_kin_ratio;
    uint64_t khot = buffer_max - kin;

    // pre-calculate kout based on average buffer size too,
    // which is typical(the warm_in and hot lists may change later)
    uint64_t kout = 0;
    uint64_t buffer_num = buffer_hot.size() + buffer_warm_in.size();
    if (buffer_num) {
      uint64_t buffer_avg_size = buffer_bytes / buffer_num;
      assert(buffer_avg_size);
      uint64_t calculated_buffer_num = buffer_max / buffer_avg_size;
      kout = calculated_buffer_num * cct->_conf->bluestore_2q_cache_kout_ratio;
    }

    if (buffer_list_bytes[BUFFER_HOT] < khot) {
      // hot is small, give slack to warm_in
      kin += khot - buffer_list_bytes[BUFFER_HOT];
    } else if (buffer_list_bytes[BUFFER_WARM_IN] < kin) {
      // warm_in is small, give slack to hot
      khot += kin - buffer_list_bytes[BUFFER_WARM_IN];
    }

    // adjust warm_in list
    int64_t to_evict_bytes = buffer_list_bytes[BUFFER_WARM_IN] - kin;
    uint64_t evicted = 0;

    while (to_evict_bytes > 0) {
      auto p = buffer_warm_in.rbegin();
      if (p == buffer_warm_in.rend()) {
        // stop if warm_in list is now empty
        break;
      }

      Buffer *b = &*p;
      assert(b->is_clean());
      dout(20) << __func__ << " buffer_warm_in -> out " << *b << dendl;
      assert(buffer_bytes >= b->length);
      buffer_bytes -= b->length;
      assert(buffer_list_bytes[BUFFER_WARM_IN] >= b->length);
      buffer_list_bytes[BUFFER_WARM_IN] -= b->length;
      to_evict_bytes -= b->length;
      evicted += b->length;
      b->state = Buffer::STATE_EMPTY;
      b->data.clear();
      buffer_warm_in.erase(buffer_warm_in.iterator_to(*b));
      buffer_warm_out.push_front(*b);
      b->cache_private = BUFFER_WARM_OUT;
    }

    if (evicted > 0) {
      dout(20) << __func__ << " evicted " << prettybyte_t(evicted)
               << " from warm_in list, done evicting warm_in buffers"
               << dendl;
    }

    // adjust hot list
    to_evict_bytes = buffer_list_bytes[BUFFER_HOT] - khot;
    evicted = 0;

    while (to_evict_bytes > 0) {
      auto p = buffer_hot.rbegin();
      if (p == buffer_hot.rend()) {
        // stop if hot list is now empty
        break;
      }

      Buffer *b = &*p;
      dout(20) << __func__ << " buffer_hot rm " << *b << dendl;
      assert(b->is_clean());
      // adjust evict size before buffer goes invalid
      to_evict_bytes -= b->length;
      evicted += b->length;
      b->space->_rm_buffer(this, b);
    }

    if (evicted > 0) {
      dout(20) << __func__ << " evicted " << prettybyte_t(evicted)
               << " from hot list, done evicting hot buffers"
               << dendl;
    }

    // adjust warm out list too, if necessary
    int64_t num = buffer_warm_out.size() - kout;
    while (num-- > 0) {
      Buffer *b = &*buffer_warm_out.rbegin();
      assert(b->is_empty());
      dout(20) << __func__ << " buffer_warm_out rm " << *b << dendl;
      b->space->_rm_buffer(this, b);
    }
  }

  // onodes
  int num = onode_lru.size() - onode_max;
  if (num <= 0)
    return; // don't even try

  auto p = onode_lru.end();
  assert(p != onode_lru.begin());
  --p;
  int skipped = 0;
  int max_skipped = g_conf->bluestore_cache_trim_max_skip_pinned;
  while (num > 0) {
    Onode *o = &*p;
    dout(20) << __func__ << " considering " << o << dendl;
    int refs = o->nref.load();
    if (refs > 1) {
      dout(20) << __func__ << "  " << o->oid << " has " << refs
	       << " refs; skipping" << dendl;
      if (++skipped >= max_skipped) {
        dout(20) << __func__ << " maximum skip pinned reached; stopping with "
                 << num << " left to trim" << dendl;
        break;
      }

      if (p == onode_lru.begin()) {
        break;
      } else {
        p--;
        num--;
        continue;
      }
    }
    dout(30) << __func__ << " " << o->oid << " num=" << num <<" lru size="<<onode_lru.size()<< dendl;
    if (p != onode_lru.begin()) {
      onode_lru.erase(p--);
    } else {
      onode_lru.erase(p);
      assert(num == 1);
    }
    o->get();  // paranoia
    o->c->onode_map.remove(o->oid);
    o->put();
    --num;
  }
}

#ifdef DEBUG_CACHE
void BlueStore::TwoQCache::_audit(const char *when)
{
  dout(10) << __func__ << " " << when << " start" << dendl;
  uint64_t s = 0;
  for (auto i = buffer_hot.begin(); i != buffer_hot.end(); ++i) {
    s += i->length;
  }

  uint64_t hot_bytes = s;
  if (hot_bytes != buffer_list_bytes[BUFFER_HOT]) {
    derr << __func__ << " hot_list_bytes "
         << buffer_list_bytes[BUFFER_HOT]
         << " != actual " << hot_bytes
         << dendl;
    assert(hot_bytes == buffer_list_bytes[BUFFER_HOT]);
  }

  for (auto i = buffer_warm_in.begin(); i != buffer_warm_in.end(); ++i) {
    s += i->length;
  }

  uint64_t warm_in_bytes = s - hot_bytes;
  if (warm_in_bytes != buffer_list_bytes[BUFFER_WARM_IN]) {
    derr << __func__ << " warm_in_list_bytes "
         << buffer_list_bytes[BUFFER_WARM_IN]
         << " != actual " << warm_in_bytes
         << dendl;
    assert(warm_in_bytes == buffer_list_bytes[BUFFER_WARM_IN]);
  }

  if (s != buffer_bytes) {
    derr << __func__ << " buffer_bytes " << buffer_bytes << " actual " << s
	 << dendl;
    assert(s == buffer_bytes);
  }

  dout(20) << __func__ << " " << when << " buffer_bytes " << buffer_bytes
	   << " ok" << dendl;
}
#endif


// BufferSpace

#undef dout_prefix
#define dout_prefix *_dout << "bluestore.BufferSpace(" << this << " in " << cache << ") "

void BlueStore::BufferSpace::_clear(Cache* cache)
{
  // note: we already hold cache->lock
  ldout(cache->cct, 20) << __func__ << dendl;
  while (!buffer_map.empty()) {
    _rm_buffer(cache, buffer_map.begin());
  }
}

int BlueStore::BufferSpace::_discard(Cache* cache, uint32_t offset, uint32_t length)
{
  // note: we already hold cache->lock
  ldout(cache->cct, 20) << __func__ << std::hex << " 0x" << offset << "~" << length
           << std::dec << dendl;
  int cache_private = 0;
  cache->_audit("discard start");
  auto i = _data_lower_bound(offset);
  uint32_t end = offset + length;
  while (i != buffer_map.end()) {
    Buffer *b = i->second.get();
    if (b->offset >= end) {
      break;
    }
    if (b->cache_private > cache_private) {
      cache_private = b->cache_private;
    }
    if (b->offset < offset) {
      int64_t front = offset - b->offset;
      if (b->end() > end) {
	// drop middle (split)
	uint32_t tail = b->end() - end;
	if (b->data.length()) {
	  bufferlist bl;
	  bl.substr_of(b->data, b->length - tail, tail);
	  Buffer *nb = new Buffer(this, b->state, b->seq, end, bl);
	  nb->maybe_rebuild();
	  _add_buffer(cache, nb, 0, b);
	} else {
	  _add_buffer(cache, new Buffer(this, b->state, b->seq, end, tail),
		      0, b);
	}
	if (!b->is_writing()) {
	  cache->_adjust_buffer_size(b, front - (int64_t)b->length);
	}
	b->truncate(front);
	b->maybe_rebuild();
	cache->_audit("discard end 1");
	break;
      } else {
	// drop tail
	if (!b->is_writing()) {
	  cache->_adjust_buffer_size(b, front - (int64_t)b->length);
	}
	b->truncate(front);
	b->maybe_rebuild();
	++i;
	continue;
      }
    }
    if (b->end() <= end) {
      // drop entire buffer
      _rm_buffer(cache, i++);
      continue;
    }
    // drop front
    uint32_t keep = b->end() - end;
    if (b->data.length()) {
      bufferlist bl;
      bl.substr_of(b->data, b->length - keep, keep);
      Buffer *nb = new Buffer(this, b->state, b->seq, end, bl);
      nb->maybe_rebuild();
      _add_buffer(cache, nb, 0, b);
    } else {
      _add_buffer(cache, new Buffer(this, b->state, b->seq, end, keep), 0, b);
    }
    _rm_buffer(cache, i);
    cache->_audit("discard end 2");
    break;
  }
  return cache_private;
}

void BlueStore::BufferSpace::read(
  Cache* cache, 
  uint32_t offset,
  uint32_t length,
  BlueStore::ready_regions_t& res,
  interval_set<uint32_t>& res_intervals)
{
  res.clear();
  res_intervals.clear();
  uint32_t want_bytes = length;
  uint32_t end = offset + length;

  {
    std::lock_guard<std::recursive_mutex> l(cache->lock);
    for (auto i = _data_lower_bound(offset);
         i != buffer_map.end() && offset < end && i->first < end;
         ++i) {
      Buffer *b = i->second.get();
      assert(b->end() > offset);
      if (b->is_writing() || b->is_clean()) {
        if (b->offset < offset) {
	  uint32_t skip = offset - b->offset;
	  uint32_t l = MIN(length, b->length - skip);
	  res[offset].substr_of(b->data, skip, l);
	  res_intervals.insert(offset, l);
	  offset += l;
	  length -= l;
	  if (!b->is_writing()) {
	    cache->_touch_buffer(b);
	  }
	  continue;
        }
        if (b->offset > offset) {
	  uint32_t gap = b->offset - offset;
	  if (length <= gap) {
	    break;
	  }
	  offset += gap;
	  length -= gap;
        }
        if (!b->is_writing()) {
	  cache->_touch_buffer(b);
        }
        if (b->length > length) {
	  res[offset].substr_of(b->data, 0, length);
	  res_intervals.insert(offset, length);
          break;
        } else {
	  res[offset].append(b->data);
	  res_intervals.insert(offset, b->length);
          if (b->length == length)
            break;
	  offset += b->length;
	  length -= b->length;
        }
      }
    }
  }

  uint64_t hit_bytes = res_intervals.size();
  assert(hit_bytes <= want_bytes);
  uint64_t miss_bytes = want_bytes - hit_bytes;
  cache->logger->inc(l_bluestore_buffer_hit_bytes, hit_bytes);
  cache->logger->inc(l_bluestore_buffer_miss_bytes, miss_bytes);
}

void BlueStore::BufferSpace::finish_write(Cache* cache, uint64_t seq)
{
  std::lock_guard<std::recursive_mutex> l(cache->lock);

  auto i = writing.begin();
  while (i != writing.end()) {
    if (i->seq > seq) {
      break;
    }
    if (i->seq < seq) {
      ++i;
      continue;
    }

    Buffer *b = &*i;
    assert(b->is_writing());

    if (b->flags & Buffer::FLAG_NOCACHE) {
      writing.erase(i++);
      ldout(cache->cct, 20) << __func__ << " discard " << *b << dendl;
      buffer_map.erase(b->offset);
    } else {
      b->state = Buffer::STATE_CLEAN;
      writing.erase(i++);
      b->maybe_rebuild();
      b->data.reassign_to_mempool(mempool::mempool_bluestore_cache_data);
      cache->_add_buffer(b, 1, nullptr);
      ldout(cache->cct, 20) << __func__ << " added " << *b << dendl;
    }
  }

  cache->_audit("finish_write end");
}

void BlueStore::BufferSpace::split(Cache* cache, size_t pos, BlueStore::BufferSpace &r)
{
  std::lock_guard<std::recursive_mutex> lk(cache->lock);
  if (buffer_map.empty())
    return;

  auto p = --buffer_map.end();
  while (true) {
    if (p->second->end() <= pos)
      break;

    if (p->second->offset < pos) {
      ldout(cache->cct, 30) << __func__ << " cut " << *p->second << dendl;
      size_t left = pos - p->second->offset;
      size_t right = p->second->length - left;
      if (p->second->data.length()) {
	bufferlist bl;
	bl.substr_of(p->second->data, left, right);
	r._add_buffer(cache, new Buffer(&r, p->second->state, p->second->seq, 0, bl),
		      0, p->second.get());
      } else {
	r._add_buffer(cache, new Buffer(&r, p->second->state, p->second->seq, 0, right),
		      0, p->second.get());
      }
      cache->_adjust_buffer_size(p->second.get(), -right);
      p->second->truncate(left);
      break;
    }

    assert(p->second->end() > pos);
    ldout(cache->cct, 30) << __func__ << " move " << *p->second << dendl;
    if (p->second->data.length()) {
      r._add_buffer(cache, new Buffer(&r, p->second->state, p->second->seq,
                               p->second->offset - pos, p->second->data),
                    0, p->second.get());
    } else {
      r._add_buffer(cache, new Buffer(&r, p->second->state, p->second->seq,
                               p->second->offset - pos, p->second->length),
                    0, p->second.get());
    }
    if (p == buffer_map.begin()) {
      _rm_buffer(cache, p);
      break;
    } else {
      _rm_buffer(cache, p--);
    }
  }
  assert(writing.empty());
}

// OnodeSpace

#undef dout_prefix
#define dout_prefix *_dout << "bluestore.OnodeSpace(" << this << " in " << cache << ") "

BlueStore::OnodeRef BlueStore::OnodeSpace::add(const ghobject_t& oid, OnodeRef o)
{
  std::lock_guard<std::recursive_mutex> l(cache->lock);
  auto p = onode_map.find(oid);
  if (p != onode_map.end()) {
    ldout(cache->cct, 30) << __func__ << " " << oid << " " << o
			  << " raced, returning existing " << p->second
			  << dendl;
    return p->second;
  }
  ldout(cache->cct, 30) << __func__ << " " << oid << " " << o << dendl;
  onode_map[oid] = o;
  cache->_add_onode(o, 1);
  return o;
}

BlueStore::OnodeRef BlueStore::OnodeSpace::lookup(const ghobject_t& oid)
{
  ldout(cache->cct, 30) << __func__ << dendl;
  OnodeRef o;
  bool hit = false;

  {
    std::lock_guard<std::recursive_mutex> l(cache->lock);
    ceph::unordered_map<ghobject_t,OnodeRef>::iterator p = onode_map.find(oid);
    if (p == onode_map.end()) {
      ldout(cache->cct, 30) << __func__ << " " << oid << " miss" << dendl;
    } else {
      ldout(cache->cct, 30) << __func__ << " " << oid << " hit " << p->second
			    << dendl;
      cache->_touch_onode(p->second);
      hit = true;
      o = p->second;
    }
  }

  if (hit) {
    cache->logger->inc(l_bluestore_onode_hits);
  } else {
    cache->logger->inc(l_bluestore_onode_misses);
  }
  return o;
}

void BlueStore::OnodeSpace::clear()
{
  std::lock_guard<std::recursive_mutex> l(cache->lock);
  ldout(cache->cct, 10) << __func__ << dendl;
  for (auto &p : onode_map) {
    cache->_rm_onode(p.second);
  }
  onode_map.clear();
}

bool BlueStore::OnodeSpace::empty()
{
  std::lock_guard<std::recursive_mutex> l(cache->lock);
  return onode_map.empty();
}

void BlueStore::OnodeSpace::rename(
  OnodeRef& oldo,
  const ghobject_t& old_oid,
  const ghobject_t& new_oid,
  const mempool::bluestore_cache_other::string& new_okey)
{
  std::lock_guard<std::recursive_mutex> l(cache->lock);
  ldout(cache->cct, 30) << __func__ << " " << old_oid << " -> " << new_oid
			<< dendl;
  ceph::unordered_map<ghobject_t,OnodeRef>::iterator po, pn;
  po = onode_map.find(old_oid);
  pn = onode_map.find(new_oid);
  assert(po != pn);

  assert(po != onode_map.end());
  if (pn != onode_map.end()) {
    ldout(cache->cct, 30) << __func__ << "  removing target " << pn->second
			  << dendl;
    cache->_rm_onode(pn->second);
    onode_map.erase(pn);
  }
  OnodeRef o = po->second;

  // install a non-existent onode at old location
  oldo.reset(new Onode(o->c, old_oid, o->key));
  po->second = oldo;
  cache->_add_onode(po->second, 1);

  // add at new position and fix oid, key
  onode_map.insert(make_pair(new_oid, o));
  cache->_touch_onode(o);
  o->oid = new_oid;
  o->key = new_okey;
}

bool BlueStore::OnodeSpace::map_any(std::function<bool(OnodeRef)> f)
{
  std::lock_guard<std::recursive_mutex> l(cache->lock);
  ldout(cache->cct, 20) << __func__ << dendl;
  for (auto& i : onode_map) {
    if (f(i.second)) {
      return true;
    }
  }
  return false;
}

void BlueStore::OnodeSpace::dump(CephContext *cct, int lvl)
{
  for (auto& i : onode_map) {
    ldout(cct, lvl) << i.first << " : " << i.second << dendl;
  }
}

// SharedBlob

#undef dout_prefix
#define dout_prefix *_dout << "bluestore.sharedblob(" << this << ") "

ostream& operator<<(ostream& out, const BlueStore::SharedBlob& sb)
{
  out << "SharedBlob(" << &sb;
  
  if (sb.loaded) {
    out << " loaded " << *sb.persistent;
  } else {
    out << " sbid 0x" << std::hex << sb.sbid_unloaded << std::dec;
  }
  return out << ")";
}

BlueStore::SharedBlob::SharedBlob(uint64_t i, Collection *_coll)
  : coll(_coll), sbid_unloaded(i)
{
  assert(sbid_unloaded > 0);
  if (get_cache()) {
    get_cache()->add_blob();
  }
}

BlueStore::SharedBlob::~SharedBlob()
{
  if (get_cache()) {   // the dummy instances have a nullptr
    std::lock_guard<std::recursive_mutex> l(get_cache()->lock);
    bc._clear(get_cache());
    get_cache()->rm_blob();
  }
  if (loaded && persistent) {
    delete persistent; 
  }
}

void BlueStore::SharedBlob::put()
{
  if (--nref == 0) {
    ldout(coll->store->cct, 20) << __func__ << " " << this
			     << " removing self from set " << get_parent()
			     << dendl;
    if (get_parent()) {
      get_parent()->remove(this);
    }
    delete this;
  }
}

void BlueStore::SharedBlob::get_ref(uint64_t offset, uint32_t length)
{
  assert(persistent);
  persistent->ref_map.get(offset, length);
}

void BlueStore::SharedBlob::put_ref(uint64_t offset, uint32_t length,
				    PExtentVector *r,
				    set<SharedBlob*> *maybe_unshared)
{
  assert(persistent);
  bool maybe = false;
  persistent->ref_map.put(offset, length, r, maybe_unshared ? &maybe : nullptr);
  if (maybe_unshared && maybe) {
    maybe_unshared->insert(this);
  }
}

// SharedBlobSet

#undef dout_prefix
#define dout_prefix *_dout << "bluestore.sharedblobset(" << this << ") "

void BlueStore::SharedBlobSet::dump(CephContext *cct, int lvl)
{
  std::lock_guard<std::mutex> l(lock);
  for (auto& i : sb_map) {
    ldout(cct, lvl) << i.first << " : " << *i.second << dendl;
  }
}

// Blob

#undef dout_prefix
#define dout_prefix *_dout << "bluestore.blob(" << this << ") "

ostream& operator<<(ostream& out, const BlueStore::Blob& b)
{
  out << "Blob(" << &b;
  if (b.is_spanning()) {
    out << " spanning " << b.id;
  }
  out << " " << b.get_blob() << " " << b.get_blob_use_tracker();
  if (b.shared_blob) {
    out << " " << *b.shared_blob;
  } else {
    out << " (shared_blob=NULL)";
  }
  out << ")";
  return out;
}

void BlueStore::Blob::discard_unallocated(Collection *coll)
{
  if (get_blob().is_shared()) {
    return;
  }
  if (get_blob().is_compressed()) {
    bool discard = false;
    bool all_invalid = true;
    for (auto e : get_blob().get_extents()) {
      if (!e.is_valid()) {
        discard = true;
      } else {
        all_invalid = false;
      }
    }
    assert(discard == all_invalid); // in case of compressed blob all
				    // or none pextents are invalid.
    if (discard) {
      shared_blob->bc.discard(shared_blob->get_cache(), 0,
                              get_blob().get_logical_length());
    }
  } else {
    size_t pos = 0;
    for (auto e : get_blob().get_extents()) {
      if (!e.is_valid()) {
	ldout(coll->store->cct, 20) << __func__ << " 0x" << std::hex << pos
				    << "~" << e.length
				    << std::dec << dendl;
	shared_blob->bc.discard(shared_blob->get_cache(), pos, e.length);
      }
      pos += e.length;
    }
    if (get_blob().can_prune_tail()) {
      dirty_blob().prune_tail();
      used_in_blob.prune_tail(get_blob().get_ondisk_length());
      auto cct = coll->store->cct; //used by dout
      dout(20) << __func__ << " pruned tail, now " << get_blob() << dendl;
    }
  }
}

void BlueStore::Blob::get_ref(
  Collection *coll,
  uint32_t offset,
  uint32_t length)
{
  // Caller has to initialize Blob's logical length prior to increment 
  // references.  Otherwise one is neither unable to determine required
  // amount of counters in case of per-au tracking nor obtain min_release_size
  // for single counter mode.
  assert(get_blob().get_logical_length() != 0);
  auto cct = coll->store->cct;
  dout(20) << __func__ << " 0x" << std::hex << offset << "~" << length
           << std::dec << " " << *this << dendl;

  if (used_in_blob.is_empty()) {
    uint32_t min_release_size =
      get_blob().get_release_size(coll->store->min_alloc_size);
    uint64_t l = get_blob().get_logical_length();
    dout(20) << __func__ << " init 0x" << std::hex << l << ", "
             << min_release_size << std::dec << dendl;
    used_in_blob.init(l, min_release_size);
  }
  used_in_blob.get(
    offset,
    length);
}

bool BlueStore::Blob::put_ref(
  Collection *coll,
  uint32_t offset,
  uint32_t length,
  PExtentVector *r)
{
  PExtentVector logical;

  auto cct = coll->store->cct;
  dout(20) << __func__ << " 0x" << std::hex << offset << "~" << length
           << std::dec << " " << *this << dendl;
  
  bool empty = used_in_blob.put(
    offset,
    length,
    &logical);
  r->clear();
  // nothing to release
  if (!empty && logical.empty()) {
    return false;
  }

  bluestore_blob_t& b = dirty_blob();
  return b.release_extents(empty, logical, r);
}

bool BlueStore::Blob::can_reuse_blob(uint32_t min_alloc_size,
                		     uint32_t target_blob_size,
		                     uint32_t b_offset,
		                     uint32_t *length0) {
  assert(min_alloc_size);
  assert(target_blob_size);
  if (!get_blob().is_mutable()) {
    return false;
  }

  uint32_t length = *length0;
  uint32_t end = b_offset + length;

  // Currently for the sake of simplicity we omit blob reuse if data is
  // unaligned with csum chunk. Later we can perform padding if needed.
  if (get_blob().has_csum() &&
     ((b_offset % get_blob().get_csum_chunk_size()) != 0 ||
      (end % get_blob().get_csum_chunk_size()) != 0)) {
    return false;
  }

  auto blen = get_blob().get_logical_length();
  uint32_t new_blen = blen;

  // make sure target_blob_size isn't less than current blob len
  target_blob_size = MAX(blen, target_blob_size);

  if (b_offset >= blen) {
    // new data totally stands out of the existing blob
    new_blen = end;
  } else {
    // new data overlaps with the existing blob
    new_blen = MAX(blen, end);

    uint32_t overlap = 0;
    if (new_blen > blen) {
      overlap = blen - b_offset;
    } else {
      overlap = length;
    }

    if (!get_blob().is_unallocated(b_offset, overlap)) {
      // abort if any piece of the overlap has already been allocated
      return false;
    }
  }

  if (new_blen > blen) {
    int64_t overflow = int64_t(new_blen) - target_blob_size;
    // Unable to decrease the provided length to fit into max_blob_size
    if (overflow >= length) {
      return false;
    }

    // FIXME: in some cases we could reduce unused resolution
    if (get_blob().has_unused()) {
      return false;
    }

    if (overflow > 0) {
      new_blen -= overflow;
      length -= overflow;
      *length0 = length;
    }

    if (new_blen > blen) {
      dirty_blob().add_tail(new_blen);
      used_in_blob.add_tail(new_blen,
                            get_blob().get_release_size(min_alloc_size));
    }
  }
  return true;
}

void BlueStore::Blob::split(Collection *coll, uint32_t blob_offset, Blob *r)
{
  auto cct = coll->store->cct; //used by dout
  dout(10) << __func__ << " 0x" << std::hex << blob_offset << std::dec
	   << " start " << *this << dendl;
  assert(blob.can_split());
  assert(used_in_blob.can_split());
  bluestore_blob_t &lb = dirty_blob();
  bluestore_blob_t &rb = r->dirty_blob();

  used_in_blob.split(
    blob_offset,
    &(r->used_in_blob));

  lb.split(blob_offset, rb);
  shared_blob->bc.split(shared_blob->get_cache(), blob_offset, r->shared_blob->bc);

  dout(10) << __func__ << " 0x" << std::hex << blob_offset << std::dec
	   << " finish " << *this << dendl;
  dout(10) << __func__ << " 0x" << std::hex << blob_offset << std::dec
	   << "    and " << *r << dendl;
}

#ifndef CACHE_BLOB_BL
void BlueStore::Blob::decode(
  Collection *coll,
  bufferptr::iterator& p,
  uint64_t struct_v,
  uint64_t* sbid,
  bool include_ref_map)
{
  denc(blob, p, struct_v);
  if (blob.is_shared()) {
    denc(*sbid, p);
  }
  if (include_ref_map) {
    if (struct_v > 1) {
      used_in_blob.decode(p);
    } else {
      used_in_blob.clear();
      bluestore_extent_ref_map_t legacy_ref_map;
      legacy_ref_map.decode(p);
      for (auto r : legacy_ref_map.ref_map) {
        get_ref(
          coll,
          r.first,
          r.second.refs * r.second.length);
      }
    }
  }
}
#endif

// Extent

ostream& operator<<(ostream& out, const BlueStore::Extent& e)
{
  return out << std::hex << "0x" << e.logical_offset << "~" << e.length
	     << ": 0x" << e.blob_offset << "~" << e.length << std::dec
	     << " " << *e.blob;
}

// OldExtent
BlueStore::OldExtent* BlueStore::OldExtent::create(CollectionRef c,
						   uint32_t lo,
						   uint32_t o,
						   uint32_t l,
						   BlobRef& b) {
  OldExtent* oe = new OldExtent(lo, o, l, b);
  b->put_ref(c.get(), o, l, &(oe->r));
  oe->blob_empty = b->get_referenced_bytes() == 0;
  return oe;
}

// ExtentMap

#undef dout_prefix
#define dout_prefix *_dout << "bluestore.extentmap(" << this << ") "

BlueStore::ExtentMap::ExtentMap(Onode *o)
  : onode(o),
    inline_bl(
      o->c->store->cct->_conf->bluestore_extent_map_inline_shard_prealloc_size) {
}

void BlueStore::ExtentMap::update(KeyValueDB::Transaction t,
                                  bool force)
{
  auto cct = onode->c->store->cct; //used by dout
  dout(20) << __func__ << " " << onode->oid << (force ? " force" : "") << dendl;
  if (onode->onode.extent_map_shards.empty()) {
    if (inline_bl.length() == 0) {
      unsigned n;
      // we need to encode inline_bl to measure encoded length
      bool never_happen = encode_some(0, OBJECT_MAX_SIZE, inline_bl, &n);
      inline_bl.reassign_to_mempool(mempool::mempool_bluestore_cache_other);
      assert(!never_happen);
      size_t len = inline_bl.length();
      dout(20) << __func__ << "  inline shard " << len << " bytes from " << n
	       << " extents" << dendl;
      if (!force && len > cct->_conf->bluestore_extent_map_shard_max_size) {
	request_reshard(0, OBJECT_MAX_SIZE);
	return;
      }
    }
    // will persist in the onode key.
  } else {
    // pending shard update
    struct dirty_shard_t {
      Shard *shard;
      bufferlist bl;
      dirty_shard_t(Shard *s) : shard(s) {}
    };
    vector<dirty_shard_t> encoded_shards;
    // allocate slots for all shards in a single call instead of
    // doing multiple allocations - one per each dirty shard
    encoded_shards.reserve(shards.size());

    auto p = shards.begin();
    auto prev_p = p;
    while (p != shards.end()) {
      assert(p->shard_info->offset >= prev_p->shard_info->offset);
      auto n = p;
      ++n;
      if (p->dirty) {
	uint32_t endoff;
	if (n == shards.end()) {
	  endoff = OBJECT_MAX_SIZE;
	} else {
	  endoff = n->shard_info->offset;
	}
	encoded_shards.emplace_back(dirty_shard_t(&(*p)));
        bufferlist& bl = encoded_shards.back().bl;
	if (encode_some(p->shard_info->offset, endoff - p->shard_info->offset,
			bl, &p->extents)) {
	  if (force) {
	    derr << __func__ << "  encode_some needs reshard" << dendl;
	    assert(!force);
	  }
	}
        size_t len = bl.length();

	dout(20) << __func__ << "  shard 0x" << std::hex
		 << p->shard_info->offset << std::dec << " is " << len
		 << " bytes (was " << p->shard_info->bytes << ") from "
		 << p->extents << " extents" << dendl;

        if (!force) {
	  if (len > cct->_conf->bluestore_extent_map_shard_max_size) {
	    // we are big; reshard ourselves
	    request_reshard(p->shard_info->offset, endoff);
	  }
	  // avoid resharding the trailing shard, even if it is small
	  else if (n != shards.end() &&
		   len < g_conf->bluestore_extent_map_shard_min_size) {
            assert(endoff != OBJECT_MAX_SIZE);
	    if (p == shards.begin()) {
	      // we are the first shard, combine with next shard
	      request_reshard(p->shard_info->offset, endoff + 1);
	    } else {
	      // combine either with the previous shard or the next,
	      // whichever is smaller
	      if (prev_p->shard_info->bytes > n->shard_info->bytes) {
		request_reshard(p->shard_info->offset, endoff + 1);
	      } else {
		request_reshard(prev_p->shard_info->offset, endoff);
	      }
	    }
	  }
        }
      }
      prev_p = p;
      p = n;
    }
    if (needs_reshard()) {
      return;
    }

    // schedule DB update for dirty shards
    string key;
    for (auto& it : encoded_shards) {
      it.shard->dirty = false;
      it.shard->shard_info->bytes = it.bl.length();
      generate_extent_shard_key_and_apply(
	onode->key,
	it.shard->shard_info->offset,
	&key,
        [&](const string& final_key) {
          t->set(PREFIX_OBJ, final_key, it.bl);
        }
      );
    }
  }
}

bid_t BlueStore::ExtentMap::allocate_spanning_blob_id()
{
  if (spanning_blob_map.empty())
    return 0;
  bid_t bid = spanning_blob_map.rbegin()->first + 1;
  // bid is valid and available.
  if (bid >= 0)
    return bid;
  // Find next unused bid;
  bid = rand() % (numeric_limits<bid_t>::max() + 1);
  const auto begin_bid = bid;
  do {
    if (!spanning_blob_map.count(bid))
      return bid;
    else {
      bid++;
      if (bid < 0) bid = 0;
    }
  } while (bid != begin_bid);
  assert(0 == "no available blob id");
}

void BlueStore::ExtentMap::reshard(
  KeyValueDB *db,
  KeyValueDB::Transaction t)
{
  auto cct = onode->c->store->cct; // used by dout

  dout(10) << __func__ << " 0x[" << std::hex << needs_reshard_begin << ","
	   << needs_reshard_end << ")" << std::dec
	   << " of " << onode->onode.extent_map_shards.size()
	   << " shards on " << onode->oid << dendl;
  for (auto& p : spanning_blob_map) {
    dout(20) << __func__ << "   spanning blob " << p.first << " " << *p.second
	     << dendl;
  }
  // determine shard index range
  unsigned si_begin = 0, si_end = 0;
  if (!shards.empty()) {
    while (si_begin + 1 < shards.size() &&
	   shards[si_begin + 1].shard_info->offset <= needs_reshard_begin) {
      ++si_begin;
    }
    needs_reshard_begin = shards[si_begin].shard_info->offset;
    for (si_end = si_begin; si_end < shards.size(); ++si_end) {
      if (shards[si_end].shard_info->offset >= needs_reshard_end) {
	needs_reshard_end = shards[si_end].shard_info->offset;
	break;
      }
    }
    if (si_end == shards.size()) {
      needs_reshard_end = OBJECT_MAX_SIZE;
    }
    dout(20) << __func__ << "   shards [" << si_begin << "," << si_end << ")"
	     << " over 0x[" << std::hex << needs_reshard_begin << ","
	     << needs_reshard_end << ")" << std::dec << dendl;
  }

  fault_range(db, needs_reshard_begin, (needs_reshard_end - needs_reshard_begin));

  // we may need to fault in a larger interval later must have all
  // referring extents for spanning blobs loaded in order to have
  // accurate use_tracker values.
  uint32_t spanning_scan_begin = needs_reshard_begin;
  uint32_t spanning_scan_end = needs_reshard_end;

  // remove old keys
  string key;
  for (unsigned i = si_begin; i < si_end; ++i) {
    generate_extent_shard_key_and_apply(
      onode->key, shards[i].shard_info->offset, &key,
      [&](const string& final_key) {
	t->rmkey(PREFIX_OBJ, final_key);
      }
      );
  }

  // calculate average extent size
  unsigned bytes = 0;
  unsigned extents = 0;
  if (onode->onode.extent_map_shards.empty()) {
    bytes = inline_bl.length();
    extents = extent_map.size();
  } else {
    for (unsigned i = si_begin; i < si_end; ++i) {
      bytes += shards[i].shard_info->bytes;
      extents += shards[i].extents;
    }
  }
  unsigned target = cct->_conf->bluestore_extent_map_shard_target_size;
  unsigned slop = target *
    cct->_conf->bluestore_extent_map_shard_target_size_slop;
  unsigned extent_avg = bytes / MAX(1, extents);
  dout(20) << __func__ << "  extent_avg " << extent_avg << ", target " << target
	   << ", slop " << slop << dendl;

  // reshard
  unsigned estimate = 0;
  unsigned offset = needs_reshard_begin;
  vector<bluestore_onode_t::shard_info> new_shard_info;
  unsigned max_blob_end = 0;
  Extent dummy(needs_reshard_begin);
  for (auto e = extent_map.lower_bound(dummy);
       e != extent_map.end();
       ++e) {
    if (e->logical_offset >= needs_reshard_end) {
      break;
    }
    dout(30) << " extent " << *e << dendl;

    // disfavor shard boundaries that span a blob
    bool would_span = (e->logical_offset < max_blob_end) || e->blob_offset;
    if (estimate &&
	estimate + extent_avg > target + (would_span ? slop : 0)) {
      // new shard
      if (offset == needs_reshard_begin) {
	new_shard_info.emplace_back(bluestore_onode_t::shard_info());
	new_shard_info.back().offset = offset;
	dout(20) << __func__ << "  new shard 0x" << std::hex << offset
                 << std::dec << dendl;
      }
      offset = e->logical_offset;
      new_shard_info.emplace_back(bluestore_onode_t::shard_info());
      new_shard_info.back().offset = offset;
      dout(20) << __func__ << "  new shard 0x" << std::hex << offset
	       << std::dec << dendl;
      estimate = 0;
    }
    estimate += extent_avg;
    unsigned bs = e->blob_start();
    if (bs < spanning_scan_begin) {
      spanning_scan_begin = bs;
    }
    uint32_t be = e->blob_end();
    if (be > max_blob_end) {
      max_blob_end = be;
    }
    if (be > spanning_scan_end) {
      spanning_scan_end = be;
    }
  }
  if (new_shard_info.empty() && (si_begin > 0 ||
				 si_end < shards.size())) {
    // we resharded a partial range; we must produce at least one output
    // shard
    new_shard_info.emplace_back(bluestore_onode_t::shard_info());
    new_shard_info.back().offset = needs_reshard_begin;
    dout(20) << __func__ << "  new shard 0x" << std::hex << needs_reshard_begin
	     << std::dec << " (singleton degenerate case)" << dendl;
  }

  auto& sv = onode->onode.extent_map_shards;
  dout(20) << __func__ << "  new " << new_shard_info << dendl;
  dout(20) << __func__ << "  old " << sv << dendl;
  if (sv.empty()) {
    // no old shards to keep
    sv.swap(new_shard_info);
    init_shards(true, true);
  } else {
    // splice in new shards
    sv.erase(sv.begin() + si_begin, sv.begin() + si_end);
    shards.erase(shards.begin() + si_begin, shards.begin() + si_end);
    sv.insert(
      sv.begin() + si_begin,
      new_shard_info.begin(),
      new_shard_info.end());
    shards.insert(shards.begin() + si_begin, new_shard_info.size(), Shard());
    si_end = si_begin + new_shard_info.size();

    assert(sv.size() == shards.size());

    // note that we need to update every shard_info of shards here,
    // as sv might have been totally re-allocated above
    for (unsigned i = 0; i < shards.size(); i++) {
      shards[i].shard_info = &sv[i];
    }

    // mark newly added shards as dirty
    for (unsigned i = si_begin; i < si_end; ++i) {
      shards[i].loaded = true;
      shards[i].dirty = true;
    }
  }
  dout(20) << __func__ << "  fin " << sv << dendl;
  inline_bl.clear();

  if (sv.empty()) {
    // no more shards; unspan all previously spanning blobs
    auto p = spanning_blob_map.begin();
    while (p != spanning_blob_map.end()) {
      p->second->id = -1;
      dout(30) << __func__ << " un-spanning " << *p->second << dendl;
      p = spanning_blob_map.erase(p);
    }
  } else {
    // identify new spanning blobs
    dout(20) << __func__ << " checking spanning blobs 0x[" << std::hex
	     << spanning_scan_begin << "," << spanning_scan_end << ")" << dendl;
    if (spanning_scan_begin < needs_reshard_begin) {
      fault_range(db, spanning_scan_begin,
		  needs_reshard_begin - spanning_scan_begin);
    }
    if (spanning_scan_end > needs_reshard_end) {
      fault_range(db, needs_reshard_end,
		  spanning_scan_end - needs_reshard_end);
    }
    auto sp = sv.begin() + si_begin;
    auto esp = sv.end();
    unsigned shard_start = sp->offset;
    unsigned shard_end;
    ++sp;
    if (sp == esp) {
      shard_end = OBJECT_MAX_SIZE;
    } else {
      shard_end = sp->offset;
    }
    Extent dummy(needs_reshard_begin);
    for (auto e = extent_map.lower_bound(dummy); e != extent_map.end(); ++e) {
      if (e->logical_offset >= needs_reshard_end) {
	break;
      }
      dout(30) << " extent " << *e << dendl;
      while (e->logical_offset >= shard_end) {
	shard_start = shard_end;
	assert(sp != esp);
	++sp;
	if (sp == esp) {
	  shard_end = OBJECT_MAX_SIZE;
	} else {
	  shard_end = sp->offset;
	}
	dout(30) << __func__ << "  shard 0x" << std::hex << shard_start
		 << " to 0x" << shard_end << std::dec << dendl;
      }
      if (e->blob_escapes_range(shard_start, shard_end - shard_start)) {
	if (!e->blob->is_spanning()) {
	  // We have two options: (1) split the blob into pieces at the
	  // shard boundaries (and adjust extents accordingly), or (2)
	  // mark it spanning.  We prefer to cut the blob if we can.  Note that
	  // we may have to split it multiple times--potentially at every
	  // shard boundary.
	  bool must_span = false;
	  BlobRef b = e->blob;
	  if (b->can_split()) {
	    uint32_t bstart = e->blob_start();
	    uint32_t bend = e->blob_end();
	    for (const auto& sh : shards) {
	      if (bstart < sh.shard_info->offset &&
		  bend > sh.shard_info->offset) {
		uint32_t blob_offset = sh.shard_info->offset - bstart;
		if (b->can_split_at(blob_offset)) {
		  dout(20) << __func__ << "    splitting blob, bstart 0x"
			   << std::hex << bstart << " blob_offset 0x"
			   << blob_offset << std::dec << " " << *b << dendl;
		  b = split_blob(b, blob_offset, sh.shard_info->offset);
		  // switch b to the new right-hand side, in case it
		  // *also* has to get split.
		  bstart += blob_offset;
		  onode->c->store->logger->inc(l_bluestore_blob_split);
		} else {
		  must_span = true;
		  break;
		}
	      }
	    }
	  } else {
	    must_span = true;
	  }
	  if (must_span) {
            auto bid = allocate_spanning_blob_id();
            b->id = bid;
	    spanning_blob_map[b->id] = b;
	    dout(20) << __func__ << "    adding spanning " << *b << dendl;
	  }
	}
      } else {
	if (e->blob->is_spanning()) {
	  spanning_blob_map.erase(e->blob->id);
	  e->blob->id = -1;
	  dout(30) << __func__ << "    un-spanning " << *e->blob << dendl;
	}
      }
    }
  }

  clear_needs_reshard();
}

bool BlueStore::ExtentMap::encode_some(
  uint32_t offset,
  uint32_t length,
  bufferlist& bl,
  unsigned *pn)
{
  auto cct = onode->c->store->cct; //used by dout
  Extent dummy(offset);
  auto start = extent_map.lower_bound(dummy);
  uint32_t end = offset + length;

  __u8 struct_v = 2; // Version 2 differs from v1 in blob's ref_map
                     // serialization only. Hence there is no specific
                     // handling at ExtentMap level.

  unsigned n = 0;
  size_t bound = 0;
  bool must_reshard = false;
  for (auto p = start;
       p != extent_map.end() && p->logical_offset < end;
       ++p, ++n) {
    assert(p->logical_offset >= offset);
    p->blob->last_encoded_id = -1;
    if (!p->blob->is_spanning() && p->blob_escapes_range(offset, length)) {
      dout(30) << __func__ << " 0x" << std::hex << offset << "~" << length
	       << std::dec << " hit new spanning blob " << *p << dendl;
      request_reshard(p->blob_start(), p->blob_end());
      must_reshard = true;
    }
    if (!must_reshard) {
      denc_varint(0, bound); // blobid
      denc_varint(0, bound); // logical_offset
      denc_varint(0, bound); // len
      denc_varint(0, bound); // blob_offset

      p->blob->bound_encode(
        bound,
        struct_v,
        p->blob->shared_blob->get_sbid(),
        false);
    }
  }
  if (must_reshard) {
    return true;
  }

  denc(struct_v, bound);
  denc_varint(0, bound); // number of extents

  {
    auto app = bl.get_contiguous_appender(bound);
    denc(struct_v, app);
    denc_varint(n, app);
    if (pn) {
      *pn = n;
    }

    n = 0;
    uint64_t pos = 0;
    uint64_t prev_len = 0;
    for (auto p = start;
	 p != extent_map.end() && p->logical_offset < end;
	 ++p, ++n) {
      unsigned blobid;
      bool include_blob = false;
      if (p->blob->is_spanning()) {
	blobid = p->blob->id << BLOBID_SHIFT_BITS;
	blobid |= BLOBID_FLAG_SPANNING;
      } else if (p->blob->last_encoded_id < 0) {
	p->blob->last_encoded_id = n + 1;  // so it is always non-zero
	include_blob = true;
	blobid = 0;  // the decoder will infer the id from n
      } else {
	blobid = p->blob->last_encoded_id << BLOBID_SHIFT_BITS;
      }
      if (p->logical_offset == pos) {
	blobid |= BLOBID_FLAG_CONTIGUOUS;
      }
      if (p->blob_offset == 0) {
	blobid |= BLOBID_FLAG_ZEROOFFSET;
      }
      if (p->length == prev_len) {
	blobid |= BLOBID_FLAG_SAMELENGTH;
      } else {
	prev_len = p->length;
      }
      denc_varint(blobid, app);
      if ((blobid & BLOBID_FLAG_CONTIGUOUS) == 0) {
	denc_varint_lowz(p->logical_offset - pos, app);
      }
      if ((blobid & BLOBID_FLAG_ZEROOFFSET) == 0) {
	denc_varint_lowz(p->blob_offset, app);
      }
      if ((blobid & BLOBID_FLAG_SAMELENGTH) == 0) {
	denc_varint_lowz(p->length, app);
      }
      pos = p->logical_end();
      if (include_blob) {
	p->blob->encode(app, struct_v, p->blob->shared_blob->get_sbid(), false);
      }
    }
  }
  /*derr << __func__ << bl << dendl;
  derr << __func__ << ":";
  bl.hexdump(*_dout);
  *_dout << dendl;
  */
  return false;
}

unsigned BlueStore::ExtentMap::decode_some(bufferlist& bl)
{
  auto cct = onode->c->store->cct; //used by dout
  /*
  derr << __func__ << ":";
  bl.hexdump(*_dout);
  *_dout << dendl;
  */

  assert(bl.get_num_buffers() <= 1);
  auto p = bl.front().begin_deep();
  __u8 struct_v;
  denc(struct_v, p);
  // Version 2 differs from v1 in blob's ref_map
  // serialization only. Hence there is no specific
  // handling at ExtentMap level below.
  assert(struct_v == 1 || struct_v == 2);

  uint32_t num;
  denc_varint(num, p);
  vector<BlobRef> blobs(num);
  uint64_t pos = 0;
  uint64_t prev_len = 0;
  unsigned n = 0;

  while (!p.end()) {
    Extent *le = new Extent();
    uint64_t blobid;
    denc_varint(blobid, p);
    if ((blobid & BLOBID_FLAG_CONTIGUOUS) == 0) {
      uint64_t gap;
      denc_varint_lowz(gap, p);
      pos += gap;
    }
    le->logical_offset = pos;
    if ((blobid & BLOBID_FLAG_ZEROOFFSET) == 0) {
      denc_varint_lowz(le->blob_offset, p);
    } else {
      le->blob_offset = 0;
    }
    if ((blobid & BLOBID_FLAG_SAMELENGTH) == 0) {
      denc_varint_lowz(prev_len, p);
    }
    le->length = prev_len;

    if (blobid & BLOBID_FLAG_SPANNING) {
      dout(30) << __func__ << "  getting spanning blob "
	       << (blobid >> BLOBID_SHIFT_BITS) << dendl;
      le->assign_blob(get_spanning_blob(blobid >> BLOBID_SHIFT_BITS));
    } else {
      blobid >>= BLOBID_SHIFT_BITS;
      if (blobid) {
	le->assign_blob(blobs[blobid - 1]);
	assert(le->blob);
      } else {
	Blob *b = new Blob();
        uint64_t sbid = 0;
        b->decode(onode->c, p, struct_v, &sbid, false);
	blobs[n] = b;
	onode->c->open_shared_blob(sbid, b);
	le->assign_blob(b);
      }
      // we build ref_map dynamically for non-spanning blobs
      le->blob->get_ref(
	onode->c,
	le->blob_offset,
	le->length);
    }
    pos += prev_len;
    ++n;
    extent_map.insert(*le);
  }

  assert(n == num);
  return num;
}

void BlueStore::ExtentMap::bound_encode_spanning_blobs(size_t& p)
{
  // Version 2 differs from v1 in blob's ref_map
  // serialization only. Hence there is no specific
  // handling at ExtentMap level.
  __u8 struct_v = 2;

  denc(struct_v, p);
  denc_varint((uint32_t)0, p);
  size_t key_size = 0;
  denc_varint((uint32_t)0, key_size);
  p += spanning_blob_map.size() * key_size;
  for (const auto& i : spanning_blob_map) {
    i.second->bound_encode(p, struct_v, i.second->shared_blob->get_sbid(), true);
  }
}

void BlueStore::ExtentMap::encode_spanning_blobs(
  bufferlist::contiguous_appender& p)
{
  // Version 2 differs from v1 in blob's ref_map
  // serialization only. Hence there is no specific
  // handling at ExtentMap level.
  __u8 struct_v = 2;

  denc(struct_v, p);
  denc_varint(spanning_blob_map.size(), p);
  for (auto& i : spanning_blob_map) {
    denc_varint(i.second->id, p);
    i.second->encode(p, struct_v, i.second->shared_blob->get_sbid(), true);
  }
}

void BlueStore::ExtentMap::decode_spanning_blobs(
  bufferptr::iterator& p)
{
  __u8 struct_v;
  denc(struct_v, p);
  // Version 2 differs from v1 in blob's ref_map
  // serialization only. Hence there is no specific
  // handling at ExtentMap level.
  assert(struct_v == 1 || struct_v == 2);

  unsigned n;
  denc_varint(n, p);
  while (n--) {
    BlobRef b(new Blob());
    denc_varint(b->id, p);
    spanning_blob_map[b->id] = b;
    uint64_t sbid = 0;
    b->decode(onode->c, p, struct_v, &sbid, true);
    onode->c->open_shared_blob(sbid, b);
  }
}

void BlueStore::ExtentMap::init_shards(bool loaded, bool dirty)
{
  shards.resize(onode->onode.extent_map_shards.size());
  unsigned i = 0;
  for (auto &s : onode->onode.extent_map_shards) {
    shards[i].shard_info = &s;
    shards[i].loaded = loaded;
    shards[i].dirty = dirty;
    ++i;
  }
}

void BlueStore::ExtentMap::fault_range(
  KeyValueDB *db,
  uint32_t offset,
  uint32_t length)
{
  auto cct = onode->c->store->cct; //used by dout
  dout(30) << __func__ << " 0x" << std::hex << offset << "~" << length
	   << std::dec << dendl;
  auto start = seek_shard(offset);
  auto last = seek_shard(offset + length);

  if (start < 0)
    return;

  assert(last >= start);
  string key;
  while (start <= last) {
    assert((size_t)start < shards.size());
    auto p = &shards[start];
    if (!p->loaded) {
      dout(30) << __func__ << " opening shard 0x" << std::hex
	       << p->shard_info->offset << std::dec << dendl;
      bufferlist v;
      generate_extent_shard_key_and_apply(
	onode->key, p->shard_info->offset, &key,
        [&](const string& final_key) {
          int r = db->get(PREFIX_OBJ, final_key, &v);
          if (r < 0) {
	    derr << __func__ << " missing shard 0x" << std::hex
		 << p->shard_info->offset << std::dec << " for " << onode->oid
		 << dendl;
	    assert(r >= 0);
          }
        }
      );
      p->extents = decode_some(v);
      p->loaded = true;
      dout(20) << __func__ << " open shard 0x" << std::hex
	       << p->shard_info->offset << std::dec
	       << " (" << v.length() << " bytes)" << dendl;
      assert(p->dirty == false);
      assert(v.length() == p->shard_info->bytes);
      onode->c->store->logger->inc(l_bluestore_onode_shard_misses);
    } else {
      onode->c->store->logger->inc(l_bluestore_onode_shard_hits);
    }
    ++start;
  }
}

void BlueStore::ExtentMap::dirty_range(
  uint32_t offset,
  uint32_t length)
{
  auto cct = onode->c->store->cct; //used by dout
  dout(30) << __func__ << " 0x" << std::hex << offset << "~" << length
	   << std::dec << dendl;
  if (shards.empty()) {
    dout(20) << __func__ << " mark inline shard dirty" << dendl;
    inline_bl.clear();
    return;
  }
  auto start = seek_shard(offset);
  auto last = seek_shard(offset + length);
  if (start < 0)
    return;

  assert(last >= start);
  while (start <= last) {
    assert((size_t)start < shards.size());
    auto p = &shards[start];
    if (!p->loaded) {
      dout(20) << __func__ << " shard 0x" << std::hex << p->shard_info->offset
	       << std::dec << " is not loaded, can't mark dirty" << dendl;
      assert(0 == "can't mark unloaded shard dirty");
    }
    if (!p->dirty) {
      dout(20) << __func__ << " mark shard 0x" << std::hex
	       << p->shard_info->offset << std::dec << " dirty" << dendl;
      p->dirty = true;
    }
    ++start;
  }
}

BlueStore::extent_map_t::iterator BlueStore::ExtentMap::find(
  uint64_t offset)
{
  Extent dummy(offset);
  return extent_map.find(dummy);
}

BlueStore::extent_map_t::iterator BlueStore::ExtentMap::seek_lextent(
  uint64_t offset)
{
  Extent dummy(offset);
  auto fp = extent_map.lower_bound(dummy);
  if (fp != extent_map.begin()) {
    --fp;
    if (fp->logical_end() <= offset) {
      ++fp;
    }
  }
  return fp;
}

BlueStore::extent_map_t::const_iterator BlueStore::ExtentMap::seek_lextent(
  uint64_t offset) const
{
  Extent dummy(offset);
  auto fp = extent_map.lower_bound(dummy);
  if (fp != extent_map.begin()) {
    --fp;
    if (fp->logical_end() <= offset) {
      ++fp;
    }
  }
  return fp;
}

bool BlueStore::ExtentMap::has_any_lextents(uint64_t offset, uint64_t length)
{
  auto fp = seek_lextent(offset);
  if (fp == extent_map.end() || fp->logical_offset >= offset + length) {
    return false;
  }
  return true;
}

int BlueStore::ExtentMap::compress_extent_map(
  uint64_t offset,
  uint64_t length)
{
  auto cct = onode->c->store->cct; //used by dout
  if (extent_map.empty())
    return 0;
  int removed = 0;
  auto p = seek_lextent(offset);
  if (p != extent_map.begin()) {
    --p;  // start to the left of offset
  }
  // the caller should have just written to this region
  assert(p != extent_map.end());

  // identify the *next* shard
  auto pshard = shards.begin();
  while (pshard != shards.end() &&
	 p->logical_offset >= pshard->shard_info->offset) {
    ++pshard;
  }
  uint64_t shard_end;
  if (pshard != shards.end()) {
    shard_end = pshard->shard_info->offset;
  } else {
    shard_end = OBJECT_MAX_SIZE;
  }

  auto n = p;
  for (++n; n != extent_map.end(); p = n++) {
    if (n->logical_offset > offset + length) {
      break;  // stop after end
    }
    while (n != extent_map.end() &&
	   p->logical_end() == n->logical_offset &&
	   p->blob == n->blob &&
	   p->blob_offset + p->length == n->blob_offset &&
	   n->logical_offset < shard_end) {
      dout(20) << __func__ << " 0x" << std::hex << offset << "~" << length
	       << " next shard 0x" << shard_end << std::dec
	       << " merging " << *p << " and " << *n << dendl;
      p->length += n->length;
      rm(n++);
      ++removed;
    }
    if (n == extent_map.end()) {
      break;
    }
    if (n->logical_offset >= shard_end) {
      assert(pshard != shards.end());
      ++pshard;
      if (pshard != shards.end()) {
	shard_end = pshard->shard_info->offset;
      } else {
	shard_end = OBJECT_MAX_SIZE;
      }
    }
  }
  if (removed && onode) {
    onode->c->store->logger->inc(l_bluestore_extent_compress, removed);
  }
  return removed;
}

void BlueStore::ExtentMap::punch_hole(
  CollectionRef &c, 
  uint64_t offset,
  uint64_t length,
  old_extent_map_t *old_extents)
{
  auto p = seek_lextent(offset);
  uint64_t end = offset + length;
  while (p != extent_map.end()) {
    if (p->logical_offset >= end) {
      break;
    }
    if (p->logical_offset < offset) {
      if (p->logical_end() > end) {
	// split and deref middle
	uint64_t front = offset - p->logical_offset;
	OldExtent* oe = OldExtent::create(c, offset, p->blob_offset + front, 
					  length, p->blob);
	old_extents->push_back(*oe);
	add(end,
	    p->blob_offset + front + length,
	    p->length - front - length,
	    p->blob);
	p->length = front;
	break;
      } else {
	// deref tail
	assert(p->logical_end() > offset); // else seek_lextent bug
	uint64_t keep = offset - p->logical_offset;
	OldExtent* oe = OldExtent::create(c, offset, p->blob_offset + keep,
					  p->length - keep, p->blob);
	old_extents->push_back(*oe);
	p->length = keep;
	++p;
	continue;
      }
    }
    if (p->logical_offset + p->length <= end) {
      // deref whole lextent
      OldExtent* oe = OldExtent::create(c, p->logical_offset, p->blob_offset,
				        p->length, p->blob);
      old_extents->push_back(*oe);
      rm(p++);
      continue;
    }
    // deref head
    uint64_t keep = p->logical_end() - end;
    OldExtent* oe = OldExtent::create(c, p->logical_offset, p->blob_offset,
				      p->length - keep, p->blob);
    old_extents->push_back(*oe);

    add(end, p->blob_offset + p->length - keep, keep, p->blob);
    rm(p);
    break;
  }
}

BlueStore::Extent *BlueStore::ExtentMap::set_lextent(
  CollectionRef &c,
  uint64_t logical_offset,
  uint64_t blob_offset, uint64_t length, BlobRef b,
  old_extent_map_t *old_extents)
{
  // We need to have completely initialized Blob to increment its ref counters.
  assert(b->get_blob().get_logical_length() != 0);

  // Do get_ref prior to punch_hole to prevent from putting reused blob into 
  // old_extents list if we overwre the blob totally
  // This might happen during WAL overwrite.
  b->get_ref(onode->c, blob_offset, length);

  if (old_extents) {
    punch_hole(c, logical_offset, length, old_extents);
  }

  Extent *le = new Extent(logical_offset, blob_offset, length, b);
  extent_map.insert(*le);
  if (spans_shard(logical_offset, length)) {
    request_reshard(logical_offset, logical_offset + length);
  }
  return le;
}

BlueStore::BlobRef BlueStore::ExtentMap::split_blob(
  BlobRef lb,
  uint32_t blob_offset,
  uint32_t pos)
{
  auto cct = onode->c->store->cct; //used by dout

  uint32_t end_pos = pos + lb->get_blob().get_logical_length() - blob_offset;
  dout(20) << __func__ << " 0x" << std::hex << pos << " end 0x" << end_pos
	   << " blob_offset 0x" << blob_offset << std::dec << " " << *lb
	   << dendl;
  BlobRef rb = onode->c->new_blob();
  lb->split(onode->c, blob_offset, rb.get());

  for (auto ep = seek_lextent(pos);
       ep != extent_map.end() && ep->logical_offset < end_pos;
       ++ep) {
    if (ep->blob != lb) {
      continue;
    }
    if (ep->logical_offset < pos) {
      // split extent
      size_t left = pos - ep->logical_offset;
      Extent *ne = new Extent(pos, 0, ep->length - left, rb);
      extent_map.insert(*ne);
      ep->length = left;
      dout(30) << __func__ << "  split " << *ep << dendl;
      dout(30) << __func__ << "     to " << *ne << dendl;
    } else {
      // switch blob
      assert(ep->blob_offset >= blob_offset);

      ep->blob = rb;
      ep->blob_offset -= blob_offset;
      dout(30) << __func__ << "  adjusted " << *ep << dendl;
    }
  }
  return rb;
}

// Onode

#undef dout_prefix
#define dout_prefix *_dout << "bluestore.onode(" << this << ")." << __func__ << " "

void BlueStore::Onode::flush()
{
  if (flushing_count.load()) {
    ldout(c->store->cct, 20) << __func__ << " cnt:" << flushing_count << dendl;
    std::unique_lock<std::mutex> l(flush_lock);
    while (flushing_count.load()) {
      flush_cond.wait(l);
    }
  }
  ldout(c->store->cct, 20) << __func__ << " done" << dendl;
}

// =======================================================
// WriteContext
 
/// Checks for writes to the same pextent within a blob
bool BlueStore::WriteContext::has_conflict(
  BlobRef b,
  uint64_t loffs,
  uint64_t loffs_end,
  uint64_t min_alloc_size)
{
  assert((loffs % min_alloc_size) == 0);
  assert((loffs_end % min_alloc_size) == 0);
  for (auto w : writes) {
    if (b == w.b) {
      auto loffs2 = P2ALIGN(w.logical_offset, min_alloc_size);
      auto loffs2_end = P2ROUNDUP(w.logical_offset + w.length0, min_alloc_size);
      if ((loffs <= loffs2 && loffs_end > loffs2) ||
          (loffs >= loffs2 && loffs < loffs2_end)) {
        return true;
      }
    }
  }
  return false;
}
 
// =======================================================

// DeferredBatch
#undef dout_prefix
#define dout_prefix *_dout << "bluestore.DeferredBatch(" << this << ") "

void BlueStore::DeferredBatch::prepare_write(
  CephContext *cct,
  uint64_t seq, uint64_t offset, uint64_t length,
  bufferlist::const_iterator& blp)
{
  _discard(cct, offset, length);
  auto i = iomap.insert(make_pair(offset, deferred_io()));
  assert(i.second);  // this should be a new insertion
  i.first->second.seq = seq;
  blp.copy(length, i.first->second.bl);
  i.first->second.bl.reassign_to_mempool(
    mempool::mempool_bluestore_writing_deferred);
  dout(20) << __func__ << " seq " << seq
	   << " 0x" << std::hex << offset << "~" << length
	   << " crc " << i.first->second.bl.crc32c(-1)
	   << std::dec << dendl;
  seq_bytes[seq] += length;
#ifdef DEBUG_DEFERRED
  _audit(cct);
#endif
}

void BlueStore::DeferredBatch::_discard(
  CephContext *cct, uint64_t offset, uint64_t length)
{
  generic_dout(20) << __func__ << " 0x" << std::hex << offset << "~" << length
		   << std::dec << dendl;
  auto p = iomap.lower_bound(offset);
  if (p != iomap.begin()) {
    --p;
    auto end = p->first + p->second.bl.length();
    if (end > offset) {
      bufferlist head;
      head.substr_of(p->second.bl, 0, offset - p->first);
      dout(20) << __func__ << "  keep head " << p->second.seq
	       << " 0x" << std::hex << p->first << "~" << p->second.bl.length()
	       << " -> 0x" << head.length() << std::dec << dendl;
      auto i = seq_bytes.find(p->second.seq);
      assert(i != seq_bytes.end());
      if (end > offset + length) {
	bufferlist tail;
	tail.substr_of(p->second.bl, offset + length - p->first,
		       end - (offset + length));
	dout(20) << __func__ << "  keep tail " << p->second.seq
		 << " 0x" << std::hex << p->first << "~" << p->second.bl.length()
		 << " -> 0x" << tail.length() << std::dec << dendl;
	auto &n = iomap[offset + length];
	n.bl.swap(tail);
	n.seq = p->second.seq;
	i->second -= length;
      } else {
	i->second -= end - offset;
      }
      assert(i->second >= 0);
      p->second.bl.swap(head);
    }
    ++p;
  }
  while (p != iomap.end()) {
    if (p->first >= offset + length) {
      break;
    }
    auto i = seq_bytes.find(p->second.seq);
    assert(i != seq_bytes.end());
    auto end = p->first + p->second.bl.length();
    if (end > offset + length) {
      unsigned drop_front = offset + length - p->first;
      unsigned keep_tail = end - (offset + length);
      dout(20) << __func__ << "  truncate front " << p->second.seq
	       << " 0x" << std::hex << p->first << "~" << p->second.bl.length()
	       << " drop_front 0x" << drop_front << " keep_tail 0x" << keep_tail
	       << " to 0x" << (offset + length) << "~" << keep_tail
	       << std::dec << dendl;
      auto &s = iomap[offset + length];
      s.seq = p->second.seq;
      s.bl.substr_of(p->second.bl, drop_front, keep_tail);
      i->second -= drop_front;
    } else {
      dout(20) << __func__ << "  drop " << p->second.seq
	       << " 0x" << std::hex << p->first << "~" << p->second.bl.length()
	       << std::dec << dendl;
      i->second -= p->second.bl.length();
    }
    assert(i->second >= 0);
    p = iomap.erase(p);
  }
}

void BlueStore::DeferredBatch::_audit(CephContext *cct)
{
  map<uint64_t,int> sb;
  for (auto p : seq_bytes) {
    sb[p.first] = 0;  // make sure we have the same set of keys
  }
  uint64_t pos = 0;
  for (auto& p : iomap) {
    assert(p.first >= pos);
    sb[p.second.seq] += p.second.bl.length();
    pos = p.first + p.second.bl.length();
  }
  assert(sb == seq_bytes);
}


// Collection

#undef dout_prefix
#define dout_prefix *_dout << "bluestore(" << store->path << ").collection(" << cid << " " << this << ") "

BlueStore::Collection::Collection(BlueStore *ns, Cache *c, coll_t cid)
  : store(ns),
    cache(c),
    cid(cid),
    lock("BlueStore::Collection::lock", true, false),
    exists(true),
    onode_map(c)
{
}

void BlueStore::Collection::open_shared_blob(uint64_t sbid, BlobRef b)
{
  assert(!b->shared_blob);
  const bluestore_blob_t& blob = b->get_blob();
  if (!blob.is_shared()) {
    b->shared_blob = new SharedBlob(this);
    return;
  }

  b->shared_blob = shared_blob_set.lookup(sbid);
  if (b->shared_blob) {
    ldout(store->cct, 10) << __func__ << " sbid 0x" << std::hex << sbid
			  << std::dec << " had " << *b->shared_blob << dendl;
  } else {
    b->shared_blob = new SharedBlob(sbid, this);
    shared_blob_set.add(this, b->shared_blob.get());
    ldout(store->cct, 10) << __func__ << " sbid 0x" << std::hex << sbid
			  << std::dec << " opened " << *b->shared_blob
			  << dendl;
  }
}

void BlueStore::Collection::load_shared_blob(SharedBlobRef sb)
{
  if (!sb->is_loaded()) {

    bufferlist v;
    string key;
    auto sbid = sb->get_sbid();
    get_shared_blob_key(sbid, &key);
    int r = store->db->get(PREFIX_SHARED_BLOB, key, &v);
    if (r < 0) {
	lderr(store->cct) << __func__ << " sbid 0x" << std::hex << sbid
			  << std::dec << " not found at key "
			  << pretty_binary_string(key) << dendl;
      assert(0 == "uh oh, missing shared_blob");
    }

    sb->loaded = true;
    sb->persistent = new bluestore_shared_blob_t(sbid);
    bufferlist::iterator p = v.begin();
    ::decode(*(sb->persistent), p);
    ldout(store->cct, 10) << __func__ << " sbid 0x" << std::hex << sbid
			  << std::dec << " loaded shared_blob " << *sb << dendl;
  }
}

void BlueStore::Collection::make_blob_shared(uint64_t sbid, BlobRef b)
{
  ldout(store->cct, 10) << __func__ << " " << *b << dendl;
  assert(!b->shared_blob->is_loaded());

  // update blob
  bluestore_blob_t& blob = b->dirty_blob();
  blob.set_flag(bluestore_blob_t::FLAG_SHARED);

  // update shared blob
  b->shared_blob->loaded = true;
  b->shared_blob->persistent = new bluestore_shared_blob_t(sbid);
  shared_blob_set.add(this, b->shared_blob.get());
  for (auto p : blob.get_extents()) {
    if (p.is_valid()) {
      b->shared_blob->get_ref(
	p.offset,
	p.length);
    }
  }
  ldout(store->cct, 20) << __func__ << " now " << *b << dendl;
}

uint64_t BlueStore::Collection::make_blob_unshared(SharedBlob *sb)
{
  ldout(store->cct, 10) << __func__ << " " << *sb << dendl;
  assert(sb->is_loaded());

  uint64_t sbid = sb->get_sbid();
  shared_blob_set.remove(sb);
  sb->loaded = false;
  delete sb->persistent;
  sb->sbid_unloaded = 0;
  ldout(store->cct, 20) << __func__ << " now " << *sb << dendl;
  return sbid;
}

BlueStore::OnodeRef BlueStore::Collection::get_onode(
  const ghobject_t& oid,
  bool create)
{
  assert(create ? lock.is_wlocked() : lock.is_locked());

  spg_t pgid;
  if (cid.is_pg(&pgid)) {
    if (!oid.match(cnode.bits, pgid.ps())) {
      lderr(store->cct) << __func__ << " oid " << oid << " not part of "
			<< pgid << " bits " << cnode.bits << dendl;
      ceph_abort();
    }
  }

  OnodeRef o = onode_map.lookup(oid);
  if (o)
    return o;

  mempool::bluestore_cache_other::string key;
  get_object_key(store->cct, oid, &key);

  ldout(store->cct, 20) << __func__ << " oid " << oid << " key "
			<< pretty_binary_string(key) << dendl;

  bufferlist v;
  int r = store->db->get(PREFIX_OBJ, key.c_str(), key.size(), &v);
  ldout(store->cct, 20) << " r " << r << " v.len " << v.length() << dendl;
  Onode *on;
  if (v.length() == 0) {
    assert(r == -ENOENT);
    if (!store->cct->_conf->bluestore_debug_misc &&
	!create)
      return OnodeRef();

    // new object, new onode
    on = new Onode(this, oid, key);
  } else {
    // loaded
    assert(r >= 0);
    on = new Onode(this, oid, key);
    on->exists = true;
    bufferptr::iterator p = v.front().begin_deep();
    on->onode.decode(p);
    for (auto& i : on->onode.attrs) {
      i.second.reassign_to_mempool(mempool::mempool_bluestore_cache_other);
    }

    // initialize extent_map
    on->extent_map.decode_spanning_blobs(p);
    if (on->onode.extent_map_shards.empty()) {
      denc(on->extent_map.inline_bl, p);
      on->extent_map.decode_some(on->extent_map.inline_bl);
      on->extent_map.inline_bl.reassign_to_mempool(
	mempool::mempool_bluestore_cache_other);
    } else {
      on->extent_map.init_shards(false, false);
    }
  }
  o.reset(on);
  return onode_map.add(oid, o);
}

void BlueStore::Collection::split_cache(
  Collection *dest)
{
  ldout(store->cct, 10) << __func__ << " to " << dest << dendl;

  // lock (one or both) cache shards
  std::lock(cache->lock, dest->cache->lock);
  std::lock_guard<std::recursive_mutex> l(cache->lock, std::adopt_lock);
  std::lock_guard<std::recursive_mutex> l2(dest->cache->lock, std::adopt_lock);

  int destbits = dest->cnode.bits;
  spg_t destpg;
  bool is_pg = dest->cid.is_pg(&destpg);
  assert(is_pg);

  auto p = onode_map.onode_map.begin();
  while (p != onode_map.onode_map.end()) {
    if (!p->second->oid.match(destbits, destpg.pgid.ps())) {
      // onode does not belong to this child
      ++p;
    } else {
      OnodeRef o = p->second;
      ldout(store->cct, 20) << __func__ << " moving " << o << " " << o->oid
			    << dendl;

      cache->_rm_onode(p->second);
      p = onode_map.onode_map.erase(p);

      o->c = dest;
      dest->cache->_add_onode(o, 1);
      dest->onode_map.onode_map[o->oid] = o;
      dest->onode_map.cache = dest->cache;

      // move over shared blobs and buffers.  cover shared blobs from
      // both extent map and spanning blob map (the full extent map
      // may not be faulted in)
      vector<SharedBlob*> sbvec;
      for (auto& e : o->extent_map.extent_map) {
	sbvec.push_back(e.blob->shared_blob.get());
      }
      for (auto& b : o->extent_map.spanning_blob_map) {
	sbvec.push_back(b.second->shared_blob.get());
      }
      for (auto sb : sbvec) {
	if (sb->coll == dest) {
	  ldout(store->cct, 20) << __func__ << "  already moved " << *sb
				<< dendl;
	  continue;
	}
	ldout(store->cct, 20) << __func__ << "  moving " << *sb << dendl;
	if (sb->get_sbid()) {
	  ldout(store->cct, 20) << __func__
				<< "   moving registration " << *sb << dendl;
	  shared_blob_set.remove(sb);
	  dest->shared_blob_set.add(dest, sb);
	}
	sb->coll = dest;
	if (dest->cache != cache) {
	  for (auto& i : sb->bc.buffer_map) {
	    if (!i.second->is_writing()) {
	      ldout(store->cct, 20) << __func__ << "   moving " << *i.second
				    << dendl;
	      dest->cache->_move_buffer(cache, i.second.get());
	    }
	  }
	}
      }
    }
  }
}

// =======================================================

void *BlueStore::MempoolThread::entry()
{
  Mutex::Locker l(lock);
  while (!stop) {
    uint64_t meta_bytes =
      mempool::bluestore_cache_other::allocated_bytes() +
      mempool::bluestore_cache_onode::allocated_bytes();
    uint64_t onode_num =
      mempool::bluestore_cache_onode::allocated_items();

    if (onode_num < 2) {
      onode_num = 2;
    }

    float bytes_per_onode = (float)meta_bytes / (float)onode_num;
    size_t num_shards = store->cache_shards.size();
    float target_ratio = store->cache_meta_ratio + store->cache_data_ratio;
    // A little sloppy but should be close enough
    uint64_t shard_target = target_ratio * (store->cache_size / num_shards);

    for (auto i : store->cache_shards) {
      i->trim(shard_target,
	      store->cache_meta_ratio,
	      store->cache_data_ratio,
	      bytes_per_onode);
    }

    store->_update_cache_logger();

    utime_t wait;
    wait += store->cct->_conf->bluestore_cache_trim_interval;
    cond.WaitInterval(lock, wait);
  }
  stop = false;
  return NULL;
}

// =======================================================

// OmapIteratorImpl

#undef dout_prefix
#define dout_prefix *_dout << "bluestore.OmapIteratorImpl(" << this << ") "

BlueStore::OmapIteratorImpl::OmapIteratorImpl(
  CollectionRef c, OnodeRef o, KeyValueDB::Iterator it)
  : c(c), o(o), it(it)
{
  RWLock::RLocker l(c->lock);
  if (o->onode.has_omap()) {
    get_omap_key(o->onode.nid, string(), &head);
    get_omap_tail(o->onode.nid, &tail);
    it->lower_bound(head);
  }
}

int BlueStore::OmapIteratorImpl::seek_to_first()
{
  RWLock::RLocker l(c->lock);
  if (o->onode.has_omap()) {
    it->lower_bound(head);
  } else {
    it = KeyValueDB::Iterator();
  }
  return 0;
}

int BlueStore::OmapIteratorImpl::upper_bound(const string& after)
{
  RWLock::RLocker l(c->lock);
  if (o->onode.has_omap()) {
    string key;
    get_omap_key(o->onode.nid, after, &key);
    ldout(c->store->cct,20) << __func__ << " after " << after << " key "
			    << pretty_binary_string(key) << dendl;
    it->upper_bound(key);
  } else {
    it = KeyValueDB::Iterator();
  }
  return 0;
}

int BlueStore::OmapIteratorImpl::lower_bound(const string& to)
{
  RWLock::RLocker l(c->lock);
  if (o->onode.has_omap()) {
    string key;
    get_omap_key(o->onode.nid, to, &key);
    ldout(c->store->cct,20) << __func__ << " to " << to << " key "
			    << pretty_binary_string(key) << dendl;
    it->lower_bound(key);
  } else {
    it = KeyValueDB::Iterator();
  }
  return 0;
}

bool BlueStore::OmapIteratorImpl::valid()
{
  RWLock::RLocker l(c->lock);
  bool r = o->onode.has_omap() && it && it->valid() &&
    it->raw_key().second <= tail;
  if (it && it->valid()) {
    ldout(c->store->cct,20) << __func__ << " is at "
			    << pretty_binary_string(it->raw_key().second)
			    << dendl;
  }
  return r;
}

int BlueStore::OmapIteratorImpl::next(bool validate)
{
  RWLock::RLocker l(c->lock);
  if (o->onode.has_omap()) {
    it->next();
    return 0;
  } else {
    return -1;
  }
}

string BlueStore::OmapIteratorImpl::key()
{
  RWLock::RLocker l(c->lock);
  assert(it->valid());
  string db_key = it->raw_key().second;
  string user_key;
  decode_omap_key(db_key, &user_key);
  return user_key;
}

bufferlist BlueStore::OmapIteratorImpl::value()
{
  RWLock::RLocker l(c->lock);
  assert(it->valid());
  return it->value();
}


