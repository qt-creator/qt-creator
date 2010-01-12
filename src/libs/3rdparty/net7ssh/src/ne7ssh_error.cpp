/***************************************************************************
 *   Copyright (C) 2005-2007 by NetSieben Technologies INC                 *
 *   Author: Andrew Useckas                                                *
 *   Email: andrew@netsieben.com                                           *
 *                                                                         *
 *   This program may be distributed under the terms of the Q Public       *
 *   License as defined by Trolltech AS of Norway and appearing in the     *
 *   file LICENSE.QPL included in the packaging of this file.              *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 ***************************************************************************/

#include "ne7ssh_error.h"
#include <string.h>
#include <cstdio>
#include <botan/secmem.h>
#include "stdarg.h"
#include "ne7ssh.h"

using namespace Botan;
Ne7ssh_Mutex Ne7sshError::mut;

Ne7sshError::Ne7sshError() : memberCount(0),  ErrorBuffer(0)
{

}

Ne7sshError::~Ne7sshError()
{
  uint16 i;

  for (i = 0; i < memberCount; i++)
  {
    if (ErrorBuffer[i] && ErrorBuffer[i]->errorStr) free (ErrorBuffer[i]->errorStr);
    free (ErrorBuffer[i]);
  }
  free (ErrorBuffer);	
}


bool Ne7sshError::push (int32 channel, const char* format, ...)
{
  va_list args;
  char *s;
  char* errStr = 0;
  uint32 len = 0, msgLen = 0, _pos = 0;
  bool isArg = false;
  bool isUnsigned = false;
  char converter[21];
  SecureVector<Botan::byte> *secVec;
  int32 i;

  if (channel < -1 || !format)
    return false;

  converter[0] = 0x00;
  len = strlen (format);

  va_start (args, format);
  errStr = (char*) malloc (len + 1);

  do
  {
    if (*format == '%' || isArg)
    {
      switch (*format)
      {
        case '%':
          isArg = true;
          break;

        case 'u':
          isUnsigned = true;
          break;

        case 's':
          s = va_arg (args, char*);
          msgLen = strlen (s);
          if (msgLen > MAX_ERROR_LEN) msgLen = MAX_ERROR_LEN;
          errStr = (char*) realloc (errStr, len + msgLen + 1);
          memcpy ((errStr + _pos), s, msgLen);
          if (isUnsigned) len += msgLen - 3;
          else len += msgLen - 2;
          _pos += msgLen;
          isUnsigned = false;
          isArg = false;
          break;

        case 'B':
          secVec = va_arg (args, SecureVector<Botan::byte>*);
	  msgLen = secVec->size();
          if (msgLen > MAX_ERROR_LEN) msgLen = MAX_ERROR_LEN;
          errStr = (char*) realloc (errStr, len + msgLen + 1);
          memcpy ((errStr + _pos), secVec->begin(), msgLen);
          if (isUnsigned) len += msgLen - 3;
          else len += msgLen - 2;
          _pos += msgLen;
          isUnsigned = false;
          isArg = false;
          break;

        case 'l':
        case 'd':
        case 'i':
          i = va_arg (args, int32);
          if (isUnsigned) sprintf (converter, "%u", i);
          else sprintf (converter, "%d", i);
          msgLen = strlen (converter);
          errStr = (char*) realloc (errStr, len + msgLen + 1);
          memcpy ((errStr + _pos), converter, msgLen);
          if (isUnsigned) len += msgLen - 3;
          else len += msgLen - 2;
          _pos += msgLen;
          isUnsigned = false;
          isArg = false;
          break;

        case 'x':
          i = va_arg (args, int32);
          sprintf (converter, "%x", i);
          msgLen = strlen (converter);
          errStr = (char*) realloc (errStr, len + msgLen + 1);
          memcpy ((errStr + _pos), converter, msgLen);
          if (isUnsigned) len += msgLen - 3;
          else len += msgLen - 2;
          _pos += msgLen;
          isUnsigned = false;
          isArg = false;
          break;
      }
    }
    else errStr[_pos++] = *format;

  } while (*format++);

  va_end (args);

  if (!lock()) return false;
  if (!memberCount) 
  {
    ErrorBuffer = (Error**) malloc (sizeof(Error*));
    ErrorBuffer[0] = (Error*) malloc (sizeof(Error));
  }
  else
  {
    ErrorBuffer = (Error**) realloc (ErrorBuffer, sizeof(Error*) * (memberCount + 1));
    ErrorBuffer[memberCount] = (Error*) malloc (sizeof(Error));
  }
		
  ErrorBuffer[memberCount]->channel = channel;
  ErrorBuffer[memberCount]->errorStr = errStr;
  memberCount++;
  if (!unlock()) return false;
  return true;
}

const char* Ne7sshError::pop ()
{
  return pop(-1);
}

const char* Ne7sshError::pop (int32 channel)
{
  uint16 i;
  int32 recID = -1;
  const char* result = 0;
  uint32 len;

  if (!memberCount) return 0;
  if (!lock()) return false;

  for (i = 0; i < memberCount; i++)
  {
    if (ErrorBuffer[i] && ErrorBuffer[i]->channel == channel)
    {
      recID = i;
      result = ErrorBuffer[i]->errorStr;
    }
  }
  if (recID < 0)
  {
    unlock();
    return 0;
  }

  if (result)
  {
    len = strlen (result) < MAX_ERROR_LEN ? strlen (result) : MAX_ERROR_LEN;
    memcpy (popedErr, result, len + 1);
    deleteRecord (recID);
  }
  else 
  {
    unlock();
    return 0;
  }
  if (!unlock()) return false;

  return popedErr;
}

bool Ne7sshError::deleteRecord (uint16 recID)
{
  uint16 i;

  if (ErrorBuffer[recID] && ErrorBuffer[recID]->errorStr) free (ErrorBuffer[recID]->errorStr);
  else return false;
  free (ErrorBuffer[recID]);
  ErrorBuffer[recID] = 0;
  for (i = recID + 1; i < memberCount; i++)
  {
    ErrorBuffer[i - 1] = ErrorBuffer[i];
  }
  memberCount--;
  return true;
}


bool Ne7sshError::deleteCoreMsgs ()
{
  return deleteChannel(-1);
}

bool Ne7sshError::deleteChannel (int32 channel)
{
  uint16 i, offset=0;

  if (!lock()) return false;
  for (i = 0; i < memberCount; i++)
  {
    if (ErrorBuffer[i] && ErrorBuffer[i]->channel == channel)
    {
      if (ErrorBuffer[i]->errorStr) free (ErrorBuffer[i]->errorStr);
      free (ErrorBuffer[i]);
      offset++;
    }
    else if (offset) ErrorBuffer[i - offset] = ErrorBuffer[i];
  }
  memberCount -= offset;
  if (!unlock()) return false;
  return true;
}

bool Ne7sshError::lock ()
{
  int status;
  status = Ne7sshError::mut.lock();
  if (status)
  {
    /// FIXME possible infinite loop
    ne7ssh::errors()->push (-1, "Could not aquire a mutex lock. Error: %i.", status);
    usleep (1000);
    return false;
  }
  return true;
}

bool Ne7sshError::unlock ()
{
  int status;
  status = Ne7sshError::mut.unlock();
  if (status)
  {
    ne7ssh::errors()->push (-1, "Error while releasing a mutex lock. Error: %i.", status);
    return false;
  }
  return true;
}
