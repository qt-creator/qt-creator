// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "terminalcommand.h"

#include "algorithm.h"
#include "commandline.h"
#include "environment.h"
#include "hostosinfo.h"
#include "layoutbuilder.h"
#include "qtcsettings.h"
#include "utilstr.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QMenu>
#include <QPushButton>
#include <QStandardItem>
#include <QToolButton>

namespace Utils {

TerminalCommand::TerminalCommand(const FilePath &command, const QString &openArgs,
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

Q_GLOBAL_STATIC_WITH_ARGS(const QList<TerminalCommand>, knownTerminals, (
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
    {"gnome-terminal", "", "--"},
    {"terminator", "", "-e", true},
}));

TerminalCommand TerminalCommand::defaultTerminalEmulator()
{
    static TerminalCommand defaultTerm;

    if (defaultTerm.command.isEmpty()) {
        if (HostOsInfo::isMacHost()) {
            return {"com.apple.Terminal", "", ""};
        } else if (HostOsInfo::isAnyUnixHost()) {
            defaultTerm = {"xterm", "", "-e"};
            const Environment env = Environment::systemEnvironment();
            for (const TerminalCommand &term : *knownTerminals) {
                const FilePath result = env.searchInPath(term.command.path());
                if (!result.isEmpty()) {
                    defaultTerm = {result, term.openArgs, term.executeArgs, term.needsQuotes};
                    break;
                }
            }
        }
    }

    return defaultTerm;
}

QList<TerminalCommand> TerminalCommand::availableTerminalEmulators()
{
    QList<TerminalCommand> result;

    if (HostOsInfo::isAnyUnixHost()) {
        const Environment env = Environment::systemEnvironment();
        for (const TerminalCommand &term : *knownTerminals) {
            const FilePath command = env.searchInPath(term.command.path());
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

const char kTerminalCommandKey[] = "General/Terminal/Command";
const char kTerminalOpenOptionsKey[] = "General/Terminal/OpenOptions";
const char kTerminalExecuteOptionsKey[] = "General/Terminal/ExecuteOptions";

TerminalCommand TerminalCommand::terminalEmulator()
{
    TerminalCommand cmd;
    QtcSettings &settings = Utils::userSettings();
    if (HostOsInfo::isAnyUnixHost() && settings.contains(kTerminalCommandKey)) {
        FilePath command = FilePath::fromSettings(settings.value(kTerminalCommandKey));

        const TerminalCommand knownCommand = Utils::findOrDefault(
            *knownTerminals(), [fileName = command.fileName()](const TerminalCommand &known) {
                return known.command.fileName() == fileName;
            });
        cmd = {command,
               settings.value(kTerminalOpenOptionsKey).toString(),
               settings.value(kTerminalExecuteOptionsKey).toString(),
               knownCommand.needsQuotes};
    } else {
        cmd = defaultTerminalEmulator();
    }

    // Special handling for the "terminator" application, which may not work when invoked
    // via a generic symlink. See QTCREATORBUG-32111.
    if (cmd.command.fileName() == "x-terminal-emulator") {
        const FilePath canonicalCommand = cmd.command.canonicalPath();
        if (canonicalCommand.fileName() == "terminator") {
            cmd.command = canonicalCommand;
            cmd.needsQuotes = true;
        }
    }

    return cmd;
}

static QString msgTerminalHereAction()
{
    if (HostOsInfo::isWindowsHost())
        return Tr::tr("Open Command Prompt Here");
    return Tr::tr("Open Terminal Here");
}

TerminalCommandAspect::TerminalCommandAspect(AspectContainer *parentContainer)
    : AspectContainer(parentContainer)
{
    terminalEmulator.setSettingsKey(kTerminalCommandKey);
    terminalEmulator.setToolTip(Tr::tr("The terminal emulator to use for opening a terminal."));
    terminalEmulator.setLabelText(Tr::tr("Command:"));
    terminalEmulator.setExpectedKind(PathChooser::ExistingCommand);
    terminalEmulator.setDefaultPathValue(TerminalCommand::defaultTerminalEmulator().command);

    terminalOpenArgs.setSettingsKey(kTerminalOpenOptionsKey);
    terminalOpenArgs.setToolTip(
        Tr::tr("Command line arguments used for \"%1\".").arg(msgTerminalHereAction()));
    terminalOpenArgs.setLabelText(Tr::tr("\"%1\" arguments:").arg(msgTerminalHereAction()));
    terminalOpenArgs.setDisplayStyle(StringAspect::LineEditDisplay);
    terminalOpenArgs.setDefaultValue(TerminalCommand::defaultTerminalEmulator().openArgs);

    terminalExecuteArgs.setSettingsKey(kTerminalExecuteOptionsKey);
    terminalExecuteArgs.setToolTip(Tr::tr("Command line arguments used for \"Run in terminal\"."));
    terminalExecuteArgs.setLabelText(Tr::tr("\"Run in Terminal\" arguments:"));
    terminalExecuteArgs.setDisplayStyle(StringAspect::LineEditDisplay);
    terminalExecuteArgs.setDefaultValue(TerminalCommand::defaultTerminalEmulator().executeArgs);
}

void TerminalCommandAspect::addToLayoutImpl(Layouting::Layout &parent)
{
    using namespace Layouting;

    auto detailLabel = new ElidingLabel;
    auto updateDetails = [this, detailLabel]() {
        const FilePath exe = terminalEmulator.expandedVolatileValue();
        detailLabel->setText(
            QString("%1: %2, %3: %4")
                .arg(msgTerminalHereAction())
                .arg(CommandLine(exe, terminalOpenArgs.volatileValue(), CommandLine::Raw)
                         .toUserOutput())
                .arg(Tr::tr("Run in Terminal"))
                .arg(CommandLine(exe, terminalExecuteArgs.volatileValue(), CommandLine::Raw)
                         .toUserOutput()));
    };
    updateDetails();
    connect(&terminalEmulator, &BaseAspect::volatileValueChanged, detailLabel, updateDetails);
    connect(&terminalOpenArgs, &BaseAspect::volatileValueChanged, detailLabel, updateDetails);
    connect(&terminalExecuteArgs, &BaseAspect::volatileValueChanged, detailLabel, updateDetails);

    QPushButton *changeButton = new QPushButton(Tr::tr("Customize..."));
    changeButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

    QPushButton *presetButton = new QPushButton(Tr::tr("Presets"));
    auto presetMenu = new QMenu(presetButton);
    presetButton->setMenu(presetMenu);
    presetButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

    for (const TerminalCommand &term : TerminalCommand::availableTerminalEmulators()) {
        QAction *action = presetMenu->addAction(term.command.toUserOutput());
        connect(action, &QAction::triggered, this, [this, term] {
            terminalEmulator.setVolatileValue(term.command.toUrlishString());
            terminalOpenArgs.setVolatileValue(term.openArgs);
            terminalExecuteArgs.setVolatileValue(term.executeArgs);
        });
    }

    connect(changeButton, &QPushButton::clicked, this, [this] {
        auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok);

        // clang-format off
        auto layout = Column {
            Form {
                terminalEmulator, br,
                terminalOpenArgs, br,
                terminalExecuteArgs, br,
            },
            buttons
        };
        // clang-format on

        QDialog *dialog = new QDialog(Utils::dialogParent());
        dialog->setWindowTitle(Tr::tr("Select Terminal Emulator"));
        dialog->setModal(true);
        layout.attachTo(dialog);
        connect(buttons, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
        dialog->show();
    });

    addLabeledItem(parent, detailLabel);
    parent.addItem(changeButton);
    parent.addItem(presetButton);
}

} // Utils
