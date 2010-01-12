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

#include <signal.h>
#include <string.h>
#include <time.h>
#include <botan/init.h>
#include <botan/auto_rng.h>
#include "ne7ssh_string.h"
#include "ne7ssh_connection.h"
#include "ne7ssh.h"
#include "ne7ssh_keys.h"
#include "ne7ssh_mutex.h"

#if defined(WIN32) || defined(__MINGW32__)
#   define kill(pid,signo) raise(signo)
#   define ne7ssh_thread_join(hndl,ign) WaitForSingleObject(hndl, INFINITE)
#   define ne7ssh_thread_exit(sig) ExitThread(sig)
#else
#   define ne7ssh_thread_create pthread_create
#       define ne7ssh_thread_join pthread_join
#       define ne7ssh_thread_exit pthread_exit
#endif

using namespace Botan;
using namespace std;

const char* ne7ssh::SSH_VERSION = "SSH-2.0-NetSieben_1.3.2";
Ne7sshError* ne7ssh::errs = NULL;

#if !BOTAN_PRE_18 && !BOTAN_PRE_15
RandomNumberGenerator* ne7ssh::rng = NULL;
#endif

#ifdef _DEMO_BUILD
const char* ne7ssh::MAC_ALGORITHMS = "none";
const char* ne7ssh::CIPHER_ALGORITHMS = "3des-cbc";
const char* ne7ssh::KEX_ALGORITHMS = "diffie-hellman-group1-sha1";
const char* ne7ssh::HOSTKEY_ALGORITHMS = "ssh-dss";
#else
const char* ne7ssh::MAC_ALGORITHMS = "hmac-md5,hmac-sha1,none";
const char* ne7ssh::CIPHER_ALGORITHMS = "aes256-cbc,aes192-cbc,twofish-cbc,twofish256-cbc,blowfish-cbc,3des-cbc,aes128-cbc,cast128-cbc";
const char* ne7ssh::KEX_ALGORITHMS = "diffie-hellman-group1-sha1,diffie-hellman-group14-sha1";
const char* ne7ssh::HOSTKEY_ALGORITHMS = "ssh-dss,ssh-rsa";
#endif

const char* ne7ssh::COMPRESSION_ALGORITHMS = "none";
char *ne7ssh::PREFERED_CIPHER = 0;
char *ne7ssh::PREFERED_MAC = 0;
Ne7ssh_Mutex ne7ssh::mut;
bool ne7ssh::running = false;
bool ne7ssh::selectActive = true;

class Locking_AutoSeeded_RNG : public Botan::RandomNumberGenerator
{
  public:
    Locking_AutoSeeded_RNG() { rng = new Botan::AutoSeeded_RNG(); }
    ~Locking_AutoSeeded_RNG() { delete rng; }
    
    void randomize(Botan::byte output[], u32bit length)
    {
      mutex.lock();
      rng->randomize(output, length);
      mutex.unlock();
    }
    
    void clear() throw()
    {
      mutex.lock();
      rng->clear();
      mutex.unlock();
    }
    
    std::string name() const { return rng->name(); }
    
    void reseed(u32bit bits_to_collect)
    {
      mutex.lock();
      rng->reseed(bits_to_collect);
      mutex.unlock();
    }
    
    void add_entropy_source(EntropySource* source)
    {
      mutex.lock();
      rng->add_entropy_source(source);
      mutex.unlock();
    }

    void add_entropy(const Botan::byte in[], u32bit length)
    {
      mutex.lock();
      rng->add_entropy(in, length);
      mutex.unlock();
    }
    
  private:    
    Ne7ssh_Mutex mutex;
    Botan::RandomNumberGenerator* rng;
};



ne7ssh::ne7ssh() : connections(0), conCount(0)
{
  errs = new Ne7sshError();
  if (ne7ssh::running)
  {
    errs->push (-1, "Cannot initialize more than more instance of ne7ssh class within the same application. Aborting.");
    kill (0, SIGABRT);
    return;
  }
  init = new LibraryInitializer("thread_safe");
  ne7ssh::running = true;
  allConns.conns = 0;
  allConns.count = 0;

#if !BOTAN_PRE_18 && !BOTAN_PRE_15
  ne7ssh::rng = new Locking_AutoSeeded_RNG();
#endif

#if !defined(WIN32) && !defined(__MINGW32__)
  int status = ne7ssh_thread_create (&select_thread, NULL, selectThread, (void*)this);
  if (status)
  {
  errs->push (-1, "Failure creating a new thread.");
    return;
  }
#else
  if ((select_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) selectThread, (LPVOID)this, 0, NULL)) == NULL)
  {
    errs->push (-1, "Failure creating new thread.");
    return;
  }
