/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "utils_global.h"

namespace Utils {

// Helper to format a Windows error message, taking the
// code as returned by the GetLastError()-API.
QTCREATOR_UTILS_EXPORT QString winErrorMessage(unsigned long error);

// Determine a DLL version
enum WinDLLVersionType { WinDLLFileVersion, WinDLLProductVersion };
QTCREATOR_UTILS_EXPORT QString winGetDLLVersion(WinDLLVersionType t,
                                                const QString &name,
                                                QString *errorMessage);

QTCREATOR_UTILS_EXPORT bool is64BitWindowsSystem();

// Check for a 64bit binary.
QTCREATOR_UTILS_EXPORT bool is64BitWindowsBinary(const QString &binary);

//
// RAII class to temporarily prevent windows crash messages from popping up using the
// application-global (!) error mode.
//
// Useful primarily for QProcess launching, since the setting will be inherited.
//
class QTCREATOR_UTILS_EXPORT WindowsCrashDialogBlocker {
public:
    WindowsCrashDialogBlocker();
    ~WindowsCrashDialogBlocker();
#ifdef Q_OS_WIN
private:
    const unsigned int silenceErrorMode;
    const unsigned int originalErrorMode;
#endif // Q_OS_WIN
};

} // namespace Utils
