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


#include <sys/stat.h>
#include <cstdio>
#include "ne7ssh_transport.h"
#include "ne7ssh_sftp.h"
#include "ne7ssh_sftp_packet.h"
#include "ne7ssh.h"
#include "ne7ssh_session.h"
#include "ne7ssh_channel.h"

using namespace Botan;

Ne7sshSftp::Ne7sshSftp (ne7ssh_session* _session, ne7ssh_channel* _channel) : ne7ssh_channel (_session), session(_session), timeout(30), seq(1), sftpCmd(0), lastError(0), currentPath(0), sftpFiles(0), sftpFilesCount(0)
{
  windowRecv = _channel->getRecvWindow();
  windowSend = _channel->getSendWindow();
}

Ne7sshSftp::~Ne7sshSftp()
{
  uint16 i;
  for (i = 0; i < sftpFilesCount; i++)
  {
    free (sftpFiles[i]);
  }
  if (sftpFiles) free (sftpFiles);
  if (currentPath) free (currentPath);
}

bool Ne7sshSftp::init ()
{
  ne7ssh_transport *_transport = session->transport;
  ne7ssh_string packet;
  bool status;

  packet.clear();
  packet.addChar (SSH2_MSG_CHANNEL_REQUEST);
  packet.addInt (session->getSendChannel());
  packet.addString ("subsystem");
  packet.addChar (0);
  packet.addString ("sftp");

  if (!_transport->sendPacket (packet.value()))
  {
    return false;
  }

  packet.clear();
  packet.addChar (SSH2_MSG_CHANNEL_DATA);
  packet.addInt (session->getSendChannel());
  packet.addInt (sizeof(uint32) * 2 + sizeof(char));
  packet.addInt (sizeof(uint32) + sizeof(char));
  packet.addChar (SSH2_FXP_INIT);
  packet.addInt (SFTP_VERSION);

  windowSend -= 9;

  if (!_transport->sendPacket (packet.value())) 
    return false;

  channelOpened = true;
  status = receiveUntil (SSH2_FXP_VERSION, this->timeout);

  return status;
    
}

bool Ne7sshSftp::handleData (Botan::SecureVector<Botan::byte>& packet)
{
  ne7ssh_string mainBuffer (packet, 0);
  uint32 channelID;
  SecureVector<Botan::byte> sftpBuffer;
  uint32 len = 0;
  Botan::byte _cmd;

  channelID = mainBuffer.getInt();

  if (!mainBuffer.getString (sftpBuffer)) return false;
  if (!sftpBuffer.size())
    ne7ssh::errors()->push (session->getSshChannel(), "Abnormal. End of stream detected in SFTP subsystem.");

  adjustRecvWindow (sftpBuffer.size());

  if (seq >= SFTP_MAX_SEQUENCE) seq = 0;

  mainBuffer.clear();

  len = commBuffer.length();

  if (len) mainBuffer.addVector (commBuffer.value());

  commBuffer.addVector (sftpBuffer);
  mainBuffer.addVector (sftpBuffer);

  len = mainBuffer.getInt();

  if (len > mainBuffer.length()) return true;
  else commBuffer.clear();

  _cmd = mainBuffer.getByte();
  
  this->sftpCmd = _cmd;
  switch (_cmd)
  {
    case SSH2_FXP_VERSION:
      return handleVersion (mainBuffer.value());
      break;

    case SSH2_FXP_HANDLE:
      return addOpenHandle (mainBuffer.value());

    case SSH2_FXP_STATUS:
      return handleStatus (mainBuffer.value());

    case SSH2_FXP_DATA:
      return handleSftpData (mainBuffer.value());
    
    case SSH2_FXP_NAME:
      return handleNames (mainBuffer.value());

    case SSH2_FXP_ATTRS:
      return processAttrs (mainBuffer.value());

    default:
      ne7ssh::errors()->push (session->getSshChannel(), "Unhandled SFTP subsystem command: %i.", _cmd);
      return false;
  }
    
  return true;
}

bool Ne7sshSftp::receiveWindowAdjust ()
{
  ne7ssh_transport *_transport = session->transport;
  SecureVector<Botan::byte> packet;

  if (!_transport->waitForPacket (SSH2_MSG_CHANNEL_WINDOW_ADJUST))
  {
    ne7ssh::errors()->push (session->getSshChannel(), "Remote side could not adjust the Window.");
    return false;
  }
  _transport->getPacket (packet);
  if (!handleReceived (packet)) return false;
  return true;
}

bool Ne7sshSftp::receiveUntil (short _cmd, uint32 timeSec)
{
  ne7ssh_transport *_transport = session->transport;
  SecureVector<Botan::byte> packet;
  uint32 cutoff = timeSec * 1000000, timeout = 0;
  uint32 prevSize = 0;
  bool status;

  this->sftpCmd = 0;
  commBuffer.clear();
  
  while (true)
  {
    status = _transport->waitForPacket (0, false);
    if (status)
    {
      _transport->getPacket (packet);
      if (!handleReceived (packet)) return false;
    }

    if (commBuffer.length() > prevSize) timeout = 0;

    prevSize = commBuffer.length();

    usleep (10000);
    
    if (sftpCmd == _cmd) return true;
    if (!cutoff) continue;
     if (timeout >= cutoff) break;
    else timeout += 10000;
  }
  return false;
}

