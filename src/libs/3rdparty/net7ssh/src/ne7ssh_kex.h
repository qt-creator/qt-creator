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

#ifndef NE7SSH_KEX_H
#define NE7SSH_KEX_H

#include "ne7ssh_types.h"
#include "ne7ssh_session.h"
#include "ne7ssh_string.h"
#include "ne7ssh_transport.h"
#include "crypt.h"

/**
@author Andrew Useckas
*/
class ne7ssh_kex
{
  private:
    ne7ssh_session* session;
    ne7ssh_string localKex;
    ne7ssh_string remotKex;
    ne7ssh_string hostKey;
    ne7ssh_string e;
    ne7ssh_string f;
    ne7ssh_string k;
    Botan::SecureVector<Botan::byte> Ciphers, Hmacs;

    /**
     * Constructs local 'KEX_INIT' payload
     */
    void constructLocalKex();

    /**
     * Computes H hash, from concated values of the local SSH version string, remote SSH version string, local KEX_INIT payload, remote KEX_INIT payload, host key, e, f and k BigInt values.
     * @param hVector Reference to a vecor where H value will be stored.
     */
    void makeH (Botan::SecureVector<Botan::byte>& hVector);

  public:
    /**
     * ne7ssh_kex class constructor.
     * @param _session Pointer to ne7ssh_session variable.
     */
    ne7ssh_kex(ne7ssh_session* _session);

    /**
     * ne7ssh_kex class destructor.
     */
    ~ne7ssh_kex();

    /**
     * Sends 'KEX_INIT' packet and waits for 'KEX_INIT' reply.
     * @return True if successful, otherwise false is returned.
     */
    bool sendInit();

    /**
     * After sendInit() function returnes true, this functions is used to parse the received 'KEX_INIT' packet.
     * <p> Used to agree on cipher, hmac, etc. algorithms used in communication between client and server.
     * @return True if parsing was succesful and all algorithms agreed upon, otherwise false is returned.
     */
    bool handleInit();

    /**
     * Sends 'KEXDH_INIT' packet and waits for 'KEXDH_REPLY'.
     * @return True if reply is received, otherwise false is returned.
     */
    bool sendKexDHInit();

    /**
     * After sendKexDHInit() returns true, this function is used to handle the received 'KEXDH_REPLY'.
     * <p> This is the function to create the shared secret K. It also extracts the host key and signature fields from the payload, generates DSA/RSA keys, and verifies the signature.
     * @return True if all operations are completed successfully, otherwise false is returned.
     */
    bool handleKexDHReply();

    /**
     * This function waits for 'NEWKEYS' packet from the remote host.
     * <p> Once the packet is received, local 'NEWKEYS' packet is sent, all encryption and hmac keys are generated and encrypted communication is established.
     * @return True if all operations are successful, otherwise false is returned.
     */
    bool sendKexNewKeys();

};

#endif
