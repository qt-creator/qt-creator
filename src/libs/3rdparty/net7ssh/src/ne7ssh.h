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

#ifndef NE7SSH_H
#define NE7SSH_H

#include <botan/build.h>

//#include <botan/zlib.h>
//#include "error.h"

#if BOTAN_VERSION_MAJOR > 1
#   error Unsupported Botan Version
#endif

#define BOTAN_PRE_15 (BOTAN_VERSION_MINOR < 5)
#define BOTAN_PRE_18 (BOTAN_VERSION_MINOR < 8)

#if !BOTAN_PRE_18 && !BOTAN_PRE_15
# include <botan/auto_rng.h>
# include <botan/rng.h>
#endif

#include <stdlib.h>
#include <string>
#include <fcntl.h>
#if !defined(WIN32) && !defined(__MINGW32__)
#   include <pthread.h>
#   include <sys/select.h>
#   include <unistd.h>
typedef pthread_t ne7ssh_thread_t;
#else
#include <windows.h>
typedef HANDLE ne7ssh_thread_t;
#endif

#include "ne7ssh_types.h"
#include "ne7ssh_error.h"
#include "ne7ssh_mutex.h"

#define SSH2_MSG_DISCONNECT 1
#define SSH2_MSG_IGNORE 2

#define SSH2_MSG_KEXINIT  20
#define SSH2_MSG_NEWKEYS  21

#define SSH2_MSG_KEXDH_INIT 30
#define SSH2_MSG_KEXDH_REPLY  31

#define SSH2_MSG_SERVICE_REQUEST 5
#define SSH2_MSG_SERVICE_ACCEPT 6

#define SSH2_MSG_USERAUTH_REQUEST 50
#define SSH2_MSG_USERAUTH_FAILURE 51
#define SSH2_MSG_USERAUTH_SUCCESS 52
#define SSH2_MSG_USERAUTH_BANNER 53
#define SSH2_MSG_USERAUTH_PK_OK 60

#define SSH2_MSG_CHANNEL_OPEN                           90
#define SSH2_MSG_CHANNEL_OPEN_CONFIRMATION              91
#define SSH2_MSG_CHANNEL_OPEN_FAILURE                   92
#define SSH2_MSG_CHANNEL_WINDOW_ADJUST                  93
#define SSH2_MSG_CHANNEL_DATA                           94
#define SSH2_MSG_CHANNEL_EXTENDED_DATA                  95
#define SSH2_MSG_CHANNEL_EOF                            96
#define SSH2_MSG_CHANNEL_CLOSE                          97
#define SSH2_MSG_CHANNEL_REQUEST                        98
#define SSH2_MSG_CHANNEL_SUCCESS                        99
#define SSH2_MSG_CHANNEL_FAILURE                        100

class ne7ssh_connection;

/** Structure used to store connections. Used to better sync with selectThread. */
typedef struct {
  /** Pointer to all active connections.*/
  ne7ssh_connection **conns;
  /** Active connection count.	*/
  uint32 count;
} connStruct;

/** definitions for Botan */
namespace Botan
{
    class LibraryInitializer;
}

class Ne7SftpSubsystem;

/**
@author Andrew Useckas
*/
class SSH_EXPORT ne7ssh
{
  private:

    static Ne7ssh_Mutex mut;
    Botan::LibraryInitializer *init;
    ne7ssh_connection **connections;
    uint32 conCount;
    static bool running;
    static bool selectActive;
    connStruct allConns;


    /**
     * Send / Receive thread.
     * <p> For Internal use only
     * @return Usually 0 when thread terminates
     */
    static void *selectThread (void*);

    /**
     * Returns the number of active channel.
     * @return Active channel.
     */
    uint32 getChannelNo ();
    ne7ssh_thread_t select_thread;
    bool connected;

    /**
    * Lock the mutex.
    * @return True if lock aquired. Oterwise false.
    */
    static bool lock ();

    /**
    * Unlock the mutext.
    * @return True if the mutext successfully unlocked. Otherwise false.
    */
    static bool unlock ();
    static Ne7sshError* errs;

  public:
#if !BOTAN_PRE_18 && !BOTAN_PRE_15
    static Botan::RandomNumberGenerator *rng;
#endif
    static const char* SSH_VERSION;
    static const char* KEX_ALGORITHMS;
    static const char* HOSTKEY_ALGORITHMS;
    static const char* MAC_ALGORITHMS;
    static const char* CIPHER_ALGORITHMS;
    static const char* COMPRESSION_ALGORITHMS;
    static char* PREFERED_CIPHER;
    static char* PREFERED_MAC;

