// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "luaengine.h"

#include "luapluginspec.h"
#include "luatr.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <utils/algorithm.h>
#include <utils/lua.h>
#include <utils/stringutils.h>
#include <utils/theme/theme.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>

using namespace Utils;

namespace Lua {

static Q_LOGGING_CATEGORY(logLuaEngine, "qtc.lua.engine", QtWarningMsg);

class LuaInterfaceImpl : public Utils::LuaInterface
{
    LuaEngine *m_engine;

public:
    LuaInterfaceImpl(LuaEngine *engine)
        : m_engine(engine)
    {
        Utils::setLuaInterface(this);
    }
    ~LuaInterfaceImpl() override { Utils::setLuaInterface(nullptr); }

    expected_str<std::unique_ptr<LuaState>> runScript(
        const QString &script, const QString &name) override
    {
        return m_engine->runScript(script, name);
    }
};

class LuaEnginePrivate
{
public:
    LuaEnginePrivate() {}

    QHash<QString, LuaEngine::PackageProvider> m_providers;
    QList<std::function<void(sol::state_view)>> m_autoProviders;

    QMap<QString, std::function<void(sol::function, QObject *)>> m_hooks;

    std::unique_ptr<LuaInterfaceImpl> m_luaInterface;
};

static LuaEngine *s_instance = nullptr;

LuaEngine &LuaEngine::instance()
{
    Q_ASSERT(s_instance);
    return *s_instance;
}

LuaEngine::LuaEngine()
    : d(new LuaEnginePrivate())
{
    s_instance = this;
    d->m_luaInterface.reset(new LuaInterfaceImpl(this));
}

LuaEngine::~LuaEngine()
{
    s_instance = nullptr;
}

class LuaStateImpl : public Utils::LuaState
{
public:
    sol::state lua;
};

// Runs the gives script in a new Lua state. The returned Object manages the lifetime of the state.
std::unique_ptr<Utils::LuaState> LuaEngine::runScript(
    const QString &script, const QString &name, std::function<void(sol::state &)> customizeState)
{
    std::unique_ptr<LuaStateImpl> opaque = std::make_unique<LuaStateImpl>();

    opaque->lua.open_libraries(
        sol::lib::base,
        sol::lib::bit32,
        sol::lib::coroutine,
        sol::lib::debug,
        sol::lib::io,
        sol::lib::math,
        sol::lib::os,
        sol::lib::package,
        sol::lib::string,
        sol::lib::table,
        sol::lib::utf8);

    opaque->lua["print"] = [prefix = name, printToOutputPane = true](sol::variadic_args va) {
        const QString msg = variadicToStringList(va).join("\t");

        qDebug().noquote() << "[" << prefix << "]" << msg;
        if (printToOutputPane) {
            static const QString p
                = ansiColoredText("[" + prefix + "]", creatorColor(Theme::Token_Text_Muted));
            Core::MessageManager::writeSilently(QString("%1 %2").arg(p, msg));
        }
    };

    opaque->lua.new_usertype<ScriptPluginSpec>(
        "PluginSpec", sol::no_constructor, "name", sol::property([](ScriptPluginSpec &self) {
            return self.name;
        }));

    opaque->lua["PluginSpec"] = ScriptPluginSpec{name, {}, std::make_unique<QObject>()};

    for (const auto &[name, func] : d->m_providers.asKeyValueRange()) {
        opaque->lua["package"]["preload"][name.toStdString()] =
            [func = func](const sol::this_state &s) { return func(s); };
    }

    for (const auto &func : d->m_autoProviders)
        func(opaque->lua);

    if (customizeState)
        customizeState(opaque->lua);

    auto result
        = opaque->lua
              .safe_script(script.toStdString(), sol::script_pass_on_error, name.toStdString());

    if (!result.valid()) {
        sol::error err = result;
        qWarning() << "Failed to run script" << name << ":" << QString::fromUtf8(err.what());
        Core::MessageManager::writeFlashing(
            Tr::tr("Failed to run script %1: %2").arg(name, QString::fromUtf8(err.what())));
    }

    return opaque;
}

void LuaEngine::registerProvider(const QString &packageName, const PackageProvider &provider)
{
    QTC_ASSERT(!instance().d->m_providers.contains(packageName), return);
    instance().d->m_providers[packageName] = provider;
}

void LuaEngine::autoRegister(const std::function<void(sol::state_view)> &registerFunction)
{
    instance().d->m_autoProviders.append(registerFunction);
}

void LuaEngine::registerHook(
    QString name, const std::function<void(sol::function, QObject *guard)> &hook)
{
    instance().d->m_hooks.insert("." + name, hook);
}

expected_str<void> LuaEngine::connectHooks(
    sol::state_view lua, const sol::table &table, const QString &path, QObject *guard)
{
    qCDebug(logLuaEngine) << "connectHooks called with path: " << path;

    for (const auto &[k, v] : table) {
        qCDebug(logLuaEngine) << "Processing key: " << k.as<QString>();
        if (v.get_type() == sol::type::table) {
            return connectHooks(
                lua, v.as<sol::table>(), QStringList{path, k.as<QString>()}.join("."), guard);
        } else if (v.get_type() == sol::type::function) {
            QString hookName = QStringList{path, k.as<QString>()}.join(".");
            qCDebug(logLuaEngine) << "Connecting function to hook: " << hookName;
            auto it = d->m_hooks.find(hookName);
            if (it == d->m_hooks.end())
                return make_unexpected(Tr::tr("No hook with the name \"%1\" found.").arg(hookName));
            else
                it.value()(v.as<sol::function>(), guard);
        }
    }

    return {};
}

expected_str<LuaPluginSpec *> LuaEngine::loadPlugin(const Utils::FilePath &path)
{
    auto contents = path.fileContents();
    if (!contents)
        return make_unexpected(contents.error());

    sol::state lua;
    lua["tr"] = [](const QString &str) { return str; };

    sol::protected_function_result result = lua.safe_script(
        std::string_view(contents->data(), contents->size()),
        sol::script_pass_on_error,
        path.fileName().toUtf8().constData());

    if (!result.valid()) {
        sol::error err = result;
        return make_unexpected(QString(QString::fromUtf8(err.what())));
    }

    if (result.get_type() != sol::type::table)
        return make_unexpected(QString("Script did not return a table"));

    sol::table pluginInfo = result.get<sol::table>();
    if (!pluginInfo.valid())
        return make_unexpected(QString("Script did not return a table with plugin info"));
    return LuaPluginSpec::create(path, pluginInfo);
}

expected_str<sol::protected_function> LuaEngine::prepareSetup(
    sol::state_view lua, const LuaPluginSpec &pluginSpec)
{
    auto contents = pluginSpec.filePath().fileContents();
    if (!contents)
        return make_unexpected(contents.error());

    // TODO: Only open libraries requested by the plugin
    lua.open_libraries(
        sol::lib::base,
        sol::lib::bit32,
        sol::lib::coroutine,
        sol::lib::debug,
        sol::lib::io,
        sol::lib::math,
        sol::lib::os,
        sol::lib::package,
        sol::lib::string,
        sol::lib::table,
        sol::lib::utf8);

    const bool printToOutputPane = pluginSpec.printToOutputPane();
    const QString prefix = pluginSpec.filePath().fileName();
    lua["print"] = [prefix, printToOutputPane](sol::variadic_args va) {
        const QString msg = variadicToStringList(va).join("\t");

        qDebug().noquote() << "[" << prefix << "]" << msg;
        if (printToOutputPane) {
            static const QString p
                = ansiColoredText("[" + prefix + "]", creatorColor(Theme::Token_Text_Muted));
            Core::MessageManager::writeSilently(QString("%1 %2").arg(p, msg));
        }
    };

    const QString searchPath = (pluginSpec.location() / "?.lua").toUserOutput();
    lua["package"]["path"] = searchPath.toStdString();

    const FilePath appDataPath = Core::ICore::userResourcePath() / "plugin-data" / "lua"
                                 / pluginSpec.location().fileName();

    lua.new_usertype<ScriptPluginSpec>(
        "PluginSpec",
        sol::no_constructor,
        "name",
        sol::property([](ScriptPluginSpec &self) { return self.name; }),
        "pluginDirectory",
        sol::property([p = pluginSpec.location()]() { return p; }));

    auto guardObject = std::make_unique<QObject>();
    auto guardObjectPtr = guardObject.get();

    lua["PluginSpec"] = ScriptPluginSpec{pluginSpec.name(), appDataPath, std::move(guardObject)};

    // TODO: only register what the plugin requested
    for (const auto &[name, func] : d->m_providers.asKeyValueRange()) {
        lua["package"]["preload"][name.toStdString()] = [func = func](const sol::this_state &s) {
            return func(s);
        };
    }

    for (const auto &func : d->m_autoProviders)
        func(lua);

    sol::protected_function_result result = lua.safe_script(
        std::string_view(contents->data(), contents->size()),
        sol::script_pass_on_error,
        pluginSpec.filePath().fileName().toUtf8().constData());

    auto pluginTable = result.get<sol::optional<sol::table>>();
    if (!pluginTable)
        return make_unexpected(Tr::tr("Script did not return a table."));

    if (logLuaEngine().isDebugEnabled()) {
        qCDebug(logLuaEngine) << "Script returned table with keys:";
        for (const auto &[key, value] : *pluginTable) {
            qCDebug(logLuaEngine) << "Key:" << key.as<QString>();
            qCDebug(logLuaEngine) << "Value:" << value.as<QString>();
        }
    }

    auto hookTable = pluginTable->get<sol::optional<sol::table>>("hooks");

    qCDebug(logLuaEngine) << "Hooks table found: " << hookTable.has_value();
    if (hookTable) {
        auto connectResult = connectHooks(lua, *hookTable, {}, guardObjectPtr);
        if (!connectResult)
            return make_unexpected(connectResult.error());
    }

    auto setupFunction = pluginTable->get_or<sol::function>("setup", {});

    if (!setupFunction)
        return make_unexpected(Tr::tr("Extension info table did not contain a setup function."));

    return setupFunction;
}

bool LuaEngine::isCoroutine(lua_State *state)
{
    bool ismain = lua_pushthread(state) == 1;
    return !ismain;
}

template<typename KeyType>
static void setFromJson(sol::table &t, KeyType k, const QJsonValue &v)
{
    if (v.isDouble())
        t[k] = v.toDouble();
    else if (v.isBool())
        t[k] = v.toBool();
    else if (v.isString())
        t[k] = v.toString();
    else if (v.isObject())
        t[k] = LuaEngine::toTable(t.lua_state(), v);
    else if (v.isArray())
        t[k] = LuaEngine::toTable(t.lua_state(), v);
}

sol::table LuaEngine::toTable(const sol::state_view &lua, const QJsonValue &v)
{
    sol::table table(lua, sol::create);

    if (v.isObject()) {
        QJsonObject o = v.toObject();
        for (auto it = o.constBegin(); it != o.constEnd(); ++it)
            setFromJson(table, it.key(), it.value());
    } else if (v.isArray()) {
        int i = 1;
        for (const auto &v : v.toArray())
            setFromJson(table, i++, v);
    }

    return table;
}

QJsonValue toJsonValue(const sol::object &object);

QJsonValue toJsonValue(const sol::table &table)
{
    if (table.get<std::optional<sol::object>>(1)) {
        // Is Array
        QJsonArray arr;

        for (size_t i = 0; i < table.size(); ++i) {
            std::optional<sol::object> v = table.get<std::optional<sol::object>>(i + 1);
            if (!v)
                continue;
            arr.append(toJsonValue(*v));
        }

        return arr;
    }

    // Is Object
    QJsonObject obj;
    for (const auto &[k, v] : table)
        obj[k.as<QString>()] = toJsonValue(v);

    return obj;
}

QJsonValue toJsonValue(const sol::object &object)
{
    switch (object.get_type()) {
    case sol::type::lua_nil:
        return {};
    case sol::type::boolean:
        return object.as<bool>();
    case sol::type::number:
        return object.as<double>();
    case sol::type::string:
        return object.as<QString>();
    case sol::type::table:
        return toJsonValue(object.as<sol::table>());
    default:
        return {};
    }
}

QJsonValue LuaEngine::toJson(const sol::table &table)
{
    return toJsonValue(table);
}

QStringList LuaEngine::variadicToStringList(const sol::variadic_args &vargs)
{
    QStringList strings;
    for (size_t i = 1, n = vargs.size(); i <= n; i++) {
        size_t l;
        const char *s = luaL_tolstring(vargs.lua_state(), int(i), &l);
        if (s != nullptr)
            strings.append(QString::fromUtf8(s, l).replace('\0', "\\0"));
    }

    return strings;
}

} // namespace Lua