#endif

}

ne7ssh::~ne7ssh()
{
  uint32 i;
  long ign;
  
  ne7ssh::running = false;
  lock();
  for (i = 0; i < conCount; i++) 
      close(i);
  unlock();

  ne7ssh_thread_join (select_thread, (void**)&ign);

  if (conCount)
  {
    for (i = 0; i < conCount; i++) 
      delete connections[i];
    free (connections);
  }
  else if (connections) free (connections);

  if (ne7ssh::PREFERED_CIPHER)
  {
    free (ne7ssh::PREFERED_CIPHER);
    ne7ssh::PREFERED_CIPHER = 0;
  }
  if (ne7ssh::PREFERED_MAC)
  {
    free (ne7ssh::PREFERED_MAC);
    ne7ssh::PREFERED_MAC = 0;
  }
  if (errs)
  {
    delete (errs);
    errs = 0;
  }
#if !BOTAN_PRE_18 && !BOTAN_PRE_15
  if (ne7ssh::rng) 
  {
    delete (rng);
    rng = 0;
  }
#endif

  delete init;

}

void *ne7ssh::selectThread (void *initData)
{
  ne7ssh *_ssh = (ne7ssh*) initData;
  uint32 i, z;
  int status = 0;
  fd_set rd;
  SOCKET rfds;
  struct timeval waitTime;
  connStruct* allConns;
  bool cmdOrShell = false;
  bool fdIsSet;

  while (running)
  {
    if (!lock())
    {
      _ssh->selectDead();
      return 0;
    }
    allConns = _ssh->getConnetions ();
    for (i = 0; i < allConns->count; i++)
    {
      if (allConns->conns[i]->isOpen() && allConns->conns[i]->data2Send() && !allConns->conns[i]->isSftpActive())
        allConns->conns[i]->sendData();
    }

    waitTime.tv_sec = 0;
    waitTime.tv_usec = 10000;

    rfds = 0;

    FD_ZERO (&rd);
    fdIsSet = false;

    for (i = 0; i < allConns->count; i++)
    {
      cmdOrShell = (allConns->conns[i]->isRemoteShell() || allConns->conns[i]->isCmdRunning()) ? true : false;
      if (allConns->conns[i]->isOpen() && cmdOrShell)
      {
        rfds = rfds > allConns->conns[i]->getSocket() ? rfds : allConns->conns[i]->getSocket();
        FD_SET (allConns->conns[i]->getSocket(), &rd);
        if (!fdIsSet) fdIsSet = true;
      }
      else if ((allConns->conns[i]->isConnected() && allConns->conns[i]->isRemoteShell()) || allConns->conns[i]->isCmdClosed())
      {
        delete (allConns->conns[i]);
    allConns->conns[i] = 0;
        allConns->count--;
        for (z = i; z < allConns->count; z++)
        {
          allConns->conns[z] = allConns->conns[z + 1];
          allConns->conns[z + 1] = 0;
        }
        _ssh->setCount (allConns->count);
    i--;
      }
    }
    if (!unlock())
    {
      _ssh->selectDead();
      return 0;
    }

    if (fdIsSet)
    {
      if (rfds) status = select(rfds + 1, &rd, NULL, NULL, &waitTime);
      else status = select(rfds + 1, NULL, &rd, NULL, &waitTime);
    }
    else usleep (1000);
    if (status == -1)
    {
      errs->push (-1, "Error within select thread.");
      usleep (1000);
    }

    if (!lock())
    {
      _ssh->selectDead();
      return 0;
    }

    allConns = _ssh->getConnetions ();
    for (i = 0; i < allConns->count; i++)
    {
      if (allConns->conns[i]->isOpen() && FD_ISSET (allConns->conns[i]->getSocket(), &rd))
        allConns->conns[i]->handleData();
    }
    if (!unlock())
    {
      _ssh->selectDead();
      return 0;
    }
  }
  return 0;
}