bool Ne7sshSftp::receiveWhile (short _cmd, uint32 timeSec)
{
  ne7ssh_transport *_transport = session->transport;
  SecureVector<Botan::byte> packet;
  uint32 cutoff = timeSec * 1000000, timeout = 0;
  uint32 prevSize = 0;
  bool status;

  this->sftpCmd = _cmd;
  commBuffer.clear();
  
  while (true)
  {
    status = _transport->waitForPacket (0, false);
    if (status)
    {
      _transport->getPacket (packet);
      if (!handleReceived (packet)) return false;
    }

    if (commBuffer.length() > prevSize) timeout = 0;
    if (commBuffer.length() == 0) return true;

    prevSize = commBuffer.length();

    usleep (10000);
    
    if (sftpCmd != _cmd) return true;

    if (!cutoff) continue;
     if (timeout >= cutoff) break;
    else timeout += 10000;
  }
  return false;
}

bool Ne7sshSftp::handleVersion (Botan::SecureVector<Botan::byte>& packet)
{
  ne7ssh_string sftpBuffer (packet, 0);
  uint32 version;

  version = sftpBuffer.getInt();

  if (version != SFTP_VERSION)
  {
    ne7ssh::errors()->push (session->getSshChannel(), "Unsupported SFTP version: %i.", version);
    return false;
  }

  return true;

}

bool Ne7sshSftp::handleStatus (Botan::SecureVector<Botan::byte>& packet)
{
  ne7ssh_string sftpBuffer (packet, 0);
  uint32 requestID, errorID;
  SecureVector<Botan::byte> errorStr;
  

  requestID = sftpBuffer.getInt();
  errorID = sftpBuffer.getInt();
  sftpBuffer.getString (errorStr);
  
  if (errorID)
  {
    lastError = errorID;
    ne7ssh::errors()->push (session->getSshChannel(), "SFTP Error code: <%i>, description: %s.", errorID, errorStr.begin());
    return false;
  }
  return true;

}

bool Ne7sshSftp::addOpenHandle (Botan::SecureVector<Botan::byte>& packet)
{
  ne7ssh_string sftpBuffer (packet, 0);
  uint32 requestID;
  SecureVector<Botan::byte> handle;
  uint16 len;

  requestID = sftpBuffer.getInt();
  sftpBuffer.getString (handle);

  if (!this->sftpFiles) this->sftpFiles = (sftpFile**) malloc (sizeof(sftpFile*));
  else this->sftpFiles = (sftpFile**) realloc (sftpFiles, sizeof(sftpFile*) * (this->sftpFilesCount + 1));

  sftpFiles[sftpFilesCount] = (sftpFile*) malloc (sizeof(sftpFile));
  sftpFiles[sftpFilesCount]->fileID = requestID;
  len = handle.size();
  if (len > 256) len = 256;
  memcpy (sftpFiles[sftpFilesCount]->handle, handle.begin(), len);
  sftpFiles[sftpFilesCount]->handleLen = len;
  sftpFilesCount++;
  return true; 
  
}

bool Ne7sshSftp::handleSftpData (Botan::SecureVector<Botan::byte>& packet)
{
  ne7ssh_string sftpBuffer (packet, 0);
  uint32 requestID;
  SecureVector<Botan::byte> data;
  uint16 len;

  requestID = sftpBuffer.getInt();
  sftpBuffer.getString (data);
  len = data.size();

  if (data.size() == 0)
  {
    ne7ssh::errors()->push (session->getSshChannel(), "Abnormal. End of stream detected.");
    return false;
  }

  commBuffer.clear();
  fileBuffer.destroy();
    fileBuffer.swap (data);
  return true;
}

bool Ne7sshSftp::handleNames (Botan::SecureVector<Botan::byte>& packet)
{
  Ne7sshSftpPacket sftpBuffer (packet, 0);
  ne7ssh_string tmpVar;
  uint32 requestID, fileCount, i;
  SecureVector<Botan::byte> fileName;

  requestID = sftpBuffer.getInt();
  fileCount = sftpBuffer.getInt();
  tmpVar.addInt (fileCount);

  if (!fileCount) return true;

  for (i = 0; i < fileCount; i++)
  {
    sftpBuffer.getString (fileName);
    tmpVar.addVectorField (fileName);
    sftpBuffer.getString (fileName);
    tmpVar.addVectorField (fileName);
    attrs.flags = sftpBuffer.getInt();
    if (attrs.flags & SSH2_FILEXFER_ATTR_SIZE)
      attrs.size = sftpBuffer.getInt64();

    if (attrs.flags & SSH2_FILEXFER_ATTR_UIDGID)
    {
      attrs.owner = sftpBuffer.getInt();
      attrs.group = sftpBuffer.getInt();
    }

    if (attrs.flags & SSH2_FILEXFER_ATTR_PERMISSIONS)
    {
      attrs.permissions = sftpBuffer.getInt();
    }

    if (attrs.flags & SSH2_FILEXFER_ATTR_ACMODTIME)
    {
      attrs.atime = sftpBuffer.getInt();
      attrs.mtime = sftpBuffer.getInt();
    }
  }
  fileBuffer.append(tmpVar.value());

  return true;
}

