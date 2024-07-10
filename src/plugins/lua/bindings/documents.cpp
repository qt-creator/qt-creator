// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../luaengine.h"
#include "sol/sol.hpp"

#include "documents.h"
#include <texteditor/textdocument.h>

namespace Lua::Internal {

void addDocumentsModule()
{
    LuaEngine::registerProvider("Documents", [](sol::state_view lua) -> sol::object {
        sol::table documents = lua.create_table();

        documents.new_usertype<LuaTextDocument>(
            "LuaTextDocument",
            sol::no_constructor,
            "setChangedCallback",
            [](LuaTextDocument &self, sol::function callback) {
                self.setChangedCallback(callback);
            });

        return documents;
    });
}

} // namespace Lua::Internal

