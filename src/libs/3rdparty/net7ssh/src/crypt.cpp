/***************************************************************************
 *   Copyright (C) 2005-2007 by NetSieben Technologies INC                 *
 *   Author: Andrew Useckas                                                *
 *   Email: andrew@netsieben.com                                           *
 *                                                                         *
 *   Windows Port and bugfixes: Keef Aragon <keef@netsieben.com>           *
 *                                                                         *
 *   This program may be distributed under the terms of the Q Public       *
 *   License as defined by Trolltech AS of Norway and appearing in the     *
 *   file LICENSE.QPL included in the packaging of this file.              *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 ***************************************************************************/

#if !defined WIN32 && !defined(__MINGW32__)
#   include <arpa/inet.h>
#endif

#include <botan/libstate.h>
#include "crypt.h"
#include "ne7ssh_session.h"
#include "ne7ssh.h"

using namespace Botan;

ne7ssh_crypt::ne7ssh_crypt(ne7ssh_session* _session) : session(_session), kexMethod(DH_GROUP1_SHA1), c2sCryptoMethod(AES128_CBC), s2cCryptoMethod(AES128_CBC), c2sMacMethod(HMAC_MD5), s2cMacMethod(HMAC_MD5), inited(false), encrypt(0), decrypt(0), compress(0), decompress(0), hmacOut(0), hmacIn(0), privKexKey(0), encryptBlock(0), decryptBlock(0)
{
}


ne7ssh_crypt::~ne7ssh_crypt()
{
  if (encrypt) delete encrypt;
  if (decrypt) delete decrypt;

  if (compress) delete compress;
  if (decompress) delete decompress;

  if (hmacOut) delete hmacOut;
  if (hmacIn) delete hmacIn;

  if (privKexKey) delete privKexKey;
}


bool ne7ssh_crypt::agree (Botan::SecureVector<Botan::byte> &result, const char* local, Botan::SecureVector<Botan::byte> &remote)
{
  ne7ssh_string localAlgos (local, 0);
  ne7ssh_string remoteAlgos (remote, 0);
  char* localAlgo, *remoteAlgo;
  bool match;
  size_t len;

  localAlgos.split (',');
  localAlgos.resetParts();
  remoteAlgos.split (',');
  remoteAlgos.resetParts();

  match = false;
  while ((localAlgo = localAlgos.nextPart()))
  {
    len = strlen(localAlgo);
    while ((remoteAlgo = remoteAlgos.nextPart()))
    {
      if (!memcmp (localAlgo, remoteAlgo, len))
      {
        match = true;
        break;
      }
    }
    if (match) break;
    remoteAlgos.resetParts();
  }
  if (match)
  {
    result.set ((Botan::byte*)localAlgo, (uint32_t) len);
    return true;
  }
  else
  {
    result.clear();
    return false;
  }
}

bool ne7ssh_crypt::negotiatedKex (Botan::SecureVector<Botan::byte> &kexAlgo)
{
  if (!memcmp (kexAlgo.begin(), "diffie-hellman-group1-sha1", kexAlgo.size()))
  {
    kexMethod = DH_GROUP1_SHA1;
    return true;
  }
  else if (!memcmp (kexAlgo.begin(), "diffie-hellman-group14-sha1", kexAlgo.size()))
  {
    kexMethod = DH_GROUP14_SHA1;
    return true;
  }

  ne7ssh::errors()->push (session->getSshChannel(), "KEX algorithm: '%B' not defined.", &kexAlgo);
  return false;
}

bool ne7ssh_crypt::negotiatedHostkey (Botan::SecureVector<Botan::byte> &hostkeyAlgo)
{
  if (!memcmp (hostkeyAlgo.begin(), "ssh-dss", hostkeyAlgo.size()))
  {
    hostkeyMethod = SSH_DSS;
    return true;
  }
  else if (!memcmp (hostkeyAlgo.begin(), "ssh-rsa", hostkeyAlgo.size()))
  {
    hostkeyMethod = SSH_RSA;
    return true;
  }

  ne7ssh::errors()->push (session->getSshChannel(), "Hostkey algorithm: '%B' not defined.", &hostkeyAlgo);
  return false;
}

