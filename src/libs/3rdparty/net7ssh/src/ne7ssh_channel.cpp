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
#include "ne7ssh_channel.h"
#include "ne7ssh_session.h"
#include "ne7ssh.h"

using namespace Botan;

//uint32 ne7ssh_channel::channelCount = 0;

ne7ssh_channel::ne7ssh_channel(ne7ssh_session *_session) : eof(false), closed(false), cmdComplete(false), shellSpawned(false), session(_session), channelOpened(false)
{
}

ne7ssh_channel::~ne7ssh_channel()
{
}

uint32 ne7ssh_channel::open (uint32 channelID)
{
  ne7ssh_string packet;
  ne7ssh_transport *_transport = session->transport;

  packet.addChar (SSH2_MSG_CHANNEL_OPEN);
  packet.addString ("session");
  packet.addInt (channelID);
//  ne7ssh_channel::channelCount++;
  windowSend = 0;
  windowRecv = MAX_PACKET_LEN - 2400;
  packet.addInt (windowRecv);
  packet.addInt (MAX_PACKET_LEN);

  if (!_transport->sendPacket (packet.value())) return 0;
  if (!_transport->waitForPacket (SSH2_MSG_CHANNEL_OPEN_CONFIRMATION))
  {
    ne7ssh::errors()->push (-1, "New channel: %i could not be open.", channelID);
    return 0;
  }
  if (handleChannelConfirm ())
  {
    channelOpened = true;
    return channelID;
//    return (ne7ssh_channel::channelCount - 1);
  }
  else return 0;
}

bool ne7ssh_channel::handleChannelConfirm ()
{
  ne7ssh_transport *_transport = session->transport;
  SecureVector<Botan::byte> packet;
  _transport->getPacket (packet);  
  ne7ssh_string channelConfirm (packet, 1);
  uint32 field;

  // Receive Channel
  field = channelConfirm.getInt();
  // Send Channel
  field = channelConfirm.getInt();
  session->setSendChannel (field);

  // Window Size
  field = channelConfirm.getInt();
  windowSend = field;

  // Max Packet
  field = channelConfirm.getInt();
  session->setMaxPacket (field);
  return true;
}

bool ne7ssh_channel::adjustWindow (Botan::SecureVector<Botan::byte>& packet)
{
  ne7ssh_string adjustWindow (packet, 0);
  ne7ssh_string newPacket;
  uint32 field;

  // channel number
  field = adjustWindow.getInt();

  // add bytes to the window
  field = adjustWindow.getInt();
  windowSend += field;
  return true;
}

bool ne7ssh_channel::handleEof (Botan::SecureVector<Botan::byte>& packet)
{
  this->cmdComplete = true;
  windowRecv = 0;
  eof = true;
  if (!closed) sendClose();
  closed = true;
  channelOpened = false;
  ne7ssh::errors()->push (session->getSshChannel(), "Remote side responded with EOF.");
  return false;
}

void ne7ssh_channel::handleClose (Botan::SecureVector<Botan::byte>& newPacket)
{
  ne7ssh_string packet;

  if (!closed) sendClose ();
  windowRecv = 0;
  closed = true;
  channelOpened = false;
}

bool ne7ssh_channel::handleDisconnect (Botan::SecureVector<Botan::byte>& packet)
{
  ne7ssh_string message (packet, 0);
//  uint32 reasonCode = message.getInt ();
  SecureVector<Botan::byte> description;

  message.getString (description);
  windowSend = windowRecv = 0;
  closed = true;
  channelOpened = false;

  ne7ssh::errors()->push (session->getSshChannel(), "Remote Site disconnected with Error: %B.",  &description);
  return false;
}

bool ne7ssh_channel::sendClose ()
{
  ne7ssh_transport *_transport = session->transport;
  ne7ssh_string packet;

  if (closed) return false;
  packet.addChar (SSH2_MSG_CHANNEL_CLOSE);
  packet.addInt (session->getSendChannel());

  if (!_transport->sendPacket (packet.value())) return false;
  windowSend = 0;
  windowRecv = 0;
  closed = true;
  return true;
}
  
bool ne7ssh_channel::sendEof ()
{
  ne7ssh_transport *_transport = session->transport;
  ne7ssh_string packet;

  if (closed) return false;
  packet.addChar (SSH2_MSG_CHANNEL_EOF);
  packet.addInt (session->getSendChannel());

  if (!_transport->sendPacket (packet.value())) return false;
  windowSend = 0;
  windowRecv = 0;
  closed = true;
  return true;
}