int ne7ssh::connectWithPassword (const char *host, const int port, const char* username, const char* password, bool shell, const int timeout)
{
  int channel;
  uint32 currentRecord, z;
  uint32 channelID;

  ne7ssh_connection* con = new ne7ssh_connection ();

  if (!lock()) return -1;
  if (!conCount) connections = (ne7ssh_connection**) malloc (sizeof (ne7ssh_connection*));
  else connections = (ne7ssh_connection**) realloc (connections, sizeof (ne7ssh_connection*) * (conCount + 1));
  connections[conCount++] = con;
  allConns.count = conCount;
  allConns.conns = connections;
  channelID = getChannelNo();
  con->setChannelNo (channelID);

  if (!unlock()) return -1;
  channel = con->connectWithPassword (channelID, host, port, username, password, shell, timeout);

  if (channel == -1) 
  {
    if (!lock()) return -1;
    for (z = 0; z < allConns.count; z++)
    {
      if (allConns.conns[z] == con) 
      {
        currentRecord = z;
        break;
      }
    }
    if (z == allConns.count)
    {
      ne7ssh::errors()->push (-1, "Unexpected behaviour!");
      return -1;
    }

    delete con;
    allConns.conns[currentRecord] = 0;
    allConns.count--;
    for (z = currentRecord; z < allConns.count; z++)
    {
      allConns.conns[z] = allConns.conns[z + 1];
      allConns.conns[z + 1] = 0;
    }
    conCount = allConns.count;
    if (!unlock()) return -1;
  }
  return channel;
}

int ne7ssh::connectWithKey (const char* host, const int port, const char* username, const char* privKeyFileName, bool shell, const int timeout)
{
  int channel;
  uint32 currentRecord, z;
  uint32 channelID;

  ne7ssh_connection* con = new ne7ssh_connection ();
  if (!lock()) return -1;
  if (!conCount) connections = (ne7ssh_connection**) malloc (sizeof (ne7ssh_connection*) * (conCount + 1));
  else connections = (ne7ssh_connection**) realloc (connections, sizeof (ne7ssh_connection*) * (conCount + 1));
  connections[conCount++] = con;
  allConns.count = conCount;
  allConns.conns = connections;
  channelID = getChannelNo();
  con->setChannelNo (channelID);

  if (!unlock()) return -1;

  channel = con->connectWithKey (channelID, host, port, username, privKeyFileName, shell, timeout);

  if (channel == -1) 
  {
    if (!lock()) return -1;
    for (z = 0; z < allConns.count; z++)
    {
      if (allConns.conns[z] == con) 
      {
        currentRecord = z;
        break;
      }
    }
    if (z == allConns.count)
    {
      ne7ssh::errors()->push (-1, "Unexpected behaviour!");
      return -1;
    }

    delete con;
    allConns.conns[currentRecord] = 0;
    allConns.count--;
    for (z = currentRecord; z < allConns.count; z++)
    {
      allConns.conns[z] = allConns.conns[z + 1];
      allConns.conns[z + 1] = 0;
    }
    conCount = allConns.count;
    if (!unlock()) return -1;
  }
  return channel;
}


bool ne7ssh::send (const char* data, int channel)
{
  uint32 i;

  if (!lock()) return false;
  for (i = 0; i < conCount; i++)
  {
    if (channel == connections[i]->getChannelNo())
    {
      connections[i]->sendData (data);
        if (!unlock()) return false;
      else return true;
    }
  }
    errs->push (-1, "Bad channel: %i specified for sending.", channel);
  if (!unlock()) return false;
  return false;
}

bool ne7ssh::initSftp (Ne7SftpSubsystem& _sftp, int channel)
{
  uint32 i;
  Ne7sshSftp *__sftp;

  if (!lock()) return false;
  for (i = 0; i < conCount; i++)
  {
    if (channel == connections[i]->getChannelNo())
    {
      __sftp = connections[i]->startSftp ();
      if (!__sftp)
      {
        if (!unlock()) return false;
    return false;
      }
      else
      {
        Ne7SftpSubsystem sftpSubsystem (__sftp);
        _sftp = sftpSubsystem;
        if (!unlock()) return false;
        return true;
      }
    }
  }
  if (!unlock()) return false;
  errs->push (-1, "Bad channel: %i specified. Cannot initialize SFTP subsystem.", channel);
  return false;
}


