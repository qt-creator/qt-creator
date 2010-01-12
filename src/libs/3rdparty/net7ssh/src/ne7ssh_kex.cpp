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

#include "ne7ssh_kex.h"
#include "ne7ssh.h"
#include <botan/rng.h>

using namespace Botan;

ne7ssh_kex::ne7ssh_kex(ne7ssh_session* _session) : session(_session)
{
}

ne7ssh_kex::~ne7ssh_kex()
{
}

void ne7ssh_kex::constructLocalKex()
{
  Botan::byte random[16];
  ne7ssh_string myCiphers (ne7ssh::CIPHER_ALGORITHMS, 0);
  ne7ssh_string myMacs (ne7ssh::MAC_ALGORITHMS, 0);
  SecureVector<Botan::byte> tmpCiphers, tmpMacs;
  char* cipher, *hmac;
  size_t len;

  localKex.clear();
  localKex.addChar (SSH2_MSG_KEXINIT);

#if BOTAN_PRE_18 || BOTAN_PRE_15
  Botan::Global_RNG::randomize (random, 16);
#else
  ne7ssh::rng->randomize (random, 16);
#endif

  localKex.addBytes (random, 16);
  localKex.addString (ne7ssh::KEX_ALGORITHMS);
  localKex.addString (ne7ssh::HOSTKEY_ALGORITHMS);

  if (ne7ssh::PREFERED_CIPHER)
  {
    myCiphers.split (',');
    myCiphers.resetParts();

    while ((cipher = myCiphers.nextPart()))
    {
      len = strlen (cipher);
      if (!memcmp (cipher, ne7ssh::PREFERED_CIPHER, len)) Ciphers.append ((Botan::byte*)cipher, (uint32_t) len);
      else 
      {
        tmpCiphers.append (',');
        tmpCiphers.append ((Botan::byte*)cipher, (uint32_t) len);
      }
    }
  }
  if (Ciphers.size()) Ciphers.append (tmpCiphers);
  else Ciphers.set (myCiphers.value());
//  Ciphers.append (&null_byte, 1);

  if (ne7ssh::PREFERED_MAC)
  {
    myMacs.split (',');
    myMacs.resetParts();

    while ((hmac = myMacs.nextPart()))
    {
      len = strlen (hmac);
      if (!memcmp (hmac, ne7ssh::PREFERED_MAC, len)) Hmacs.append ((Botan::byte*)hmac, (uint32_t) len);
      else 
      {
        tmpMacs.append (',');
        tmpMacs.append ((Botan::byte*)hmac, (uint32_t) len);
      }
    }
  }
  if (Hmacs.size()) Hmacs.append (tmpMacs);
  else Hmacs.set (myMacs.value());
//  Hmacs.append (&null_byte, 1);

  localKex.addVectorField (Ciphers);
  localKex.addVectorField (Ciphers);
  localKex.addVectorField (Hmacs);
  localKex.addVectorField (Hmacs);
  localKex.addString (ne7ssh::COMPRESSION_ALGORITHMS);
  localKex.addString (ne7ssh::COMPRESSION_ALGORITHMS);
  localKex.addInt (0);
  localKex.addInt (0);
  localKex.addChar ('\0');
  localKex.addInt (0);
}


bool ne7ssh_kex::sendInit ()
{
  ne7ssh_transport *_transport;

  if (!session->transport) 
  {
    ne7ssh::errors()->push (session->getSshChannel(), "No transport. Cannot initialize key exchange.");
    return false;
  }
  _transport = session->transport;

  constructLocalKex();

  if (!_transport->sendPacket (localKex.value())) return false;
  if (!_transport->waitForPacket (SSH2_MSG_KEXINIT))
  {
    ne7ssh::errors()->push (session->getSshChannel(), "Timeout while waiting for key exchange init reply");
    return false;
  }

  return true;
}

