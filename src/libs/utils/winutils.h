/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef WINUTILS_H
#define WINUTILS_H

#include "utils_global.h"

#include <QtCore/QProcess> // Q_PID (is PROCESS_INFORMATION*)

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

namespace Utils {

// Helper to format a Windows error message, taking the
// code as returned by the GetLastError()-API.
QTCREATOR_UTILS_EXPORT QString winErrorMessage(unsigned long error);

// Determine a DLL version
enum WinDLLVersionType { WinDLLFileVersion, WinDLLProductVersion };
QTCREATOR_UTILS_EXPORT QString winGetDLLVersion(WinDLLVersionType t,
                                                const QString &name,
                                                QString *errorMessage);

// Return the short (8.3) file name
QTCREATOR_UTILS_EXPORT QString getShortPathName(const QString &name,
                                                QString *errorMessage);

QTCREATOR_UTILS_EXPORT unsigned long winQPidToPid(const Q_PID qpid);

QTCREATOR_UTILS_EXPORT bool winIs64BitSystem();

// Check for a 64bit binary.
QTCREATOR_UTILS_EXPORT bool winIs64BitBinary(const QString &binary);

} // namespace Utils
#endif // WINUTILS_H
