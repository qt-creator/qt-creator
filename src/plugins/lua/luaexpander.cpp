// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "luaengine.h"
#include "luatr.h"

#include <utils/macroexpander.h>

using namespace Utils;

namespace Lua::Internal {

void setNext(
    MacroExpander *expander,
    sol::state &lua,
    auto &table,
    const QByteArray &key,
    QList<QByteArray>::const_iterator it,
    QList<QByteArray>::const_iterator end)
{
    if (it + 1 == end) {
        if (expander->isPrefixVariable(key)) {
            table.set_function(it->toStdString(), [key, expander](const QString &s) -> QString {
                return expander->value(key + s.toUtf8());
            });
        } else {
            table.set(it->toStdString(), expander->value(key));
        }
    } else {
        auto existingTable = table.template get<sol::optional<sol::table>>(it->toStdString());
        if (existingTable) {
            setNext(expander, lua, *existingTable, key, it + 1, end);
        } else {
            sol::table newTable = lua.create_table();
            setNext(expander, lua, newTable, key, it + 1, end);
            table.set(it->toStdString(), newTable);
        }
    }
};

sol::protected_function_result run(sol::state &lua, QString statement, MacroExpander *expander)
{
    return runFunction(lua, statement, "Statement", [expander](sol::state &lua) {
        sol::global_table &t = lua.globals();

        for (QByteArray key : expander->visibleVariables()) {
            if (key != "Lua:<value>") {
                if (key.endsWith(":<value>"))
                    key = key.chopped(7);

                QList<QByteArray> parts = key.split(':');
                parts.removeIf([](const QByteArray &part) { return part.isEmpty(); });
                setNext(expander, lua, t, key, parts.cbegin(), parts.cend());
            }
        }
    });
}

expected_str<QString> tryRun(const QString statement, MacroExpander *expander)
{
    sol::state lua;

    auto result = run(lua, statement, expander);
    if (!result.valid()) {
        sol::error err = result;
        return make_unexpected(QString::fromStdString(err.what()));
    }

    QStringList results;

    for (int i = 1; i <= result.return_count(); i++) {
        size_t l;
        const char *s = luaL_tolstring(result.lua_state(), int(i), &l);
        results.append(QString::fromUtf8(s, int(l)));
    }
    return results.join(QLatin1Char(' '));
}

void setupLuaExpander(MacroExpander *expander)
{
    expander->registerPrefix(
        "Lua",
        Tr::tr("Evaluate simple Lua statements.<br>"
               "Literal '}' characters must be escaped as \"\\}\", "
               "'\\' characters must be escaped as \"\\\\\", "
               "'#' characters must be escaped as \"\\#\", "
               "and \"%{\" must be escaped as \"%\\{\"."),
        [expander](const QString &statement) -> QString {
            if (statement.isEmpty())
                return Tr::tr("No Lua statement to evaluate.");

            expected_str<QString> result = tryRun("return " + statement, expander);
            if (result)
                return *result;

            result = tryRun(statement, expander);
            if (!result)
                return result.error();

            return *result;
        });
}

} // namespace Lua::Internal
