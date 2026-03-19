// This file is auto-generated. Do not edit manually.
#include "acpregistry.h"

namespace Acp::Registry {

template<>
Utils::Result<binaryTarget> fromJson<binaryTarget>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for binaryTarget");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("archive"))
        return Utils::ResultError("Missing required field: archive");
    if (!obj.contains("cmd"))
        return Utils::ResultError("Missing required field: cmd");
    binaryTarget result;
    result._archive = obj.value("archive").toString();
    result._cmd = obj.value("cmd").toString();
    if (obj.contains("args") && obj["args"].isArray()) {
        const QJsonArray arr = obj["args"].toArray();
        QStringList list_args;
        for (const QJsonValue &v : arr) {
            list_args.append(v.toString());
        }
        result._args = list_args;
    }
    if (obj.contains("env") && obj["env"].isObject()) {
        const QJsonObject mapObj_env = obj["env"].toObject();
        QMap<QString, QString> map_env;
        for (auto it = mapObj_env.constBegin(); it != mapObj_env.constEnd(); ++it)
            map_env.insert(it.key(), it.value().toString());
        result._env = map_env;
    }
    return result;
}

QJsonObject toJson(const binaryTarget &data) {
    QJsonObject obj{
        {"archive", data._archive},
        {"cmd", data._cmd}
    };
    if (data._args.has_value()) {
        QJsonArray arr_args;
        for (const auto &v : *data._args) arr_args.append(v);
        obj.insert("args", arr_args);
    }
    if (data._env.has_value()) {
        QJsonObject map_env;
        for (auto it = data._env->constBegin(); it != data._env->constEnd(); ++it)
            map_env.insert(it.key(), QJsonValue(it.value()));
        obj.insert("env", map_env);
    }
    return obj;
}

template<>
Utils::Result<binaryDistribution> fromJson<binaryDistribution>(const QJsonValue &val) {
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for binaryDistribution");
    const QJsonObject obj = val.toObject();
    binaryDistribution result;
    if (obj.contains("darwin-aarch64"))
        result._darwinminusaarch64 = co_await fromJson<binaryTarget>(obj.value("darwin-aarch64"));
    if (obj.contains("darwin-x86_64"))
        result._darwinminusx86_64 = co_await fromJson<binaryTarget>(obj.value("darwin-x86_64"));
    if (obj.contains("linux-aarch64"))
        result._linuxminusaarch64 = co_await fromJson<binaryTarget>(obj.value("linux-aarch64"));
    if (obj.contains("linux-x86_64"))
        result._linuxminusx86_64 = co_await fromJson<binaryTarget>(obj.value("linux-x86_64"));
    if (obj.contains("windows-aarch64"))
        result._windowsminusaarch64 = co_await fromJson<binaryTarget>(obj.value("windows-aarch64"));
    if (obj.contains("windows-x86_64"))
        result._windowsminusx86_64 = co_await fromJson<binaryTarget>(obj.value("windows-x86_64"));
    co_return result;
}

QJsonObject toJson(const binaryDistribution &data) {
    QJsonObject obj;
    if (data._darwinminusaarch64.has_value())
        obj.insert("darwin-aarch64", toJson(*data._darwinminusaarch64));
    if (data._darwinminusx86_64.has_value())
        obj.insert("darwin-x86_64", toJson(*data._darwinminusx86_64));
    if (data._linuxminusaarch64.has_value())
        obj.insert("linux-aarch64", toJson(*data._linuxminusaarch64));
    if (data._linuxminusx86_64.has_value())
        obj.insert("linux-x86_64", toJson(*data._linuxminusx86_64));
    if (data._windowsminusaarch64.has_value())
        obj.insert("windows-aarch64", toJson(*data._windowsminusaarch64));
    if (data._windowsminusx86_64.has_value())
        obj.insert("windows-x86_64", toJson(*data._windowsminusx86_64));
    return obj;
}

template<>
Utils::Result<packageDistribution> fromJson<packageDistribution>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for packageDistribution");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("package"))
        return Utils::ResultError("Missing required field: package");
    packageDistribution result;
    result._package = obj.value("package").toString();
    if (obj.contains("args") && obj["args"].isArray()) {
        const QJsonArray arr = obj["args"].toArray();
        QStringList list_args;
        for (const QJsonValue &v : arr) {
            list_args.append(v.toString());
        }
        result._args = list_args;
    }
    if (obj.contains("env") && obj["env"].isObject()) {
        const QJsonObject mapObj_env = obj["env"].toObject();
        QMap<QString, QString> map_env;
        for (auto it = mapObj_env.constBegin(); it != mapObj_env.constEnd(); ++it)
            map_env.insert(it.key(), it.value().toString());
        result._env = map_env;
    }
    return result;
}

