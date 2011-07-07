/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef BOTAN_BUILD_CONFIG_H_QT
#define BOTAN_BUILD_CONFIG_H_QT

#include <qglobal.h>

#if defined(Q_OS_WIN)
#   include "build_windows.h"
#elif defined(Q_OS_UNIX)
#   include "build_unix.h"
#endif

#ifdef QT_ARCH_I386
#define BOTAN_TARGET_ARCH_IS_IA32
#define BOTAN_TARGET_CPU_IS_I686
#elif defined(QT_ARCH_X86_64)
#define BOTAN_TARGET_ARCH_IS_AMD64
#endif

#endif
