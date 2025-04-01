// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "luapluginspec.h"

#include "luaengine.h"
#include "luatr.h"

#include <coreplugin/icore.h>

#include <extensionsystem/extensionsystemtr.h>

#include <utils/algorithm.h>
#include <utils/appinfo.h>
#include <utils/expected.h>

#include <QJsonDocument>
#include <QLoggingCategory>
#include <QTranslator>

Q_LOGGING_CATEGORY(luaPluginSpecLog, "qtc.lua.pluginspec", QtWarningMsg)

using namespace ExtensionSystem;
using namespace Utils;
using namespace std::string_view_literals;

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
    bool printToOutputPane = false;
    std::unique_ptr<sol::state> activeLuaState;
};

LuaPluginSpec::LuaPluginSpec()
    : d(new LuaPluginSpecPrivate())
{}

expected_str<LuaPluginSpec *> LuaPluginSpec::create(const FilePath &filePath, sol::table pluginTable)
{
    const FilePath directory = filePath.parentDir();
    std::unique_ptr<LuaPluginSpec> pluginSpec(new LuaPluginSpec());

    if (!pluginTable.get_or<sol::function>("setup"sv, {}))
        return make_unexpected(QString("Plugin info table did not contain a setup function"));

    if (pluginTable.get_or<QString>("Type"sv, {}).toLower() != "script") {
        qCWarning(luaPluginSpecLog) << "Plugin info table did not contain a Type=\"Script\" field";
        pluginTable.set("Type"sv, "script"sv);
    }

    QJsonValue v = toJson(pluginTable);
    if (luaPluginSpecLog().isDebugEnabled()) {
        qCDebug(luaPluginSpecLog).noquote()
            << "Plugin info table:" << QJsonDocument(v.toObject()).toJson(QJsonDocument::Indented);
    }

    QJsonObject obj = v.toObject();
    obj["SoftLoadable"] = true;

    auto r = pluginSpec->PluginSpec::readMetaData(obj);
    if (!r)
        return make_unexpected(r.error());

    const QString langId = Core::ICore::userInterfaceLanguage();
    FilePath path = directory / "ts" / QString("%1_%2.qm").arg(directory.fileName()).arg(langId);

    QTranslator *translator = new QTranslator(qApp);
    bool success = translator->load(path.toFSPathString(), directory.toFSPathString());
    if (success)
        qApp->installTranslator(translator);
    else {
        delete translator;
        qCInfo(luaPluginSpecLog) << "No translation found";
    }

    pluginSpec->setFilePath(filePath);
    pluginSpec->setLocation(directory);

    pluginSpec->d->pluginScriptPath = filePath;
    pluginSpec->d->printToOutputPane = pluginTable.get_or("printToOutputPane", false);

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
    QTC_ASSERT(!d->activeLuaState, return false);

    std::unique_ptr<sol::state> activeLuaState = std::make_unique<sol::state>();

    expected_str<sol::protected_function> setupResult = prepareSetup(*activeLuaState, *this);

    if (!setupResult) {
        setError(Lua::Tr::tr("Cannot prepare extension setup: %1").arg(setupResult.error()));
        return false;
    }

    auto result = setupResult->call();

    if (result.get_type() == sol::type::boolean && result.get<bool>() == false) {
        setError(Lua::Tr::tr("Extension setup function returned false."));
        return false;
    } else if (result.get_type() == sol::type::string) {
        std::string error = result.get<sol::error>().what();
        if (!error.empty()) {
            setError(Lua::Tr::tr("Extension setup function returned error: %1")
                         .arg(QString::fromStdString(error)));
            return false;
        }
    }

    d->activeLuaState = std::move(activeLuaState);

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
    return false;
}
ExtensionSystem::IPlugin::ShutdownFlag LuaPluginSpec::stop()
{
    setState(PluginSpec::State::Stopped);
    return ExtensionSystem::IPlugin::ShutdownFlag::SynchronousShutdown;
}

void LuaPluginSpec::kill()
{
    if (!d->activeLuaState)
        return;

    d->activeLuaState.reset();
    setState(PluginSpec::State::Deleted);
}

bool LuaPluginSpec::printToOutputPane() const
{
    return d->printToOutputPane;
}

Utils::FilePath LuaPluginSpec::installLocation(bool inUserFolder) const
{
    if (inUserFolder)
        return appInfo().userLuaPlugins;

    return appInfo().luaPlugins;
}

} // namespace Lua
