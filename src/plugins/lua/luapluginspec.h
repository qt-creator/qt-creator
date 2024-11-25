// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sol/forward.hpp"

#include <extensionsystem/iplugin.h>
#include <extensionsystem/pluginspec.h>

#include <utils/expected.h>
#include <utils/filepath.h>

#include <QString>

namespace Lua {
class LuaScriptPluginPrivate;
class LuaPluginSpecPrivate;

class LuaScriptPlugin : public ExtensionSystem::IPlugin
{
public:
    LuaScriptPlugin() = delete;
    LuaScriptPlugin(const LuaScriptPlugin &other);
    LuaScriptPlugin(const std::shared_ptr<LuaScriptPluginPrivate> &d);

    QString name() const;
    QStringList cppDependencies() const;
    void setup() const;

private:
    std::shared_ptr<LuaScriptPluginPrivate> d;
};

class LuaPluginSpec : public ExtensionSystem::PluginSpec
{
    std::unique_ptr<LuaPluginSpecPrivate> d;

    LuaPluginSpec();

public:
    static Utils::expected_str<LuaPluginSpec *> create(
        const Utils::FilePath &filePath, sol::table pluginTable);

    ExtensionSystem::IPlugin *plugin() const override;

    bool provides(
        PluginSpec *spec, const ExtensionSystem::PluginDependency &dependency) const override;

    // For internal use only
    bool loadLibrary() override;
    bool initializePlugin() override;
    bool initializeExtensions() override;
    bool delayedInitialize() override;
    ExtensionSystem::IPlugin::ShutdownFlag stop() override;
    void kill() override;

    Utils::FilePath installLocation(bool inUserFolder) const override;

public:
    bool printToOutputPane() const;
};

} // namespace Lua
