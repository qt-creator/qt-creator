// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "luapluginloader.h"

#include "luaengine.h"
#include "luapluginspec.h"

#include <coreplugin/messagemanager.h>

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <utils/algorithm.h>
#include <utils/filepath.h>

namespace Lua {

class LuaPluginLoaderPrivate
{
public:
};

LuaPluginLoader::LuaPluginLoader()
    : d(std::make_unique<LuaPluginLoaderPrivate>())
{}
LuaPluginLoader::~LuaPluginLoader() = default;

LuaPluginLoader &LuaPluginLoader::instance()
{
    static LuaPluginLoader luaPluginLoader;
    return luaPluginLoader;
}

void LuaPluginLoader::scan(const QStringList &paths)
{
    QVector<ExtensionSystem::PluginSpec *> plugins;
    for (const auto &path : paths) {
        const auto folders = Utils::FilePath::fromUserInput(path).dirEntries(
            Utils::FileFilter({}, QDir::Dirs | QDir::NoDotAndDotDot));

        for (const auto &folder : folders) {
            const auto script = folder / (folder.baseName() + ".lua");
            const Utils::expected_str<LuaPluginSpec *> result = LuaEngine::instance().loadPlugin(
                script);

            if (!result) {
                qWarning() << "Failed to load plugin" << script << ":" << result.error();
                Core::MessageManager::writeFlashing(tr("Failed to load plugin %1: %2")
                                                        .arg(script.toUserOutput())
                                                        .arg(result.error()));
                continue;
            }

            plugins.push_back(*result);
        }
    }

    ExtensionSystem::PluginManager::addPlugins(plugins);
    ExtensionSystem::PluginManager::loadPluginsAtRuntime(
        QSet<ExtensionSystem::PluginSpec *>(plugins.begin(), plugins.end()));
}

} // namespace Lua