bool Ne7sshSftp::processAttrs (Botan::SecureVector<Botan::byte>& packet)
{
  Ne7sshSftpPacket sftpBuffer (packet, 0);
  uint32 requestID;
  SecureVector<Botan::byte> data;

  requestID = sftpBuffer.getInt();
  attrs.flags = sftpBuffer.getInt();
  if (attrs.flags & SSH2_FILEXFER_ATTR_SIZE)
    attrs.size = sftpBuffer.getInt64();

  if (attrs.flags & SSH2_FILEXFER_ATTR_UIDGID)
  {
    attrs.owner = sftpBuffer.getInt();
    attrs.group = sftpBuffer.getInt();
  }

  if (attrs.flags & SSH2_FILEXFER_ATTR_PERMISSIONS)
  {
    attrs.permissions = sftpBuffer.getInt();
  }

  if (attrs.flags & SSH2_FILEXFER_ATTR_ACMODTIME)
  {
    attrs.atime = sftpBuffer.getInt();
    attrs.mtime = sftpBuffer.getInt();
  }

  return true;
}


uint32 Ne7sshSftp::openFile (const char* filename, uint8 shortMode)
{
  uint32 mode;
  Ne7sshSftpPacket packet (session->getSendChannel());
  ne7ssh_transport *_transport = session->transport;
  bool status;
  ne7ssh_string fullPath;

   fullPath = getFullPath (filename);

  if (!fullPath.length()) return 0;

  switch (shortMode)
  {
    case READ:
      mode = SSH2_FXF_READ;
      break;

    case OVERWRITE:
      mode = SSH2_FXF_WRITE | SSH2_FXF_CREAT | SSH2_FXF_TRUNC;
      break;

    case APPEND:
      mode = SSH2_FXF_WRITE | SSH2_FXF_CREAT;
      break;

    default:
      ne7ssh::errors()->push (session->getSshChannel(), "Unsupported file opening mode: %i.", shortMode);
      return 0;

  }

  packet.addChar (SSH2_FXP_OPEN);
  packet.addInt (this->seq++);
  packet.addVectorField (fullPath.value());
  packet.addInt (mode);
  packet.addInt (0);

  if (!packet.isChannelSet())
  {
    ne7ssh::errors()->push (session->getSshChannel(), "Channel not set in sftp packet class.");
    return 0;
  }

  if (!_transport->sendPacket (packet.value())) 
    return 0;

  windowSend -= 21 + fullPath.length();

  status = receiveUntil (SSH2_FXP_HANDLE, this->timeout);

  if (!status) return 0;
  else return (seq - 1);  
}

uint32 Ne7sshSftp::openDir (const char* dirname)
{
  Ne7sshSftpPacket packet (session->getSendChannel());
  ne7ssh_transport *_transport = session->transport;
  bool status;
  ne7ssh_string fullPath = getFullPath (dirname);

  if (!fullPath.length()) return 0;

  packet.addChar (SSH2_FXP_OPENDIR);
  packet.addInt (this->seq++);
  packet.addVectorField (fullPath.value());

  if (!packet.isChannelSet())
  {
    ne7ssh::errors()->push (session->getSshChannel(), "Channel not set in sftp packet class.");
    return 0;
  }

  if (!_transport->sendPacket (packet.value())) 
    return 0;

  windowSend -= 13 + fullPath.length();

  status = receiveUntil (SSH2_FXP_HANDLE, this->timeout);

  if (!status) return 0;
  else return (seq - 1);  
}

bool Ne7sshSftp::readFile (uint32 fileID, uint64 offset)
{
  Ne7sshSftpPacket packet (session->getSendChannel());
  ne7ssh_transport *_transport = session->transport;
  bool status;
  sftpFile *remoteFile = getFileHandle (fileID);

  if (!remoteFile) return false;

  packet.addChar (SSH2_FXP_READ);
  packet.addInt (this->seq++);
  packet.addInt (remoteFile->handleLen);
  packet.addBytes ((Botan::byte*)remoteFile->handle, remoteFile->handleLen);
  packet.addInt64 (offset);
  packet.addInt (SFTP_MAX_MSG_SIZE);
    
  if (!packet.isChannelSet())
  {
    ne7ssh::errors()->push (session->getSshChannel(), "Channel not set in sftp packet class.");
    return 0;
  }

  if (!_transport->sendPacket (packet.value())) 
    return false;

  windowSend -= remoteFile->handleLen + 25;

  status = receiveWhile (SSH2_FXP_DATA, this->timeout);

  return status;
}

