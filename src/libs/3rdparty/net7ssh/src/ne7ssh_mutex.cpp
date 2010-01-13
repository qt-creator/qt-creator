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

#include "ne7ssh_mutex.h"

Ne7ssh_Mutex::Ne7ssh_Mutex()
{
#if defined(WIN32) || defined(__MINGW32__)
    InitializeCriticalSection(&mutint);
#else
    pthread_mutexattr_t mattr;

    pthread_mutexattr_init (&mattr);
    pthread_mutexattr_settype (&mattr, PTHREAD_MUTEX_ERRORCHECK);
    pthread_mutex_init (&mutint, &mattr);
    pthread_mutexattr_destroy (&mattr);
#endif
}

int Ne7ssh_Mutex::lock()
{
#if defined(WIN32) || defined(__MINGW32__)
    try
    {
        EnterCriticalSection(&mutint);
        return 0;
    }
    catch (...)
    {
        return -1;
    }
#else
    return pthread_mutex_lock(&mutint);
#endif
}

int Ne7ssh_Mutex::unlock()
{
#if defined(WIN32) || defined(__MINGW32__)
    LeaveCriticalSection(&mutint);
    return 0;
#else
    return pthread_mutex_unlock(&mutint);
#endif
}

Ne7ssh_Mutex::~Ne7ssh_Mutex()
{
#if defined(WIN32) || defined(__MINGW32__)
    DeleteCriticalSection(&mutint);
#else
    pthread_mutex_destroy(&mutint);
#endif
}
