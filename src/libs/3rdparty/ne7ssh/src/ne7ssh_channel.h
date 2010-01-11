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

#ifndef NE7SSH_CHANNEL_H
#define NE7SSH_CHANNEL_H

#include "ne7ssh_types.h"
#include "ne7ssh_string.h"

class ne7ssh_session;

/**
@author Andrew Useckas
*/
class ne7ssh_channel
{
  private:
    bool eof, closed;
    bool cmdComplete;
    bool shellSpawned;

//    static uint32 channelCount;
    ne7ssh_session *session;
    ne7ssh_string inBuffer;
    ne7ssh_string outBuffer;
    ne7ssh_string delayedBuffer;

    /**
     * This function is used to handle the 'CHANNEL_OPEN_CONFIRMATION' packet.
     * <p> After parsing the payload, send channel ID is assigned, along with send windows size and maximum packer size.
     * @return Always returns true.
     */
    bool handleChannelConfirm ();

    /**
     * This function is used to handle the 'WINDOWS_ADJUST' packet.
     * <p>It's used to increase our sending window size.
     * @param packet Reference to vector containing WINDOW_ADJUST packet.
     * @return If parsing of payload is successful, returns true, otherwise false is returned.
     */
    bool adjustWindow (Botan::SecureVector<Botan::byte>& packet);

    /**
     * This function is used to handle the 'DATA' packet.
     * <p>It's used to parse the payload, and add received data to the buffer.
     * @param packet Reference to vector containing 'DATA' packet.
     * @return If parsing of payload is successful, returns true, otherwise false is returned.
     */
    virtual bool handleData (Botan::SecureVector<Botan::byte>& packet);

    /**
     * This function is used to handle 'EXTENDED_DATA' packet. This packet is mostly used to transmit remote side errors.
     * @param packet Reference to vector containing 'EXTENDED_DATA' packet.
     * @return If parsing of payload is successful, returns true, otherwise false is returned.
     */
    bool handleExtendedData (Botan::SecureVector<Botan::byte>& packet);

    /**
     * This function is used to handle the 'EOF' packet.
     * <p>It's  used  to close the receiving window and channel.
     * @param packet Reference to vector containing EOF packet.
     */
    bool handleEof (Botan::SecureVector<Botan::byte>& packet);

    /**
     * This function is used to handle the 'CLOSE' packet.
     * <p> If the close action wasn't initiated on this end, we also send a 'CLOSE' packet to the remote side, prompting the closing of remote side's receiving channel.
     * @param packet Reference to vector containing the 'CLOSE' packet.
     */
    void handleClose (Botan::SecureVector<Botan::byte>& packet);

    /**
     * This function is used to handle the 'REQUEST' packet.
     * <p> At this point only two requests are supported, namely "exit-signal" and "exit-status". For the most part we ignore this packet, which is safe to do according to SSH specs.
     * @param packet Reference to vector containing the 'REQUEST' packet.
     */
    void handleRequest (Botan::SecureVector<Botan::byte>& packet);

    /**
     * This function is used to handle the 'DISCONNECT' packet.
     * <p> In normal operation we should not get this packet. Only if some serious error occurs, and makes remote side drop the connection, will this packet be received. And at that point we disconnect right away, and throw an error.
     * @param packet Reference to vector containing the 'DISCONNECT' packet.
     */
    bool handleDisconnect (Botan::SecureVector<Botan::byte>& packet);

  protected:
    uint32 windowRecv, windowSend;

    bool channelOpened;

    /**
     * Request adjustment of the send window size on the remote end, so we can receive more data.
     */
    void sendAdjustWindow ();

  public:
    /**
     * ne7ssh_channel class consturctor.
     * @param _session Pointer to ne7ssh_session.
     */
    ne7ssh_channel(ne7ssh_session* _session);

    /**
     * ne7ssh_channel class destructor.
     */
    virtual ~ne7ssh_channel();

    /**
     * Requests 'CHANNEL_OPEN' from the remote side.
     * @param channelID New receiving channel ID.
     * @return Returns new channel ID, or 0 if open fails.
     */
    uint32 open (uint32 channelID);

    /**
     * Requests shell from remote side. Does not wait for or expect a reply. According to SSH specs that's an acceptable behavior.
     */
    void getShell ();

    /**
     * Executes a single command on the remote end and terminates the connection.
     * @param cmd Remote command to execute.
     * @return True if command if sening of a command succeded. False returned on failure.
     */
    bool execCmd (const char* cmd);

    /**
     * Receives new packet from remote side. This function is mostly used from selectThread.
     */
    void receive ();

    /**
    * Handle a packet received from remote side.
    * @param _packet Reference to a newly received packet.
    * @return True if the packet successfully processed. False on any error.
    */
    bool handleReceived (Botan::SecureVector<Botan::byte>& _packet);

    /**
     * Pushes a new command to the buffer where the selectThread will catch and send it.
     * @param data Reference to vector containing a command to be added to the buffer.
     */
    void write (Botan::SecureVector<Botan::byte>& data);

    /**
     * Sends the entire buffer. Most often called from selectThread via ne7ssh_connection class.
     */
    void sendAll ();

    /**
     * Checks if there is any data waiting to be sent. Most often called from selectThread via ne7ssh_connection class.
     * @return True if there is data to send, otherwise false is returned.
     */
    bool data2Send () { if (outBuffer.length() || delayedBuffer.length()) return true; else return false; }

    /**
     * Checks if current channel is in an open state.
     * @return True if channel is open, otherwise false is returned.
     */
    bool isOpen () { return channelOpened; } 

    /**
     * When closing a channel, initiates the closing procedure.
     * @return False if sending fails. Otherwise true is returned.
     */
    bool sendClose ();

    /**
     * Send EOF to the remote side.
     * @return True if sending succeeds, otherwise false.
     */
    bool sendEof ();

    /**
     * Gets last received packet.
     * @return Reference to a vector containing the last received packet.
     */
    Botan::SecureVector<Botan::byte>& getReceived () { return inBuffer.value(); }

    /**
    * When executing a single command with ne7ssh::sendCmd this command is used to determine when remote side finishes the execution.
    * @return True if execution of the command is complete. Otherwise false.
    */
    bool getCmdComplete () { return cmdComplete; }

    /**
    * Determines if the shell has been spawned on the remote side.
    * @return True if the shell has been spawned. Otherwise false.
    */
    bool isRemoteShell () { return shellSpawned; }

    /**
    * Checks if receive window needs adjusting, if so send a window adjust request.
    * @param bufferSize Current buffer size.
    * @return False on any error, otherwise true.
    */
    bool adjustRecvWindow (int bufferSize);

    /**
    * Gets the current size of the receive window.
    * @return Size of the revceive window.
    */
    uint32 getRecvWindow () { return windowRecv; }

    /**
    * Gets the current size of the send window.
    * @return Size of the send window.
    */
    uint32 getSendWindow () { return windowSend; }
};

#endif
