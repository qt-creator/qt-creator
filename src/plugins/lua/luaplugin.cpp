// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "luaengine.h"
#include "luapluginspec.h"

#include <coreplugin/icore.h>
#include <coreplugin/jsexpander.h>
#include <coreplugin/messagemanager.h>

#include <extensionsystem/iplugin.h>
#include <extensionsystem/pluginmanager.h>

#include <utils/algorithm.h>

#include <QDebug>

using namespace Core;
using namespace Utils;
using namespace ExtensionSystem;

namespace Lua::Internal {

void addAsyncModule();
void addFetchModule();
void addActionModule();
void addUtilsModule();
void addMessageManagerModule();
void addProcessModule();
void addSettingsModule();
void addGuiModule();
void addQtModule();
void addCoreModule();
void addHookModule();
void addInstallModule();

class LuaJsExtension : public QObject
{
    Q_OBJECT

public:
    explicit LuaJsExtension(QObject *parent = nullptr)
        : QObject(parent)
    {}

    Q_INVOKABLE QString metaFolder() const
    {
        return Core::ICore::resourcePath("lua/meta").toFSPathString();
    }
};

class LuaPlugin : public IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Lua.json")

private:
    std::unique_ptr<LuaEngine> m_luaEngine;

public:
    LuaPlugin() {}
    ~LuaPlugin() override = default;

    void initialize() final
    {
        m_luaEngine.reset(new LuaEngine());

        addAsyncModule();
        addFetchModule();
        addActionModule();
        addUtilsModule();
        addMessageManagerModule();
        addProcessModule();
        addSettingsModule();
        addGuiModule();
        addQtModule();
        addCoreModule();
        addHookModule();
        addInstallModule();

        Core::JsExpander::registerGlobalObject("Lua", [] { return new LuaJsExtension(); });
    }

    bool delayedInitialize() final
    {
        scanForPlugins(PluginManager::pluginPaths());
        return true;
    }

    void scanForPlugins(const FilePaths &pluginPaths)
    {
        QSet<PluginSpec *> plugins;
        for (const FilePath &path : pluginPaths) {
            FilePaths folders = path.dirEntries(FileFilter({}, QDir::Dirs | QDir::NoDotAndDotDot));

            for (const FilePath &folder : folders) {
                const FilePath script = folder / (folder.baseName() + ".lua");
                if (!script.exists())
                    continue;

                const expected_str<LuaPluginSpec *> result = m_luaEngine->loadPlugin(script);

                if (!result) {
                    qWarning() << "Failed to load plugin" << script << ":" << result.error();
                    MessageManager::writeFlashing(tr("Failed to load plugin %1: %2")
                                                      .arg(script.toUserOutput())
                                                      .arg(result.error()));
                    continue;
                }

                plugins.insert(*result);
            }
        }

        PluginManager::addPlugins({plugins.begin(), plugins.end()});
        PluginManager::loadPluginsAtRuntime(plugins);
    }
};

} // namespace Lua::Internal

#include "luaplugin.moc"
