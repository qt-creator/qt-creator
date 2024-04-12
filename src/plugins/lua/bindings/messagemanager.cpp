// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../luaengine.h"

#include <coreplugin/messagemanager.h>

namespace Lua::Internal {

static QString variadicToString(sol::state_view lua, sol::variadic_args vargs)
{
    sol::function tostring = lua["tostring"];
    QStringList msg;
    for (auto v : vargs) {
        if (v.get_type() != sol::type::string) {
            lua_getglobal(lua.lua_state(), "tostring");
            v.push();
            if (lua_pcall(lua.lua_state(), 1, 1, 0) != LUA_OK) {
                msg.append("<invalid>");
                continue;
            }
            if (lua_isstring(lua.lua_state(), -1) != 1) {
                msg.append("<invalid>");
                continue;
            }
            auto str = sol::stack::pop<QString>(lua.lua_state());
            msg.append(str);
        } else {
            msg.append(v.get<QString>());
        }
    }
    return msg.join("");
}

void addMessageManagerModule()
{
    LuaEngine::registerProvider("MessageManager", [](sol::state_view lua) -> sol::object {
        sol::table mm = lua.create_table();

        mm.set_function("writeFlashing", [](sol::variadic_args vargs, sol::this_state s) {
            sol::state_view lua(s);
            Core::MessageManager::writeFlashing(variadicToString(lua, vargs));
        });
        mm.set_function("writeDisrupting", [](sol::variadic_args vargs, sol::this_state s) {
            sol::state_view lua(s);
            Core::MessageManager::writeDisrupting(variadicToString(lua, vargs));
        });
        mm.set_function("writeSilently", [](sol::variadic_args vargs, sol::this_state s) {
            sol::state_view lua(s);
            Core::MessageManager::writeSilently(variadicToString(lua, vargs));
        });

        return mm;
    });
}

} // namespace Lua::Internal
