###################################################################
# flag
###################################################################

MOD_SRC  = 
MOD_OBJ  = $(PRELOADER_OUT)/obj

###################################################################
# source files
# - add your file here !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
###################################################################

kivvissl_src := \
./wolfcrypt/curve25519.c \
./wolfcrypt/chacha20_poly1305.c\
./wolfcrypt/ripemd.c\
./wolfcrypt/hc128.c\
./wolfcrypt/ecc_fp.c\
./wolfcrypt/ge_low_mem.c\
./wolfcrypt/memory.c\
./wolfcrypt/ed25519.c\
./wolfcrypt/asm.c\
./wolfcrypt/camellia.c\
./wolfcrypt/pkcs7.c\
./wolfcrypt/srp.c\
./wolfcrypt/integer.c\
./wolfcrypt/ecc.c\
./wolfcrypt/random.c\
./wolfcrypt/blake2b.c\
./wolfcrypt/misc.c\
./wolfcrypt/wc_encrypt.c\
./wolfcrypt/chacha.c\
./wolfcrypt/md2.c\
./wolfcrypt/sha256.c\
./wolfcrypt/ge_operations.c\
./wolfcrypt/fe_low_mem.c\
./wolfcrypt/hmac.c\
./wolfcrypt/poly1305.c\
./wolfcrypt/sha.c\
./wolfcrypt/rsa.c\
./wolfcrypt/des3.c\
./wolfcrypt/signature.c\
./wolfcrypt/sha512.c\
./wolfcrypt/tfm.c\
./wolfcrypt/hash.c\
./wolfcrypt/aes.c\
./wolfcrypt/rabbit.c\
./wolfcrypt/pwdbased.c\
./wolfcrypt/error.c\
./wolfcrypt/arc4.c\
./wolfcrypt/md5.c\
./wolfcrypt/wc_port.c\
./wolfcrypt/coding.c\
./wolfcrypt/compress.c\
./wolfcrypt/idea.c\
./wolfcrypt/dsa.c\
./wolfcrypt/dh.c\
./wolfcrypt/logging.c\
./wolfcrypt/md4.c\
./wolfcrypt/fe_operations.c\
./wolfcrypt/asn.c\
./wolfsrc/keys.c\
./wolfsrc/crl.c\
./wolfsrc/internal.c\
./wolfsrc/sniffer.c\
./wolfsrc/ocsp.c\
./wolfsrc/io.c\
./wolfsrc/tls.c\
./wolfsrc/ssl.c\
./cynovo/util.c\
./cynovo/wolf_cert_util.c \
./cynovo/wolf_rsa_util.c \
./cynovo/der.c \
./cynovo/der_cert.c \
./cynovo/der_rsa.c \
./cynovo/malloc.c \
./cynovo/heap.c

MOD_SRC  := \
	$(kivvissl_src) \
	./kivvissl.c

###################################################################
# target
###################################################################

include $(COMMON_FILE_MK)
