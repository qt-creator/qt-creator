// This file is auto-generated. Do not edit manually.
#include "acp.h"

namespace Acp {

template<>
Utils::Result<McpCapabilities> fromJson<McpCapabilities>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for McpCapabilities");
    const QJsonObject obj = val.toObject();
    McpCapabilities result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    if (obj.contains("http"))
        result._http = obj.value("http").toBool();
    if (obj.contains("sse"))
        result._sse = obj.value("sse").toBool();
    return result;
}

QJsonObject toJson(const McpCapabilities &data) {
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    if (data._http.has_value())
        obj.insert("http", *data._http);
    if (data._sse.has_value())
        obj.insert("sse", *data._sse);
    return obj;
}

template<>
Utils::Result<PromptCapabilities> fromJson<PromptCapabilities>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for PromptCapabilities");
    const QJsonObject obj = val.toObject();
    PromptCapabilities result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    if (obj.contains("audio"))
        result._audio = obj.value("audio").toBool();
    if (obj.contains("embeddedContext"))
        result._embeddedContext = obj.value("embeddedContext").toBool();
    if (obj.contains("image"))
        result._image = obj.value("image").toBool();
    return result;
}

QJsonObject toJson(const PromptCapabilities &data) {
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    if (data._audio.has_value())
        obj.insert("audio", *data._audio);
    if (data._embeddedContext.has_value())
        obj.insert("embeddedContext", *data._embeddedContext);
    if (data._image.has_value())
        obj.insert("image", *data._image);
    return obj;
}

template<>
Utils::Result<SessionCapabilities> fromJson<SessionCapabilities>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SessionCapabilities");
    const QJsonObject obj = val.toObject();
    SessionCapabilities result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const SessionCapabilities &data) {
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<AgentCapabilities> fromJson<AgentCapabilities>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for AgentCapabilities");
    const QJsonObject obj = val.toObject();
    AgentCapabilities result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    if (obj.contains("loadSession"))
        result._loadSession = obj.value("loadSession").toBool();
    if (obj.contains("mcpCapabilities"))
        result._mcpCapabilities = obj.value("mcpCapabilities").toString();
    if (obj.contains("promptCapabilities"))
        result._promptCapabilities = obj.value("promptCapabilities").toString();
    if (obj.contains("sessionCapabilities"))
        result._sessionCapabilities = obj.value("sessionCapabilities").toString();
    return result;
}

QJsonObject toJson(const AgentCapabilities &data) {
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    if (data._loadSession.has_value())
        obj.insert("loadSession", *data._loadSession);
    if (data._mcpCapabilities.has_value())
        obj.insert("mcpCapabilities", *data._mcpCapabilities);
    if (data._promptCapabilities.has_value())
        obj.insert("promptCapabilities", *data._promptCapabilities);
    if (data._sessionCapabilities.has_value())
        obj.insert("sessionCapabilities", *data._sessionCapabilities);
    return obj;
}

template<> Utils::Result<SessionId> fromJson<SessionId>(const QJsonValue &val) {
    if (!val.isString()) return Utils::ResultError("Expected string");
    return val.toString();
}

template<>
Utils::Result<UnstructuredCommandInput> fromJson<UnstructuredCommandInput>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for UnstructuredCommandInput");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("hint"))
        return Utils::ResultError("Missing required field: hint");
    UnstructuredCommandInput result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    result._hint = obj.value("hint").toString();
    return result;
}

