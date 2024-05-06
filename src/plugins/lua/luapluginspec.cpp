// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "luapluginspec.h"

#include "luaengine.h"
#include "luatr.h"

#include <extensionsystem/extensionsystemtr.h>

#include <utils/algorithm.h>
#include <utils/expected.h>

#include <QJsonDocument>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(luaPluginSpecLog, "qtc.lua.pluginspec", QtWarningMsg)

using namespace ExtensionSystem;
using namespace Utils;

namespace Lua {

class LuaScriptPluginPrivate
{
public:
    QString name;
    QList<QString> cppDepends;
    sol::function setup;
    sol::environment pluginEnvironment;
};

class LuaPluginSpecPrivate
{
public:
    FilePath pluginScriptPath;

    sol::state lua;
    sol::table pluginTable;

    sol::function setupFunction;
};

LuaPluginSpec::LuaPluginSpec()
    : d(new LuaPluginSpecPrivate())
{}

expected_str<LuaPluginSpec *> LuaPluginSpec::create(const FilePath &filePath,
                                                    sol::state lua,
                                                    sol::table pluginTable)
{
    std::unique_ptr<LuaPluginSpec> pluginSpec(new LuaPluginSpec());

    pluginSpec->d->lua = std::move(lua);
    pluginSpec->d->pluginTable = pluginTable;

    pluginSpec->d->setupFunction = pluginTable.get_or<sol::function>("setup", {});
    if (!pluginSpec->d->setupFunction)
        return make_unexpected(QString("Plugin info table did not contain a setup function"));

    QJsonValue v = LuaEngine::toJson(pluginTable);
    if (luaPluginSpecLog().isDebugEnabled()) {
        qCDebug(luaPluginSpecLog).noquote()
            << "Plugin info table:" << QJsonDocument(v.toObject()).toJson(QJsonDocument::Indented);
    }

    QJsonObject obj = v.toObject();
    obj["SoftLoadable"] = true;

    auto r = pluginSpec->PluginSpec::readMetaData(obj);
    if (!r)
        return make_unexpected(r.error());

    pluginSpec->setFilePath(filePath);
    pluginSpec->setLocation(filePath.parentDir());

    pluginSpec->d->pluginScriptPath = filePath;

    return pluginSpec.release();
}

ExtensionSystem::IPlugin *LuaPluginSpec::plugin() const
{
    return nullptr;
}

// LuaPluginSpec::For internal use {}
bool LuaPluginSpec::loadLibrary()
{
    // We are actually already loaded, but we need to set the state to loaded as well.
    // We cannot set it earlier as it is used as a state machine that would break for earlier steps.
    setState(PluginSpec::State::Loaded);
    return true;
}
bool LuaPluginSpec::initializePlugin()
{
    expected_str<void> setupResult
        = LuaEngine::instance()
              .prepareSetup(d->lua, *this, d->pluginTable.get<sol::optional<sol::table>>("hooks"));

    if (!setupResult) {
        setError(Lua::Tr::tr("Failed to prepare plugin setup: %1").arg(setupResult.error()));
        return false;
    }

    auto result = d->setupFunction.call();

    if (result.get_type() == sol::type::boolean && result.get<bool>() == false) {
        setError(Lua::Tr::tr("Plugin setup function returned false"));
        return false;
    } else if (result.get_type() == sol::type::string) {
        std::string error = result.get<sol::error>().what();
        if (!error.empty()) {
            setError(Lua::Tr::tr("Plugin setup function returned error: %1")
                         .arg(QString::fromStdString(error)));
            return false;
        }
    }

    setState(PluginSpec::State::Initialized);
    return true;
}

bool LuaPluginSpec::initializeExtensions()
{
    setState(PluginSpec::State::Running);
    return true;
}

bool LuaPluginSpec::delayedInitialize()
{
    return true;
}
ExtensionSystem::IPlugin::ShutdownFlag LuaPluginSpec::stop()
{
    return ExtensionSystem::IPlugin::ShutdownFlag::SynchronousShutdown;
}

void LuaPluginSpec::kill() {}

bool LuaPluginSpec::printToOutputPane() const
{
    return d->pluginTable.get_or("printToOutputPane", false);
}

} // namespace Lua
