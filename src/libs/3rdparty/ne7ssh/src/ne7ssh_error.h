/***************************************************************************
 *   Copyright (C) 2005-2007 by NetSieben Technologies INC		             *
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

#ifndef NE7SSH_ERROR_H
#define NE7SSH_ERROR_H

#include <stdlib.h>
#if !defined(WIN32) && !defined(__MINGW32__)
#   include <sys/select.h>
#endif

#define MAX_ERROR_LEN 500

#include "ne7ssh_types.h"
#include "ne7ssh_mutex.h"

/**
	@author Andrew Useckas <andrew@netsieben.com>
*/
class SSH_EXPORT Ne7sshError
{
  private:
    uint16 memberCount;
    char popedErr[MAX_ERROR_LEN + 1];
    static Ne7ssh_Mutex mut;

    /**
    * Structure for storing error messages.
    */
    struct Error
    {
      int32 channel;
      char* errorStr;
    } **ErrorBuffer;

    /**
    * Delete a single error message.
    * @param recID Position within the array.
    * @return True on success, false on failure.
    */
    bool deleteRecord (uint16 recID);

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

  public:
    /**
     * Ne7sshError constructor.
     */
    Ne7sshError();

    /**
     * Ne7sshError destructor.
     */
    ~Ne7sshError();

    /**
    * Pushes a new error message into the stack.
    * @param channel Specifies the channel to bind the error message to. This is ne7ssh library channel, not the receive or send channels used by the transport layer.
    * @param format Specifies the error message followed by argument in printf format. The following formatting characters are supported: %s,%d,%i,%l,%x. Modifier %u can be used together with decimal to specify an unsigned variable. Returns null if no there are no erros in the Core context.
    * @return True on success, false on failure.
    */
    bool push (int32 channel, const char* format, ...);

    /**
    * Pops an error message from the Core context.
    * @return The last error message in the Core context. The message is removed from the stack.
    */
    const char* pop ();

    /**
    * Pops an error message from the Channel context.
    * @param channel Specifies the channel error message was bound to. This is ne7ssh library channel, not the receive or send channels used by the transport layer.
    * @return The last error message in the Channel context. The message is removed from the stack. Returns null if no there are no erros in the Channel context.
    */
    const char* pop (int32 channel);

    /**
    * Removes all error messages within Core context from the stack.
    * @return True on success, false on failure.
    */
    bool deleteCoreMsgs ();

    /**
    * Removes all error messages within Channel context from the stack.
    * @param channel Specifies the channel error message was bound to. This is ne7ssh library channel, not the receive or send channels used by the transport layer.
    * @return True on succes, false on failure.
    */
    bool deleteChannel (int32 channel);

};

#endif
