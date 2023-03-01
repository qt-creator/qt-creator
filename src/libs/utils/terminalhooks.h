// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "commandline.h"
#include "environment.h"
#include "filepath.h"

#include <functional>
#include <memory>

namespace Utils {
class ProcessInterface;

template<typename R, typename... Params>
class Hook
{
public:
    using Callback = std::function<R(Params...)>;

public:
    Hook() = delete;
    Hook(const Hook &other) = delete;
    Hook(Hook &&other) = delete;
    Hook &operator=(const Hook &other) = delete;
    Hook &operator=(Hook &&other) = delete;

    explicit Hook(Callback defaultCallback) { set(defaultCallback); }

    void set(Callback cb) { m_callback = cb; }
    R operator()(Params &&...params) { return m_callback(std::forward<Params>(params)...); }

private:
    Callback m_callback;
};

namespace Terminal {
struct HooksPrivate;

enum class ExitBehavior { Close, Restart, Keep };

struct OpenTerminalParameters
{
    std::optional<CommandLine> shellCommand;
    std::optional<FilePath> workingDirectory;
    std::optional<Environment> environment;
    ExitBehavior m_exitBehavior{ExitBehavior::Close};
};

struct NameAndCommandLine
{
    QString name;
    CommandLine commandLine;
};

QTCREATOR_UTILS_EXPORT FilePath defaultShellForDevice(const FilePath &deviceRoot);

class QTCREATOR_UTILS_EXPORT Hooks
{
public:
    using OpenTerminalHook = Hook<void, const OpenTerminalParameters &>;
    using CreateTerminalProcessInterfaceHook = Hook<ProcessInterface *>;
    using GetTerminalCommandsForDevicesHook = Hook<QList<NameAndCommandLine>>;

public:
    static Hooks &instance();
    ~Hooks();

    OpenTerminalHook &openTerminalHook();
    CreateTerminalProcessInterfaceHook &createTerminalProcessInterfaceHook();
    GetTerminalCommandsForDevicesHook &getTerminalCommandsForDevicesHook();

private:
    Hooks();
    std::unique_ptr<HooksPrivate> d;
};

} // namespace Terminal
} // namespace Utils
