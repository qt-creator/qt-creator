// Copyright (C) 2026 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "remotedebuggerconfiguration.h"

#include "debuggerinternalconstants.h"
#include "debuggertr.h"

#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runconfigurationaspects.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Debugger::Internal {

class RemoteDebuggerConfiguration final : public RunConfiguration
{
public:
    RemoteDebuggerConfiguration(BuildConfiguration *bc, Id id);

    QString runConfigDefaultDisplayName();

private:
    Tasks checkForIssues() const final;

    ExecutableAspect executable{this};
    ArgumentsAspect arguments{this};
    SymbolFileAspect symbolFile{this};
    StringAspect gdbServerChannel{this};
    BoolAspect breakOnMain{this};
    TerminalAspect terminal{this};
    BoolAspect extended{this};
};

RemoteDebuggerConfiguration::RemoteDebuggerConfiguration(BuildConfiguration *bc, Id id)
    : RunConfiguration(bc, id)
{
    executable.setLabelText(Tr::tr("Launcher command:"));
    executable.setReadOnly(false);
    executable.setSettingsKey("RemoteDebugger.Executable");
    executable.setHistoryCompleter("RemoteDebugger.Executable.History");
    executable.setPlaceHolderText(
        Tr::tr("Script or command that will set up the gdb server connection"));
    executable.setExpectedKind(PathChooser::Command);

    arguments.setSettingsKey("RemoteDebugger.Arguments");

    symbolFile.setSettingsKey("RemoteDebugger.SymbolFile");
    symbolFile.setLabelText(Tr::tr("Symbol file (local executable):"));

    gdbServerChannel.setId(Constants::GdbServerAddressAspectId);
    gdbServerChannel.setSettingsKey("RemoteDebugger.GdbServerAddress");
    gdbServerChannel.setLabelText(Tr::tr("Server channel:"));
    gdbServerChannel.setDisplayStyle(StringAspect::LineEditDisplay);
    gdbServerChannel.setDefaultValue("tcp://127.0.0.1:1234");
    gdbServerChannel.setPlaceHolderText(Tr::tr("For example, %1").arg(":1234, /dev/ttyS0, COM1"));

    breakOnMain.setId(Constants::GdbServerBreakOnMainAspectId);
    breakOnMain.setSettingsKey("RemoteDebugger.BreakOnMain");
    breakOnMain.setLabelText(Tr::tr("Break at \"&main\""));

    terminal.setSettingsKey("RemoteDebugger.Terminal");

    extended.setId(Constants::GdbServerExtendedModeAspectId);
    extended.setSettingsKey("RemoteDebugger.ExtendedMode");
    //: %1 = "target extended-remote"
    extended.setLabelText(Tr::tr("Use \"%1\" to connect").arg("target extended-remote"));

    setDefaultDisplayName(runConfigDefaultDisplayName());
    setUsesEmptyBuildKeys();
}

QString RemoteDebuggerConfiguration::runConfigDefaultDisplayName()
{
    const FilePath exe = executable();
    return exe.isEmpty() ? Tr::tr("RemoteDebugger Runner") : Tr::tr("RemoteDebugger: %1").arg(exe.toUserOutput());
}

Tasks RemoteDebuggerConfiguration::checkForIssues() const
{
    Tasks tasks;
    if (executable().isEmpty())
        tasks << createConfigurationIssue(Tr::tr("The launcher command must be set in order to run."));
    if (symbolFile().isEmpty())
        tasks << createConfigurationIssue(Tr::tr("The symbol file must be set in order to debug."));
    if (gdbServerChannel().isEmpty())
        tasks << createConfigurationIssue(Tr::tr("The GDB server channel must be set in order to debug."));

    return tasks;
}

class RemoteDebuggerConfigurationFactory : public FixedRunConfigurationFactory
{
public:
    RemoteDebuggerConfigurationFactory()
        : FixedRunConfigurationFactory(Tr::tr("Remote Debugger"), true)
    {
        registerRunConfiguration<RemoteDebuggerConfiguration>(Constants::RunConfigId);
    }
};

void setupRemoteDebuggerConfiguration()
{
    static RemoteDebuggerConfigurationFactory theFactory;
}

} // Debugger::Internal
