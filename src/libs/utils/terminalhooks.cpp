// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "terminalhooks.h"

#include "filepath.h"
#include "process.h"
#include "terminalcommand.h"
#include "terminalinterface.h"
#include "utilstr.h"

#include <QMutex>
#include <QTemporaryFile>

namespace Utils::Terminal {

FilePath defaultShellForDevice(const FilePath &deviceRoot)
{
    if (deviceRoot.osType() == OsTypeWindows)
        return deviceRoot.withNewPath("cmd.exe").searchInPath();

    const Environment env = deviceRoot.deviceEnvironment();
    FilePath shell = FilePath::fromUserInput(env.value_or("SHELL", "/bin/sh"));

    if (!shell.isAbsolutePath())
        shell = env.searchInPath(shell.nativePath());

    if (shell.isEmpty())
        return shell;

    return deviceRoot.withNewMappedPath(shell);
}

class ExternalTerminalProcessImpl final : public TerminalInterface
{
    class ProcessStubCreator : public StubCreator
    {
    public:
        ProcessStubCreator(ExternalTerminalProcessImpl *interface)
            : m_interface(interface)
        {}

        ~ProcessStubCreator() override = default;

        expected_str<qint64> startStubProcess(const CommandLine &cmd,
                                              const ProcessSetupData &setupData) override
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
                            .arg(cmd.executable().nativePath())
                            .arg(cmd.arguments())
                            .toUtf8());
                f.close();

                const QString path = f.fileName();
                const QString exe
                    = QString("tell app \"Terminal\" to do script \"'%1'; rm -f '%1'; exit\"")
                          .arg(path);

                Process process;

                process.setCommand(
                    {"osascript", {"-e", "tell app \"Terminal\" to activate", "-e", exe}});
                process.runBlocking();

                if (process.exitCode() != 0) {
                    return make_unexpected(Tr::tr("Failed to start terminal process: \"%1\"")
                                               .arg(process.errorString()));
                }

                return 0;
            }

            bool detached = setupData.m_terminalMode == TerminalMode::Detached;

            Process *process = new Process(detached ? nullptr : this);
            if (detached)
                QObject::connect(process, &Process::done, process, &Process::deleteLater);

            QObject::connect(process,
                             &Process::done,
                             m_interface,
                             &ExternalTerminalProcessImpl::onStubExited);

            process->setWorkingDirectory(setupData.m_workingDirectory);

            if constexpr (HostOsInfo::isWindowsHost()) {
                process->setCommand(cmd);
                process->setCreateConsoleOnWindows(true);
                process->setProcessMode(ProcessMode::Writer);
            } else {
                QString extraArgsFromOptions = detached ? terminal.openArgs : terminal.executeArgs;
                CommandLine cmdLine = {terminal.command, {}};
                if (!extraArgsFromOptions.isEmpty())
                    cmdLine.addArgs(extraArgsFromOptions, CommandLine::Raw);
                cmdLine.addCommandLineAsArgs(cmd, CommandLine::Raw);
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

        ExternalTerminalProcessImpl *m_interface;
    };

public:
    ExternalTerminalProcessImpl() { setStubCreator(new ProcessStubCreator(this)); }
};

class HooksPrivate
{
public:
    HooksPrivate()
        : m_getTerminalCommandsForDevicesHook([] { return QList<NameAndCommandLine>{}; })
    {
        auto openTerminal = [](const OpenTerminalParameters &parameters) {
            DeviceFileHooks::instance().openTerminal(parameters.workingDirectory.value_or(
                                                         FilePath{}),
                                                     parameters.environment.value_or(Environment{}));
        };
        auto createProcessInterface = []() { return new ExternalTerminalProcessImpl(); };

        addCallbackSet("External", {openTerminal, createProcessInterface});
    }

    void addCallbackSet(const QString &name, const Hooks::CallbackSet &callbackSet)
    {
        QMutexLocker lk(&m_mutex);
        m_callbackSets.push_back(qMakePair(name, callbackSet));

        m_createTerminalProcessInterface
            = m_callbackSets.back().second.createTerminalProcessInterface;
        m_openTerminal = m_callbackSets.back().second.openTerminal;
    }

    void removeCallbackSet(const QString &name)
    {
        if (name == "External")
            return;

        QMutexLocker lk(&m_mutex);
        m_callbackSets.removeIf([name](const auto &pair) { return pair.first == name; });

        m_createTerminalProcessInterface
            = m_callbackSets.back().second.createTerminalProcessInterface;
        m_openTerminal = m_callbackSets.back().second.openTerminal;
    }

    Hooks::CreateTerminalProcessInterface createTerminalProcessInterface()
    {
        QMutexLocker lk(&m_mutex);
        return m_createTerminalProcessInterface;
    }

    Hooks::OpenTerminal openTerminal()
    {
        QMutexLocker lk(&m_mutex);
        return m_openTerminal;
    }

    Hooks::GetTerminalCommandsForDevicesHook m_getTerminalCommandsForDevicesHook;

private:
    Hooks::OpenTerminal m_openTerminal;
    Hooks::CreateTerminalProcessInterface m_createTerminalProcessInterface;

    QMutex m_mutex;
    QList<QPair<QString, Hooks::CallbackSet>> m_callbackSets;
};

Hooks &Hooks::instance()
{
    static Hooks manager;
    return manager;
}

Hooks::Hooks()
    : d(new HooksPrivate())
{}

Hooks::~Hooks() = default;

void Hooks::openTerminal(const OpenTerminalParameters &parameters) const
{
    d->openTerminal()(parameters);
}

ProcessInterface *Hooks::createTerminalProcessInterface() const
{
    return d->createTerminalProcessInterface()();
}

Hooks::GetTerminalCommandsForDevicesHook &Hooks::getTerminalCommandsForDevicesHook()
{
    return d->m_getTerminalCommandsForDevicesHook;
}

void Hooks::addCallbackSet(const QString &name, const CallbackSet &callbackSet)
{
    d->addCallbackSet(name, callbackSet);
}

void Hooks::removeCallbackSet(const QString &name)
{
    d->removeCallbackSet(name);
}

} // namespace Utils::Terminal
