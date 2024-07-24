// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "../luaengine.h"

#include <QJsonArray>
#include <QJsonDocument>

namespace Lua::Internal {

void addJsonModule()
{
    LuaEngine::registerProvider("Json", [](sol::state_view lua) -> sol::object {
        sol::table json = lua.create_table();
        json["encode"] = &LuaEngine::toJsonString;

        json["decode"] = [](sol::this_state l, const QString &str) -> sol::table {
            QJsonParseError error;
            auto doc = QJsonDocument::fromJson(str.toUtf8(), &error);
            if (error.error != QJsonParseError::NoError)
                throw sol::error(error.errorString().toStdString());

            return LuaEngine::toTable(l.lua_state(), doc);
        };

        return json;
    });
}

} // namespace Lua::Internal