bool ne7ssh_crypt::negotiatedCryptoC2s (Botan::SecureVector<Botan::byte> &cryptoAlgo)
{
  if (!memcmp (cryptoAlgo.begin(), "3des-cbc", cryptoAlgo.size()))
  {
    c2sCryptoMethod = TDES_CBC;
    return true;
  }
  else if (!memcmp (cryptoAlgo.begin(), "aes128-cbc", cryptoAlgo.size()))
  {
    c2sCryptoMethod = AES128_CBC;
    return true;
  }
  else if (!memcmp (cryptoAlgo.begin(), "aes192-cbc", cryptoAlgo.size()))
  {
    c2sCryptoMethod = AES192_CBC;
    return true;
  }
  else if (!memcmp (cryptoAlgo.begin(), "aes256-cbc", cryptoAlgo.size()))
  {
    c2sCryptoMethod = AES256_CBC;
    return true;
  }
  else if (!memcmp (cryptoAlgo.begin(), "blowfish-cbc", cryptoAlgo.size()))
  {
    c2sCryptoMethod = BLOWFISH_CBC;
    return true;
  }
  else if (!memcmp (cryptoAlgo.begin(), "cast128-cbc", cryptoAlgo.size()))
  {
    c2sCryptoMethod = CAST128_CBC;
    return true;
  }
  else if (!memcmp (cryptoAlgo.begin(), "twofish-cbc", cryptoAlgo.size()) || !memcmp (cryptoAlgo.begin(), "twofish256-cbc", cryptoAlgo.size()))
  {
    c2sCryptoMethod = TWOFISH_CBC;
    return true;
  }

  ne7ssh::errors()->push (session->getSshChannel(), "Cryptographic algorithm: '%B' not defined.", &cryptoAlgo);
  return false;
}

bool ne7ssh_crypt::negotiatedCryptoS2c (Botan::SecureVector<Botan::byte> &cryptoAlgo)
{
  if (!memcmp (cryptoAlgo.begin(), "3des-cbc", cryptoAlgo.size()))
  {
    s2cCryptoMethod = TDES_CBC;
    return true;
  }
  else if (!memcmp (cryptoAlgo.begin(), "aes128-cbc", cryptoAlgo.size()))
  {
    s2cCryptoMethod = AES128_CBC;
    return true;
  }
  else if (!memcmp (cryptoAlgo.begin(), "aes192-cbc", cryptoAlgo.size()))
  {
    s2cCryptoMethod = AES192_CBC;
    return true;
  }
  else if (!memcmp (cryptoAlgo.begin(), "aes256-cbc", cryptoAlgo.size()))
  {
    s2cCryptoMethod = AES256_CBC;
    return true;
  }
  else if (!memcmp (cryptoAlgo.begin(), "blowfish-cbc", cryptoAlgo.size()))
  {
    s2cCryptoMethod = BLOWFISH_CBC;
    return true;
  }
  else if (!memcmp (cryptoAlgo.begin(), "cast128-cbc", cryptoAlgo.size()))
  {
    s2cCryptoMethod = CAST128_CBC;
    return true;
  }
  else if (!memcmp (cryptoAlgo.begin(), "twofish-cbc", cryptoAlgo.size()) || !memcmp (cryptoAlgo.begin(), "twofish256-cbc", cryptoAlgo.size()))
  {
    s2cCryptoMethod = TWOFISH_CBC;
    return true;
  }

  ne7ssh::errors()->push (session->getSshChannel(), "Cryptographic method: '%B' not defined.", &cryptoAlgo);
  return false;
}

bool ne7ssh_crypt::negotiatedMacC2s (Botan::SecureVector<Botan::byte> &macAlgo)
{
  if (!memcmp (macAlgo.begin(), "hmac-sha1", macAlgo.size()))
  {
    c2sMacMethod = HMAC_SHA1;
    return true;
  }
  else if (!memcmp (macAlgo.begin(), "hmac-md5", macAlgo.size()))
  {
    c2sMacMethod = HMAC_MD5;
    return true;
  }
  else if (!memcmp (macAlgo.begin(), "none", macAlgo.size()))
  {
    c2sMacMethod = HMAC_NONE;
    return true;
  }

  ne7ssh::errors()->push (session->getSshChannel(), "HMAC algorithm: '%B' not defined.", &macAlgo);
  return false;
}