bool ne7ssh_kex::handleInit ()
{
  ne7ssh_transport *_transport = session->transport;
  ne7ssh_crypt *_crypto = session->crypto;
  SecureVector<Botan::byte> packet;
  uint32 padLen = _transport->getPacket (packet);
  ne7ssh_string remoteKex (packet, 17);
  SecureVector<Botan::byte> algos;
  SecureVector<Botan::byte> agreed;

  if (!_transport || !_crypto) return false;
  remotKex.clear();
  remotKex.addBytes (packet.begin(), packet.size() - padLen - 1);

  if (!remoteKex.getString (algos)) return false;
  if (!_crypto->agree (agreed, ne7ssh::KEX_ALGORITHMS, algos))
  {
    ne7ssh::errors()->push (session->getSshChannel(), "No compatible key exchange algorithms.");
    return false;
  }
  if (!_crypto->negotiatedKex (agreed)) return false;


  if (!remoteKex.getString (algos)) return false;
  if (!_crypto->agree (agreed, ne7ssh::HOSTKEY_ALGORITHMS, algos))
  {
    ne7ssh::errors()->push (session->getSshChannel(), "No compatible Hostkey algorithms.");
    return false;
  }
  if (!_crypto->negotiatedHostkey (agreed)) return false;


  if (!remoteKex.getString (algos)) return false;
  if (!_crypto->agree (agreed, (char*)Ciphers.begin(), algos))
  {
    ne7ssh::errors()->push (session->getSshChannel(), "No compatible cryptographic algorithms.");
    return false;
  }
  if (!_crypto->negotiatedCryptoC2s (agreed)) return false;


  if (!remoteKex.getString (algos)) return false;
  if (!_crypto->agree (agreed, (char*)Ciphers.begin(), algos))
  {
    ne7ssh::errors()->push (session->getSshChannel(), "No compatible cryptographic algorithms.");
    return false;
  }
  if (!_crypto->negotiatedCryptoS2c (agreed)) return false;


  if (!remoteKex.getString (algos)) return false;
  if (!_crypto->agree (agreed, (char*)Hmacs.begin(), algos))
  {
    ne7ssh::errors()->push (session->getSshChannel(), "No compatible HMAC algorithms.");
    return false;
  }
  if (!_crypto->negotiatedMacC2s (agreed)) return false;

  if (!remoteKex.getString (algos)) return false;
  if (!_crypto->agree (agreed, (char*)Hmacs.begin(), algos))
  {
    ne7ssh::errors()->push (session->getSshChannel(), "No compatible HMAC algorithms.");
    return false;
  }
  if (!_crypto->negotiatedMacS2c (agreed)) return false;


  if (!remoteKex.getString (algos)) return false;
  if (!_crypto->agree (agreed, ne7ssh::COMPRESSION_ALGORITHMS, algos))
  {
    ne7ssh::errors()->push (session->getSshChannel(), "No compatible compression algorithms.");
    return false;
  }
  if (!_crypto->negotiatedCmprsC2s (agreed)) return false;


  if (!remoteKex.getString (algos)) return false;
  if (!_crypto->agree (agreed, ne7ssh::COMPRESSION_ALGORITHMS, algos))
  {
    ne7ssh::errors()->push (session->getSshChannel(), "No compatible compression algorithms.");
    return false;
  }
  if (!_crypto->negotiatedCmprsS2c (agreed)) return false;

  return true;  
}

bool ne7ssh_kex::sendKexDHInit ()
{
  ne7ssh_string dhInit;
  ne7ssh_transport *_transport = session->transport;
  ne7ssh_crypt *_crypto = session->crypto;
  BigInt publicKey;
  SecureVector<Botan::byte> eVector;

  if (!_crypto->getKexPublic (publicKey)) return false;

  dhInit.addChar (SSH2_MSG_KEXDH_INIT);
  dhInit.addBigInt (publicKey);
  ne7ssh_string::bn2vector (eVector, publicKey);
  e.clear();
  e.addVector (eVector);

  if (!_transport->sendPacket (dhInit.value())) return false;
  if (!_transport->waitForPacket (SSH2_MSG_KEXDH_REPLY))
  {
    ne7ssh::errors()->push (session->getSshChannel(), "Timeout while waiting for key exchange dh reply.");
    return false;
  }
  return true;
}

bool ne7ssh_kex::handleKexDHReply ()
{
  ne7ssh_transport *_transport = session->transport;
  ne7ssh_crypt *_crypto = session->crypto;
  SecureVector<Botan::byte> packet;
  _transport->getPacket (packet);
  ne7ssh_string remoteKexDH (packet, 1);
  SecureVector<Botan::byte> field, fVector, hSig, kVector, hVector;
  BigInt publicKey;

  if (!remoteKexDH.getString (field)) return false;
  hostKey.clear();
  hostKey.addVector (field);

  if (!remoteKexDH.getBigInt (publicKey)) return false;
  ne7ssh_string::bn2vector (fVector, publicKey);
  f.clear();
  f.addVector (fVector);

  if (!remoteKexDH.getString (hSig)) return false;

  if (!_crypto->makeKexSecret (kVector, publicKey)) return false;
  k.clear();
  k.addVector (kVector);

  makeH (hVector);
  if (hVector.is_empty()) return false;
  if (!_crypto->isInited()) session->setSessionID (hVector);

  if (!_crypto->verifySig (hostKey.value(), hSig)) return false;

  return true;
}

bool ne7ssh_kex::sendKexNewKeys ()
{
  ne7ssh_transport *_transport = session->transport;
  ne7ssh_crypt *_crypto = session->crypto;
  ne7ssh_string newKeys;

  if (!_transport->waitForPacket (SSH2_MSG_NEWKEYS))
  {
    ne7ssh::errors()->push (session->getSshChannel(), "Timeout while waiting for key exchange newkeys reply.");
    return false;
  }

  newKeys.addChar (SSH2_MSG_NEWKEYS);
  if (!_transport->sendPacket (newKeys.value())) return false;

  if (!_crypto->makeNewKeys())
  {
    ne7ssh::errors()->push (session->getSshChannel(), "Could not make keys.");
    return false;
  }

  return true;
}


void ne7ssh_kex::makeH (Botan::SecureVector<Botan::byte> &hVector)
{
  ne7ssh_crypt *_crypto = session->crypto;
  ne7ssh_string hashBytes;

  hashBytes.addVectorField (session->getLocalVersion());
  hashBytes.addVectorField (session->getRemoteVersion());
  hashBytes.addVectorField (localKex.value());
  hashBytes.addVectorField (remotKex.value());
  hashBytes.addVectorField (hostKey.value());
  hashBytes.addVectorField (e.value());
  hashBytes.addVectorField (f.value());
  hashBytes.addVectorField (k.value());

  _crypto->computeH (hVector, hashBytes.value());
}