bool ne7ssh::sendCmd (const char* cmd, int channel, int timeout)
{
  uint32 i;
  time_t cutoff = 0;
  bool status;

  if (timeout) cutoff = time(NULL) + timeout;

  if (!lock()) return false;
  for (i = 0; i < conCount; i++)
  {
    if (channel == connections[i]->getChannelNo())
    {
      status = connections[i]->sendCmd (cmd);
      if (!status)
      {
        if (!unlock()) return false;
    return false;
      }

      if (!timeout)
      {
        while (!connections[i]->getCmdComplete())
    {
          for (i = 0; i < conCount; i++)
      {
            if (channel == connections[i]->getChannelNo())
          break;
          }
      if (i == conCount)
      {
        errs->push (-1, "Bad channel: %i specified for sending.", channel);
        unlock();
        return false;
      }
          if (!connections[i]->getCmdComplete())
          {
            if (!unlock()) return false;
            usleep (10000);
            if (!lock()) return false;
          }
    }
      }
      else if (timeout > 0)
      {
        while (!connections[i]->getCmdComplete())
    {
          for (i = 0; i < conCount; i++)
      {
            if (channel == connections[i]->getChannelNo())
          break;
          }
      if (i == conCount)
      {
        errs->push (-1, "Bad channel: %i specified for sending.", channel);
        if (!unlock()) return false;
        return false;
      }
          if (!connections[i]->getCmdComplete())
          {
            if (!unlock()) return false;
            usleep (10000);
            if (!lock()) return false;
            if (!cutoff) continue;
            if (time(NULL) >= cutoff) break;
          }
    }
      }
      if (!unlock()) return false;
      return true;
    }
  }
  errs->push (-1, "Bad channel: %i specified for sending.", channel);
  if (!unlock()) return false;
  return false;
}


bool ne7ssh::close (int channel)
{
  uint32 i;
  bool status = false;

  if (channel == -1)
  {
    errs->push (-1, "Bad channel: %i specified for closing.", channel);
    return false;
  }

  if (!lock()) return false;
  for (i = 0; i < conCount; i++)
  {
    if (channel == connections[i]->getChannelNo())
      status = connections[i]->sendClose();
  }
  errs->deleteChannel (channel);

  if (!unlock()) return false;

  return status;
}

bool ne7ssh::waitFor (int channel, const char* str, uint32 timeSec)
{
  Botan::byte one;
  const Botan::byte *carret;
  const char* buffer;
  size_t len = 0, carretLen = 0, str_len = 0, prevLen = 0;
  time_t cutoff = 0;
  bool dataChange;

  if (timeSec) cutoff = time(NULL) + timeSec;

  if (channel == -1)
  {
    errs->push (-1, "Bad channel: %i specified for waiting.", channel);
    return false;
  }

  str_len = strlen (str);

  while (1)
  {
    if (!lock()) return false;
    buffer = read (channel, false);
    if (buffer)
    {
      len = getReceivedSize(channel, false);
      if (cutoff && prevLen && len == prevLen) dataChange = false;
      else
      {
        dataChange = true;
        prevLen = len;
      }
      carret = (const Botan::byte*) buffer + len - 1;
      one = *str;
      carretLen = 1;

      while (carretLen <= len)
      {
        if ((*carret == one) && (str_len <= carretLen))
        {
          if (!memcmp (carret, str, str_len)) 
          {
            if (!unlock()) return false;
            return true;
          }
        }
        carretLen++;
        carret--;
      }
    }
    else (dataChange = false);
    if (!unlock()) return false;
    usleep (10000);
    if (!cutoff) continue;
    if (time(NULL) >= cutoff) break;
  }
  return false;
}


const char* ne7ssh::read (int channel, bool do_lock)
{
  uint32 i;
  SecureVector<Botan::byte> data;

  if (channel == -1)
  {
    errs->push (-1, "Bad channel: %i specified for reading.", channel);
    return false;
  }
  if (do_lock && !lock()) return false;
  for (i = 0; i < conCount; i++)
  {
    if (channel == connections[i]->getChannelNo())
    {
      data = connections[i]->getReceived();
      if (data.size())
      {
        if (do_lock && !unlock()) return false;
        return ((const char*)connections[i]->getReceived().begin());
      }
    }
  }
  if (do_lock && !unlock()) return false;
  return 0;
}

char *ne7ssh::readAndReset(int channel, char *(*alloc)(size_t))
{
  if (channel == -1)
  {
    errs->push (-1, "Bad channel: %i specified for reading.", channel);
    return 0;
  }
  if (!lock()) return 0;
  char *buffer = 0;
  for (uint32 i = 0; i < conCount; i++)
  {
    SecureVector<Botan::byte> data;
    if (channel == connections[i]->getChannelNo())
    {
      data = connections[i]->getReceived();
      if (data.size())
      {
        buffer = alloc(connections[i]->getReceived().size());
        strcpy(buffer, reinterpret_cast<char*>(connections[i]->getReceived().begin()));
        connections[i]->resetReceiveBuffer();
      }
      break;
    }
  }
  if (!unlock()) return false;
  return buffer;
}