bool ne7ssh_crypt::negotiatedMacS2c (Botan::SecureVector<Botan::byte> &macAlgo)
{
  if (!memcmp (macAlgo.begin(), "hmac-sha1", macAlgo.size()))
  {
    s2cMacMethod = HMAC_SHA1;
    return true;
  }
  else if (!memcmp (macAlgo.begin(), "hmac-md5", macAlgo.size()))
  {
    s2cMacMethod = HMAC_MD5;
    return true;
  }
  else if (!memcmp (macAlgo.begin(), "none", macAlgo.size()))
  {
    s2cMacMethod = HMAC_NONE;
    return true;
  }

  ne7ssh::errors()->push (session->getSshChannel(), "HMAC algorithm: '%B' not defined.", &macAlgo);
  return false;
}

bool ne7ssh_crypt::negotiatedCmprsC2s (Botan::SecureVector<Botan::byte> &cmprsAlgo)
{
  if (!memcmp (cmprsAlgo.begin(), "none", cmprsAlgo.size()))
  {
    c2sCmprsMethod = NONE;
    return true;
  }
  else if (!memcmp (cmprsAlgo.begin(), "zlib", cmprsAlgo.size()))
  {
    c2sCmprsMethod = ZLIB;
    return true;
  }

  ne7ssh::errors()->push (session->getSshChannel(), "Compression algorithm: '%B' not defined.", &cmprsAlgo);
  return false;
}

bool ne7ssh_crypt::negotiatedCmprsS2c (Botan::SecureVector<Botan::byte> &cmprsAlgo)
{
  if (!memcmp (cmprsAlgo.begin(), "none", cmprsAlgo.size()))
  {
    s2cCmprsMethod = NONE;
    return true;
  }
  else if (!memcmp (cmprsAlgo.begin(), "zlib", cmprsAlgo.size()))
  {
    s2cCmprsMethod = ZLIB;
    return true;
  }

  ne7ssh::errors()->push (session->getSshChannel(), "Compression algorithm: '%B' not defined.", &cmprsAlgo);
  return false;
}

bool ne7ssh_crypt::getKexPublic (Botan::BigInt &publicKey)
{
  switch (kexMethod)
  {
    case DH_GROUP1_SHA1:
      return getDHGroup1Sha1Public (publicKey);

    case DH_GROUP14_SHA1:
      return getDHGroup14Sha1Public (publicKey);

    default:
      ne7ssh::errors()->push (session->getSshChannel(), "Undefined DH Group: '%s'.", kexMethod);
      return false;
  }
}

bool ne7ssh_crypt::computeH (Botan::SecureVector<Botan::byte> &result, Botan::SecureVector<Botan::byte> &val)
{
  HashFunction* hashIt;

  switch (kexMethod)
  {
    case DH_GROUP1_SHA1:
    case DH_GROUP14_SHA1:
      hashIt = get_hash ("SHA-1");
      break;

    default:
      ne7ssh::errors()->push (session->getSshChannel(), "Undefined DH Group: '%s' while computing H.", kexMethod);
      return false;
  }

  if (!hashIt) return false;
  H = hashIt->process (val);
  result.set (H);
  delete (hashIt);
  return true;
}

bool ne7ssh_crypt::verifySig (Botan::SecureVector<Botan::byte> &hostKey, Botan::SecureVector<Botan::byte> &sig)
{
  DSA_PublicKey *dsaKey = 0;
	RSA_PublicKey *rsaKey = 0;
  PK_Verifier *verifier;
  ne7ssh_string signature (sig, 0);
  SecureVector<Botan::byte> sigType, sigData;
  bool result;

  if (H.is_empty())
  {
    ne7ssh::errors()->push (session->getSshChannel(), "H was not initialzed.");
    return false;
  }

  if (!signature.getString (sigType))
  {
    ne7ssh::errors()->push (session->getSshChannel(), "Signature without type.");
    return false;
  }
  if (!signature.getString (sigData))
  {
    ne7ssh::errors()->push (session->getSshChannel(), "Signature without data.");
    return false;
  }

  switch (hostkeyMethod)
  {
    case SSH_DSS:
      dsaKey = getDSAKey (hostKey);
      if (!dsaKey)
      {
        ne7ssh::errors()->push (session->getSshChannel(), "DSA key not generated.");
        return false;
      }
      break;

    case SSH_RSA:
      rsaKey = getRSAKey (hostKey);
      if (!rsaKey)
      {
	ne7ssh::errors()->push (session->getSshChannel(), "RSA key not generated.");
        return false;
      }
      break;

    default:
      ne7ssh::errors()->push (session->getSshChannel(), "Hostkey algorithm: %i not supported.", hostkeyMethod);
      return false;
  }

  switch (kexMethod)
  {
    case DH_GROUP1_SHA1:
    case DH_GROUP14_SHA1:
      if (dsaKey) verifier = get_pk_verifier (*dsaKey, "EMSA1(SHA-1)");
      else if (rsaKey) verifier = get_pk_verifier (*rsaKey, "EMSA3(SHA-1)");
      break;

    default:
      ne7ssh::errors()->push (session->getSshChannel(), "Key Exchange algorithm: %i not supported.", kexMethod);
      return false;
  }

  result = verifier->verify_message (H, sigData);
  delete verifier;
  if (dsaKey) delete dsaKey;
  if (rsaKey) delete dsaKey;

  if (!result)
  {
    ne7ssh::errors()->push (session->getSshChannel(), "Failure to verify host signature.");
    return false;
  }
  else return true;
}

