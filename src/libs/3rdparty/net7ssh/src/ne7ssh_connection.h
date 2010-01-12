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

#ifndef NE7SSH_CONNECTION_H
#define NE7SSH_CONNECTION_H

#include "ne7ssh_transport.h"
#include "ne7ssh_session.h"
#include "ne7ssh_channel.h"
#include "ne7ssh_keys.h"
#include "crypt.h"
#include "ne7ssh_types.h"
#include "ne7ssh_string.h"
#include "ne7ssh_sftp.h"


/**
@author Andrew Useckas
*/
class ne7ssh_connection
{
  private:
    SOCKET sock;
    int thisChannel;
    ne7ssh_crypt *crypto;
    ne7ssh_transport *transport;
    ne7ssh_session *session;
    ne7ssh_channel *channel;
    Ne7sshSftp* sftp;

    Ne7ssh_Mutex mut;
    bool connected;
    bool cmdRunning;
    bool cmdClosed;


    /**
     * Checks if remote side is returning a correctly formated SSH version string, and makes sure that version 2 of SSH protocol is supported by the remote side.
     * @return False if version string is malformed, or version 2 is not supported, otherwise true is returned.
     */
    bool checkRemoteVersion ();

    /**
     * Sends local version string.
     * @return Returns false is there any communication errors occur, otherwise true is returned.
     */
    bool sendLocalVersion ();

    /**
     * Sends an SSH service request, waits for 'SERVICE_ACCEPT' packet.
     * @param service pointer to a string containing the requested SSH service. For example "ssh-userauth".
     * @return True If SERVICE_ACCEPT packet was received, otherwise false is returned.
     */
    bool requestService (const char* service);

    /**
     * Sends an authentication request of "password" type. Waits for packet 'USERAUTH_SUCESS'.
     * @param username Username used for authentication.
     * @param password Password used for authentication.
     * @return True if authentication was successful, otherwise false is returned.
     */
    bool authWithPassword (const char* username, const char* password);

    /**
     * Sends a test message to check if "publickey" authentication is allowed fo specified user.
     * If succesfull proceeds wtih generating a signature and sending real authentication packet
     * of "publickey" type.
     * @param username Username used for authentication.
     * @param privKeyFileName Full path to file containing private key to be used in authentication.
     * @return True if authentication was successful, otherwise false is returned.
     */
    bool authWithKey (const char* username, const char* privKeyFileName);

  public:
    /**
     * ne7ssh_connection class constructor.
     */
    ne7ssh_connection();

    /**
     * ne7ssh_connection class destructor.
     */
    ~ne7ssh_connection();

    /**
     * Connects to a remote host using SSH protocol version 2, with password based authentication.
     * @param channelID ID of the new channel.
     * @param host Hostname / IP of the remote host.
     * @param port Connection port.
     * @param username Username to use in the authentication.
     * @param password Password to use in the authentication.
     * @param shell Set this to true if you wish to launch the shell on the remote end. By default set to true.
     * @param timeout Timeout for the connection procedure, in seconds.
     * @return A newly assigned channel ID, or -1 if connection failed.
     */
    int connectWithPassword (uint32 channelID, const char *host, uint32 port, const char* username, const char* password, bool shell = true, int timeout = 0);

    /**
     * Connects to a remote host using SSH protocol version 2, with publickey based authentication.
     * @param channelID ID assigned to the new channel.
     * @param host Hostname / IP of the remote host.
     * @param port Connection port.
     * @param username Username to use in the authentication.
     * @param privKeyFileName Full path to file containing private key to be used in authentication.
     * @param shell Set this to true if you wish to launch the shell on the remote end. By default set to true.
     * @param timeout Timeout for the connection procedure, in seconds.
     * @return A newly assigned channel ID, or -1 if connection failed.
     */
    int connectWithKey (uint32 channelID, const char *host, uint32 port, const char* username, const char* privKeyFileName, bool shell = true, int timeout = 0);

    /**
     * Retrieves the tcp socket number.
     * @return Socket, or -1 if not connected.
     */
    SOCKET getSocket () { return sock; }

    /**
     * When new data arrives, and is available for reading, this function is called from selectThread to handle it.
     */
    void handleData ();

    /**
     * This function is used to write commands to the buffer, later to be sent to the remote site for execution.
     * @param data Pointer to a string, containing command to be written to the buffer.
     */
    void sendData (const char* data);

    /**
     * Sets the current SSH channel number.
     */
    void setChannelNo (int channelID) { thisChannel = channelID; }

    /**
     * Retrieves the current SSH channel.
     * @return Returns SSH channel or -1 if not connected.
     */
    int getChannelNo () { return thisChannel; }

    /**
     * Checks for the data in the send buffer.
     * @return True is there is data to send, otherwise false.
     */
    bool data2Send () { return channel->data2Send(); }

    /**
     * Sends the content of the buffer.,
     *<p>Usually used after data2Send returns true, executed by selectThread.
     */
    void sendData () { channel->sendAll (); }

    /**
    * 
    * @param cmd 
    * @return 
    */
    bool sendCmd (const char* cmd);

    /**
     * This function is used to close the current connection.   
     *<p>First closes the channel, and then the connection itself.
     * @return True, if packet sent successfully, otherwise false is returned.
     */
    bool sendClose ();

    /**
     * Checks if channel is open.
     * @return True if channel is open, otherwise false is returned.
     */
    bool isOpen () { return channel->isOpen(); }

    /**
     * Checks if process is connected and authenticated to the remote side.
     * @return True if connected, otherwise false is returned.
     */
    bool isConnected () { return connected; }

    /**
     * Retrieves the last received packet.
     * @return A reference to a buffer containing the last received packet.
     */
    Botan::SecureVector<Botan::byte>& getReceived () { return channel->getReceived(); }
		
    /**
    * When executing a single command with ne7ssh::sendCmd this command is used to determine when remote side finishes the xecution.
    * @return True if execution of the command is complete. Otherwise false.
    */
    bool getCmdComplete() { return channel->getCmdComplete(); }

    /**
    * When executing a single command with ne7ssh::sendCmd this command is used to determine when the user requested a close() on the channel.
    * @return True if the user requested to close the channel. Otherwise false.
    */
    bool isCmdClosed() { return cmdClosed; }

    /**
    * Determines if the shell has been spawned on the remote side.
    * @return True if the shell has been spawned. Otherwise false.
    */
    bool isRemoteShell () { return channel->isRemoteShell(); }

    /**
    * Checks if current connection is executing a single command, without a shell.
    * @return True if a single command is running. Otherwise false.
    */
    bool isCmdRunning () { return cmdRunning; }

    /**
    * Starts a new sftp subsystem.
    * @return Returns a pointer to the newly started Ne7sshSftp instance.
    */
    Ne7sshSftp* startSftp ();

    /**
    * Checks if SFTP subsystem is active on the current connection.
    * @return True if SFTP subsystem is active, otherwise false.
    */
    bool isSftpActive ();
};

#endif
