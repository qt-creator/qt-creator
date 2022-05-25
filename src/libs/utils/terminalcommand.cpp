/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "terminalcommand.h"

#include "algorithm.h"
#include "commandline.h"
#include "environment.h"
#include "hostosinfo.h"
#include "qtcassert.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QSettings>

namespace Utils {

static QSettings *s_settings = nullptr;

TerminalCommand::TerminalCommand(const QString &command, const QString &openArgs,
                                 const QString &executeArgs, bool needsQuotes)
    : command(command)
    , openArgs(openArgs)
    , executeArgs(executeArgs)
    , needsQuotes(needsQuotes)
{
}

bool TerminalCommand::operator==(const TerminalCommand &other) const
{
    return other.command == command && other.openArgs == openArgs
           && other.executeArgs == executeArgs;
}

bool TerminalCommand::operator<(const TerminalCommand &other) const
{
    if (command == other.command) {
        if (openArgs == other.openArgs)
            return executeArgs < other.executeArgs;
        return openArgs < other.openArgs;
    }
    return command < other.command;
}

void TerminalCommand::setSettings(QSettings *settings)
{
    s_settings = settings;
}

Q_GLOBAL_STATIC_WITH_ARGS(const QVector<TerminalCommand>, knownTerminals, (
{
    {"x-terminal-emulator", "", "-e"},
    {"xdg-terminal", "", "", true},
    {"xterm", "", "-e"},
    {"aterm", "", "-e"},
    {"Eterm", "", "-e"},
    {"rxvt", "", "-e"},
    {"urxvt", "", "-e"},
    {"xfce4-terminal", "", "-x"},
    {"konsole", "--separate --workdir .", "-e"},
    {"gnome-terminal", "", "--"}
}));

TerminalCommand TerminalCommand::defaultTerminalEmulator()
{
    static TerminalCommand defaultTerm;

    if (defaultTerm.command.isEmpty()) {
        if (HostOsInfo::isMacHost()) {
            const QString termCmd = QCoreApplication::applicationDirPath()
                            + "/../Resources/scripts/openTerminal.py";
            if (QFileInfo::exists(termCmd))
                defaultTerm = {termCmd, "", ""};
            else
                defaultTerm = {"/usr/X11/bin/xterm", "", "-e"};

        } else if (HostOsInfo::isAnyUnixHost()) {
            defaultTerm = {"xterm", "", "-e"};
            const Environment env = Environment::systemEnvironment();
            for (const TerminalCommand &term : *knownTerminals) {
                const QString result = env.searchInPath(term.command).toString();
                if (!result.isEmpty()) {
                    defaultTerm = {result, term.openArgs, term.executeArgs, term.needsQuotes};
                    break;
                }
            }
        }
    }

    return defaultTerm;
}

QVector<TerminalCommand> TerminalCommand::availableTerminalEmulators()
{
    QVector<TerminalCommand> result;

    if (HostOsInfo::isAnyUnixHost()) {
        const Environment env = Environment::systemEnvironment();
        for (const TerminalCommand &term : *knownTerminals) {
            const QString command = env.searchInPath(term.command).toString();
            if (!command.isEmpty())
                result.push_back({command, term.openArgs, term.executeArgs});
        }
        // sort and put default terminal on top
        const TerminalCommand defaultTerm = defaultTerminalEmulator();
        result.removeAll(defaultTerm);
        sort(result);
        result.prepend(defaultTerm);
    }

    return result;
}

const char kTerminalVersion[] = "4.8";
const char kTerminalVersionKey[] = "General/Terminal/SettingsVersion";
const char kTerminalCommandKey[] = "General/Terminal/Command";
const char kTerminalOpenOptionsKey[] = "General/Terminal/OpenOptions";
const char kTerminalExecuteOptionsKey[] = "General/Terminal/ExecuteOptions";

TerminalCommand TerminalCommand::terminalEmulator()
{
    if (s_settings && HostOsInfo::isAnyUnixHost()) {
        if (s_settings->value(kTerminalVersionKey).toString() == kTerminalVersion) {
            if (s_settings->contains(kTerminalCommandKey))
                return {s_settings->value(kTerminalCommandKey).toString(),
                                    s_settings->value(kTerminalOpenOptionsKey).toString(),
                                    s_settings->value(kTerminalExecuteOptionsKey).toString()};
        } else {
            // TODO remove reading of old settings some time after 4.8
            const QString value = s_settings->value("General/TerminalEmulator").toString().trimmed();
            if (!value.isEmpty()) {
                // split off command and options
                const QStringList splitCommand = ProcessArgs::splitArgs(value);
                if (QTC_GUARD(!splitCommand.isEmpty())) {
                    const QString command = splitCommand.first();
                    const QStringList quotedArgs = transform(splitCommand.mid(1),
                                                             &ProcessArgs::quoteArgUnix);
                    const QString options = quotedArgs.join(' ');
                    return {command, "", options};
                }
            }
        }
    }

    return defaultTerminalEmulator();
}

void TerminalCommand::setTerminalEmulator(const TerminalCommand &term)
{
    if (s_settings && HostOsInfo::isAnyUnixHost()) {
        s_settings->setValue(kTerminalVersionKey, kTerminalVersion);
        if (term == defaultTerminalEmulator()) {
            s_settings->remove(kTerminalCommandKey);
            s_settings->remove(kTerminalOpenOptionsKey);
            s_settings->remove(kTerminalExecuteOptionsKey);
        } else {
            s_settings->setValue(kTerminalCommandKey, term.command);
            s_settings->setValue(kTerminalOpenOptionsKey, term.openArgs);
            s_settings->setValue(kTerminalExecuteOptionsKey, term.executeArgs);
        }
    }
}

} // Utils
