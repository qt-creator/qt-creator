// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "commandline.h"
#include "environment.h"
#include "filepath.h"
#include "id.h"

#include <QIcon>

#include <functional>
#include <memory>

namespace Utils {
class ProcessInterface;

template<typename R, typename... Params>
class Hook
{
    Q_DISABLE_COPY_MOVE(Hook)

public:
    using Callback = std::function<R(Params...)>;

public:
    Hook() = delete;

    explicit Hook(Callback defaultCallback) { set(defaultCallback); }

    void set(Callback cb) { m_callback = cb; }
    R operator()(Params &&...params) { return m_callback(std::forward<Params>(params)...); }

private:
    Callback m_callback;
};

namespace Terminal {
class HooksPrivate;

enum class ExitBehavior { Close, Restart, Keep };

struct OpenTerminalParameters
{
    OpenTerminalParameters() = default;
    OpenTerminalParameters(const CommandLine &commandLine) : shellCommand(commandLine) {}
    OpenTerminalParameters(const FilePath &directory, std::optional<Environment> env) :
        workingDirectory(directory),
        environment(env)
    {}
    OpenTerminalParameters(const CommandLine &commandLine,
                           const FilePath &directory,
                           std::optional<Environment> env) :
        shellCommand(commandLine),
        workingDirectory(directory),
        environment(env)
    {}

    std::optional<CommandLine> shellCommand;
    std::optional<FilePath> workingDirectory;
    std::optional<Environment> environment;
    QIcon icon;
    ExitBehavior m_exitBehavior{ExitBehavior::Close};
    std::optional<Id> identifier{std::nullopt};
};

struct NameAndCommandLine
{
    QString name;
    CommandLine commandLine;
};

QTCREATOR_UTILS_EXPORT expected_str<FilePath> defaultShellForDevice(const FilePath &deviceRoot);

class QTCREATOR_UTILS_EXPORT Hooks
{
public:
    using OpenTerminal = std::function<void(const OpenTerminalParameters &)>;
    using CreateTerminalProcessInterface = std::function<ProcessInterface *()>;

    struct CallbackSet
    {
        OpenTerminal openTerminal;
        CreateTerminalProcessInterface createTerminalProcessInterface;
    };

public:
    static Hooks &instance();
    ~Hooks();

    void openTerminal(const OpenTerminalParameters &parameters) const;
    ProcessInterface *createTerminalProcessInterface() const;

    void addCallbackSet(const QString &name, const CallbackSet &callbackSet);
    void removeCallbackSet(const QString &name);

private:
    Hooks();
    std::unique_ptr<HooksPrivate> d;
};

} // namespace Terminal
} // namespace Utils
