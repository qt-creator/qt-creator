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


#ifndef NE7SSHSFTP_H
#define NE7SSHSFTP_H

#include "ne7ssh_channel.h"
#include "ne7ssh.h"

#ifdef WIN32
// 777 = 0x1ff
#ifndef __MINGW32__
#define S_IRUSR 0x100
#endif
#define S_IRGRP 0x020
#define S_IROTH 0x004
#ifndef __MINGW32__
#define S_IWUSR 0x080
#endif
#define S_IWGRP 0x010
#define S_IWOTH 0x002
#ifndef __MINGW32__
#define S_IXUSR 0x040
#endif
#define S_IXGRP 0x008
#define S_IXOTH 0x001
#define S_ISUID 0x800
#define S_ISGID 0x400
#define S_ISVTX 0x200
#ifndef __MINGW32__
#define S_IRWXU (S_IRUSR | S_IWUSR | S_IXUSR)
#endif
#define S_IRWXG (S_IRGRP | S_IWGRP | S_IXGRP)
#define S_IRWXO (S_IROTH | S_IWOTH | S_IXOTH)
#endif

#define SSH2_FXP_INIT                1
#define SSH2_FXP_VERSION             2
#define SSH2_FXP_OPEN                3
#define SSH2_FXP_CLOSE               4
#define SSH2_FXP_READ                5
#define SSH2_FXP_WRITE               6
#define SSH2_FXP_LSTAT               7
#define SSH2_FXP_FSTAT               8
#define SSH2_FXP_SETSTAT             9
#define SSH2_FXP_FSETSTAT           10
#define SSH2_FXP_OPENDIR            11
#define SSH2_FXP_READDIR            12
#define SSH2_FXP_REMOVE             13
#define SSH2_FXP_MKDIR              14
#define SSH2_FXP_RMDIR              15
#define SSH2_FXP_REALPATH           16
#define SSH2_FXP_STAT               17
#define SSH2_FXP_RENAME             18
#define SSH2_FXP_READLINK           19
#define SSH2_FXP_LINK               21
#define SSH2_FXP_BLOCK              22
#define SSH2_FXP_UNBLOCK            23

#define SSH2_FXP_STATUS            101
#define SSH2_FXP_HANDLE            102
#define SSH2_FXP_DATA              103
#define SSH2_FXP_NAME              104
#define SSH2_FXP_ATTRS             105

#define SSH2_FXP_EXTENDED          200
#define SSH2_FXP_EXTENDED_REPLY    201

#define SFTP_VERSION 3
#define SFTP_MAX_SEQUENCE 4294967295U
#define SFTP_MAX_PACKET_SIZE (32 * 1024)
#define SFTP_MAX_MSG_SIZE (256 * 1024)

#define SSH2_FXF_READ           0x00000001
#define SSH2_FXF_WRITE          0x00000002
#define SSH2_FXF_APPEND         0x00000004
#define SSH2_FXF_CREAT          0x00000008
#define SSH2_FXF_TRUNC          0x00000010
#define SSH2_FXF_EXCL           0x00000020

#define SSH2_FILEXFER_ATTR_SIZE         0x00000001
#define SSH2_FILEXFER_ATTR_UIDGID       0x00000002
#define SSH2_FILEXFER_ATTR_ACMODTIME    0x00000008
#define SSH2_FILEXFER_ATTR_PERMISSIONS  0x00000004

class ne7ssh_session;
class ne7ssh_transport;

/**
  @author Andrew Useckas <andrew@netsieben.com>
*/
class Ne7sshSftp : public ne7ssh_channel
{
  private:
    ne7ssh_session* session;
    uint32 timeout;
    uint32 seq;
    uint8 sftpCmd;
    ne7ssh_string commBuffer;
    Botan::SecureVector<Botan::byte> fileBuffer;
    enum writeMode { READ, OVERWRITE, APPEND };
    uint8 lastError;
    char* currentPath;

    /**
    * Structure used to store rmote file attributes.
    */
    typedef struct
    {
      uint32 flags;
      uint64 size;
      uint32 owner;
      uint32 group;
      uint32 permissions;
      uint32 atime;
      uint32 mtime;
    } sftpFileAttrs;

    sftpFileAttrs attrs;