    /**
     * Default constructor. Used to allocate required memory, as well as initializing cryptographic routines.
     */
    ne7ssh();
    /**
     * Destructor.
     */
    ~ne7ssh();

    /**
     * Connect to remote host using SSH2 protocol, with password authentication.
     * @param host Hostname or IP to connect to.
     * @param port Port to connect to.
     * @param username Username to use in authentication.
     * @param password Password to use in authentication.
     * @param shell Set this to true if you wish to launch the shell on the remote end. By default set to true.
     * @param timeout Timeout for the connection procedure, in seconds.
     * @return Returns newly assigned channel ID, or -1 if connection failed.
     */
    int connectWithPassword (const char* host, const int port, const char* username, const char* password, bool shell = true, const int timeout = 0);

    /**
     * Connect to remote host using SSH2 protocol, with publickey authentication.
     * <p> Reads private key from a file specified, and uses it to authenticate to remote host.
     * Remote side must have public key from the key pair for authentication to succeed.
     * @param host Hostname or IP to connect to.
     * @param port Port to connect to.
     * @param username Username to use in authentication.
     * @param privKeyFileName Full path to file containing private key used in authentication.
     * @param shell Set this to true if you wish to launch the shell on the remote end. By default set to true.
     * @param timeout Timeout for the connection procedure, in seconds.
     * @return Returns newly assigned channel ID, or -1 if connection failed.
     */
    int connectWithKey (const char* host, const int port, const char* username, const char* privKeyFileName, bool shell = true, const int timeout = 0);

    /**
     * Retrieves a pointer to all current connections.
     * <p> For internal use only.
     * @return Returns pointer to pointers to ne7ssh_connection class, or 0 if no connection exist.
     */
//    ne7ssh_connection** getConnections () { return connections; }

    connStruct* getConnetions () { return &allConns; }

    /**
     * Retreives count of current connections
     * <p> For internal use only.
     * @return Returns current count of connections.
     */
//    uint32 getConCount () { return conCount; }

    /**
     * Sends a command string on specified channel, provided the specified channel has been previously opened through connectWithPassword() function.
     * @param data Pointer to the command string to send to a channel.
     * @param channel Channel to send data on.
     * @return Returns true if the send was successful, otherwise false returned.
     */
    bool send (const char* data, int channel);

    /**
    * Can be used to send a single command and disconnect, similiar behavior to openssh when one appends a command to the end of ssh command.
    * @param cmd Remote command to execute. Can be used to read files on unix with 'cat [filename]'.
    * @param channel Channel to send the command.
    * @param timeout How long to wait before giving up.
    * @return Returns true if the send was successful, otherwise false returned.
    */
    bool sendCmd (const char* cmd, int channel, int timeout);

    /**
     * Closes specified channel.
     * @param channel Channel to close.
     * @return Returns true if closing was successful, otherwise false is returned.
     */
    bool close (int channel);

    /**
     * Sets connection count.
     * <p> For internal use only.
     * @param count Integer to set connection count.
     */
    void setCount (uint32 count) { conCount = count; }

    /**
    * Reads all data from receiving buffer on specified channel.
    * @param channel Channel to read data on.
    * @return Returns string read from receiver buffer or 0 if buffer is empty.
    */
    const char* read (int channel, bool do_lock=true);

    /**
    * Reads all data from receiving buffer on specified channel. Returns pointer to void. Together with getReceivedSize and sendCmd can be used to read remote files.
    * @param channel Channel to read data on.
    * @return Returns pointer to the start of binary data or 0 if nothing received.
    */
    void* readBinary (int channel);

    /**
     * Returns the size of all data read. Used to read buffer passed 0x0.
     * @param channel Channel number which buffer size to check.
     * @return Return size of the buffer, or 0x0 if receive buffer empty.
     */
    int getReceivedSize (int channel, bool do_lock=true);

    /**
     * Wait until receiving buffer contains a string passed in str, or until the function timeouts as specified in timeout.
     * @param channel Channel to wait on.
     * @param str String to wait for.
     * @param timeout Timeout in seconds.
     * @return Returns true if string specified in str variable has been received, otherwise false returned.
     */
    bool waitFor (int channel, const char* str, uint32 timeout=0);

