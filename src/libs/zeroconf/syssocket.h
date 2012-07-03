/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef SYSSOCKET_H
#define SYSSOCKET_H

// WARNING:
//
// we use Winsock2, which means that this header should be included *before*
// any other windows header

#include <qglobal.h>

#ifdef Q_OS_WIN32
# include <winsock2.h>
# ifndef SHUT_RDWR
#  ifdef SD_BOTH
#    define SHUT_RDWR SD_BOTH
#  else
#    define SHUT_RDWR 2
#  endif
# endif
#else
# include <sys/socket.h>
#endif

#endif // SYSSOCKET_H
