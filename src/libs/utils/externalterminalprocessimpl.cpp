// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "externalterminalprocessimpl.h"

#include "algorithm.h"
#include "process.h"
#include "terminalcommand.h"
#include "utilstr.h"

#include <QLoggingCategory>
#include <QTemporaryFile>
#include <QVersionNumber>

Q_LOGGING_CATEGORY(log, "terminal.externalprocess", QtWarningMsg)

namespace Utils {

ExternalTerminalProcessImpl::ExternalTerminalProcessImpl()
{
    setStubCreator(new ProcessStubCreator(this));
}

ProcessStubCreator::ProcessStubCreator(TerminalInterface *interface)
    : m_interface(interface)
{}

static const QLatin1String TerminalAppScriptAttached{R"(
    tell application "Terminal"
        activate
        set newTab to do script "%1 && exit"
        set win to (the id of window 1 where its tab 1 = newTab) as text
        repeat until ((count of processes of newTab) = 0)
            delay 0.1
        end repeat
        close window id win
    end tell
)"};

static const QLatin1String TerminalAppScriptAttachedWithoutWindowClose{R"(
    tell application "Terminal"
        activate
        set newTab to do script "%1 && exit"
        repeat until ((count of processes of newTab) = 0)
            delay 0.1
        end repeat
    end tell
)"};

static const QLatin1String TerminalAppScriptDetached{R"(
    tell application "Terminal"
        activate
        do script "%1 && exit"
    end tell
)"};

struct AppScript
{
    QString attached;
    QString detached;
};

expected_str<qint64> ProcessStubCreator::startStubProcess(const ProcessSetupData &setupData)
{
    const TerminalCommand terminal = TerminalCommand::terminalEmulator();
    bool detached = setupData.m_terminalMode == TerminalMode::Detached;

    if (HostOsInfo::isMacHost()) {
        // There is a bug in macOS 14.0 where the script fails if it tries to find
        // the window id. We will have to check in future versions of macOS if they fixed
        // the issue.
        static const QVersionNumber osVersionNumber = QVersionNumber::fromString(
            QSysInfo::productVersion());

        static const QMap<QString, AppScript> terminalMap = {
            {"Terminal.app",
             {osVersionNumber.majorVersion() >= 14 ? TerminalAppScriptAttachedWithoutWindowClose
                                                   : TerminalAppScriptAttached,
              TerminalAppScriptDetached}},
        };

        if (terminalMap.contains(terminal.command.toString())) {
            const QString env
                = Utils::transform(setupData.m_environment.toStringList(), [](const QString &env) {
                      return CommandLine{"export", {env}}.toUserOutput();
                  }).join('\n');

            Process *process = new Process(detached ? nullptr : this);
            if (detached) {
                QObject::connect(process, &Process::done, process, &Process::deleteLater);
            }

            QTemporaryFile shFile;
            shFile.setAutoRemove(false);
            QTC_ASSERT(shFile.open(),
                       return make_unexpected(Tr::tr("Failed to open temporary script file.")));

            const QString shScript = QString("cd '%1'\n%2\nclear\n'%3' %4\nrm '%5'\n")
                                         .arg(setupData.m_workingDirectory.nativePath())
                                         .arg(env)
                                         .arg(setupData.m_commandLine.executable().nativePath())
                                         .arg(setupData.m_commandLine.arguments())
                                         .arg(shFile.fileName());

            shFile.write(shScript.toUtf8());
            shFile.close();

            FilePath::fromUserInput(shFile.fileName())
                .setPermissions(QFile::ExeUser | QFile::ExeGroup | QFile::ExeOther | QFile::ReadUser
                                | QFile::ReadGroup | QFile::ReadOther | QFile::WriteUser
                                | QFile::WriteGroup | QFile::WriteOther);

            const QString script = (detached
                                        ? terminalMap.value(terminal.command.toString()).detached
                                        : terminalMap.value(terminal.command.toString()).attached)
                                       .arg(shFile.fileName());

            process->setCommand({"osascript", {"-"}});
            process->setWriteData(script.toUtf8());
            process->start();

            if (!process->waitForStarted()) {
                return make_unexpected(
                    Tr::tr("Failed to start terminal process: \"%1\".").arg(process->errorString()));
            }

            QObject::connect(process, &Process::readyReadStandardOutput, process, [process] {
                const QString output = process->readAllStandardOutput();
                if (!output.isEmpty())
                    qCWarning(log).noquote() << output;
            });
            QObject::connect(process, &Process::readyReadStandardError, process, [process] {
                const QString output = process->readAllStandardError();
                if (!output.isEmpty())
                    qCCritical(log).noquote() << output;
            });

            QObject::connect(process, &Process::done, m_interface, &TerminalInterface::onStubExited);

            return 0;
        }
    }

    Process *process = new Process(detached ? nullptr : this);
    if (detached)
        QObject::connect(process, &Process::done, process, &Process::deleteLater);

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
    process->setEnvironment(
        setupData.m_environment.appliedToEnvironment(Environment::systemEnvironment()));

    process->setEnvironment(setupData.m_environment);

    process->start();
    process->waitForStarted();
    if (process->error() != QProcess::UnknownError) {
        return make_unexpected(
            Tr::tr("Failed to start terminal process: \"%1\".").arg(process->errorString()));
    }

    qint64 pid = process->processId();

    return pid;
}

} // namespace Utils