void ne7ssh_channel::sendAdjustWindow ()
{
  uint32 len = session->getMaxPacket () - windowRecv - 2400;
  ne7ssh_string packet;
  ne7ssh_transport *_transport = session->transport;

  packet.addChar (SSH2_MSG_CHANNEL_WINDOW_ADJUST);
  packet.addInt (session->getSendChannel());
  packet.addInt (len);
  windowRecv = len;

  _transport->sendPacket (packet.value());
}

bool ne7ssh_channel::handleData (Botan::SecureVector<Botan::byte>& packet)
{
  ne7ssh_string handleData (packet, 0);
  uint32 channelID;
  SecureVector<Botan::byte> data;

  channelID = handleData.getInt();

  if (!handleData.getString (data)) return false;
  if (!data.size())
		ne7ssh::errors()->push (session->getSshChannel(), "Abnormal. End of stream detected.");
  if (inBuffer.length()) inBuffer.chop(1);
  inBuffer.addVector (data);
  if (inBuffer.length()) inBuffer.addChar(0x00);
  windowRecv -= data.size();
  if (windowRecv == 0) sendAdjustWindow ();
  return true;
}

bool ne7ssh_channel::handleExtendedData (Botan::SecureVector<Botan::byte>& packet)
{
  ne7ssh_string handleData (packet, 0);
  uint32 channelID, dataType;
  SecureVector<Botan::byte> data;

  channelID = handleData.getInt();
  dataType = handleData.getInt();
  if (dataType != 1)
  {
    ne7ssh::errors()->push (session->getSshChannel(), "Unable to handle received request.");
    return false;
  }

  if (handleData.getString (data))
    ne7ssh::errors()->push (session->getSshChannel(), "Remote side returned the following error: %B", &data);
  else return false;

  windowRecv -= data.size();
  if (windowRecv == 0) sendAdjustWindow ();
  return true;
}

void ne7ssh_channel::handleRequest (Botan::SecureVector<Botan::byte>& packet)
{
  ne7ssh_string handleRequest (packet, 0);
  uint32 channelID;
  SecureVector<Botan::byte> field;
  uint32 signal;

  channelID = handleRequest.getInt();
  handleRequest.getString (field);
  if (!memcmp((char*)field.begin(), "exit-signal", 11))
    ne7ssh::errors()->push (session->getSshChannel(),	"exit-signal ignored.");
  else if (!memcmp((char*)field.begin(), "exit-status", 11))
  {
    handleRequest.getByte();
    signal = handleRequest.getInt();
    ne7ssh::errors()->push (session->getSshChannel(), "Remote side exited with status: %i.", signal);
  }

//  handleRequest.getByte();
//  handleRequest.getString (field);
}

bool ne7ssh_channel::execCmd (const char* cmd)
{
  ne7ssh_transport *_transport = session->transport;
  ne7ssh_string packet;

  if (this->shellSpawned)
  {
    ne7ssh::errors()->push (session->getSshChannel(), "Remote shell is running. This command cannot be executed.");
    return false;
  }
  
  packet.clear();
  packet.addChar (SSH2_MSG_CHANNEL_REQUEST);
  packet.addInt (session->getSendChannel());
  packet.addString ("exec");
  packet.addChar (0);
  packet.addString (cmd);

  if (!_transport->sendPacket (packet.value())) 
    return false;

  cmdComplete = false;
  return true;	
}

void ne7ssh_channel::getShell ()
{
  ne7ssh_transport *_transport = session->transport;
  ne7ssh_string packet;

  packet.clear();
  packet.addChar (SSH2_MSG_CHANNEL_REQUEST);
  packet.addInt (session->getSendChannel());
  packet.addString ("pty-req");
  packet.addChar (0);
  packet.addString ("dumb");
  packet.addInt (80);
  packet.addInt (24);
  packet.addInt (0);
  packet.addInt (0);
  packet.addString ("");
  if (!_transport->sendPacket (packet.value())) return;

  packet.clear();
  packet.addChar (SSH2_MSG_CHANNEL_REQUEST);
  packet.addInt (session->getSendChannel());
  packet.addString ("shell");
  packet.addChar (0);
  if (!_transport->sendPacket (packet.value())) return;
  this->shellSpawned = true;
}

