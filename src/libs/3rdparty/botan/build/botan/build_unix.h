
#ifndef BOTAN_BUILD_CONFIG_H__
#define BOTAN_BUILD_CONFIG_H__

/* This file was automatically generated Tue Jan 12 09:38:57 2010 UTC */

#define BOTAN_VERSION_MAJOR 1
#define BOTAN_VERSION_MINOR 8
#define BOTAN_VERSION_PATCH 8

#ifndef BOTAN_DLL
  #define BOTAN_DLL __attribute__ ((visibility("default")))
#endif

/* Chunk sizes */
#define BOTAN_DEFAULT_BUFFER_SIZE 4096
#define BOTAN_MEM_POOL_CHUNK_SIZE 64*1024

/* BigInt toggles */
#define BOTAN_MP_WORD_BITS 32
#define BOTAN_KARAT_MUL_THRESHOLD 32
#define BOTAN_KARAT_SQR_THRESHOLD 32
#define BOTAN_PRIVATE_KEY_OP_BLINDING_BITS 64

/* PK key consistency checking toggles */
#define BOTAN_PUBLIC_KEY_STRONG_CHECKS_ON_LOAD 1
#define BOTAN_PRIVATE_KEY_STRONG_CHECKS_ON_LOAD 1
#define BOTAN_PRIVATE_KEY_STRONG_CHECKS_ON_GENERATE 1

/* Should we use GCC-style inline assembler? */
#if !defined(BOTAN_USE_GCC_INLINE_ASM) && defined(__GNUG__)
  #define BOTAN_USE_GCC_INLINE_ASM 1
#endif

#ifndef BOTAN_USE_GCC_INLINE_ASM
  #define BOTAN_USE_GCC_INLINE_ASM 0
#endif

/* Target identification and feature test macros */
#define BOTAN_TARGET_OS_IS_LINUX
#define BOTAN_TARGET_OS_HAS_POSIX_MLOCK

#define BOTAN_TARGET_ARCH_IS_IA32
#define BOTAN_TARGET_CPU_IS_LITTLE_ENDIAN
#define BOTAN_TARGET_UNALIGNED_LOADSTOR_OK 1

#define BOTAN_USE_STD_TR1