    /**
    * Structure used to store open rmote file.
    */
    typedef struct 
    {
      uint32 fileID;
      uint16 handleLen;
      char handle[256];
    } sftpFile;
    sftpFile **sftpFiles;
    uint16 sftpFilesCount;

    /**
    * Replacement for ne7ssh_channel handleData method. Processes SFTP specific packets.
    * @param packet Reference to the newly received packet.
    * @return True if data successfully processed. False on any error.
    */
    bool handleData (Botan::SecureVector<Botan::byte>& packet);

    /**
    * Processes the VERSION packet received from the server.
    * @param packet VERSION packet.
    * @return True if processing successful, otherwise false.
    */
    bool handleVersion (Botan::SecureVector<Botan::byte>& packet);

    /**
    * Processes the STATUS packet received from the server.
    * @param packet STATUS packet.
    * @return True if processing successful, otherwise false.
    */
    bool handleStatus (Botan::SecureVector<Botan::byte>& packet);

    /**
    * Method to add a new file to sftpFiles variable from the HANDLE packet.
    * @param packet HANDLE packet.
    * @return True if processing successful, otherwise false.
    */
    bool addOpenHandle (Botan::SecureVector<Botan::byte>& packet);

    /**
    * Method to process DATA packets.
    * @param packet DATA packet.
    * @return True if processing successful, otherwise false.
    */
    bool handleSftpData (Botan::SecureVector<Botan::byte>& packet);

    /**
    * Method to process NAME packets.
    * @param packet NAME packet.
    * @return True if processing successful, otherwise false.
    */
    bool handleNames (Botan::SecureVector<Botan::byte>& packet);

    /**
    * This method is used to get a pointer to currently open file stored in sftpFile structure.
    * @param fileID File ID, received from fileOpen() method.
    * @return Returns a pointer to sftpFile structyure containing opened file or directory. If file specified by fileID has not been opened, NULL is returned.
    */
    sftpFile* getFileHandle (uint32 fileID);

    /**
    * Receive packets until specific SFTP subsystem command is received.
    * @param _cmd SFTP command to wait for.
    * @param timeSec Timeout in seconds.
    * @return True if the command specified has been received. Otherwise false.
    */
    bool receiveUntil (short _cmd, uint32 timeSec = 0);

    /**
    * Receive packets while SFTP subsystem commands received matches specified command.
    * @param _cmd Command to receive.
    * @param timeSec Timeout in seconds.
    * @return True if all expected data received. Otherwise false.
    */
    bool receiveWhile (short _cmd, uint32 timeSec = 0);

    /**
    * Method to process ATTRS packet.
    * @param packet ATTRS packet.
    * @return True if processing successful, otherwise false.
    */
    bool processAttrs (Botan::SecureVector<Botan::byte>& packet);

    /**
    * Low level method to request file attributes.
    * @param remoteFile Full or relative path to a remote file.
    * @param followSymLinks If set to true symbolic links will be followed. That is the default behavior. If following symbolic links is undesired set to "false".
    * @return True if the request succeeds. False on any error.
    */
    bool getFileStats (const char* remoteFile, bool followSymLinks = true);

    /**
    * Gets attributes of a remote file and dumps them into sfptFileAtts structure.
    * @param attributes reference to sftpFileAttrs structure where the result will be stored.
    * @param remoteFile Full or relative path to a remote file.
    * @param followSymLinks If set to true symbolic links will be followed. That is the default behavior. If following symbolic links is undesired set to "false".
    * @return True if the attributes successfully received. False on any error.
    */
    bool getFileAttrs (sftpFileAttrs& attributes, Botan::SecureVector<Botan::byte>& remoteFile,  bool followSymLinks = true);

    /**
    * Works like getFileStats() method, except that it operates on a handle of already opened file instead of path.
    * @param fileID File ID, returned by the openFile() method.
    * @return True if the request succeeds. False on any error.
    */
    bool getFStat (uint32 fileID);

    /**
    * This methods returnes the size of an open file.
    * @param fileID File ID, returned by the openFile() method.
    * @return The size of the remote file. False on any error.
    */
    uint64 getFileSize (uint32 fileID);

    /**
    * This method is used to wait for an ADJUST_WINDOW packet, when the send window size is zero.
    * @return True if the ADJUST_WINDOW packet has been received, otherwise false.
    */
    bool receiveWindowAdjust ();

