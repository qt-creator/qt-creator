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

#ifndef NE7SSHSFTPPACKET_H
#define NE7SSHSFTPPACKET_H

#include "ne7ssh_types.h"
#include "ne7ssh_string.h"

/**
  @author Andrew Useckas <andrew@netsieben.com>
*/
class Ne7sshSftpPacket : public ne7ssh_string
{
  private:
    int channel;

  public:
    /**
     * Default constructor.
     */

    Ne7sshSftpPacket();
    /**
     * Constructor.
     * @param channel Channel ID, returned by connect methods.
     */
    Ne7sshSftpPacket(int channel);

    /**
     * Constructor. Intializes the class with packet data.
     * @param var Reference to packet data.
     * @param position Offset.
     */
    Ne7sshSftpPacket(Botan::SecureVector<Botan::byte>& var, uint32 position);

    /**
     * Default destructor.
     */
    ~Ne7sshSftpPacket();

    /**
     * Returns buffer as a vector appending the SFTP subsystem specific packet headers.
     * @return Reference to the buffer.
     */
    Botan::SecureVector<Botan::byte> &value ();

    /**
     * Returns buffer as a vector appending the SFTP subsystem specific packet headers, including the length in the first packet transmitted. Used in transmissions when it's necessary to split the message into multiple packets.
     * @param len Length to append to the first packet in the message.
     * @return Reference to SFTP packet or empty vector on error.
     */
    Botan::SecureVector<Botan::byte> valueFragment (uint32 len = 0);

    /**
     * Appends 64 bit integer to the packet buffer.
     * @param var 64 bit integer.
     */
    void addInt64 (const uint64 var);

    /**
     * Retrieves a 64 bit integer from a packet buffer.
     * @return Unsigned 64 bit integer.
     */
    uint64 getInt64 ();

    /**
     * Checks if the channel ID is set in the instance. Channel ID is needed to construct a SFTP packet.
     * @return True if the channel ID is set. Otherwise false.
     */
    bool isChannelSet ();

};

#endif
