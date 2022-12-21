// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <debugger/debugger_global.h>

#include <utils/filepath.h>

#include <QDebug>
#include <QMetaType>
#include <QString>

namespace Debugger {

class DEBUGGER_EXPORT DiagnosticLocation
{
public:
    DiagnosticLocation();
    DiagnosticLocation(const Utils::FilePath &filePath, int line, int column);

    bool isValid() const;

    DEBUGGER_EXPORT friend bool operator==(const DiagnosticLocation &first, const DiagnosticLocation &second);
    DEBUGGER_EXPORT friend bool operator<(const DiagnosticLocation &first, const DiagnosticLocation &second);
    DEBUGGER_EXPORT friend QDebug operator<<(QDebug dbg, const DiagnosticLocation &location);

    Utils::FilePath filePath;

    // Both values start at 1.
    int line = 0;
    int column = 0;
};

} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::DiagnosticLocation)
