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

#include "ne7ssh_string.h"
#include "ne7ssh.h"
#include <cstdio>
#if !defined(WIN32) && !defined(__MINGW32__)
#   include <arpa/inet.h>
#endif

using namespace Botan;

ne7ssh_string::ne7ssh_string() : positions(0), parts(0)
{
}

ne7ssh_string::ne7ssh_string(Botan::SecureVector<Botan::byte>& var, uint32 position) : positions(0), parts(0)
{
  buffer.set ((var.begin() + position), (var.size() - position));
}

ne7ssh_string::ne7ssh_string(const char* var, uint32 position) : positions(0), parts(0)
{
  char null_char=0x0;
  buffer.set ((Botan::byte*)(var + position), (u32bit) (strlen(var) - position));
  buffer.append ((Botan::byte*) &null_char, 1);
}

ne7ssh_string::~ne7ssh_string()
{
  if (positions) 
      free (positions);
}

void ne7ssh_string::addString (const char* str)
{
  Botan::byte* value = (Botan::byte*) str;
  size_t len = strlen(str);
  uint32 nLen = htonl((long) len);

  buffer.append ((Botan::byte*)&nLen, sizeof(uint32));
  buffer.append (value, (u32bit) len);
}

bool ne7ssh_string::addFile (const char* filename)
{
  FILE *FI = fopen (filename, "rb");
  size_t size;
  Botan::byte* data;

  if (!FI)
  {
    ne7ssh::errors()->push (-1, "Could not open key file: %s.", filename);
    return false;
  }

  fseek (FI, 0L, SEEK_END);
  size = ftell (FI);
  rewind (FI);

  data = (Botan::byte*) malloc (size);
  fread (data, size, 1, FI);
  fclose (FI);
  buffer.append (data, (u32bit) size);
  free (data);
  return true;
}

void ne7ssh_string::addBigInt (const Botan::BigInt& bn)
{
  SecureVector<Botan::byte> converted;
  bn2vector (converted, bn);
  uint32 nLen = htonl (converted.size());

  buffer.append ((Botan::byte*)&nLen, sizeof(uint32));
  buffer.append (converted);
}

void ne7ssh_string::addVectorField (const Botan::SecureVector<Botan::byte> &vector)
{
  uint32 nLen = htonl (vector.size());

  buffer.append ((Botan::byte*)&nLen, sizeof(uint32));
  buffer.append (vector);
}


void ne7ssh_string::addBytes (const Botan::byte* buff, uint32 len)
{
  buffer.append(buff, len);
}

void ne7ssh_string::addVector (Botan::SecureVector<Botan::byte> &secvec)
{
  buffer.append(secvec.begin(), secvec.size());
}

void ne7ssh_string::addChar (const char ch)
{
  buffer.append((Botan::byte*)&ch, 1);
}

void ne7ssh_string::addInt (const uint32 var)
{
  uint32 nVar = htonl (var);

  buffer.append((Botan::byte*)&nVar, sizeof(uint32));
}

bool ne7ssh_string::getString (Botan::SecureVector<Botan::byte>& result)
{
  SecureVector<Botan::byte> tmpVar (buffer);
  uint32 len;

  len = ntohl (*((uint32*)tmpVar.begin()));
  if (len > tmpVar.size()) return false;

  result.set (tmpVar.begin() + sizeof(uint32), len);
  buffer.set (tmpVar.begin() + sizeof(uint32) + len, tmpVar.size() - sizeof(uint32) - len);
  return true;
}

bool ne7ssh_string::getBigInt (Botan::BigInt& result)
{
  SecureVector<Botan::byte> tmpVar (buffer);
  uint32 len;

  len = ntohl (*((uint32*)tmpVar.begin()));
  if (len > tmpVar.size()) return false;

  BigInt tmpBI (tmpVar.begin() + sizeof(uint32), len);
  result.swap (tmpBI);
  buffer.set (tmpVar.begin() + sizeof(uint32) + len, tmpVar.size() - sizeof(uint32) - len);
  return true;
}

uint32 ne7ssh_string::getInt ()
{
  SecureVector<Botan::byte> tmpVar (buffer);
  uint32 result;

  result = ntohl (*((uint32*)tmpVar.begin()));
  buffer.set (tmpVar.begin() + sizeof(uint32), tmpVar.size() - sizeof(uint32));
  return result;
}

Botan::byte ne7ssh_string::getByte ()
{
  SecureVector<Botan::byte> tmpVar (buffer);
  Botan::byte result;

  result = *(tmpVar.begin());
  buffer.set (tmpVar.begin() + 1, tmpVar.size() - 1);
  return result;
}


void ne7ssh_string::split (const char token)
{
  Botan::byte* _buffer = buffer.begin();
  uint32 len = buffer.size();
  uint32 i;

  if (positions) return;
  positions = (Botan::byte**) malloc (sizeof(Botan::byte*) * (parts + 1));
  positions[parts] = _buffer;
  parts++;

  for (i = 0; i < len; i++)
  {
    if (_buffer[i] == token)
    {
      _buffer[i] = '\0';
      positions = (Botan::byte**) realloc (positions, sizeof(Botan::byte*) * (parts + 1));

      positions[parts] = _buffer + i + 1;
      parts++;
    }
  }
}

char* ne7ssh_string::nextPart ()
{
  char* result;
  if (currentPart >= parts || !positions) return 0;

  result = (char*) positions[currentPart];
  currentPart++;

  return result;
}

void ne7ssh_string::chop (uint32 nBytes)
{
  SecureVector<Botan::byte> tmpVar (buffer);
  buffer.set (tmpVar.begin(), tmpVar.size() - nBytes);
}


void ne7ssh_string::bn2vector (Botan::SecureVector<Botan::byte>& result, const Botan::BigInt& bi)
{
  int high;
  Botan::byte zero = '\0';

  SecureVector<Botan::byte> strVector = BigInt::encode (bi);

  high = (*(strVector.begin()) & 0x80) ? 1 : 0;

  if (high) result.set (&zero, 1);
  else result.clear();
  result.append (strVector);
}
