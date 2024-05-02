// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "luaengine.h"
#include "luapluginloader.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>

#include <extensionsystem/iplugin.h>
#include <extensionsystem/pluginmanager.h>

#include <utils/algorithm.h>

#include <QAction>
#include <QDebug>
#include <QMenu>

namespace Lua::Internal {

void addAsyncModule();
void addFetchModule();
void addActionModule();
void addUtilsModule();
void addMessageManagerModule();
void addProcessModule();
void addSettingsModule();
void addLayoutModule();
void addQtModule();
void addCoreModule();
void addHookModule();

class LuaPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Lua.json")

private:
    std::unique_ptr<LuaEngine> m_luaEngine;

public:
    LuaPlugin()
        : m_luaEngine(new LuaEngine())
    {}
    ~LuaPlugin() override = default;

    void initialize() final
    {
        addAsyncModule();
        addFetchModule();
        addActionModule();
        addUtilsModule();
        addMessageManagerModule();
        addProcessModule();
        addSettingsModule();
        addLayoutModule();
        addQtModule();
        addCoreModule();
        addHookModule();
    }

    bool delayedInitialize() final
    {
        LuaPluginLoader::instance().scan(
            Utils::transform(ExtensionSystem::PluginManager::pluginPaths(),
                             [](const QString &path) -> QString { return path + "/lua-plugins/"; }));

        return true;
    }
};

} // namespace Lua::Internal

#include "luaplugin.moc"