void* ne7ssh::readBinary (int channel)
{
  uint32 i;
  SecureVector<Botan::byte> data;

  if (channel == -1)
  {
    errs->push (-1, "Bad channel: %i specified for reading.", channel);
    return false;
  }

  if (!lock()) return 0;
  for (i = 0; i < conCount; i++)
  {
    if (channel == connections[i]->getChannelNo())
    {
      data = connections[i]->getReceived();
      if (data.size())
      {
        if (!unlock()) return 0;
        return ((void*)connections[i]->getReceived().begin());
      }
    }
  }
  if (!unlock()) return 0;
  return 0;
}


int ne7ssh::getReceivedSize (int channel, bool do_lock)
{
  uint32 i;
  int size;

  if (do_lock && !lock()) return 0;
  for (i = 0; i < conCount; i++)
  {
    if (channel == connections[i]->getChannelNo())
    {
      if (!connections[i]->getReceived().size())
      {
        if (do_lock && !unlock()) return 0;
        return 0;
      }
      else
      {
        size = connections[i]->getReceived().size();
        if (do_lock && !unlock()) return 0;
        return (size);
      }
    }
  }
  if (do_lock && !unlock()) return 0;
  return 0;
}

uint32 ne7ssh::getChannelNo ()
{
  uint32 i;
  int32 channelID = 1;

  if (!conCount)
  {
    return channelID;
  }

  for (channelID = 1; channelID != 0x7FFFFFFF; channelID++)
  {
    for (i = 0; i < conCount; i++)
    {
      if (connections[i] && (connections[i]->getChannelNo() == channelID))
        break;
    }
    if (i == conCount) break;
  }

  if (channelID == 0x7FFFFFFF)
  {
    errs->push (-1, "Maximum theoretical channel count reached!");
    return 0;
  }
  else return channelID;
}

void ne7ssh::setOptions (const char* prefCipher, const char* prefHmac)
{
  size_t len = 0;

  if (prefCipher) len = strlen (prefCipher);
  if (!ne7ssh::PREFERED_CIPHER && len) ne7ssh::PREFERED_CIPHER = (char*) malloc (len);
  else if (len) ne7ssh::PREFERED_CIPHER = (char*) realloc (ne7ssh::PREFERED_CIPHER, len);
  memcpy (ne7ssh::PREFERED_CIPHER, prefCipher, len);

  len = 0;
  if (prefHmac) len = strlen (prefHmac);
  if (!ne7ssh::PREFERED_MAC && len) ne7ssh::PREFERED_MAC = (char*) malloc (len);
  else if (len) ne7ssh::PREFERED_MAC = (char*) realloc (ne7ssh::PREFERED_MAC, len);
  memcpy (ne7ssh::PREFERED_MAC, prefHmac, len);
}

bool ne7ssh::lock ()
{
  int status;
  status = ne7ssh::mut.lock();

  if (!isSelectActive())
  {
    errs->push (-1, "Select thread appears to be dead.");
    return false;
  }

  if (status)
  {
    errs->push (-1, "Could not aquire a mutex lock. Error: %i.", status);
    return false;
  }
  return true;
}

bool ne7ssh::unlock ()
{
  int status;
  status = ne7ssh::mut.unlock();

  if (!isSelectActive())
  {
    errs->push (-1, "Select thread appears to be dead.");
    return false;
  }

  if (status)
  {
    errs->push (-1, "Error while releasing a mutex lock. Error: %i.", status);
    return false;
  }
  return true;
}

SSH_EXPORT Ne7sshError* ne7ssh::errors()
{
  return errs;
}

bool ne7ssh::generateKeyPair (const char* type, const char* fqdn, const char* privKeyFileName, const char* pubKeyFileName, uint16 keySize)
{
  ne7ssh_keys keyPair;
  enum keyAlgos { DSA, RSA };
  uint8 keyAlgo;

  if (!memcmp (type, "dsa", 3))
    keyAlgo = DSA;
  else if (!memcmp (type, "rsa", 3))
    keyAlgo = RSA;

  switch (keyAlgo)
  {
    case DSA:
      if (!keySize)
        return keyPair.generateDSAKeys (fqdn, privKeyFileName, pubKeyFileName);
      else
        return keyPair.generateDSAKeys (fqdn, privKeyFileName, pubKeyFileName, keySize);

    case RSA:
      if (!keySize)
        return keyPair.generateRSAKeys (fqdn, privKeyFileName, pubKeyFileName);
      else
        return keyPair.generateRSAKeys (fqdn, privKeyFileName, pubKeyFileName, keySize);

    default:
      errs->push (-1, "The specfied key algorithm: %i not supported", keyAlgo);
      return false;
  }
  return false;
}

