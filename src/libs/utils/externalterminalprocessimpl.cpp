// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "externalterminalprocessimpl.h"
#include "process.h"
#include "terminalcommand.h"
#include "utilstr.h"

#include <QTemporaryFile>

namespace Utils {

ExternalTerminalProcessImpl::ExternalTerminalProcessImpl()
{
    setStubCreator(new ProcessStubCreator(this));
}

ProcessStubCreator::ProcessStubCreator(TerminalInterface *interface)
    : m_interface(interface)
{}

expected_str<qint64> ProcessStubCreator::startStubProcess(const ProcessSetupData &setupData)
{
    const TerminalCommand terminal = TerminalCommand::terminalEmulator();

    if (HostOsInfo::isMacHost() && terminal.command == "Terminal.app") {
        QTemporaryFile f;
        f.setAutoRemove(false);
        f.open();
        f.setPermissions(QFile::ExeUser | QFile::ReadUser | QFile::WriteUser);
        f.write("#!/bin/sh\n");
        f.write(QString("cd %1\n").arg(setupData.m_workingDirectory.nativePath()).toUtf8());
        f.write("clear\n");
        f.write(QString("exec '%1' %2\n")
                    .arg(setupData.m_commandLine.executable().nativePath())
                    .arg(setupData.m_commandLine.arguments())
                    .toUtf8());
        f.close();

        const QString path = f.fileName();
        const QString exe
            = QString("tell app \"Terminal\" to do script \"'%1'; rm -f '%1'; exit\"").arg(path);

        Process process;

        process.setCommand({"osascript", {"-e", "tell app \"Terminal\" to activate", "-e", exe}});
        process.runBlocking();

        if (process.exitCode() != 0) {
            return make_unexpected(
                Tr::tr("Failed to start terminal process: \"%1\"").arg(process.errorString()));
        }

        return 0;
    }

    bool detached = setupData.m_terminalMode == TerminalMode::Detached;

    Process *process = new Process(detached ? nullptr : this);
    if (detached)
        QObject::connect(process, &Process::done, process, &Process::deleteLater);

    QObject::connect(process, &Process::done, m_interface, &TerminalInterface::onStubExited);

    process->setWorkingDirectory(setupData.m_workingDirectory);

    if constexpr (HostOsInfo::isWindowsHost()) {
        process->setCommand(setupData.m_commandLine);
        process->setCreateConsoleOnWindows(true);
        process->setProcessMode(ProcessMode::Writer);
    } else {
        QString extraArgsFromOptions = terminal.executeArgs;
        CommandLine cmdLine = {terminal.command, {}};
        if (!extraArgsFromOptions.isEmpty())
            cmdLine.addArgs(extraArgsFromOptions, CommandLine::Raw);
        cmdLine.addCommandLineAsArgs(setupData.m_commandLine, CommandLine::Raw);
        process->setCommand(cmdLine);
    }

    process->start();
    process->waitForStarted();
    if (process->error() != QProcess::UnknownError) {
        return make_unexpected(
            Tr::tr("Failed to start terminal process: \"%1\"").arg(process->errorString()));
    }

    qint64 pid = process->processId();

    return pid;
}

} // namespace Utils
