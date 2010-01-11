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

#ifndef NE7SSH_TRANSPORT_H
#define NE7SSH_TRANSPORT_H

#include "crypt.h"
#include "ne7ssh_types.h"
#include "ne7ssh_string.h"

#if defined(WIN32) || defined(__MINGW32__)
#   include <winsock.h>
#endif
#include <sys/types.h>

//#define MAX_PACKET_LEN 35000
#define MAX_PACKET_LEN 34816
#define MAX_SEQUENCE 4294967295U

#if !defined(WIN32) && !defined(__MINGW32__)
#  define SOCKET int
#endif

class ne7ssh_session;

/**
@author Andrew Useckas
*/
class ne7ssh_transport
{
  private:
    uint32 seq, rSeq;
    const ne7ssh_session* session;
    SOCKET sock;
    Botan::SecureVector<Botan::byte> in;
    Botan::SecureVector<Botan::byte> inBuffer;

    /**
     * Switches socket's NonBlocking option on or off.
     * @param socket Socket number.
     * @param on If set to true, NonBlocking option will be turned on, and vice versa.
     * @return True if options have been successfuly set, otherwise false is returned.
     */
    bool NoBlock (SOCKET socket, bool on);

    /**
     * Waits for activity on a socket.
     * @param socket Socket number.
     * @param rw If set to true, checks if process can write to the socket, otherwise checks if there is data to be read from the socket.
     * @param timeout Desired timeout. By default the function will block until socket is ready for reading/writting. If set to '0', the function will return right away.
     * @return True if socket is ready for reading/writting, otherwise false is returned.
     */
    bool wait (SOCKET socket, int rw, int timeout = -1);

  public:
    /**
     * ne7ssh_transport class constructor.
     * <p> Transport class handles all socket communications for the ne7ssh library.
     * @param _session Pointer to ne7ssh_session instance.
     */
    ne7ssh_transport(ne7ssh_session* _session);

    /**
     * ne7ssh_transport class destructor.
     */
    ~ne7ssh_transport();

    /**
     * Establishes connection to a remote host.
     * @param host Host name or IP.
     * @param port Port.
     * @param timeout Timeout for the establish procedure, in seconds.
     * @return Socket number or -1 on failure.
     */
    SOCKET establish (const char *host, uint32 port, int timeout = 0);

    /**
     * Reads data from the socket.
     * @param buffer The data will be placed here.
     * @param append If set to true, received data will be appended to the buffer, instead of overwriting it.
     * @return True if data successfuly read, otherwise false is returned.
     */
    bool receive (Botan::SecureVector<Botan::byte>& buffer, bool append = false);

    /**
     * Writes a buffer to the socket.
     * @param buffer Data to be written to the socket.
     * @return True if data successful sent, otherwise false is returned.
     */
    bool send (Botan::SecureVector<Botan::byte>& buffer);

    /**
     * Assembles an SSH packet, as specified in SSH standards and passes the buffer to send() function.
     * @param buffer Payload to be sent.
     * @return True if send successful, otherwise false is returned.
     */
    bool sendPacket (Botan::SecureVector<Botan::byte>& buffer);

    /**
     * Waits until specified type of packet is received.
     * <p> If cmd is 0, waits for the first available packet of any kind.
     * <p> Once the desired packet is received, it is decrypted / decommpressed, the hMac is checked, and dropped into inBuffer class variable.
     * @param cmd SSH2 packet to wait for. If 0, first available packet will be read into inBuffer class variable.
     * @param bufferOnly Does not wait to receive a new packet, only checks existing receive buffer for unprocessed packets.
     * @return 1 if desired packet is received, 0 if there another packet is received, or -1 if HMAC checking is enabled, and remote and local HMACs do not match.
     */
    short waitForPacket (Botan::byte cmd, bool bufferOnly = false);

    /**
     * Gets the payload section from an SSH packet received by waitForPacket() function.
     * @param result The payload will be stored here.
     * @return The SSH packet passing length.
     */
    uint32 getPacket (Botan::SecureVector<Botan::byte>& result);

    /**
     * Checks to see if there is more data to be read from the socket.
     * @return True if there is data to be read, otherwise false is returned.
     */
    bool haveData ();
};

#endif
