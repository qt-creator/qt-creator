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

#include "ne7ssh_transport.h"
#include "ne7ssh.h"
#include "ne7ssh_session.h"

#if defined(WIN32) || defined(__MINGW32__)
#   define SOCKET_BUFFER_TYPE char
#   define close closesocket
#   define SOCK_CAST (char *)
class WSockInitializer
{
public:
  WSockInitializer()
  {
    static WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
  }
  ~WSockInitializer()
  {
    WSACleanup();
  }
}
;
WSockInitializer _wsock32_;
#else
#   define SOCKET_BUFFER_TYPE void
#   define SOCK_CAST (void *)
#   include <sys/socket.h>
#   include <netinet/in.h>
#   include <netdb.h>
#endif

using namespace Botan;

ne7ssh_transport::ne7ssh_transport(ne7ssh_session* _session) : seq(0), rSeq(0), session(_session), sock(-1)
{
}

ne7ssh_transport::~ne7ssh_transport()
{
  if (((long) sock) > -1) close (sock);
}

SOCKET ne7ssh_transport::establish (const char *host, uint32 port, int timeout)
{
  sockaddr_in remoteAddr;
  hostent *remoteHost;

  remoteHost = gethostbyname (host);
  if (!remoteHost || remoteHost->h_length == 0)
  {
    ne7ssh::errors()->push (((ne7ssh_session*)session)->getSshChannel(), "Host: '%s' not found.", host);
    return -1;
  }
  remoteAddr.sin_family = AF_INET;
  remoteAddr.sin_addr.s_addr = *(long*) remoteHost->h_addr_list[0];
  remoteAddr.sin_port = htons (port);

  sock = socket (AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
  {
    ne7ssh::errors()->push (((ne7ssh_session*)session)->getSshChannel(), "Failure to bind to socket.");
    return -1;
  }

  if (timeout < 1)
  {
    if (connect (sock, (struct sockaddr*) &remoteAddr, sizeof(remoteAddr)))
    {
      ne7ssh::errors()->push (((ne7ssh_session*)session)->getSshChannel(), "Unable to connect to remote server: '%s'.", host);
      return -1;
    }

    if (!NoBlock (sock, true)) return -1;
    else return sock;
  }
  else
  {
    if (!NoBlock (sock, true))
        return -1;

    if (connect (sock, (struct sockaddr*) &remoteAddr, sizeof(remoteAddr)) == -1)
    {
      fd_set rfds;
      struct timeval waitTime;

      waitTime.tv_sec = timeout;
      waitTime.tv_usec = 0;

      FD_ZERO(&rfds);
      FD_SET(sock, &rfds);

      int status;
      status = select(sock+1, &rfds, NULL, NULL, &waitTime);

      if ( status == 0 )
      {
          if ( ! FD_ISSET(sock, &rfds) )
          {
              ne7ssh::errors()->push (((ne7ssh_session*)session)->getSshChannel(), "Couldn't connect to remote server : timeout");
              return -1;
          }
      }
      if ( status < 0 )
      {
          ne7ssh::errors()->push (((ne7ssh_session*)session)->getSshChannel(), "Couldn't connect to remote server during select");
          return -1;
      }
    }
    return sock;
  }
}

bool ne7ssh_transport::NoBlock (SOCKET socket, bool on)
{
#ifndef WIN32
  int options;
  if ((options = fcntl ( socket, F_GETFL )) < 0)
  {
    ne7ssh::errors()->push (((ne7ssh_session*)session)->getSshChannel(), "Cannot read options of the socket: %i.", (int)socket);
    return false;
  }

  if ( on ) options = ( options | O_NONBLOCK );
  else  options = ( options & ~O_NONBLOCK );
  fcntl (socket, F_SETFL, options);
#else
  unsigned long options = 1;
  if (ioctlsocket(socket, FIONBIO, &options))
  {
    ne7ssh::errors()->push (((ne7ssh_session*)session)->getSshChannel(), "Cannot set asynch I/O on the socket: %i.", (int)socket);
    return false;
  }
#endif
  return true;
}

bool ne7ssh_transport::haveData ()
{
  return wait (sock, 0, 0);
}

bool ne7ssh_transport::wait (SOCKET socket, int rw, int timeout)
{
    int status;
    fd_set rfds, wfds;
    struct timeval waitTime;

    if (timeout > -1)
    {
      waitTime.tv_sec = timeout;
      waitTime.tv_usec = 0;
    }

    if (!rw)
    {
      FD_ZERO(&rfds);
      FD_SET(socket, &rfds);
    }
    else
    {
      FD_ZERO(&wfds);
      FD_SET(socket, &wfds);
    }

    if (!rw)
    {
      if (timeout > -1) status = select(socket + 1, &rfds, NULL, NULL, &waitTime);
      else status = select(socket + 1, &rfds, NULL, NULL, NULL);
    }
    else status = select(socket + 1, NULL, &wfds, NULL, NULL);

    if (status > 0)
      return true;
    else
      return false;
}


bool ne7ssh_transport::send (Botan::SecureVector<Botan::byte>& buffer)
{
  long byteCount;
  uint32 sent = 0;

  if (buffer.size() > MAX_PACKET_LEN)
  {
    ne7ssh::errors()->push (((ne7ssh_session*)session)->getSshChannel(), "Cannot send. Packet too large for the transport layer.");
    return false;
  }

  while (sent < buffer.size())
  {
    if (wait(sock, 1))
      byteCount = ::send(sock, (const SOCKET_BUFFER_TYPE *) (buffer.begin() + sent), buffer.size() - sent, 0);
    else return false;
    if (byteCount < 0)
        return false;
    sent += byteCount;
  }

  return true;
}

bool ne7ssh_transport::receive (Botan::SecureVector<Botan::byte>& buffer, bool append)
{
  Botan::byte in_buffer[MAX_PACKET_LEN];
  int len;

  if (wait(sock, 0))
    len = ::recv (sock, (char*) in_buffer, MAX_PACKET_LEN, 0);

  if (!len)
  {
    ne7ssh::errors()->push (((ne7ssh_session*)session)->getSshChannel(), "Received a packet of zero length.");
    return false;
  }

  if (len > MAX_PACKET_LEN)
  {
    ne7ssh::errors()->push (((ne7ssh_session*)session)->getSshChannel(), "Received packet exceeds the maximum size");
    return false;
  }

  if (len < 0)
  {
    ne7ssh::errors()->push (((ne7ssh_session*)session)->getSshChannel(), "Connection dropped");
    return false;
  }

  if (append) buffer.append (in_buffer, len);
  else
  {
    buffer.clear();
    buffer.set (in_buffer, len);
  }

  return true;
}

bool ne7ssh_transport::sendPacket (Botan::SecureVector<Botan::byte> &buffer)
{
  ne7ssh_crypt *_crypto = session->crypto;
  ne7ssh_string out;
  uint32 crypt_block;
  char padLen;
  uint32 packetLen;
  Botan::byte *padBytes;
  uint32 length;
  SecureVector<Botan::byte> crypted, hmac;

// No Zlib support right now
//  if (_crypto->isInited()) _crypto->compressData (buffer);
  length = buffer.size();

  crypt_block = _crypto->getEncryptBlock();
  if (!crypt_block) crypt_block = 8;

  padLen = 3 + crypt_block - ((length + 8) % crypt_block);
  packetLen = 1 + length + padLen;

  out.addInt (packetLen);
  out.addChar (padLen);
  out.addVector (buffer);

  padBytes = (Botan::byte*) malloc (padLen);
  memset (padBytes, 0x00, padLen);
  out.addBytes (padBytes, padLen);
  free (padBytes);

  if (_crypto->isInited())
  {
    if (!_crypto->encryptPacket (crypted, hmac, out.value(), seq))
    {
      ne7ssh::errors()->push (((ne7ssh_session*)session)->getSshChannel(), "Failure to encrypt the payload.");
      return false;
    }
    crypted.append (hmac);
    if (!send (crypted)) return false;
  }
  else if (!send (out.value())) return false;
  if (seq == MAX_SEQUENCE) seq = 0;
  else seq++;
  return true;
}

short ne7ssh_transport::waitForPacket (Botan::byte cmd, bool bufferOnly)
{
  ne7ssh_crypt *_crypto = session->crypto;
  Botan::byte _cmd;
  SecureVector<Botan::byte> tmpVar, decrypted, uncommpressed, ourMac, hMac;
  uint32 len, cryptoLen;
  bool havePacket = false;

/*  if (_crypto->isInited())
  {
    if (!in.is_empty()) _crypto->decryptPacket (tmpVar, in, _crypto->getDecryptBlock(), seq);
    else tmpVar.destroy();
  }
  else*/
  tmpVar.set (in);

  if (!tmpVar.is_empty())
  {
    if (_crypto->isInited())
    {
      if (!in.is_empty()) _crypto->decryptPacket (tmpVar, in, _crypto->getDecryptBlock());
      else tmpVar.destroy();
    }
    _cmd = *(tmpVar.begin() + 5);

    if (_cmd > 0 && _cmd < 0xff) havePacket = true;
  }

  if (!havePacket)
  {
    if (bufferOnly) return 0;
    if (!receive(in)) return -1;

    if (_crypto->isInited())
    {
      if (!in.is_empty()) _crypto->decryptPacket (tmpVar, in, _crypto->getDecryptBlock());
      else tmpVar.destroy();
    }
    else
    {
      while (in.size() < 4)
        if (!receive(in, true)) return -1;

      cryptoLen = ntohl (*((int*)in.begin())) + sizeof(uint32);
      while (in.size() < cryptoLen)
        if (!receive(in, true)) return -1;

      tmpVar.set (in);
    }
  }

  len = ntohl (*((int*)tmpVar.begin()));
  cryptoLen = len + sizeof(uint32);

  decrypted.set (tmpVar);
  if (_crypto->isInited())
  {
    while (((cryptoLen + _crypto->getMacInLen()) > in.size()) || (in.size() % _crypto->getDecryptBlock()))
    {
      if (!receive(in, true)) return 0;
    }
    if (cryptoLen > _crypto->getDecryptBlock())
    {
      tmpVar.set (in.begin() + _crypto->getDecryptBlock(), (cryptoLen - _crypto->getDecryptBlock()));
      if (!in.is_empty()) _crypto->decryptPacket (tmpVar, tmpVar, tmpVar.size());
      decrypted.append (tmpVar);
    }
    if (_crypto->getMacInLen())
    {
      _crypto->computeMac (ourMac, decrypted, rSeq );
      hMac.set (in.begin() + cryptoLen, _crypto->getMacInLen());
      if (hMac != ourMac)
      {
        ne7ssh::errors()->push (((ne7ssh_session*)session)->getSshChannel(), "Mismatched HMACs.");
        return -1;
      }
      cryptoLen += _crypto->getMacInLen();
    }


// No Zlib support right now
/*    if (_crypto->isCompressed())
    {
      tmpVar.set (decrypted.begin() + 5, len);
      _crypto->decompressData (tmpVar);
      uncommpressed.set (decrypted.begin(), 4);
      uncommpressed.append (tmpVar);
      decrypted.set (uncommpressed);
    }*/
  }

   if (rSeq == MAX_SEQUENCE) seq = 0;
   else rSeq++;
  _cmd = *(decrypted.begin() + 5);

  if (cmd == _cmd || !cmd)
  {
    inBuffer.set (decrypted);
    if (!(in.size() - cryptoLen)) in.destroy();
    else
    {
      tmpVar.swap (in);
      in.set (tmpVar.begin() + cryptoLen, tmpVar.size() - cryptoLen);
    }
    return _cmd;
  }
  else return 0;

}

uint32 ne7ssh_transport::getPacket (Botan::SecureVector<Botan::byte> &result)
{
  ne7ssh_crypt *_crypto = session->crypto;
  SecureVector<Botan::byte> tmpVector(inBuffer);
  uint32 len = ntohl (*((uint32*)tmpVector.begin()));
  Botan::byte padLen = *(tmpVector.begin() + 4);
  uint32 macLen = _crypto->getMacInLen();

  if (inBuffer.is_empty())
  {
    result.clear();
    return 0;
  }

  if (_crypto->isInited())
  {
    len += macLen;
    if (len > tmpVector.size()) len -= macLen;
  }

  tmpVector.append ((uint8*)"\0", 1);
  result.set (tmpVector.begin() + 5, len);
  _crypto->decompressData (result);

  inBuffer.destroy();
  return padLen;
}

