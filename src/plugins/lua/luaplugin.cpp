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

struct Arguments
{
    std::optional<FilePath> loadPlugin;
};

Arguments parseArguments(const QStringList &arguments)
{
    Arguments args;
    for (int i = 0; i < arguments.size() - 1; ++i) {
        if (arguments.at(i) == QLatin1String("-loadluaplugin")) {
            const QString path(arguments.at(i + 1));
            args.loadPlugin = FilePath::fromUserInput(path);
            i++; // skip the argument
        }
    }
    return args;
}

class LuaPlugin : public IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Lua.json")

private:
    std::unique_ptr<LuaEngine> m_luaEngine;
    Arguments m_arguments;

public:
    LuaPlugin() {}
    ~LuaPlugin() override = default;

    bool initialize(const QStringList &arguments, QString *) final
    {
        m_luaEngine.reset(new LuaEngine());

        m_arguments = parseArguments(arguments);

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

        return true;
    }

    bool delayedInitialize() final
    {
        scanForPlugins(PluginManager::pluginPaths());
        return true;
    }

    void scanForPlugins(const FilePaths &paths)
    {
        QSet<PluginSpec *> plugins;
        for (const FilePath &path : paths) {
            FilePaths folders = path.dirEntries(FileFilter({}, QDir::Dirs | QDir::NoDotAndDotDot));

            for (const FilePath &folder : folders) {
                const FilePath script = folder / (folder.baseName() + ".lua");
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

        if (m_arguments.loadPlugin) {
            const FilePath folder = *m_arguments.loadPlugin;
            const FilePath script = folder / (folder.baseName() + ".lua");
            const expected_str<LuaPluginSpec *> result = m_luaEngine->loadPlugin(script);

            if (!result) {
                qWarning() << "Failed to load plugin" << script << ":" << result.error();
                MessageManager::writeFlashing(tr("Failed to load plugin %1: %2")
                                                  .arg(script.toUserOutput())
                                                  .arg(result.error()));

            } else {
                (*result)->setEnabledBySettings(true);
                plugins.insert(*result);
            }
        }

        PluginManager::addPlugins({plugins.begin(), plugins.end()});
        PluginManager::loadPluginsAtRuntime(plugins);
    }
};

} // namespace Lua::Internal

#include "luaplugin.moc"
