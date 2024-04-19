// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../luaengine.h"

#include <coreplugin/editormanager/editormanager.h>

namespace Lua {

class Hook : public QObject
{
    Q_OBJECT

public:
    Hook(QObject *source);

signals:
    void trigger(sol::table &args);
};

Hook::Hook(QObject *source)
    : QObject(source)
{}

namespace Internal {

void addHookModule()
{
    LuaEngine::autoRegister([](sol::state_view lua) {
        auto connection = lua.new_usertype<QMetaObject::Connection>("QMetaConnection",
                                                                    sol::no_constructor);

        auto hook = lua.new_usertype<Hook>(
            "Hook",
            sol::no_constructor,
            "connect",
            [](Hook *hook, const sol::function &func) -> QMetaObject::Connection {
                QMetaObject::Connection con
                    = QObject::connect(hook, &Hook::trigger, [func](sol::table args) {
                          auto res = LuaEngine::void_safe_call(func, args);
                          QTC_CHECK_EXPECTED(res);
                      });
                return con;
            },
            "disconnect",
            [](Hook *, QMetaObject::Connection con) { QObject::disconnect(con); });
    });

    LuaEngine::registerHook("editors.documentOpened", [](const sol::function &func) {
        QObject::connect(Core::EditorManager::instance(),
                         &Core::EditorManager::documentOpened,
                         [func](Core::IDocument *document) { func(document); });
    });
    LuaEngine::registerHook("editors.documentClosed", [](const sol::function &func) {
        QObject::connect(Core::EditorManager::instance(),
                         &Core::EditorManager::documentClosed,
                         [func](Core::IDocument *document) { func(document); });
    });
}

} // namespace Internal

} // namespace Lua

#include "hook.moc"
