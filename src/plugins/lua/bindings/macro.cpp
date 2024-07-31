// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../luaengine.h"

#include <utils/macroexpander.h>

namespace Lua::Internal {

void setupMacroModule()
{
    registerProvider("Macro", [](sol::state_view lua) {
        sol::table module = lua.create_table();

        module.set_function("expand", [](const QString &s) -> QString {
            return Utils::globalMacroExpander()->expand(s);
        });

        module.set_function("value", [](const QString &s) -> QString {
            return Utils::globalMacroExpander()->value(s.toUtf8());
        });

        return module;
    });
}
} // namespace Lua::Internal