DSA_PublicKey* ne7ssh_crypt::getDSAKey (Botan::SecureVector<Botan::byte> &hostKey)
{
  ne7ssh_string hKey;
  SecureVector<Botan::byte> field;
  BigInt p, q, g, y;
  DSA_PublicKey *pubKey;

  hKey.addVector (hostKey);

  if (!hKey.getString (field)) return 0;
  if (!negotiatedHostkey (field)) return 0;

  if (!hKey.getBigInt (p)) return 0;
  if (!hKey.getBigInt (q)) return 0;
  if (!hKey.getBigInt (g)) return 0;
  if (!hKey.getBigInt (y)) return 0;

  DL_Group keyDL (p, q, g);
  pubKey  = new DSA_PublicKey (keyDL, y);
  return pubKey;
}

RSA_PublicKey* ne7ssh_crypt::getRSAKey (Botan::SecureVector<Botan::byte> &hostKey)
{
  ne7ssh_string hKey;
  SecureVector<Botan::byte> field;
  BigInt e, n;

  hKey.addVector (hostKey);

  if (!hKey.getString (field)) return 0;
  if (!negotiatedHostkey (field)) return 0;

  if (!hKey.getBigInt (e)) return 0;
  if (!hKey.getBigInt (n)) return 0;

  return (new RSA_PublicKey (n, e));
}



bool ne7ssh_crypt::makeKexSecret (Botan::SecureVector<Botan::byte> &result, Botan::BigInt &f)
{
  SymmetricKey negotiated = privKexKey->derive_key (f);
  if (!negotiated.length()) return false;

  BigInt Kint (negotiated.begin(), negotiated.length());
  ne7ssh_string::bn2vector(result, Kint);
  K.set (result);
  delete privKexKey;
  privKexKey = 0;
  return true;
}

bool ne7ssh_crypt::getDHGroup1Sha1Public (Botan::BigInt &publicKey)
{
#if BOTAN_PRE_15
  privKexKey = new DH_PrivateKey (get_dl_group("IETF-1024"));
#elif BOTAN_PRE_18
  privKexKey = new DH_PrivateKey (DL_Group("modp/ietf/1024"));
#else
  privKexKey = new DH_PrivateKey (*ne7ssh::rng, DL_Group("modp/ietf/1024"));
#endif
  DH_PublicKey pubKexKey = *privKexKey;

  publicKey = pubKexKey.get_y();
  if (publicKey.is_zero()) return false;
  else return true;
}

bool ne7ssh_crypt::getDHGroup14Sha1Public (Botan::BigInt &publicKey)
{
#if BOTAN_PRE_15
  privKexKey = new DH_PrivateKey (get_dl_group("IETF-2048"));
#elif BOTAN_PRE_18
  privKexKey = new DH_PrivateKey (DL_Group("modp/ietf/2048"));
#else
  privKexKey = new DH_PrivateKey (*ne7ssh::rng, DL_Group("modp/ietf/2048"));
#endif
  DH_PublicKey pubKexKey = *privKexKey;

  publicKey = pubKexKey.get_y();
  if (publicKey.is_zero()) return false;
  else return true;
}

const char* ne7ssh_crypt::getHashAlgo ()
{
  switch (kexMethod)
  {
    case DH_GROUP1_SHA1:
    case DH_GROUP14_SHA1:
      return "SHA-1";

    default:
      ne7ssh::errors()->push (session->getSshChannel(), "DH Group: %i was not defined.", kexMethod);
      return 0;
  }
}