bool Ne7sshSftp::writeFile (uint32 fileID, const uint8* data, uint32 len, uint64 offset)
{
  Ne7sshSftpPacket packet (session->getSendChannel());
  ne7ssh_transport *_transport = session->transport;
  bool status;
  sftpFile *remoteFile = getFileHandle (fileID);
  uint32 sent = 0, currentLen = 0;
  Botan::SecureVector<Botan::byte> sendVector;

  if (len > SFTP_MAX_MSG_SIZE)
  {
    ne7ssh::errors()->push (session->getSshChannel(), "Could not write. Datablock larger than maximum msg size. Remote file ID %i.", fileID);
    return false;
  }

  if (!remoteFile) return false;

  packet.addChar (SSH2_FXP_WRITE);
  packet.addInt (this->seq++);
  packet.addInt (remoteFile->handleLen);
  packet.addBytes ((Botan::byte*)remoteFile->handle, remoteFile->handleLen);
  packet.addInt64 (offset);
  packet.addInt (len);

  if (!packet.isChannelSet())
  {
    ne7ssh::errors()->push (session->getSshChannel(), "Channel not set in sftp packet class.");
    return false;
  }
  windowSend -= remoteFile->handleLen + 25;

  while (sent < len)
  {
    currentLen = len - sent < windowSend ? len - sent : windowSend;
    currentLen = currentLen < (uint32)(SFTP_MAX_PACKET_SIZE - (remoteFile->handleLen + 86)) ? currentLen : SFTP_MAX_PACKET_SIZE - (remoteFile->handleLen + 86);

    if (sent) packet.clear();
    packet.addBytes (data + sent, currentLen);
    
    if (sent)
      sendVector = packet.valueFragment();
    else 
      sendVector = packet.valueFragment(remoteFile->handleLen + 21 + len);

    if (!sendVector.size()) return false;

    status = _transport->sendPacket (sendVector);
    if (!status) return false;

    windowSend -= currentLen;
    sent += currentLen;
    if (!windowSend)
    {
      if (!receiveWindowAdjust())
      {
        ne7ssh::errors()->push (session->getSshChannel(), "Remote side could not adjust the Window.");
        return false;
      }
    }
//    if (sent - currentLen) break;
  }
  status = receiveUntil (SSH2_FXP_STATUS, this->timeout);
  return status;
}

bool Ne7sshSftp::closeFile (uint32 fileID)
{
  Ne7sshSftpPacket packet (session->getSendChannel());
  ne7ssh_transport *_transport = session->transport;
  uint16 i;
  bool status, match=false;
  sftpFile *remoteFile = getFileHandle (fileID);
  
  if (!remoteFile) return false;

  packet.addChar (SSH2_FXP_CLOSE);
  packet.addInt (this->seq++);
  packet.addInt (remoteFile->handleLen);
  packet.addBytes ((Botan::byte*)remoteFile->handle, remoteFile->handleLen);
    
  if (!packet.isChannelSet())
  {
    ne7ssh::errors()->push (session->getSshChannel(), "Channel not set in sftp packet class.");
    return 0;
  }

  if (!_transport->sendPacket (packet.value())) 
    return false;
  
  windowSend -= remoteFile->handleLen + 13;

  for (i = 0; i < sftpFilesCount; i++)
  {
    if (match) sftpFiles[i - 1] = sftpFiles[i];
    else if (sftpFiles[i]->fileID == fileID)
    {
      free (sftpFiles[i]);
      sftpFiles[i] = 0;
      match = true;
    }
  }
  if (match) sftpFilesCount--;

  status = receiveUntil (SSH2_FXP_STATUS, this->timeout);
  return status;
}

ne7ssh_string Ne7sshSftp::getFullPath (const char* filename)
{
  Botan::SecureVector<Botan::byte> result;
  char *buffer = 0;
  uint32 len, pos, last_char, i = 0;
  
  if (!filename) return ne7ssh_string();
  len = strlen (filename);

  buffer = (char*) malloc (len + 1);
  memcpy (buffer, filename, len);

  while (isspace (buffer[i])) i++;

  for (pos = 0; i < len; i++)
  {
    if (buffer[i] == '\\')
      buffer[pos] = '/';
    else buffer[pos] = buffer[i];
    pos++;
  }
  pos--;
  while (isspace (buffer[pos])) pos--;
  if (pos > 1 && buffer[pos] == '.' && buffer[pos-1] != '.') pos--;
  else if (!pos && buffer[pos] == '.') buffer[pos] = 0;

  result.destroy();
  if ((buffer[0] != '/') && currentPath)
  {
    if (currentPath) len = strlen (this->currentPath);
    else 
    {
      free (buffer);
      return ne7ssh_string();
    }
    result.append ((uint8*)currentPath, len);
    last_char = len - 1;
    if (currentPath[last_char] && currentPath[last_char] != '/')
      result.append ((uint8*)"/", 1);
  }
  while (buffer[pos] == '/') pos--;
  buffer[++pos] = 0x00;
  result.append ((uint8*)buffer, pos);
  free (buffer);
  return ne7ssh_string(result, 0);
}

Ne7sshSftp::sftpFile* Ne7sshSftp::getFileHandle (uint32 fileID)
{
  uint16 i;
  uint32 _fileID;

  for (i = 0; i < sftpFilesCount; i++)
  {
    _fileID = sftpFiles[i]->fileID;
    if (sftpFiles[i]->fileID == fileID)
    {
      return sftpFiles[i];
    }
  }
  ne7ssh::errors()->push (session->getSshChannel(), "Invalid file ID: %i.", fileID);
  return 0;
}

