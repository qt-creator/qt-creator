// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../luaengine.h"

#include <utils/environment.h>
#include <utils/qtcprocess.h>

using namespace Utils;

namespace Lua::Internal {

void addProcessModule()
{
    LuaEngine::registerProvider("__process", [](sol::state_view lua) -> sol::object {
        sol::table process = lua.create_table();

        process["runInTerminal_cb"] = [](const QString &cmdline, sol::function cb) {
            Process *p = new Process;
            p->setTerminalMode(TerminalMode::Run);
            p->setCommand(CommandLine::fromUserInput((cmdline)));
            p->setEnvironment(Environment::systemEnvironment());

            QObject::connect(p, &Process::done, [p, cb]() { cb(p->exitCode()); });

            p->start();
        };

        return process;
    });

    LuaEngine::registerProvider("Process", [](sol::state_view lua) -> sol::object {
        return lua
            .script(
                R"(
local p = require("__process")
local a = require("async")

return {
    runInTerminal_cb = p.runInTerminal_cb,
    runInTerminal = a.wrap(p.runInTerminal_cb)
}
)",
                "_process_")
            .get<sol::table>();
    });
}

} // namespace Lua::Internal
