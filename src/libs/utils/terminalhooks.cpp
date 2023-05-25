// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "terminalhooks.h"

#include "externalterminalprocessimpl.h"
#include "filepath.h"
#include "process.h"

#include <QMutex>

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