    /**
    * Returns full path to a file or directory.
    * @param filename Relative path to a remote file or directory.
    * @return ne7ssh_string class containing full path to the remote file or directory. The class will contain an empty string on error.
    */
    ne7ssh_string getFullPath (const char* filename);

    /**
    * Determines the type of a remote file.
    * @param remoteFile Relative or full path to the remote file.
    * @param type Type, taken from sys/stat.h.
    * @return True if file is of specified type. Otherwise false.
    */
    bool isType (const char* remoteFile, uint32 type);

  public:
    /**
    * Constructor.
    * @param _session Pointer to connections session data.
    * @param _channel Pointer to the ne7ssh_channel instance, taken from the new ne7ssh_connection instance.
    */
    Ne7sshSftp(ne7ssh_session* _session, ne7ssh_channel* _channel);

    /**
    * Default destructor.
    */
    ~Ne7sshSftp();

    /**
    * Initializes SFTP subsystem.
    * @return True if the subsystem successfully initialized. False on any error.
    */
    bool init();

    /**
    * This method is used to set a timeout for all SFTP subsystem communications.
    * @param _timeout Timeout in seconds.
    */
    void setTimeout (uint32 _timeout) { timeout = _timeout; }

    /**
    * Low level method used to open a remote file.
    * @param filename Relative or full path to the file.
    * @param shortMode Mode to be used when opening the file. Can be one of the modes defined by writeMode class variable.
    * @return Newly opened file ID or 0 if file could not be opened.
    */
    uint32 openFile (const char* filename, uint8 shortMode);

    /**
    * Low level method used to open an inode containing file entries a.k.a directory.
    * @param dirname Relative or full path to the inode.
    * @return Newly opened file ID or 0 if the inode could not be opened.
    */
    uint32 openDir (const char* dirname);

    /**
    * Low level method used to read datablock up to the size of SFTP_MAX_MSG_SIZE from a file.
    * @param fileID File ID retruned by openFile() method.
    * @param offset Offset. 0 by default.
    * @return True if file content successfully read and placed in the buffer. Otherwise false.
    */
    bool readFile (uint32 fileID, uint64 offset = 0);

    /**
    * Low level method used to write data-block up to the size of SFTP_MAX_MSG_SIZE to a remote file.
    * @param fileID File ID returned by openFile() method.
    * @param data Pointer to a buffer containing the data.
    * @param len Length of the block.
    * @param offset Offset in the remote file. If offset is passed EOF the space between EOF and offset will be filled by 0x0. Variable is set to 0 by default.
    * @return True if file contect successfully written. False on any error.
    */
    bool writeFile (uint32 fileID, const uint8* data, uint32 len, uint64 offset = 0);

    /**
    * Low level method used to close a file opened using openFile() method.
    * @param fileID File ID returned by openFile() method.
    * @return True on success. False on any error.
    */
    bool closeFile (uint32 fileID);

    /**
    * This method is used to retrieve remote file attributes and place them into fileAttrs structure.
    * @param attributes Reference to fileAttrs structure where retrieved attributes should be placed.
    * @param remoteFile Name of the remote file.
    * @param followSymLinks If this variable is set to true, symbolic links will be followed. That is the default befavour. If this behavour is undesired, pass "false".
    * @return True if the attributes successfully retrieved. Otherwise false is returned.
    */
    bool getFileAttrs (Ne7SftpSubsystem::fileAttrs& attributes, const char* remoteFile,  bool followSymLinks = true);

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


    /**
    * This method is used to retrieve a remote file and dump it into local file. 
    * @param remoteFile Full or relative path to the file on the remote side.
    * @param localFile Pointer to the FILE structure. If the file being retrieved is binary, use "w+" attributes in fopen function.
    * @return True if getting the file is succeeds. False on any error.
    */
    bool get (const char* remoteFile, FILE* localFile);

    /**
    * This method is used to upload a file to a remote server.
    * @param localFile Pointer to the FILE structure. If the file being retrieved is binary, use "w+" attributes in fopen function.
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
    * @param mode Mode string. It can be wither a numerical mode expression such as "755" or an expression showing the modifications to be made, such as "ug+w". Mode string is the same as used by *nix chmod command.
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
    bool chown (const char* remoteFile, uint32 uid, uint32 gid = 0);
};

#endif
