// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "luaengine.h"

#include "luapluginspec.h"

#include <utils/algorithm.h>

#include <QJsonArray>
#include <QJsonObject>

using namespace Utils;

namespace Lua {

class LuaEnginePrivate
{
public:
    LuaEnginePrivate() {}

    QHash<QString, LuaEngine::PackageProvider> m_providers;
    QList<std::function<void(sol::state_view)>> m_autoProviders;

    QMap<QString, std::function<void(sol::function)>> m_hooks;
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
}

LuaEngine::~LuaEngine()
{
    s_instance = nullptr;
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

void LuaEngine::registerHook(QString name, const std::function<void(sol::function)> &hook)
{
    instance().d->m_hooks.insert("." + name, hook);
}

expected_str<void> LuaEngine::connectHooks(
    sol::state_view lua, const sol::table &table, const QString &path)
{
    for (const auto &[k, v] : table) {
        if (v.get_type() == sol::type::table) {
            return connectHooks(lua, v.as<sol::table>(), QStringList{path, k.as<QString>()}.join("."));
        } else if (v.get_type() == sol::type::function) {
            QString hookName = QStringList{path, k.as<QString>()}.join(".");
            auto it = d->m_hooks.find(hookName);
            if (it == d->m_hooks.end())
                return make_unexpected(QString("No hook named '%1' found").arg(hookName));
            else
                it.value()(v.as<sol::function>());
        }
    }

    return {};
}

expected_str<void> LuaEngine::connectHooks(sol::state_view lua, const sol::table &hookTable)
{
    if (!hookTable)
        return {};

    return instance().connectHooks(lua, hookTable, "");
}

expected_str<LuaPluginSpec *> LuaEngine::loadPlugin(const Utils::FilePath &path)
{
    auto contents = path.fileContents();
    if (!contents)
        return make_unexpected(contents.error());

    sol::state lua;

    lua["print"] = [prefix = path.fileName()](sol::variadic_args va) {
        qDebug().noquote() << "[" << prefix << "]" << variadicToStringList(va).join("\t");
    };

    auto result = lua.safe_script(
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
    return LuaPluginSpec::create(path, std::move(lua), pluginInfo);
}

expected_str<void> LuaEngine::prepareSetup(
    sol::state_view &lua, const LuaPluginSpec &pluginSpec, sol::optional<sol::table> hookTable)
{
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

    const QString searchPath
        = (FilePath::fromUserInput(pluginSpec.filePath()).parentDir() / "?.lua").toUserOutput();
    lua["package"]["path"] = searchPath.toStdString();

    // TODO: only register what the plugin requested
    for (const auto &[name, func] : d->m_providers.asKeyValueRange()) {
        lua["package"]["preload"][name.toStdString()] = [func = func](const sol::this_state &s) {
            return func(s);
        };
    }

    for (const auto &func : d->m_autoProviders)
        func(lua);

    if (hookTable)
        return LuaEngine::connectHooks(lua, *hookTable);

    return {};
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
    int n = vargs.size();
    int i;
    for (i = 1; i <= n; i++) {
        size_t l;
        const char *s = luaL_tolstring(vargs.lua_state(), i, &l);
        if (s != nullptr)
            strings.append(QString::fromUtf8(s, l));
    }

    return strings;
}

} // namespace Lua
