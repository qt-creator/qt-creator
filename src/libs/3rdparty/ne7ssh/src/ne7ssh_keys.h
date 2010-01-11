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

#ifndef NE7SSH_KEYS_H
#define NE7SSH_KEYS_H
#include <botan/pubkey.h>
#include <botan/pem.h>
#include <botan/dsa.h>
#include <botan/rsa.h>
#include <botan/numthry.h>
#include <botan/der_enc.h>
#include <botan/ber_dec.h>

#include "ne7ssh_types.h"
#include "ne7ssh_string.h"

#define MAX_KEYSIZE 8192


/**
 @author Andrew Useckas <andrew@netsieben.com>
*/
class ne7ssh_keys
{
  private:
    Botan::DSA_PrivateKey *dsaPrivateKey;
    Botan::RSA_PrivateKey *rsaPrivateKey;
    ne7ssh_string publicKeyBlob;
    Botan::SecureVector<Botan::byte> signature;

    uint8 keyAlgo;

    /**
     * Extracts DSA key pair from a PEM encoded stream.
     * @param buffer PEM encoded string.
     * @param size Length of the stream.
     * @return True if keys succesfully extracted. Otherwise False is returned.
     */
    bool getDSAKeys (char* buffer, uint32 size);

    /**
     * Extracts RSA key pair from a PEM encoded stream.
     * @param buffer PEM encoded string.
     * @param size Length of the stream.
     * @return True if keys succesfully extracted. Otherwise False is returned.
     */
    bool getRSAKeys (char* buffer, uint32 size);

  public:
    enum keyAlgos { DSA, RSA };

    /**
     * ne7ssh_keys constructor.
     */
    ne7ssh_keys();

    /**
     * ne7ssh_keys destructor.
     * @return 
     */
    ~ne7ssh_keys();

    /**
     * Generates DSA Key pair and saves keys in specified files.
     * @param fqdn User id. Usually an Email. For example "test@netsieben.com"
     * @param privKeyFileName Full path to a file where generated private key should be written.
     * @param pubKeyFileName Full path to a file where generated public key should be written.
     * @param keySize Desired key size in bits. If not specified will default to 2048.
     * @return True if keys generated and written to the files. Otherwise false is returned.
     */
    bool generateDSAKeys (const char* fqdn, const char* privKeyFileName, const char* pubKeyFileName, uint16 keySize = 2048);

    /**
     * Generates RSA Key pair and saves keys in specified files.
     * @param fqdn User id. Usually an Email. For example "test@netsieben.com"
     * @param privKeyFileName Full path to a file where generated private key should be written.
     * @param pubKeyFileName Full path to a file where generated public key should be written.
     * @param keySize Desired key size in bits. If not specified will default to 2048.
     * @return True if keys generated and written to the files. Otherwise false is returned.
     */
    bool generateRSAKeys (const char* fqdn, const char* privKeyFileName, const char* pubKeyFileName, uint16 keySize = 2048);

    /**
     * Extracts key pair from a PEM encoded file. 
     * <p>Reads the file and determines the type of key, then passes processing to either getDsaKeys() or getRSAKeys(*) functions.
     * @param privKeyFileName Full path to PEM encoded file.
     * @return True if key succesfully extracted, otherwise False is returned.
     */
    bool getKeyPairFromFile (const char* privKeyFileName);

    /**
     * Generates a SHA-1 signature from sessionID and packet data provided.
     * <p>Determines key type and passed the processing either to generateDSASignature() or generateRSAKeys() functions.
     * @param sessionID SSH2 SessionID.
     * @param signingData Packet data to sign.
     * @return Returns signature, or 0 length vector if operation failed.
     */
    Botan::SecureVector<Botan::byte>& generateSignature (Botan::SecureVector<Botan::byte>& sessionID, Botan::SecureVector<Botan::byte>& signingData);

    /**
     * Generates a SHA-1 signature from sessionID and packet data provided, using DSA private key initialized before.
     * @param sessionID SSH2 SessionID.
     * @param signingData Packet data to sign.
     * @return Returns signature, or 0 length vector if operation failed.
     */
    Botan::SecureVector<Botan::byte> generateDSASignature (Botan::SecureVector<Botan::byte>& sessionID, Botan::SecureVector<Botan::byte>& signingData);

    /**
     * Generates a SHA-1 signature from sessionID and packet data provided, using DSA private key initialized before.
     * @param sessionID SSH2 SessionID.
     * @param signingData Packet data to sign.
     * @return Returns signature, or 0 length vector if operation failed.
     */
    Botan::SecureVector<Botan::byte> generateRSASignature (Botan::SecureVector<Botan::byte>& sessionID, Botan::SecureVector<Botan::byte>& signingData);

    /**
     * After key pair has been initialized, this function returns public key blob, as specified by SSH2 specs.
     * @return Public key blob or zero length vector, if there are initialized keys.
     */
    Botan::SecureVector<Botan::byte>& getPublicKeyBlob ();

    /**
     * Returns type of initialized keys.
     * @return Type of keys.
     */
    uint8 getKeyAlgo () { return keyAlgo; }

};

#endif
