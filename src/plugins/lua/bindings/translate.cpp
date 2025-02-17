// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../luaengine.h"

#include <QCoreApplication>

namespace Lua::Internal {

void setupTranslateModule()
{
    autoRegister([](sol::state_view lua) {
        const ScriptPluginSpec *pluginSpec = lua.get<ScriptPluginSpec *>("PluginSpec");
        static const QRegularExpression regexp("[^a-zA-Z]");
        const QString trContext = QString(pluginSpec->name).replace(regexp, "_");

        lua["tr"] = [trContext](const char *text) -> QString {
            return QCoreApplication::translate(trContext.toUtf8().constData(), text);
        };
    });
}

} // namespace Lua::Internal