bool Ne7sshSftp::getFileStats (const char* remoteFile, bool followSymLinks)
{
  Ne7sshSftpPacket packet (session->getSendChannel());
  ne7ssh_transport *_transport = session->transport;
  bool status;
  uint8 _cmd = followSymLinks ? SSH2_FXP_STAT : SSH2_FXP_LSTAT;
  ne7ssh_string fullPath = getFullPath (remoteFile);

  if (!fullPath.length()) return 0;

  if (!remoteFile) return false;

  packet.addChar (_cmd);
  packet.addInt (this->seq++);
  packet.addVectorField (fullPath.value());
  packet.addInt (SSH2_FILEXFER_ATTR_SIZE | SSH2_FILEXFER_ATTR_UIDGID | SSH2_FILEXFER_ATTR_PERMISSIONS | SSH2_FILEXFER_ATTR_ACMODTIME);
    
  if (!packet.isChannelSet())
  {
    ne7ssh::errors()->push (session->getSshChannel(), "Channel not set in sftp packet class.");
    return 0;
  }

  if (!_transport->sendPacket (packet.value())) 
    return false;

  windowSend -= 17 + fullPath.length();

  status = receiveWhile (SSH2_FXP_ATTRS, this->timeout);
  return status;
}

bool Ne7sshSftp::getFStat (uint32 fileID)
{
  Ne7sshSftpPacket packet (session->getSendChannel());
  ne7ssh_transport *_transport = session->transport;
  bool status;
  sftpFile *remoteFile = getFileHandle (fileID);
  
  if (!remoteFile) return false;
  packet.addChar (SSH2_FXP_FSTAT);
  packet.addInt (this->seq++);
  packet.addInt (remoteFile->handleLen);
  packet.addBytes ((Botan::byte*)remoteFile->handle, remoteFile->handleLen);
    
  if (!packet.isChannelSet())
  {
    ne7ssh::errors()->push (session->getSshChannel(), "Channel not set in sftp packet class.");
    return 0;
  }

  if (!_transport->sendPacket (packet.value())) 
    return false;

  windowSend -= remoteFile->handleLen + 13;

  status = receiveWhile (SSH2_FXP_ATTRS, this->timeout);
  return status;
}

uint64 Ne7sshSftp::getFileSize (uint32 fileID)
{
  if (!getFStat (fileID))
  {
    ne7ssh::errors()->push (session->getSshChannel(), "Failed to get remote file attributes.");
    return 0;
  }
  
  return attrs.size;
}

bool Ne7sshSftp::getFileAttrs (Ne7SftpSubsystem::fileAttrs& attributes, const char* remoteFile,  bool followSymLinks)
{
  if (!remoteFile)
  {
    ne7ssh::errors()->push (session->getSshChannel(), "Failed to get remote file attributes.");
    return false;
  }
  ne7ssh_string fullPath = getFullPath (remoteFile);
  if (!fullPath.length()) return false;
  
  if (!getFileStats ((const char*)fullPath.value().begin()))
  {
    ne7ssh::errors()->push (session->getSshChannel(), "Failed to get remote file attributes.");
    return false;
  }

  attributes.size = attrs.size;
  attributes.owner = attrs.owner;
  attributes.group = attrs.group;
  attributes.permissions = attrs.permissions;
  attributes.atime = attrs.atime;
  attributes.mtime = attrs.mtime;

  return true;
}

bool Ne7sshSftp::getFileAttrs (sftpFileAttrs& attributes, Botan::SecureVector<Botan::byte>& remoteFile,  bool followSymLinks)
{
  if (!remoteFile.size()) return false;
  if (!getFileStats ((const char*)remoteFile.begin()))
  {
    ne7ssh::errors()->push (session->getSshChannel(), "Failed to get remote file attributes.");
    return false;
  }
  attributes.size = attrs.size;
  attributes.owner = attrs.owner;
  attributes.group = attrs.group;
  attributes.permissions = attrs.permissions;
  attributes.atime = attrs.atime;
  attributes.mtime = attrs.mtime;

  return true;
}

bool Ne7sshSftp::isType (const char* remoteFile, uint32 type)
{
  uint32 perms;
  if (!remoteFile)
  {
    ne7ssh::errors()->push (session->getSshChannel(), "Failed to get remote file attributes.");
    return false;
  }
  ne7ssh_string fullPath = getFullPath (remoteFile);
  if (!fullPath.length()) return false;
  
  if (!getFileStats ((const char*)fullPath.value().begin()))
  {
    ne7ssh::errors()->push (session->getSshChannel(), "Failed to get remote file attributes.");
    return false;
  }
  
  perms = attrs.permissions;
  if (perms & type) return true;
  else return false;
}

bool Ne7sshSftp::isFile (const char* remoteFile)
{
  return isType (remoteFile, S_IFREG);
}

bool Ne7sshSftp::isDir (const char* remoteFile)
{
  return isType (remoteFile, S_IFDIR);
}

bool Ne7sshSftp::get (const char* remoteFile, FILE* localFile)
{
  uint32 size;
  uint64 offset = 0;
  Botan::SecureVector<Botan::byte> localBuffer;
  uint32 fileID;
  
  if (!localFile)
  {
    ne7ssh::errors()->push (session->getSshChannel(), "Invalid local or remote file.");
     return false;
  }

  fileID = openFile (remoteFile, READ);
  
  if (!fileID) return false;
    
  size = getFileSize (fileID);

  if (!size)
  {
    ne7ssh::errors()->push (session->getSshChannel(), "File size is zero.");
     return false;
  }

  while (size > offset)
  {
    readFile (fileID, offset);
    if (fileBuffer.size() == 0) return false;
    localBuffer.destroy();
    localBuffer.swap (fileBuffer);

        if (!fwrite (localBuffer.begin(), (size_t) localBuffer.size(), 1, localFile))
    {
      ne7ssh::errors()->push (session->getSshChannel(), "Could not write to local file. Remote file ID %i.", fileID);
      return false;
    }
    offset += localBuffer.size();
  }
  
  if (!closeFile (fileID)) return false;
  return true;

}