QJsonObject toJson(const packageDistribution &data) {
    QJsonObject obj{{"package", data._package}};
    if (data._args.has_value()) {
        QJsonArray arr_args;
        for (const auto &v : *data._args) arr_args.append(v);
        obj.insert("args", arr_args);
    }
    if (data._env.has_value()) {
        QJsonObject map_env;
        for (auto it = data._env->constBegin(); it != data._env->constEnd(); ++it)
            map_env.insert(it.key(), QJsonValue(it.value()));
        obj.insert("env", map_env);
    }
    return obj;
}

template<>
Utils::Result<ACPAgent::Distribution> fromJson<ACPAgent::Distribution>(const QJsonValue &val) {
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for Distribution");
    const QJsonObject obj = val.toObject();
    ACPAgent::Distribution result;
    if (obj.contains("binary") && obj["binary"].isObject())
        result._binary = co_await fromJson<binaryDistribution>(obj["binary"]);
    if (obj.contains("npx") && obj["npx"].isObject())
        result._npx = co_await fromJson<packageDistribution>(obj["npx"]);
    if (obj.contains("uvx") && obj["uvx"].isObject())
        result._uvx = co_await fromJson<packageDistribution>(obj["uvx"]);
    co_return result;
}

QJsonObject toJson(const ACPAgent::Distribution &data) {
    QJsonObject obj;
    if (data._binary.has_value())
        obj.insert("binary", toJson(*data._binary));
    if (data._npx.has_value())
        obj.insert("npx", toJson(*data._npx));
    if (data._uvx.has_value())
        obj.insert("uvx", toJson(*data._uvx));
    return obj;
}

template<>
Utils::Result<ACPAgent> fromJson<ACPAgent>(const QJsonValue &val) {
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for ACPAgent");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        co_return Utils::ResultError("Missing required field: id");
    if (!obj.contains("name"))
        co_return Utils::ResultError("Missing required field: name");
    if (!obj.contains("version"))
        co_return Utils::ResultError("Missing required field: version");
    if (!obj.contains("description"))
        co_return Utils::ResultError("Missing required field: description");
    if (!obj.contains("distribution"))
        co_return Utils::ResultError("Missing required field: distribution");
    ACPAgent result;
    result._id = obj.value("id").toString();
    result._name = obj.value("name").toString();
    result._version = obj.value("version").toString();
    result._description = obj.value("description").toString();
    if (obj.contains("repository"))
        result._repository = obj.value("repository").toString();
    if (obj.contains("authors") && obj["authors"].isArray()) {
        const QJsonArray arr = obj["authors"].toArray();
        QStringList list_authors;
        for (const QJsonValue &v : arr) {
            list_authors.append(v.toString());
        }
        result._authors = list_authors;
    }
    if (obj.contains("license"))
        result._license = obj.value("license").toString();
    if (obj.contains("icon"))
        result._icon = obj.value("icon").toString();
    if (obj.contains("distribution") && obj["distribution"].isObject())
        result._distribution = co_await fromJson<ACPAgent::Distribution>(obj["distribution"]);
    co_return result;
}

QJsonObject toJson(const ACPAgent &data) {
    QJsonObject obj{
        {"id", data._id},
        {"name", data._name},
        {"version", data._version},
        {"description", data._description},
        {"distribution", toJson(data._distribution)}
    };
    if (data._repository.has_value())
        obj.insert("repository", *data._repository);
    if (data._authors.has_value()) {
        QJsonArray arr_authors;
        for (const auto &v : *data._authors) arr_authors.append(v);
        obj.insert("authors", arr_authors);
    }
    if (data._license.has_value())
        obj.insert("license", *data._license);
    if (data._icon.has_value())
        obj.insert("icon", *data._icon);
    return obj;
}

template<>
Utils::Result<ACPAgentRegistry> fromJson<ACPAgentRegistry>(const QJsonValue &val) {
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for ACPAgentRegistry");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("version"))
        co_return Utils::ResultError("Missing required field: version");
    if (!obj.contains("agents"))
        co_return Utils::ResultError("Missing required field: agents");
    ACPAgentRegistry result;
    result._version = obj.value("version").toString();
    if (obj.contains("agents") && obj["agents"].isArray()) {
        const QJsonArray arr = obj["agents"].toArray();
        for (const QJsonValue &v : arr) {
            result._agents.append(co_await fromJson<ACPAgent>(v));
        }
    }
    co_return result;
}

QJsonObject toJson(const ACPAgentRegistry &data) {
    QJsonObject obj{{"version", data._version}};
    QJsonArray arr_agents;
    for (const auto &v : data._agents) arr_agents.append(toJson(v));
    obj.insert("agents", arr_agents);
    return obj;
}

} // namespace Acp::Registry