/* Module definitions */
#define BOTAN_HAS_ADLER32
#define BOTAN_HAS_AES
#define BOTAN_HAS_ALGORITHM_FACTORY
#define BOTAN_HAS_ALLOC_MMAP
#define BOTAN_HAS_ANSI_X919_MAC
#define BOTAN_HAS_ARC4
#define BOTAN_HAS_ASN1
#define BOTAN_HAS_AUTO_SEEDING_RNG
#define BOTAN_HAS_BASE64_CODEC
#define BOTAN_HAS_BIGINT
#define BOTAN_HAS_BIGINT_GFP
#define BOTAN_HAS_BIGINT_MATH
#define BOTAN_HAS_BLOCK_CIPHER
#define BOTAN_HAS_BLOWFISH
#define BOTAN_HAS_CARD_VERIFIABLE_CERTIFICATES
#define BOTAN_HAS_CAST
#define BOTAN_HAS_CBC
#define BOTAN_HAS_CBC_MAC
#define BOTAN_HAS_CFB
#define BOTAN_HAS_CIPHER_MODEBASE
#define BOTAN_HAS_CIPHER_MODE_PADDING
#define BOTAN_HAS_CMAC
#define BOTAN_HAS_CMS
#define BOTAN_HAS_CRC24
#define BOTAN_HAS_CRC32
#define BOTAN_HAS_CRYPTO_BOX
#define BOTAN_HAS_CTR
#define BOTAN_HAS_CTS
#define BOTAN_HAS_DEFAULT_ENGINE
#define BOTAN_HAS_DES
#define BOTAN_HAS_DIFFIE_HELLMAN
#define BOTAN_HAS_DLIES
#define BOTAN_HAS_DL_GROUP
#define BOTAN_HAS_DL_PUBLIC_KEY_FAMILY
#define BOTAN_HAS_DSA
#define BOTAN_HAS_EAX
#define BOTAN_HAS_ECB
#define BOTAN_HAS_ECC_DOMAIN_PARAMATERS
#define BOTAN_HAS_ECC_PUBLIC_KEY_CRYPTO
#define BOTAN_HAS_ECDSA
#define BOTAN_HAS_ECKAEG
#define BOTAN_HAS_ELGAMAL
#define BOTAN_HAS_EME1
#define BOTAN_HAS_EME_PKCS1v15
#define BOTAN_HAS_EMSA1
#define BOTAN_HAS_EMSA1_BSI
#define BOTAN_HAS_EMSA2
#define BOTAN_HAS_EMSA3
#define BOTAN_HAS_EMSA4
#define BOTAN_HAS_EMSA_RAW
#define BOTAN_HAS_ENGINES
#define BOTAN_HAS_ENTROPY_SRC_DEVICE
#define BOTAN_HAS_ENTROPY_SRC_EGD
#define BOTAN_HAS_ENTROPY_SRC_FTW
#define BOTAN_HAS_ENTROPY_SRC_UNIX
#define BOTAN_HAS_FILTERS
#define BOTAN_HAS_FORK_256
#define BOTAN_HAS_GOST_28147_89
#define BOTAN_HAS_GOST_34_11
#define BOTAN_HAS_HASH_ID
#define BOTAN_HAS_HAS_160
#define BOTAN_HAS_HEX_CODEC
#define BOTAN_HAS_HMAC
#define BOTAN_HAS_HMAC_RNG
#define BOTAN_HAS_IDEA
#define BOTAN_HAS_IF_PUBLIC_KEY_FAMILY
#define BOTAN_HAS_KASUMI
#define BOTAN_HAS_KDF1
#define BOTAN_HAS_KDF2
#define BOTAN_HAS_KDF_BASE
#define BOTAN_HAS_KEYPAIR_TESTING
#define BOTAN_HAS_LIBSTATE_MODULE
#define BOTAN_HAS_LION
#define BOTAN_HAS_LUBY_RACKOFF
#define BOTAN_HAS_MARS
#define BOTAN_HAS_MD2
#define BOTAN_HAS_MD4
#define BOTAN_HAS_MD5
#define BOTAN_HAS_MDX_HASH_FUNCTION
#define BOTAN_HAS_MGF1
#define BOTAN_HAS_MISTY1
#define BOTAN_HAS_MUTEX_NOOP
#define BOTAN_HAS_MUTEX_PTHREAD
#define BOTAN_HAS_MUTEX_WRAPPERS
#define BOTAN_HAS_NOEKEON
#define BOTAN_HAS_NYBERG_RUEPPEL
#define BOTAN_HAS_OFB
#define BOTAN_HAS_OID_LOOKUP
#define BOTAN_HAS_OPENPGP_CODEC
#define BOTAN_HAS_PARALLEL_HASH
#define BOTAN_HAS_PASSWORD_BASED_ENCRYPTION
#define BOTAN_HAS_PBE_PKCS_V15
#define BOTAN_HAS_PBE_PKCS_V20
#define BOTAN_HAS_PBKDF1
#define BOTAN_HAS_PBKDF2
#define BOTAN_HAS_PEM_CODEC
#define BOTAN_HAS_PGPS2K
#define BOTAN_HAS_PIPE_UNIXFD_IO
#define BOTAN_HAS_PK_PADDING
#define BOTAN_HAS_PUBLIC_KEY_CRYPTO
#define BOTAN_HAS_RANDPOOL
#define BOTAN_HAS_RC2
#define BOTAN_HAS_RC5
#define BOTAN_HAS_RC6
#define BOTAN_HAS_RIPEMD_128
#define BOTAN_HAS_RIPEMD_160
#define BOTAN_HAS_RSA
#define BOTAN_HAS_RUNTIME_BENCHMARKING
#define BOTAN_HAS_RW
#define BOTAN_HAS_SAFER
#define BOTAN_HAS_SALSA20
#define BOTAN_HAS_SEED
#define BOTAN_HAS_SELFTESTS
#define BOTAN_HAS_SERPENT
#define BOTAN_HAS_SHA1
#define BOTAN_HAS_SHA2
#define BOTAN_HAS_SKEIN_512
#define BOTAN_HAS_SKIPJACK
#define BOTAN_HAS_SQUARE
#define BOTAN_HAS_SSL3_MAC
#define BOTAN_HAS_SSL_V3_PRF
#define BOTAN_HAS_STREAM_CIPHER
#define BOTAN_HAS_TEA
#define BOTAN_HAS_TIGER
#define BOTAN_HAS_TIMER
#define BOTAN_HAS_TIMER_POSIX
#define BOTAN_HAS_TIMER_UNIX
#define BOTAN_HAS_TLS_V10_PRF
#define BOTAN_HAS_TURING
#define BOTAN_HAS_TWOFISH
#define BOTAN_HAS_UTIL_FUNCTIONS
#define BOTAN_HAS_WHIRLPOOL
#define BOTAN_HAS_WID_WAKE
#define BOTAN_HAS_X509
#define BOTAN_HAS_X931_RNG
#define BOTAN_HAS_X942_PRF
#define BOTAN_HAS_XTEA
#define BOTAN_HAS_XTS

/* Local configuration options */


