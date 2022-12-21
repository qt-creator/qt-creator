// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QMetaType>
#include <QVector>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Utils {

class Environment;

class QTCREATOR_UTILS_EXPORT TerminalCommand
{
public:
    TerminalCommand() = default;
    TerminalCommand(const QString &command, const QString &openArgs,
                    const QString &executeArgs, bool needsQuotes = false);

    bool operator==(const TerminalCommand &other) const;
    bool operator<(const TerminalCommand &other) const;

    QString command;
    QString openArgs;
    QString executeArgs;
    bool needsQuotes = false;

    static void setSettings(QSettings *settings);
    static TerminalCommand defaultTerminalEmulator();
    static QVector<TerminalCommand> availableTerminalEmulators();
    static TerminalCommand terminalEmulator();
    static void setTerminalEmulator(const TerminalCommand &term);
};

} // Utils

Q_DECLARE_METATYPE(Utils::TerminalCommand)
