// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

class CrashHandlerSetup
{
public:
    enum RestartCapability { EnableRestart, DisableRestart };

    CrashHandlerSetup(const QString &appName,
                      RestartCapability restartCap = EnableRestart,
                      const QString &executableDirPath = QString());
    ~CrashHandlerSetup();
};