/*
christian@christian-linux ran 'configure.py --os=linux --cpu=ia32 --cc=gcc --disable-asm'

Target
-------
Compiler: g++ -O2 -finline-functions 
Arch: ia32/ia32
OS: linux

Modules
-------
adler32 (Adler32)
aes (AES)
algo_factory (Algorithm Factory)
alloc (Allocator)
alloc_mmap (Disk Based Allocation System)
arc4 (ARC4)
asn1 (ASN.1/BER/DER module)
auto_rng (Auto-seeded Random Number Generator)
base64 (Base64 Codec)
benchmark (Benchmarking)
bigint (BigInt)
block (Block Ciphers)
blowfish (Blowfish)
buf_comp (Buffered Computation)
cast (CAST)
cbc (CBC block cipher mode)
cbc_mac (CBC-MAC)
cfb (CFB block cipher mode)
cmac (CMAC)
cms (CMS)
crc24 (CRC-24)
crc32 (CRC-32)
cryptobox (Crypto Box)
ctr (CTR block cipher mode)
cts (CTS block cipher mode)
cvc (Card Verifiable Certificates)
datastor (Datastore)
def_engine (Default Engine)
des (DES)
dev_random (RNG Device Reader)
dh (Diffie-Hellman Key Agreement)
dl_algo (Discrete Logarithm PK Algorithms)
dl_group (DL Group)
dlies (DLIES)
dsa (DSA)
eax (EAX block cipher mode)
ec_dompar (ECC Domain Parameters)
ecb (ECB block cipher mode)
ecc_key (ECC Public Key)
ecdsa (ECDSA)
eckaeg (ECKAEG)
egd (EGD Entropy Source)
elgamal (ElGamal)
eme1 (EME1)
eme_pkcs (PKCSv1 v1.5 EME)
emsa1 (EMSA1)
emsa1_bsi (EMSA1 (BSI variant))
emsa2 (EMSA2)
emsa3 (EMSA3)
emsa4 (EMSA4)
emsa_raw (EMSA-Raw)
engine (Engines)
entropy (Entropy Sources)
fd_unix (Unix I/O support for Pipe)
filters (Pipe/Filter)
fork256 (FORK-256)
gettimeofday (Unix Timer)
gfpmath (GF(p) Math)
gost_28147 (GOST 28147-89)
gost_3411 (GOST 34.11)
has160 (HAS-160)
hash (Hash Functions)
hash_id (Hash Function Identifiers)
hex (Hex Codec)
hmac (HMAC)
hmac_rng (HMAC RNG)
idea (IDEA)
if_algo (Integer Factorization Algorithms)
kasumi (Kasumi)
kdf (KDF Base Class)
kdf1 (KDF1)
kdf2 (KDF2)
keypair (Keypair Testing)
libstate (Botan Libstate Module)
lion (Lion)
lubyrack (Luby-Rackoff)
mac (Message Authentication Codes)
mars (MARS)
md2 (MD2)
md4 (MD4)
md5 (MD5)
mdx_hash (MDx Hash Base)
mem_pool (Memory Pool Allocator)
mgf1 (MGF1)
misty1 (MISTY-1)
mode_pad (Cipher Mode Padding Method)
modes (Cipher Mode Base Class)
monty_generic (Montgomery Reduction)
mp_generic (MPI Core (C++))
mulop_generic (BigInt Multiply-Add)
mutex (Mutex Wrappers)
noekeon (Noekeon)
noop_mutex (No-Op Mutex)
nr (Nyberg-Rueppel)
numbertheory (Math Functions)
ofb (OFB block cipher mode)
oid_lookup (OID Lookup)
openpgp (OpenPGP Codec)
par_hash (Parallel Hash)
pbe (PBE Base)
pbes1 (PKCS5 v1.5 PBE)
pbes2 (PKCS5 v2.0 PBE)
pbkdf1 (Pbkdf1)
pbkdf2 (Pbkdf2)
pem (PEM Codec)
pgps2k (Pgps2k)
pk_codecs (PK codecs (PKCS8, X.509))
pk_pad (Public Key EME/EMSA Padding Modes)
posix_rt (POSIX Timer)
proc_walk (File Tree Walking Entropy Source)
pthreads (Pthread Mutex)
pubkey (Public Key Base)
randpool (Randpool RNG)
rc2 (RC2)
rc5 (RC5)
rc6 (RC6)
rmd128 (RIPEMD-128)
rmd160 (RIPEMD-160)
rng (Random Number Generators)
rsa (RSA)
rw (Rabin-Williams)
s2k (String to Key Functions)
safer (SAFER)
salsa20 (Salsa20)
seed (SEED)
selftest (Selftests)
serpent (Serpent)
sha1 (SHA-1)
sha2 (SHA-2 (224, 256, 384, 512))
skein (Skein)
skipjack (Skipjack)
square (Square)
ssl3mac (SSLv3 MAC)
ssl_prf (SSLv3 PRF)
stream (Stream Ciphers)
sym_algo (Symmetric Algorithms)
system_alloc (Default (Malloc) Allocators)
tea (TEA)
tiger (Tiger)
timer (Timer Base Class)
tls_prf (TLS v1.0 PRF)
turing (Turing)
twofish (Twofish)
unix_procs (Generic Unix Entropy Source)
utils (Utility Functions)
whirlpool (Whirlpool)
wid_wake (WiderWake)
x509 (X.509)
x919_mac (ANSI X9.19 MAC)
x931_rng (ANSI X9.31 PRNG)
x942_prf (X942 PRF)
xtea (XTEA)
xts (XTS block cipher mode)
*/

#endif
