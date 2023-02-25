// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "terminalhooks.h"

#include "filepath.h"
#include "terminalprocess_p.h"

namespace Utils::Terminal {

struct HooksPrivate
{
    HooksPrivate()
        : m_openTerminalHook([](const OpenTerminalParameters &parameters) {
            DeviceFileHooks::instance().openTerminal(parameters.workingDirectory.value_or(
                                                         FilePath{}),
                                                     parameters.environment.value_or(Environment{}));
        })
        , m_createTerminalProcessInterfaceHook(
              []() -> ProcessInterface * { return new Internal::TerminalImpl(); })
        , m_getTerminalCommandsForDevicesHook([]() -> QList<NameAndCommandLine> { return {}; })
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