Ne7SftpSubsystem::Ne7SftpSubsystem () : inited(false), sftp(0)
{
}

Ne7SftpSubsystem::Ne7SftpSubsystem (Ne7sshSftp* _sftp) : inited(true), sftp((Ne7sshSftp*)_sftp)
{
}

Ne7SftpSubsystem::~Ne7SftpSubsystem ()
{
}

bool Ne7SftpSubsystem::setTimeout (uint32 _timeout)
{
  if (!inited) return errorNotInited();
  sftp->setTimeout (_timeout);
  return true;
}

uint32 Ne7SftpSubsystem::openFile (const char* filename, uint8 mode)
{
  if (!inited) return errorNotInited();
  return sftp->openFile (filename, mode);
}

uint32 Ne7SftpSubsystem::openDir (const char* dirname)
{
  if (!inited) return errorNotInited();
  return sftp->openDir (dirname);
}

bool Ne7SftpSubsystem::readFile (uint32 fileID, uint64 offset)
{
  if (!inited) return errorNotInited();
  return sftp->readFile (fileID, offset);
}

bool Ne7SftpSubsystem::writeFile (uint32 fileID, const uint8* data, uint32 len, uint64 offset)
{
  if (!inited) return errorNotInited();
  return sftp->writeFile (fileID, data, len);
}

bool Ne7SftpSubsystem::closeFile (uint32 fileID)
{
  if (!inited) return errorNotInited();
  return sftp->closeFile (fileID);
}

bool Ne7SftpSubsystem::errorNotInited ()
{
  ne7ssh::errors()->push (-1, "This SFTP system has not been initialized.");
  return false;
}

bool Ne7SftpSubsystem::getFileAttrs (fileAttrs& attrs, const char* filename, bool followSymLinks)
{
  if (!inited) return errorNotInited();
  return sftp->getFileAttrs (attrs, filename, followSymLinks);
}

bool Ne7SftpSubsystem::get (const char* remoteFile, FILE* localFile)
{
  if (!inited) return errorNotInited();
  return sftp->get (remoteFile, localFile);
}

bool Ne7SftpSubsystem::put (FILE* localFile, const char* remoteFile)
{
  if (!inited) return errorNotInited();
  return sftp->put (localFile, remoteFile);
}

bool Ne7SftpSubsystem::rm (const char* remoteFile)
{
  if (!inited) return errorNotInited();
  return sftp->rm (remoteFile);
}

bool Ne7SftpSubsystem::mv (const char* oldFile, const char* newFile)
{
  if (!inited) return errorNotInited();
  return sftp->mv (oldFile, newFile);
}

bool Ne7SftpSubsystem::mkdir (const char* remoteDir)
{
  if (!inited) return errorNotInited();
  return sftp->mkdir (remoteDir);
}

bool Ne7SftpSubsystem::rmdir (const char* remoteDir)
{
  if (!inited) return errorNotInited();
  return sftp->rmdir (remoteDir);
}

const char* Ne7SftpSubsystem::ls (const char* remoteDir, bool longNames)
{
  if (!inited)
  {
    errorNotInited();
    return 0;
  }
  return sftp->ls (remoteDir, longNames);
}

bool Ne7SftpSubsystem::cd (const char* remoteDir)
{
  if (!inited) return errorNotInited();
  return sftp->cd (remoteDir);
}

bool Ne7SftpSubsystem::chmod (const char* remoteFile, const char* mode)
{
  if (!inited) return errorNotInited();
  return sftp->chmod (remoteFile, mode);
}

bool Ne7SftpSubsystem::chown (const char* remoteFile, uint32_t uid, uint32_t gid)
{
  if (!inited) return errorNotInited();
  return sftp->chown (remoteFile, uid, gid);
}

bool Ne7SftpSubsystem::isFile (const char* remoteFile)
{
  if (!inited) return errorNotInited();
  return sftp->isFile (remoteFile);
}

bool Ne7SftpSubsystem::isDir (const char* remoteFile)
{
  if (!inited) return errorNotInited();
  return sftp->isDir (remoteFile);
}