const char* ne7ssh_crypt::getCryptAlgo (uint32 crypto)
{
  switch (crypto)
  {
    case TDES_CBC:
      return "TripleDES";

    case AES128_CBC:
      return "AES-128";

    case AES192_CBC:
      return "AES-192";

    case AES256_CBC:
      return "AES-256";

    case BLOWFISH_CBC:
      return "Blowfish";

    case CAST128_CBC:
      return "CAST-128";

    case TWOFISH_CBC:
      return "Twofish";

    default:
      ne7ssh::errors()->push (session->getSshChannel(), "Cryptographic algorithm: %i was not defined.", crypto);
      return 0;
  }
}

const char* ne7ssh_crypt::getHmacAlgo (uint32 method)
{
  switch (method)
  {
    case HMAC_SHA1:
      return "SHA-1";

    case HMAC_MD5:
      return "MD5";

    case HMAC_NONE:
      return 0;

    default:
      ne7ssh::errors()->push (session->getSshChannel(), "HMAC algorithm: %i was not defined.", method);
      return 0;
  }
}

uint32 ne7ssh_crypt::getMacKeyLen (uint32 method)
{
  switch (method)
  {
    case HMAC_SHA1:
      return 20;

    case HMAC_MD5:
      return 16;

    case HMAC_NONE:
      return 0;

    default:
      ne7ssh::errors()->push (session->getSshChannel(), "HMAC algorithm: %i was not defined.", method);
      return 0;
  }
}

uint32 ne7ssh_crypt::getMacDigestLen (uint32 method)
{
  switch (method)
  {
    case HMAC_SHA1:
      return 20;

    case HMAC_MD5:
      return 16;

    case HMAC_NONE:
      return 0;

    default:
      return 0;
  }
}

bool ne7ssh_crypt::makeNewKeys ()
{
  const char* algo;
  uint32 key_len, iv_len, macLen;
  SecureVector<Botan::byte> key;
#if !BOTAN_PRE_18 && !BOTAN_PRE_15
  const Botan::BlockCipher *cipher;
  const Botan::HashFunction *hash_algo;
#endif

  algo = getCryptAlgo (c2sCryptoMethod);
  key_len = max_keylength_of (algo);
  if (c2sCryptoMethod == BLOWFISH_CBC) key_len = 16;
  else if (c2sCryptoMethod == TWOFISH_CBC) key_len = 32;
  encryptBlock = iv_len = block_size_of (algo);
  macLen = getMacKeyLen (c2sMacMethod);
  if (!algo) return false;

  if (!compute_key(key, 'A', iv_len)) return false;
  InitializationVector c2s_iv (key);

  if (!compute_key(key, 'C', key_len)) return false;
  SymmetricKey c2s_key (key);

  if (!compute_key(key, 'E', macLen)) return false;
  SymmetricKey c2s_mac (key);

#if BOTAN_PRE_18 && !BOTAN_PRE_15
  encrypt = new Pipe (new CBC_Encryption (algo, "NoPadding", c2s_key, c2s_iv));
#else
  Algorithm_Factory &af = global_state().algorithm_factory();
  cipher = af.prototype_block_cipher (algo);
  encrypt = new Pipe (new CBC_Encryption (cipher->clone(), new Null_Padding, c2s_key, c2s_iv));
#endif

  if (macLen)
  {
#if BOTAN_PRE_18 && !BOTAN_PRE_15
    hmacOut = new HMAC (getHmacAlgo(c2sMacMethod));
#else
    hash_algo = af.prototype_hash_function (getHmacAlgo (c2sMacMethod));
    hmacOut = new HMAC (hash_algo->clone());
#endif
    hmacOut->set_key (c2s_mac);
  }
//  if (c2sCmprsMethod == ZLIB) compress = new Pipe (new Zlib_Compression(9));


  algo = getCryptAlgo (s2cCryptoMethod);
  key_len = max_keylength_of (algo);
  if (s2cCryptoMethod == BLOWFISH_CBC) key_len = 16;
  else if (s2cCryptoMethod == TWOFISH_CBC) key_len = 32;
  decryptBlock = iv_len = block_size_of (algo);
  macLen = getMacKeyLen (c2sMacMethod);
  if (!algo) return false;

  if (!compute_key(key, 'B', iv_len)) return false;
  InitializationVector s2c_iv (key);

  if (!compute_key(key, 'D', key_len)) return false;
  SymmetricKey s2c_key (key);

  if (!compute_key(key, 'F', macLen)) return false;
  SymmetricKey s2c_mac (key);

#if BOTAN_PRE_18 && !BOTAN_PRE_15
  decrypt = new Pipe (new CBC_Decryption (algo, "NoPadding", s2c_key, s2c_iv));
#else
  cipher = af.prototype_block_cipher (algo);
  decrypt = new Pipe (new CBC_Decryption (cipher->clone(), new Null_Padding, s2c_key, s2c_iv));
#endif

  if (macLen)
  {
#if BOTAN_PRE_18 && !BOTAN_PRE_15
    hmacIn = new HMAC (getHmacAlgo(s2cMacMethod));
#else
    hash_algo = af.prototype_hash_function (getHmacAlgo (s2cMacMethod));
    hmacIn = new HMAC (hash_algo->clone());
#endif
    hmacIn->set_key (s2c_mac);
  }
//  if (s2cCmprsMethod == ZLIB) decompress = new Pipe (new Zlib_Decompression);

  inited = true;
  return true;
}

