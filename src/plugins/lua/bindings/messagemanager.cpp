// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../luaengine.h"

#include <coreplugin/messagemanager.h>

namespace Lua::Internal {

void addMessageManagerModule()
{
    LuaEngine::registerProvider("MessageManager", [](sol::state_view lua) -> sol::object {
        sol::table mm = lua.create_table();

        mm.set_function("writeFlashing", [](const sol::variadic_args &vargs) {
            Core::MessageManager::writeFlashing(LuaEngine::variadicToStringList(vargs).join(""));
        });
        mm.set_function("writeDisrupting", [](const sol::variadic_args &vargs) {
            Core::MessageManager::writeDisrupting(LuaEngine::variadicToStringList(vargs).join(""));
        });
        mm.set_function("writeSilently", [](const sol::variadic_args &vargs) {
            Core::MessageManager::writeSilently(LuaEngine::variadicToStringList(vargs).join(""));
        });

        return mm;
    });
}

} // namespace Lua::Internal