QJsonObject toJson(const UnstructuredCommandInput &data) {
    QJsonObject obj{{"hint", data._hint}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<AvailableCommandInput> fromJson<AvailableCommandInput>(const QJsonValue &val) {
    if (!val.isObject())
        co_return Utils::ResultError("Invalid AvailableCommandInput: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("hint"))
        co_return AvailableCommandInput(co_await fromJson<UnstructuredCommandInput>(val));
    co_return Utils::ResultError("Invalid AvailableCommandInput");
}

QString hint(const AvailableCommandInput &val) {
    return std::visit([](const auto &v) -> QString { return v._hint; }, val);
}

QJsonObject toJson(const AvailableCommandInput &val) {
    return std::visit([](const auto &v) -> QJsonObject {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
}

QJsonValue toJsonValue(const AvailableCommandInput &val) {
    return toJson(val);
}

template<>
Utils::Result<AvailableCommand> fromJson<AvailableCommand>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for AvailableCommand");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    if (!obj.contains("description"))
        return Utils::ResultError("Missing required field: description");
    AvailableCommand result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    result._description = obj.value("description").toString();
    if (obj.contains("input"))
        result._input = obj.value("input").toString();
    result._name = obj.value("name").toString();
    return result;
}

QJsonObject toJson(const AvailableCommand &data) {
    QJsonObject obj{
        {"description", data._description},
        {"name", data._name}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    if (data._input.has_value())
        obj.insert("input", *data._input);
    return obj;
}

template<>
Utils::Result<AvailableCommandsUpdate> fromJson<AvailableCommandsUpdate>(const QJsonValue &val) {
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for AvailableCommandsUpdate");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("availableCommands"))
        co_return Utils::ResultError("Missing required field: availableCommands");
    AvailableCommandsUpdate result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    if (obj.contains("availableCommands") && obj["availableCommands"].isArray()) {
        QJsonArray arr = obj["availableCommands"].toArray();
        for (const QJsonValue &v : arr) {
            result._availableCommands.append(co_await fromJson<AvailableCommand>(v));
        }
    }
    co_return result;
}

QJsonObject toJson(const AvailableCommandsUpdate &data) {
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    QJsonArray arr_availableCommands;
    for (const auto &v : data._availableCommands) arr_availableCommands.append(toJson(v));
    obj.insert("availableCommands", arr_availableCommands);
    return obj;
}

QString toString(SessionConfigOptionCategory v) {
    switch(v) {
        case SessionConfigOptionCategory::mode: return "mode";
        case SessionConfigOptionCategory::model: return "model";
        case SessionConfigOptionCategory::thought_level: return "thought_level";
    }
    return {};
}

template<>
Utils::Result<SessionConfigOptionCategory> fromJson<SessionConfigOptionCategory>(const QJsonValue &val) {
    const QString str = val.toString();
    if (str == "mode") return SessionConfigOptionCategory::mode;
    if (str == "model") return SessionConfigOptionCategory::model;
    if (str == "thought_level") return SessionConfigOptionCategory::thought_level;
    return Utils::ResultError("Invalid SessionConfigOptionCategory value: " + str);
}

QJsonValue toJsonValue(const SessionConfigOptionCategory &v) {
    return toString(v);
}

template<>
Utils::Result<SessionConfigSelectOption> fromJson<SessionConfigSelectOption>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SessionConfigSelectOption");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("value"))
        return Utils::ResultError("Missing required field: value");
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    SessionConfigSelectOption result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    if (obj.contains("description"))
        if (!obj["description"].isNull()) {
            result._description = obj.value("description").toString();
        }
    result._name = obj.value("name").toString();
    result._value = obj.value("value").toString();
    return result;
}

QJsonObject toJson(const SessionConfigSelectOption &data) {
    QJsonObject obj{
        {"name", data._name},
        {"value", data._value}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    if (data._description.has_value())
        obj.insert("description", *data._description);
    return obj;
}

template<>
Utils::Result<SessionConfigSelectGroup> fromJson<SessionConfigSelectGroup>(const QJsonValue &val) {
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
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    result._group = obj.value("group").toString();
    result._name = obj.value("name").toString();
    if (obj.contains("options") && obj["options"].isArray()) {
        QJsonArray arr = obj["options"].toArray();
        for (const QJsonValue &v : arr) {
            result._options.append(co_await fromJson<SessionConfigSelectOption>(v));
        }
    }
    co_return result;
}

QJsonObject toJson(const SessionConfigSelectGroup &data) {
    QJsonObject obj{
        {"group", data._group},
        {"name", data._name}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    QJsonArray arr_options;
    for (const auto &v : data._options) arr_options.append(toJson(v));
    obj.insert("options", arr_options);
    return obj;
}

template<>
Utils::Result<SessionConfigSelectOptions> fromJson<SessionConfigSelectOptions>(const QJsonValue &val) {
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

QJsonValue toJsonValue(const SessionConfigSelectOptions &val) {
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
Utils::Result<SessionConfigSelect> fromJson<SessionConfigSelect>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SessionConfigSelect");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("currentValue"))
        return Utils::ResultError("Missing required field: currentValue");
    if (!obj.contains("options"))
        return Utils::ResultError("Missing required field: options");
    SessionConfigSelect result;
    result._currentValue = obj.value("currentValue").toString();
    result._options = obj.value("options").toString();
    return result;
}

QJsonObject toJson(const SessionConfigSelect &data) {
    QJsonObject obj{
        {"currentValue", data._currentValue},
        {"options", data._options}
    };
    return obj;
}

template<>
Utils::Result<SessionConfigOption> fromJson<SessionConfigOption>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SessionConfigOption");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    SessionConfigOption result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    if (obj.contains("category"))
        result._category = obj.value("category").toString();
    if (obj.contains("description"))
        if (!obj["description"].isNull()) {
            result._description = obj.value("description").toString();
        }
    result._id = obj.value("id").toString();
    result._name = obj.value("name").toString();
    return result;
}

QJsonObject toJson(const SessionConfigOption &data) {
    QJsonObject obj{
        {"id", data._id},
        {"name", data._name}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    if (data._category.has_value())
        obj.insert("category", *data._category);
    if (data._description.has_value())
        obj.insert("description", *data._description);
    return obj;
}

template<>
Utils::Result<ConfigOptionUpdate> fromJson<ConfigOptionUpdate>(const QJsonValue &val) {
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for ConfigOptionUpdate");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("configOptions"))
        co_return Utils::ResultError("Missing required field: configOptions");
    ConfigOptionUpdate result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    if (obj.contains("configOptions") && obj["configOptions"].isArray()) {
        QJsonArray arr = obj["configOptions"].toArray();
        for (const QJsonValue &v : arr) {
            result._configOptions.append(co_await fromJson<SessionConfigOption>(v));
        }
    }
    co_return result;
}

QJsonObject toJson(const ConfigOptionUpdate &data) {
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    QJsonArray arr_configOptions;
    for (const auto &v : data._configOptions) arr_configOptions.append(toJson(v));
    obj.insert("configOptions", arr_configOptions);
    return obj;
}

QString toString(Role v) {
    switch(v) {
        case Role::assistant: return "assistant";
        case Role::user: return "user";
    }
    return {};
}

template<>
Utils::Result<Role> fromJson<Role>(const QJsonValue &val) {
    const QString str = val.toString();
    if (str == "assistant") return Role::assistant;
    if (str == "user") return Role::user;
    return Utils::ResultError("Invalid Role value: " + str);
}

QJsonValue toJsonValue(const Role &v) {
    return toString(v);
}

template<>
Utils::Result<Annotations> fromJson<Annotations>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Annotations");
    const QJsonObject obj = val.toObject();
    Annotations result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
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
    return result;
}

QJsonObject toJson(const Annotations &data) {
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    if (data._audience.has_value())
        obj.insert("audience", *data._audience);
    if (data._lastModified.has_value())
        obj.insert("lastModified", *data._lastModified);
    if (data._priority.has_value())
        obj.insert("priority", *data._priority);
    return obj;
}

template<>
Utils::Result<AudioContent> fromJson<AudioContent>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for AudioContent");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("data"))
        return Utils::ResultError("Missing required field: data");
    if (!obj.contains("mimeType"))
        return Utils::ResultError("Missing required field: mimeType");
    AudioContent result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    if (obj.contains("annotations"))
        result._annotations = obj.value("annotations").toString();
    result._data = obj.value("data").toString();
    result._mimeType = obj.value("mimeType").toString();
    return result;
}

QJsonObject toJson(const AudioContent &data) {
    QJsonObject obj{
        {"data", data._data},
        {"mimeType", data._mimeType}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    if (data._annotations.has_value())
        obj.insert("annotations", *data._annotations);
    return obj;
}

template<>
Utils::Result<BlobResourceContents> fromJson<BlobResourceContents>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for BlobResourceContents");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("blob"))
        return Utils::ResultError("Missing required field: blob");
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    BlobResourceContents result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    result._blob = obj.value("blob").toString();
    if (obj.contains("mimeType"))
        if (!obj["mimeType"].isNull()) {
            result._mimeType = obj.value("mimeType").toString();
        }
    result._uri = obj.value("uri").toString();
    return result;
}

QJsonObject toJson(const BlobResourceContents &data) {
    QJsonObject obj{
        {"blob", data._blob},
        {"uri", data._uri}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    if (data._mimeType.has_value())
        obj.insert("mimeType", *data._mimeType);
    return obj;
}

template<>
Utils::Result<TextResourceContents> fromJson<TextResourceContents>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TextResourceContents");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("text"))
        return Utils::ResultError("Missing required field: text");
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    TextResourceContents result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    if (obj.contains("mimeType"))
        if (!obj["mimeType"].isNull()) {
            result._mimeType = obj.value("mimeType").toString();
        }
    result._text = obj.value("text").toString();
    result._uri = obj.value("uri").toString();
    return result;
}

QJsonObject toJson(const TextResourceContents &data) {
    QJsonObject obj{
        {"text", data._text},
        {"uri", data._uri}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    if (data._mimeType.has_value())
        obj.insert("mimeType", *data._mimeType);
    return obj;
}

template<>
Utils::Result<EmbeddedResourceResource> fromJson<EmbeddedResourceResource>(const QJsonValue &val) {
    if (!val.isObject())
        co_return Utils::ResultError("Invalid EmbeddedResourceResource: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("text"))
        co_return EmbeddedResourceResource(co_await fromJson<TextResourceContents>(val));
    if (obj.contains("blob"))
        co_return EmbeddedResourceResource(co_await fromJson<BlobResourceContents>(val));
    co_return Utils::ResultError("Invalid EmbeddedResourceResource");
}

QString uri(const EmbeddedResourceResource &val) {
    return std::visit([](const auto &v) -> QString { return v._uri; }, val);
}

QJsonObject toJson(const EmbeddedResourceResource &val) {
    return std::visit([](const auto &v) -> QJsonObject {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
}

QJsonValue toJsonValue(const EmbeddedResourceResource &val) {
    return toJson(val);
}

template<>
Utils::Result<EmbeddedResource> fromJson<EmbeddedResource>(const QJsonValue &val) {
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for EmbeddedResource");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("resource"))
        co_return Utils::ResultError("Missing required field: resource");
    EmbeddedResource result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    if (obj.contains("annotations"))
        result._annotations = obj.value("annotations").toString();
    if (obj.contains("resource"))
        result._resource = co_await fromJson<EmbeddedResourceResource>(obj["resource"]);
    co_return result;
}

QJsonObject toJson(const EmbeddedResource &data) {
    QJsonObject obj{{"resource", toJsonValue(data._resource)}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    if (data._annotations.has_value())
        obj.insert("annotations", *data._annotations);
    return obj;
}

template<>
Utils::Result<ImageContent> fromJson<ImageContent>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ImageContent");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("data"))
        return Utils::ResultError("Missing required field: data");
    if (!obj.contains("mimeType"))
        return Utils::ResultError("Missing required field: mimeType");
    ImageContent result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    if (obj.contains("annotations"))
        result._annotations = obj.value("annotations").toString();
    result._data = obj.value("data").toString();
    result._mimeType = obj.value("mimeType").toString();
    if (obj.contains("uri"))
        if (!obj["uri"].isNull()) {
            result._uri = obj.value("uri").toString();
        }
    return result;
}

QJsonObject toJson(const ImageContent &data) {
    QJsonObject obj{
        {"data", data._data},
        {"mimeType", data._mimeType}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    if (data._annotations.has_value())
        obj.insert("annotations", *data._annotations);
    if (data._uri.has_value())
        obj.insert("uri", *data._uri);
    return obj;
}

template<>
Utils::Result<ResourceLink> fromJson<ResourceLink>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ResourceLink");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    ResourceLink result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    if (obj.contains("annotations"))
        result._annotations = obj.value("annotations").toString();
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
    return result;
}

QJsonObject toJson(const ResourceLink &data) {
    QJsonObject obj{
        {"name", data._name},
        {"uri", data._uri}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    if (data._annotations.has_value())
        obj.insert("annotations", *data._annotations);
    if (data._description.has_value())
        obj.insert("description", *data._description);
    if (data._mimeType.has_value())
        obj.insert("mimeType", *data._mimeType);
    if (data._size.has_value())
        obj.insert("size", *data._size);
    if (data._title.has_value())
        obj.insert("title", *data._title);
    return obj;
}

template<>
Utils::Result<TextContent> fromJson<TextContent>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TextContent");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("text"))
        return Utils::ResultError("Missing required field: text");
    TextContent result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    if (obj.contains("annotations"))
        result._annotations = obj.value("annotations").toString();
    result._text = obj.value("text").toString();
    return result;
}

QJsonObject toJson(const TextContent &data) {
    QJsonObject obj{{"text", data._text}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    if (data._annotations.has_value())
        obj.insert("annotations", *data._annotations);
    return obj;
}

template<>
Utils::Result<ContentBlock> fromJson<ContentBlock>(const QJsonValue &val) {
    if (!val.isObject())
        co_return Utils::ResultError("Invalid ContentBlock: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("text"))
        co_return ContentBlock(co_await fromJson<TextContent>(val));
    if (obj.contains("name"))
        co_return ContentBlock(co_await fromJson<ResourceLink>(val));
    if (obj.contains("resource"))
        co_return ContentBlock(co_await fromJson<EmbeddedResource>(val));
    {
        auto result = fromJson<ImageContent>(val);
        if (result) co_return ContentBlock(*result);
    }
    {
        auto result = fromJson<AudioContent>(val);
        if (result) co_return ContentBlock(*result);
    }
    co_return Utils::ResultError("Invalid ContentBlock");
}

QJsonObject toJson(const ContentBlock &val) {
    return std::visit([](const auto &v) -> QJsonObject {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
}

QJsonValue toJsonValue(const ContentBlock &val) {
    return toJson(val);
}

template<>
Utils::Result<ContentChunk> fromJson<ContentChunk>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ContentChunk");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("content"))
        return Utils::ResultError("Missing required field: content");
    ContentChunk result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    result._content = obj.value("content").toString();
    return result;
}

QJsonObject toJson(const ContentChunk &data) {
    QJsonObject obj{{"content", data._content}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<CurrentModeUpdate> fromJson<CurrentModeUpdate>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CurrentModeUpdate");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("currentModeId"))
        return Utils::ResultError("Missing required field: currentModeId");
    CurrentModeUpdate result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    result._currentModeId = obj.value("currentModeId").toString();
    return result;
}

QJsonObject toJson(const CurrentModeUpdate &data) {
    QJsonObject obj{{"currentModeId", data._currentModeId}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

QString toString(PlanEntryPriority v) {
    switch(v) {
        case PlanEntryPriority::high: return "high";
        case PlanEntryPriority::medium: return "medium";
        case PlanEntryPriority::low: return "low";
    }
    return {};
}

template<>
Utils::Result<PlanEntryPriority> fromJson<PlanEntryPriority>(const QJsonValue &val) {
    const QString str = val.toString();
    if (str == "high") return PlanEntryPriority::high;
    if (str == "medium") return PlanEntryPriority::medium;
    if (str == "low") return PlanEntryPriority::low;
    return Utils::ResultError("Invalid PlanEntryPriority value: " + str);
}

QJsonValue toJsonValue(const PlanEntryPriority &v) {
    return toString(v);
}

QString toString(PlanEntryStatus v) {
    switch(v) {
        case PlanEntryStatus::pending: return "pending";
        case PlanEntryStatus::in_progress: return "in_progress";
        case PlanEntryStatus::completed: return "completed";
    }
    return {};
}

template<>
Utils::Result<PlanEntryStatus> fromJson<PlanEntryStatus>(const QJsonValue &val) {
    const QString str = val.toString();
    if (str == "pending") return PlanEntryStatus::pending;
    if (str == "in_progress") return PlanEntryStatus::in_progress;
    if (str == "completed") return PlanEntryStatus::completed;
    return Utils::ResultError("Invalid PlanEntryStatus value: " + str);
}

QJsonValue toJsonValue(const PlanEntryStatus &v) {
    return toString(v);
}

template<>
Utils::Result<PlanEntry> fromJson<PlanEntry>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for PlanEntry");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("content"))
        return Utils::ResultError("Missing required field: content");
    if (!obj.contains("priority"))
        return Utils::ResultError("Missing required field: priority");
    if (!obj.contains("status"))
        return Utils::ResultError("Missing required field: status");
    PlanEntry result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    result._content = obj.value("content").toString();
    result._priority = obj.value("priority").toString();
    result._status = obj.value("status").toString();
    return result;
}

QJsonObject toJson(const PlanEntry &data) {
    QJsonObject obj{
        {"content", data._content},
        {"priority", data._priority},
        {"status", data._status}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<Plan> fromJson<Plan>(const QJsonValue &val) {
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for Plan");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("entries"))
        co_return Utils::ResultError("Missing required field: entries");
    Plan result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    if (obj.contains("entries") && obj["entries"].isArray()) {
        QJsonArray arr = obj["entries"].toArray();
        for (const QJsonValue &v : arr) {
            result._entries.append(co_await fromJson<PlanEntry>(v));
        }
    }
    co_return result;
}

QJsonObject toJson(const Plan &data) {
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    QJsonArray arr_entries;
    for (const auto &v : data._entries) arr_entries.append(toJson(v));
    obj.insert("entries", arr_entries);
    return obj;
}

template<>
Utils::Result<Content> fromJson<Content>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Content");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("content"))
        return Utils::ResultError("Missing required field: content");
    Content result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    result._content = obj.value("content").toString();
    return result;
}

QJsonObject toJson(const Content &data) {
    QJsonObject obj{{"content", data._content}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<Diff> fromJson<Diff>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Diff");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("path"))
        return Utils::ResultError("Missing required field: path");
    if (!obj.contains("newText"))
        return Utils::ResultError("Missing required field: newText");
    Diff result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    result._newText = obj.value("newText").toString();
    if (obj.contains("oldText"))
        if (!obj["oldText"].isNull()) {
            result._oldText = obj.value("oldText").toString();
        }
    result._path = obj.value("path").toString();
    return result;
}

QJsonObject toJson(const Diff &data) {
    QJsonObject obj{
        {"newText", data._newText},
        {"path", data._path}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    if (data._oldText.has_value())
        obj.insert("oldText", *data._oldText);
    return obj;
}

template<>
Utils::Result<Terminal> fromJson<Terminal>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Terminal");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("terminalId"))
        return Utils::ResultError("Missing required field: terminalId");
    Terminal result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    result._terminalId = obj.value("terminalId").toString();
    return result;
}

QJsonObject toJson(const Terminal &data) {
    QJsonObject obj{{"terminalId", data._terminalId}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<ToolCallContent> fromJson<ToolCallContent>(const QJsonValue &val) {
    if (!val.isObject())
        co_return Utils::ResultError("Invalid ToolCallContent: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("content"))
        co_return ToolCallContent(co_await fromJson<Content>(val));
    if (obj.contains("newText"))
        co_return ToolCallContent(co_await fromJson<Diff>(val));
    if (obj.contains("terminalId"))
        co_return ToolCallContent(co_await fromJson<Terminal>(val));
    co_return Utils::ResultError("Invalid ToolCallContent");
}

QJsonObject toJson(const ToolCallContent &val) {
    return std::visit([](const auto &v) -> QJsonObject {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
}

QJsonValue toJsonValue(const ToolCallContent &val) {
    return toJson(val);
}

template<>
Utils::Result<ToolCallLocation> fromJson<ToolCallLocation>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ToolCallLocation");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("path"))
        return Utils::ResultError("Missing required field: path");
    ToolCallLocation result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    if (obj.contains("line"))
        if (!obj["line"].isNull()) {
            result._line = obj.value("line").toInt();
        }
    result._path = obj.value("path").toString();
    return result;
}

QJsonObject toJson(const ToolCallLocation &data) {
    QJsonObject obj{{"path", data._path}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    if (data._line.has_value())
        obj.insert("line", *data._line);
    return obj;
}

QString toString(ToolCallStatus v) {
    switch(v) {
        case ToolCallStatus::pending: return "pending";
        case ToolCallStatus::in_progress: return "in_progress";
        case ToolCallStatus::completed: return "completed";
        case ToolCallStatus::failed: return "failed";
    }
    return {};
}

template<>
Utils::Result<ToolCallStatus> fromJson<ToolCallStatus>(const QJsonValue &val) {
    const QString str = val.toString();
    if (str == "pending") return ToolCallStatus::pending;
    if (str == "in_progress") return ToolCallStatus::in_progress;
    if (str == "completed") return ToolCallStatus::completed;
    if (str == "failed") return ToolCallStatus::failed;
    return Utils::ResultError("Invalid ToolCallStatus value: " + str);
}

QJsonValue toJsonValue(const ToolCallStatus &v) {
    return toString(v);
}

QString toString(ToolKind v) {
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
Utils::Result<ToolKind> fromJson<ToolKind>(const QJsonValue &val) {
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

QJsonValue toJsonValue(const ToolKind &v) {
    return toString(v);
}

template<>
Utils::Result<ToolCall> fromJson<ToolCall>(const QJsonValue &val) {
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for ToolCall");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("toolCallId"))
        co_return Utils::ResultError("Missing required field: toolCallId");
    if (!obj.contains("title"))
        co_return Utils::ResultError("Missing required field: title");
    ToolCall result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    if (obj.contains("content") && obj["content"].isArray()) {
        QJsonArray arr = obj["content"].toArray();
        QList<ToolCallContent> list_content;
        for (const QJsonValue &v : arr) {
            list_content.append(co_await fromJson<ToolCallContent>(v));
        }
        result._content = list_content;
    }
    if (obj.contains("kind"))
        result._kind = obj.value("kind").toString();
    if (obj.contains("locations") && obj["locations"].isArray()) {
        QJsonArray arr = obj["locations"].toArray();
        QList<ToolCallLocation> list_locations;
        for (const QJsonValue &v : arr) {
            list_locations.append(co_await fromJson<ToolCallLocation>(v));
        }
        result._locations = list_locations;
    }
    if (obj.contains("rawInput"))
        result._rawInput = obj.value("rawInput").toString();
    if (obj.contains("rawOutput"))
        result._rawOutput = obj.value("rawOutput").toString();
    if (obj.contains("status"))
        result._status = obj.value("status").toString();
    result._title = obj.value("title").toString();
    result._toolCallId = obj.value("toolCallId").toString();
    co_return result;
}

QJsonObject toJson(const ToolCall &data) {
    QJsonObject obj{
        {"title", data._title},
        {"toolCallId", data._toolCallId}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    if (data._content.has_value()) {
        QJsonArray arr_content;
        for (const auto &v : *data._content) arr_content.append(toJsonValue(v));
        obj.insert("content", arr_content);
    }
    if (data._kind.has_value())
        obj.insert("kind", *data._kind);
    if (data._locations.has_value()) {
        QJsonArray arr_locations;
        for (const auto &v : *data._locations) arr_locations.append(toJson(v));
        obj.insert("locations", arr_locations);
    }
    if (data._rawInput.has_value())
        obj.insert("rawInput", *data._rawInput);
    if (data._rawOutput.has_value())
        obj.insert("rawOutput", *data._rawOutput);
    if (data._status.has_value())
        obj.insert("status", *data._status);
    return obj;
}

template<>
Utils::Result<ToolCallUpdate> fromJson<ToolCallUpdate>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ToolCallUpdate");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("toolCallId"))
        return Utils::ResultError("Missing required field: toolCallId");
    ToolCallUpdate result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    if (obj.contains("content"))
        if (!obj["content"].isNull()) {
            result._content = obj.value("content").toArray();
        }
    if (obj.contains("kind"))
        result._kind = obj.value("kind").toString();
    if (obj.contains("locations"))
        if (!obj["locations"].isNull()) {
            result._locations = obj.value("locations").toArray();
        }
    if (obj.contains("rawInput"))
        result._rawInput = obj.value("rawInput").toString();
    if (obj.contains("rawOutput"))
        result._rawOutput = obj.value("rawOutput").toString();
    if (obj.contains("status"))
        result._status = obj.value("status").toString();
    if (obj.contains("title"))
        if (!obj["title"].isNull()) {
            result._title = obj.value("title").toString();
        }
    result._toolCallId = obj.value("toolCallId").toString();
    return result;
}

QJsonObject toJson(const ToolCallUpdate &data) {
    QJsonObject obj{{"toolCallId", data._toolCallId}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    if (data._content.has_value())
        obj.insert("content", *data._content);
    if (data._kind.has_value())
        obj.insert("kind", *data._kind);
    if (data._locations.has_value())
        obj.insert("locations", *data._locations);
    if (data._rawInput.has_value())
        obj.insert("rawInput", *data._rawInput);
    if (data._rawOutput.has_value())
        obj.insert("rawOutput", *data._rawOutput);
    if (data._status.has_value())
        obj.insert("status", *data._status);
    if (data._title.has_value())
        obj.insert("title", *data._title);
    return obj;
}

template<>
Utils::Result<SessionUpdate> fromJson<SessionUpdate>(const QJsonValue &val) {
    if (!val.isObject())
        co_return Utils::ResultError("Invalid SessionUpdate: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("entries"))
        co_return SessionUpdate(co_await fromJson<Plan>(val));
    if (obj.contains("availableCommands"))
        co_return SessionUpdate(co_await fromJson<AvailableCommandsUpdate>(val));
    if (obj.contains("currentModeId"))
        co_return SessionUpdate(co_await fromJson<CurrentModeUpdate>(val));
    if (obj.contains("configOptions"))
        co_return SessionUpdate(co_await fromJson<ConfigOptionUpdate>(val));
    {
        auto result = fromJson<ContentChunk>(val);
        if (result) co_return SessionUpdate(*result);
    }
    {
        auto result = fromJson<ContentChunk>(val);
        if (result) co_return SessionUpdate(*result);
    }
    {
        auto result = fromJson<ContentChunk>(val);
        if (result) co_return SessionUpdate(*result);
    }
    {
        auto result = fromJson<ToolCall>(val);
        if (result) co_return SessionUpdate(*result);
    }
    {
        auto result = fromJson<ToolCallUpdate>(val);
        if (result) co_return SessionUpdate(*result);
    }
    co_return Utils::ResultError("Invalid SessionUpdate");
}

QJsonObject toJson(const SessionUpdate &val) {
    return std::visit([](const auto &v) -> QJsonObject {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
}

QJsonValue toJsonValue(const SessionUpdate &val) {
    return toJson(val);
}

template<>
Utils::Result<SessionNotification> fromJson<SessionNotification>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SessionNotification");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    if (!obj.contains("update"))
        return Utils::ResultError("Missing required field: update");
    SessionNotification result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    result._sessionId = obj.value("sessionId").toString();
    result._update = obj.value("update").toString();
    return result;
}

QJsonObject toJson(const SessionNotification &data) {
    QJsonObject obj{
        {"sessionId", data._sessionId},
        {"update", data._update}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<AgentNotification> fromJson<AgentNotification>(const QJsonValue &val) {
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

QJsonObject toJson(const AgentNotification &data) {
    QJsonObject obj{{"method", data._method}};
    if (data._params.has_value())
        obj.insert("params", *data._params);
    return obj;
}

template<>
Utils::Result<EnvVariable> fromJson<EnvVariable>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for EnvVariable");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    if (!obj.contains("value"))
        return Utils::ResultError("Missing required field: value");
    EnvVariable result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    result._name = obj.value("name").toString();
    result._value = obj.value("value").toString();
    return result;
}

QJsonObject toJson(const EnvVariable &data) {
    QJsonObject obj{
        {"name", data._name},
        {"value", data._value}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<CreateTerminalRequest> fromJson<CreateTerminalRequest>(const QJsonValue &val) {
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for CreateTerminalRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        co_return Utils::ResultError("Missing required field: sessionId");
    if (!obj.contains("command"))
        co_return Utils::ResultError("Missing required field: command");
    CreateTerminalRequest result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    if (obj.contains("args") && obj["args"].isArray()) {
        QJsonArray arr = obj["args"].toArray();
        QStringList list_args;
        for (const QJsonValue &v : arr) {
            list_args.append(v.toString());
        }
        result._args = list_args;
    }
    result._command = obj.value("command").toString();
    if (obj.contains("cwd"))
        if (!obj["cwd"].isNull()) {
            result._cwd = obj.value("cwd").toString();
        }
    if (obj.contains("env") && obj["env"].isArray()) {
        QJsonArray arr = obj["env"].toArray();
        QList<EnvVariable> list_env;
        for (const QJsonValue &v : arr) {
            list_env.append(co_await fromJson<EnvVariable>(v));
        }
        result._env = list_env;
    }
    if (obj.contains("outputByteLimit"))
        if (!obj["outputByteLimit"].isNull()) {
            result._outputByteLimit = obj.value("outputByteLimit").toInt();
        }
    result._sessionId = obj.value("sessionId").toString();
    co_return result;
}

QJsonObject toJson(const CreateTerminalRequest &data) {
    QJsonObject obj{
        {"command", data._command},
        {"sessionId", data._sessionId}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    if (data._args.has_value()) {
        QJsonArray arr_args;
        for (const auto &v : *data._args) arr_args.append(v);
        obj.insert("args", arr_args);
    }
    if (data._cwd.has_value())
        obj.insert("cwd", *data._cwd);
    if (data._env.has_value()) {
        QJsonArray arr_env;
        for (const auto &v : *data._env) arr_env.append(toJson(v));
        obj.insert("env", arr_env);
    }
    if (data._outputByteLimit.has_value())
        obj.insert("outputByteLimit", *data._outputByteLimit);
    return obj;
}

template<>
Utils::Result<KillTerminalCommandRequest> fromJson<KillTerminalCommandRequest>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for KillTerminalCommandRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    if (!obj.contains("terminalId"))
        return Utils::ResultError("Missing required field: terminalId");
    KillTerminalCommandRequest result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    result._sessionId = obj.value("sessionId").toString();
    result._terminalId = obj.value("terminalId").toString();
    return result;
}

QJsonObject toJson(const KillTerminalCommandRequest &data) {
    QJsonObject obj{
        {"sessionId", data._sessionId},
        {"terminalId", data._terminalId}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<ReadTextFileRequest> fromJson<ReadTextFileRequest>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ReadTextFileRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    if (!obj.contains("path"))
        return Utils::ResultError("Missing required field: path");
    ReadTextFileRequest result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    if (obj.contains("limit"))
        if (!obj["limit"].isNull()) {
            result._limit = obj.value("limit").toInt();
        }
    if (obj.contains("line"))
        if (!obj["line"].isNull()) {
            result._line = obj.value("line").toInt();
        }
    result._path = obj.value("path").toString();
    result._sessionId = obj.value("sessionId").toString();
    return result;
}

QJsonObject toJson(const ReadTextFileRequest &data) {
    QJsonObject obj{
        {"path", data._path},
        {"sessionId", data._sessionId}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    if (data._limit.has_value())
        obj.insert("limit", *data._limit);
    if (data._line.has_value())
        obj.insert("line", *data._line);
    return obj;
}

template<>
Utils::Result<ReleaseTerminalRequest> fromJson<ReleaseTerminalRequest>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ReleaseTerminalRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    if (!obj.contains("terminalId"))
        return Utils::ResultError("Missing required field: terminalId");
    ReleaseTerminalRequest result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    result._sessionId = obj.value("sessionId").toString();
    result._terminalId = obj.value("terminalId").toString();
    return result;
}

QJsonObject toJson(const ReleaseTerminalRequest &data) {
    QJsonObject obj{
        {"sessionId", data._sessionId},
        {"terminalId", data._terminalId}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<RequestId> fromJson<RequestId>(const QJsonValue &val) {
    if (val.isNull())
        return RequestId(std::monostate{});
    if (val.isDouble())
        return RequestId(static_cast<int>(val.toDouble()));
    if (val.isString())
        return RequestId(val.toString());
    return Utils::ResultError("Invalid RequestId");
}

QJsonValue toJsonValue(const RequestId &val) {
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

QString toString(PermissionOptionKind v) {
    switch(v) {
        case PermissionOptionKind::allow_once: return "allow_once";
        case PermissionOptionKind::allow_always: return "allow_always";
        case PermissionOptionKind::reject_once: return "reject_once";
        case PermissionOptionKind::reject_always: return "reject_always";
    }
    return {};
}

template<>
Utils::Result<PermissionOptionKind> fromJson<PermissionOptionKind>(const QJsonValue &val) {
    const QString str = val.toString();
    if (str == "allow_once") return PermissionOptionKind::allow_once;
    if (str == "allow_always") return PermissionOptionKind::allow_always;
    if (str == "reject_once") return PermissionOptionKind::reject_once;
    if (str == "reject_always") return PermissionOptionKind::reject_always;
    return Utils::ResultError("Invalid PermissionOptionKind value: " + str);
}

QJsonValue toJsonValue(const PermissionOptionKind &v) {
    return toString(v);
}

template<>
Utils::Result<PermissionOption> fromJson<PermissionOption>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for PermissionOption");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("optionId"))
        return Utils::ResultError("Missing required field: optionId");
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    if (!obj.contains("kind"))
        return Utils::ResultError("Missing required field: kind");
    PermissionOption result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    result._kind = obj.value("kind").toString();
    result._name = obj.value("name").toString();
    result._optionId = obj.value("optionId").toString();
    return result;
}

QJsonObject toJson(const PermissionOption &data) {
    QJsonObject obj{
        {"kind", data._kind},
        {"name", data._name},
        {"optionId", data._optionId}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<RequestPermissionRequest> fromJson<RequestPermissionRequest>(const QJsonValue &val) {
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
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    if (obj.contains("options") && obj["options"].isArray()) {
        QJsonArray arr = obj["options"].toArray();
        for (const QJsonValue &v : arr) {
            result._options.append(co_await fromJson<PermissionOption>(v));
        }
    }
    result._sessionId = obj.value("sessionId").toString();
    result._toolCall = obj.value("toolCall").toString();
    co_return result;
}

QJsonObject toJson(const RequestPermissionRequest &data) {
    QJsonObject obj{
        {"sessionId", data._sessionId},
        {"toolCall", data._toolCall}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    QJsonArray arr_options;
    for (const auto &v : data._options) arr_options.append(toJson(v));
    obj.insert("options", arr_options);
    return obj;
}

template<>
Utils::Result<TerminalOutputRequest> fromJson<TerminalOutputRequest>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TerminalOutputRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    if (!obj.contains("terminalId"))
        return Utils::ResultError("Missing required field: terminalId");
    TerminalOutputRequest result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    result._sessionId = obj.value("sessionId").toString();
    result._terminalId = obj.value("terminalId").toString();
    return result;
}

QJsonObject toJson(const TerminalOutputRequest &data) {
    QJsonObject obj{
        {"sessionId", data._sessionId},
        {"terminalId", data._terminalId}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<WaitForTerminalExitRequest> fromJson<WaitForTerminalExitRequest>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for WaitForTerminalExitRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    if (!obj.contains("terminalId"))
        return Utils::ResultError("Missing required field: terminalId");
    WaitForTerminalExitRequest result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    result._sessionId = obj.value("sessionId").toString();
    result._terminalId = obj.value("terminalId").toString();
    return result;
}

QJsonObject toJson(const WaitForTerminalExitRequest &data) {
    QJsonObject obj{
        {"sessionId", data._sessionId},
        {"terminalId", data._terminalId}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<WriteTextFileRequest> fromJson<WriteTextFileRequest>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for WriteTextFileRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    if (!obj.contains("path"))
        return Utils::ResultError("Missing required field: path");
    if (!obj.contains("content"))
        return Utils::ResultError("Missing required field: content");
    WriteTextFileRequest result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    result._content = obj.value("content").toString();
    result._path = obj.value("path").toString();
    result._sessionId = obj.value("sessionId").toString();
    return result;
}

QJsonObject toJson(const WriteTextFileRequest &data) {
    QJsonObject obj{
        {"content", data._content},
        {"path", data._path},
        {"sessionId", data._sessionId}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<AgentRequest> fromJson<AgentRequest>(const QJsonValue &val) {
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

QJsonObject toJson(const AgentRequest &data) {
    QJsonObject obj{
        {"id", toJsonValue(data._id)},
        {"method", data._method}
    };
    if (data._params.has_value())
        obj.insert("params", *data._params);
    return obj;
}

template<>
Utils::Result<AuthenticateResponse> fromJson<AuthenticateResponse>(const QJsonValue &val) {
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

QJsonObject toJson(const AuthenticateResponse &data) {
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<Error> fromJson<Error>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Error");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("code"))
        return Utils::ResultError("Missing required field: code");
    if (!obj.contains("message"))
        return Utils::ResultError("Missing required field: message");
    Error result;
    result._code = obj.value("code").toString();
    if (obj.contains("data"))
        result._data = obj.value("data").toString();
    result._message = obj.value("message").toString();
    return result;
}

QJsonObject toJson(const Error &data) {
    QJsonObject obj{
        {"code", data._code},
        {"message", data._message}
    };
    if (data._data.has_value())
        obj.insert("data", *data._data);
    return obj;
}

template<>
Utils::Result<AuthMethod> fromJson<AuthMethod>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for AuthMethod");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    AuthMethod result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    if (obj.contains("description"))
        if (!obj["description"].isNull()) {
            result._description = obj.value("description").toString();
        }
    result._id = obj.value("id").toString();
    result._name = obj.value("name").toString();
    return result;
}

QJsonObject toJson(const AuthMethod &data) {
    QJsonObject obj{
        {"id", data._id},
        {"name", data._name}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    if (data._description.has_value())
        obj.insert("description", *data._description);
    return obj;
}

template<>
Utils::Result<Implementation> fromJson<Implementation>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Implementation");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    if (!obj.contains("version"))
        return Utils::ResultError("Missing required field: version");
    Implementation result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    result._name = obj.value("name").toString();
    if (obj.contains("title"))
        if (!obj["title"].isNull()) {
            result._title = obj.value("title").toString();
        }
    result._version = obj.value("version").toString();
    return result;
}

QJsonObject toJson(const Implementation &data) {
    QJsonObject obj{
        {"name", data._name},
        {"version", data._version}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    if (data._title.has_value())
        obj.insert("title", *data._title);
    return obj;
}

template<> Utils::Result<ProtocolVersion> fromJson<ProtocolVersion>(const QJsonValue &val) {
    if (!val.isDouble()) return Utils::ResultError("Expected number");
    return static_cast<int>(val.toDouble());
}

template<>
Utils::Result<InitializeResponse> fromJson<InitializeResponse>(const QJsonValue &val) {
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for InitializeResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("protocolVersion"))
        co_return Utils::ResultError("Missing required field: protocolVersion");
    InitializeResponse result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    if (obj.contains("agentCapabilities"))
        result._agentCapabilities = obj.value("agentCapabilities").toString();
    if (obj.contains("agentInfo"))
        result._agentInfo = obj.value("agentInfo").toString();
    if (obj.contains("authMethods") && obj["authMethods"].isArray()) {
        QJsonArray arr = obj["authMethods"].toArray();
        QList<AuthMethod> list_authMethods;
        for (const QJsonValue &v : arr) {
            list_authMethods.append(co_await fromJson<AuthMethod>(v));
        }
        result._authMethods = list_authMethods;
    }
    result._protocolVersion = obj.value("protocolVersion").toString();
    co_return result;
}

QJsonObject toJson(const InitializeResponse &data) {
    QJsonObject obj{{"protocolVersion", data._protocolVersion}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    if (data._agentCapabilities.has_value())
        obj.insert("agentCapabilities", *data._agentCapabilities);
    if (data._agentInfo.has_value())
        obj.insert("agentInfo", *data._agentInfo);
    if (data._authMethods.has_value()) {
        QJsonArray arr_authMethods;
        for (const auto &v : *data._authMethods) arr_authMethods.append(toJson(v));
        obj.insert("authMethods", arr_authMethods);
    }
    return obj;
}

template<>
Utils::Result<SessionMode> fromJson<SessionMode>(const QJsonValue &val) {
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for SessionMode");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        co_return Utils::ResultError("Missing required field: id");
    if (!obj.contains("name"))
        co_return Utils::ResultError("Missing required field: name");
    SessionMode result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    if (obj.contains("description"))
        if (!obj["description"].isNull()) {
            result._description = obj.value("description").toString();
        }
    if (obj.contains("id") && obj["id"].isString())
        result._id = co_await fromJson<SessionModeId>(obj["id"]);
    result._name = obj.value("name").toString();
    co_return result;
}

QJsonObject toJson(const SessionMode &data) {
    QJsonObject obj{
        {"id", data._id},
        {"name", data._name}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    if (data._description.has_value())
        obj.insert("description", *data._description);
    return obj;
}

template<>
Utils::Result<SessionModeState> fromJson<SessionModeState>(const QJsonValue &val) {
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for SessionModeState");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("currentModeId"))
        co_return Utils::ResultError("Missing required field: currentModeId");
    if (!obj.contains("availableModes"))
        co_return Utils::ResultError("Missing required field: availableModes");
    SessionModeState result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    if (obj.contains("availableModes") && obj["availableModes"].isArray()) {
        QJsonArray arr = obj["availableModes"].toArray();
        for (const QJsonValue &v : arr) {
            result._availableModes.append(co_await fromJson<SessionMode>(v));
        }
    }
    result._currentModeId = obj.value("currentModeId").toString();
    co_return result;
}

QJsonObject toJson(const SessionModeState &data) {
    QJsonObject obj{{"currentModeId", data._currentModeId}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    QJsonArray arr_availableModes;
    for (const auto &v : data._availableModes) arr_availableModes.append(toJson(v));
    obj.insert("availableModes", arr_availableModes);
    return obj;
}

template<>
Utils::Result<LoadSessionResponse> fromJson<LoadSessionResponse>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for LoadSessionResponse");
    const QJsonObject obj = val.toObject();
    LoadSessionResponse result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    if (obj.contains("configOptions"))
        if (!obj["configOptions"].isNull()) {
            result._configOptions = obj.value("configOptions").toArray();
        }
    if (obj.contains("modes"))
        result._modes = obj.value("modes").toString();
    return result;
}

QJsonObject toJson(const LoadSessionResponse &data) {
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    if (data._configOptions.has_value())
        obj.insert("configOptions", *data._configOptions);
    if (data._modes.has_value())
        obj.insert("modes", *data._modes);
    return obj;
}

template<>
Utils::Result<NewSessionResponse> fromJson<NewSessionResponse>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for NewSessionResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    NewSessionResponse result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    if (obj.contains("configOptions"))
        if (!obj["configOptions"].isNull()) {
            result._configOptions = obj.value("configOptions").toArray();
        }
    if (obj.contains("modes"))
        result._modes = obj.value("modes").toString();
    result._sessionId = obj.value("sessionId").toString();
    return result;
}

QJsonObject toJson(const NewSessionResponse &data) {
    QJsonObject obj{{"sessionId", data._sessionId}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    if (data._configOptions.has_value())
        obj.insert("configOptions", *data._configOptions);
    if (data._modes.has_value())
        obj.insert("modes", *data._modes);
    return obj;
}

QString toString(StopReason v) {
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
Utils::Result<StopReason> fromJson<StopReason>(const QJsonValue &val) {
    const QString str = val.toString();
    if (str == "end_turn") return StopReason::end_turn;
    if (str == "max_tokens") return StopReason::max_tokens;
    if (str == "max_turn_requests") return StopReason::max_turn_requests;
    if (str == "refusal") return StopReason::refusal;
    if (str == "cancelled") return StopReason::cancelled;
    return Utils::ResultError("Invalid StopReason value: " + str);
}

QJsonValue toJsonValue(const StopReason &v) {
    return toString(v);
}

template<>
Utils::Result<PromptResponse> fromJson<PromptResponse>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for PromptResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("stopReason"))
        return Utils::ResultError("Missing required field: stopReason");
    PromptResponse result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    result._stopReason = obj.value("stopReason").toString();
    return result;
}

QJsonObject toJson(const PromptResponse &data) {
    QJsonObject obj{{"stopReason", data._stopReason}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<SetSessionConfigOptionResponse> fromJson<SetSessionConfigOptionResponse>(const QJsonValue &val) {
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for SetSessionConfigOptionResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("configOptions"))
        co_return Utils::ResultError("Missing required field: configOptions");
    SetSessionConfigOptionResponse result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    if (obj.contains("configOptions") && obj["configOptions"].isArray()) {
        QJsonArray arr = obj["configOptions"].toArray();
        for (const QJsonValue &v : arr) {
            result._configOptions.append(co_await fromJson<SessionConfigOption>(v));
        }
    }
    co_return result;
}

QJsonObject toJson(const SetSessionConfigOptionResponse &data) {
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    QJsonArray arr_configOptions;
    for (const auto &v : data._configOptions) arr_configOptions.append(toJson(v));
    obj.insert("configOptions", arr_configOptions);
    return obj;
}

template<>
Utils::Result<SetSessionModeResponse> fromJson<SetSessionModeResponse>(const QJsonValue &val) {
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

QJsonObject toJson(const SetSessionModeResponse &data) {
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<AgentResponse> fromJson<AgentResponse>(const QJsonValue &val) {
    return Utils::ResultError("Invalid AgentResponse");
}

QJsonValue toJsonValue(const AgentResponse &val) {
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<AuthenticateRequest> fromJson<AuthenticateRequest>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for AuthenticateRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("methodId"))
        return Utils::ResultError("Missing required field: methodId");
    AuthenticateRequest result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    result._methodId = obj.value("methodId").toString();
    return result;
}

QJsonObject toJson(const AuthenticateRequest &data) {
    QJsonObject obj{{"methodId", data._methodId}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<CancelNotification> fromJson<CancelNotification>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CancelNotification");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    CancelNotification result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    result._sessionId = obj.value("sessionId").toString();
    return result;
}

QJsonObject toJson(const CancelNotification &data) {
    QJsonObject obj{{"sessionId", data._sessionId}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<FileSystemCapability> fromJson<FileSystemCapability>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for FileSystemCapability");
    const QJsonObject obj = val.toObject();
    FileSystemCapability result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    if (obj.contains("readTextFile"))
        result._readTextFile = obj.value("readTextFile").toBool();
    if (obj.contains("writeTextFile"))
        result._writeTextFile = obj.value("writeTextFile").toBool();
    return result;
}

QJsonObject toJson(const FileSystemCapability &data) {
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    if (data._readTextFile.has_value())
        obj.insert("readTextFile", *data._readTextFile);
    if (data._writeTextFile.has_value())
        obj.insert("writeTextFile", *data._writeTextFile);
    return obj;
}

template<>
Utils::Result<ClientCapabilities> fromJson<ClientCapabilities>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ClientCapabilities");
    const QJsonObject obj = val.toObject();
    ClientCapabilities result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    if (obj.contains("fs"))
        result._fs = obj.value("fs").toString();
    if (obj.contains("terminal"))
        result._terminal = obj.value("terminal").toBool();
    return result;
}

QJsonObject toJson(const ClientCapabilities &data) {
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    if (data._fs.has_value())
        obj.insert("fs", *data._fs);
    if (data._terminal.has_value())
        obj.insert("terminal", *data._terminal);
    return obj;
}

template<>
Utils::Result<ClientNotification> fromJson<ClientNotification>(const QJsonValue &val) {
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

QJsonObject toJson(const ClientNotification &data) {
    QJsonObject obj{{"method", data._method}};
    if (data._params.has_value())
        obj.insert("params", *data._params);
    return obj;
}

template<>
Utils::Result<InitializeRequest> fromJson<InitializeRequest>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InitializeRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("protocolVersion"))
        return Utils::ResultError("Missing required field: protocolVersion");
    InitializeRequest result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    if (obj.contains("clientCapabilities"))
        result._clientCapabilities = obj.value("clientCapabilities").toString();
    if (obj.contains("clientInfo"))
        result._clientInfo = obj.value("clientInfo").toString();
    result._protocolVersion = obj.value("protocolVersion").toString();
    return result;
}

QJsonObject toJson(const InitializeRequest &data) {
    QJsonObject obj{{"protocolVersion", data._protocolVersion}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    if (data._clientCapabilities.has_value())
        obj.insert("clientCapabilities", *data._clientCapabilities);
    if (data._clientInfo.has_value())
        obj.insert("clientInfo", *data._clientInfo);
    return obj;
}

template<>
Utils::Result<HttpHeader> fromJson<HttpHeader>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for HttpHeader");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    if (!obj.contains("value"))
        return Utils::ResultError("Missing required field: value");
    HttpHeader result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    result._name = obj.value("name").toString();
    result._value = obj.value("value").toString();
    return result;
}

QJsonObject toJson(const HttpHeader &data) {
    QJsonObject obj{
        {"name", data._name},
        {"value", data._value}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<McpServerHttp> fromJson<McpServerHttp>(const QJsonValue &val) {
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
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    if (obj.contains("headers") && obj["headers"].isArray()) {
        QJsonArray arr = obj["headers"].toArray();
        for (const QJsonValue &v : arr) {
            result._headers.append(co_await fromJson<HttpHeader>(v));
        }
    }
    result._name = obj.value("name").toString();
    result._url = obj.value("url").toString();
    co_return result;
}

QJsonObject toJson(const McpServerHttp &data) {
    QJsonObject obj{
        {"name", data._name},
        {"url", data._url}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    QJsonArray arr_headers;
    for (const auto &v : data._headers) arr_headers.append(toJson(v));
    obj.insert("headers", arr_headers);
    return obj;
}

template<>
Utils::Result<McpServerSse> fromJson<McpServerSse>(const QJsonValue &val) {
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
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    if (obj.contains("headers") && obj["headers"].isArray()) {
        QJsonArray arr = obj["headers"].toArray();
        for (const QJsonValue &v : arr) {
            result._headers.append(co_await fromJson<HttpHeader>(v));
        }
    }
    result._name = obj.value("name").toString();
    result._url = obj.value("url").toString();
    co_return result;
}

QJsonObject toJson(const McpServerSse &data) {
    QJsonObject obj{
        {"name", data._name},
        {"url", data._url}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    QJsonArray arr_headers;
    for (const auto &v : data._headers) arr_headers.append(toJson(v));
    obj.insert("headers", arr_headers);
    return obj;
}

template<>
Utils::Result<McpServerStdio> fromJson<McpServerStdio>(const QJsonValue &val) {
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
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    if (obj.contains("args") && obj["args"].isArray()) {
        QJsonArray arr = obj["args"].toArray();
        for (const QJsonValue &v : arr) {
            result._args.append(v.toString());
        }
    }
    result._command = obj.value("command").toString();
    if (obj.contains("env") && obj["env"].isArray()) {
        QJsonArray arr = obj["env"].toArray();
        for (const QJsonValue &v : arr) {
            result._env.append(co_await fromJson<EnvVariable>(v));
        }
    }
    result._name = obj.value("name").toString();
    co_return result;
}

QJsonObject toJson(const McpServerStdio &data) {
    QJsonObject obj{
        {"command", data._command},
        {"name", data._name}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    QJsonArray arr_args;
    for (const auto &v : data._args) arr_args.append(v);
    obj.insert("args", arr_args);
    QJsonArray arr_env;
    for (const auto &v : data._env) arr_env.append(toJson(v));
    obj.insert("env", arr_env);
    return obj;
}

template<>
Utils::Result<McpServer> fromJson<McpServer>(const QJsonValue &val) {
    if (!val.isObject())
        co_return Utils::ResultError("Invalid McpServer: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("args"))
        co_return McpServer(co_await fromJson<McpServerStdio>(val));
    {
        auto result = fromJson<McpServerHttp>(val);
        if (result) co_return McpServer(*result);
    }
    {
        auto result = fromJson<McpServerSse>(val);
        if (result) co_return McpServer(*result);
    }
    co_return Utils::ResultError("Invalid McpServer");
}

QString name(const McpServer &val) {
    return std::visit([](const auto &v) -> QString { return v._name; }, val);
}

QJsonObject toJson(const McpServer &val) {
    return std::visit([](const auto &v) -> QJsonObject {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
}

QJsonValue toJsonValue(const McpServer &val) {
    return toJson(val);
}

template<>
Utils::Result<LoadSessionRequest> fromJson<LoadSessionRequest>(const QJsonValue &val) {
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
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    result._cwd = obj.value("cwd").toString();
    if (obj.contains("mcpServers") && obj["mcpServers"].isArray()) {
        QJsonArray arr = obj["mcpServers"].toArray();
        for (const QJsonValue &v : arr) {
            result._mcpServers.append(co_await fromJson<McpServer>(v));
        }
    }
    result._sessionId = obj.value("sessionId").toString();
    co_return result;
}

QJsonObject toJson(const LoadSessionRequest &data) {
    QJsonObject obj{
        {"cwd", data._cwd},
        {"sessionId", data._sessionId}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    QJsonArray arr_mcpServers;
    for (const auto &v : data._mcpServers) arr_mcpServers.append(toJsonValue(v));
    obj.insert("mcpServers", arr_mcpServers);
    return obj;
}

template<>
Utils::Result<NewSessionRequest> fromJson<NewSessionRequest>(const QJsonValue &val) {
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for NewSessionRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("cwd"))
        co_return Utils::ResultError("Missing required field: cwd");
    if (!obj.contains("mcpServers"))
        co_return Utils::ResultError("Missing required field: mcpServers");
    NewSessionRequest result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    result._cwd = obj.value("cwd").toString();
    if (obj.contains("mcpServers") && obj["mcpServers"].isArray()) {
        QJsonArray arr = obj["mcpServers"].toArray();
        for (const QJsonValue &v : arr) {
            result._mcpServers.append(co_await fromJson<McpServer>(v));
        }
    }
    co_return result;
}

QJsonObject toJson(const NewSessionRequest &data) {
    QJsonObject obj{{"cwd", data._cwd}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    QJsonArray arr_mcpServers;
    for (const auto &v : data._mcpServers) arr_mcpServers.append(toJsonValue(v));
    obj.insert("mcpServers", arr_mcpServers);
    return obj;
}

template<>
Utils::Result<PromptRequest> fromJson<PromptRequest>(const QJsonValue &val) {
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for PromptRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        co_return Utils::ResultError("Missing required field: sessionId");
    if (!obj.contains("prompt"))
        co_return Utils::ResultError("Missing required field: prompt");
    PromptRequest result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    if (obj.contains("prompt") && obj["prompt"].isArray()) {
        QJsonArray arr = obj["prompt"].toArray();
        for (const QJsonValue &v : arr) {
            result._prompt.append(co_await fromJson<ContentBlock>(v));
        }
    }
    result._sessionId = obj.value("sessionId").toString();
    co_return result;
}

QJsonObject toJson(const PromptRequest &data) {
    QJsonObject obj{{"sessionId", data._sessionId}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    QJsonArray arr_prompt;
    for (const auto &v : data._prompt) arr_prompt.append(toJsonValue(v));
    obj.insert("prompt", arr_prompt);
    return obj;
}

template<>
Utils::Result<SetSessionConfigOptionRequest> fromJson<SetSessionConfigOptionRequest>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SetSessionConfigOptionRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    if (!obj.contains("configId"))
        return Utils::ResultError("Missing required field: configId");
    if (!obj.contains("value"))
        return Utils::ResultError("Missing required field: value");
    SetSessionConfigOptionRequest result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    result._configId = obj.value("configId").toString();
    result._sessionId = obj.value("sessionId").toString();
    result._value = obj.value("value").toString();
    return result;
}

QJsonObject toJson(const SetSessionConfigOptionRequest &data) {
    QJsonObject obj{
        {"configId", data._configId},
        {"sessionId", data._sessionId},
        {"value", data._value}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<SetSessionModeRequest> fromJson<SetSessionModeRequest>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SetSessionModeRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    if (!obj.contains("modeId"))
        return Utils::ResultError("Missing required field: modeId");
    SetSessionModeRequest result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    result._modeId = obj.value("modeId").toString();
    result._sessionId = obj.value("sessionId").toString();
    return result;
}

QJsonObject toJson(const SetSessionModeRequest &data) {
    QJsonObject obj{
        {"modeId", data._modeId},
        {"sessionId", data._sessionId}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<ClientRequest> fromJson<ClientRequest>(const QJsonValue &val) {
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

QJsonObject toJson(const ClientRequest &data) {
    QJsonObject obj{
        {"id", toJsonValue(data._id)},
        {"method", data._method}
    };
    if (data._params.has_value())
        obj.insert("params", *data._params);
    return obj;
}

template<>
Utils::Result<CreateTerminalResponse> fromJson<CreateTerminalResponse>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CreateTerminalResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("terminalId"))
        return Utils::ResultError("Missing required field: terminalId");
    CreateTerminalResponse result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    result._terminalId = obj.value("terminalId").toString();
    return result;
}

QJsonObject toJson(const CreateTerminalResponse &data) {
    QJsonObject obj{{"terminalId", data._terminalId}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<KillTerminalCommandResponse> fromJson<KillTerminalCommandResponse>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for KillTerminalCommandResponse");
    const QJsonObject obj = val.toObject();
    KillTerminalCommandResponse result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const KillTerminalCommandResponse &data) {
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<ReadTextFileResponse> fromJson<ReadTextFileResponse>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ReadTextFileResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("content"))
        return Utils::ResultError("Missing required field: content");
    ReadTextFileResponse result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    result._content = obj.value("content").toString();
    return result;
}

QJsonObject toJson(const ReadTextFileResponse &data) {
    QJsonObject obj{{"content", data._content}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<ReleaseTerminalResponse> fromJson<ReleaseTerminalResponse>(const QJsonValue &val) {
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

QJsonObject toJson(const ReleaseTerminalResponse &data) {
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<SelectedPermissionOutcome> fromJson<SelectedPermissionOutcome>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SelectedPermissionOutcome");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("optionId"))
        return Utils::ResultError("Missing required field: optionId");
    SelectedPermissionOutcome result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    result._optionId = obj.value("optionId").toString();
    return result;
}

QJsonObject toJson(const SelectedPermissionOutcome &data) {
    QJsonObject obj{{"optionId", data._optionId}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<RequestPermissionOutcome> fromJson<RequestPermissionOutcome>(const QJsonValue &val) {
    if (!val.isObject())
        co_return Utils::ResultError("Invalid RequestPermissionOutcome: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("optionId"))
        co_return RequestPermissionOutcome(co_await fromJson<SelectedPermissionOutcome>(val));
    co_return Utils::ResultError("Invalid RequestPermissionOutcome");
}

QJsonObject toJson(const RequestPermissionOutcome &val) {
    return std::visit([](const auto &v) -> QJsonObject {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
}

QJsonValue toJsonValue(const RequestPermissionOutcome &val) {
    return toJson(val);
}

template<>
Utils::Result<RequestPermissionResponse> fromJson<RequestPermissionResponse>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for RequestPermissionResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("outcome"))
        return Utils::ResultError("Missing required field: outcome");
    RequestPermissionResponse result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    result._outcome = obj.value("outcome").toString();
    return result;
}

QJsonObject toJson(const RequestPermissionResponse &data) {
    QJsonObject obj{{"outcome", data._outcome}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<TerminalExitStatus> fromJson<TerminalExitStatus>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TerminalExitStatus");
    const QJsonObject obj = val.toObject();
    TerminalExitStatus result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    if (obj.contains("exitCode"))
        if (!obj["exitCode"].isNull()) {
            result._exitCode = obj.value("exitCode").toInt();
        }
    if (obj.contains("signal"))
        if (!obj["signal"].isNull()) {
            result._signal = obj.value("signal").toString();
        }
    return result;
}

QJsonObject toJson(const TerminalExitStatus &data) {
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    if (data._exitCode.has_value())
        obj.insert("exitCode", *data._exitCode);
    if (data._signal.has_value())
        obj.insert("signal", *data._signal);
    return obj;
}

template<>
Utils::Result<TerminalOutputResponse> fromJson<TerminalOutputResponse>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TerminalOutputResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("output"))
        return Utils::ResultError("Missing required field: output");
    if (!obj.contains("truncated"))
        return Utils::ResultError("Missing required field: truncated");
    TerminalOutputResponse result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    if (obj.contains("exitStatus"))
        result._exitStatus = obj.value("exitStatus").toString();
    result._output = obj.value("output").toString();
    result._truncated = obj.value("truncated").toBool();
    return result;
}

QJsonObject toJson(const TerminalOutputResponse &data) {
    QJsonObject obj{
        {"output", data._output},
        {"truncated", data._truncated}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    if (data._exitStatus.has_value())
        obj.insert("exitStatus", *data._exitStatus);
    return obj;
}

template<>
Utils::Result<WaitForTerminalExitResponse> fromJson<WaitForTerminalExitResponse>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for WaitForTerminalExitResponse");
    const QJsonObject obj = val.toObject();
    WaitForTerminalExitResponse result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    if (obj.contains("exitCode"))
        if (!obj["exitCode"].isNull()) {
            result._exitCode = obj.value("exitCode").toInt();
        }
    if (obj.contains("signal"))
        if (!obj["signal"].isNull()) {
            result._signal = obj.value("signal").toString();
        }
    return result;
}

QJsonObject toJson(const WaitForTerminalExitResponse &data) {
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    if (data._exitCode.has_value())
        obj.insert("exitCode", *data._exitCode);
    if (data._signal.has_value())
        obj.insert("signal", *data._signal);
    return obj;
}

template<>
Utils::Result<WriteTextFileResponse> fromJson<WriteTextFileResponse>(const QJsonValue &val) {
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

QJsonObject toJson(const WriteTextFileResponse &data) {
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

} // namespace Acp
