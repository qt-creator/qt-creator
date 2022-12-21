// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

namespace Utils {

class FilePath;

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
QTCREATOR_UTILS_EXPORT bool is64BitWindowsBinary(const FilePath &binary);

// Get the path to the executable for a given PID.
QTCREATOR_UTILS_EXPORT QString imageName(quint32 processId);

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