    /**
     * Sets prefered cipher and hmac algorithms.
     * <p> This function as to be executed before connection functions, just after initialization of ne7ssh class.
     * @param prefCipher prefered cipher algorithm string representation. Possible cipher algorithms are aes256-cbc, twofish-cbc, twofish256-cbc, blowfish-cbc, 3des-cbc, aes128-cbc, cast128-cbc.
     * @param prefHmac preferede hmac algorithm string representation. Possible hmac algorithms are hmac-md5, hmac-sha1, none.
     */
    void setOptions (const char* prefCipher, const char* prefHmac);


    /**
     * Generate key pair.
     * @param type String specifying key type. Currently "dsa" and "rsa" are supported.
     * @param fqdn User id. Usually an Email. For example "test@netsieben.com"
     * @param privKeyFileName Full path to a file where generated private key should be written.
     * @param pubKeyFileName Full path to a file where generated public key should be written.
     * @param keySize Desired key size in bits. If not specified will default to 2048.
     * @return Return true if keys generated and written to the files. Otherwise false is returned.
     */
    bool generateKeyPair (const char* type, const char* fqdn, const char* privKeyFileName, const char* pubKeyFileName, uint16 keySize = 0);

    /**
     * This method is used to initialize a new SFTP subsystem.
     * @param _sftp Reference to SFTP subsystem to be initialized.
     * @param channel Channel ID returned by one of the connect methods.
     * @return True if the new subsystem successfully initialized. False on any error.
     */
    bool initSftp (Ne7SftpSubsystem& _sftp, int channel);

    /**
     * This method returns a pointer to the current Error collection.
     * @return the Error collection
     */
    static Ne7sshError* errors();

    static bool isSelectActive() { return selectActive; }
    static void selectDead() { selectActive = false; }
};

class Ne7sshSftp;

/**
	@author Andrew Useckas <andrew@netsieben.com>
*/
class SSH_EXPORT Ne7SftpSubsystem
{
  private:
    bool inited;
    Ne7sshSftp* sftp;

/**
 * Pushes and error to the error buffer, if this subsystem has not been initialized before usage.
 * @return True if the push succeeds. Otherwise false.
 */
bool errorNotInited ();

  public:
    /** Structure used to store rmote file attributes. */
    typedef struct
    {
      uint64_t  size;
      uint32_t	owner;
      uint32_t	group;
      uint32_t	permissions;
      uint32_t	atime;
      uint32_t	mtime;
    } fileAttrs;

    /** Modes used when opening a remote file. */
    enum writeMode { READ, OVERWRITE, APPEND };

    /**
     * Default constructor.
     */
    Ne7SftpSubsystem ();

    /**
     * Constructor used to initialize the subsystem with a new Ne7sshSftp class instance.
     * @param _sftp Ne7sshSftp class instance.
     */
    Ne7SftpSubsystem (class Ne7sshSftp* _sftp);

    /**
     * Default destructor.
     */
    ~Ne7SftpSubsystem();

    /**
    * This method is used to set a timeout for all SFTP subsystem communications.
    * @param _timeout Timeout in seconds.
    * @return True if timeout set, otherwise false.
    */
    bool setTimeout (uint32 _timeout);

    /**
    * Low level method used to open a remote file.
    * @param filename Relative or full path to the file.
    * @param mode Mode to be used when opening the file. Can be one of the modes defined by writeMode class variable.
    * @return Newly opened file ID or 0 if file could not be opened.
    */
    uint32 openFile (const char* filename, uint8 mode);

    /**
    * Low level method used to open an inode containing file entries a.k.a directory.
    * @param dirname Relative or full path to the inode.
    * @return Newly opened file ID or 0 if the inode could not be opened.
    */
    uint32 openDir (const char* dirname);

    /**
    * Low level method used to read datablock up to the size of SFTP_MAX_MSG_SIZE from a file.
    * @param fileID File ID retruned by openFile() method.
    * @param offset Offset.
    * @return True if file content successfully read and placed in the buffer. Otherwise false.
    */
    bool readFile (uint32 fileID, uint64 offset = 0);

    /**
    * Low level method used to write data-block up to the size of SFTP_MAX_MSG_SIZE to a remote file.
    * @param fileID File ID returned by openFile() method.
    * @param data Pointer to a buffer containing the data.
    * @param len Length of the block.
    * @param offset Offset in the remote file. If offset is passed EOF the space between EOF and offset will be filled by 0x0.
    * @return True if file contect successfully written. False on any error.
    */
    bool writeFile (uint32 fileID, const uint8* data, uint32 len, uint64 offset = 0);

