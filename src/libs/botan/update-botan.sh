#!/bin/sh

test $# -eq 1 || { echo "This script needs a Botan archive as its sole argument."; exit 1; }

set -e

script_dir=$(readlink -f $(dirname $0))
botan_archive=$1
botan_extracted_dir_name=$(basename -s .tgz $botan_archive)
botan_parent_dir=$script_dir/../3rdparty

cd $botan_parent_dir

echo "Removing old botan sources..."
rm -rf botan

echo "Extracting new botan sources..."
tar xf $botan_archive
mv $botan_extracted_dir_name botan

echo "Removing unneeded components..."
cd botan
rm -r doc news.rst
cd src
rm -r build-data/policy/* cli configs fuzzer python scripts tests
cd lib
rm -r codec/base32 compression ffi misc passhash pbkdf/pbkdf1 pbkdf/pgp_s2k pbkdf/scrypt prov \
      psk_db tls
cd block
rm -r aria blowfish camellia cascade cast128 cast256 gost_28147 idea kasumi lion misty1 noekeon seed serpent\
       shacal2 sm4 threefish_512 twofish xtea
cd ../filters
rm -r codec_filt fd_unix
cd ../hash
rm -r blake2 checksum comb4p gost_3411 keccak md4 md5 par_hash rmd160 sha3 shake skein sm3 \
       streebog tiger whirlpool
cd ../kdf
rm -r hkdf kdf1 kdf1_iso18033 kdf2 prf_tls prf_x942 sp800_108 sp800_56a sp800_56c
cd ../mac
rm -r cbc_mac cmac gmac poly1305 siphash x919_mac
cd ../modes
rm -r aead cfb xts
cd ../pk_pad
rm -r eme_oaep eme_pkcs1 eme_raw emsa_raw emsa_x931 iso9796
cd ../pubkey
rm -r cecpq1 curve25519 dlies ecgdsa ecies eckcdsa ed25519 elgamal gost_3410 mce mceies newhope \
      rfc6979 sm2 xmss
cd ../rng
rm -r chacha_rng
cd ../stream
rm -r chacha ofb rc4 salsa20 shake_cipher
cd ../utils
rm -r boost http_util locking_allocator mem_pool poly_dbl socket sqlite3 thread_utils uuid
cd ../x509
rm -r certstor_sql certstor_sqlite3

echo "Patching..."
# Fix annoying linker warning on macOS
sed -i 's/all!haiku -> "-pthread"/all!haiku,darwin -> "-pthread"/g' \
    "$botan_parent_dir/botan/src/build-data/cc/clang.txt"

echo "Done."