bool Ne7sshSftp::put (FILE* localFile, const char* remoteFile)
{
  size_t size;
  uint64 offset = 0;
  Botan::SecureVector<Botan::byte> localBuffer;
  uint32 fileID;
  uint8* buffer = 0;
  uint32 len;
  
  if (!localFile || !remoteFile)
  {
    ne7ssh::errors()->push (session->getSshChannel(), "Invalid local or remote file.");
     return false;
  }

  fileID = openFile (remoteFile, OVERWRITE);
  
  if (!fileID) return false;
    
  fseek (localFile, 0L, SEEK_END);
  size = ftell (localFile);
  rewind (localFile);

  if (!size)
  {
    ne7ssh::errors()->push (session->getSshChannel(), "File size is zero.");
     return false;
  }

  buffer = (uint8*) malloc (SFTP_MAX_MSG_SIZE);
  while (size > offset)
  {
    len = (size - offset) < SFTP_MAX_MSG_SIZE - 384 ? (size - offset) : SFTP_MAX_MSG_SIZE - 384;
    
    if (!fread (buffer, len, 1, localFile))
    {
      ne7ssh::errors()->push (session->getSshChannel(), "Could not read from local file. Remote file ID %i.", fileID);
      if (buffer) free (buffer);
      return false;
    }
    if (!writeFile (fileID, buffer, len, offset))
    {
      if (buffer) free (buffer);
      return false;
    }
    offset += len;
  }
  
  if (buffer) free (buffer);
  if (!closeFile (fileID)) return false;
  return true;

}

bool Ne7sshSftp::rm (const char* remoteFile)
{
  Ne7sshSftpPacket packet (session->getSendChannel());
  ne7ssh_transport *_transport = session->transport;
  bool status;
  if (!remoteFile) return false;
  ne7ssh_string fullPath = getFullPath (remoteFile);

  if (!fullPath.length()) return false;
  
  packet.addChar (SSH2_FXP_REMOVE);
  packet.addInt (this->seq++);
  packet.addVectorField (fullPath.value());
    
  if (!packet.isChannelSet())
  {
    ne7ssh::errors()->push (session->getSshChannel(), "Channel not set in sftp packet class.");
    return false;
  }

  if (!_transport->sendPacket (packet.value())) 
    return false;

  windowSend -= 13 + fullPath.length();

  status = receiveWhile (SSH2_FXP_STATUS, this->timeout);
  return status;
}

bool Ne7sshSftp::mv (const char* oldFile, const char* newFile)
{
  Ne7sshSftpPacket packet (session->getSendChannel());
  ne7ssh_transport *_transport = session->transport;
  bool status;
  if (!oldFile || !newFile) return false;
  ne7ssh_string oldPath = getFullPath (oldFile);
  ne7ssh_string newPath = getFullPath (newFile);

  if (!oldPath.length() || !newPath.length()) return false;

  packet.addChar (SSH2_FXP_RENAME);
  packet.addInt (this->seq++);
  packet.addVectorField (oldPath.value());
  packet.addVectorField (newPath.value());
    
  if (!packet.isChannelSet())
  {
    ne7ssh::errors()->push (session->getSshChannel(), "Channel not set in sftp packet class.");
    return 0;
  }

  if (!_transport->sendPacket (packet.value())) 
    return false;

  windowSend -= oldPath.length() + newPath.length() + 17;

  status = receiveWhile (SSH2_FXP_STATUS, this->timeout);
  return status;
}

bool Ne7sshSftp::mkdir (const char* remoteDir)
{
  Ne7sshSftpPacket packet (session->getSendChannel());
  ne7ssh_transport *_transport = session->transport;
  bool status;
  if (!remoteDir) return false;
  ne7ssh_string fullPath = getFullPath (remoteDir);

  if (!fullPath.length()) return false;

  packet.addChar (SSH2_FXP_MKDIR);
  packet.addInt (this->seq++);
  packet.addVectorField (fullPath.value());
  packet.addInt (0);
    
  if (!packet.isChannelSet())
  {
    ne7ssh::errors()->push (session->getSshChannel(), "Channel not set in sftp packet class.");
    return 0;
  }

  if (!_transport->sendPacket (packet.value())) 
    return false;

  windowSend -= fullPath.length() + 17;

  status = receiveWhile (SSH2_FXP_STATUS, this->timeout);
  return status;
}

bool Ne7sshSftp::rmdir (const char* remoteDir)
{
  Ne7sshSftpPacket packet (session->getSendChannel());
  ne7ssh_transport *_transport = session->transport;
  bool status;
  if (!remoteDir) return false;
  ne7ssh_string fullPath = getFullPath (remoteDir);

  if (!fullPath.length()) return false;

  packet.addChar (SSH2_FXP_RMDIR);
  packet.addInt (this->seq++);
  packet.addVectorField (fullPath.value());
    
  if (!packet.isChannelSet())
  {
    ne7ssh::errors()->push (session->getSshChannel(), "Channel not set in sftp packet class.");
    return 0;
  }

  if (!_transport->sendPacket (packet.value())) 
    return false;

  windowSend -= fullPath.length() + 13;

  status = receiveWhile (SSH2_FXP_STATUS, this->timeout);
  return status;
}

