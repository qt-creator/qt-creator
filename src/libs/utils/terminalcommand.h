// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "filepath.h"

#include <QList>
#include <QMetaType>

namespace Utils {

class Environment;
class QtcSettings;

class QTCREATOR_UTILS_EXPORT TerminalCommand
{
public:
    TerminalCommand() = default;
    TerminalCommand(const FilePath &command, const QString &openArgs,
                    const QString &executeArgs, bool needsQuotes = false);

    bool operator==(const TerminalCommand &other) const;
    bool operator<(const TerminalCommand &other) const;

    Utils::FilePath command;
    QString openArgs;
    QString executeArgs;
    bool needsQuotes = false;

    static void setSettings(QtcSettings *settings);
    static TerminalCommand defaultTerminalEmulator();
    static QList<TerminalCommand> availableTerminalEmulators();
    static TerminalCommand terminalEmulator();
    static void setTerminalEmulator(const TerminalCommand &term);
};

} // Utils

Q_DECLARE_METATYPE(Utils::TerminalCommand)