bool ne7ssh_crypt::compute_key (Botan::SecureVector<Botan::byte>& key,  Botan::byte ID, uint32 nBytes)
{
  SecureVector<Botan::byte> hash, newKey;
  ne7ssh_string hashBytes;
  HashFunction* hashIt;
  const char* algo = getHashAlgo();
  uint32 len;

  if (!algo) return false;
  hashIt = get_hash (algo);
  if (!hashIt)
  {
    ne7ssh::errors()->push (session->getSshChannel(), "Undefined HASH algorithm encountered while computing the key.");
    return false;
  }

  hashBytes.addVectorField (K);
  hashBytes.addVector (H);
  hashBytes.addChar (ID);
  hashBytes.addVector (session->getSessionID());

  hash = hashIt->process (hashBytes.value());
  newKey.set (hash);
  len = newKey.size();

  while (len < nBytes)
  {
    hashBytes.clear();
    hashBytes.addVectorField (K);
    hashBytes.addVector (H);
    hashBytes.addVector (newKey);
    hash = hashIt->process (hashBytes.value());
    newKey.append (hash);
    len = newKey.size();
  }

  key.set (newKey.begin(), nBytes);
  delete (hashIt);
  return true;
}

bool ne7ssh_crypt::encryptPacket (Botan::SecureVector<Botan::byte> &crypted, Botan::SecureVector<Botan::byte> &hmac, Botan::SecureVector<Botan::byte> &packet, uint32 seq)
{
  SecureVector<Botan::byte> macStr;
  uint32 nSeq = (uint32)htonl (seq);

  encrypt->start_msg();
  encrypt->write (packet.begin(), packet.size());
  encrypt->end_msg();
//  encrypt->process_msg (packet);
  crypted = encrypt->read_all (encrypt->message_count() - 1);

  if (hmacOut)
  {
    macStr.set ((Botan::byte*)&nSeq, 4);
    macStr.append (packet);
    hmac = hmacOut->process (macStr);
  }

  return true;
}

bool ne7ssh_crypt::decryptPacket (Botan::SecureVector<Botan::byte> &decrypted, Botan::SecureVector<Botan::byte> &packet, uint32 len)
{
  uint32 pLen = packet.size();

  if (len % decryptBlock) len = len + (len % decryptBlock);

  if (len > pLen) len = pLen;

  decrypt->process_msg (packet.begin(), len);
  decrypted = decrypt->read_all (decrypt->message_count() - 1);

  return true;
}

void ne7ssh_crypt::compressData (Botan::SecureVector<Botan::byte> &buffer)
{
  SecureVector<Botan::byte> tmpVar;
  if (!compress) return;

  tmpVar.swap (buffer);
  compress->process_msg (tmpVar);
  tmpVar = compress->read_all (compress->message_count() - 1);
  buffer.set (tmpVar);
}

void ne7ssh_crypt::decompressData (Botan::SecureVector<Botan::byte> &buffer)
{
  SecureVector<Botan::byte> tmpVar;
  if (!decompress) return;

  tmpVar.swap (buffer);
  decompress->process_msg (tmpVar);
  tmpVar = decompress->read_all (compress->message_count() - 1);
  buffer.set (tmpVar);
}

void ne7ssh_crypt::computeMac (Botan::SecureVector<Botan::byte> &hmac, Botan::SecureVector<Botan::byte> &packet, uint32 seq)
{
  SecureVector<Botan::byte> macStr;
  uint32 nSeq = htonl (seq);

  if (hmacIn)
  {
    macStr.set ((Botan::byte*)&nSeq, 4);
    macStr.append (packet);
    hmac = hmacIn->process (macStr);
  }
  else hmac.destroy();
}