const char* Ne7sshSftp::ls (const char* remoteDir, bool longNames)
{
  Ne7sshSftpPacket packet (session->getSendChannel());
  ne7ssh_transport *_transport = session->transport;
  ne7ssh_string tmpVar;
  SecureVector<Botan::byte> fileName;
  bool status = true;
  uint32 fileID, fileCount, i;
  sftpFile *remoteFile;
  if (!remoteDir) return 0;

  fileID = openDir (remoteDir);
  
  if (!fileID) return 0;
  remoteFile = getFileHandle (fileID);
  if (!remoteFile) return 0;
    fileBuffer.destroy();

  while (status)
  {
    packet.clear();
    packet.addChar (SSH2_FXP_READDIR);
    packet.addInt (this->seq++);
    packet.addInt (remoteFile->handleLen);
    packet.addBytes ((Botan::byte*)remoteFile->handle, remoteFile->handleLen);
    
    if (!packet.isChannelSet())
    {
      ne7ssh::errors()->push (session->getSshChannel(), "Channel not set in sftp packet class.");
      return 0;
    }

    if (!_transport->sendPacket (packet.value())) 
      return 0;

    windowSend -= remoteFile->handleLen + 13;

    status = receiveWhile (SSH2_FXP_NAME, this->timeout);
  }
  if (lastError > 1) return 0;
  
  packet.clear();
  packet.addVector (fileBuffer);
  fileCount = packet.getInt();
  tmpVar.clear();
  for (i = 0; i < fileCount; i++)
  {
    packet.getString (fileName);
    fileName.append ((const Botan::byte*)"\n", 1);
    if (!longNames)  tmpVar.addVector (fileName);

    packet.getString (fileName);
    fileName.append ((const Botan::byte*)"\n", 1);
    if (longNames)  tmpVar.addVector (fileName);
  }
  fileBuffer.swap (tmpVar.value());

  if (!closeFile (fileID)) return 0;
  return (const char*)fileBuffer.begin();
}

bool Ne7sshSftp::cd (const char* remoteDir)
{
  Ne7sshSftpPacket packet (session->getSendChannel());
  ne7ssh_transport *_transport = session->transport;
  SecureVector<Botan::byte> fileName;
  uint32 fileCount;
  bool status;
  if (!remoteDir) return false;
  ne7ssh_string fullPath = getFullPath (remoteDir);

  if (!fullPath.length()) return false;

    fileBuffer.destroy();

  packet.addChar (SSH2_FXP_REALPATH);
  packet.addInt (this->seq++);
  packet.addVectorField (fullPath.value());
    
  if (!packet.isChannelSet())
  {
    ne7ssh::errors()->push (session->getSshChannel(), "Channel not set in sftp packet class.");
    return false;
  }

  if (!_transport->sendPacket (packet.value())) 
    return false;

  windowSend -= fullPath.length() + 13;
  
  status = receiveWhile (SSH2_FXP_NAME, this->timeout);
  if (!status)
  {
    ne7ssh::errors()->push (session->getSshChannel(), "Could not change to remote directory: %s.", remoteDir);
    return false;
  }


  packet.clear();
  packet.addVector (fileBuffer);
  fileCount = packet.getInt();
  if (!fileCount) return false;
  packet.getString (fileName);

  if (!currentPath) currentPath = (char*) malloc (fileName.size() + 1);
  else currentPath = (char*) realloc (currentPath, fileName.size() + 1);
  memcpy (currentPath, fileName.begin(), fileName.size());
  currentPath[fileName.size()] = 0;
  return status;
}

