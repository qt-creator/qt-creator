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
        json["encode"] = [](const sol::table &tbl) -> QString {
            QJsonValue value = LuaEngine::toJson(tbl);
            QJsonDocument doc;
            if (value.isObject())
                doc.setObject(value.toObject());
            else if (value.isArray())
                doc.setArray(value.toArray());
            else
                return QString();

            return QString::fromUtf8(doc.toJson());
        };

        json["decode"] = [](sol::this_state l, const QString &str) -> sol::table {
            QJsonParseError error;
            auto doc = QJsonDocument::fromJson(str.toUtf8(), &error);
            if (error.error != QJsonParseError::NoError)
                throw sol::error(error.errorString().toStdString());

            if (doc.isObject())
                return LuaEngine::toTable(l.lua_state(), doc.object());
            else if (doc.isArray())
                return LuaEngine::toTable(l.lua_state(), doc.array());

            return sol::table();
        };

        return json;
    });
}

} // namespace Lua::Internal
