/***************************************************************************
 *   Copyright (C) 2005-2006 by NetSieben Technologies INC                 *
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


#ifndef NE7SSH_MUTEX_H
#define NE7SSH_MUTEX_H

#if defined(WIN32) || defined(__MINGW32__)
#   include <windows.h>
#else
#   include <pthread.h>
#endif

class Ne7ssh_Mutex
{
public:
    Ne7ssh_Mutex();

    int lock();

    int unlock();

    ~Ne7ssh_Mutex();
private:
#if defined(WIN32) || defined(__MINGW32__)
    CRITICAL_SECTION mutint;
#else
    pthread_mutex_t mutint;
#endif
};

#endif