bool Ne7sshSftp::chmod (const char* remoteFile, const char* mode)
{
  Ne7sshSftpPacket packet (session->getSendChannel());
  ne7ssh_transport *_transport = session->transport;
  bool status;
  if (!remoteFile) return false;
  ne7ssh_string fullPath = getFullPath (remoteFile);
  uint32 perms, len, octet = 0;
  bool u, g, o, plus;
  const char* _pos;
  uint8 i;
  char converter[] = {'0', '0', '0', '0', '0'};

  if (!fullPath.length()) return false;

  if (!getFileAttrs (attrs, fullPath.value(), true))
    return false;

  perms = attrs.permissions;

  len = strlen (mode);
  if (len < 5 && len > 2)
  {
    for (i = 0; i < len; i++)
    {
      if (!isdigit (mode[i])) break;
    }
    
    if (i != len) _pos = mode;
    else 
    {
      memcpy (converter + (5 - len), mode, len);
      octet = strtol (converter, (char**)&_pos, 8);
      if (octet > 07777)
      {
        ne7ssh::errors()->push (session->getSshChannel(), "Invalid permission octet.");
        return false;
      }
      if (len == 3) perms = (perms & ~0777) | octet;
      else perms = (perms & ~07777) | octet;
    }
  }

  _pos = mode;
  if (!octet)
  {
    while (*_pos)
    {
      if (*_pos == ',') _pos++;
      u = g = o = plus = false;
      while (*_pos && *_pos != '+' && *_pos != '-')
      {
        switch (*_pos)
        {
          case 'u':
            u = true;
            break;

          case 'g':
            g = true;
            break;

          case 'o':
            o = true;
            break;

          case 'a':
            u = g = o = true;
            break;

          default:
            ne7ssh::errors()->push (session->getSshChannel(), "Invalid mode string.");
            return false;

        }
      _pos++;
      }
  
      if (*_pos == '+') plus = true;
      _pos++;
      while (*_pos && *_pos != ',')
      {
        switch (*_pos)
        {
          case 'r':
            if (u) perms = plus ?  perms | S_IRUSR : perms ^ S_IRUSR;
            if (g) perms = plus ?  perms | S_IRGRP : perms ^ S_IRGRP;
            if (o) perms = plus ?  perms | S_IROTH : perms ^ S_IROTH;
            break;

          case 'w':
            if (u) perms = plus ?  perms | S_IWUSR : perms ^ S_IWUSR;
            if (g) perms = plus ?  perms | S_IWGRP : perms ^ S_IWGRP;
            if (o) perms = plus ?  perms | S_IWOTH : perms ^ S_IWOTH;
            break;

          case 'x':
            if (u) perms = plus ?  perms | S_IXUSR : perms ^ S_IXUSR;
            if (g) perms = plus ?  perms | S_IXGRP : perms ^ S_IXGRP;
            if (o) perms = plus ?  perms | S_IXOTH : perms ^ S_IXOTH;
            break;

          case 's':
            if (u) perms = plus ?  perms | S_ISUID : perms ^ S_ISUID;
            if (g) perms = plus ?  perms | S_ISGID : perms ^ S_ISGID;
            break;

          case 't':
            perms = plus ?  perms | S_ISVTX : perms ^ S_ISVTX;
            break;

          case 'X':
            if ((perms & 111) == 0) break;
            if (u) perms = plus ?  perms | S_IXUSR : perms ^ S_IXUSR;
            if (g) perms = plus ?  perms | S_IXGRP : perms ^ S_IXGRP;
            if (o) perms = plus ?  perms | S_IXOTH : perms ^ S_IXOTH;
            break;

          case 'u':
            if (u) perms = plus ?  perms | (perms & S_IRWXU) : perms ^ (perms & S_IRWXU);
            if (g) perms = plus ?  perms | (perms & S_IRWXU) : perms ^ (perms & S_IRWXU);
            if (o) perms = plus ?  perms | (perms & S_IRWXU) : perms ^ (perms & S_IRWXU);
            break;

          case 'g':
            if (u) perms = plus ?  perms | (perms & S_IRWXG) : perms ^ (perms & S_IRWXG);
            if (g) perms = plus ?  perms | (perms & S_IRWXG) : perms ^ (perms & S_IRWXG);
            if (o) perms = plus ?  perms | (perms & S_IRWXG) : perms ^ (perms & S_IRWXG);
            break;

          case 'o':
            if (u) perms = plus ?  perms | (perms & S_IRWXO) : perms ^ (perms & S_IRWXO);
            if (g) perms = plus ?  perms | (perms & S_IRWXO) : perms ^ (perms & S_IRWXO);
            if (o) perms = plus ?  perms | (perms & S_IRWXO) : perms ^ (perms & S_IRWXO);
            break;

          default:
            ne7ssh::errors()->push (session->getSshChannel(), "Invalid mode string.");
            return false;
        }
        _pos++;
      }
    }
  }
  

  packet.addChar (SSH2_FXP_SETSTAT);
  packet.addInt (this->seq++);
  packet.addVectorField (fullPath.value());
  packet.addInt (SSH2_FILEXFER_ATTR_PERMISSIONS);
  packet.addInt (perms);
    
  if (!packet.isChannelSet())
  {
    ne7ssh::errors()->push (session->getSshChannel(), "Channel not set in sftp packet class.");
    return 0;
  }

  if (!_transport->sendPacket (packet.value())) 
    return false;

  windowSend -= fullPath.length() + 21;

  status = receiveWhile (SSH2_FXP_STATUS, this->timeout);
  return status;
}

bool Ne7sshSftp::chown (const char* remoteFile, uint32 uid, uint32 gid)
{
  Ne7sshSftpPacket packet (session->getSendChannel());
  ne7ssh_transport *_transport = session->transport;
  bool status;
  uint32 old_uid, old_gid;
  if (!remoteFile) return false;
  ne7ssh_string fullPath = getFullPath (remoteFile);

  if (!fullPath.length()) return false;

  if (!getFileAttrs (attrs, fullPath.value(), true))
    return false;

  old_uid = attrs.owner;
  old_gid = attrs.group;

  packet.addChar (SSH2_FXP_SETSTAT);
  packet.addInt (this->seq++);
  packet.addVectorField (fullPath.value());
  packet.addInt (SSH2_FILEXFER_ATTR_UIDGID);
  if (uid) packet.addInt (uid);
  else packet.addInt (old_uid);
  if (gid) packet.addInt (gid);
  else packet.addInt (old_gid);
      
  if (!packet.isChannelSet())
  {
    ne7ssh::errors()->push (session->getSshChannel(), "Channel not set in sftp packet class.");
    return false;
  }

  if (!_transport->sendPacket (packet.value())) 
    return false;

  windowSend -= fullPath.length() + 25;

  status = receiveWhile (SSH2_FXP_STATUS, this->timeout);
  return status;
}

