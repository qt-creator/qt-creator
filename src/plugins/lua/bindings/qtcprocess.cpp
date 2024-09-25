// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../luaengine.h"

#include "utils.h"

#include <utils/environment.h>
#include <utils/qtcprocess.h>

using namespace Utils;

namespace Lua::Internal {

void setupProcessModule()
{
    registerProvider("Process", [](sol::state_view lua) -> sol::object {
        const ScriptPluginSpec *pluginSpec = lua.get<ScriptPluginSpec *>("PluginSpec");
        QObject *guard = pluginSpec->connectionGuard.get();

        sol::table async = lua.script("return require('async')", "_process_").get<sol::table>();
        sol::function wrap = async["wrap"];

        sol::table process = lua.create_table();

        process["runInTerminal_cb"] =
            [guard
             = pluginSpec->connectionGuard.get()](const QString &cmdline, const sol::function &cb) {
                Process *p = new Process;
                p->setTerminalMode(TerminalMode::Run);
                p->setCommand(CommandLine::fromUserInput((cmdline)));
                p->setEnvironment(Environment::systemEnvironment());

                QObject::connect(p, &Process::done, guard, [p, cb]() { cb(p->exitCode()); });
            };

        process["commandOutput_cb"] =
            [guard
             = pluginSpec->connectionGuard.get()](const QString &cmdline, const sol::function &cb) {
                Process *p = new Process;
                p->setCommand(CommandLine::fromUserInput((cmdline)));
                p->setEnvironment(Environment::systemEnvironment());

                QObject::connect(p, &Process::done, guard, [p, cb]() { cb(p->allOutput()); });

                p->start();
            };

        process["runInTerminal"] = wrap(process["runInTerminal_cb"]);
        process["commandOutput"] = wrap(process["commandOutput_cb"]);

        process["create"] = [](const sol::table &parameter) {
            const auto cmd = toFilePath(parameter.get<std::variant<FilePath, QString>>("command"));

            const QStringList arguments
                = parameter.get_or<QStringList, const char *, QStringList>("arguments", {});
            const std::optional<FilePath> workingDirectory = parameter.get<std::optional<FilePath>>(
                "workingDirectory");

            const auto stdOut = parameter.get<std::optional<sol::function>>("stdout");
            const auto stdErr = parameter.get<std::optional<sol::function>>("stderr");
            const auto stdIn = parameter.get<sol::optional<QString>>("stdin");

            auto p = std::make_unique<Process>();

            p->setCommand({cmd, arguments});
            if (workingDirectory)
                p->setWorkingDirectory(*workingDirectory);
            if (stdIn)
                p->setWriteData(stdIn->toUtf8());
            if (stdOut) {
                // clang-format off
                QObject::connect(p.get(), &Process::readyReadStandardOutput,
                    p.get(),
                    [p = p.get(), cb = *stdOut]() {
                        void_safe_call(cb, p->readAllStandardOutput());
                    });
                // clang-format on
            }
            if (stdErr) {
                // clang-format off
                QObject::connect(p.get(), &Process::readyReadStandardError,
                    p.get(),
                    [p = p.get(), cb = *stdErr]() {
                        void_safe_call(cb, p->readAllStandardError());
                    });
                // clang-format on
            }
            return p;
        };

        process.new_usertype<Process>(
            "Process",
            sol::no_constructor,
            "start_cb",
            [guard](Process *process, sol::function callback) {
                if (process->state() != QProcess::NotRunning)
                    callback(false, "Process is already running");

                struct Connections
                {
                    QMetaObject::Connection startedConnection;
                    QMetaObject::Connection doneConnection;
                };
                std::shared_ptr<Connections> connections = std::make_shared<Connections>();

                // clang-format off
                connections->startedConnection
                    = QObject::connect(process, &Process::started,
                        guard, [callback, process, connections]() {
                            process->disconnect(connections->doneConnection);
                            callback(true);
                        },
                        Qt::SingleShotConnection);
                connections->doneConnection
                    = QObject::connect(process, &Process::done,
                        guard, [callback, process, connections]() {
                            process->disconnect(connections->startedConnection);
                            callback(false, process->errorString());
                        },
                        Qt::SingleShotConnection);
                // clang-format on
                process->start();
            },
            "stop_cb",
            [](Process *process, sol::function callback) {
                if (process->state() != QProcess::Running)
                    callback(false, "Process is not running");

                // clang-format off
                QObject::connect(process, &Process::done,
                    process, [callback, process]() {
                        callback(true);
                        process->disconnect();
                    },
                    Qt::SingleShotConnection);
                // clang-format on
                process->stop();
            },
            "isRunning",
            &Process::isRunning);

        process["Process"]["start"] = wrap(process["Process"]["start_cb"]);
        process["Process"]["stop"] = wrap(process["Process"]["stop_cb"]);

        return process;
    });
}

} // namespace Lua::Internal
