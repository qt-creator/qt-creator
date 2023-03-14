// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "terminalhooks.h"

#include "filepath.h"
#include "qtcprocess.h"
#include "terminalcommand.h"
#include "terminalinterface.h"

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

    return shell.onDevice(deviceRoot);
}

class ExternalTerminalProcessImpl final : public TerminalInterface
{
    class ProcessStubCreator : public StubCreator
    {
    public:
        ProcessStubCreator(ExternalTerminalProcessImpl *interface)
            : m_interface(interface)
        {}

        void startStubProcess(const CommandLine &cmd, const ProcessSetupData &) override
        {
            const TerminalCommand terminal = TerminalCommand::terminalEmulator();

            if (HostOsInfo::isWindowsHost()) {
                m_terminalProcess.setCommand(cmd);
                QObject::connect(&m_terminalProcess, &QtcProcess::done, this, [this] {
                    m_interface->onStubExited();
                });
                m_terminalProcess.setCreateConsoleOnWindows(true);
                m_terminalProcess.setProcessMode(ProcessMode::Writer);
                m_terminalProcess.start();
            } else if (HostOsInfo::isMacHost() && terminal.command == "Terminal.app") {
                QTemporaryFile f;
                f.setAutoRemove(false);
                f.open();
                f.setPermissions(QFile::ExeUser | QFile::ReadUser | QFile::WriteUser);
                f.write("#!/bin/sh\n");
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

                m_terminalProcess.setCommand(
                    {"osascript", {"-e", "tell app \"Terminal\" to activate", "-e", exe}});
                m_terminalProcess.runBlocking();
            } else {
                CommandLine cmdLine = {terminal.command, {terminal.executeArgs}};
                cmdLine.addCommandLineAsArgs(cmd, CommandLine::Raw);

                m_terminalProcess.setCommand(cmdLine);
                m_terminalProcess.start();
            }
        }

        ExternalTerminalProcessImpl *m_interface;
        QtcProcess m_terminalProcess;
    };

public:
    ExternalTerminalProcessImpl() { setStubCreator(new ProcessStubCreator(this)); }
};

struct HooksPrivate
{
    HooksPrivate()
        : m_openTerminalHook([](const OpenTerminalParameters &parameters) {
            DeviceFileHooks::instance().openTerminal(parameters.workingDirectory.value_or(
                                                         FilePath{}),
                                                     parameters.environment.value_or(Environment{}));
        })
        , m_createTerminalProcessInterfaceHook([] { return new ExternalTerminalProcessImpl(); })
        , m_getTerminalCommandsForDevicesHook([] { return QList<NameAndCommandLine>{}; })
    {}

    Hooks::OpenTerminalHook m_openTerminalHook;
    Hooks::CreateTerminalProcessInterfaceHook m_createTerminalProcessInterfaceHook;
    Hooks::GetTerminalCommandsForDevicesHook m_getTerminalCommandsForDevicesHook;
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

Hooks::OpenTerminalHook &Hooks::openTerminalHook()
{
    return d->m_openTerminalHook;
}

Hooks::CreateTerminalProcessInterfaceHook &Hooks::createTerminalProcessInterfaceHook()
{
    return d->m_createTerminalProcessInterfaceHook;
}

Hooks::GetTerminalCommandsForDevicesHook &Hooks::getTerminalCommandsForDevicesHook()
{
    return d->m_getTerminalCommandsForDevicesHook;
}

} // namespace Utils::Terminal