void ne7ssh_channel::receive ()
{
  ne7ssh_transport *_transport = session->transport;
  SecureVector<Botan::byte> packet;
  uint32 padLen;
  bool notFirst = false;
  short status;

  if (eof) 
  {
    return;
  }

  while ((status = _transport->waitForPacket (0, notFirst)))
  {
    if (status == -1)
    {
      eof = true;
      closed = true;
      channelOpened = false;
      return;
    }
    if (!notFirst) notFirst = true;
    padLen = _transport->getPacket (packet);
    handleReceived (packet);
  }
}

bool ne7ssh_channel::handleReceived (Botan::SecureVector<Botan::byte>& _packet)
{
  ne7ssh_string newPacket;
  Botan::byte cmd;

  newPacket.addVector (_packet);
  cmd = newPacket.getByte();
  switch (cmd)
  {
    case SSH2_MSG_CHANNEL_WINDOW_ADJUST:
      adjustWindow(newPacket.value());
      break;

    case SSH2_MSG_CHANNEL_DATA:
      return handleData (newPacket.value());
      break;

    case SSH2_MSG_CHANNEL_EXTENDED_DATA:
      handleExtendedData(newPacket.value());
      break;

    case SSH2_MSG_CHANNEL_EOF:
      return handleEof (newPacket.value());
      break;

    case SSH2_MSG_CHANNEL_CLOSE:
      handleClose (newPacket.value());
      break;

    case SSH2_MSG_CHANNEL_REQUEST:
      handleRequest (newPacket.value());
      break;

    case SSH2_MSG_IGNORE:
      break;

    case SSH2_MSG_DISCONNECT:
      return handleDisconnect(newPacket.value());
      break;

    default:
      ne7ssh::errors()->push (session->getSshChannel(), "Unhandled command encountered: %i.", cmd);
      return false;
    }
    return true;
}

void ne7ssh_channel::write (Botan::SecureVector<Botan::byte>& data)
{
  SecureVector<Botan::byte> dataBuff, outBuff, delayedBuff;
  uint32 len, maxBytes, i, dataStart;

  if (delayedBuffer.length()) 
  {
    dataBuff.set (delayedBuffer.value());
    delayedBuffer.clear();
  }
  dataBuff.append (data);

  if (!windowSend) delayedBuff.set (dataBuff);
  else if (windowSend < dataBuff.size())
  {
    outBuff.append (dataBuff.begin(), windowSend);
    delayedBuff.set (dataBuff.begin() + windowSend, dataBuff.size() - windowSend);
  }
  else outBuff.append (dataBuff);

  if (delayedBuff.size()) delayedBuffer.addVector (delayedBuff);
  if (!outBuff.size()) return;

  len = outBuff.size();
  windowSend -= len;

  maxBytes = session->getMaxPacket();
  for (i = 0; len > maxBytes - 64; i++)
  {
    dataStart = maxBytes * i;
    if (i) dataStart -= 64;
    dataBuff.set (outBuff.begin() + dataStart, maxBytes - 64);
    outBuffer.addVector (dataBuff);
    len -= maxBytes - 64;
  }
  if (len)
  {
    dataStart = maxBytes * i;
    if (i) dataStart -= 64;
    dataBuff.set (outBuff.begin() + dataStart, len);
    outBuffer.addVector (dataBuff);
    inBuffer.clear();
  }
}

void ne7ssh_channel::sendAll ()
{
  ne7ssh_transport *_transport = session->transport;
  SecureVector<Botan::byte> tmpVar;
  ne7ssh_string packet;

  if (!outBuffer.length() && delayedBuffer.length())
  {
    tmpVar.swap(delayedBuffer.value());
    delayedBuffer.clear();
    write (tmpVar);
  }
  if (!outBuffer.length()) return;
  packet.clear();
  packet.addChar (SSH2_MSG_CHANNEL_DATA);
  packet.addInt (session->getSendChannel());
  packet.addVectorField (outBuffer.value());

  windowSend -= outBuffer.length();
  inBuffer.clear();
  if (!_transport->sendPacket (packet.value())) return;
  else outBuffer.clear();

}

bool ne7ssh_channel::adjustRecvWindow (int bufferSize)
{
  windowRecv -= bufferSize;
  if (windowRecv == 0) sendAdjustWindow ();
  return true;
}

