// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../luaengine.h"

#include <utils/macroexpander.h>

using namespace Utils;

namespace Lua::Internal {

void setupMacroModule()
{
    registerProvider("Macro", [](sol::state_view lua) {
        sol::table module = lua.create_table();

        module.new_usertype<MacroExpander>(
            "MacroExpander",
            sol::no_constructor,
            "expand",
            [](MacroExpander *self, const QString &str) { return self->expand(str); },
            "value",
            [](MacroExpander *self, const QByteArray &str) {
                bool found = false;
                const QString res = self->value(str, &found);
                return std::make_pair(found, res);
            });

        module.set_function("globalExpander", [] { return globalMacroExpander(); });

        module.set_function("expand", [](const QString &s) {
            return globalMacroExpander()->expand(s);
        });

        module.set_function("value", [](const QString &s) {
            bool found = false;
            const QString res = globalMacroExpander()->value(s.toUtf8(), &found);
            return std::make_pair(found, res);
        });

        return module;
    });
}
} // namespace Lua::Internal
