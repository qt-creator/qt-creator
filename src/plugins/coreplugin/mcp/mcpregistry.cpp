// This file is auto-generated. Do not edit manually.
#include "mcpregistry.h"

namespace Core::McpRegistry {

QString toString(const Icon::Theme &v)
{
    switch(v) {
        case Icon::Theme::light: return "light";
        case Icon::Theme::dark: return "dark";
    }
    return {};
}

template<>
Utils::Result<Icon::Theme> fromJson<Icon::Theme>(const QJsonValue &val)
{
    const QString str = val.toString();
    if (str == "light") return Icon::Theme::light;
    if (str == "dark") return Icon::Theme::dark;
    return Utils::ResultError("Invalid Icon::Theme value: " + str);
}

template<>
Utils::Result<Icon> fromJson<Icon>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Icon");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("url"))
        return Utils::ResultError("Missing required field: url");
    Icon result;
    result._url = obj.value("url").toString();
    if (obj.contains("data"))
        result._data = obj.value("data").toString();
    if (obj.contains("mime_type"))
        result._mime_type = obj.value("mime_type").toString();
    if (obj.contains("sizes") && obj["sizes"].isArray()) {
        const QJsonArray arr = obj["sizes"].toArray();
        QStringList list_sizes;
        for (const QJsonValue &v : arr) {
            list_sizes.append(v.toString());
        }
        result._sizes = list_sizes;
    }
    if (obj.contains("theme") && obj["theme"].isString()) {
        const auto res0 = fromJson<Icon::Theme>(obj["theme"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._theme = *res0;
    }
    return result;
}

QString toString(const Argument::Type &v)
{
    switch(v) {
        case Argument::Type::positional: return "positional";
        case Argument::Type::named: return "named";
    }
    return {};
}

template<>
Utils::Result<Argument::Type> fromJson<Argument::Type>(const QJsonValue &val)
{
    const QString str = val.toString();
    if (str == "positional") return Argument::Type::positional;
    if (str == "named") return Argument::Type::named;
    return Utils::ResultError("Invalid Argument::Type value: " + str);
}

template<>
Utils::Result<Argument> fromJson<Argument>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Argument");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("type"))
        return Utils::ResultError("Missing required field: type");
    if (!obj.contains("required"))
        return Utils::ResultError("Missing required field: required");
    if (!obj.contains("repeated"))
        return Utils::ResultError("Missing required field: repeated");
    Argument result;
    if (obj.contains("type") && obj["type"].isString()) {
        const auto res0 = fromJson<Argument::Type>(obj["type"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._type = *res0;
    }
    if (obj.contains("name"))
        result._name = obj.value("name").toString();
    if (obj.contains("value_hint"))
        result._value_hint = obj.value("value_hint").toString();
    if (obj.contains("description"))
        result._description = obj.value("description").toString();
    result._required = obj.value("required").toBool();
    result._repeated = obj.value("repeated").toBool();
    if (obj.contains("default"))
        result._default_ = obj.value("default").toString();
    if (obj.contains("value"))
        result._value = obj.value("value").toString();
    return result;
}

template<>
Utils::Result<KeyValueInput> fromJson<KeyValueInput>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for KeyValueInput");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    if (!obj.contains("required"))
        return Utils::ResultError("Missing required field: required");
    if (!obj.contains("secret"))
        return Utils::ResultError("Missing required field: secret");
    KeyValueInput result;
    result._name = obj.value("name").toString();
    if (obj.contains("description"))
        result._description = obj.value("description").toString();
    result._required = obj.value("required").toBool();
    result._secret = obj.value("secret").toBool();
    if (obj.contains("default"))
        result._default_ = obj.value("default").toString();
    return result;
}

QString toString(const Package::Registry_type &v)
{
    switch(v) {
        case Package::Registry_type::npm: return "npm";
        case Package::Registry_type::pypi: return "pypi";
        case Package::Registry_type::oci: return "oci";
        case Package::Registry_type::nuget: return "nuget";
        case Package::Registry_type::mcpb: return "mcpb";
    }
    return {};
}

template<>
Utils::Result<Package::Registry_type> fromJson<Package::Registry_type>(const QJsonValue &val)
{
    const QString str = val.toString();
    if (str == "npm") return Package::Registry_type::npm;
    if (str == "pypi") return Package::Registry_type::pypi;
    if (str == "oci") return Package::Registry_type::oci;
    if (str == "nuget") return Package::Registry_type::nuget;
    if (str == "mcpb") return Package::Registry_type::mcpb;
    return Utils::ResultError("Invalid Package::Registry_type value: " + str);
}

QString toString(const Package::Transport_type &v)
{
    switch(v) {
        case Package::Transport_type::stdio: return "stdio";
        case Package::Transport_type::streamableminushttp: return "streamable-http";
        case Package::Transport_type::sse: return "sse";
    }
    return {};
}

template<>
Utils::Result<Package::Transport_type> fromJson<Package::Transport_type>(const QJsonValue &val)
{
    const QString str = val.toString();
    if (str == "stdio") return Package::Transport_type::stdio;
    if (str == "streamable-http") return Package::Transport_type::streamableminushttp;
    if (str == "sse") return Package::Transport_type::sse;
    return Utils::ResultError("Invalid Package::Transport_type value: " + str);
}

template<>
Utils::Result<Package> fromJson<Package>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Package");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("registry_type"))
        return Utils::ResultError("Missing required field: registry_type");
    if (!obj.contains("identifier"))
        return Utils::ResultError("Missing required field: identifier");
    if (!obj.contains("transport_type"))
        return Utils::ResultError("Missing required field: transport_type");
    Package result;
    if (obj.contains("registry_type") && obj["registry_type"].isString()) {
        const auto res0 = fromJson<Package::Registry_type>(obj["registry_type"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._registry_type = *res0;
    }
    result._identifier = obj.value("identifier").toString();
    if (obj.contains("version"))
        result._version = obj.value("version").toString();
    if (obj.contains("transport_type") && obj["transport_type"].isString()) {
        const auto res1 = fromJson<Package::Transport_type>(obj["transport_type"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._transport_type = *res1;
    }
    if (obj.contains("runtime_hint"))
        result._runtime_hint = obj.value("runtime_hint").toString();
    if (obj.contains("runtime_arguments") && obj["runtime_arguments"].isArray()) {
        const QJsonArray arr = obj["runtime_arguments"].toArray();
        QList<Argument> list_runtime_arguments;
        for (const QJsonValue &v : arr) {
            const auto res2 = fromJson<Argument>(v);
            if (!res2)
                return Utils::ResultError(res2.error());
            list_runtime_arguments.append(*res2);
        }
        result._runtime_arguments = list_runtime_arguments;
    }
    if (obj.contains("package_arguments") && obj["package_arguments"].isArray()) {
        const QJsonArray arr = obj["package_arguments"].toArray();
        QList<Argument> list_package_arguments;
        for (const QJsonValue &v : arr) {
            const auto res3 = fromJson<Argument>(v);
            if (!res3)
                return Utils::ResultError(res3.error());
            list_package_arguments.append(*res3);
        }
        result._package_arguments = list_package_arguments;
    }
    if (obj.contains("headers") && obj["headers"].isArray()) {
        const QJsonArray arr = obj["headers"].toArray();
        QList<KeyValueInput> list_headers;
        for (const QJsonValue &v : arr) {
            const auto res4 = fromJson<KeyValueInput>(v);
            if (!res4)
                return Utils::ResultError(res4.error());
            list_headers.append(*res4);
        }
        result._headers = list_headers;
    }
    if (obj.contains("env_vars") && obj["env_vars"].isArray()) {
        const QJsonArray arr = obj["env_vars"].toArray();
        QList<KeyValueInput> list_env_vars;
        for (const QJsonValue &v : arr) {
            const auto res5 = fromJson<KeyValueInput>(v);
            if (!res5)
                return Utils::ResultError(res5.error());
            list_env_vars.append(*res5);
        }
        result._env_vars = list_env_vars;
    }
    return result;
}

QString toString(const Remote::Type &v)
{
    switch(v) {
        case Remote::Type::streamableminushttp: return "streamable-http";
        case Remote::Type::sse: return "sse";
    }
    return {};
}

template<>
Utils::Result<Remote::Type> fromJson<Remote::Type>(const QJsonValue &val)
{
    const QString str = val.toString();
    if (str == "streamable-http") return Remote::Type::streamableminushttp;
    if (str == "sse") return Remote::Type::sse;
    return Utils::ResultError("Invalid Remote::Type value: " + str);
}

template<>
Utils::Result<Remote> fromJson<Remote>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Remote");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("type"))
        return Utils::ResultError("Missing required field: type");
    if (!obj.contains("url"))
        return Utils::ResultError("Missing required field: url");
    Remote result;
    if (obj.contains("type") && obj["type"].isString()) {
        const auto res0 = fromJson<Remote::Type>(obj["type"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._type = *res0;
    }
    result._url = obj.value("url").toString();
    if (obj.contains("headers") && obj["headers"].isArray()) {
        const QJsonArray arr = obj["headers"].toArray();
        QList<KeyValueInput> list_headers;
        for (const QJsonValue &v : arr) {
            const auto res1 = fromJson<KeyValueInput>(v);
            if (!res1)
                return Utils::ResultError(res1.error());
            list_headers.append(*res1);
        }
        result._headers = list_headers;
    }
    return result;
}

QString toString(const Server::Status &v)
{
    switch(v) {
        case Server::Status::active: return "active";
        case Server::Status::deprecated: return "deprecated";
        case Server::Status::deleted: return "deleted";
    }
    return {};
}

template<>
Utils::Result<Server::Status> fromJson<Server::Status>(const QJsonValue &val)
{
    const QString str = val.toString();
    if (str == "active") return Server::Status::active;
    if (str == "deprecated") return Server::Status::deprecated;
    if (str == "deleted") return Server::Status::deleted;
    return Utils::ResultError("Invalid Server::Status value: " + str);
}

template<>
Utils::Result<Server> fromJson<Server>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Server");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    if (!obj.contains("description"))
        return Utils::ResultError("Missing required field: description");
    if (!obj.contains("version"))
        return Utils::ResultError("Missing required field: version");
    if (!obj.contains("status"))
        return Utils::ResultError("Missing required field: status");
    Server result;
    result._name = obj.value("name").toString();
    if (obj.contains("title"))
        result._title = obj.value("title").toString();
    result._description = obj.value("description").toString();
    result._version = obj.value("version").toString();
    if (obj.contains("status") && obj["status"].isString()) {
        const auto res0 = fromJson<Server::Status>(obj["status"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._status = *res0;
    }
    if (obj.contains("repository_url"))
        result._repository_url = obj.value("repository_url").toString();
    if (obj.contains("website_url"))
        result._website_url = obj.value("website_url").toString();
    if (obj.contains("icons") && obj["icons"].isArray()) {
        const QJsonArray arr = obj["icons"].toArray();
        QList<Icon> list_icons;
        for (const QJsonValue &v : arr) {
            const auto res1 = fromJson<Icon>(v);
            if (!res1)
                return Utils::ResultError(res1.error());
            list_icons.append(*res1);
        }
        result._icons = list_icons;
    }
    if (obj.contains("packages") && obj["packages"].isArray()) {
        const QJsonArray arr = obj["packages"].toArray();
        QList<Package> list_packages;
        for (const QJsonValue &v : arr) {
            const auto res2 = fromJson<Package>(v);
            if (!res2)
                return Utils::ResultError(res2.error());
            list_packages.append(*res2);
        }
        result._packages = list_packages;
    }
    if (obj.contains("remotes") && obj["remotes"].isArray()) {
        const QJsonArray arr = obj["remotes"].toArray();
        QList<Remote> list_remotes;
        for (const QJsonValue &v : arr) {
            const auto res3 = fromJson<Remote>(v);
            if (!res3)
                return Utils::ResultError(res3.error());
            list_remotes.append(*res3);
        }
        result._remotes = list_remotes;
    }
    return result;
}

template<>
Utils::Result<McpRegistry> fromJson<McpRegistry>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for McpRegistry");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("generated_at"))
        return Utils::ResultError("Missing required field: generated_at");
    if (!obj.contains("count"))
        return Utils::ResultError("Missing required field: count");
    if (!obj.contains("servers"))
        return Utils::ResultError("Missing required field: servers");
    McpRegistry result;
    result._generated_at = obj.value("generated_at").toString();
    result._count = obj.value("count").toInt();
    if (obj.contains("servers") && obj["servers"].isArray()) {
        const QJsonArray arr = obj["servers"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<Server>(v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._servers.append(*res0);
        }
    }
    return result;
}

} // namespace Core::McpRegistry
