// This file is auto-generated. Do not edit manually.
#include "acp.h"

namespace Acp {

template<>
Utils::Result<EnvVariable> fromJson<EnvVariable>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for EnvVariable");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    if (!obj.contains("value"))
        return Utils::ResultError("Missing required field: value");
    EnvVariable result;
    result._name = obj.value("name").toString();
    result._value = obj.value("value").toString();
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const EnvVariable &data)
{
    QJsonObject obj{
        {"name", data._name},
        {"value", data._value}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<> Utils::Result<SessionId> fromJson<SessionId>(const QJsonValue &val)
{
    if (!val.isString()) return Utils::ResultError("Expected string");
    return val.toString();
}

template<>
Utils::Result<CreateTerminalRequest> fromJson<CreateTerminalRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for CreateTerminalRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        co_return Utils::ResultError("Missing required field: sessionId");
    if (!obj.contains("command"))
        co_return Utils::ResultError("Missing required field: command");
    CreateTerminalRequest result;
    if (obj.contains("sessionId") && obj["sessionId"].isString())
        result._sessionId = co_await fromJson<SessionId>(obj["sessionId"]);
    result._command = obj.value("command").toString();
    if (obj.contains("args") && obj["args"].isArray()) {
        const QJsonArray arr = obj["args"].toArray();
        QStringList list_args;
        for (const QJsonValue &v : arr) {
            list_args.append(v.toString());
        }
        result._args = list_args;
    }
    if (obj.contains("env") && obj["env"].isArray()) {
        const QJsonArray arr = obj["env"].toArray();
        QList<EnvVariable> list_env;
        for (const QJsonValue &v : arr) {
            list_env.append(co_await fromJson<EnvVariable>(v));
        }
        result._env = list_env;
    }
    if (obj.contains("cwd"))
        if (!obj["cwd"].isNull()) {
            result._cwd = obj.value("cwd").toString();
        }
    if (obj.contains("outputByteLimit"))
        if (!obj["outputByteLimit"].isNull()) {
            result._outputByteLimit = obj.value("outputByteLimit").toInt();
        }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const CreateTerminalRequest &data)
{
    QJsonObject obj{
        {"sessionId", data._sessionId},
        {"command", data._command}
    };
    if (data._args.has_value()) {
        QJsonArray arr_args;
        for (const auto &v : *data._args) arr_args.append(v);
        obj.insert("args", arr_args);
    }
    if (data._env.has_value()) {
        QJsonArray arr_env;
        for (const auto &v : *data._env) arr_env.append(toJson(v));
        obj.insert("env", arr_env);
    }
    if (data._cwd.has_value())
        obj.insert("cwd", *data._cwd);
    if (data._outputByteLimit.has_value())
        obj.insert("outputByteLimit", *data._outputByteLimit);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<KillTerminalRequest> fromJson<KillTerminalRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for KillTerminalRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        co_return Utils::ResultError("Missing required field: sessionId");
    if (!obj.contains("terminalId"))
        co_return Utils::ResultError("Missing required field: terminalId");
    KillTerminalRequest result;
    if (obj.contains("sessionId") && obj["sessionId"].isString())
        result._sessionId = co_await fromJson<SessionId>(obj["sessionId"]);
    if (obj.contains("terminalId") && obj["terminalId"].isString())
        result._terminalId = co_await fromJson<TerminalId>(obj["terminalId"]);
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const KillTerminalRequest &data)
{
    QJsonObject obj{
        {"sessionId", data._sessionId},
        {"terminalId", data._terminalId}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<ReadTextFileRequest> fromJson<ReadTextFileRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for ReadTextFileRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        co_return Utils::ResultError("Missing required field: sessionId");
    if (!obj.contains("path"))
        co_return Utils::ResultError("Missing required field: path");
    ReadTextFileRequest result;
    if (obj.contains("sessionId") && obj["sessionId"].isString())
        result._sessionId = co_await fromJson<SessionId>(obj["sessionId"]);
    result._path = obj.value("path").toString();
    if (obj.contains("line"))
        if (!obj["line"].isNull()) {
            result._line = obj.value("line").toInt();
        }
    if (obj.contains("limit"))
        if (!obj["limit"].isNull()) {
            result._limit = obj.value("limit").toInt();
        }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const ReadTextFileRequest &data)
{
    QJsonObject obj{
        {"sessionId", data._sessionId},
        {"path", data._path}
    };
    if (data._line.has_value())
        obj.insert("line", *data._line);
    if (data._limit.has_value())
        obj.insert("limit", *data._limit);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<ReleaseTerminalRequest> fromJson<ReleaseTerminalRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for ReleaseTerminalRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        co_return Utils::ResultError("Missing required field: sessionId");
    if (!obj.contains("terminalId"))
        co_return Utils::ResultError("Missing required field: terminalId");
    ReleaseTerminalRequest result;
    if (obj.contains("sessionId") && obj["sessionId"].isString())
        result._sessionId = co_await fromJson<SessionId>(obj["sessionId"]);
    if (obj.contains("terminalId") && obj["terminalId"].isString())
        result._terminalId = co_await fromJson<TerminalId>(obj["terminalId"]);
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const ReleaseTerminalRequest &data)
{
    QJsonObject obj{
        {"sessionId", data._sessionId},
        {"terminalId", data._terminalId}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<RequestId> fromJson<RequestId>(const QJsonValue &val)
{
    if (val.isNull())
        return RequestId(std::monostate{});
    if (val.isDouble())
        return RequestId(static_cast<int>(val.toDouble()));
    if (val.isString())
        return RequestId(val.toString());
    return Utils::ResultError("Invalid RequestId");
}

QJsonValue toJsonValue(const RequestId &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            return QJsonValue(QJsonValue::Null);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

QString toString(PermissionOptionKind v)
{
    switch(v) {
        case PermissionOptionKind::allow_once: return "allow_once";
        case PermissionOptionKind::allow_always: return "allow_always";
        case PermissionOptionKind::reject_once: return "reject_once";
        case PermissionOptionKind::reject_always: return "reject_always";
    }
    return {};
}

template<>
Utils::Result<PermissionOptionKind> fromJson<PermissionOptionKind>(const QJsonValue &val)
{
    const QString str = val.toString();
    if (str == "allow_once") return PermissionOptionKind::allow_once;
    if (str == "allow_always") return PermissionOptionKind::allow_always;
    if (str == "reject_once") return PermissionOptionKind::reject_once;
    if (str == "reject_always") return PermissionOptionKind::reject_always;
    return Utils::ResultError("Invalid PermissionOptionKind value: " + str);
}

QJsonValue toJsonValue(const PermissionOptionKind &v)
{
    return toString(v);
}

template<>
Utils::Result<PermissionOption> fromJson<PermissionOption>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for PermissionOption");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("optionId"))
        co_return Utils::ResultError("Missing required field: optionId");
    if (!obj.contains("name"))
        co_return Utils::ResultError("Missing required field: name");
    if (!obj.contains("kind"))
        co_return Utils::ResultError("Missing required field: kind");
    PermissionOption result;
    if (obj.contains("optionId") && obj["optionId"].isString())
        result._optionId = co_await fromJson<PermissionOptionId>(obj["optionId"]);
    result._name = obj.value("name").toString();
    if (obj.contains("kind") && obj["kind"].isString())
        result._kind = co_await fromJson<PermissionOptionKind>(obj["kind"]);
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const PermissionOption &data)
{
    QJsonObject obj{
        {"optionId", data._optionId},
        {"name", data._name},
        {"kind", toJsonValue(data._kind)}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

QString toString(Role v)
{
    switch(v) {
        case Role::assistant: return "assistant";
        case Role::user: return "user";
    }
    return {};
}

template<>
Utils::Result<Role> fromJson<Role>(const QJsonValue &val)
{
    const QString str = val.toString();
    if (str == "assistant") return Role::assistant;
    if (str == "user") return Role::user;
    return Utils::ResultError("Invalid Role value: " + str);
}

QJsonValue toJsonValue(const Role &v)
{
    return toString(v);
}

template<>
Utils::Result<Annotations> fromJson<Annotations>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Annotations");
    const QJsonObject obj = val.toObject();
    Annotations result;
    if (obj.contains("audience"))
        if (!obj["audience"].isNull()) {
            result._audience = obj.value("audience").toArray();
        }
    if (obj.contains("lastModified"))
        if (!obj["lastModified"].isNull()) {
            result._lastModified = obj.value("lastModified").toString();
        }
    if (obj.contains("priority"))
        if (!obj["priority"].isNull()) {
            result._priority = obj.value("priority").toDouble();
        }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const Annotations &data)
{
    QJsonObject obj;
    if (data._audience.has_value())
        obj.insert("audience", *data._audience);
    if (data._lastModified.has_value())
        obj.insert("lastModified", *data._lastModified);
    if (data._priority.has_value())
        obj.insert("priority", *data._priority);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<AudioContent> fromJson<AudioContent>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for AudioContent");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("data"))
        co_return Utils::ResultError("Missing required field: data");
    if (!obj.contains("mimeType"))
        co_return Utils::ResultError("Missing required field: mimeType");
    AudioContent result;
    if (obj.contains("annotations") && !obj["annotations"].isNull())
        result._annotations = co_await fromJson<Annotations>(obj["annotations"]);
    result._data = obj.value("data").toString();
    result._mimeType = obj.value("mimeType").toString();
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const AudioContent &data)
{
    QJsonObject obj{
        {"data", data._data},
        {"mimeType", data._mimeType}
    };
    if (data._annotations.has_value())
        obj.insert("annotations", toJson(*data._annotations));
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<BlobResourceContents> fromJson<BlobResourceContents>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for BlobResourceContents");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("blob"))
        return Utils::ResultError("Missing required field: blob");
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    BlobResourceContents result;
    result._blob = obj.value("blob").toString();
    if (obj.contains("mimeType"))
        if (!obj["mimeType"].isNull()) {
            result._mimeType = obj.value("mimeType").toString();
        }
    result._uri = obj.value("uri").toString();
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const BlobResourceContents &data)
{
    QJsonObject obj{
        {"blob", data._blob},
        {"uri", data._uri}
    };
    if (data._mimeType.has_value())
        obj.insert("mimeType", *data._mimeType);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<TextResourceContents> fromJson<TextResourceContents>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TextResourceContents");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("text"))
        return Utils::ResultError("Missing required field: text");
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    TextResourceContents result;
    if (obj.contains("mimeType"))
        if (!obj["mimeType"].isNull()) {
            result._mimeType = obj.value("mimeType").toString();
        }
    result._text = obj.value("text").toString();
    result._uri = obj.value("uri").toString();
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const TextResourceContents &data)
{
    QJsonObject obj{
        {"text", data._text},
        {"uri", data._uri}
    };
    if (data._mimeType.has_value())
        obj.insert("mimeType", *data._mimeType);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<EmbeddedResourceResource> fromJson<EmbeddedResourceResource>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Invalid EmbeddedResourceResource: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("text"))
        co_return EmbeddedResourceResource(co_await fromJson<TextResourceContents>(val));
    if (obj.contains("blob"))
        co_return EmbeddedResourceResource(co_await fromJson<BlobResourceContents>(val));
    co_return Utils::ResultError("Invalid EmbeddedResourceResource");
}

QString uri(const EmbeddedResourceResource &val)
{
    return std::visit([](const auto &v) -> QString { return v._uri; }, val);
}

QJsonObject toJson(const EmbeddedResourceResource &val)
{
    return std::visit([](const auto &v) -> QJsonObject {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
}

QJsonValue toJsonValue(const EmbeddedResourceResource &val)
{
    return toJson(val);
}

template<>
Utils::Result<EmbeddedResource> fromJson<EmbeddedResource>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for EmbeddedResource");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("resource"))
        co_return Utils::ResultError("Missing required field: resource");
    EmbeddedResource result;
    if (obj.contains("annotations") && !obj["annotations"].isNull())
        result._annotations = co_await fromJson<Annotations>(obj["annotations"]);
    if (obj.contains("resource"))
        result._resource = co_await fromJson<EmbeddedResourceResource>(obj["resource"]);
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const EmbeddedResource &data)
{
    QJsonObject obj{{"resource", toJsonValue(data._resource)}};
    if (data._annotations.has_value())
        obj.insert("annotations", toJson(*data._annotations));
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<ImageContent> fromJson<ImageContent>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for ImageContent");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("data"))
        co_return Utils::ResultError("Missing required field: data");
    if (!obj.contains("mimeType"))
        co_return Utils::ResultError("Missing required field: mimeType");
    ImageContent result;
    if (obj.contains("annotations") && !obj["annotations"].isNull())
        result._annotations = co_await fromJson<Annotations>(obj["annotations"]);
    result._data = obj.value("data").toString();
    result._mimeType = obj.value("mimeType").toString();
    if (obj.contains("uri"))
        if (!obj["uri"].isNull()) {
            result._uri = obj.value("uri").toString();
        }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const ImageContent &data)
{
    QJsonObject obj{
        {"data", data._data},
        {"mimeType", data._mimeType}
    };
    if (data._annotations.has_value())
        obj.insert("annotations", toJson(*data._annotations));
    if (data._uri.has_value())
        obj.insert("uri", *data._uri);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<ResourceLink> fromJson<ResourceLink>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for ResourceLink");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        co_return Utils::ResultError("Missing required field: name");
    if (!obj.contains("uri"))
        co_return Utils::ResultError("Missing required field: uri");
    ResourceLink result;
    if (obj.contains("annotations") && !obj["annotations"].isNull())
        result._annotations = co_await fromJson<Annotations>(obj["annotations"]);
    if (obj.contains("description"))
        if (!obj["description"].isNull()) {
            result._description = obj.value("description").toString();
        }
    if (obj.contains("mimeType"))
        if (!obj["mimeType"].isNull()) {
            result._mimeType = obj.value("mimeType").toString();
        }
    result._name = obj.value("name").toString();
    if (obj.contains("size"))
        if (!obj["size"].isNull()) {
            result._size = obj.value("size").toInt();
        }
    if (obj.contains("title"))
        if (!obj["title"].isNull()) {
            result._title = obj.value("title").toString();
        }
    result._uri = obj.value("uri").toString();
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const ResourceLink &data)
{
    QJsonObject obj{
        {"name", data._name},
        {"uri", data._uri}
    };
    if (data._annotations.has_value())
        obj.insert("annotations", toJson(*data._annotations));
    if (data._description.has_value())
        obj.insert("description", *data._description);
    if (data._mimeType.has_value())
        obj.insert("mimeType", *data._mimeType);
    if (data._size.has_value())
        obj.insert("size", *data._size);
    if (data._title.has_value())
        obj.insert("title", *data._title);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<TextContent> fromJson<TextContent>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for TextContent");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("text"))
        co_return Utils::ResultError("Missing required field: text");
    TextContent result;
    if (obj.contains("annotations") && !obj["annotations"].isNull())
        result._annotations = co_await fromJson<Annotations>(obj["annotations"]);
    result._text = obj.value("text").toString();
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const TextContent &data)
{
    QJsonObject obj{{"text", data._text}};
    if (data._annotations.has_value())
        obj.insert("annotations", toJson(*data._annotations));
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<ContentBlock> fromJson<ContentBlock>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Invalid ContentBlock: expected object");
    const QString dispatchValue = val.toObject().value("type").toString();
    if (dispatchValue == "text")
        co_return ContentBlock(co_await fromJson<TextContent>(val));
    else if (dispatchValue == "image")
        co_return ContentBlock(co_await fromJson<ImageContent>(val));
    else if (dispatchValue == "audio")
        co_return ContentBlock(co_await fromJson<AudioContent>(val));
    else if (dispatchValue == "resource_link")
        co_return ContentBlock(co_await fromJson<ResourceLink>(val));
    else if (dispatchValue == "resource")
        co_return ContentBlock(co_await fromJson<EmbeddedResource>(val));
    co_return Utils::ResultError("Invalid ContentBlock: unknown type \"" + dispatchValue + "\"");
}

QString dispatchValue(const ContentBlock &val)
{
    return std::visit([](const auto &v) -> QString {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, TextContent>) return "text";
        else if constexpr (std::is_same_v<T, ImageContent>) return "image";
        else if constexpr (std::is_same_v<T, AudioContent>) return "audio";
        else if constexpr (std::is_same_v<T, ResourceLink>) return "resource_link";
        else if constexpr (std::is_same_v<T, EmbeddedResource>) return "resource";
        return {};
    }, val);
}

QJsonObject toJson(const ContentBlock &val)
{
    QJsonObject obj = std::visit([](const auto &v) -> QJsonObject {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
    obj.insert("type", dispatchValue(val));
    return obj;
}

QJsonValue toJsonValue(const ContentBlock &val)
{
    return toJson(val);
}

template<>
Utils::Result<Content> fromJson<Content>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for Content");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("content"))
        co_return Utils::ResultError("Missing required field: content");
    Content result;
    if (obj.contains("content"))
        result._content = co_await fromJson<ContentBlock>(obj["content"]);
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const Content &data)
{
    QJsonObject obj{{"content", toJsonValue(data._content)}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<Diff> fromJson<Diff>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Diff");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("path"))
        return Utils::ResultError("Missing required field: path");
    if (!obj.contains("newText"))
        return Utils::ResultError("Missing required field: newText");
    Diff result;
    result._path = obj.value("path").toString();
    if (obj.contains("oldText"))
        if (!obj["oldText"].isNull()) {
            result._oldText = obj.value("oldText").toString();
        }
    result._newText = obj.value("newText").toString();
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const Diff &data)
{
    QJsonObject obj{
        {"path", data._path},
        {"newText", data._newText}
    };
    if (data._oldText.has_value())
        obj.insert("oldText", *data._oldText);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<Terminal> fromJson<Terminal>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for Terminal");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("terminalId"))
        co_return Utils::ResultError("Missing required field: terminalId");
    Terminal result;
    if (obj.contains("terminalId") && obj["terminalId"].isString())
        result._terminalId = co_await fromJson<TerminalId>(obj["terminalId"]);
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const Terminal &data)
{
    QJsonObject obj{{"terminalId", data._terminalId}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<ToolCallContent> fromJson<ToolCallContent>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Invalid ToolCallContent: expected object");
    const QString dispatchValue = val.toObject().value("type").toString();
    if (dispatchValue == "content")
        co_return ToolCallContent(co_await fromJson<Content>(val));
    else if (dispatchValue == "diff")
        co_return ToolCallContent(co_await fromJson<Diff>(val));
    else if (dispatchValue == "terminal")
        co_return ToolCallContent(co_await fromJson<Terminal>(val));
    co_return Utils::ResultError("Invalid ToolCallContent: unknown type \"" + dispatchValue + "\"");
}

QString dispatchValue(const ToolCallContent &val)
{
    return std::visit([](const auto &v) -> QString {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, Content>) return "content";
        else if constexpr (std::is_same_v<T, Diff>) return "diff";
        else if constexpr (std::is_same_v<T, Terminal>) return "terminal";
        return {};
    }, val);
}

QJsonObject toJson(const ToolCallContent &val)
{
    QJsonObject obj = std::visit([](const auto &v) -> QJsonObject {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
    obj.insert("type", dispatchValue(val));
    return obj;
}

QJsonValue toJsonValue(const ToolCallContent &val)
{
    return toJson(val);
}

template<>
Utils::Result<ToolCallLocation> fromJson<ToolCallLocation>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ToolCallLocation");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("path"))
        return Utils::ResultError("Missing required field: path");
    ToolCallLocation result;
    result._path = obj.value("path").toString();
    if (obj.contains("line"))
        if (!obj["line"].isNull()) {
            result._line = obj.value("line").toInt();
        }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const ToolCallLocation &data)
{
    QJsonObject obj{{"path", data._path}};
    if (data._line.has_value())
        obj.insert("line", *data._line);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

QString toString(ToolCallStatus v)
{
    switch(v) {
        case ToolCallStatus::pending: return "pending";
        case ToolCallStatus::in_progress: return "in_progress";
        case ToolCallStatus::completed: return "completed";
        case ToolCallStatus::failed: return "failed";
    }
    return {};
}

template<>
Utils::Result<ToolCallStatus> fromJson<ToolCallStatus>(const QJsonValue &val)
{
    const QString str = val.toString();
    if (str == "pending") return ToolCallStatus::pending;
    if (str == "in_progress") return ToolCallStatus::in_progress;
    if (str == "completed") return ToolCallStatus::completed;
    if (str == "failed") return ToolCallStatus::failed;
    return Utils::ResultError("Invalid ToolCallStatus value: " + str);
}

QJsonValue toJsonValue(const ToolCallStatus &v)
{
    return toString(v);
}

QString toString(ToolKind v)
{
    switch(v) {
        case ToolKind::read: return "read";
        case ToolKind::edit: return "edit";
        case ToolKind::delete_: return "delete";
        case ToolKind::move: return "move";
        case ToolKind::search: return "search";
        case ToolKind::execute: return "execute";
        case ToolKind::think: return "think";
        case ToolKind::fetch: return "fetch";
        case ToolKind::switch_mode: return "switch_mode";
        case ToolKind::other: return "other";
    }
    return {};
}

template<>
Utils::Result<ToolKind> fromJson<ToolKind>(const QJsonValue &val)
{
    const QString str = val.toString();
    if (str == "read") return ToolKind::read;
    if (str == "edit") return ToolKind::edit;
    if (str == "delete") return ToolKind::delete_;
    if (str == "move") return ToolKind::move;
    if (str == "search") return ToolKind::search;
    if (str == "execute") return ToolKind::execute;
    if (str == "think") return ToolKind::think;
    if (str == "fetch") return ToolKind::fetch;
    if (str == "switch_mode") return ToolKind::switch_mode;
    if (str == "other") return ToolKind::other;
    return Utils::ResultError("Invalid ToolKind value: " + str);
}

QJsonValue toJsonValue(const ToolKind &v)
{
    return toString(v);
}

template<>
Utils::Result<ToolCallUpdate> fromJson<ToolCallUpdate>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for ToolCallUpdate");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("toolCallId"))
        co_return Utils::ResultError("Missing required field: toolCallId");
    ToolCallUpdate result;
    if (obj.contains("toolCallId") && obj["toolCallId"].isString())
        result._toolCallId = co_await fromJson<ToolCallId>(obj["toolCallId"]);
    if (obj.contains("kind") && !obj["kind"].isNull())
        result._kind = co_await fromJson<ToolKind>(obj["kind"]);
    if (obj.contains("status") && !obj["status"].isNull())
        result._status = co_await fromJson<ToolCallStatus>(obj["status"]);
    if (obj.contains("title"))
        if (!obj["title"].isNull()) {
            result._title = obj.value("title").toString();
        }
    if (obj.contains("content"))
        if (!obj["content"].isNull()) {
            result._content = obj.value("content").toArray();
        }
    if (obj.contains("locations"))
        if (!obj["locations"].isNull()) {
            result._locations = obj.value("locations").toArray();
        }
    if (obj.contains("rawInput"))
        result._rawInput = obj.value("rawInput");
    if (obj.contains("rawOutput"))
        result._rawOutput = obj.value("rawOutput");
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const ToolCallUpdate &data)
{
    QJsonObject obj{{"toolCallId", data._toolCallId}};
    if (data._kind.has_value())
        obj.insert("kind", toJsonValue(*data._kind));
    if (data._status.has_value())
        obj.insert("status", toJsonValue(*data._status));
    if (data._title.has_value())
        obj.insert("title", *data._title);
    if (data._content.has_value())
        obj.insert("content", *data._content);
    if (data._locations.has_value())
        obj.insert("locations", *data._locations);
    if (data._rawInput.has_value())
        obj.insert("rawInput", *data._rawInput);
    if (data._rawOutput.has_value())
        obj.insert("rawOutput", *data._rawOutput);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<RequestPermissionRequest> fromJson<RequestPermissionRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for RequestPermissionRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        co_return Utils::ResultError("Missing required field: sessionId");
    if (!obj.contains("toolCall"))
        co_return Utils::ResultError("Missing required field: toolCall");
    if (!obj.contains("options"))
        co_return Utils::ResultError("Missing required field: options");
    RequestPermissionRequest result;
    if (obj.contains("sessionId") && obj["sessionId"].isString())
        result._sessionId = co_await fromJson<SessionId>(obj["sessionId"]);
    if (obj.contains("toolCall") && obj["toolCall"].isObject())
        result._toolCall = co_await fromJson<ToolCallUpdate>(obj["toolCall"]);
    if (obj.contains("options") && obj["options"].isArray()) {
        const QJsonArray arr = obj["options"].toArray();
        for (const QJsonValue &v : arr) {
            result._options.append(co_await fromJson<PermissionOption>(v));
        }
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const RequestPermissionRequest &data)
{
    QJsonObject obj{
        {"sessionId", data._sessionId},
        {"toolCall", toJson(data._toolCall)}
    };
    QJsonArray arr_options;
    for (const auto &v : data._options) arr_options.append(toJson(v));
    obj.insert("options", arr_options);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<TerminalOutputRequest> fromJson<TerminalOutputRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for TerminalOutputRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        co_return Utils::ResultError("Missing required field: sessionId");
    if (!obj.contains("terminalId"))
        co_return Utils::ResultError("Missing required field: terminalId");
    TerminalOutputRequest result;
    if (obj.contains("sessionId") && obj["sessionId"].isString())
        result._sessionId = co_await fromJson<SessionId>(obj["sessionId"]);
    if (obj.contains("terminalId") && obj["terminalId"].isString())
        result._terminalId = co_await fromJson<TerminalId>(obj["terminalId"]);
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const TerminalOutputRequest &data)
{
    QJsonObject obj{
        {"sessionId", data._sessionId},
        {"terminalId", data._terminalId}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<WaitForTerminalExitRequest> fromJson<WaitForTerminalExitRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for WaitForTerminalExitRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        co_return Utils::ResultError("Missing required field: sessionId");
    if (!obj.contains("terminalId"))
        co_return Utils::ResultError("Missing required field: terminalId");
    WaitForTerminalExitRequest result;
    if (obj.contains("sessionId") && obj["sessionId"].isString())
        result._sessionId = co_await fromJson<SessionId>(obj["sessionId"]);
    if (obj.contains("terminalId") && obj["terminalId"].isString())
        result._terminalId = co_await fromJson<TerminalId>(obj["terminalId"]);
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const WaitForTerminalExitRequest &data)
{
    QJsonObject obj{
        {"sessionId", data._sessionId},
        {"terminalId", data._terminalId}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<WriteTextFileRequest> fromJson<WriteTextFileRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for WriteTextFileRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        co_return Utils::ResultError("Missing required field: sessionId");
    if (!obj.contains("path"))
        co_return Utils::ResultError("Missing required field: path");
    if (!obj.contains("content"))
        co_return Utils::ResultError("Missing required field: content");
    WriteTextFileRequest result;
    if (obj.contains("sessionId") && obj["sessionId"].isString())
        result._sessionId = co_await fromJson<SessionId>(obj["sessionId"]);
    result._path = obj.value("path").toString();
    result._content = obj.value("content").toString();
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const WriteTextFileRequest &data)
{
    QJsonObject obj{
        {"sessionId", data._sessionId},
        {"path", data._path},
        {"content", data._content}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<AgentRequest> fromJson<AgentRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for AgentRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        co_return Utils::ResultError("Missing required field: id");
    if (!obj.contains("method"))
        co_return Utils::ResultError("Missing required field: method");
    AgentRequest result;
    if (obj.contains("id"))
        result._id = co_await fromJson<RequestId>(obj["id"]);
    result._method = obj.value("method").toString();
    if (obj.contains("params"))
        result._params = obj.value("params").toString();
    co_return result;
}

QJsonObject toJson(const AgentRequest &data)
{
    QJsonObject obj{
        {"id", toJsonValue(data._id)},
        {"method", data._method}
    };
    if (data._params.has_value())
        obj.insert("params", *data._params);
    return obj;
}

template<>
Utils::Result<AuthenticateResponse> fromJson<AuthenticateResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for AuthenticateResponse");
    const QJsonObject obj = val.toObject();
    AuthenticateResponse result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const AuthenticateResponse &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<CloseSessionResponse> fromJson<CloseSessionResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CloseSessionResponse");
    const QJsonObject obj = val.toObject();
    CloseSessionResponse result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const CloseSessionResponse &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<DeleteSessionResponse> fromJson<DeleteSessionResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DeleteSessionResponse");
    const QJsonObject obj = val.toObject();
    DeleteSessionResponse result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const DeleteSessionResponse &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<Error> fromJson<Error>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Error");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("code"))
        return Utils::ResultError("Missing required field: code");
    if (!obj.contains("message"))
        return Utils::ResultError("Missing required field: message");
    Error result;
    result._code = obj.value("code").toInt();
    result._message = obj.value("message").toString();
    if (obj.contains("data"))
        result._data = obj.value("data");
    return result;
}

QJsonObject toJson(const Error &data)
{
    QJsonObject obj{
        {"code", data._code},
        {"message", data._message}
    };
    if (data._data.has_value())
        obj.insert("data", *data._data);
    return obj;
}

template<>
Utils::Result<LogoutCapabilities> fromJson<LogoutCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for LogoutCapabilities");
    const QJsonObject obj = val.toObject();
    LogoutCapabilities result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const LogoutCapabilities &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<AgentAuthCapabilities> fromJson<AgentAuthCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for AgentAuthCapabilities");
    const QJsonObject obj = val.toObject();
    AgentAuthCapabilities result;
    if (obj.contains("logout") && !obj["logout"].isNull())
        result._logout = co_await fromJson<LogoutCapabilities>(obj["logout"]);
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const AgentAuthCapabilities &data)
{
    QJsonObject obj;
    if (data._logout.has_value())
        obj.insert("logout", toJson(*data._logout));
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<McpCapabilities> fromJson<McpCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for McpCapabilities");
    const QJsonObject obj = val.toObject();
    McpCapabilities result;
    if (obj.contains("http"))
        result._http = obj.value("http").toBool();
    if (obj.contains("sse"))
        result._sse = obj.value("sse").toBool();
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const McpCapabilities &data)
{
    QJsonObject obj;
    if (data._http.has_value())
        obj.insert("http", *data._http);
    if (data._sse.has_value())
        obj.insert("sse", *data._sse);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<PromptCapabilities> fromJson<PromptCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for PromptCapabilities");
    const QJsonObject obj = val.toObject();
    PromptCapabilities result;
    if (obj.contains("image"))
        result._image = obj.value("image").toBool();
    if (obj.contains("audio"))
        result._audio = obj.value("audio").toBool();
    if (obj.contains("embeddedContext"))
        result._embeddedContext = obj.value("embeddedContext").toBool();
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const PromptCapabilities &data)
{
    QJsonObject obj;
    if (data._image.has_value())
        obj.insert("image", *data._image);
    if (data._audio.has_value())
        obj.insert("audio", *data._audio);
    if (data._embeddedContext.has_value())
        obj.insert("embeddedContext", *data._embeddedContext);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<SessionAdditionalDirectoriesCapabilities> fromJson<SessionAdditionalDirectoriesCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SessionAdditionalDirectoriesCapabilities");
    const QJsonObject obj = val.toObject();
    SessionAdditionalDirectoriesCapabilities result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const SessionAdditionalDirectoriesCapabilities &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<SessionCloseCapabilities> fromJson<SessionCloseCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SessionCloseCapabilities");
    const QJsonObject obj = val.toObject();
    SessionCloseCapabilities result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const SessionCloseCapabilities &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<SessionDeleteCapabilities> fromJson<SessionDeleteCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SessionDeleteCapabilities");
    const QJsonObject obj = val.toObject();
    SessionDeleteCapabilities result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const SessionDeleteCapabilities &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<SessionListCapabilities> fromJson<SessionListCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SessionListCapabilities");
    const QJsonObject obj = val.toObject();
    SessionListCapabilities result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const SessionListCapabilities &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<SessionResumeCapabilities> fromJson<SessionResumeCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SessionResumeCapabilities");
    const QJsonObject obj = val.toObject();
    SessionResumeCapabilities result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const SessionResumeCapabilities &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<SessionCapabilities> fromJson<SessionCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for SessionCapabilities");
    const QJsonObject obj = val.toObject();
    SessionCapabilities result;
    if (obj.contains("list") && !obj["list"].isNull())
        result._list = co_await fromJson<SessionListCapabilities>(obj["list"]);
    if (obj.contains("delete") && !obj["delete"].isNull())
        result._delete_ = co_await fromJson<SessionDeleteCapabilities>(obj["delete"]);
    if (obj.contains("additionalDirectories") && !obj["additionalDirectories"].isNull())
        result._additionalDirectories = co_await fromJson<SessionAdditionalDirectoriesCapabilities>(obj["additionalDirectories"]);
    if (obj.contains("resume") && !obj["resume"].isNull())
        result._resume = co_await fromJson<SessionResumeCapabilities>(obj["resume"]);
    if (obj.contains("close") && !obj["close"].isNull())
        result._close = co_await fromJson<SessionCloseCapabilities>(obj["close"]);
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const SessionCapabilities &data)
{
    QJsonObject obj;
    if (data._list.has_value())
        obj.insert("list", toJson(*data._list));
    if (data._delete_.has_value())
        obj.insert("delete", toJson(*data._delete_));
    if (data._additionalDirectories.has_value())
        obj.insert("additionalDirectories", toJson(*data._additionalDirectories));
    if (data._resume.has_value())
        obj.insert("resume", toJson(*data._resume));
    if (data._close.has_value())
        obj.insert("close", toJson(*data._close));
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<AgentCapabilities> fromJson<AgentCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for AgentCapabilities");
    const QJsonObject obj = val.toObject();
    AgentCapabilities result;
    if (obj.contains("loadSession"))
        result._loadSession = obj.value("loadSession").toBool();
    if (obj.contains("promptCapabilities") && obj["promptCapabilities"].isObject())
        result._promptCapabilities = co_await fromJson<PromptCapabilities>(obj["promptCapabilities"]);
    if (obj.contains("mcpCapabilities") && obj["mcpCapabilities"].isObject())
        result._mcpCapabilities = co_await fromJson<McpCapabilities>(obj["mcpCapabilities"]);
    if (obj.contains("sessionCapabilities") && obj["sessionCapabilities"].isObject())
        result._sessionCapabilities = co_await fromJson<SessionCapabilities>(obj["sessionCapabilities"]);
    if (obj.contains("auth") && obj["auth"].isObject())
        result._auth = co_await fromJson<AgentAuthCapabilities>(obj["auth"]);
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const AgentCapabilities &data)
{
    QJsonObject obj;
    if (data._loadSession.has_value())
        obj.insert("loadSession", *data._loadSession);
    if (data._promptCapabilities.has_value())
        obj.insert("promptCapabilities", toJson(*data._promptCapabilities));
    if (data._mcpCapabilities.has_value())
        obj.insert("mcpCapabilities", toJson(*data._mcpCapabilities));
    if (data._sessionCapabilities.has_value())
        obj.insert("sessionCapabilities", toJson(*data._sessionCapabilities));
    if (data._auth.has_value())
        obj.insert("auth", toJson(*data._auth));
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<AuthMethodAgent> fromJson<AuthMethodAgent>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for AuthMethodAgent");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        co_return Utils::ResultError("Missing required field: id");
    if (!obj.contains("name"))
        co_return Utils::ResultError("Missing required field: name");
    AuthMethodAgent result;
    if (obj.contains("id") && obj["id"].isString())
        result._id = co_await fromJson<AuthMethodId>(obj["id"]);
    result._name = obj.value("name").toString();
    if (obj.contains("description"))
        if (!obj["description"].isNull()) {
            result._description = obj.value("description").toString();
        }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const AuthMethodAgent &data)
{
    QJsonObject obj{
        {"id", data._id},
        {"name", data._name}
    };
    if (data._description.has_value())
        obj.insert("description", *data._description);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<AuthMethod> fromJson<AuthMethod>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Invalid AuthMethod: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("id"))
        co_return AuthMethod(co_await fromJson<AuthMethodAgent>(val));
    co_return Utils::ResultError("Invalid AuthMethod");
}

QString name(const AuthMethod &val)
{
    return std::visit([](const auto &v) -> QString { return v._name; }, val);
}

QJsonObject toJson(const AuthMethod &val)
{
    return std::visit([](const auto &v) -> QJsonObject {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
}

QJsonValue toJsonValue(const AuthMethod &val)
{
    return toJson(val);
}

template<>
Utils::Result<Implementation> fromJson<Implementation>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Implementation");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    if (!obj.contains("version"))
        return Utils::ResultError("Missing required field: version");
    Implementation result;
    result._name = obj.value("name").toString();
    if (obj.contains("title"))
        if (!obj["title"].isNull()) {
            result._title = obj.value("title").toString();
        }
    result._version = obj.value("version").toString();
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const Implementation &data)
{
    QJsonObject obj{
        {"name", data._name},
        {"version", data._version}
    };
    if (data._title.has_value())
        obj.insert("title", *data._title);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<> Utils::Result<ProtocolVersion> fromJson<ProtocolVersion>(const QJsonValue &val)
{
    if (!val.isDouble()) return Utils::ResultError("Expected number");
    return static_cast<int>(val.toDouble());
}

template<>
Utils::Result<InitializeResponse> fromJson<InitializeResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for InitializeResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("protocolVersion"))
        co_return Utils::ResultError("Missing required field: protocolVersion");
    InitializeResponse result;
    if (obj.contains("protocolVersion") && obj["protocolVersion"].isDouble())
        result._protocolVersion = co_await fromJson<ProtocolVersion>(obj["protocolVersion"]);
    if (obj.contains("agentCapabilities") && obj["agentCapabilities"].isObject())
        result._agentCapabilities = co_await fromJson<AgentCapabilities>(obj["agentCapabilities"]);
    if (obj.contains("authMethods") && obj["authMethods"].isArray()) {
        const QJsonArray arr = obj["authMethods"].toArray();
        QList<AuthMethod> list_authMethods;
        for (const QJsonValue &v : arr) {
            list_authMethods.append(co_await fromJson<AuthMethod>(v));
        }
        result._authMethods = list_authMethods;
    }
    if (obj.contains("agentInfo") && !obj["agentInfo"].isNull())
        result._agentInfo = co_await fromJson<Implementation>(obj["agentInfo"]);
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const InitializeResponse &data)
{
    QJsonObject obj{{"protocolVersion", data._protocolVersion}};
    if (data._agentCapabilities.has_value())
        obj.insert("agentCapabilities", toJson(*data._agentCapabilities));
    if (data._authMethods.has_value()) {
        QJsonArray arr_authMethods;
        for (const auto &v : *data._authMethods) arr_authMethods.append(toJsonValue(v));
        obj.insert("authMethods", arr_authMethods);
    }
    if (data._agentInfo.has_value())
        obj.insert("agentInfo", toJson(*data._agentInfo));
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<SessionInfo> fromJson<SessionInfo>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for SessionInfo");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        co_return Utils::ResultError("Missing required field: sessionId");
    if (!obj.contains("cwd"))
        co_return Utils::ResultError("Missing required field: cwd");
    SessionInfo result;
    if (obj.contains("sessionId") && obj["sessionId"].isString())
        result._sessionId = co_await fromJson<SessionId>(obj["sessionId"]);
    result._cwd = obj.value("cwd").toString();
    if (obj.contains("additionalDirectories") && obj["additionalDirectories"].isArray()) {
        const QJsonArray arr = obj["additionalDirectories"].toArray();
        QStringList list_additionalDirectories;
        for (const QJsonValue &v : arr) {
            list_additionalDirectories.append(v.toString());
        }
        result._additionalDirectories = list_additionalDirectories;
    }
    if (obj.contains("title"))
        if (!obj["title"].isNull()) {
            result._title = obj.value("title").toString();
        }
    if (obj.contains("updatedAt"))
        if (!obj["updatedAt"].isNull()) {
            result._updatedAt = obj.value("updatedAt").toString();
        }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const SessionInfo &data)
{
    QJsonObject obj{
        {"sessionId", data._sessionId},
        {"cwd", data._cwd}
    };
    if (data._additionalDirectories.has_value()) {
        QJsonArray arr_additionalDirectories;
        for (const auto &v : *data._additionalDirectories) arr_additionalDirectories.append(v);
        obj.insert("additionalDirectories", arr_additionalDirectories);
    }
    if (data._title.has_value())
        obj.insert("title", *data._title);
    if (data._updatedAt.has_value())
        obj.insert("updatedAt", *data._updatedAt);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<ListSessionsResponse> fromJson<ListSessionsResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for ListSessionsResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessions"))
        co_return Utils::ResultError("Missing required field: sessions");
    ListSessionsResponse result;
    if (obj.contains("sessions") && obj["sessions"].isArray()) {
        const QJsonArray arr = obj["sessions"].toArray();
        for (const QJsonValue &v : arr) {
            result._sessions.append(co_await fromJson<SessionInfo>(v));
        }
    }
    if (obj.contains("nextCursor"))
        if (!obj["nextCursor"].isNull()) {
            result._nextCursor = obj.value("nextCursor").toString();
        }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const ListSessionsResponse &data)
{
    QJsonObject obj;
    QJsonArray arr_sessions;
    for (const auto &v : data._sessions) arr_sessions.append(toJson(v));
    obj.insert("sessions", arr_sessions);
    if (data._nextCursor.has_value())
        obj.insert("nextCursor", *data._nextCursor);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

QString toString(SessionConfigOptionCategory v)
{
    switch(v) {
        case SessionConfigOptionCategory::mode: return "mode";
        case SessionConfigOptionCategory::model: return "model";
        case SessionConfigOptionCategory::thought_level: return "thought_level";
    }
    return {};
}

template<>
Utils::Result<SessionConfigOptionCategory> fromJson<SessionConfigOptionCategory>(const QJsonValue &val)
{
    const QString str = val.toString();
    if (str == "mode") return SessionConfigOptionCategory::mode;
    if (str == "model") return SessionConfigOptionCategory::model;
    if (str == "thought_level") return SessionConfigOptionCategory::thought_level;
    return Utils::ResultError("Invalid SessionConfigOptionCategory value: " + str);
}

QJsonValue toJsonValue(const SessionConfigOptionCategory &v)
{
    return toString(v);
}

template<>
Utils::Result<SessionConfigSelectOption> fromJson<SessionConfigSelectOption>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for SessionConfigSelectOption");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("value"))
        co_return Utils::ResultError("Missing required field: value");
    if (!obj.contains("name"))
        co_return Utils::ResultError("Missing required field: name");
    SessionConfigSelectOption result;
    if (obj.contains("value") && obj["value"].isString())
        result._value = co_await fromJson<SessionConfigValueId>(obj["value"]);
    result._name = obj.value("name").toString();
    if (obj.contains("description"))
        if (!obj["description"].isNull()) {
            result._description = obj.value("description").toString();
        }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const SessionConfigSelectOption &data)
{
    QJsonObject obj{
        {"value", data._value},
        {"name", data._name}
    };
    if (data._description.has_value())
        obj.insert("description", *data._description);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<SessionConfigSelectGroup> fromJson<SessionConfigSelectGroup>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for SessionConfigSelectGroup");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("group"))
        co_return Utils::ResultError("Missing required field: group");
    if (!obj.contains("name"))
        co_return Utils::ResultError("Missing required field: name");
    if (!obj.contains("options"))
        co_return Utils::ResultError("Missing required field: options");
    SessionConfigSelectGroup result;
    if (obj.contains("group") && obj["group"].isString())
        result._group = co_await fromJson<SessionConfigGroupId>(obj["group"]);
    result._name = obj.value("name").toString();
    if (obj.contains("options") && obj["options"].isArray()) {
        const QJsonArray arr = obj["options"].toArray();
        for (const QJsonValue &v : arr) {
            result._options.append(co_await fromJson<SessionConfigSelectOption>(v));
        }
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const SessionConfigSelectGroup &data)
{
    QJsonObject obj{
        {"group", data._group},
        {"name", data._name}
    };
    QJsonArray arr_options;
    for (const auto &v : data._options) arr_options.append(toJson(v));
    obj.insert("options", arr_options);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<SessionConfigSelectOptions> fromJson<SessionConfigSelectOptions>(const QJsonValue &val)
{
    if (val.isArray()) {
        bool ok = true;
        QList<SessionConfigSelectOption> list;
        for (const auto &elem : val.toArray()) {
            auto r = fromJson<SessionConfigSelectOption>(elem);
            if (!r) { ok = false; break; }
            list.append(*r);
        }
        if (ok) return SessionConfigSelectOptions(std::move(list));
    }
    if (val.isArray()) {
        bool ok = true;
        QList<SessionConfigSelectGroup> list;
        for (const auto &elem : val.toArray()) {
            auto r = fromJson<SessionConfigSelectGroup>(elem);
            if (!r) { ok = false; break; }
            list.append(*r);
        }
        if (ok) return SessionConfigSelectOptions(std::move(list));
    }
    return Utils::ResultError("Invalid SessionConfigSelectOptions");
}

QJsonValue toJsonValue(const SessionConfigSelectOptions &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QList<SessionConfigSelectOption>>) {
            QJsonArray arr;
            for (const auto &elem : v)
                arr.append(toJson(elem));
            return arr;
        } else
        if constexpr (std::is_same_v<T, QList<SessionConfigSelectGroup>>) {
            QJsonArray arr;
            for (const auto &elem : v)
                arr.append(toJson(elem));
            return arr;
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<SessionConfigSelect> fromJson<SessionConfigSelect>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for SessionConfigSelect");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("currentValue"))
        co_return Utils::ResultError("Missing required field: currentValue");
    if (!obj.contains("options"))
        co_return Utils::ResultError("Missing required field: options");
    SessionConfigSelect result;
    if (obj.contains("currentValue") && obj["currentValue"].isString())
        result._currentValue = co_await fromJson<SessionConfigValueId>(obj["currentValue"]);
    if (obj.contains("options"))
        result._options = co_await fromJson<SessionConfigSelectOptions>(obj["options"]);
    co_return result;
}

QJsonObject toJson(const SessionConfigSelect &data)
{
    QJsonObject obj{
        {"currentValue", data._currentValue},
        {"options", toJsonValue(data._options)}
    };
    return obj;
}

template<>
Utils::Result<SessionConfigOption> fromJson<SessionConfigOption>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for SessionConfigOption");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        co_return Utils::ResultError("Missing required field: id");
    if (!obj.contains("name"))
        co_return Utils::ResultError("Missing required field: name");
    SessionConfigOption result;
    if (obj.contains("id") && obj["id"].isString())
        result._id = co_await fromJson<SessionConfigId>(obj["id"]);
    result._name = obj.value("name").toString();
    if (obj.contains("description"))
        if (!obj["description"].isNull()) {
            result._description = obj.value("description").toString();
        }
    if (obj.contains("category") && !obj["category"].isNull())
        result._category = co_await fromJson<SessionConfigOptionCategory>(obj["category"]);
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    {
        const QSet<QString> knownKeys{"id", "name", "description", "category", "_meta"};
        for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
            if (!knownKeys.contains(it.key()))
                result._additionalProperties.insert(it.key(), it.value());
        }
    }
    co_return result;
}

QJsonObject toJson(const SessionConfigOption &data)
{
    QJsonObject obj{
        {"id", data._id},
        {"name", data._name}
    };
    if (data._description.has_value())
        obj.insert("description", *data._description);
    if (data._category.has_value())
        obj.insert("category", toJsonValue(*data._category));
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    for (auto it = data._additionalProperties.constBegin(); it != data._additionalProperties.constEnd(); ++it)
        obj.insert(it.key(), it.value());
    return obj;
}

template<>
Utils::Result<SessionMode> fromJson<SessionMode>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for SessionMode");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        co_return Utils::ResultError("Missing required field: id");
    if (!obj.contains("name"))
        co_return Utils::ResultError("Missing required field: name");
    SessionMode result;
    if (obj.contains("id") && obj["id"].isString())
        result._id = co_await fromJson<SessionModeId>(obj["id"]);
    result._name = obj.value("name").toString();
    if (obj.contains("description"))
        if (!obj["description"].isNull()) {
            result._description = obj.value("description").toString();
        }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const SessionMode &data)
{
    QJsonObject obj{
        {"id", data._id},
        {"name", data._name}
    };
    if (data._description.has_value())
        obj.insert("description", *data._description);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<SessionModeState> fromJson<SessionModeState>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for SessionModeState");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("currentModeId"))
        co_return Utils::ResultError("Missing required field: currentModeId");
    if (!obj.contains("availableModes"))
        co_return Utils::ResultError("Missing required field: availableModes");
    SessionModeState result;
    if (obj.contains("currentModeId") && obj["currentModeId"].isString())
        result._currentModeId = co_await fromJson<SessionModeId>(obj["currentModeId"]);
    if (obj.contains("availableModes") && obj["availableModes"].isArray()) {
        const QJsonArray arr = obj["availableModes"].toArray();
        for (const QJsonValue &v : arr) {
            result._availableModes.append(co_await fromJson<SessionMode>(v));
        }
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const SessionModeState &data)
{
    QJsonObject obj{{"currentModeId", data._currentModeId}};
    QJsonArray arr_availableModes;
    for (const auto &v : data._availableModes) arr_availableModes.append(toJson(v));
    obj.insert("availableModes", arr_availableModes);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<LoadSessionResponse> fromJson<LoadSessionResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for LoadSessionResponse");
    const QJsonObject obj = val.toObject();
    LoadSessionResponse result;
    if (obj.contains("modes") && !obj["modes"].isNull())
        result._modes = co_await fromJson<SessionModeState>(obj["modes"]);
    if (obj.contains("configOptions"))
        if (!obj["configOptions"].isNull()) {
            result._configOptions = obj.value("configOptions").toArray();
        }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const LoadSessionResponse &data)
{
    QJsonObject obj;
    if (data._modes.has_value())
        obj.insert("modes", toJson(*data._modes));
    if (data._configOptions.has_value())
        obj.insert("configOptions", *data._configOptions);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<LogoutResponse> fromJson<LogoutResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for LogoutResponse");
    const QJsonObject obj = val.toObject();
    LogoutResponse result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const LogoutResponse &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<NewSessionResponse> fromJson<NewSessionResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for NewSessionResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        co_return Utils::ResultError("Missing required field: sessionId");
    NewSessionResponse result;
    if (obj.contains("sessionId") && obj["sessionId"].isString())
        result._sessionId = co_await fromJson<SessionId>(obj["sessionId"]);
    if (obj.contains("modes") && !obj["modes"].isNull())
        result._modes = co_await fromJson<SessionModeState>(obj["modes"]);
    if (obj.contains("configOptions"))
        if (!obj["configOptions"].isNull()) {
            result._configOptions = obj.value("configOptions").toArray();
        }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const NewSessionResponse &data)
{
    QJsonObject obj{{"sessionId", data._sessionId}};
    if (data._modes.has_value())
        obj.insert("modes", toJson(*data._modes));
    if (data._configOptions.has_value())
        obj.insert("configOptions", *data._configOptions);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

QString toString(StopReason v)
{
    switch(v) {
        case StopReason::end_turn: return "end_turn";
        case StopReason::max_tokens: return "max_tokens";
        case StopReason::max_turn_requests: return "max_turn_requests";
        case StopReason::refusal: return "refusal";
        case StopReason::cancelled: return "cancelled";
    }
    return {};
}

template<>
Utils::Result<StopReason> fromJson<StopReason>(const QJsonValue &val)
{
    const QString str = val.toString();
    if (str == "end_turn") return StopReason::end_turn;
    if (str == "max_tokens") return StopReason::max_tokens;
    if (str == "max_turn_requests") return StopReason::max_turn_requests;
    if (str == "refusal") return StopReason::refusal;
    if (str == "cancelled") return StopReason::cancelled;
    return Utils::ResultError("Invalid StopReason value: " + str);
}

QJsonValue toJsonValue(const StopReason &v)
{
    return toString(v);
}

template<>
Utils::Result<PromptResponse> fromJson<PromptResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for PromptResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("stopReason"))
        co_return Utils::ResultError("Missing required field: stopReason");
    PromptResponse result;
    if (obj.contains("stopReason") && obj["stopReason"].isString())
        result._stopReason = co_await fromJson<StopReason>(obj["stopReason"]);
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const PromptResponse &data)
{
    QJsonObject obj{{"stopReason", toJsonValue(data._stopReason)}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<ResumeSessionResponse> fromJson<ResumeSessionResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for ResumeSessionResponse");
    const QJsonObject obj = val.toObject();
    ResumeSessionResponse result;
    if (obj.contains("modes") && !obj["modes"].isNull())
        result._modes = co_await fromJson<SessionModeState>(obj["modes"]);
    if (obj.contains("configOptions"))
        if (!obj["configOptions"].isNull()) {
            result._configOptions = obj.value("configOptions").toArray();
        }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const ResumeSessionResponse &data)
{
    QJsonObject obj;
    if (data._modes.has_value())
        obj.insert("modes", toJson(*data._modes));
    if (data._configOptions.has_value())
        obj.insert("configOptions", *data._configOptions);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<SetSessionConfigOptionResponse> fromJson<SetSessionConfigOptionResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for SetSessionConfigOptionResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("configOptions"))
        co_return Utils::ResultError("Missing required field: configOptions");
    SetSessionConfigOptionResponse result;
    if (obj.contains("configOptions") && obj["configOptions"].isArray()) {
        const QJsonArray arr = obj["configOptions"].toArray();
        for (const QJsonValue &v : arr) {
            result._configOptions.append(co_await fromJson<SessionConfigOption>(v));
        }
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const SetSessionConfigOptionResponse &data)
{
    QJsonObject obj;
    QJsonArray arr_configOptions;
    for (const auto &v : data._configOptions) arr_configOptions.append(toJson(v));
    obj.insert("configOptions", arr_configOptions);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<SetSessionModeResponse> fromJson<SetSessionModeResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SetSessionModeResponse");
    const QJsonObject obj = val.toObject();
    SetSessionModeResponse result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const SetSessionModeResponse &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<AgentResponse> fromJson<AgentResponse>(const QJsonValue & /*val*/)
{
    return Utils::ResultError("Invalid AgentResponse");
}

QJsonValue toJsonValue(const AgentResponse &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<UnstructuredCommandInput> fromJson<UnstructuredCommandInput>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for UnstructuredCommandInput");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("hint"))
        return Utils::ResultError("Missing required field: hint");
    UnstructuredCommandInput result;
    result._hint = obj.value("hint").toString();
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const UnstructuredCommandInput &data)
{
    QJsonObject obj{{"hint", data._hint}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<AvailableCommandInput> fromJson<AvailableCommandInput>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Invalid AvailableCommandInput: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("hint"))
        co_return AvailableCommandInput(co_await fromJson<UnstructuredCommandInput>(val));
    co_return Utils::ResultError("Invalid AvailableCommandInput");
}

QString hint(const AvailableCommandInput &val)
{
    return std::visit([](const auto &v) -> QString { return v._hint; }, val);
}

QJsonObject toJson(const AvailableCommandInput &val)
{
    return std::visit([](const auto &v) -> QJsonObject {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
}

QJsonValue toJsonValue(const AvailableCommandInput &val)
{
    return toJson(val);
}

template<>
Utils::Result<AvailableCommand> fromJson<AvailableCommand>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for AvailableCommand");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        co_return Utils::ResultError("Missing required field: name");
    if (!obj.contains("description"))
        co_return Utils::ResultError("Missing required field: description");
    AvailableCommand result;
    result._name = obj.value("name").toString();
    result._description = obj.value("description").toString();
    if (obj.contains("input") && !obj["input"].isNull())
        result._input = co_await fromJson<AvailableCommandInput>(obj["input"]);
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const AvailableCommand &data)
{
    QJsonObject obj{
        {"name", data._name},
        {"description", data._description}
    };
    if (data._input.has_value())
        obj.insert("input", toJsonValue(*data._input));
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<AvailableCommandsUpdate> fromJson<AvailableCommandsUpdate>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for AvailableCommandsUpdate");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("availableCommands"))
        co_return Utils::ResultError("Missing required field: availableCommands");
    AvailableCommandsUpdate result;
    if (obj.contains("availableCommands") && obj["availableCommands"].isArray()) {
        const QJsonArray arr = obj["availableCommands"].toArray();
        for (const QJsonValue &v : arr) {
            result._availableCommands.append(co_await fromJson<AvailableCommand>(v));
        }
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const AvailableCommandsUpdate &data)
{
    QJsonObject obj;
    QJsonArray arr_availableCommands;
    for (const auto &v : data._availableCommands) arr_availableCommands.append(toJson(v));
    obj.insert("availableCommands", arr_availableCommands);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<ConfigOptionUpdate> fromJson<ConfigOptionUpdate>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for ConfigOptionUpdate");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("configOptions"))
        co_return Utils::ResultError("Missing required field: configOptions");
    ConfigOptionUpdate result;
    if (obj.contains("configOptions") && obj["configOptions"].isArray()) {
        const QJsonArray arr = obj["configOptions"].toArray();
        for (const QJsonValue &v : arr) {
            result._configOptions.append(co_await fromJson<SessionConfigOption>(v));
        }
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const ConfigOptionUpdate &data)
{
    QJsonObject obj;
    QJsonArray arr_configOptions;
    for (const auto &v : data._configOptions) arr_configOptions.append(toJson(v));
    obj.insert("configOptions", arr_configOptions);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<ContentChunk> fromJson<ContentChunk>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for ContentChunk");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("content"))
        co_return Utils::ResultError("Missing required field: content");
    ContentChunk result;
    if (obj.contains("content"))
        result._content = co_await fromJson<ContentBlock>(obj["content"]);
    if (obj.contains("messageId") && !obj["messageId"].isNull())
        result._messageId = co_await fromJson<MessageId>(obj["messageId"]);
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const ContentChunk &data)
{
    QJsonObject obj{{"content", toJsonValue(data._content)}};
    if (data._messageId.has_value())
        obj.insert("messageId", *data._messageId);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<CurrentModeUpdate> fromJson<CurrentModeUpdate>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for CurrentModeUpdate");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("currentModeId"))
        co_return Utils::ResultError("Missing required field: currentModeId");
    CurrentModeUpdate result;
    if (obj.contains("currentModeId") && obj["currentModeId"].isString())
        result._currentModeId = co_await fromJson<SessionModeId>(obj["currentModeId"]);
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const CurrentModeUpdate &data)
{
    QJsonObject obj{{"currentModeId", data._currentModeId}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

QString toString(PlanEntryPriority v)
{
    switch(v) {
        case PlanEntryPriority::high: return "high";
        case PlanEntryPriority::medium: return "medium";
        case PlanEntryPriority::low: return "low";
    }
    return {};
}

template<>
Utils::Result<PlanEntryPriority> fromJson<PlanEntryPriority>(const QJsonValue &val)
{
    const QString str = val.toString();
    if (str == "high") return PlanEntryPriority::high;
    if (str == "medium") return PlanEntryPriority::medium;
    if (str == "low") return PlanEntryPriority::low;
    return Utils::ResultError("Invalid PlanEntryPriority value: " + str);
}

QJsonValue toJsonValue(const PlanEntryPriority &v)
{
    return toString(v);
}

QString toString(PlanEntryStatus v)
{
    switch(v) {
        case PlanEntryStatus::pending: return "pending";
        case PlanEntryStatus::in_progress: return "in_progress";
        case PlanEntryStatus::completed: return "completed";
    }
    return {};
}

template<>
Utils::Result<PlanEntryStatus> fromJson<PlanEntryStatus>(const QJsonValue &val)
{
    const QString str = val.toString();
    if (str == "pending") return PlanEntryStatus::pending;
    if (str == "in_progress") return PlanEntryStatus::in_progress;
    if (str == "completed") return PlanEntryStatus::completed;
    return Utils::ResultError("Invalid PlanEntryStatus value: " + str);
}

QJsonValue toJsonValue(const PlanEntryStatus &v)
{
    return toString(v);
}

template<>
Utils::Result<PlanEntry> fromJson<PlanEntry>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for PlanEntry");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("content"))
        co_return Utils::ResultError("Missing required field: content");
    if (!obj.contains("priority"))
        co_return Utils::ResultError("Missing required field: priority");
    if (!obj.contains("status"))
        co_return Utils::ResultError("Missing required field: status");
    PlanEntry result;
    result._content = obj.value("content").toString();
    if (obj.contains("priority") && obj["priority"].isString())
        result._priority = co_await fromJson<PlanEntryPriority>(obj["priority"]);
    if (obj.contains("status") && obj["status"].isString())
        result._status = co_await fromJson<PlanEntryStatus>(obj["status"]);
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const PlanEntry &data)
{
    QJsonObject obj{
        {"content", data._content},
        {"priority", toJsonValue(data._priority)},
        {"status", toJsonValue(data._status)}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<Plan> fromJson<Plan>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for Plan");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("entries"))
        co_return Utils::ResultError("Missing required field: entries");
    Plan result;
    if (obj.contains("entries") && obj["entries"].isArray()) {
        const QJsonArray arr = obj["entries"].toArray();
        for (const QJsonValue &v : arr) {
            result._entries.append(co_await fromJson<PlanEntry>(v));
        }
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const Plan &data)
{
    QJsonObject obj;
    QJsonArray arr_entries;
    for (const auto &v : data._entries) arr_entries.append(toJson(v));
    obj.insert("entries", arr_entries);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<SessionInfoUpdate> fromJson<SessionInfoUpdate>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SessionInfoUpdate");
    const QJsonObject obj = val.toObject();
    SessionInfoUpdate result;
    if (obj.contains("title"))
        if (!obj["title"].isNull()) {
            result._title = obj.value("title").toString();
        }
    if (obj.contains("updatedAt"))
        if (!obj["updatedAt"].isNull()) {
            result._updatedAt = obj.value("updatedAt").toString();
        }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const SessionInfoUpdate &data)
{
    QJsonObject obj;
    if (data._title.has_value())
        obj.insert("title", *data._title);
    if (data._updatedAt.has_value())
        obj.insert("updatedAt", *data._updatedAt);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<ToolCall> fromJson<ToolCall>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for ToolCall");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("toolCallId"))
        co_return Utils::ResultError("Missing required field: toolCallId");
    if (!obj.contains("title"))
        co_return Utils::ResultError("Missing required field: title");
    ToolCall result;
    if (obj.contains("toolCallId") && obj["toolCallId"].isString())
        result._toolCallId = co_await fromJson<ToolCallId>(obj["toolCallId"]);
    result._title = obj.value("title").toString();
    if (obj.contains("kind") && obj["kind"].isString())
        result._kind = co_await fromJson<ToolKind>(obj["kind"]);
    if (obj.contains("status") && obj["status"].isString())
        result._status = co_await fromJson<ToolCallStatus>(obj["status"]);
    if (obj.contains("content") && obj["content"].isArray()) {
        const QJsonArray arr = obj["content"].toArray();
        QList<ToolCallContent> list_content;
        for (const QJsonValue &v : arr) {
            list_content.append(co_await fromJson<ToolCallContent>(v));
        }
        result._content = list_content;
    }
    if (obj.contains("locations") && obj["locations"].isArray()) {
        const QJsonArray arr = obj["locations"].toArray();
        QList<ToolCallLocation> list_locations;
        for (const QJsonValue &v : arr) {
            list_locations.append(co_await fromJson<ToolCallLocation>(v));
        }
        result._locations = list_locations;
    }
    if (obj.contains("rawInput"))
        result._rawInput = obj.value("rawInput");
    if (obj.contains("rawOutput"))
        result._rawOutput = obj.value("rawOutput");
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const ToolCall &data)
{
    QJsonObject obj{
        {"toolCallId", data._toolCallId},
        {"title", data._title}
    };
    if (data._kind.has_value())
        obj.insert("kind", toJsonValue(*data._kind));
    if (data._status.has_value())
        obj.insert("status", toJsonValue(*data._status));
    if (data._content.has_value()) {
        QJsonArray arr_content;
        for (const auto &v : *data._content) arr_content.append(toJsonValue(v));
        obj.insert("content", arr_content);
    }
    if (data._locations.has_value()) {
        QJsonArray arr_locations;
        for (const auto &v : *data._locations) arr_locations.append(toJson(v));
        obj.insert("locations", arr_locations);
    }
    if (data._rawInput.has_value())
        obj.insert("rawInput", *data._rawInput);
    if (data._rawOutput.has_value())
        obj.insert("rawOutput", *data._rawOutput);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<Cost> fromJson<Cost>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Cost");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("amount"))
        return Utils::ResultError("Missing required field: amount");
    if (!obj.contains("currency"))
        return Utils::ResultError("Missing required field: currency");
    Cost result;
    result._amount = obj.value("amount").toDouble();
    result._currency = obj.value("currency").toString();
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const Cost &data)
{
    QJsonObject obj{
        {"amount", data._amount},
        {"currency", data._currency}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<UsageUpdate> fromJson<UsageUpdate>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for UsageUpdate");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("used"))
        co_return Utils::ResultError("Missing required field: used");
    if (!obj.contains("size"))
        co_return Utils::ResultError("Missing required field: size");
    UsageUpdate result;
    result._used = obj.value("used").toInt();
    result._size = obj.value("size").toInt();
    if (obj.contains("cost") && !obj["cost"].isNull())
        result._cost = co_await fromJson<Cost>(obj["cost"]);
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const UsageUpdate &data)
{
    QJsonObject obj{
        {"used", data._used},
        {"size", data._size}
    };
    if (data._cost.has_value())
        obj.insert("cost", toJson(*data._cost));
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<SessionUpdate> fromJson<SessionUpdate>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Invalid SessionUpdate: expected object");
    const QJsonObject obj = val.toObject();
    const QString kind = obj.value("sessionUpdate").toString();
    SessionUpdate result;
    result._kind = kind;
    if (kind == "user_message_chunk")
        result._value = co_await fromJson<ContentChunk>(val);
    else if (kind == "agent_message_chunk")
        result._value = co_await fromJson<ContentChunk>(val);
    else if (kind == "agent_thought_chunk")
        result._value = co_await fromJson<ContentChunk>(val);
    else if (kind == "tool_call")
        result._value = co_await fromJson<ToolCall>(val);
    else if (kind == "tool_call_update")
        result._value = co_await fromJson<ToolCallUpdate>(val);
    else if (kind == "plan")
        result._value = co_await fromJson<Plan>(val);
    else if (kind == "available_commands_update")
        result._value = co_await fromJson<AvailableCommandsUpdate>(val);
    else if (kind == "current_mode_update")
        result._value = co_await fromJson<CurrentModeUpdate>(val);
    else if (kind == "config_option_update")
        result._value = co_await fromJson<ConfigOptionUpdate>(val);
    else if (kind == "session_info_update")
        result._value = co_await fromJson<SessionInfoUpdate>(val);
    else if (kind == "usage_update")
        result._value = co_await fromJson<UsageUpdate>(val);
    else
        co_return Utils::ResultError("Invalid SessionUpdate: unknown sessionUpdate \"" + kind + "\"");
    co_return result;
}

QJsonObject toJson(const SessionUpdate &data)
{
    QJsonObject obj = std::visit([](const auto &v) -> QJsonObject {
        return toJson(v);
    }, data._value);
    obj.insert("sessionUpdate", data._kind);
    return obj;
}

QJsonValue toJsonValue(const SessionUpdate &val)
{
    return toJson(val);
}

template<>
Utils::Result<SessionNotification> fromJson<SessionNotification>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for SessionNotification");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        co_return Utils::ResultError("Missing required field: sessionId");
    if (!obj.contains("update"))
        co_return Utils::ResultError("Missing required field: update");
    SessionNotification result;
    if (obj.contains("sessionId") && obj["sessionId"].isString())
        result._sessionId = co_await fromJson<SessionId>(obj["sessionId"]);
    if (obj.contains("update"))
        result._update = co_await fromJson<SessionUpdate>(obj["update"]);
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const SessionNotification &data)
{
    QJsonObject obj{
        {"sessionId", data._sessionId},
        {"update", toJsonValue(data._update)}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<AgentNotification> fromJson<AgentNotification>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for AgentNotification");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    AgentNotification result;
    result._method = obj.value("method").toString();
    if (obj.contains("params"))
        result._params = obj.value("params").toString();
    return result;
}

QJsonObject toJson(const AgentNotification &data)
{
    QJsonObject obj{{"method", data._method}};
    if (data._params.has_value())
        obj.insert("params", *data._params);
    return obj;
}

template<>
Utils::Result<AuthenticateRequest> fromJson<AuthenticateRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for AuthenticateRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("methodId"))
        co_return Utils::ResultError("Missing required field: methodId");
    AuthenticateRequest result;
    if (obj.contains("methodId") && obj["methodId"].isString())
        result._methodId = co_await fromJson<AuthMethodId>(obj["methodId"]);
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const AuthenticateRequest &data)
{
    QJsonObject obj{{"methodId", data._methodId}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<CloseSessionRequest> fromJson<CloseSessionRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for CloseSessionRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        co_return Utils::ResultError("Missing required field: sessionId");
    CloseSessionRequest result;
    if (obj.contains("sessionId") && obj["sessionId"].isString())
        result._sessionId = co_await fromJson<SessionId>(obj["sessionId"]);
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const CloseSessionRequest &data)
{
    QJsonObject obj{{"sessionId", data._sessionId}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<DeleteSessionRequest> fromJson<DeleteSessionRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for DeleteSessionRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        co_return Utils::ResultError("Missing required field: sessionId");
    DeleteSessionRequest result;
    if (obj.contains("sessionId") && obj["sessionId"].isString())
        result._sessionId = co_await fromJson<SessionId>(obj["sessionId"]);
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const DeleteSessionRequest &data)
{
    QJsonObject obj{{"sessionId", data._sessionId}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<FileSystemCapabilities> fromJson<FileSystemCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for FileSystemCapabilities");
    const QJsonObject obj = val.toObject();
    FileSystemCapabilities result;
    if (obj.contains("readTextFile"))
        result._readTextFile = obj.value("readTextFile").toBool();
    if (obj.contains("writeTextFile"))
        result._writeTextFile = obj.value("writeTextFile").toBool();
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const FileSystemCapabilities &data)
{
    QJsonObject obj;
    if (data._readTextFile.has_value())
        obj.insert("readTextFile", *data._readTextFile);
    if (data._writeTextFile.has_value())
        obj.insert("writeTextFile", *data._writeTextFile);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<ClientCapabilities> fromJson<ClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for ClientCapabilities");
    const QJsonObject obj = val.toObject();
    ClientCapabilities result;
    if (obj.contains("fs") && obj["fs"].isObject())
        result._fs = co_await fromJson<FileSystemCapabilities>(obj["fs"]);
    if (obj.contains("terminal"))
        result._terminal = obj.value("terminal").toBool();
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const ClientCapabilities &data)
{
    QJsonObject obj;
    if (data._fs.has_value())
        obj.insert("fs", toJson(*data._fs));
    if (data._terminal.has_value())
        obj.insert("terminal", *data._terminal);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<InitializeRequest> fromJson<InitializeRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for InitializeRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("protocolVersion"))
        co_return Utils::ResultError("Missing required field: protocolVersion");
    InitializeRequest result;
    if (obj.contains("protocolVersion") && obj["protocolVersion"].isDouble())
        result._protocolVersion = co_await fromJson<ProtocolVersion>(obj["protocolVersion"]);
    if (obj.contains("clientCapabilities") && obj["clientCapabilities"].isObject())
        result._clientCapabilities = co_await fromJson<ClientCapabilities>(obj["clientCapabilities"]);
    if (obj.contains("clientInfo") && !obj["clientInfo"].isNull())
        result._clientInfo = co_await fromJson<Implementation>(obj["clientInfo"]);
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const InitializeRequest &data)
{
    QJsonObject obj{{"protocolVersion", data._protocolVersion}};
    if (data._clientCapabilities.has_value())
        obj.insert("clientCapabilities", toJson(*data._clientCapabilities));
    if (data._clientInfo.has_value())
        obj.insert("clientInfo", toJson(*data._clientInfo));
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<ListSessionsRequest> fromJson<ListSessionsRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ListSessionsRequest");
    const QJsonObject obj = val.toObject();
    ListSessionsRequest result;
    if (obj.contains("cwd"))
        if (!obj["cwd"].isNull()) {
            result._cwd = obj.value("cwd").toString();
        }
    if (obj.contains("cursor"))
        if (!obj["cursor"].isNull()) {
            result._cursor = obj.value("cursor").toString();
        }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const ListSessionsRequest &data)
{
    QJsonObject obj;
    if (data._cwd.has_value())
        obj.insert("cwd", *data._cwd);
    if (data._cursor.has_value())
        obj.insert("cursor", *data._cursor);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<HttpHeader> fromJson<HttpHeader>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for HttpHeader");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    if (!obj.contains("value"))
        return Utils::ResultError("Missing required field: value");
    HttpHeader result;
    result._name = obj.value("name").toString();
    result._value = obj.value("value").toString();
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const HttpHeader &data)
{
    QJsonObject obj{
        {"name", data._name},
        {"value", data._value}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<McpServerHttp> fromJson<McpServerHttp>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for McpServerHttp");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        co_return Utils::ResultError("Missing required field: name");
    if (!obj.contains("url"))
        co_return Utils::ResultError("Missing required field: url");
    if (!obj.contains("headers"))
        co_return Utils::ResultError("Missing required field: headers");
    McpServerHttp result;
    result._name = obj.value("name").toString();
    result._url = obj.value("url").toString();
    if (obj.contains("headers") && obj["headers"].isArray()) {
        const QJsonArray arr = obj["headers"].toArray();
        for (const QJsonValue &v : arr) {
            result._headers.append(co_await fromJson<HttpHeader>(v));
        }
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const McpServerHttp &data)
{
    QJsonObject obj{
        {"name", data._name},
        {"url", data._url}
    };
    QJsonArray arr_headers;
    for (const auto &v : data._headers) arr_headers.append(toJson(v));
    obj.insert("headers", arr_headers);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<McpServerSse> fromJson<McpServerSse>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for McpServerSse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        co_return Utils::ResultError("Missing required field: name");
    if (!obj.contains("url"))
        co_return Utils::ResultError("Missing required field: url");
    if (!obj.contains("headers"))
        co_return Utils::ResultError("Missing required field: headers");
    McpServerSse result;
    result._name = obj.value("name").toString();
    result._url = obj.value("url").toString();
    if (obj.contains("headers") && obj["headers"].isArray()) {
        const QJsonArray arr = obj["headers"].toArray();
        for (const QJsonValue &v : arr) {
            result._headers.append(co_await fromJson<HttpHeader>(v));
        }
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const McpServerSse &data)
{
    QJsonObject obj{
        {"name", data._name},
        {"url", data._url}
    };
    QJsonArray arr_headers;
    for (const auto &v : data._headers) arr_headers.append(toJson(v));
    obj.insert("headers", arr_headers);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<McpServerStdio> fromJson<McpServerStdio>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for McpServerStdio");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        co_return Utils::ResultError("Missing required field: name");
    if (!obj.contains("command"))
        co_return Utils::ResultError("Missing required field: command");
    if (!obj.contains("args"))
        co_return Utils::ResultError("Missing required field: args");
    if (!obj.contains("env"))
        co_return Utils::ResultError("Missing required field: env");
    McpServerStdio result;
    result._name = obj.value("name").toString();
    result._command = obj.value("command").toString();
    if (obj.contains("args") && obj["args"].isArray()) {
        const QJsonArray arr = obj["args"].toArray();
        for (const QJsonValue &v : arr) {
            result._args.append(v.toString());
        }
    }
    if (obj.contains("env") && obj["env"].isArray()) {
        const QJsonArray arr = obj["env"].toArray();
        for (const QJsonValue &v : arr) {
            result._env.append(co_await fromJson<EnvVariable>(v));
        }
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const McpServerStdio &data)
{
    QJsonObject obj{
        {"name", data._name},
        {"command", data._command}
    };
    QJsonArray arr_args;
    for (const auto &v : data._args) arr_args.append(v);
    obj.insert("args", arr_args);
    QJsonArray arr_env;
    for (const auto &v : data._env) arr_env.append(toJson(v));
    obj.insert("env", arr_env);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<McpServer> fromJson<McpServer>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Invalid McpServer: expected object");
    const QString dispatchValue = val.toObject().value("type").toString();
    if (dispatchValue == "http")
        co_return McpServer(co_await fromJson<McpServerHttp>(val));
    else if (dispatchValue == "sse")
        co_return McpServer(co_await fromJson<McpServerSse>(val));
    else if (dispatchValue == "stdio")
        co_return McpServer(co_await fromJson<McpServerStdio>(val));
    co_return Utils::ResultError("Invalid McpServer: unknown type \"" + dispatchValue + "\"");
}

QString dispatchValue(const McpServer &val)
{
    return std::visit([](const auto &v) -> QString {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, McpServerHttp>) return "http";
        else if constexpr (std::is_same_v<T, McpServerSse>) return "sse";
        else if constexpr (std::is_same_v<T, McpServerStdio>) return "stdio";
        return {};
    }, val);
}

QJsonObject toJson(const McpServer &val)
{
    QJsonObject obj = std::visit([](const auto &v) -> QJsonObject {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
    obj.insert("type", dispatchValue(val));
    return obj;
}

QJsonValue toJsonValue(const McpServer &val)
{
    return toJson(val);
}

QString name(const McpServer &val)
{
    return std::visit([](const auto &v) -> QString { return v._name; }, val);
}

template<>
Utils::Result<LoadSessionRequest> fromJson<LoadSessionRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for LoadSessionRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("mcpServers"))
        co_return Utils::ResultError("Missing required field: mcpServers");
    if (!obj.contains("cwd"))
        co_return Utils::ResultError("Missing required field: cwd");
    if (!obj.contains("sessionId"))
        co_return Utils::ResultError("Missing required field: sessionId");
    LoadSessionRequest result;
    if (obj.contains("mcpServers") && obj["mcpServers"].isArray()) {
        const QJsonArray arr = obj["mcpServers"].toArray();
        for (const QJsonValue &v : arr) {
            result._mcpServers.append(co_await fromJson<McpServer>(v));
        }
    }
    result._cwd = obj.value("cwd").toString();
    if (obj.contains("additionalDirectories") && obj["additionalDirectories"].isArray()) {
        const QJsonArray arr = obj["additionalDirectories"].toArray();
        QStringList list_additionalDirectories;
        for (const QJsonValue &v : arr) {
            list_additionalDirectories.append(v.toString());
        }
        result._additionalDirectories = list_additionalDirectories;
    }
    if (obj.contains("sessionId") && obj["sessionId"].isString())
        result._sessionId = co_await fromJson<SessionId>(obj["sessionId"]);
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const LoadSessionRequest &data)
{
    QJsonObject obj{
        {"cwd", data._cwd},
        {"sessionId", data._sessionId}
    };
    QJsonArray arr_mcpServers;
    for (const auto &v : data._mcpServers) arr_mcpServers.append(toJsonValue(v));
    obj.insert("mcpServers", arr_mcpServers);
    if (data._additionalDirectories.has_value()) {
        QJsonArray arr_additionalDirectories;
        for (const auto &v : *data._additionalDirectories) arr_additionalDirectories.append(v);
        obj.insert("additionalDirectories", arr_additionalDirectories);
    }
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<LogoutRequest> fromJson<LogoutRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for LogoutRequest");
    const QJsonObject obj = val.toObject();
    LogoutRequest result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const LogoutRequest &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<NewSessionRequest> fromJson<NewSessionRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for NewSessionRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("cwd"))
        co_return Utils::ResultError("Missing required field: cwd");
    if (!obj.contains("mcpServers"))
        co_return Utils::ResultError("Missing required field: mcpServers");
    NewSessionRequest result;
    result._cwd = obj.value("cwd").toString();
    if (obj.contains("additionalDirectories") && obj["additionalDirectories"].isArray()) {
        const QJsonArray arr = obj["additionalDirectories"].toArray();
        QStringList list_additionalDirectories;
        for (const QJsonValue &v : arr) {
            list_additionalDirectories.append(v.toString());
        }
        result._additionalDirectories = list_additionalDirectories;
    }
    if (obj.contains("mcpServers") && obj["mcpServers"].isArray()) {
        const QJsonArray arr = obj["mcpServers"].toArray();
        for (const QJsonValue &v : arr) {
            result._mcpServers.append(co_await fromJson<McpServer>(v));
        }
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const NewSessionRequest &data)
{
    QJsonObject obj{{"cwd", data._cwd}};
    if (data._additionalDirectories.has_value()) {
        QJsonArray arr_additionalDirectories;
        for (const auto &v : *data._additionalDirectories) arr_additionalDirectories.append(v);
        obj.insert("additionalDirectories", arr_additionalDirectories);
    }
    QJsonArray arr_mcpServers;
    for (const auto &v : data._mcpServers) arr_mcpServers.append(toJsonValue(v));
    obj.insert("mcpServers", arr_mcpServers);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<PromptRequest> fromJson<PromptRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for PromptRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        co_return Utils::ResultError("Missing required field: sessionId");
    if (!obj.contains("prompt"))
        co_return Utils::ResultError("Missing required field: prompt");
    PromptRequest result;
    if (obj.contains("sessionId") && obj["sessionId"].isString())
        result._sessionId = co_await fromJson<SessionId>(obj["sessionId"]);
    if (obj.contains("prompt") && obj["prompt"].isArray()) {
        const QJsonArray arr = obj["prompt"].toArray();
        for (const QJsonValue &v : arr) {
            result._prompt.append(co_await fromJson<ContentBlock>(v));
        }
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const PromptRequest &data)
{
    QJsonObject obj{{"sessionId", data._sessionId}};
    QJsonArray arr_prompt;
    for (const auto &v : data._prompt) arr_prompt.append(toJsonValue(v));
    obj.insert("prompt", arr_prompt);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<ResumeSessionRequest> fromJson<ResumeSessionRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for ResumeSessionRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        co_return Utils::ResultError("Missing required field: sessionId");
    if (!obj.contains("cwd"))
        co_return Utils::ResultError("Missing required field: cwd");
    ResumeSessionRequest result;
    if (obj.contains("sessionId") && obj["sessionId"].isString())
        result._sessionId = co_await fromJson<SessionId>(obj["sessionId"]);
    result._cwd = obj.value("cwd").toString();
    if (obj.contains("additionalDirectories") && obj["additionalDirectories"].isArray()) {
        const QJsonArray arr = obj["additionalDirectories"].toArray();
        QStringList list_additionalDirectories;
        for (const QJsonValue &v : arr) {
            list_additionalDirectories.append(v.toString());
        }
        result._additionalDirectories = list_additionalDirectories;
    }
    if (obj.contains("mcpServers") && obj["mcpServers"].isArray()) {
        const QJsonArray arr = obj["mcpServers"].toArray();
        QList<McpServer> list_mcpServers;
        for (const QJsonValue &v : arr) {
            list_mcpServers.append(co_await fromJson<McpServer>(v));
        }
        result._mcpServers = list_mcpServers;
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const ResumeSessionRequest &data)
{
    QJsonObject obj{
        {"sessionId", data._sessionId},
        {"cwd", data._cwd}
    };
    if (data._additionalDirectories.has_value()) {
        QJsonArray arr_additionalDirectories;
        for (const auto &v : *data._additionalDirectories) arr_additionalDirectories.append(v);
        obj.insert("additionalDirectories", arr_additionalDirectories);
    }
    if (data._mcpServers.has_value()) {
        QJsonArray arr_mcpServers;
        for (const auto &v : *data._mcpServers) arr_mcpServers.append(toJsonValue(v));
        obj.insert("mcpServers", arr_mcpServers);
    }
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<SetSessionConfigOptionRequest> fromJson<SetSessionConfigOptionRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for SetSessionConfigOptionRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        co_return Utils::ResultError("Missing required field: sessionId");
    if (!obj.contains("configId"))
        co_return Utils::ResultError("Missing required field: configId");
    if (!obj.contains("value"))
        co_return Utils::ResultError("Missing required field: value");
    SetSessionConfigOptionRequest result;
    if (obj.contains("sessionId") && obj["sessionId"].isString())
        result._sessionId = co_await fromJson<SessionId>(obj["sessionId"]);
    if (obj.contains("configId") && obj["configId"].isString())
        result._configId = co_await fromJson<SessionConfigId>(obj["configId"]);
    if (obj.contains("value") && obj["value"].isString())
        result._value = co_await fromJson<SessionConfigValueId>(obj["value"]);
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const SetSessionConfigOptionRequest &data)
{
    QJsonObject obj{
        {"sessionId", data._sessionId},
        {"configId", data._configId},
        {"value", data._value}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<SetSessionModeRequest> fromJson<SetSessionModeRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for SetSessionModeRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        co_return Utils::ResultError("Missing required field: sessionId");
    if (!obj.contains("modeId"))
        co_return Utils::ResultError("Missing required field: modeId");
    SetSessionModeRequest result;
    if (obj.contains("sessionId") && obj["sessionId"].isString())
        result._sessionId = co_await fromJson<SessionId>(obj["sessionId"]);
    if (obj.contains("modeId") && obj["modeId"].isString())
        result._modeId = co_await fromJson<SessionModeId>(obj["modeId"]);
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const SetSessionModeRequest &data)
{
    QJsonObject obj{
        {"sessionId", data._sessionId},
        {"modeId", data._modeId}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<ClientRequest> fromJson<ClientRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for ClientRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        co_return Utils::ResultError("Missing required field: id");
    if (!obj.contains("method"))
        co_return Utils::ResultError("Missing required field: method");
    ClientRequest result;
    if (obj.contains("id"))
        result._id = co_await fromJson<RequestId>(obj["id"]);
    result._method = obj.value("method").toString();
    if (obj.contains("params"))
        result._params = obj.value("params").toString();
    co_return result;
}

QJsonObject toJson(const ClientRequest &data)
{
    QJsonObject obj{
        {"id", toJsonValue(data._id)},
        {"method", data._method}
    };
    if (data._params.has_value())
        obj.insert("params", *data._params);
    return obj;
}

template<>
Utils::Result<CreateTerminalResponse> fromJson<CreateTerminalResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for CreateTerminalResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("terminalId"))
        co_return Utils::ResultError("Missing required field: terminalId");
    CreateTerminalResponse result;
    if (obj.contains("terminalId") && obj["terminalId"].isString())
        result._terminalId = co_await fromJson<TerminalId>(obj["terminalId"]);
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const CreateTerminalResponse &data)
{
    QJsonObject obj{{"terminalId", data._terminalId}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<KillTerminalResponse> fromJson<KillTerminalResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for KillTerminalResponse");
    const QJsonObject obj = val.toObject();
    KillTerminalResponse result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const KillTerminalResponse &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<ReadTextFileResponse> fromJson<ReadTextFileResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ReadTextFileResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("content"))
        return Utils::ResultError("Missing required field: content");
    ReadTextFileResponse result;
    result._content = obj.value("content").toString();
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const ReadTextFileResponse &data)
{
    QJsonObject obj{{"content", data._content}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<ReleaseTerminalResponse> fromJson<ReleaseTerminalResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ReleaseTerminalResponse");
    const QJsonObject obj = val.toObject();
    ReleaseTerminalResponse result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const ReleaseTerminalResponse &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<SelectedPermissionOutcome> fromJson<SelectedPermissionOutcome>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for SelectedPermissionOutcome");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("optionId"))
        co_return Utils::ResultError("Missing required field: optionId");
    SelectedPermissionOutcome result;
    if (obj.contains("optionId") && obj["optionId"].isString())
        result._optionId = co_await fromJson<PermissionOptionId>(obj["optionId"]);
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const SelectedPermissionOutcome &data)
{
    QJsonObject obj{{"optionId", data._optionId}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<RequestPermissionOutcome> fromJson<RequestPermissionOutcome>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Invalid RequestPermissionOutcome: expected object");
    const QJsonObject obj = val.toObject();
    const QString kind = obj.value("outcome").toString();
    RequestPermissionOutcome result;
    result._kind = kind;
    if (kind == "cancelled")
        result._value = std::monostate{};
    else if (kind == "selected")
        result._value = co_await fromJson<SelectedPermissionOutcome>(val);
    else
        co_return Utils::ResultError("Invalid RequestPermissionOutcome: unknown outcome \"" + kind + "\"");
    co_return result;
}

QJsonObject toJson(const RequestPermissionOutcome &data)
{
    QJsonObject obj = std::visit([](const auto &v) -> QJsonObject {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) return {};
        else return toJson(v);
    }, data._value);
    obj.insert("outcome", data._kind);
    return obj;
}

QJsonValue toJsonValue(const RequestPermissionOutcome &val)
{
    return toJson(val);
}

template<>
Utils::Result<RequestPermissionResponse> fromJson<RequestPermissionResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for RequestPermissionResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("outcome"))
        co_return Utils::ResultError("Missing required field: outcome");
    RequestPermissionResponse result;
    if (obj.contains("outcome"))
        result._outcome = co_await fromJson<RequestPermissionOutcome>(obj["outcome"]);
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const RequestPermissionResponse &data)
{
    QJsonObject obj{{"outcome", toJsonValue(data._outcome)}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<TerminalExitStatus> fromJson<TerminalExitStatus>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TerminalExitStatus");
    const QJsonObject obj = val.toObject();
    TerminalExitStatus result;
    if (obj.contains("exitCode"))
        if (!obj["exitCode"].isNull()) {
            result._exitCode = obj.value("exitCode").toInt();
        }
    if (obj.contains("signal"))
        if (!obj["signal"].isNull()) {
            result._signal = obj.value("signal").toString();
        }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const TerminalExitStatus &data)
{
    QJsonObject obj;
    if (data._exitCode.has_value())
        obj.insert("exitCode", *data._exitCode);
    if (data._signal.has_value())
        obj.insert("signal", *data._signal);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<TerminalOutputResponse> fromJson<TerminalOutputResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for TerminalOutputResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("output"))
        co_return Utils::ResultError("Missing required field: output");
    if (!obj.contains("truncated"))
        co_return Utils::ResultError("Missing required field: truncated");
    TerminalOutputResponse result;
    result._output = obj.value("output").toString();
    result._truncated = obj.value("truncated").toBool();
    if (obj.contains("exitStatus") && !obj["exitStatus"].isNull())
        result._exitStatus = co_await fromJson<TerminalExitStatus>(obj["exitStatus"]);
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const TerminalOutputResponse &data)
{
    QJsonObject obj{
        {"output", data._output},
        {"truncated", data._truncated}
    };
    if (data._exitStatus.has_value())
        obj.insert("exitStatus", toJson(*data._exitStatus));
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<WaitForTerminalExitResponse> fromJson<WaitForTerminalExitResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for WaitForTerminalExitResponse");
    const QJsonObject obj = val.toObject();
    WaitForTerminalExitResponse result;
    if (obj.contains("exitCode"))
        if (!obj["exitCode"].isNull()) {
            result._exitCode = obj.value("exitCode").toInt();
        }
    if (obj.contains("signal"))
        if (!obj["signal"].isNull()) {
            result._signal = obj.value("signal").toString();
        }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const WaitForTerminalExitResponse &data)
{
    QJsonObject obj;
    if (data._exitCode.has_value())
        obj.insert("exitCode", *data._exitCode);
    if (data._signal.has_value())
        obj.insert("signal", *data._signal);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<WriteTextFileResponse> fromJson<WriteTextFileResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for WriteTextFileResponse");
    const QJsonObject obj = val.toObject();
    WriteTextFileResponse result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const WriteTextFileResponse &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<CancelNotification> fromJson<CancelNotification>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for CancelNotification");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        co_return Utils::ResultError("Missing required field: sessionId");
    CancelNotification result;
    if (obj.contains("sessionId") && obj["sessionId"].isString())
        result._sessionId = co_await fromJson<SessionId>(obj["sessionId"]);
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    co_return result;
}

QJsonObject toJson(const CancelNotification &data)
{
    QJsonObject obj{{"sessionId", data._sessionId}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<ClientNotification> fromJson<ClientNotification>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ClientNotification");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    ClientNotification result;
    result._method = obj.value("method").toString();
    if (obj.contains("params"))
        result._params = obj.value("params").toString();
    return result;
}

QJsonObject toJson(const ClientNotification &data)
{
    QJsonObject obj{{"method", data._method}};
    if (data._params.has_value())
        obj.insert("params", *data._params);
    return obj;
}

} // namespace Acp