    /**
    * Low level method used to close a file opened by using openFile() method.
    * @param fileID File ID returned by openFile() method.
    * @return True on success. False on any error.
    */
    bool closeFile (uint32 fileID);

    /**
    * This method is used to retrieve remote file attributes and place them into fileAttrs structure.
    * @param attrs Reference to fileAttrs structure where retrieved attributes should be placed.
    * @param filename Name of the remote file.
    * @param followSymLinks If this variable is set to true, symbolic links will be followed. That is the default befavour. If this behavour is undesired, pass "false".
    * @return True if the attributes successfully retrieved. Otherwise false is returned.
    */
    bool getFileAttrs (fileAttrs& attrs, const char* filename, bool followSymLinks = true);


    /**
    * This method is used to retrieve a remote file and dump it into local file.
    * @param remoteFile Full or relative path to the file on the remote side.
    * @param localFile Pointer to the FILE structure. If the file being retrieved is binary, use "w+" attributes in fopen function.
    * @return True if getting the file is succeeds. False on any error.
    */
    bool get (const char* remoteFile, FILE* localFile);

    /**
    * This method is used to upload a file to a remote server.
    * @param localFile Pointer to the FILE structure. If the file being retrieved is binary, use "r+" attributes in fopen function.
    * @param remoteFile Full or relative path to the file on the remote side.
    * @return True if putting the file succeeds. False on any error.
    */
    bool put (FILE* localFile, const char* remoteFile);

    /**
    * This method is used to remove a file on a remote server.
    * @param remoteFile Full or relative path to the file on the remote side.
    * @return True if remove succeeds. False on any error.
    */
    bool rm (const char* remoteFile);

    /**
    * This method is used to rename/move files.
    * @param oldFile Full or relative path to an old file on the remote server.
    * @param newFile Full or relative path to a new file on the remote side.
    * @return True if renaming successfull. False on any error.
    */
    bool mv (const char* oldFile, const char* newFile);

    /**
    * This method is used to create a new directory.
    * @param remoteDir Full or relative path to a new directory on the remote server.
    * @return True if the directory successfully created. False on any error.
    */
    bool mkdir (const char* remoteDir);

    /**
    * This method is used to remove a remote directory.
    * @param remoteDir Full or relative path to a directory to be removed.
    * @return True if the directory successfully removed. False on any error.
    */
    bool rmdir (const char* remoteDir);

    /**
    * This methods is used retrieve a listing of a remote directory.
    * @param remoteDir Full or relative path to a directory.
    * @param longNames If set to "true" the returned string in addition to file strings will contain attributes for each file.
    * @return A pointer to a string containing the directory listing.
    */
    const char* ls (const char* remoteDir, bool longNames=false);

    /**
    * This method is used to change the current working directory.
    * @param remoteDir Full or relative path to the new working directory on the remote server.
    * @return True if change of directory succedded. False on any error.
    */
    bool cd (const char* remoteDir);

    /**
    * This method is used for changing the permissions associated with a remote file.
    * @param remoteFile Full or relative path to the remote file.
    * @param mode Mode string. It can be either a numerical mode expression such as "755" or an expression showing the modifications to be made, such as "ug+w". Mode string is the same as used by *nix chmod command.
    * @return True if the new permissions are succesfully applied to the remote file. False on any error.
    */
    bool chmod (const char* remoteFile, const char* mode);

    /**
    * This method is used to change the owner of a remote file.
    * @param remoteFile Full or relative path to the remote file.
    * @param uid Numerical new owner user ID.
    * @param gid Numerical new owner group ID.
    * @return True if the change of ownership succeeds. False on any error.
    */
    bool chown (const char* remoteFile, uint32_t uid, uint32_t gid = 0);

    /**
    * This method is used to determine if a remote inode is a regular file.
    * @param remoteFile Full or relative path to the remote inode.
    * @return True if the remote inode is a regular file. Otherwise false.
    */
    bool isFile (const char* remoteFile);

    /**
    * This method is used to determine if a remote inode is a directory.
    * @param remoteFile Full or relative path to the remote file.
    * @return True if the remote inode is a directory. Otherwise false.
    */
    bool isDir (const char* remoteFile);
};

#endif
