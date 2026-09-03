// This file is auto-generated. Do not edit manually.
#include "schema_2025_11_25.h"

namespace Mcp::Generated::Schema::_2025_11_25 {

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
    if (obj.contains("audience") && obj["audience"].isArray()) {
        const QJsonArray arr = obj["audience"].toArray();
        QList<Role> list_audience;
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<Role>(v);
            if (!res0)
                return Utils::ResultError(res0.error());
            list_audience.append(*res0);
        }
        result._audience = list_audience;
    }
    if (obj.contains("lastModified"))
        result._lastModified = obj.value("lastModified").toString();
    if (obj.contains("priority"))
        result._priority = obj.value("priority").toDouble();
    return result;
}

QJsonObject toJson(const Annotations &data)
{
    QJsonObject obj;
    if (data._audience.has_value()) {
        QJsonArray arr_audience;
        for (const auto &v : *data._audience) arr_audience.append(toJsonValue(v));
        obj.insert("audience", arr_audience);
    }
    if (data._lastModified.has_value())
        obj.insert("lastModified", *data._lastModified);
    if (data._priority.has_value())
        obj.insert("priority", *data._priority);
    return obj;
}

template<>
Utils::Result<AudioContent> fromJson<AudioContent>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for AudioContent");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("data"))
        return Utils::ResultError("Missing required field: data");
    if (!obj.contains("mimeType"))
        return Utils::ResultError("Missing required field: mimeType");
    if (!obj.contains("type"))
        return Utils::ResultError("Missing required field: type");
    AudioContent result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const QJsonObject mapObj__meta = obj["_meta"].toObject();
        QMap<QString, QJsonValue> map__meta;
        for (auto it = mapObj__meta.constBegin(); it != mapObj__meta.constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        result.__meta = map__meta;
    }
    if (obj.contains("annotations") && obj["annotations"].isObject()) {
        const auto res0 = fromJson<Annotations>(obj["annotations"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._annotations = *res0;
    }
    result._data = obj.value("data").toString();
    result._mimeType = obj.value("mimeType").toString();
    if (obj.value("type").toString() != "audio")
        return Utils::ResultError("Field 'type' must be 'audio', got: " + obj.value("type").toString());
    return result;
}

QJsonObject toJson(const AudioContent &data)
{
    QJsonObject obj{
        {"data", data._data},
        {"mimeType", data._mimeType},
        {"type", QString("audio")}
    };
    if (data.__meta.has_value()) {
        QJsonObject map__meta;
        for (auto it = data.__meta->constBegin(); it != data.__meta->constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        obj.insert("_meta", map__meta);
    }
    if (data._annotations.has_value())
        obj.insert("annotations", toJson(*data._annotations));
    return obj;
}

template<>
Utils::Result<BaseMetadata> fromJson<BaseMetadata>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for BaseMetadata");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    BaseMetadata result;
    result._name = obj.value("name").toString();
    if (obj.contains("title"))
        result._title = obj.value("title").toString();
    return result;
}

QJsonObject toJson(const BaseMetadata &data)
{
    QJsonObject obj{{"name", data._name}};
    if (data._title.has_value())
        obj.insert("title", *data._title);
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
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const QJsonObject mapObj__meta = obj["_meta"].toObject();
        QMap<QString, QJsonValue> map__meta;
        for (auto it = mapObj__meta.constBegin(); it != mapObj__meta.constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        result.__meta = map__meta;
    }
    result._blob = obj.value("blob").toString();
    if (obj.contains("mimeType"))
        result._mimeType = obj.value("mimeType").toString();
    result._uri = obj.value("uri").toString();
    return result;
}

QJsonObject toJson(const BlobResourceContents &data)
{
    QJsonObject obj{
        {"blob", data._blob},
        {"uri", data._uri}
    };
    if (data.__meta.has_value()) {
        QJsonObject map__meta;
        for (auto it = data.__meta->constBegin(); it != data.__meta->constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        obj.insert("_meta", map__meta);
    }
    if (data._mimeType.has_value())
        obj.insert("mimeType", *data._mimeType);
    return obj;
}

template<>
Utils::Result<BooleanSchema> fromJson<BooleanSchema>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for BooleanSchema");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("type"))
        return Utils::ResultError("Missing required field: type");
    BooleanSchema result;
    if (obj.contains("default"))
        result._default_ = obj.value("default").toBool();
    if (obj.contains("description"))
        result._description = obj.value("description").toString();
    if (obj.contains("title"))
        result._title = obj.value("title").toString();
    if (obj.value("type").toString() != "boolean")
        return Utils::ResultError("Field 'type' must be 'boolean', got: " + obj.value("type").toString());
    return result;
}

QJsonObject toJson(const BooleanSchema &data)
{
    QJsonObject obj{{"type", QString("boolean")}};
    if (data._default_.has_value())
        obj.insert("default", *data._default_);
    if (data._description.has_value())
        obj.insert("description", *data._description);
    if (data._title.has_value())
        obj.insert("title", *data._title);
    return obj;
}

template<>
Utils::Result<ProgressToken> fromJson<ProgressToken>(const QJsonValue &val)
{
    if (val.isString()) {
        return ProgressToken(val.toString());
    }
    if (val.isDouble()) {
        return ProgressToken(val.toInt());
    }
    return Utils::ResultError("Invalid ProgressToken");
}

QJsonValue toJsonValue(const ProgressToken &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<TaskMetadata> fromJson<TaskMetadata>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TaskMetadata");
    const QJsonObject obj = val.toObject();
    TaskMetadata result;
    if (obj.contains("ttl"))
        result._ttl = obj.value("ttl").toInt();
    return result;
}

QJsonObject toJson(const TaskMetadata &data)
{
    QJsonObject obj;
    if (data._ttl.has_value())
        obj.insert("ttl", *data._ttl);
    return obj;
}

template<>
Utils::Result<CallToolRequestParams::Meta> fromJson<CallToolRequestParams::Meta>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Meta");
    const QJsonObject obj = val.toObject();
    CallToolRequestParams::Meta result;
    if (obj.contains("progressToken")) {
        const auto res0 = fromJson<ProgressToken>(obj["progressToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._progressToken = *res0;
    }
    return result;
}

QJsonObject toJson(const CallToolRequestParams::Meta &data)
{
    QJsonObject obj;
    if (data._progressToken.has_value())
        obj.insert("progressToken", toJsonValue(*data._progressToken));
    return obj;
}

template<>
Utils::Result<CallToolRequestParams> fromJson<CallToolRequestParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CallToolRequestParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    CallToolRequestParams result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<CallToolRequestParams::Meta>(obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    if (obj.contains("arguments") && obj["arguments"].isObject()) {
        const QJsonObject mapObj_arguments = obj["arguments"].toObject();
        QMap<QString, QJsonValue> map_arguments;
        for (auto it = mapObj_arguments.constBegin(); it != mapObj_arguments.constEnd(); ++it)
            map_arguments.insert(it.key(), it.value());
        result._arguments = map_arguments;
    }
    result._name = obj.value("name").toString();
    if (obj.contains("task") && obj["task"].isObject()) {
        const auto res1 = fromJson<TaskMetadata>(obj["task"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._task = *res1;
    }
    return result;
}

QJsonObject toJson(const CallToolRequestParams &data)
{
    QJsonObject obj{{"name", data._name}};
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
    if (data._arguments.has_value()) {
        QJsonObject map_arguments;
        for (auto it = data._arguments->constBegin(); it != data._arguments->constEnd(); ++it)
            map_arguments.insert(it.key(), it.value());
        obj.insert("arguments", map_arguments);
    }
    if (data._task.has_value())
        obj.insert("task", toJson(*data._task));
    return obj;
}

template<>
Utils::Result<CallToolRequest> fromJson<CallToolRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CallToolRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    if (!obj.contains("params"))
        return Utils::ResultError("Missing required field: params");
    CallToolRequest result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>(obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "tools/call")
        return Utils::ResultError("Field 'method' must be 'tools/call', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res1 = fromJson<CallToolRequestParams>(obj["params"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._params = *res1;
    }
    return result;
}

QJsonObject toJson(const CallToolRequest &data)
{
    QJsonObject obj{
        {"id", toJsonValue(data._id)},
        {"jsonrpc", QString("2.0")},
        {"method", QString("tools/call")},
        {"params", toJson(data._params)}
    };
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
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const QJsonObject mapObj__meta = obj["_meta"].toObject();
        QMap<QString, QJsonValue> map__meta;
        for (auto it = mapObj__meta.constBegin(); it != mapObj__meta.constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        result.__meta = map__meta;
    }
    if (obj.contains("mimeType"))
        result._mimeType = obj.value("mimeType").toString();
    result._text = obj.value("text").toString();
    result._uri = obj.value("uri").toString();
    return result;
}

QJsonObject toJson(const TextResourceContents &data)
{
    QJsonObject obj{
        {"text", data._text},
        {"uri", data._uri}
    };
    if (data.__meta.has_value()) {
        QJsonObject map__meta;
        for (auto it = data.__meta->constBegin(); it != data.__meta->constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        obj.insert("_meta", map__meta);
    }
    if (data._mimeType.has_value())
        obj.insert("mimeType", *data._mimeType);
    return obj;
}

template<>
Utils::Result<EmbeddedResourceResource> fromJson<EmbeddedResourceResource>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid EmbeddedResourceResource: expected object or array");
    const QJsonObject obj = val.toObject();
    if (obj.contains("text")) {
        const auto res0 = fromJson<TextResourceContents>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return EmbeddedResourceResource(*res0);
    }
    if (obj.contains("blob")) {
        const auto res1 = fromJson<BlobResourceContents>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        return EmbeddedResourceResource(*res1);
    }
    return Utils::ResultError("Invalid EmbeddedResourceResource");
}

QJsonValue toJsonValue(const EmbeddedResourceResource &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
}

template<>
Utils::Result<EmbeddedResource> fromJson<EmbeddedResource>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for EmbeddedResource");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("resource"))
        return Utils::ResultError("Missing required field: resource");
    if (!obj.contains("type"))
        return Utils::ResultError("Missing required field: type");
    EmbeddedResource result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const QJsonObject mapObj__meta = obj["_meta"].toObject();
        QMap<QString, QJsonValue> map__meta;
        for (auto it = mapObj__meta.constBegin(); it != mapObj__meta.constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        result.__meta = map__meta;
    }
    if (obj.contains("annotations") && obj["annotations"].isObject()) {
        const auto res0 = fromJson<Annotations>(obj["annotations"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._annotations = *res0;
    }
    if (obj.contains("resource")) {
        const auto res1 = fromJson<EmbeddedResourceResource>(obj["resource"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._resource = *res1;
    }
    if (obj.value("type").toString() != "resource")
        return Utils::ResultError("Field 'type' must be 'resource', got: " + obj.value("type").toString());
    return result;
}

QJsonObject toJson(const EmbeddedResource &data)
{
    QJsonObject obj{
        {"resource", toJsonValue(data._resource)},
        {"type", QString("resource")}
    };
    if (data.__meta.has_value()) {
        QJsonObject map__meta;
        for (auto it = data.__meta->constBegin(); it != data.__meta->constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        obj.insert("_meta", map__meta);
    }
    if (data._annotations.has_value())
        obj.insert("annotations", toJson(*data._annotations));
    return obj;
}

template<>
Utils::Result<ImageContent> fromJson<ImageContent>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ImageContent");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("data"))
        return Utils::ResultError("Missing required field: data");
    if (!obj.contains("mimeType"))
        return Utils::ResultError("Missing required field: mimeType");
    if (!obj.contains("type"))
        return Utils::ResultError("Missing required field: type");
    ImageContent result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const QJsonObject mapObj__meta = obj["_meta"].toObject();
        QMap<QString, QJsonValue> map__meta;
        for (auto it = mapObj__meta.constBegin(); it != mapObj__meta.constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        result.__meta = map__meta;
    }
    if (obj.contains("annotations") && obj["annotations"].isObject()) {
        const auto res0 = fromJson<Annotations>(obj["annotations"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._annotations = *res0;
    }
    result._data = obj.value("data").toString();
    result._mimeType = obj.value("mimeType").toString();
    if (obj.value("type").toString() != "image")
        return Utils::ResultError("Field 'type' must be 'image', got: " + obj.value("type").toString());
    return result;
}

QJsonObject toJson(const ImageContent &data)
{
    QJsonObject obj{
        {"data", data._data},
        {"mimeType", data._mimeType},
        {"type", QString("image")}
    };
    if (data.__meta.has_value()) {
        QJsonObject map__meta;
        for (auto it = data.__meta->constBegin(); it != data.__meta->constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        obj.insert("_meta", map__meta);
    }
    if (data._annotations.has_value())
        obj.insert("annotations", toJson(*data._annotations));
    return obj;
}

QString toString(const Icon::Theme &v)
{
    switch(v) {
        case Icon::Theme::dark: return "dark";
        case Icon::Theme::light: return "light";
    }
    return {};
}

template<>
Utils::Result<Icon::Theme> fromJson<Icon::Theme>(const QJsonValue &val)
{
    const QString str = val.toString();
    if (str == "dark") return Icon::Theme::dark;
    if (str == "light") return Icon::Theme::light;
    return Utils::ResultError("Invalid Icon::Theme value: " + str);
}

QJsonValue toJsonValue(const Icon::Theme &v)
{
    return toString(v);
}

template<>
Utils::Result<Icon> fromJson<Icon>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Icon");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("src"))
        return Utils::ResultError("Missing required field: src");
    Icon result;
    if (obj.contains("mimeType"))
        result._mimeType = obj.value("mimeType").toString();
    if (obj.contains("sizes") && obj["sizes"].isArray()) {
        const QJsonArray arr = obj["sizes"].toArray();
        QStringList list_sizes;
        for (const QJsonValue &v : arr) {
            list_sizes.append(v.toString());
        }
        result._sizes = list_sizes;
    }
    result._src = obj.value("src").toString();
    if (obj.contains("theme") && obj["theme"].isString()) {
        const auto res0 = fromJson<Icon::Theme>(obj["theme"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._theme = *res0;
    }
    return result;
}

QJsonObject toJson(const Icon &data)
{
    QJsonObject obj{{"src", data._src}};
    if (data._mimeType.has_value())
        obj.insert("mimeType", *data._mimeType);
    if (data._sizes.has_value()) {
        QJsonArray arr_sizes;
        for (const auto &v : *data._sizes) arr_sizes.append(v);
        obj.insert("sizes", arr_sizes);
    }
    if (data._theme.has_value())
        obj.insert("theme", toJsonValue(*data._theme));
    return obj;
}

template<>
Utils::Result<ResourceLink> fromJson<ResourceLink>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ResourceLink");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    if (!obj.contains("type"))
        return Utils::ResultError("Missing required field: type");
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    ResourceLink result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const QJsonObject mapObj__meta = obj["_meta"].toObject();
        QMap<QString, QJsonValue> map__meta;
        for (auto it = mapObj__meta.constBegin(); it != mapObj__meta.constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        result.__meta = map__meta;
    }
    if (obj.contains("annotations") && obj["annotations"].isObject()) {
        const auto res0 = fromJson<Annotations>(obj["annotations"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._annotations = *res0;
    }
    if (obj.contains("description"))
        result._description = obj.value("description").toString();
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
    if (obj.contains("mimeType"))
        result._mimeType = obj.value("mimeType").toString();
    result._name = obj.value("name").toString();
    if (obj.contains("size"))
        result._size = obj.value("size").toInt();
    if (obj.contains("title"))
        result._title = obj.value("title").toString();
    if (obj.value("type").toString() != "resource_link")
        return Utils::ResultError("Field 'type' must be 'resource_link', got: " + obj.value("type").toString());
    result._uri = obj.value("uri").toString();
    return result;
}

QJsonObject toJson(const ResourceLink &data)
{
    QJsonObject obj{
        {"name", data._name},
        {"type", QString("resource_link")},
        {"uri", data._uri}
    };
    if (data.__meta.has_value()) {
        QJsonObject map__meta;
        for (auto it = data.__meta->constBegin(); it != data.__meta->constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        obj.insert("_meta", map__meta);
    }
    if (data._annotations.has_value())
        obj.insert("annotations", toJson(*data._annotations));
    if (data._description.has_value())
        obj.insert("description", *data._description);
    if (data._icons.has_value()) {
        QJsonArray arr_icons;
        for (const auto &v : *data._icons) arr_icons.append(toJson(v));
        obj.insert("icons", arr_icons);
    }
    if (data._mimeType.has_value())
        obj.insert("mimeType", *data._mimeType);
    if (data._size.has_value())
        obj.insert("size", *data._size);
    if (data._title.has_value())
        obj.insert("title", *data._title);
    return obj;
}

template<>
Utils::Result<TextContent> fromJson<TextContent>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TextContent");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("text"))
        return Utils::ResultError("Missing required field: text");
    if (!obj.contains("type"))
        return Utils::ResultError("Missing required field: type");
    TextContent result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const QJsonObject mapObj__meta = obj["_meta"].toObject();
        QMap<QString, QJsonValue> map__meta;
        for (auto it = mapObj__meta.constBegin(); it != mapObj__meta.constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        result.__meta = map__meta;
    }
    if (obj.contains("annotations") && obj["annotations"].isObject()) {
        const auto res0 = fromJson<Annotations>(obj["annotations"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._annotations = *res0;
    }
    result._text = obj.value("text").toString();
    if (obj.value("type").toString() != "text")
        return Utils::ResultError("Field 'type' must be 'text', got: " + obj.value("type").toString());
    return result;
}

QJsonObject toJson(const TextContent &data)
{
    QJsonObject obj{
        {"text", data._text},
        {"type", QString("text")}
    };
    if (data.__meta.has_value()) {
        QJsonObject map__meta;
        for (auto it = data.__meta->constBegin(); it != data.__meta->constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        obj.insert("_meta", map__meta);
    }
    if (data._annotations.has_value())
        obj.insert("annotations", toJson(*data._annotations));
    return obj;
}

template<>
Utils::Result<ContentBlock> fromJson<ContentBlock>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid ContentBlock: expected object");
    const QString dispatchValue = val.toObject().value("type").toString();
    if (dispatchValue == "text") {
        const auto res0 = fromJson<TextContent>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return ContentBlock(*res0);
    }
    else if (dispatchValue == "image") {
        const auto res1 = fromJson<ImageContent>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        return ContentBlock(*res1);
    }
    else if (dispatchValue == "audio") {
        const auto res2 = fromJson<AudioContent>(val);
        if (!res2)
            return Utils::ResultError(res2.error());
        return ContentBlock(*res2);
    }
    else if (dispatchValue == "resource_link") {
        const auto res3 = fromJson<ResourceLink>(val);
        if (!res3)
            return Utils::ResultError(res3.error());
        return ContentBlock(*res3);
    }
    else if (dispatchValue == "resource") {
        const auto res4 = fromJson<EmbeddedResource>(val);
        if (!res4)
            return Utils::ResultError(res4.error());
        return ContentBlock(*res4);
    }
    return Utils::ResultError("Invalid ContentBlock: unknown type \"" + dispatchValue + "\"");
}

QJsonObject toJson(const ContentBlock &val)
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

QJsonValue toJsonValue(const ContentBlock &val)
{
    return toJson(val);
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

template<>
Utils::Result<CallToolResult> fromJson<CallToolResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CallToolResult");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("content"))
        return Utils::ResultError("Missing required field: content");
    CallToolResult result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const QJsonObject mapObj__meta = obj["_meta"].toObject();
        QMap<QString, QJsonValue> map__meta;
        for (auto it = mapObj__meta.constBegin(); it != mapObj__meta.constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        result.__meta = map__meta;
    }
    if (obj.contains("content") && obj["content"].isArray()) {
        const QJsonArray arr = obj["content"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<ContentBlock>(v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._content.append(*res0);
        }
    }
    if (obj.contains("isError"))
        result._isError = obj.value("isError").toBool();
    if (obj.contains("structuredContent") && obj["structuredContent"].isObject()) {
        const QJsonObject mapObj_structuredContent = obj["structuredContent"].toObject();
        QMap<QString, QJsonValue> map_structuredContent;
        for (auto it = mapObj_structuredContent.constBegin(); it != mapObj_structuredContent.constEnd(); ++it)
            map_structuredContent.insert(it.key(), it.value());
        result._structuredContent = map_structuredContent;
    }
    return result;
}

QJsonObject toJson(const CallToolResult &data)
{
    QJsonObject obj;
    if (data.__meta.has_value()) {
        QJsonObject map__meta;
        for (auto it = data.__meta->constBegin(); it != data.__meta->constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        obj.insert("_meta", map__meta);
    }
    QJsonArray arr_content;
    for (const auto &v : data._content) arr_content.append(toJsonValue(v));
    obj.insert("content", arr_content);
    if (data._isError.has_value())
        obj.insert("isError", *data._isError);
    if (data._structuredContent.has_value()) {
        QJsonObject map_structuredContent;
        for (auto it = data._structuredContent->constBegin(); it != data._structuredContent->constEnd(); ++it)
            map_structuredContent.insert(it.key(), it.value());
        obj.insert("structuredContent", map_structuredContent);
    }
    return obj;
}

template<>
Utils::Result<CancelTaskRequest::Params> fromJson<CancelTaskRequest::Params>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Params");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("taskId"))
        return Utils::ResultError("Missing required field: taskId");
    CancelTaskRequest::Params result;
    result._taskId = obj.value("taskId").toString();
    return result;
}

QJsonObject toJson(const CancelTaskRequest::Params &data)
{
    QJsonObject obj{{"taskId", data._taskId}};
    return obj;
}

template<>
Utils::Result<CancelTaskRequest> fromJson<CancelTaskRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CancelTaskRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    if (!obj.contains("params"))
        return Utils::ResultError("Missing required field: params");
    CancelTaskRequest result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>(obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "tasks/cancel")
        return Utils::ResultError("Field 'method' must be 'tasks/cancel', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res1 = fromJson<CancelTaskRequest::Params>(obj["params"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._params = *res1;
    }
    return result;
}

QJsonObject toJson(const CancelTaskRequest &data)
{
    QJsonObject obj{
        {"id", toJsonValue(data._id)},
        {"jsonrpc", QString("2.0")},
        {"method", QString("tasks/cancel")},
        {"params", toJson(data._params)}
    };
    return obj;
}

template<>
Utils::Result<Result> fromJson<Result>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Result");
    const QJsonObject obj = val.toObject();
    Result result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const QJsonObject mapObj__meta = obj["_meta"].toObject();
        QMap<QString, QJsonValue> map__meta;
        for (auto it = mapObj__meta.constBegin(); it != mapObj__meta.constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        result.__meta = map__meta;
    }
    {
        const QSet<QString> knownKeys{"_meta"};
        for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
            if (!knownKeys.contains(it.key()))
                result._additionalProperties.insert(it.key(), it.value());
        }
    }
    return result;
}

QJsonObject toJson(const Result &data)
{
    QJsonObject obj;
    if (data.__meta.has_value()) {
        QJsonObject map__meta;
        for (auto it = data.__meta->constBegin(); it != data.__meta->constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        obj.insert("_meta", map__meta);
    }
    for (auto it = data._additionalProperties.constBegin(); it != data._additionalProperties.constEnd(); ++it)
        obj.insert(it.key(), it.value());
    return obj;
}

QString toString(TaskStatus v)
{
    switch(v) {
        case TaskStatus::cancelled: return "cancelled";
        case TaskStatus::completed: return "completed";
        case TaskStatus::failed: return "failed";
        case TaskStatus::input_required: return "input_required";
        case TaskStatus::working: return "working";
    }
    return {};
}

template<>
Utils::Result<TaskStatus> fromJson<TaskStatus>(const QJsonValue &val)
{
    const QString str = val.toString();
    if (str == "cancelled") return TaskStatus::cancelled;
    if (str == "completed") return TaskStatus::completed;
    if (str == "failed") return TaskStatus::failed;
    if (str == "input_required") return TaskStatus::input_required;
    if (str == "working") return TaskStatus::working;
    return Utils::ResultError("Invalid TaskStatus value: " + str);
}

QJsonValue toJsonValue(const TaskStatus &v)
{
    return toString(v);
}

template<>
Utils::Result<Task> fromJson<Task>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Task");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("createdAt"))
        return Utils::ResultError("Missing required field: createdAt");
    if (!obj.contains("lastUpdatedAt"))
        return Utils::ResultError("Missing required field: lastUpdatedAt");
    if (!obj.contains("status"))
        return Utils::ResultError("Missing required field: status");
    if (!obj.contains("taskId"))
        return Utils::ResultError("Missing required field: taskId");
    if (!obj.contains("ttl"))
        return Utils::ResultError("Missing required field: ttl");
    Task result;
    result._createdAt = obj.value("createdAt").toString();
    result._lastUpdatedAt = obj.value("lastUpdatedAt").toString();
    if (obj.contains("pollInterval"))
        result._pollInterval = obj.value("pollInterval").toInt();
    if (obj.contains("status") && obj["status"].isString()) {
        const auto res0 = fromJson<TaskStatus>(obj["status"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._status = *res0;
    }
    if (obj.contains("statusMessage"))
        result._statusMessage = obj.value("statusMessage").toString();
    result._taskId = obj.value("taskId").toString();
    if (!obj["ttl"].isNull()) {
        result._ttl = obj.value("ttl").toInt();
    }
    return result;
}

QJsonObject toJson(const Task &data)
{
    QJsonObject obj{
        {"createdAt", data._createdAt},
        {"lastUpdatedAt", data._lastUpdatedAt},
        {"status", toJsonValue(data._status)},
        {"taskId", data._taskId}
    };
    if (data._pollInterval.has_value())
        obj.insert("pollInterval", *data._pollInterval);
    if (data._statusMessage.has_value())
        obj.insert("statusMessage", *data._statusMessage);
    if (data._ttl.has_value())
        obj.insert("ttl", *data._ttl);
    else
        obj.insert("ttl", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<CancelTaskResult> fromJson<CancelTaskResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CancelTaskResult");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("createdAt"))
        return Utils::ResultError("Missing required field: createdAt");
    if (!obj.contains("lastUpdatedAt"))
        return Utils::ResultError("Missing required field: lastUpdatedAt");
    if (!obj.contains("status"))
        return Utils::ResultError("Missing required field: status");
    if (!obj.contains("taskId"))
        return Utils::ResultError("Missing required field: taskId");
    if (!obj.contains("ttl"))
        return Utils::ResultError("Missing required field: ttl");
    CancelTaskResult result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const QJsonObject mapObj__meta = obj["_meta"].toObject();
        QMap<QString, QJsonValue> map__meta;
        for (auto it = mapObj__meta.constBegin(); it != mapObj__meta.constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        result.__meta = map__meta;
    }
    result._createdAt = obj.value("createdAt").toString();
    result._lastUpdatedAt = obj.value("lastUpdatedAt").toString();
    if (obj.contains("pollInterval"))
        result._pollInterval = obj.value("pollInterval").toInt();
    if (obj.contains("status") && obj["status"].isString()) {
        const auto res0 = fromJson<TaskStatus>(obj["status"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._status = *res0;
    }
    if (obj.contains("statusMessage"))
        result._statusMessage = obj.value("statusMessage").toString();
    result._taskId = obj.value("taskId").toString();
    if (!obj["ttl"].isNull()) {
        result._ttl = obj.value("ttl").toInt();
    }
    return result;
}

QJsonObject toJson(const CancelTaskResult &data)
{
    QJsonObject obj{
        {"createdAt", data._createdAt},
        {"lastUpdatedAt", data._lastUpdatedAt},
        {"status", toJsonValue(data._status)},
        {"taskId", data._taskId}
    };
    if (data.__meta.has_value()) {
        QJsonObject map__meta;
        for (auto it = data.__meta->constBegin(); it != data.__meta->constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        obj.insert("_meta", map__meta);
    }
    if (data._pollInterval.has_value())
        obj.insert("pollInterval", *data._pollInterval);
    if (data._statusMessage.has_value())
        obj.insert("statusMessage", *data._statusMessage);
    if (data._ttl.has_value())
        obj.insert("ttl", *data._ttl);
    else
        obj.insert("ttl", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<CancelledNotificationParams> fromJson<CancelledNotificationParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CancelledNotificationParams");
    const QJsonObject obj = val.toObject();
    CancelledNotificationParams result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const QJsonObject mapObj__meta = obj["_meta"].toObject();
        QMap<QString, QJsonValue> map__meta;
        for (auto it = mapObj__meta.constBegin(); it != mapObj__meta.constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        result.__meta = map__meta;
    }
    if (obj.contains("reason"))
        result._reason = obj.value("reason").toString();
    if (obj.contains("requestId")) {
        const auto res0 = fromJson<RequestId>(obj["requestId"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._requestId = *res0;
    }
    return result;
}

QJsonObject toJson(const CancelledNotificationParams &data)
{
    QJsonObject obj;
    if (data.__meta.has_value()) {
        QJsonObject map__meta;
        for (auto it = data.__meta->constBegin(); it != data.__meta->constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        obj.insert("_meta", map__meta);
    }
    if (data._reason.has_value())
        obj.insert("reason", *data._reason);
    if (data._requestId.has_value())
        obj.insert("requestId", toJsonValue(*data._requestId));
    return obj;
}

template<>
Utils::Result<CancelledNotification> fromJson<CancelledNotification>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CancelledNotification");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    if (!obj.contains("params"))
        return Utils::ResultError("Missing required field: params");
    CancelledNotification result;
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "notifications/cancelled")
        return Utils::ResultError("Field 'method' must be 'notifications/cancelled', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res0 = fromJson<CancelledNotificationParams>(obj["params"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._params = *res0;
    }
    return result;
}

QJsonObject toJson(const CancelledNotification &data)
{
    QJsonObject obj{
        {"jsonrpc", QString("2.0")},
        {"method", QString("notifications/cancelled")},
        {"params", toJson(data._params)}
    };
    return obj;
}

template<>
Utils::Result<ClientCapabilities::Elicitation> fromJson<ClientCapabilities::Elicitation>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Elicitation");
    const QJsonObject obj = val.toObject();
    ClientCapabilities::Elicitation result;
    if (obj.contains("form") && obj["form"].isObject()) {
        const QJsonObject mapObj_form = obj["form"].toObject();
        QMap<QString, QJsonValue> map_form;
        for (auto it = mapObj_form.constBegin(); it != mapObj_form.constEnd(); ++it)
            map_form.insert(it.key(), it.value());
        result._form = map_form;
    }
    if (obj.contains("url") && obj["url"].isObject()) {
        const QJsonObject mapObj_url = obj["url"].toObject();
        QMap<QString, QJsonValue> map_url;
        for (auto it = mapObj_url.constBegin(); it != mapObj_url.constEnd(); ++it)
            map_url.insert(it.key(), it.value());
        result._url = map_url;
    }
    return result;
}

QJsonObject toJson(const ClientCapabilities::Elicitation &data)
{
    QJsonObject obj;
    if (data._form.has_value()) {
        QJsonObject map_form;
        for (auto it = data._form->constBegin(); it != data._form->constEnd(); ++it)
            map_form.insert(it.key(), it.value());
        obj.insert("form", map_form);
    }
    if (data._url.has_value()) {
        QJsonObject map_url;
        for (auto it = data._url->constBegin(); it != data._url->constEnd(); ++it)
            map_url.insert(it.key(), it.value());
        obj.insert("url", map_url);
    }
    return obj;
}

template<>
Utils::Result<ClientCapabilities::Roots> fromJson<ClientCapabilities::Roots>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Roots");
    const QJsonObject obj = val.toObject();
    ClientCapabilities::Roots result;
    if (obj.contains("listChanged"))
        result._listChanged = obj.value("listChanged").toBool();
    return result;
}

QJsonObject toJson(const ClientCapabilities::Roots &data)
{
    QJsonObject obj;
    if (data._listChanged.has_value())
        obj.insert("listChanged", *data._listChanged);
    return obj;
}

template<>
Utils::Result<ClientCapabilities::Sampling> fromJson<ClientCapabilities::Sampling>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Sampling");
    const QJsonObject obj = val.toObject();
    ClientCapabilities::Sampling result;
    if (obj.contains("context") && obj["context"].isObject()) {
        const QJsonObject mapObj_context = obj["context"].toObject();
        QMap<QString, QJsonValue> map_context;
        for (auto it = mapObj_context.constBegin(); it != mapObj_context.constEnd(); ++it)
            map_context.insert(it.key(), it.value());
        result._context = map_context;
    }
    if (obj.contains("tools") && obj["tools"].isObject()) {
        const QJsonObject mapObj_tools = obj["tools"].toObject();
        QMap<QString, QJsonValue> map_tools;
        for (auto it = mapObj_tools.constBegin(); it != mapObj_tools.constEnd(); ++it)
            map_tools.insert(it.key(), it.value());
        result._tools = map_tools;
    }
    return result;
}

QJsonObject toJson(const ClientCapabilities::Sampling &data)
{
    QJsonObject obj;
    if (data._context.has_value()) {
        QJsonObject map_context;
        for (auto it = data._context->constBegin(); it != data._context->constEnd(); ++it)
            map_context.insert(it.key(), it.value());
        obj.insert("context", map_context);
    }
    if (data._tools.has_value()) {
        QJsonObject map_tools;
        for (auto it = data._tools->constBegin(); it != data._tools->constEnd(); ++it)
            map_tools.insert(it.key(), it.value());
        obj.insert("tools", map_tools);
    }
    return obj;
}

template<>
Utils::Result<ClientCapabilities::Tasks::Requests::Elicitation> fromJson<ClientCapabilities::Tasks::Requests::Elicitation>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Elicitation");
    const QJsonObject obj = val.toObject();
    ClientCapabilities::Tasks::Requests::Elicitation result;
    if (obj.contains("create") && obj["create"].isObject()) {
        const QJsonObject mapObj_create = obj["create"].toObject();
        QMap<QString, QJsonValue> map_create;
        for (auto it = mapObj_create.constBegin(); it != mapObj_create.constEnd(); ++it)
            map_create.insert(it.key(), it.value());
        result._create = map_create;
    }
    return result;
}

QJsonObject toJson(const ClientCapabilities::Tasks::Requests::Elicitation &data)
{
    QJsonObject obj;
    if (data._create.has_value()) {
        QJsonObject map_create;
        for (auto it = data._create->constBegin(); it != data._create->constEnd(); ++it)
            map_create.insert(it.key(), it.value());
        obj.insert("create", map_create);
    }
    return obj;
}

template<>
Utils::Result<ClientCapabilities::Tasks::Requests::Sampling> fromJson<ClientCapabilities::Tasks::Requests::Sampling>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Sampling");
    const QJsonObject obj = val.toObject();
    ClientCapabilities::Tasks::Requests::Sampling result;
    if (obj.contains("createMessage") && obj["createMessage"].isObject()) {
        const QJsonObject mapObj_createMessage = obj["createMessage"].toObject();
        QMap<QString, QJsonValue> map_createMessage;
        for (auto it = mapObj_createMessage.constBegin(); it != mapObj_createMessage.constEnd(); ++it)
            map_createMessage.insert(it.key(), it.value());
        result._createMessage = map_createMessage;
    }
    return result;
}

QJsonObject toJson(const ClientCapabilities::Tasks::Requests::Sampling &data)
{
    QJsonObject obj;
    if (data._createMessage.has_value()) {
        QJsonObject map_createMessage;
        for (auto it = data._createMessage->constBegin(); it != data._createMessage->constEnd(); ++it)
            map_createMessage.insert(it.key(), it.value());
        obj.insert("createMessage", map_createMessage);
    }
    return obj;
}

template<>
Utils::Result<ClientCapabilities::Tasks::Requests> fromJson<ClientCapabilities::Tasks::Requests>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Requests");
    const QJsonObject obj = val.toObject();
    ClientCapabilities::Tasks::Requests result;
    if (obj.contains("elicitation") && obj["elicitation"].isObject()) {
        const auto res0 = fromJson<ClientCapabilities::Tasks::Requests::Elicitation>(obj["elicitation"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._elicitation = *res0;
    }
    if (obj.contains("sampling") && obj["sampling"].isObject()) {
        const auto res1 = fromJson<ClientCapabilities::Tasks::Requests::Sampling>(obj["sampling"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._sampling = *res1;
    }
    return result;
}

QJsonObject toJson(const ClientCapabilities::Tasks::Requests &data)
{
    QJsonObject obj;
    if (data._elicitation.has_value())
        obj.insert("elicitation", toJson(*data._elicitation));
    if (data._sampling.has_value())
        obj.insert("sampling", toJson(*data._sampling));
    return obj;
}

template<>
Utils::Result<ClientCapabilities::Tasks> fromJson<ClientCapabilities::Tasks>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Tasks");
    const QJsonObject obj = val.toObject();
    ClientCapabilities::Tasks result;
    if (obj.contains("cancel") && obj["cancel"].isObject()) {
        const QJsonObject mapObj_cancel = obj["cancel"].toObject();
        QMap<QString, QJsonValue> map_cancel;
        for (auto it = mapObj_cancel.constBegin(); it != mapObj_cancel.constEnd(); ++it)
            map_cancel.insert(it.key(), it.value());
        result._cancel = map_cancel;
    }
    if (obj.contains("list") && obj["list"].isObject()) {
        const QJsonObject mapObj_list = obj["list"].toObject();
        QMap<QString, QJsonValue> map_list;
        for (auto it = mapObj_list.constBegin(); it != mapObj_list.constEnd(); ++it)
            map_list.insert(it.key(), it.value());
        result._list = map_list;
    }
    if (obj.contains("requests") && obj["requests"].isObject()) {
        const auto res0 = fromJson<ClientCapabilities::Tasks::Requests>(obj["requests"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._requests = *res0;
    }
    return result;
}

QJsonObject toJson(const ClientCapabilities::Tasks &data)
{
    QJsonObject obj;
    if (data._cancel.has_value()) {
        QJsonObject map_cancel;
        for (auto it = data._cancel->constBegin(); it != data._cancel->constEnd(); ++it)
            map_cancel.insert(it.key(), it.value());
        obj.insert("cancel", map_cancel);
    }
    if (data._list.has_value()) {
        QJsonObject map_list;
        for (auto it = data._list->constBegin(); it != data._list->constEnd(); ++it)
            map_list.insert(it.key(), it.value());
        obj.insert("list", map_list);
    }
    if (data._requests.has_value())
        obj.insert("requests", toJson(*data._requests));
    return obj;
}

template<>
Utils::Result<ClientCapabilities> fromJson<ClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ClientCapabilities");
    const QJsonObject obj = val.toObject();
    ClientCapabilities result;
    if (obj.contains("elicitation") && obj["elicitation"].isObject()) {
        const auto res0 = fromJson<ClientCapabilities::Elicitation>(obj["elicitation"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._elicitation = *res0;
    }
    if (obj.contains("experimental") && obj["experimental"].isObject()) {
        const QJsonObject mapObj_experimental = obj["experimental"].toObject();
        QMap<QString, QJsonObject> map_experimental;
        for (auto it = mapObj_experimental.constBegin(); it != mapObj_experimental.constEnd(); ++it)
            map_experimental.insert(it.key(), it.value().toObject());
        result._experimental = map_experimental;
    }
    if (obj.contains("roots") && obj["roots"].isObject()) {
        const auto res1 = fromJson<ClientCapabilities::Roots>(obj["roots"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._roots = *res1;
    }
    if (obj.contains("sampling") && obj["sampling"].isObject()) {
        const auto res2 = fromJson<ClientCapabilities::Sampling>(obj["sampling"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._sampling = *res2;
    }
    if (obj.contains("tasks") && obj["tasks"].isObject()) {
        const auto res3 = fromJson<ClientCapabilities::Tasks>(obj["tasks"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._tasks = *res3;
    }
    return result;
}

QJsonObject toJson(const ClientCapabilities &data)
{
    QJsonObject obj;
    if (data._elicitation.has_value())
        obj.insert("elicitation", toJson(*data._elicitation));
    if (data._experimental.has_value()) {
        QJsonObject map_experimental;
        for (auto it = data._experimental->constBegin(); it != data._experimental->constEnd(); ++it)
            map_experimental.insert(it.key(), QJsonValue(it.value()));
        obj.insert("experimental", map_experimental);
    }
    if (data._roots.has_value())
        obj.insert("roots", toJson(*data._roots));
    if (data._sampling.has_value())
        obj.insert("sampling", toJson(*data._sampling));
    if (data._tasks.has_value())
        obj.insert("tasks", toJson(*data._tasks));
    return obj;
}

template<>
Utils::Result<NotificationParams> fromJson<NotificationParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for NotificationParams");
    const QJsonObject obj = val.toObject();
    NotificationParams result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const QJsonObject mapObj__meta = obj["_meta"].toObject();
        QMap<QString, QJsonValue> map__meta;
        for (auto it = mapObj__meta.constBegin(); it != mapObj__meta.constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        result.__meta = map__meta;
    }
    return result;
}

QJsonObject toJson(const NotificationParams &data)
{
    QJsonObject obj;
    if (data.__meta.has_value()) {
        QJsonObject map__meta;
        for (auto it = data.__meta->constBegin(); it != data.__meta->constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        obj.insert("_meta", map__meta);
    }
    return obj;
}

template<>
Utils::Result<InitializedNotification> fromJson<InitializedNotification>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InitializedNotification");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    InitializedNotification result;
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "notifications/initialized")
        return Utils::ResultError("Field 'method' must be 'notifications/initialized', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res0 = fromJson<NotificationParams>(obj["params"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._params = *res0;
    }
    return result;
}

QJsonObject toJson(const InitializedNotification &data)
{
    QJsonObject obj{
        {"jsonrpc", QString("2.0")},
        {"method", QString("notifications/initialized")}
    };
    if (data._params.has_value())
        obj.insert("params", toJson(*data._params));
    return obj;
}

template<>
Utils::Result<ProgressNotificationParams> fromJson<ProgressNotificationParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ProgressNotificationParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("progress"))
        return Utils::ResultError("Missing required field: progress");
    if (!obj.contains("progressToken"))
        return Utils::ResultError("Missing required field: progressToken");
    ProgressNotificationParams result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const QJsonObject mapObj__meta = obj["_meta"].toObject();
        QMap<QString, QJsonValue> map__meta;
        for (auto it = mapObj__meta.constBegin(); it != mapObj__meta.constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        result.__meta = map__meta;
    }
    if (obj.contains("message"))
        result._message = obj.value("message").toString();
    result._progress = obj.value("progress").toDouble();
    if (obj.contains("progressToken")) {
        const auto res0 = fromJson<ProgressToken>(obj["progressToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._progressToken = *res0;
    }
    if (obj.contains("total"))
        result._total = obj.value("total").toDouble();
    return result;
}

QJsonObject toJson(const ProgressNotificationParams &data)
{
    QJsonObject obj{
        {"progress", data._progress},
        {"progressToken", toJsonValue(data._progressToken)}
    };
    if (data.__meta.has_value()) {
        QJsonObject map__meta;
        for (auto it = data.__meta->constBegin(); it != data.__meta->constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        obj.insert("_meta", map__meta);
    }
    if (data._message.has_value())
        obj.insert("message", *data._message);
    if (data._total.has_value())
        obj.insert("total", *data._total);
    return obj;
}

template<>
Utils::Result<ProgressNotification> fromJson<ProgressNotification>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ProgressNotification");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    if (!obj.contains("params"))
        return Utils::ResultError("Missing required field: params");
    ProgressNotification result;
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "notifications/progress")
        return Utils::ResultError("Field 'method' must be 'notifications/progress', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res0 = fromJson<ProgressNotificationParams>(obj["params"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._params = *res0;
    }
    return result;
}

QJsonObject toJson(const ProgressNotification &data)
{
    QJsonObject obj{
        {"jsonrpc", QString("2.0")},
        {"method", QString("notifications/progress")},
        {"params", toJson(data._params)}
    };
    return obj;
}

template<>
Utils::Result<RootsListChangedNotification> fromJson<RootsListChangedNotification>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for RootsListChangedNotification");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    RootsListChangedNotification result;
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "notifications/roots/list_changed")
        return Utils::ResultError("Field 'method' must be 'notifications/roots/list_changed', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res0 = fromJson<NotificationParams>(obj["params"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._params = *res0;
    }
    return result;
}

QJsonObject toJson(const RootsListChangedNotification &data)
{
    QJsonObject obj{
        {"jsonrpc", QString("2.0")},
        {"method", QString("notifications/roots/list_changed")}
    };
    if (data._params.has_value())
        obj.insert("params", toJson(*data._params));
    return obj;
}

template<>
Utils::Result<TaskStatusNotificationParams> fromJson<TaskStatusNotificationParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TaskStatusNotificationParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("createdAt"))
        return Utils::ResultError("Missing required field: createdAt");
    if (!obj.contains("lastUpdatedAt"))
        return Utils::ResultError("Missing required field: lastUpdatedAt");
    if (!obj.contains("status"))
        return Utils::ResultError("Missing required field: status");
    if (!obj.contains("taskId"))
        return Utils::ResultError("Missing required field: taskId");
    if (!obj.contains("ttl"))
        return Utils::ResultError("Missing required field: ttl");
    TaskStatusNotificationParams result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const QJsonObject mapObj__meta = obj["_meta"].toObject();
        QMap<QString, QJsonValue> map__meta;
        for (auto it = mapObj__meta.constBegin(); it != mapObj__meta.constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        result.__meta = map__meta;
    }
    result._createdAt = obj.value("createdAt").toString();
    result._lastUpdatedAt = obj.value("lastUpdatedAt").toString();
    if (obj.contains("pollInterval"))
        result._pollInterval = obj.value("pollInterval").toInt();
    if (obj.contains("status") && obj["status"].isString()) {
        const auto res0 = fromJson<TaskStatus>(obj["status"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._status = *res0;
    }
    if (obj.contains("statusMessage"))
        result._statusMessage = obj.value("statusMessage").toString();
    result._taskId = obj.value("taskId").toString();
    if (!obj["ttl"].isNull()) {
        result._ttl = obj.value("ttl").toInt();
    }
    return result;
}

QJsonObject toJson(const TaskStatusNotificationParams &data)
{
    QJsonObject obj{
        {"createdAt", data._createdAt},
        {"lastUpdatedAt", data._lastUpdatedAt},
        {"status", toJsonValue(data._status)},
        {"taskId", data._taskId}
    };
    if (data.__meta.has_value()) {
        QJsonObject map__meta;
        for (auto it = data.__meta->constBegin(); it != data.__meta->constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        obj.insert("_meta", map__meta);
    }
    if (data._pollInterval.has_value())
        obj.insert("pollInterval", *data._pollInterval);
    if (data._statusMessage.has_value())
        obj.insert("statusMessage", *data._statusMessage);
    if (data._ttl.has_value())
        obj.insert("ttl", *data._ttl);
    else
        obj.insert("ttl", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<TaskStatusNotification> fromJson<TaskStatusNotification>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TaskStatusNotification");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    if (!obj.contains("params"))
        return Utils::ResultError("Missing required field: params");
    TaskStatusNotification result;
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "notifications/tasks/status")
        return Utils::ResultError("Field 'method' must be 'notifications/tasks/status', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res0 = fromJson<TaskStatusNotificationParams>(obj["params"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._params = *res0;
    }
    return result;
}

QJsonObject toJson(const TaskStatusNotification &data)
{
    QJsonObject obj{
        {"jsonrpc", QString("2.0")},
        {"method", QString("notifications/tasks/status")},
        {"params", toJson(data._params)}
    };
    return obj;
}

template<>
Utils::Result<ClientNotification> fromJson<ClientNotification>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid ClientNotification: expected object");
    const QString dispatchValue = val.toObject().value("method").toString();
    if (dispatchValue == "notifications/cancelled") {
        const auto res0 = fromJson<CancelledNotification>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return ClientNotification(*res0);
    }
    else if (dispatchValue == "notifications/initialized") {
        const auto res1 = fromJson<InitializedNotification>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        return ClientNotification(*res1);
    }
    else if (dispatchValue == "notifications/progress") {
        const auto res2 = fromJson<ProgressNotification>(val);
        if (!res2)
            return Utils::ResultError(res2.error());
        return ClientNotification(*res2);
    }
    else if (dispatchValue == "notifications/tasks/status") {
        const auto res3 = fromJson<TaskStatusNotification>(val);
        if (!res3)
            return Utils::ResultError(res3.error());
        return ClientNotification(*res3);
    }
    else if (dispatchValue == "notifications/roots/list_changed") {
        const auto res4 = fromJson<RootsListChangedNotification>(val);
        if (!res4)
            return Utils::ResultError(res4.error());
        return ClientNotification(*res4);
    }
    return Utils::ResultError("Invalid ClientNotification: unknown method \"" + dispatchValue + "\"");
}

QJsonObject toJson(const ClientNotification &val)
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

QJsonValue toJsonValue(const ClientNotification &val)
{
    return toJson(val);
}

QString dispatchValue(const ClientNotification &val)
{
    return std::visit([](const auto &v) -> QString {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, CancelledNotification>) return "notifications/cancelled";
        else if constexpr (std::is_same_v<T, InitializedNotification>) return "notifications/initialized";
        else if constexpr (std::is_same_v<T, ProgressNotification>) return "notifications/progress";
        else if constexpr (std::is_same_v<T, TaskStatusNotification>) return "notifications/tasks/status";
        else if constexpr (std::is_same_v<T, RootsListChangedNotification>) return "notifications/roots/list_changed";
        return {};
    }, val);
}

template<>
Utils::Result<PromptReference> fromJson<PromptReference>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for PromptReference");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    if (!obj.contains("type"))
        return Utils::ResultError("Missing required field: type");
    PromptReference result;
    result._name = obj.value("name").toString();
    if (obj.contains("title"))
        result._title = obj.value("title").toString();
    if (obj.value("type").toString() != "ref/prompt")
        return Utils::ResultError("Field 'type' must be 'ref/prompt', got: " + obj.value("type").toString());
    return result;
}

QJsonObject toJson(const PromptReference &data)
{
    QJsonObject obj{
        {"name", data._name},
        {"type", QString("ref/prompt")}
    };
    if (data._title.has_value())
        obj.insert("title", *data._title);
    return obj;
}

template<>
Utils::Result<ResourceTemplateReference> fromJson<ResourceTemplateReference>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ResourceTemplateReference");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("type"))
        return Utils::ResultError("Missing required field: type");
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    ResourceTemplateReference result;
    if (obj.value("type").toString() != "ref/resource")
        return Utils::ResultError("Field 'type' must be 'ref/resource', got: " + obj.value("type").toString());
    result._uri = obj.value("uri").toString();
    return result;
}

QJsonObject toJson(const ResourceTemplateReference &data)
{
    QJsonObject obj{
        {"type", QString("ref/resource")},
        {"uri", data._uri}
    };
    return obj;
}

template<>
Utils::Result<CompleteRequestParamsRef> fromJson<CompleteRequestParamsRef>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid CompleteRequestParamsRef: expected object or array");
    const QString dispatchValue = val.toObject().value("type").toString();
    if (dispatchValue == "ref/prompt") {
        const auto res0 = fromJson<PromptReference>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return CompleteRequestParamsRef(*res0);
    }
    else if (dispatchValue == "ref/resource") {
        const auto res1 = fromJson<ResourceTemplateReference>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        return CompleteRequestParamsRef(*res1);
    }
    return Utils::ResultError("Invalid CompleteRequestParamsRef: unknown type \"" + dispatchValue + "\"");
}

QJsonValue toJsonValue(const CompleteRequestParamsRef &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
}

template<>
Utils::Result<CompleteRequestParams::Meta> fromJson<CompleteRequestParams::Meta>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Meta");
    const QJsonObject obj = val.toObject();
    CompleteRequestParams::Meta result;
    if (obj.contains("progressToken")) {
        const auto res0 = fromJson<ProgressToken>(obj["progressToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._progressToken = *res0;
    }
    return result;
}

QJsonObject toJson(const CompleteRequestParams::Meta &data)
{
    QJsonObject obj;
    if (data._progressToken.has_value())
        obj.insert("progressToken", toJsonValue(*data._progressToken));
    return obj;
}

template<>
Utils::Result<CompleteRequestParams::Argument> fromJson<CompleteRequestParams::Argument>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Argument");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    if (!obj.contains("value"))
        return Utils::ResultError("Missing required field: value");
    CompleteRequestParams::Argument result;
    result._name = obj.value("name").toString();
    result._value = obj.value("value").toString();
    return result;
}

QJsonObject toJson(const CompleteRequestParams::Argument &data)
{
    QJsonObject obj{
        {"name", data._name},
        {"value", data._value}
    };
    return obj;
}

template<>
Utils::Result<CompleteRequestParams::Context> fromJson<CompleteRequestParams::Context>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Context");
    const QJsonObject obj = val.toObject();
    CompleteRequestParams::Context result;
    if (obj.contains("arguments") && obj["arguments"].isObject()) {
        const QJsonObject mapObj_arguments = obj["arguments"].toObject();
        QMap<QString, QString> map_arguments;
        for (auto it = mapObj_arguments.constBegin(); it != mapObj_arguments.constEnd(); ++it)
            map_arguments.insert(it.key(), it.value().toString());
        result._arguments = map_arguments;
    }
    return result;
}

QJsonObject toJson(const CompleteRequestParams::Context &data)
{
    QJsonObject obj;
    if (data._arguments.has_value()) {
        QJsonObject map_arguments;
        for (auto it = data._arguments->constBegin(); it != data._arguments->constEnd(); ++it)
            map_arguments.insert(it.key(), QJsonValue(it.value()));
        obj.insert("arguments", map_arguments);
    }
    return obj;
}

template<>
Utils::Result<CompleteRequestParams> fromJson<CompleteRequestParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CompleteRequestParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("argument"))
        return Utils::ResultError("Missing required field: argument");
    if (!obj.contains("ref"))
        return Utils::ResultError("Missing required field: ref");
    CompleteRequestParams result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<CompleteRequestParams::Meta>(obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    if (obj.contains("argument") && obj["argument"].isObject()) {
        const auto res1 = fromJson<CompleteRequestParams::Argument>(obj["argument"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._argument = *res1;
    }
    if (obj.contains("context") && obj["context"].isObject()) {
        const auto res2 = fromJson<CompleteRequestParams::Context>(obj["context"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._context = *res2;
    }
    if (obj.contains("ref")) {
        const auto res3 = fromJson<CompleteRequestParamsRef>(obj["ref"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._ref = *res3;
    }
    return result;
}

QJsonObject toJson(const CompleteRequestParams &data)
{
    QJsonObject obj{
        {"argument", toJson(data._argument)},
        {"ref", toJsonValue(data._ref)}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
    if (data._context.has_value())
        obj.insert("context", toJson(*data._context));
    return obj;
}

template<>
Utils::Result<CompleteRequest> fromJson<CompleteRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CompleteRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    if (!obj.contains("params"))
        return Utils::ResultError("Missing required field: params");
    CompleteRequest result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>(obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "completion/complete")
        return Utils::ResultError("Field 'method' must be 'completion/complete', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res1 = fromJson<CompleteRequestParams>(obj["params"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._params = *res1;
    }
    return result;
}

QJsonObject toJson(const CompleteRequest &data)
{
    QJsonObject obj{
        {"id", toJsonValue(data._id)},
        {"jsonrpc", QString("2.0")},
        {"method", QString("completion/complete")},
        {"params", toJson(data._params)}
    };
    return obj;
}

template<>
Utils::Result<GetPromptRequestParams::Meta> fromJson<GetPromptRequestParams::Meta>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Meta");
    const QJsonObject obj = val.toObject();
    GetPromptRequestParams::Meta result;
    if (obj.contains("progressToken")) {
        const auto res0 = fromJson<ProgressToken>(obj["progressToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._progressToken = *res0;
    }
    return result;
}

QJsonObject toJson(const GetPromptRequestParams::Meta &data)
{
    QJsonObject obj;
    if (data._progressToken.has_value())
        obj.insert("progressToken", toJsonValue(*data._progressToken));
    return obj;
}

template<>
Utils::Result<GetPromptRequestParams> fromJson<GetPromptRequestParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for GetPromptRequestParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    GetPromptRequestParams result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<GetPromptRequestParams::Meta>(obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    if (obj.contains("arguments") && obj["arguments"].isObject()) {
        const QJsonObject mapObj_arguments = obj["arguments"].toObject();
        QMap<QString, QString> map_arguments;
        for (auto it = mapObj_arguments.constBegin(); it != mapObj_arguments.constEnd(); ++it)
            map_arguments.insert(it.key(), it.value().toString());
        result._arguments = map_arguments;
    }
    result._name = obj.value("name").toString();
    return result;
}

QJsonObject toJson(const GetPromptRequestParams &data)
{
    QJsonObject obj{{"name", data._name}};
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
    if (data._arguments.has_value()) {
        QJsonObject map_arguments;
        for (auto it = data._arguments->constBegin(); it != data._arguments->constEnd(); ++it)
            map_arguments.insert(it.key(), QJsonValue(it.value()));
        obj.insert("arguments", map_arguments);
    }
    return obj;
}

template<>
Utils::Result<GetPromptRequest> fromJson<GetPromptRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for GetPromptRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    if (!obj.contains("params"))
        return Utils::ResultError("Missing required field: params");
    GetPromptRequest result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>(obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "prompts/get")
        return Utils::ResultError("Field 'method' must be 'prompts/get', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res1 = fromJson<GetPromptRequestParams>(obj["params"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._params = *res1;
    }
    return result;
}

QJsonObject toJson(const GetPromptRequest &data)
{
    QJsonObject obj{
        {"id", toJsonValue(data._id)},
        {"jsonrpc", QString("2.0")},
        {"method", QString("prompts/get")},
        {"params", toJson(data._params)}
    };
    return obj;
}

template<>
Utils::Result<GetTaskPayloadRequest::Params> fromJson<GetTaskPayloadRequest::Params>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Params");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("taskId"))
        return Utils::ResultError("Missing required field: taskId");
    GetTaskPayloadRequest::Params result;
    result._taskId = obj.value("taskId").toString();
    return result;
}

QJsonObject toJson(const GetTaskPayloadRequest::Params &data)
{
    QJsonObject obj{{"taskId", data._taskId}};
    return obj;
}

template<>
Utils::Result<GetTaskPayloadRequest> fromJson<GetTaskPayloadRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for GetTaskPayloadRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    if (!obj.contains("params"))
        return Utils::ResultError("Missing required field: params");
    GetTaskPayloadRequest result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>(obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "tasks/result")
        return Utils::ResultError("Field 'method' must be 'tasks/result', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res1 = fromJson<GetTaskPayloadRequest::Params>(obj["params"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._params = *res1;
    }
    return result;
}

QJsonObject toJson(const GetTaskPayloadRequest &data)
{
    QJsonObject obj{
        {"id", toJsonValue(data._id)},
        {"jsonrpc", QString("2.0")},
        {"method", QString("tasks/result")},
        {"params", toJson(data._params)}
    };
    return obj;
}

template<>
Utils::Result<GetTaskRequest::Params> fromJson<GetTaskRequest::Params>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Params");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("taskId"))
        return Utils::ResultError("Missing required field: taskId");
    GetTaskRequest::Params result;
    result._taskId = obj.value("taskId").toString();
    return result;
}

QJsonObject toJson(const GetTaskRequest::Params &data)
{
    QJsonObject obj{{"taskId", data._taskId}};
    return obj;
}

template<>
Utils::Result<GetTaskRequest> fromJson<GetTaskRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for GetTaskRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    if (!obj.contains("params"))
        return Utils::ResultError("Missing required field: params");
    GetTaskRequest result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>(obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "tasks/get")
        return Utils::ResultError("Field 'method' must be 'tasks/get', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res1 = fromJson<GetTaskRequest::Params>(obj["params"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._params = *res1;
    }
    return result;
}

QJsonObject toJson(const GetTaskRequest &data)
{
    QJsonObject obj{
        {"id", toJsonValue(data._id)},
        {"jsonrpc", QString("2.0")},
        {"method", QString("tasks/get")},
        {"params", toJson(data._params)}
    };
    return obj;
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
    if (obj.contains("description"))
        result._description = obj.value("description").toString();
    if (obj.contains("icons") && obj["icons"].isArray()) {
        const QJsonArray arr = obj["icons"].toArray();
        QList<Icon> list_icons;
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<Icon>(v);
            if (!res0)
                return Utils::ResultError(res0.error());
            list_icons.append(*res0);
        }
        result._icons = list_icons;
    }
    result._name = obj.value("name").toString();
    if (obj.contains("title"))
        result._title = obj.value("title").toString();
    result._version = obj.value("version").toString();
    if (obj.contains("websiteUrl"))
        result._websiteUrl = obj.value("websiteUrl").toString();
    return result;
}

QJsonObject toJson(const Implementation &data)
{
    QJsonObject obj{
        {"name", data._name},
        {"version", data._version}
    };
    if (data._description.has_value())
        obj.insert("description", *data._description);
    if (data._icons.has_value()) {
        QJsonArray arr_icons;
        for (const auto &v : *data._icons) arr_icons.append(toJson(v));
        obj.insert("icons", arr_icons);
    }
    if (data._title.has_value())
        obj.insert("title", *data._title);
    if (data._websiteUrl.has_value())
        obj.insert("websiteUrl", *data._websiteUrl);
    return obj;
}

template<>
Utils::Result<InitializeRequestParams::Meta> fromJson<InitializeRequestParams::Meta>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Meta");
    const QJsonObject obj = val.toObject();
    InitializeRequestParams::Meta result;
    if (obj.contains("progressToken")) {
        const auto res0 = fromJson<ProgressToken>(obj["progressToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._progressToken = *res0;
    }
    return result;
}

QJsonObject toJson(const InitializeRequestParams::Meta &data)
{
    QJsonObject obj;
    if (data._progressToken.has_value())
        obj.insert("progressToken", toJsonValue(*data._progressToken));
    return obj;
}

template<>
Utils::Result<InitializeRequestParams> fromJson<InitializeRequestParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InitializeRequestParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("capabilities"))
        return Utils::ResultError("Missing required field: capabilities");
    if (!obj.contains("clientInfo"))
        return Utils::ResultError("Missing required field: clientInfo");
    if (!obj.contains("protocolVersion"))
        return Utils::ResultError("Missing required field: protocolVersion");
    InitializeRequestParams result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<InitializeRequestParams::Meta>(obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    if (obj.contains("capabilities") && obj["capabilities"].isObject()) {
        const auto res1 = fromJson<ClientCapabilities>(obj["capabilities"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._capabilities = *res1;
    }
    if (obj.contains("clientInfo") && obj["clientInfo"].isObject()) {
        const auto res2 = fromJson<Implementation>(obj["clientInfo"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._clientInfo = *res2;
    }
    result._protocolVersion = obj.value("protocolVersion").toString();
    return result;
}

QJsonObject toJson(const InitializeRequestParams &data)
{
    QJsonObject obj{
        {"capabilities", toJson(data._capabilities)},
        {"clientInfo", toJson(data._clientInfo)},
        {"protocolVersion", data._protocolVersion}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
    return obj;
}

template<>
Utils::Result<InitializeRequest> fromJson<InitializeRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InitializeRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    if (!obj.contains("params"))
        return Utils::ResultError("Missing required field: params");
    InitializeRequest result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>(obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "initialize")
        return Utils::ResultError("Field 'method' must be 'initialize', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res1 = fromJson<InitializeRequestParams>(obj["params"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._params = *res1;
    }
    return result;
}

QJsonObject toJson(const InitializeRequest &data)
{
    QJsonObject obj{
        {"id", toJsonValue(data._id)},
        {"jsonrpc", QString("2.0")},
        {"method", QString("initialize")},
        {"params", toJson(data._params)}
    };
    return obj;
}

template<>
Utils::Result<PaginatedRequestParams::Meta> fromJson<PaginatedRequestParams::Meta>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Meta");
    const QJsonObject obj = val.toObject();
    PaginatedRequestParams::Meta result;
    if (obj.contains("progressToken")) {
        const auto res0 = fromJson<ProgressToken>(obj["progressToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._progressToken = *res0;
    }
    return result;
}

QJsonObject toJson(const PaginatedRequestParams::Meta &data)
{
    QJsonObject obj;
    if (data._progressToken.has_value())
        obj.insert("progressToken", toJsonValue(*data._progressToken));
    return obj;
}

template<>
Utils::Result<PaginatedRequestParams> fromJson<PaginatedRequestParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for PaginatedRequestParams");
    const QJsonObject obj = val.toObject();
    PaginatedRequestParams result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<PaginatedRequestParams::Meta>(obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    if (obj.contains("cursor"))
        result._cursor = obj.value("cursor").toString();
    return result;
}

QJsonObject toJson(const PaginatedRequestParams &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
    if (data._cursor.has_value())
        obj.insert("cursor", *data._cursor);
    return obj;
}

template<>
Utils::Result<ListPromptsRequest> fromJson<ListPromptsRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ListPromptsRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    ListPromptsRequest result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>(obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "prompts/list")
        return Utils::ResultError("Field 'method' must be 'prompts/list', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res1 = fromJson<PaginatedRequestParams>(obj["params"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._params = *res1;
    }
    return result;
}

QJsonObject toJson(const ListPromptsRequest &data)
{
    QJsonObject obj{
        {"id", toJsonValue(data._id)},
        {"jsonrpc", QString("2.0")},
        {"method", QString("prompts/list")}
    };
    if (data._params.has_value())
        obj.insert("params", toJson(*data._params));
    return obj;
}

template<>
Utils::Result<ListResourceTemplatesRequest> fromJson<ListResourceTemplatesRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ListResourceTemplatesRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    ListResourceTemplatesRequest result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>(obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "resources/templates/list")
        return Utils::ResultError("Field 'method' must be 'resources/templates/list', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res1 = fromJson<PaginatedRequestParams>(obj["params"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._params = *res1;
    }
    return result;
}

QJsonObject toJson(const ListResourceTemplatesRequest &data)
{
    QJsonObject obj{
        {"id", toJsonValue(data._id)},
        {"jsonrpc", QString("2.0")},
        {"method", QString("resources/templates/list")}
    };
    if (data._params.has_value())
        obj.insert("params", toJson(*data._params));
    return obj;
}

template<>
Utils::Result<ListResourcesRequest> fromJson<ListResourcesRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ListResourcesRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    ListResourcesRequest result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>(obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "resources/list")
        return Utils::ResultError("Field 'method' must be 'resources/list', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res1 = fromJson<PaginatedRequestParams>(obj["params"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._params = *res1;
    }
    return result;
}

QJsonObject toJson(const ListResourcesRequest &data)
{
    QJsonObject obj{
        {"id", toJsonValue(data._id)},
        {"jsonrpc", QString("2.0")},
        {"method", QString("resources/list")}
    };
    if (data._params.has_value())
        obj.insert("params", toJson(*data._params));
    return obj;
}

template<>
Utils::Result<ListTasksRequest> fromJson<ListTasksRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ListTasksRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    ListTasksRequest result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>(obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "tasks/list")
        return Utils::ResultError("Field 'method' must be 'tasks/list', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res1 = fromJson<PaginatedRequestParams>(obj["params"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._params = *res1;
    }
    return result;
}

QJsonObject toJson(const ListTasksRequest &data)
{
    QJsonObject obj{
        {"id", toJsonValue(data._id)},
        {"jsonrpc", QString("2.0")},
        {"method", QString("tasks/list")}
    };
    if (data._params.has_value())
        obj.insert("params", toJson(*data._params));
    return obj;
}

template<>
Utils::Result<ListToolsRequest> fromJson<ListToolsRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ListToolsRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    ListToolsRequest result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>(obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "tools/list")
        return Utils::ResultError("Field 'method' must be 'tools/list', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res1 = fromJson<PaginatedRequestParams>(obj["params"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._params = *res1;
    }
    return result;
}

QJsonObject toJson(const ListToolsRequest &data)
{
    QJsonObject obj{
        {"id", toJsonValue(data._id)},
        {"jsonrpc", QString("2.0")},
        {"method", QString("tools/list")}
    };
    if (data._params.has_value())
        obj.insert("params", toJson(*data._params));
    return obj;
}

template<>
Utils::Result<RequestParams::Meta> fromJson<RequestParams::Meta>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Meta");
    const QJsonObject obj = val.toObject();
    RequestParams::Meta result;
    if (obj.contains("progressToken")) {
        const auto res0 = fromJson<ProgressToken>(obj["progressToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._progressToken = *res0;
    }
    return result;
}

QJsonObject toJson(const RequestParams::Meta &data)
{
    QJsonObject obj;
    if (data._progressToken.has_value())
        obj.insert("progressToken", toJsonValue(*data._progressToken));
    return obj;
}

template<>
Utils::Result<RequestParams> fromJson<RequestParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for RequestParams");
    const QJsonObject obj = val.toObject();
    RequestParams result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<RequestParams::Meta>(obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    return result;
}

QJsonObject toJson(const RequestParams &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
    return obj;
}

template<>
Utils::Result<PingRequest> fromJson<PingRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for PingRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    PingRequest result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>(obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "ping")
        return Utils::ResultError("Field 'method' must be 'ping', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res1 = fromJson<RequestParams>(obj["params"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._params = *res1;
    }
    return result;
}

QJsonObject toJson(const PingRequest &data)
{
    QJsonObject obj{
        {"id", toJsonValue(data._id)},
        {"jsonrpc", QString("2.0")},
        {"method", QString("ping")}
    };
    if (data._params.has_value())
        obj.insert("params", toJson(*data._params));
    return obj;
}

template<>
Utils::Result<ReadResourceRequestParams::Meta> fromJson<ReadResourceRequestParams::Meta>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Meta");
    const QJsonObject obj = val.toObject();
    ReadResourceRequestParams::Meta result;
    if (obj.contains("progressToken")) {
        const auto res0 = fromJson<ProgressToken>(obj["progressToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._progressToken = *res0;
    }
    return result;
}

QJsonObject toJson(const ReadResourceRequestParams::Meta &data)
{
    QJsonObject obj;
    if (data._progressToken.has_value())
        obj.insert("progressToken", toJsonValue(*data._progressToken));
    return obj;
}

template<>
Utils::Result<ReadResourceRequestParams> fromJson<ReadResourceRequestParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ReadResourceRequestParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    ReadResourceRequestParams result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<ReadResourceRequestParams::Meta>(obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    result._uri = obj.value("uri").toString();
    return result;
}

QJsonObject toJson(const ReadResourceRequestParams &data)
{
    QJsonObject obj{{"uri", data._uri}};
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
    return obj;
}

template<>
Utils::Result<ReadResourceRequest> fromJson<ReadResourceRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ReadResourceRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    if (!obj.contains("params"))
        return Utils::ResultError("Missing required field: params");
    ReadResourceRequest result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>(obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "resources/read")
        return Utils::ResultError("Field 'method' must be 'resources/read', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res1 = fromJson<ReadResourceRequestParams>(obj["params"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._params = *res1;
    }
    return result;
}

QJsonObject toJson(const ReadResourceRequest &data)
{
    QJsonObject obj{
        {"id", toJsonValue(data._id)},
        {"jsonrpc", QString("2.0")},
        {"method", QString("resources/read")},
        {"params", toJson(data._params)}
    };
    return obj;
}

QString toString(LoggingLevel v)
{
    switch(v) {
        case LoggingLevel::alert: return "alert";
        case LoggingLevel::critical: return "critical";
        case LoggingLevel::debug: return "debug";
        case LoggingLevel::emergency: return "emergency";
        case LoggingLevel::error: return "error";
        case LoggingLevel::info: return "info";
        case LoggingLevel::notice: return "notice";
        case LoggingLevel::warning: return "warning";
    }
    return {};
}

template<>
Utils::Result<LoggingLevel> fromJson<LoggingLevel>(const QJsonValue &val)
{
    const QString str = val.toString();
    if (str == "alert") return LoggingLevel::alert;
    if (str == "critical") return LoggingLevel::critical;
    if (str == "debug") return LoggingLevel::debug;
    if (str == "emergency") return LoggingLevel::emergency;
    if (str == "error") return LoggingLevel::error;
    if (str == "info") return LoggingLevel::info;
    if (str == "notice") return LoggingLevel::notice;
    if (str == "warning") return LoggingLevel::warning;
    return Utils::ResultError("Invalid LoggingLevel value: " + str);
}

QJsonValue toJsonValue(const LoggingLevel &v)
{
    return toString(v);
}

template<>
Utils::Result<SetLevelRequestParams::Meta> fromJson<SetLevelRequestParams::Meta>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Meta");
    const QJsonObject obj = val.toObject();
    SetLevelRequestParams::Meta result;
    if (obj.contains("progressToken")) {
        const auto res0 = fromJson<ProgressToken>(obj["progressToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._progressToken = *res0;
    }
    return result;
}

QJsonObject toJson(const SetLevelRequestParams::Meta &data)
{
    QJsonObject obj;
    if (data._progressToken.has_value())
        obj.insert("progressToken", toJsonValue(*data._progressToken));
    return obj;
}

template<>
Utils::Result<SetLevelRequestParams> fromJson<SetLevelRequestParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SetLevelRequestParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("level"))
        return Utils::ResultError("Missing required field: level");
    SetLevelRequestParams result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<SetLevelRequestParams::Meta>(obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    if (obj.contains("level") && obj["level"].isString()) {
        const auto res1 = fromJson<LoggingLevel>(obj["level"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._level = *res1;
    }
    return result;
}

QJsonObject toJson(const SetLevelRequestParams &data)
{
    QJsonObject obj{{"level", toJsonValue(data._level)}};
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
    return obj;
}

template<>
Utils::Result<SetLevelRequest> fromJson<SetLevelRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SetLevelRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    if (!obj.contains("params"))
        return Utils::ResultError("Missing required field: params");
    SetLevelRequest result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>(obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "logging/setLevel")
        return Utils::ResultError("Field 'method' must be 'logging/setLevel', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res1 = fromJson<SetLevelRequestParams>(obj["params"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._params = *res1;
    }
    return result;
}

QJsonObject toJson(const SetLevelRequest &data)
{
    QJsonObject obj{
        {"id", toJsonValue(data._id)},
        {"jsonrpc", QString("2.0")},
        {"method", QString("logging/setLevel")},
        {"params", toJson(data._params)}
    };
    return obj;
}

template<>
Utils::Result<SubscribeRequestParams::Meta> fromJson<SubscribeRequestParams::Meta>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Meta");
    const QJsonObject obj = val.toObject();
    SubscribeRequestParams::Meta result;
    if (obj.contains("progressToken")) {
        const auto res0 = fromJson<ProgressToken>(obj["progressToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._progressToken = *res0;
    }
    return result;
}

QJsonObject toJson(const SubscribeRequestParams::Meta &data)
{
    QJsonObject obj;
    if (data._progressToken.has_value())
        obj.insert("progressToken", toJsonValue(*data._progressToken));
    return obj;
}

template<>
Utils::Result<SubscribeRequestParams> fromJson<SubscribeRequestParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SubscribeRequestParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    SubscribeRequestParams result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<SubscribeRequestParams::Meta>(obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    result._uri = obj.value("uri").toString();
    return result;
}

QJsonObject toJson(const SubscribeRequestParams &data)
{
    QJsonObject obj{{"uri", data._uri}};
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
    return obj;
}

template<>
Utils::Result<SubscribeRequest> fromJson<SubscribeRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SubscribeRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    if (!obj.contains("params"))
        return Utils::ResultError("Missing required field: params");
    SubscribeRequest result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>(obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "resources/subscribe")
        return Utils::ResultError("Field 'method' must be 'resources/subscribe', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res1 = fromJson<SubscribeRequestParams>(obj["params"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._params = *res1;
    }
    return result;
}

QJsonObject toJson(const SubscribeRequest &data)
{
    QJsonObject obj{
        {"id", toJsonValue(data._id)},
        {"jsonrpc", QString("2.0")},
        {"method", QString("resources/subscribe")},
        {"params", toJson(data._params)}
    };
    return obj;
}

template<>
Utils::Result<UnsubscribeRequestParams::Meta> fromJson<UnsubscribeRequestParams::Meta>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Meta");
    const QJsonObject obj = val.toObject();
    UnsubscribeRequestParams::Meta result;
    if (obj.contains("progressToken")) {
        const auto res0 = fromJson<ProgressToken>(obj["progressToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._progressToken = *res0;
    }
    return result;
}

QJsonObject toJson(const UnsubscribeRequestParams::Meta &data)
{
    QJsonObject obj;
    if (data._progressToken.has_value())
        obj.insert("progressToken", toJsonValue(*data._progressToken));
    return obj;
}

template<>
Utils::Result<UnsubscribeRequestParams> fromJson<UnsubscribeRequestParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for UnsubscribeRequestParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    UnsubscribeRequestParams result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<UnsubscribeRequestParams::Meta>(obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    result._uri = obj.value("uri").toString();
    return result;
}

QJsonObject toJson(const UnsubscribeRequestParams &data)
{
    QJsonObject obj{{"uri", data._uri}};
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
    return obj;
}

template<>
Utils::Result<UnsubscribeRequest> fromJson<UnsubscribeRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for UnsubscribeRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    if (!obj.contains("params"))
        return Utils::ResultError("Missing required field: params");
    UnsubscribeRequest result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>(obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "resources/unsubscribe")
        return Utils::ResultError("Field 'method' must be 'resources/unsubscribe', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res1 = fromJson<UnsubscribeRequestParams>(obj["params"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._params = *res1;
    }
    return result;
}

QJsonObject toJson(const UnsubscribeRequest &data)
{
    QJsonObject obj{
        {"id", toJsonValue(data._id)},
        {"jsonrpc", QString("2.0")},
        {"method", QString("resources/unsubscribe")},
        {"params", toJson(data._params)}
    };
    return obj;
}

template<>
Utils::Result<ClientRequest> fromJson<ClientRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid ClientRequest: expected object");
    const QString dispatchValue = val.toObject().value("method").toString();
    if (dispatchValue == "initialize") {
        const auto res0 = fromJson<InitializeRequest>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return ClientRequest(*res0);
    }
    else if (dispatchValue == "ping") {
        const auto res1 = fromJson<PingRequest>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        return ClientRequest(*res1);
    }
    else if (dispatchValue == "resources/list") {
        const auto res2 = fromJson<ListResourcesRequest>(val);
        if (!res2)
            return Utils::ResultError(res2.error());
        return ClientRequest(*res2);
    }
    else if (dispatchValue == "resources/templates/list") {
        const auto res3 = fromJson<ListResourceTemplatesRequest>(val);
        if (!res3)
            return Utils::ResultError(res3.error());
        return ClientRequest(*res3);
    }
    else if (dispatchValue == "resources/read") {
        const auto res4 = fromJson<ReadResourceRequest>(val);
        if (!res4)
            return Utils::ResultError(res4.error());
        return ClientRequest(*res4);
    }
    else if (dispatchValue == "resources/subscribe") {
        const auto res5 = fromJson<SubscribeRequest>(val);
        if (!res5)
            return Utils::ResultError(res5.error());
        return ClientRequest(*res5);
    }
    else if (dispatchValue == "resources/unsubscribe") {
        const auto res6 = fromJson<UnsubscribeRequest>(val);
        if (!res6)
            return Utils::ResultError(res6.error());
        return ClientRequest(*res6);
    }
    else if (dispatchValue == "prompts/list") {
        const auto res7 = fromJson<ListPromptsRequest>(val);
        if (!res7)
            return Utils::ResultError(res7.error());
        return ClientRequest(*res7);
    }
    else if (dispatchValue == "prompts/get") {
        const auto res8 = fromJson<GetPromptRequest>(val);
        if (!res8)
            return Utils::ResultError(res8.error());
        return ClientRequest(*res8);
    }
    else if (dispatchValue == "tools/list") {
        const auto res9 = fromJson<ListToolsRequest>(val);
        if (!res9)
            return Utils::ResultError(res9.error());
        return ClientRequest(*res9);
    }
    else if (dispatchValue == "tools/call") {
        const auto res10 = fromJson<CallToolRequest>(val);
        if (!res10)
            return Utils::ResultError(res10.error());
        return ClientRequest(*res10);
    }
    else if (dispatchValue == "tasks/get") {
        const auto res11 = fromJson<GetTaskRequest>(val);
        if (!res11)
            return Utils::ResultError(res11.error());
        return ClientRequest(*res11);
    }
    else if (dispatchValue == "tasks/result") {
        const auto res12 = fromJson<GetTaskPayloadRequest>(val);
        if (!res12)
            return Utils::ResultError(res12.error());
        return ClientRequest(*res12);
    }
    else if (dispatchValue == "tasks/cancel") {
        const auto res13 = fromJson<CancelTaskRequest>(val);
        if (!res13)
            return Utils::ResultError(res13.error());
        return ClientRequest(*res13);
    }
    else if (dispatchValue == "tasks/list") {
        const auto res14 = fromJson<ListTasksRequest>(val);
        if (!res14)
            return Utils::ResultError(res14.error());
        return ClientRequest(*res14);
    }
    else if (dispatchValue == "logging/setLevel") {
        const auto res15 = fromJson<SetLevelRequest>(val);
        if (!res15)
            return Utils::ResultError(res15.error());
        return ClientRequest(*res15);
    }
    else if (dispatchValue == "completion/complete") {
        const auto res16 = fromJson<CompleteRequest>(val);
        if (!res16)
            return Utils::ResultError(res16.error());
        return ClientRequest(*res16);
    }
    return Utils::ResultError("Invalid ClientRequest: unknown method \"" + dispatchValue + "\"");
}

QJsonObject toJson(const ClientRequest &val)
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

QJsonValue toJsonValue(const ClientRequest &val)
{
    return toJson(val);
}

QString dispatchValue(const ClientRequest &val)
{
    return std::visit([](const auto &v) -> QString {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, InitializeRequest>) return "initialize";
        else if constexpr (std::is_same_v<T, PingRequest>) return "ping";
        else if constexpr (std::is_same_v<T, ListResourcesRequest>) return "resources/list";
        else if constexpr (std::is_same_v<T, ListResourceTemplatesRequest>) return "resources/templates/list";
        else if constexpr (std::is_same_v<T, ReadResourceRequest>) return "resources/read";
        else if constexpr (std::is_same_v<T, SubscribeRequest>) return "resources/subscribe";
        else if constexpr (std::is_same_v<T, UnsubscribeRequest>) return "resources/unsubscribe";
        else if constexpr (std::is_same_v<T, ListPromptsRequest>) return "prompts/list";
        else if constexpr (std::is_same_v<T, GetPromptRequest>) return "prompts/get";
        else if constexpr (std::is_same_v<T, ListToolsRequest>) return "tools/list";
        else if constexpr (std::is_same_v<T, CallToolRequest>) return "tools/call";
        else if constexpr (std::is_same_v<T, GetTaskRequest>) return "tasks/get";
        else if constexpr (std::is_same_v<T, GetTaskPayloadRequest>) return "tasks/result";
        else if constexpr (std::is_same_v<T, CancelTaskRequest>) return "tasks/cancel";
        else if constexpr (std::is_same_v<T, ListTasksRequest>) return "tasks/list";
        else if constexpr (std::is_same_v<T, SetLevelRequest>) return "logging/setLevel";
        else if constexpr (std::is_same_v<T, CompleteRequest>) return "completion/complete";
        return {};
    }, val);
}

RequestId id(const ClientRequest &val)
{
    return std::visit([](const auto &v) -> RequestId { return v._id; }, val);
}

template<>
Utils::Result<ToolResultContent> fromJson<ToolResultContent>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ToolResultContent");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("content"))
        return Utils::ResultError("Missing required field: content");
    if (!obj.contains("toolUseId"))
        return Utils::ResultError("Missing required field: toolUseId");
    if (!obj.contains("type"))
        return Utils::ResultError("Missing required field: type");
    ToolResultContent result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const QJsonObject mapObj__meta = obj["_meta"].toObject();
        QMap<QString, QJsonValue> map__meta;
        for (auto it = mapObj__meta.constBegin(); it != mapObj__meta.constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        result.__meta = map__meta;
    }
    if (obj.contains("content") && obj["content"].isArray()) {
        const QJsonArray arr = obj["content"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<ContentBlock>(v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._content.append(*res0);
        }
    }
    if (obj.contains("isError"))
        result._isError = obj.value("isError").toBool();
    if (obj.contains("structuredContent") && obj["structuredContent"].isObject()) {
        const QJsonObject mapObj_structuredContent = obj["structuredContent"].toObject();
        QMap<QString, QJsonValue> map_structuredContent;
        for (auto it = mapObj_structuredContent.constBegin(); it != mapObj_structuredContent.constEnd(); ++it)
            map_structuredContent.insert(it.key(), it.value());
        result._structuredContent = map_structuredContent;
    }
    result._toolUseId = obj.value("toolUseId").toString();
    if (obj.value("type").toString() != "tool_result")
        return Utils::ResultError("Field 'type' must be 'tool_result', got: " + obj.value("type").toString());
    return result;
}

QJsonObject toJson(const ToolResultContent &data)
{
    QJsonObject obj{
        {"toolUseId", data._toolUseId},
        {"type", QString("tool_result")}
    };
    if (data.__meta.has_value()) {
        QJsonObject map__meta;
        for (auto it = data.__meta->constBegin(); it != data.__meta->constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        obj.insert("_meta", map__meta);
    }
    QJsonArray arr_content;
    for (const auto &v : data._content) arr_content.append(toJsonValue(v));
    obj.insert("content", arr_content);
    if (data._isError.has_value())
        obj.insert("isError", *data._isError);
    if (data._structuredContent.has_value()) {
        QJsonObject map_structuredContent;
        for (auto it = data._structuredContent->constBegin(); it != data._structuredContent->constEnd(); ++it)
            map_structuredContent.insert(it.key(), it.value());
        obj.insert("structuredContent", map_structuredContent);
    }
    return obj;
}

template<>
Utils::Result<ToolUseContent> fromJson<ToolUseContent>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ToolUseContent");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("input"))
        return Utils::ResultError("Missing required field: input");
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    if (!obj.contains("type"))
        return Utils::ResultError("Missing required field: type");
    ToolUseContent result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const QJsonObject mapObj__meta = obj["_meta"].toObject();
        QMap<QString, QJsonValue> map__meta;
        for (auto it = mapObj__meta.constBegin(); it != mapObj__meta.constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        result.__meta = map__meta;
    }
    result._id = obj.value("id").toString();
    if (obj.contains("input") && obj["input"].isObject()) {
        const QJsonObject mapObj_input = obj["input"].toObject();
        QMap<QString, QJsonValue> map_input;
        for (auto it = mapObj_input.constBegin(); it != mapObj_input.constEnd(); ++it)
            map_input.insert(it.key(), it.value());
        result._input = map_input;
    }
    result._name = obj.value("name").toString();
    if (obj.value("type").toString() != "tool_use")
        return Utils::ResultError("Field 'type' must be 'tool_use', got: " + obj.value("type").toString());
    return result;
}

QJsonObject toJson(const ToolUseContent &data)
{
    QJsonObject obj{
        {"id", data._id},
        {"name", data._name},
        {"type", QString("tool_use")}
    };
    if (data.__meta.has_value()) {
        QJsonObject map__meta;
        for (auto it = data.__meta->constBegin(); it != data.__meta->constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        obj.insert("_meta", map__meta);
    }
    QJsonObject map_input;
    for (auto it = data._input.constBegin(); it != data._input.constEnd(); ++it)
        map_input.insert(it.key(), it.value());
    obj.insert("input", map_input);
    return obj;
}

template<>
Utils::Result<SamplingMessageContentBlock> fromJson<SamplingMessageContentBlock>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid SamplingMessageContentBlock: expected object");
    const QString dispatchValue = val.toObject().value("type").toString();
    if (dispatchValue == "text") {
        const auto res0 = fromJson<TextContent>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return SamplingMessageContentBlock(*res0);
    }
    else if (dispatchValue == "image") {
        const auto res1 = fromJson<ImageContent>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        return SamplingMessageContentBlock(*res1);
    }
    else if (dispatchValue == "audio") {
        const auto res2 = fromJson<AudioContent>(val);
        if (!res2)
            return Utils::ResultError(res2.error());
        return SamplingMessageContentBlock(*res2);
    }
    else if (dispatchValue == "tool_use") {
        const auto res3 = fromJson<ToolUseContent>(val);
        if (!res3)
            return Utils::ResultError(res3.error());
        return SamplingMessageContentBlock(*res3);
    }
    else if (dispatchValue == "tool_result") {
        const auto res4 = fromJson<ToolResultContent>(val);
        if (!res4)
            return Utils::ResultError(res4.error());
        return SamplingMessageContentBlock(*res4);
    }
    return Utils::ResultError("Invalid SamplingMessageContentBlock: unknown type \"" + dispatchValue + "\"");
}

QJsonObject toJson(const SamplingMessageContentBlock &val)
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

QJsonValue toJsonValue(const SamplingMessageContentBlock &val)
{
    return toJson(val);
}

QString dispatchValue(const SamplingMessageContentBlock &val)
{
    return std::visit([](const auto &v) -> QString {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, TextContent>) return "text";
        else if constexpr (std::is_same_v<T, ImageContent>) return "image";
        else if constexpr (std::is_same_v<T, AudioContent>) return "audio";
        else if constexpr (std::is_same_v<T, ToolUseContent>) return "tool_use";
        else if constexpr (std::is_same_v<T, ToolResultContent>) return "tool_result";
        return {};
    }, val);
}

template<>
Utils::Result<CreateMessageResultContent> fromJson<CreateMessageResultContent>(const QJsonValue &val)
{
    if (val.isArray()) {
        QList<SamplingMessageContentBlock> list;
        for (const QJsonValue &v : val.toArray()) {
            const auto res0 = fromJson<SamplingMessageContentBlock>(v);
            if (!res0)
                return Utils::ResultError(res0.error());
            list.append(*res0);
        }
        return CreateMessageResultContent(std::move(list));
    }
    if (!val.isObject())
        return Utils::ResultError("Invalid CreateMessageResultContent: expected object or array");
    const QString dispatchValue = val.toObject().value("type").toString();
    if (dispatchValue == "text") {
        const auto res1 = fromJson<TextContent>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        return CreateMessageResultContent(*res1);
    }
    else if (dispatchValue == "image") {
        const auto res2 = fromJson<ImageContent>(val);
        if (!res2)
            return Utils::ResultError(res2.error());
        return CreateMessageResultContent(*res2);
    }
    else if (dispatchValue == "audio") {
        const auto res3 = fromJson<AudioContent>(val);
        if (!res3)
            return Utils::ResultError(res3.error());
        return CreateMessageResultContent(*res3);
    }
    else if (dispatchValue == "tool_use") {
        const auto res4 = fromJson<ToolUseContent>(val);
        if (!res4)
            return Utils::ResultError(res4.error());
        return CreateMessageResultContent(*res4);
    }
    else if (dispatchValue == "tool_result") {
        const auto res5 = fromJson<ToolResultContent>(val);
        if (!res5)
            return Utils::ResultError(res5.error());
        return CreateMessageResultContent(*res5);
    }
    return Utils::ResultError("Invalid CreateMessageResultContent: unknown type \"" + dispatchValue + "\"");
}

QJsonValue toJsonValue(const CreateMessageResultContent &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QList<SamplingMessageContentBlock>>) {
            QJsonArray arr;
            for (const auto &item : v) arr.append(toJson(item));
            return arr;
        } else if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
}

template<>
Utils::Result<CreateMessageResult> fromJson<CreateMessageResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CreateMessageResult");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("content"))
        return Utils::ResultError("Missing required field: content");
    if (!obj.contains("model"))
        return Utils::ResultError("Missing required field: model");
    if (!obj.contains("role"))
        return Utils::ResultError("Missing required field: role");
    CreateMessageResult result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const QJsonObject mapObj__meta = obj["_meta"].toObject();
        QMap<QString, QJsonValue> map__meta;
        for (auto it = mapObj__meta.constBegin(); it != mapObj__meta.constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        result.__meta = map__meta;
    }
    if (obj.contains("content")) {
        const auto res0 = fromJson<CreateMessageResultContent>(obj["content"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._content = *res0;
    }
    result._model = obj.value("model").toString();
    if (obj.contains("role") && obj["role"].isString()) {
        const auto res1 = fromJson<Role>(obj["role"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._role = *res1;
    }
    if (obj.contains("stopReason"))
        result._stopReason = obj.value("stopReason").toString();
    return result;
}

QJsonObject toJson(const CreateMessageResult &data)
{
    QJsonObject obj{
        {"content", toJsonValue(data._content)},
        {"model", data._model},
        {"role", toJsonValue(data._role)}
    };
    if (data.__meta.has_value()) {
        QJsonObject map__meta;
        for (auto it = data.__meta->constBegin(); it != data.__meta->constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        obj.insert("_meta", map__meta);
    }
    if (data._stopReason.has_value())
        obj.insert("stopReason", *data._stopReason);
    return obj;
}

template<>
Utils::Result<ElicitResultContentValue> fromJson<ElicitResultContentValue>(const QJsonValue &val)
{
    if (val.isArray()) {
        QStringList list;
        for (const QJsonValue &v : val.toArray())
            list.append(v.toString());
        return ElicitResultContentValue(list);
    }
    if (val.isString())
        return ElicitResultContentValue(val.toString());
    if (val.isDouble())
        return ElicitResultContentValue(val.toInt());
    if (val.isBool())
        return ElicitResultContentValue(val.toBool());
    return Utils::ResultError("Invalid ElicitResultContentValue");
}

QJsonValue toJsonValue(const ElicitResultContentValue &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QStringList>) {
            QJsonArray arr;
            for (const QString &s : v) arr.append(s);
            return arr;
        }
        if constexpr (std::is_same_v<T, QString>) return v;
        if constexpr (std::is_same_v<T, int>) return v;
        if constexpr (std::is_same_v<T, bool>) return v;
        return QJsonValue{};
    }, val);
}

QString toString(const ElicitResult::Action &v)
{
    switch(v) {
        case ElicitResult::Action::accept: return "accept";
        case ElicitResult::Action::cancel: return "cancel";
        case ElicitResult::Action::decline: return "decline";
    }
    return {};
}

template<>
Utils::Result<ElicitResult::Action> fromJson<ElicitResult::Action>(const QJsonValue &val)
{
    const QString str = val.toString();
    if (str == "accept") return ElicitResult::Action::accept;
    if (str == "cancel") return ElicitResult::Action::cancel;
    if (str == "decline") return ElicitResult::Action::decline;
    return Utils::ResultError("Invalid ElicitResult::Action value: " + str);
}

QJsonValue toJsonValue(const ElicitResult::Action &v)
{
    return toString(v);
}

template<>
Utils::Result<ElicitResult> fromJson<ElicitResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ElicitResult");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("action"))
        return Utils::ResultError("Missing required field: action");
    ElicitResult result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const QJsonObject mapObj__meta = obj["_meta"].toObject();
        QMap<QString, QJsonValue> map__meta;
        for (auto it = mapObj__meta.constBegin(); it != mapObj__meta.constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        result.__meta = map__meta;
    }
    if (obj.contains("action") && obj["action"].isString()) {
        const auto res0 = fromJson<ElicitResult::Action>(obj["action"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._action = *res0;
    }
    if (obj.contains("content") && obj["content"].isObject()) {
        const QJsonObject mapObj_content = obj["content"].toObject();
        QMap<QString, ElicitResultContentValue> map_content;
        for (auto it = mapObj_content.constBegin(); it != mapObj_content.constEnd(); ++it) {
            const auto res1 = fromJson<ElicitResultContentValue>(it.value());
            if (!res1)
                return Utils::ResultError(res1.error());
            map_content.insert(it.key(), *res1);
        }
        result._content = map_content;
    }
    return result;
}

QJsonObject toJson(const ElicitResult &data)
{
    QJsonObject obj{{"action", toJsonValue(data._action)}};
    if (data.__meta.has_value()) {
        QJsonObject map__meta;
        for (auto it = data.__meta->constBegin(); it != data.__meta->constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        obj.insert("_meta", map__meta);
    }
    if (data._content.has_value()) {
        QJsonObject map_content;
        for (auto it = data._content->constBegin(); it != data._content->constEnd(); ++it)
            map_content.insert(it.key(), toJsonValue(it.value()));
        obj.insert("content", map_content);
    }
    return obj;
}

template<>
Utils::Result<GetTaskPayloadResult> fromJson<GetTaskPayloadResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for GetTaskPayloadResult");
    const QJsonObject obj = val.toObject();
    GetTaskPayloadResult result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const QJsonObject mapObj__meta = obj["_meta"].toObject();
        QMap<QString, QJsonValue> map__meta;
        for (auto it = mapObj__meta.constBegin(); it != mapObj__meta.constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        result.__meta = map__meta;
    }
    {
        const QSet<QString> knownKeys{"_meta"};
        for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
            if (!knownKeys.contains(it.key()))
                result._additionalProperties.insert(it.key(), it.value());
        }
    }
    return result;
}

QJsonObject toJson(const GetTaskPayloadResult &data)
{
    QJsonObject obj;
    if (data.__meta.has_value()) {
        QJsonObject map__meta;
        for (auto it = data.__meta->constBegin(); it != data.__meta->constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        obj.insert("_meta", map__meta);
    }
    for (auto it = data._additionalProperties.constBegin(); it != data._additionalProperties.constEnd(); ++it)
        obj.insert(it.key(), it.value());
    return obj;
}

template<>
Utils::Result<GetTaskResult> fromJson<GetTaskResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for GetTaskResult");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("createdAt"))
        return Utils::ResultError("Missing required field: createdAt");
    if (!obj.contains("lastUpdatedAt"))
        return Utils::ResultError("Missing required field: lastUpdatedAt");
    if (!obj.contains("status"))
        return Utils::ResultError("Missing required field: status");
    if (!obj.contains("taskId"))
        return Utils::ResultError("Missing required field: taskId");
    if (!obj.contains("ttl"))
        return Utils::ResultError("Missing required field: ttl");
    GetTaskResult result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const QJsonObject mapObj__meta = obj["_meta"].toObject();
        QMap<QString, QJsonValue> map__meta;
        for (auto it = mapObj__meta.constBegin(); it != mapObj__meta.constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        result.__meta = map__meta;
    }
    result._createdAt = obj.value("createdAt").toString();
    result._lastUpdatedAt = obj.value("lastUpdatedAt").toString();
    if (obj.contains("pollInterval"))
        result._pollInterval = obj.value("pollInterval").toInt();
    if (obj.contains("status") && obj["status"].isString()) {
        const auto res0 = fromJson<TaskStatus>(obj["status"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._status = *res0;
    }
    if (obj.contains("statusMessage"))
        result._statusMessage = obj.value("statusMessage").toString();
    result._taskId = obj.value("taskId").toString();
    if (!obj["ttl"].isNull()) {
        result._ttl = obj.value("ttl").toInt();
    }
    return result;
}

QJsonObject toJson(const GetTaskResult &data)
{
    QJsonObject obj{
        {"createdAt", data._createdAt},
        {"lastUpdatedAt", data._lastUpdatedAt},
        {"status", toJsonValue(data._status)},
        {"taskId", data._taskId}
    };
    if (data.__meta.has_value()) {
        QJsonObject map__meta;
        for (auto it = data.__meta->constBegin(); it != data.__meta->constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        obj.insert("_meta", map__meta);
    }
    if (data._pollInterval.has_value())
        obj.insert("pollInterval", *data._pollInterval);
    if (data._statusMessage.has_value())
        obj.insert("statusMessage", *data._statusMessage);
    if (data._ttl.has_value())
        obj.insert("ttl", *data._ttl);
    else
        obj.insert("ttl", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<Root> fromJson<Root>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Root");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    Root result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const QJsonObject mapObj__meta = obj["_meta"].toObject();
        QMap<QString, QJsonValue> map__meta;
        for (auto it = mapObj__meta.constBegin(); it != mapObj__meta.constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        result.__meta = map__meta;
    }
    if (obj.contains("name"))
        result._name = obj.value("name").toString();
    result._uri = obj.value("uri").toString();
    return result;
}

QJsonObject toJson(const Root &data)
{
    QJsonObject obj{{"uri", data._uri}};
    if (data.__meta.has_value()) {
        QJsonObject map__meta;
        for (auto it = data.__meta->constBegin(); it != data.__meta->constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        obj.insert("_meta", map__meta);
    }
    if (data._name.has_value())
        obj.insert("name", *data._name);
    return obj;
}

template<>
Utils::Result<ListRootsResult> fromJson<ListRootsResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ListRootsResult");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("roots"))
        return Utils::ResultError("Missing required field: roots");
    ListRootsResult result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const QJsonObject mapObj__meta = obj["_meta"].toObject();
        QMap<QString, QJsonValue> map__meta;
        for (auto it = mapObj__meta.constBegin(); it != mapObj__meta.constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        result.__meta = map__meta;
    }
    if (obj.contains("roots") && obj["roots"].isArray()) {
        const QJsonArray arr = obj["roots"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<Root>(v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._roots.append(*res0);
        }
    }
    return result;
}

QJsonObject toJson(const ListRootsResult &data)
{
    QJsonObject obj;
    if (data.__meta.has_value()) {
        QJsonObject map__meta;
        for (auto it = data.__meta->constBegin(); it != data.__meta->constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        obj.insert("_meta", map__meta);
    }
    QJsonArray arr_roots;
    for (const auto &v : data._roots) arr_roots.append(toJson(v));
    obj.insert("roots", arr_roots);
    return obj;
}

template<>
Utils::Result<ListTasksResult> fromJson<ListTasksResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ListTasksResult");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("tasks"))
        return Utils::ResultError("Missing required field: tasks");
    ListTasksResult result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const QJsonObject mapObj__meta = obj["_meta"].toObject();
        QMap<QString, QJsonValue> map__meta;
        for (auto it = mapObj__meta.constBegin(); it != mapObj__meta.constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        result.__meta = map__meta;
    }
    if (obj.contains("nextCursor"))
        result._nextCursor = obj.value("nextCursor").toString();
    if (obj.contains("tasks") && obj["tasks"].isArray()) {
        const QJsonArray arr = obj["tasks"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<Task>(v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._tasks.append(*res0);
        }
    }
    return result;
}

QJsonObject toJson(const ListTasksResult &data)
{
    QJsonObject obj;
    if (data.__meta.has_value()) {
        QJsonObject map__meta;
        for (auto it = data.__meta->constBegin(); it != data.__meta->constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        obj.insert("_meta", map__meta);
    }
    if (data._nextCursor.has_value())
        obj.insert("nextCursor", *data._nextCursor);
    QJsonArray arr_tasks;
    for (const auto &v : data._tasks) arr_tasks.append(toJson(v));
    obj.insert("tasks", arr_tasks);
    return obj;
}

template<>
Utils::Result<ClientResult> fromJson<ClientResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid ClientResult: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("tasks")) {
        const auto res0 = fromJson<ListTasksResult>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return ClientResult(*res0);
    }
    if (obj.contains("model")) {
        const auto res1 = fromJson<CreateMessageResult>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        return ClientResult(*res1);
    }
    if (obj.contains("roots")) {
        const auto res2 = fromJson<ListRootsResult>(val);
        if (!res2)
            return Utils::ResultError(res2.error());
        return ClientResult(*res2);
    }
    if (obj.contains("action")) {
        const auto res3 = fromJson<ElicitResult>(val);
        if (!res3)
            return Utils::ResultError(res3.error());
        return ClientResult(*res3);
    }
    {
        auto result = fromJson<Result>(val);
        if (result) return ClientResult(*result);
    }
    {
        auto result = fromJson<GetTaskResult>(val);
        if (result) return ClientResult(*result);
    }
    {
        auto result = fromJson<GetTaskPayloadResult>(val);
        if (result) return ClientResult(*result);
    }
    {
        auto result = fromJson<CancelTaskResult>(val);
        if (result) return ClientResult(*result);
    }
    return Utils::ResultError("Invalid ClientResult");
}

QJsonObject toJson(const ClientResult &val)
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

QJsonValue toJsonValue(const ClientResult &val)
{
    return toJson(val);
}

template<>
Utils::Result<CompleteResult::Completion> fromJson<CompleteResult::Completion>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Completion");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("values"))
        return Utils::ResultError("Missing required field: values");
    CompleteResult::Completion result;
    if (obj.contains("hasMore"))
        result._hasMore = obj.value("hasMore").toBool();
    if (obj.contains("total"))
        result._total = obj.value("total").toInt();
    if (obj.contains("values") && obj["values"].isArray()) {
        const QJsonArray arr = obj["values"].toArray();
        for (const QJsonValue &v : arr) {
            result._values.append(v.toString());
        }
    }
    return result;
}

QJsonObject toJson(const CompleteResult::Completion &data)
{
    QJsonObject obj;
    if (data._hasMore.has_value())
        obj.insert("hasMore", *data._hasMore);
    if (data._total.has_value())
        obj.insert("total", *data._total);
    QJsonArray arr_values;
    for (const auto &v : data._values) arr_values.append(v);
    obj.insert("values", arr_values);
    return obj;
}

template<>
Utils::Result<CompleteResult> fromJson<CompleteResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CompleteResult");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("completion"))
        return Utils::ResultError("Missing required field: completion");
    CompleteResult result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const QJsonObject mapObj__meta = obj["_meta"].toObject();
        QMap<QString, QJsonValue> map__meta;
        for (auto it = mapObj__meta.constBegin(); it != mapObj__meta.constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        result.__meta = map__meta;
    }
    if (obj.contains("completion") && obj["completion"].isObject()) {
        const auto res0 = fromJson<CompleteResult::Completion>(obj["completion"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._completion = *res0;
    }
    return result;
}

QJsonObject toJson(const CompleteResult &data)
{
    QJsonObject obj{{"completion", toJson(data._completion)}};
    if (data.__meta.has_value()) {
        QJsonObject map__meta;
        for (auto it = data.__meta->constBegin(); it != data.__meta->constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        obj.insert("_meta", map__meta);
    }
    return obj;
}

template<>
Utils::Result<ModelHint> fromJson<ModelHint>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ModelHint");
    const QJsonObject obj = val.toObject();
    ModelHint result;
    if (obj.contains("name"))
        result._name = obj.value("name").toString();
    return result;
}

QJsonObject toJson(const ModelHint &data)
{
    QJsonObject obj;
    if (data._name.has_value())
        obj.insert("name", *data._name);
    return obj;
}

template<>
Utils::Result<ModelPreferences> fromJson<ModelPreferences>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ModelPreferences");
    const QJsonObject obj = val.toObject();
    ModelPreferences result;
    if (obj.contains("costPriority"))
        result._costPriority = obj.value("costPriority").toDouble();
    if (obj.contains("hints") && obj["hints"].isArray()) {
        const QJsonArray arr = obj["hints"].toArray();
        QList<ModelHint> list_hints;
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<ModelHint>(v);
            if (!res0)
                return Utils::ResultError(res0.error());
            list_hints.append(*res0);
        }
        result._hints = list_hints;
    }
    if (obj.contains("intelligencePriority"))
        result._intelligencePriority = obj.value("intelligencePriority").toDouble();
    if (obj.contains("speedPriority"))
        result._speedPriority = obj.value("speedPriority").toDouble();
    return result;
}

QJsonObject toJson(const ModelPreferences &data)
{
    QJsonObject obj;
    if (data._costPriority.has_value())
        obj.insert("costPriority", *data._costPriority);
    if (data._hints.has_value()) {
        QJsonArray arr_hints;
        for (const auto &v : *data._hints) arr_hints.append(toJson(v));
        obj.insert("hints", arr_hints);
    }
    if (data._intelligencePriority.has_value())
        obj.insert("intelligencePriority", *data._intelligencePriority);
    if (data._speedPriority.has_value())
        obj.insert("speedPriority", *data._speedPriority);
    return obj;
}

template<>
Utils::Result<SamplingMessage> fromJson<SamplingMessage>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SamplingMessage");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("content"))
        return Utils::ResultError("Missing required field: content");
    if (!obj.contains("role"))
        return Utils::ResultError("Missing required field: role");
    SamplingMessage result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const QJsonObject mapObj__meta = obj["_meta"].toObject();
        QMap<QString, QJsonValue> map__meta;
        for (auto it = mapObj__meta.constBegin(); it != mapObj__meta.constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        result.__meta = map__meta;
    }
    if (obj.contains("content")) {
        const auto res0 = fromJson<CreateMessageResultContent>(obj["content"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._content = *res0;
    }
    if (obj.contains("role") && obj["role"].isString()) {
        const auto res1 = fromJson<Role>(obj["role"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._role = *res1;
    }
    return result;
}

QJsonObject toJson(const SamplingMessage &data)
{
    QJsonObject obj{
        {"content", toJsonValue(data._content)},
        {"role", toJsonValue(data._role)}
    };
    if (data.__meta.has_value()) {
        QJsonObject map__meta;
        for (auto it = data.__meta->constBegin(); it != data.__meta->constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        obj.insert("_meta", map__meta);
    }
    return obj;
}

template<>
Utils::Result<ToolAnnotations> fromJson<ToolAnnotations>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ToolAnnotations");
    const QJsonObject obj = val.toObject();
    ToolAnnotations result;
    if (obj.contains("destructiveHint"))
        result._destructiveHint = obj.value("destructiveHint").toBool();
    if (obj.contains("idempotentHint"))
        result._idempotentHint = obj.value("idempotentHint").toBool();
    if (obj.contains("openWorldHint"))
        result._openWorldHint = obj.value("openWorldHint").toBool();
    if (obj.contains("readOnlyHint"))
        result._readOnlyHint = obj.value("readOnlyHint").toBool();
    if (obj.contains("title"))
        result._title = obj.value("title").toString();
    return result;
}

QJsonObject toJson(const ToolAnnotations &data)
{
    QJsonObject obj;
    if (data._destructiveHint.has_value())
        obj.insert("destructiveHint", *data._destructiveHint);
    if (data._idempotentHint.has_value())
        obj.insert("idempotentHint", *data._idempotentHint);
    if (data._openWorldHint.has_value())
        obj.insert("openWorldHint", *data._openWorldHint);
    if (data._readOnlyHint.has_value())
        obj.insert("readOnlyHint", *data._readOnlyHint);
    if (data._title.has_value())
        obj.insert("title", *data._title);
    return obj;
}

QString toString(const ToolExecution::TaskSupport &v)
{
    switch(v) {
        case ToolExecution::TaskSupport::forbidden: return "forbidden";
        case ToolExecution::TaskSupport::optional: return "optional";
        case ToolExecution::TaskSupport::required: return "required";
    }
    return {};
}

template<>
Utils::Result<ToolExecution::TaskSupport> fromJson<ToolExecution::TaskSupport>(const QJsonValue &val)
{
    const QString str = val.toString();
    if (str == "forbidden") return ToolExecution::TaskSupport::forbidden;
    if (str == "optional") return ToolExecution::TaskSupport::optional;
    if (str == "required") return ToolExecution::TaskSupport::required;
    return Utils::ResultError("Invalid ToolExecution::TaskSupport value: " + str);
}

QJsonValue toJsonValue(const ToolExecution::TaskSupport &v)
{
    return toString(v);
}

template<>
Utils::Result<ToolExecution> fromJson<ToolExecution>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ToolExecution");
    const QJsonObject obj = val.toObject();
    ToolExecution result;
    if (obj.contains("taskSupport") && obj["taskSupport"].isString()) {
        const auto res0 = fromJson<ToolExecution::TaskSupport>(obj["taskSupport"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._taskSupport = *res0;
    }
    return result;
}

QJsonObject toJson(const ToolExecution &data)
{
    QJsonObject obj;
    if (data._taskSupport.has_value())
        obj.insert("taskSupport", toJsonValue(*data._taskSupport));
    return obj;
}

template<>
Utils::Result<Tool::InputSchema> fromJson<Tool::InputSchema>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InputSchema");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("type"))
        return Utils::ResultError("Missing required field: type");
    Tool::InputSchema result;
    if (obj.contains("$schema"))
        result._dollarschema = obj.value("$schema").toString();
    if (obj.contains("properties") && obj["properties"].isObject()) {
        const QJsonObject mapObj_properties = obj["properties"].toObject();
        QMap<QString, QJsonObject> map_properties;
        for (auto it = mapObj_properties.constBegin(); it != mapObj_properties.constEnd(); ++it)
            map_properties.insert(it.key(), it.value().toObject());
        result._properties = map_properties;
    }
    if (obj.contains("required") && obj["required"].isArray()) {
        const QJsonArray arr = obj["required"].toArray();
        QStringList list_required;
        for (const QJsonValue &v : arr) {
            list_required.append(v.toString());
        }
        result._required = list_required;
    }
    if (obj.value("type").toString() != "object")
        return Utils::ResultError("Field 'type' must be 'object', got: " + obj.value("type").toString());
    return result;
}

QJsonObject toJson(const Tool::InputSchema &data)
{
    QJsonObject obj{{"type", QString("object")}};
    if (data._dollarschema.has_value())
        obj.insert("$schema", *data._dollarschema);
    if (data._properties.has_value()) {
        QJsonObject map_properties;
        for (auto it = data._properties->constBegin(); it != data._properties->constEnd(); ++it)
            map_properties.insert(it.key(), QJsonValue(it.value()));
        obj.insert("properties", map_properties);
    }
    if (data._required.has_value()) {
        QJsonArray arr_required;
        for (const auto &v : *data._required) arr_required.append(v);
        obj.insert("required", arr_required);
    }
    return obj;
}

template<>
Utils::Result<Tool::OutputSchema> fromJson<Tool::OutputSchema>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for OutputSchema");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("type"))
        return Utils::ResultError("Missing required field: type");
    Tool::OutputSchema result;
    if (obj.contains("$schema"))
        result._dollarschema = obj.value("$schema").toString();
    if (obj.contains("properties") && obj["properties"].isObject()) {
        const QJsonObject mapObj_properties = obj["properties"].toObject();
        QMap<QString, QJsonObject> map_properties;
        for (auto it = mapObj_properties.constBegin(); it != mapObj_properties.constEnd(); ++it)
            map_properties.insert(it.key(), it.value().toObject());
        result._properties = map_properties;
    }
    if (obj.contains("required") && obj["required"].isArray()) {
        const QJsonArray arr = obj["required"].toArray();
        QStringList list_required;
        for (const QJsonValue &v : arr) {
            list_required.append(v.toString());
        }
        result._required = list_required;
    }
    if (obj.value("type").toString() != "object")
        return Utils::ResultError("Field 'type' must be 'object', got: " + obj.value("type").toString());
    return result;
}

QJsonObject toJson(const Tool::OutputSchema &data)
{
    QJsonObject obj{{"type", QString("object")}};
    if (data._dollarschema.has_value())
        obj.insert("$schema", *data._dollarschema);
    if (data._properties.has_value()) {
        QJsonObject map_properties;
        for (auto it = data._properties->constBegin(); it != data._properties->constEnd(); ++it)
            map_properties.insert(it.key(), QJsonValue(it.value()));
        obj.insert("properties", map_properties);
    }
    if (data._required.has_value()) {
        QJsonArray arr_required;
        for (const auto &v : *data._required) arr_required.append(v);
        obj.insert("required", arr_required);
    }
    return obj;
}

template<>
Utils::Result<Tool> fromJson<Tool>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Tool");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("inputSchema"))
        return Utils::ResultError("Missing required field: inputSchema");
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    Tool result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const QJsonObject mapObj__meta = obj["_meta"].toObject();
        QMap<QString, QJsonValue> map__meta;
        for (auto it = mapObj__meta.constBegin(); it != mapObj__meta.constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        result.__meta = map__meta;
    }
    if (obj.contains("annotations") && obj["annotations"].isObject()) {
        const auto res0 = fromJson<ToolAnnotations>(obj["annotations"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._annotations = *res0;
    }
    if (obj.contains("description"))
        result._description = obj.value("description").toString();
    if (obj.contains("execution") && obj["execution"].isObject()) {
        const auto res1 = fromJson<ToolExecution>(obj["execution"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._execution = *res1;
    }
    if (obj.contains("icons") && obj["icons"].isArray()) {
        const QJsonArray arr = obj["icons"].toArray();
        QList<Icon> list_icons;
        for (const QJsonValue &v : arr) {
            const auto res2 = fromJson<Icon>(v);
            if (!res2)
                return Utils::ResultError(res2.error());
            list_icons.append(*res2);
        }
        result._icons = list_icons;
    }
    if (obj.contains("inputSchema") && obj["inputSchema"].isObject()) {
        const auto res3 = fromJson<Tool::InputSchema>(obj["inputSchema"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._inputSchema = *res3;
    }
    result._name = obj.value("name").toString();
    if (obj.contains("outputSchema") && obj["outputSchema"].isObject()) {
        const auto res4 = fromJson<Tool::OutputSchema>(obj["outputSchema"]);
        if (!res4)
            return Utils::ResultError(res4.error());
        result._outputSchema = *res4;
    }
    if (obj.contains("title"))
        result._title = obj.value("title").toString();
    return result;
}

QJsonObject toJson(const Tool &data)
{
    QJsonObject obj{
        {"inputSchema", toJson(data._inputSchema)},
        {"name", data._name}
    };
    if (data.__meta.has_value()) {
        QJsonObject map__meta;
        for (auto it = data.__meta->constBegin(); it != data.__meta->constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        obj.insert("_meta", map__meta);
    }
    if (data._annotations.has_value())
        obj.insert("annotations", toJson(*data._annotations));
    if (data._description.has_value())
        obj.insert("description", *data._description);
    if (data._execution.has_value())
        obj.insert("execution", toJson(*data._execution));
    if (data._icons.has_value()) {
        QJsonArray arr_icons;
        for (const auto &v : *data._icons) arr_icons.append(toJson(v));
        obj.insert("icons", arr_icons);
    }
    if (data._outputSchema.has_value())
        obj.insert("outputSchema", toJson(*data._outputSchema));
    if (data._title.has_value())
        obj.insert("title", *data._title);
    return obj;
}

QString toString(const ToolChoice::Mode &v)
{
    switch(v) {
        case ToolChoice::Mode::auto_: return "auto";
        case ToolChoice::Mode::none: return "none";
        case ToolChoice::Mode::required: return "required";
    }
    return {};
}

template<>
Utils::Result<ToolChoice::Mode> fromJson<ToolChoice::Mode>(const QJsonValue &val)
{
    const QString str = val.toString();
    if (str == "auto") return ToolChoice::Mode::auto_;
    if (str == "none") return ToolChoice::Mode::none;
    if (str == "required") return ToolChoice::Mode::required;
    return Utils::ResultError("Invalid ToolChoice::Mode value: " + str);
}

QJsonValue toJsonValue(const ToolChoice::Mode &v)
{
    return toString(v);
}

template<>
Utils::Result<ToolChoice> fromJson<ToolChoice>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ToolChoice");
    const QJsonObject obj = val.toObject();
    ToolChoice result;
    if (obj.contains("mode") && obj["mode"].isString()) {
        const auto res0 = fromJson<ToolChoice::Mode>(obj["mode"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._mode = *res0;
    }
    return result;
}

QJsonObject toJson(const ToolChoice &data)
{
    QJsonObject obj;
    if (data._mode.has_value())
        obj.insert("mode", toJsonValue(*data._mode));
    return obj;
}

template<>
Utils::Result<CreateMessageRequestParams::Meta> fromJson<CreateMessageRequestParams::Meta>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Meta");
    const QJsonObject obj = val.toObject();
    CreateMessageRequestParams::Meta result;
    if (obj.contains("progressToken")) {
        const auto res0 = fromJson<ProgressToken>(obj["progressToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._progressToken = *res0;
    }
    return result;
}

QJsonObject toJson(const CreateMessageRequestParams::Meta &data)
{
    QJsonObject obj;
    if (data._progressToken.has_value())
        obj.insert("progressToken", toJsonValue(*data._progressToken));
    return obj;
}

QString toString(const CreateMessageRequestParams::IncludeContext &v)
{
    switch(v) {
        case CreateMessageRequestParams::IncludeContext::allServers: return "allServers";
        case CreateMessageRequestParams::IncludeContext::none: return "none";
        case CreateMessageRequestParams::IncludeContext::thisServer: return "thisServer";
    }
    return {};
}

template<>
Utils::Result<CreateMessageRequestParams::IncludeContext> fromJson<CreateMessageRequestParams::IncludeContext>(const QJsonValue &val)
{
    const QString str = val.toString();
    if (str == "allServers") return CreateMessageRequestParams::IncludeContext::allServers;
    if (str == "none") return CreateMessageRequestParams::IncludeContext::none;
    if (str == "thisServer") return CreateMessageRequestParams::IncludeContext::thisServer;
    return Utils::ResultError("Invalid CreateMessageRequestParams::IncludeContext value: " + str);
}

QJsonValue toJsonValue(const CreateMessageRequestParams::IncludeContext &v)
{
    return toString(v);
}

template<>
Utils::Result<CreateMessageRequestParams> fromJson<CreateMessageRequestParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CreateMessageRequestParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("maxTokens"))
        return Utils::ResultError("Missing required field: maxTokens");
    if (!obj.contains("messages"))
        return Utils::ResultError("Missing required field: messages");
    CreateMessageRequestParams result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<CreateMessageRequestParams::Meta>(obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    if (obj.contains("includeContext") && obj["includeContext"].isString()) {
        const auto res1 = fromJson<CreateMessageRequestParams::IncludeContext>(obj["includeContext"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._includeContext = *res1;
    }
    result._maxTokens = obj.value("maxTokens").toInt();
    if (obj.contains("messages") && obj["messages"].isArray()) {
        const QJsonArray arr = obj["messages"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res2 = fromJson<SamplingMessage>(v);
            if (!res2)
                return Utils::ResultError(res2.error());
            result._messages.append(*res2);
        }
    }
    if (obj.contains("metadata") && obj["metadata"].isObject()) {
        const QJsonObject mapObj_metadata = obj["metadata"].toObject();
        QMap<QString, QJsonValue> map_metadata;
        for (auto it = mapObj_metadata.constBegin(); it != mapObj_metadata.constEnd(); ++it)
            map_metadata.insert(it.key(), it.value());
        result._metadata = map_metadata;
    }
    if (obj.contains("modelPreferences") && obj["modelPreferences"].isObject()) {
        const auto res3 = fromJson<ModelPreferences>(obj["modelPreferences"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._modelPreferences = *res3;
    }
    if (obj.contains("stopSequences") && obj["stopSequences"].isArray()) {
        const QJsonArray arr = obj["stopSequences"].toArray();
        QStringList list_stopSequences;
        for (const QJsonValue &v : arr) {
            list_stopSequences.append(v.toString());
        }
        result._stopSequences = list_stopSequences;
    }
    if (obj.contains("systemPrompt"))
        result._systemPrompt = obj.value("systemPrompt").toString();
    if (obj.contains("task") && obj["task"].isObject()) {
        const auto res4 = fromJson<TaskMetadata>(obj["task"]);
        if (!res4)
            return Utils::ResultError(res4.error());
        result._task = *res4;
    }
    if (obj.contains("temperature"))
        result._temperature = obj.value("temperature").toDouble();
    if (obj.contains("toolChoice") && obj["toolChoice"].isObject()) {
        const auto res5 = fromJson<ToolChoice>(obj["toolChoice"]);
        if (!res5)
            return Utils::ResultError(res5.error());
        result._toolChoice = *res5;
    }
    if (obj.contains("tools") && obj["tools"].isArray()) {
        const QJsonArray arr = obj["tools"].toArray();
        QList<Tool> list_tools;
        for (const QJsonValue &v : arr) {
            const auto res6 = fromJson<Tool>(v);
            if (!res6)
                return Utils::ResultError(res6.error());
            list_tools.append(*res6);
        }
        result._tools = list_tools;
    }
    return result;
}

QJsonObject toJson(const CreateMessageRequestParams &data)
{
    QJsonObject obj{{"maxTokens", data._maxTokens}};
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
    if (data._includeContext.has_value())
        obj.insert("includeContext", toJsonValue(*data._includeContext));
    QJsonArray arr_messages;
    for (const auto &v : data._messages) arr_messages.append(toJson(v));
    obj.insert("messages", arr_messages);
    if (data._metadata.has_value()) {
        QJsonObject map_metadata;
        for (auto it = data._metadata->constBegin(); it != data._metadata->constEnd(); ++it)
            map_metadata.insert(it.key(), it.value());
        obj.insert("metadata", map_metadata);
    }
    if (data._modelPreferences.has_value())
        obj.insert("modelPreferences", toJson(*data._modelPreferences));
    if (data._stopSequences.has_value()) {
        QJsonArray arr_stopSequences;
        for (const auto &v : *data._stopSequences) arr_stopSequences.append(v);
        obj.insert("stopSequences", arr_stopSequences);
    }
    if (data._systemPrompt.has_value())
        obj.insert("systemPrompt", *data._systemPrompt);
    if (data._task.has_value())
        obj.insert("task", toJson(*data._task));
    if (data._temperature.has_value())
        obj.insert("temperature", *data._temperature);
    if (data._toolChoice.has_value())
        obj.insert("toolChoice", toJson(*data._toolChoice));
    if (data._tools.has_value()) {
        QJsonArray arr_tools;
        for (const auto &v : *data._tools) arr_tools.append(toJson(v));
        obj.insert("tools", arr_tools);
    }
    return obj;
}

template<>
Utils::Result<CreateMessageRequest> fromJson<CreateMessageRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CreateMessageRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    if (!obj.contains("params"))
        return Utils::ResultError("Missing required field: params");
    CreateMessageRequest result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>(obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "sampling/createMessage")
        return Utils::ResultError("Field 'method' must be 'sampling/createMessage', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res1 = fromJson<CreateMessageRequestParams>(obj["params"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._params = *res1;
    }
    return result;
}

QJsonObject toJson(const CreateMessageRequest &data)
{
    QJsonObject obj{
        {"id", toJsonValue(data._id)},
        {"jsonrpc", QString("2.0")},
        {"method", QString("sampling/createMessage")},
        {"params", toJson(data._params)}
    };
    return obj;
}

template<>
Utils::Result<CreateTaskResult> fromJson<CreateTaskResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CreateTaskResult");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("task"))
        return Utils::ResultError("Missing required field: task");
    CreateTaskResult result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const QJsonObject mapObj__meta = obj["_meta"].toObject();
        QMap<QString, QJsonValue> map__meta;
        for (auto it = mapObj__meta.constBegin(); it != mapObj__meta.constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        result.__meta = map__meta;
    }
    if (obj.contains("task") && obj["task"].isObject()) {
        const auto res0 = fromJson<Task>(obj["task"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._task = *res0;
    }
    return result;
}

QJsonObject toJson(const CreateTaskResult &data)
{
    QJsonObject obj{{"task", toJson(data._task)}};
    if (data.__meta.has_value()) {
        QJsonObject map__meta;
        for (auto it = data.__meta->constBegin(); it != data.__meta->constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        obj.insert("_meta", map__meta);
    }
    return obj;
}

template<> Utils::Result<Cursor> fromJson<Cursor>(const QJsonValue &val)
{
    if (!val.isString()) return Utils::ResultError("Expected string");
    return val.toString();
}

template<>
Utils::Result<LegacyTitledEnumSchema> fromJson<LegacyTitledEnumSchema>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for LegacyTitledEnumSchema");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("enum"))
        return Utils::ResultError("Missing required field: enum");
    if (!obj.contains("type"))
        return Utils::ResultError("Missing required field: type");
    LegacyTitledEnumSchema result;
    if (obj.contains("default"))
        result._default_ = obj.value("default").toString();
    if (obj.contains("description"))
        result._description = obj.value("description").toString();
    if (obj.contains("enum") && obj["enum"].isArray()) {
        const QJsonArray arr = obj["enum"].toArray();
        for (const QJsonValue &v : arr) {
            result._enum_.append(v.toString());
        }
    }
    if (obj.contains("enumNames") && obj["enumNames"].isArray()) {
        const QJsonArray arr = obj["enumNames"].toArray();
        QStringList list_enumNames;
        for (const QJsonValue &v : arr) {
            list_enumNames.append(v.toString());
        }
        result._enumNames = list_enumNames;
    }
    if (obj.contains("title"))
        result._title = obj.value("title").toString();
    if (obj.value("type").toString() != "string")
        return Utils::ResultError("Field 'type' must be 'string', got: " + obj.value("type").toString());
    return result;
}

QJsonObject toJson(const LegacyTitledEnumSchema &data)
{
    QJsonObject obj{{"type", QString("string")}};
    if (data._default_.has_value())
        obj.insert("default", *data._default_);
    if (data._description.has_value())
        obj.insert("description", *data._description);
    QJsonArray arr_enum_;
    for (const auto &v : data._enum_) arr_enum_.append(v);
    obj.insert("enum", arr_enum_);
    if (data._enumNames.has_value()) {
        QJsonArray arr_enumNames;
        for (const auto &v : *data._enumNames) arr_enumNames.append(v);
        obj.insert("enumNames", arr_enumNames);
    }
    if (data._title.has_value())
        obj.insert("title", *data._title);
    return obj;
}

QString toString(const NumberSchema::Type &v)
{
    switch(v) {
        case NumberSchema::Type::integer: return "integer";
        case NumberSchema::Type::number: return "number";
    }
    return {};
}

template<>
Utils::Result<NumberSchema::Type> fromJson<NumberSchema::Type>(const QJsonValue &val)
{
    const QString str = val.toString();
    if (str == "integer") return NumberSchema::Type::integer;
    if (str == "number") return NumberSchema::Type::number;
    return Utils::ResultError("Invalid NumberSchema::Type value: " + str);
}

QJsonValue toJsonValue(const NumberSchema::Type &v)
{
    return toString(v);
}

template<>
Utils::Result<NumberSchema> fromJson<NumberSchema>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for NumberSchema");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("type"))
        return Utils::ResultError("Missing required field: type");
    NumberSchema result;
    if (obj.contains("default"))
        result._default_ = obj.value("default").toInt();
    if (obj.contains("description"))
        result._description = obj.value("description").toString();
    if (obj.contains("maximum"))
        result._maximum = obj.value("maximum").toInt();
    if (obj.contains("minimum"))
        result._minimum = obj.value("minimum").toInt();
    if (obj.contains("title"))
        result._title = obj.value("title").toString();
    if (obj.contains("type") && obj["type"].isString()) {
        const auto res0 = fromJson<NumberSchema::Type>(obj["type"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._type = *res0;
    }
    return result;
}

QJsonObject toJson(const NumberSchema &data)
{
    QJsonObject obj{{"type", toJsonValue(data._type)}};
    if (data._default_.has_value())
        obj.insert("default", *data._default_);
    if (data._description.has_value())
        obj.insert("description", *data._description);
    if (data._maximum.has_value())
        obj.insert("maximum", *data._maximum);
    if (data._minimum.has_value())
        obj.insert("minimum", *data._minimum);
    if (data._title.has_value())
        obj.insert("title", *data._title);
    return obj;
}

QString toString(const StringSchema::Format &v)
{
    switch(v) {
        case StringSchema::Format::date: return "date";
        case StringSchema::Format::dateminustime: return "date-time";
        case StringSchema::Format::email: return "email";
        case StringSchema::Format::uri: return "uri";
    }
    return {};
}

template<>
Utils::Result<StringSchema::Format> fromJson<StringSchema::Format>(const QJsonValue &val)
{
    const QString str = val.toString();
    if (str == "date") return StringSchema::Format::date;
    if (str == "date-time") return StringSchema::Format::dateminustime;
    if (str == "email") return StringSchema::Format::email;
    if (str == "uri") return StringSchema::Format::uri;
    return Utils::ResultError("Invalid StringSchema::Format value: " + str);
}

QJsonValue toJsonValue(const StringSchema::Format &v)
{
    return toString(v);
}

template<>
Utils::Result<StringSchema> fromJson<StringSchema>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for StringSchema");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("type"))
        return Utils::ResultError("Missing required field: type");
    StringSchema result;
    if (obj.contains("default"))
        result._default_ = obj.value("default").toString();
    if (obj.contains("description"))
        result._description = obj.value("description").toString();
    if (obj.contains("format") && obj["format"].isString()) {
        const auto res0 = fromJson<StringSchema::Format>(obj["format"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._format = *res0;
    }
    if (obj.contains("maxLength"))
        result._maxLength = obj.value("maxLength").toInt();
    if (obj.contains("minLength"))
        result._minLength = obj.value("minLength").toInt();
    if (obj.contains("title"))
        result._title = obj.value("title").toString();
    if (obj.value("type").toString() != "string")
        return Utils::ResultError("Field 'type' must be 'string', got: " + obj.value("type").toString());
    return result;
}

QJsonObject toJson(const StringSchema &data)
{
    QJsonObject obj{{"type", QString("string")}};
    if (data._default_.has_value())
        obj.insert("default", *data._default_);
    if (data._description.has_value())
        obj.insert("description", *data._description);
    if (data._format.has_value())
        obj.insert("format", toJsonValue(*data._format));
    if (data._maxLength.has_value())
        obj.insert("maxLength", *data._maxLength);
    if (data._minLength.has_value())
        obj.insert("minLength", *data._minLength);
    if (data._title.has_value())
        obj.insert("title", *data._title);
    return obj;
}

template<>
Utils::Result<TitledMultiSelectEnumSchema::Items::AnyOfItem> fromJson<TitledMultiSelectEnumSchema::Items::AnyOfItem>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for AnyOfItem");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("const"))
        return Utils::ResultError("Missing required field: const");
    if (!obj.contains("title"))
        return Utils::ResultError("Missing required field: title");
    TitledMultiSelectEnumSchema::Items::AnyOfItem result;
    result._const_ = obj.value("const").toString();
    result._title = obj.value("title").toString();
    return result;
}

QJsonObject toJson(const TitledMultiSelectEnumSchema::Items::AnyOfItem &data)
{
    QJsonObject obj{
        {"const", data._const_},
        {"title", data._title}
    };
    return obj;
}

template<>
Utils::Result<TitledMultiSelectEnumSchema::Items> fromJson<TitledMultiSelectEnumSchema::Items>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Items");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("anyOf"))
        return Utils::ResultError("Missing required field: anyOf");
    TitledMultiSelectEnumSchema::Items result;
    if (obj.contains("anyOf") && obj["anyOf"].isArray()) {
        const QJsonArray arr = obj["anyOf"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<TitledMultiSelectEnumSchema::Items::AnyOfItem>(v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._anyOf.append(*res0);
        }
    }
    return result;
}

QJsonObject toJson(const TitledMultiSelectEnumSchema::Items &data)
{
    QJsonObject obj;
    QJsonArray arr_anyOf;
    for (const auto &v : data._anyOf) arr_anyOf.append(toJson(v));
    obj.insert("anyOf", arr_anyOf);
    return obj;
}

template<>
Utils::Result<TitledMultiSelectEnumSchema> fromJson<TitledMultiSelectEnumSchema>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TitledMultiSelectEnumSchema");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("items"))
        return Utils::ResultError("Missing required field: items");
    if (!obj.contains("type"))
        return Utils::ResultError("Missing required field: type");
    TitledMultiSelectEnumSchema result;
    if (obj.contains("default") && obj["default"].isArray()) {
        const QJsonArray arr = obj["default"].toArray();
        QStringList list_default_;
        for (const QJsonValue &v : arr) {
            list_default_.append(v.toString());
        }
        result._default_ = list_default_;
    }
    if (obj.contains("description"))
        result._description = obj.value("description").toString();
    if (obj.contains("items") && obj["items"].isObject()) {
        const auto res0 = fromJson<TitledMultiSelectEnumSchema::Items>(obj["items"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._items = *res0;
    }
    if (obj.contains("maxItems"))
        result._maxItems = obj.value("maxItems").toInt();
    if (obj.contains("minItems"))
        result._minItems = obj.value("minItems").toInt();
    if (obj.contains("title"))
        result._title = obj.value("title").toString();
    if (obj.value("type").toString() != "array")
        return Utils::ResultError("Field 'type' must be 'array', got: " + obj.value("type").toString());
    return result;
}

QJsonObject toJson(const TitledMultiSelectEnumSchema &data)
{
    QJsonObject obj{
        {"items", toJson(data._items)},
        {"type", QString("array")}
    };
    if (data._default_.has_value()) {
        QJsonArray arr_default_;
        for (const auto &v : *data._default_) arr_default_.append(v);
        obj.insert("default", arr_default_);
    }
    if (data._description.has_value())
        obj.insert("description", *data._description);
    if (data._maxItems.has_value())
        obj.insert("maxItems", *data._maxItems);
    if (data._minItems.has_value())
        obj.insert("minItems", *data._minItems);
    if (data._title.has_value())
        obj.insert("title", *data._title);
    return obj;
}

template<>
Utils::Result<TitledSingleSelectEnumSchema::OneOfItem> fromJson<TitledSingleSelectEnumSchema::OneOfItem>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for OneOfItem");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("const"))
        return Utils::ResultError("Missing required field: const");
    if (!obj.contains("title"))
        return Utils::ResultError("Missing required field: title");
    TitledSingleSelectEnumSchema::OneOfItem result;
    result._const_ = obj.value("const").toString();
    result._title = obj.value("title").toString();
    return result;
}

QJsonObject toJson(const TitledSingleSelectEnumSchema::OneOfItem &data)
{
    QJsonObject obj{
        {"const", data._const_},
        {"title", data._title}
    };
    return obj;
}

template<>
Utils::Result<TitledSingleSelectEnumSchema> fromJson<TitledSingleSelectEnumSchema>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TitledSingleSelectEnumSchema");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("oneOf"))
        return Utils::ResultError("Missing required field: oneOf");
    if (!obj.contains("type"))
        return Utils::ResultError("Missing required field: type");
    TitledSingleSelectEnumSchema result;
    if (obj.contains("default"))
        result._default_ = obj.value("default").toString();
    if (obj.contains("description"))
        result._description = obj.value("description").toString();
    if (obj.contains("oneOf") && obj["oneOf"].isArray()) {
        const QJsonArray arr = obj["oneOf"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<TitledSingleSelectEnumSchema::OneOfItem>(v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._oneOf.append(*res0);
        }
    }
    if (obj.contains("title"))
        result._title = obj.value("title").toString();
    if (obj.value("type").toString() != "string")
        return Utils::ResultError("Field 'type' must be 'string', got: " + obj.value("type").toString());
    return result;
}

QJsonObject toJson(const TitledSingleSelectEnumSchema &data)
{
    QJsonObject obj{{"type", QString("string")}};
    if (data._default_.has_value())
        obj.insert("default", *data._default_);
    if (data._description.has_value())
        obj.insert("description", *data._description);
    QJsonArray arr_oneOf;
    for (const auto &v : data._oneOf) arr_oneOf.append(toJson(v));
    obj.insert("oneOf", arr_oneOf);
    if (data._title.has_value())
        obj.insert("title", *data._title);
    return obj;
}

template<>
Utils::Result<UntitledMultiSelectEnumSchema::Items> fromJson<UntitledMultiSelectEnumSchema::Items>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Items");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("enum"))
        return Utils::ResultError("Missing required field: enum");
    if (!obj.contains("type"))
        return Utils::ResultError("Missing required field: type");
    UntitledMultiSelectEnumSchema::Items result;
    if (obj.contains("enum") && obj["enum"].isArray()) {
        const QJsonArray arr = obj["enum"].toArray();
        for (const QJsonValue &v : arr) {
            result._enum_.append(v.toString());
        }
    }
    if (obj.value("type").toString() != "string")
        return Utils::ResultError("Field 'type' must be 'string', got: " + obj.value("type").toString());
    return result;
}

QJsonObject toJson(const UntitledMultiSelectEnumSchema::Items &data)
{
    QJsonObject obj{{"type", QString("string")}};
    QJsonArray arr_enum_;
    for (const auto &v : data._enum_) arr_enum_.append(v);
    obj.insert("enum", arr_enum_);
    return obj;
}

template<>
Utils::Result<UntitledMultiSelectEnumSchema> fromJson<UntitledMultiSelectEnumSchema>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for UntitledMultiSelectEnumSchema");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("items"))
        return Utils::ResultError("Missing required field: items");
    if (!obj.contains("type"))
        return Utils::ResultError("Missing required field: type");
    UntitledMultiSelectEnumSchema result;
    if (obj.contains("default") && obj["default"].isArray()) {
        const QJsonArray arr = obj["default"].toArray();
        QStringList list_default_;
        for (const QJsonValue &v : arr) {
            list_default_.append(v.toString());
        }
        result._default_ = list_default_;
    }
    if (obj.contains("description"))
        result._description = obj.value("description").toString();
    if (obj.contains("items") && obj["items"].isObject()) {
        const auto res0 = fromJson<UntitledMultiSelectEnumSchema::Items>(obj["items"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._items = *res0;
    }
    if (obj.contains("maxItems"))
        result._maxItems = obj.value("maxItems").toInt();
    if (obj.contains("minItems"))
        result._minItems = obj.value("minItems").toInt();
    if (obj.contains("title"))
        result._title = obj.value("title").toString();
    if (obj.value("type").toString() != "array")
        return Utils::ResultError("Field 'type' must be 'array', got: " + obj.value("type").toString());
    return result;
}

QJsonObject toJson(const UntitledMultiSelectEnumSchema &data)
{
    QJsonObject obj{
        {"items", toJson(data._items)},
        {"type", QString("array")}
    };
    if (data._default_.has_value()) {
        QJsonArray arr_default_;
        for (const auto &v : *data._default_) arr_default_.append(v);
        obj.insert("default", arr_default_);
    }
    if (data._description.has_value())
        obj.insert("description", *data._description);
    if (data._maxItems.has_value())
        obj.insert("maxItems", *data._maxItems);
    if (data._minItems.has_value())
        obj.insert("minItems", *data._minItems);
    if (data._title.has_value())
        obj.insert("title", *data._title);
    return obj;
}

template<>
Utils::Result<UntitledSingleSelectEnumSchema> fromJson<UntitledSingleSelectEnumSchema>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for UntitledSingleSelectEnumSchema");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("enum"))
        return Utils::ResultError("Missing required field: enum");
    if (!obj.contains("type"))
        return Utils::ResultError("Missing required field: type");
    UntitledSingleSelectEnumSchema result;
    if (obj.contains("default"))
        result._default_ = obj.value("default").toString();
    if (obj.contains("description"))
        result._description = obj.value("description").toString();
    if (obj.contains("enum") && obj["enum"].isArray()) {
        const QJsonArray arr = obj["enum"].toArray();
        for (const QJsonValue &v : arr) {
            result._enum_.append(v.toString());
        }
    }
    if (obj.contains("title"))
        result._title = obj.value("title").toString();
    if (obj.value("type").toString() != "string")
        return Utils::ResultError("Field 'type' must be 'string', got: " + obj.value("type").toString());
    return result;
}

QJsonObject toJson(const UntitledSingleSelectEnumSchema &data)
{
    QJsonObject obj{{"type", QString("string")}};
    if (data._default_.has_value())
        obj.insert("default", *data._default_);
    if (data._description.has_value())
        obj.insert("description", *data._description);
    QJsonArray arr_enum_;
    for (const auto &v : data._enum_) arr_enum_.append(v);
    obj.insert("enum", arr_enum_);
    if (data._title.has_value())
        obj.insert("title", *data._title);
    return obj;
}

template<>
Utils::Result<PrimitiveSchemaDefinition> fromJson<PrimitiveSchemaDefinition>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid PrimitiveSchemaDefinition: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("oneOf")) {
        const auto res0 = fromJson<TitledSingleSelectEnumSchema>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return PrimitiveSchemaDefinition(*res0);
    }
    {
        auto result = fromJson<StringSchema>(val);
        if (result) return PrimitiveSchemaDefinition(*result);
    }
    {
        auto result = fromJson<NumberSchema>(val);
        if (result) return PrimitiveSchemaDefinition(*result);
    }
    {
        auto result = fromJson<BooleanSchema>(val);
        if (result) return PrimitiveSchemaDefinition(*result);
    }
    {
        auto result = fromJson<UntitledSingleSelectEnumSchema>(val);
        if (result) return PrimitiveSchemaDefinition(*result);
    }
    {
        auto result = fromJson<UntitledMultiSelectEnumSchema>(val);
        if (result) return PrimitiveSchemaDefinition(*result);
    }
    {
        auto result = fromJson<TitledMultiSelectEnumSchema>(val);
        if (result) return PrimitiveSchemaDefinition(*result);
    }
    {
        auto result = fromJson<LegacyTitledEnumSchema>(val);
        if (result) return PrimitiveSchemaDefinition(*result);
    }
    return Utils::ResultError("Invalid PrimitiveSchemaDefinition");
}

QJsonObject toJson(const PrimitiveSchemaDefinition &val)
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

QJsonValue toJsonValue(const PrimitiveSchemaDefinition &val)
{
    return toJson(val);
}

template<>
Utils::Result<ElicitRequestFormParams::Meta> fromJson<ElicitRequestFormParams::Meta>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Meta");
    const QJsonObject obj = val.toObject();
    ElicitRequestFormParams::Meta result;
    if (obj.contains("progressToken")) {
        const auto res0 = fromJson<ProgressToken>(obj["progressToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._progressToken = *res0;
    }
    return result;
}

QJsonObject toJson(const ElicitRequestFormParams::Meta &data)
{
    QJsonObject obj;
    if (data._progressToken.has_value())
        obj.insert("progressToken", toJsonValue(*data._progressToken));
    return obj;
}

template<>
Utils::Result<ElicitRequestFormParams::RequestedSchema> fromJson<ElicitRequestFormParams::RequestedSchema>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for RequestedSchema");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("properties"))
        return Utils::ResultError("Missing required field: properties");
    if (!obj.contains("type"))
        return Utils::ResultError("Missing required field: type");
    ElicitRequestFormParams::RequestedSchema result;
    if (obj.contains("$schema"))
        result._dollarschema = obj.value("$schema").toString();
    if (obj.contains("properties") && obj["properties"].isObject()) {
        const QJsonObject mapObj_properties = obj["properties"].toObject();
        QMap<QString, PrimitiveSchemaDefinition> map_properties;
        for (auto it = mapObj_properties.constBegin(); it != mapObj_properties.constEnd(); ++it) {
            const auto res0 = fromJson<PrimitiveSchemaDefinition>(it.value());
            if (!res0)
                return Utils::ResultError(res0.error());
            map_properties.insert(it.key(), *res0);
        }
        result._properties = map_properties;
    }
    if (obj.contains("required") && obj["required"].isArray()) {
        const QJsonArray arr = obj["required"].toArray();
        QStringList list_required;
        for (const QJsonValue &v : arr) {
            list_required.append(v.toString());
        }
        result._required = list_required;
    }
    if (obj.value("type").toString() != "object")
        return Utils::ResultError("Field 'type' must be 'object', got: " + obj.value("type").toString());
    return result;
}

QJsonObject toJson(const ElicitRequestFormParams::RequestedSchema &data)
{
    QJsonObject obj{{"type", QString("object")}};
    if (data._dollarschema.has_value())
        obj.insert("$schema", *data._dollarschema);
    QJsonObject map_properties;
    for (auto it = data._properties.constBegin(); it != data._properties.constEnd(); ++it)
        map_properties.insert(it.key(), toJsonValue(it.value()));
    obj.insert("properties", map_properties);
    if (data._required.has_value()) {
        QJsonArray arr_required;
        for (const auto &v : *data._required) arr_required.append(v);
        obj.insert("required", arr_required);
    }
    return obj;
}

template<>
Utils::Result<ElicitRequestFormParams> fromJson<ElicitRequestFormParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ElicitRequestFormParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("message"))
        return Utils::ResultError("Missing required field: message");
    if (!obj.contains("requestedSchema"))
        return Utils::ResultError("Missing required field: requestedSchema");
    ElicitRequestFormParams result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<ElicitRequestFormParams::Meta>(obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    result._message = obj.value("message").toString();
    if (obj.value("mode").toString() != "form")
        return Utils::ResultError("Field 'mode' must be 'form', got: " + obj.value("mode").toString());
    if (obj.contains("requestedSchema") && obj["requestedSchema"].isObject()) {
        const auto res1 = fromJson<ElicitRequestFormParams::RequestedSchema>(obj["requestedSchema"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._requestedSchema = *res1;
    }
    if (obj.contains("task") && obj["task"].isObject()) {
        const auto res2 = fromJson<TaskMetadata>(obj["task"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._task = *res2;
    }
    return result;
}

QJsonObject toJson(const ElicitRequestFormParams &data)
{
    QJsonObject obj{
        {"message", data._message},
        {"mode", QString("form")},
        {"requestedSchema", toJson(data._requestedSchema)}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
    if (data._task.has_value())
        obj.insert("task", toJson(*data._task));
    return obj;
}

template<>
Utils::Result<ElicitRequestURLParams::Meta> fromJson<ElicitRequestURLParams::Meta>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Meta");
    const QJsonObject obj = val.toObject();
    ElicitRequestURLParams::Meta result;
    if (obj.contains("progressToken")) {
        const auto res0 = fromJson<ProgressToken>(obj["progressToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._progressToken = *res0;
    }
    return result;
}

QJsonObject toJson(const ElicitRequestURLParams::Meta &data)
{
    QJsonObject obj;
    if (data._progressToken.has_value())
        obj.insert("progressToken", toJsonValue(*data._progressToken));
    return obj;
}

template<>
Utils::Result<ElicitRequestURLParams> fromJson<ElicitRequestURLParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ElicitRequestURLParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("elicitationId"))
        return Utils::ResultError("Missing required field: elicitationId");
    if (!obj.contains("message"))
        return Utils::ResultError("Missing required field: message");
    if (!obj.contains("mode"))
        return Utils::ResultError("Missing required field: mode");
    if (!obj.contains("url"))
        return Utils::ResultError("Missing required field: url");
    ElicitRequestURLParams result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<ElicitRequestURLParams::Meta>(obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    result._elicitationId = obj.value("elicitationId").toString();
    result._message = obj.value("message").toString();
    if (obj.value("mode").toString() != "url")
        return Utils::ResultError("Field 'mode' must be 'url', got: " + obj.value("mode").toString());
    if (obj.contains("task") && obj["task"].isObject()) {
        const auto res1 = fromJson<TaskMetadata>(obj["task"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._task = *res1;
    }
    result._url = obj.value("url").toString();
    return result;
}

QJsonObject toJson(const ElicitRequestURLParams &data)
{
    QJsonObject obj{
        {"elicitationId", data._elicitationId},
        {"message", data._message},
        {"mode", QString("url")},
        {"url", data._url}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
    if (data._task.has_value())
        obj.insert("task", toJson(*data._task));
    return obj;
}

template<>
Utils::Result<ElicitRequestParams> fromJson<ElicitRequestParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid ElicitRequestParams: expected object");
    const QString dispatchValue = val.toObject().value("mode").toString();
    if (dispatchValue == "url") {
        const auto res0 = fromJson<ElicitRequestURLParams>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return ElicitRequestParams(*res0);
    }
    else if (dispatchValue == "form") {
        const auto res1 = fromJson<ElicitRequestFormParams>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        return ElicitRequestParams(*res1);
    }
    return Utils::ResultError("Invalid ElicitRequestParams: unknown mode \"" + dispatchValue + "\"");
}

QJsonObject toJson(const ElicitRequestParams &val)
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

QJsonValue toJsonValue(const ElicitRequestParams &val)
{
    return toJson(val);
}

QString dispatchValue(const ElicitRequestParams &val)
{
    return std::visit([](const auto &v) -> QString {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, ElicitRequestURLParams>) return "url";
        else if constexpr (std::is_same_v<T, ElicitRequestFormParams>) return "form";
        return {};
    }, val);
}

QString message(const ElicitRequestParams &val)
{
    return std::visit([](const auto &v) -> QString { return v._message; }, val);
}

template<>
Utils::Result<ElicitRequest> fromJson<ElicitRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ElicitRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    if (!obj.contains("params"))
        return Utils::ResultError("Missing required field: params");
    ElicitRequest result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>(obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "elicitation/create")
        return Utils::ResultError("Field 'method' must be 'elicitation/create', got: " + obj.value("method").toString());
    if (obj.contains("params")) {
        const auto res1 = fromJson<ElicitRequestParams>(obj["params"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._params = *res1;
    }
    return result;
}

QJsonObject toJson(const ElicitRequest &data)
{
    QJsonObject obj{
        {"id", toJsonValue(data._id)},
        {"jsonrpc", QString("2.0")},
        {"method", QString("elicitation/create")},
        {"params", toJsonValue(data._params)}
    };
    return obj;
}

template<>
Utils::Result<ElicitationCompleteNotification::Params> fromJson<ElicitationCompleteNotification::Params>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Params");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("elicitationId"))
        return Utils::ResultError("Missing required field: elicitationId");
    ElicitationCompleteNotification::Params result;
    result._elicitationId = obj.value("elicitationId").toString();
    return result;
}

QJsonObject toJson(const ElicitationCompleteNotification::Params &data)
{
    QJsonObject obj{{"elicitationId", data._elicitationId}};
    return obj;
}

template<>
Utils::Result<ElicitationCompleteNotification> fromJson<ElicitationCompleteNotification>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ElicitationCompleteNotification");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    if (!obj.contains("params"))
        return Utils::ResultError("Missing required field: params");
    ElicitationCompleteNotification result;
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "notifications/elicitation/complete")
        return Utils::ResultError("Field 'method' must be 'notifications/elicitation/complete', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res0 = fromJson<ElicitationCompleteNotification::Params>(obj["params"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._params = *res0;
    }
    return result;
}

QJsonObject toJson(const ElicitationCompleteNotification &data)
{
    QJsonObject obj{
        {"jsonrpc", QString("2.0")},
        {"method", QString("notifications/elicitation/complete")},
        {"params", toJson(data._params)}
    };
    return obj;
}

template<>
Utils::Result<EnumSchema> fromJson<EnumSchema>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid EnumSchema: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("oneOf")) {
        const auto res0 = fromJson<TitledSingleSelectEnumSchema>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return EnumSchema(*res0);
    }
    {
        auto result = fromJson<UntitledSingleSelectEnumSchema>(val);
        if (result) return EnumSchema(*result);
    }
    {
        auto result = fromJson<UntitledMultiSelectEnumSchema>(val);
        if (result) return EnumSchema(*result);
    }
    {
        auto result = fromJson<TitledMultiSelectEnumSchema>(val);
        if (result) return EnumSchema(*result);
    }
    {
        auto result = fromJson<LegacyTitledEnumSchema>(val);
        if (result) return EnumSchema(*result);
    }
    return Utils::ResultError("Invalid EnumSchema");
}

QJsonObject toJson(const EnumSchema &val)
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

QJsonValue toJsonValue(const EnumSchema &val)
{
    return toJson(val);
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
    if (obj.contains("data"))
        result._data = obj.value("data");
    result._message = obj.value("message").toString();
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
Utils::Result<PromptMessage> fromJson<PromptMessage>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for PromptMessage");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("content"))
        return Utils::ResultError("Missing required field: content");
    if (!obj.contains("role"))
        return Utils::ResultError("Missing required field: role");
    PromptMessage result;
    if (obj.contains("content")) {
        const auto res0 = fromJson<ContentBlock>(obj["content"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._content = *res0;
    }
    if (obj.contains("role") && obj["role"].isString()) {
        const auto res1 = fromJson<Role>(obj["role"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._role = *res1;
    }
    return result;
}

QJsonObject toJson(const PromptMessage &data)
{
    QJsonObject obj{
        {"content", toJsonValue(data._content)},
        {"role", toJsonValue(data._role)}
    };
    return obj;
}

template<>
Utils::Result<GetPromptResult> fromJson<GetPromptResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for GetPromptResult");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("messages"))
        return Utils::ResultError("Missing required field: messages");
    GetPromptResult result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const QJsonObject mapObj__meta = obj["_meta"].toObject();
        QMap<QString, QJsonValue> map__meta;
        for (auto it = mapObj__meta.constBegin(); it != mapObj__meta.constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        result.__meta = map__meta;
    }
    if (obj.contains("description"))
        result._description = obj.value("description").toString();
    if (obj.contains("messages") && obj["messages"].isArray()) {
        const QJsonArray arr = obj["messages"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<PromptMessage>(v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._messages.append(*res0);
        }
    }
    return result;
}

QJsonObject toJson(const GetPromptResult &data)
{
    QJsonObject obj;
    if (data.__meta.has_value()) {
        QJsonObject map__meta;
        for (auto it = data.__meta->constBegin(); it != data.__meta->constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        obj.insert("_meta", map__meta);
    }
    if (data._description.has_value())
        obj.insert("description", *data._description);
    QJsonArray arr_messages;
    for (const auto &v : data._messages) arr_messages.append(toJson(v));
    obj.insert("messages", arr_messages);
    return obj;
}

template<>
Utils::Result<Icons> fromJson<Icons>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Icons");
    const QJsonObject obj = val.toObject();
    Icons result;
    if (obj.contains("icons") && obj["icons"].isArray()) {
        const QJsonArray arr = obj["icons"].toArray();
        QList<Icon> list_icons;
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<Icon>(v);
            if (!res0)
                return Utils::ResultError(res0.error());
            list_icons.append(*res0);
        }
        result._icons = list_icons;
    }
    return result;
}

QJsonObject toJson(const Icons &data)
{
    QJsonObject obj;
    if (data._icons.has_value()) {
        QJsonArray arr_icons;
        for (const auto &v : *data._icons) arr_icons.append(toJson(v));
        obj.insert("icons", arr_icons);
    }
    return obj;
}

template<>
Utils::Result<ServerCapabilities::Prompts> fromJson<ServerCapabilities::Prompts>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Prompts");
    const QJsonObject obj = val.toObject();
    ServerCapabilities::Prompts result;
    if (obj.contains("listChanged"))
        result._listChanged = obj.value("listChanged").toBool();
    return result;
}

QJsonObject toJson(const ServerCapabilities::Prompts &data)
{
    QJsonObject obj;
    if (data._listChanged.has_value())
        obj.insert("listChanged", *data._listChanged);
    return obj;
}

template<>
Utils::Result<ServerCapabilities::Resources> fromJson<ServerCapabilities::Resources>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Resources");
    const QJsonObject obj = val.toObject();
    ServerCapabilities::Resources result;
    if (obj.contains("listChanged"))
        result._listChanged = obj.value("listChanged").toBool();
    if (obj.contains("subscribe"))
        result._subscribe = obj.value("subscribe").toBool();
    return result;
}

QJsonObject toJson(const ServerCapabilities::Resources &data)
{
    QJsonObject obj;
    if (data._listChanged.has_value())
        obj.insert("listChanged", *data._listChanged);
    if (data._subscribe.has_value())
        obj.insert("subscribe", *data._subscribe);
    return obj;
}

template<>
Utils::Result<ServerCapabilities::Tasks::Requests::Tools> fromJson<ServerCapabilities::Tasks::Requests::Tools>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Tools");
    const QJsonObject obj = val.toObject();
    ServerCapabilities::Tasks::Requests::Tools result;
    if (obj.contains("call") && obj["call"].isObject()) {
        const QJsonObject mapObj_call = obj["call"].toObject();
        QMap<QString, QJsonValue> map_call;
        for (auto it = mapObj_call.constBegin(); it != mapObj_call.constEnd(); ++it)
            map_call.insert(it.key(), it.value());
        result._call = map_call;
    }
    return result;
}

QJsonObject toJson(const ServerCapabilities::Tasks::Requests::Tools &data)
{
    QJsonObject obj;
    if (data._call.has_value()) {
        QJsonObject map_call;
        for (auto it = data._call->constBegin(); it != data._call->constEnd(); ++it)
            map_call.insert(it.key(), it.value());
        obj.insert("call", map_call);
    }
    return obj;
}

template<>
Utils::Result<ServerCapabilities::Tasks::Requests> fromJson<ServerCapabilities::Tasks::Requests>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Requests");
    const QJsonObject obj = val.toObject();
    ServerCapabilities::Tasks::Requests result;
    if (obj.contains("tools") && obj["tools"].isObject()) {
        const auto res0 = fromJson<ServerCapabilities::Tasks::Requests::Tools>(obj["tools"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._tools = *res0;
    }
    return result;
}

QJsonObject toJson(const ServerCapabilities::Tasks::Requests &data)
{
    QJsonObject obj;
    if (data._tools.has_value())
        obj.insert("tools", toJson(*data._tools));
    return obj;
}

template<>
Utils::Result<ServerCapabilities::Tasks> fromJson<ServerCapabilities::Tasks>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Tasks");
    const QJsonObject obj = val.toObject();
    ServerCapabilities::Tasks result;
    if (obj.contains("cancel") && obj["cancel"].isObject()) {
        const QJsonObject mapObj_cancel = obj["cancel"].toObject();
        QMap<QString, QJsonValue> map_cancel;
        for (auto it = mapObj_cancel.constBegin(); it != mapObj_cancel.constEnd(); ++it)
            map_cancel.insert(it.key(), it.value());
        result._cancel = map_cancel;
    }
    if (obj.contains("list") && obj["list"].isObject()) {
        const QJsonObject mapObj_list = obj["list"].toObject();
        QMap<QString, QJsonValue> map_list;
        for (auto it = mapObj_list.constBegin(); it != mapObj_list.constEnd(); ++it)
            map_list.insert(it.key(), it.value());
        result._list = map_list;
    }
    if (obj.contains("requests") && obj["requests"].isObject()) {
        const auto res0 = fromJson<ServerCapabilities::Tasks::Requests>(obj["requests"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._requests = *res0;
    }
    return result;
}

QJsonObject toJson(const ServerCapabilities::Tasks &data)
{
    QJsonObject obj;
    if (data._cancel.has_value()) {
        QJsonObject map_cancel;
        for (auto it = data._cancel->constBegin(); it != data._cancel->constEnd(); ++it)
            map_cancel.insert(it.key(), it.value());
        obj.insert("cancel", map_cancel);
    }
    if (data._list.has_value()) {
        QJsonObject map_list;
        for (auto it = data._list->constBegin(); it != data._list->constEnd(); ++it)
            map_list.insert(it.key(), it.value());
        obj.insert("list", map_list);
    }
    if (data._requests.has_value())
        obj.insert("requests", toJson(*data._requests));
    return obj;
}

template<>
Utils::Result<ServerCapabilities::Tools> fromJson<ServerCapabilities::Tools>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Tools");
    const QJsonObject obj = val.toObject();
    ServerCapabilities::Tools result;
    if (obj.contains("listChanged"))
        result._listChanged = obj.value("listChanged").toBool();
    return result;
}

QJsonObject toJson(const ServerCapabilities::Tools &data)
{
    QJsonObject obj;
    if (data._listChanged.has_value())
        obj.insert("listChanged", *data._listChanged);
    return obj;
}

template<>
Utils::Result<ServerCapabilities> fromJson<ServerCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ServerCapabilities");
    const QJsonObject obj = val.toObject();
    ServerCapabilities result;
    if (obj.contains("completions") && obj["completions"].isObject()) {
        const QJsonObject mapObj_completions = obj["completions"].toObject();
        QMap<QString, QJsonValue> map_completions;
        for (auto it = mapObj_completions.constBegin(); it != mapObj_completions.constEnd(); ++it)
            map_completions.insert(it.key(), it.value());
        result._completions = map_completions;
    }
    if (obj.contains("experimental") && obj["experimental"].isObject()) {
        const QJsonObject mapObj_experimental = obj["experimental"].toObject();
        QMap<QString, QJsonObject> map_experimental;
        for (auto it = mapObj_experimental.constBegin(); it != mapObj_experimental.constEnd(); ++it)
            map_experimental.insert(it.key(), it.value().toObject());
        result._experimental = map_experimental;
    }
    if (obj.contains("logging") && obj["logging"].isObject()) {
        const QJsonObject mapObj_logging = obj["logging"].toObject();
        QMap<QString, QJsonValue> map_logging;
        for (auto it = mapObj_logging.constBegin(); it != mapObj_logging.constEnd(); ++it)
            map_logging.insert(it.key(), it.value());
        result._logging = map_logging;
    }
    if (obj.contains("prompts") && obj["prompts"].isObject()) {
        const auto res0 = fromJson<ServerCapabilities::Prompts>(obj["prompts"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._prompts = *res0;
    }
    if (obj.contains("resources") && obj["resources"].isObject()) {
        const auto res1 = fromJson<ServerCapabilities::Resources>(obj["resources"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._resources = *res1;
    }
    if (obj.contains("tasks") && obj["tasks"].isObject()) {
        const auto res2 = fromJson<ServerCapabilities::Tasks>(obj["tasks"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._tasks = *res2;
    }
    if (obj.contains("tools") && obj["tools"].isObject()) {
        const auto res3 = fromJson<ServerCapabilities::Tools>(obj["tools"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._tools = *res3;
    }
    return result;
}

QJsonObject toJson(const ServerCapabilities &data)
{
    QJsonObject obj;
    if (data._completions.has_value()) {
        QJsonObject map_completions;
        for (auto it = data._completions->constBegin(); it != data._completions->constEnd(); ++it)
            map_completions.insert(it.key(), it.value());
        obj.insert("completions", map_completions);
    }
    if (data._experimental.has_value()) {
        QJsonObject map_experimental;
        for (auto it = data._experimental->constBegin(); it != data._experimental->constEnd(); ++it)
            map_experimental.insert(it.key(), QJsonValue(it.value()));
        obj.insert("experimental", map_experimental);
    }
    if (data._logging.has_value()) {
        QJsonObject map_logging;
        for (auto it = data._logging->constBegin(); it != data._logging->constEnd(); ++it)
            map_logging.insert(it.key(), it.value());
        obj.insert("logging", map_logging);
    }
    if (data._prompts.has_value())
        obj.insert("prompts", toJson(*data._prompts));
    if (data._resources.has_value())
        obj.insert("resources", toJson(*data._resources));
    if (data._tasks.has_value())
        obj.insert("tasks", toJson(*data._tasks));
    if (data._tools.has_value())
        obj.insert("tools", toJson(*data._tools));
    return obj;
}

template<>
Utils::Result<InitializeResult> fromJson<InitializeResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InitializeResult");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("capabilities"))
        return Utils::ResultError("Missing required field: capabilities");
    if (!obj.contains("protocolVersion"))
        return Utils::ResultError("Missing required field: protocolVersion");
    if (!obj.contains("serverInfo"))
        return Utils::ResultError("Missing required field: serverInfo");
    InitializeResult result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const QJsonObject mapObj__meta = obj["_meta"].toObject();
        QMap<QString, QJsonValue> map__meta;
        for (auto it = mapObj__meta.constBegin(); it != mapObj__meta.constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        result.__meta = map__meta;
    }
    if (obj.contains("capabilities") && obj["capabilities"].isObject()) {
        const auto res0 = fromJson<ServerCapabilities>(obj["capabilities"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._capabilities = *res0;
    }
    if (obj.contains("instructions"))
        result._instructions = obj.value("instructions").toString();
    result._protocolVersion = obj.value("protocolVersion").toString();
    if (obj.contains("serverInfo") && obj["serverInfo"].isObject()) {
        const auto res1 = fromJson<Implementation>(obj["serverInfo"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._serverInfo = *res1;
    }
    return result;
}

QJsonObject toJson(const InitializeResult &data)
{
    QJsonObject obj{
        {"capabilities", toJson(data._capabilities)},
        {"protocolVersion", data._protocolVersion},
        {"serverInfo", toJson(data._serverInfo)}
    };
    if (data.__meta.has_value()) {
        QJsonObject map__meta;
        for (auto it = data.__meta->constBegin(); it != data.__meta->constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        obj.insert("_meta", map__meta);
    }
    if (data._instructions.has_value())
        obj.insert("instructions", *data._instructions);
    return obj;
}

template<>
Utils::Result<JSONRPCErrorResponse> fromJson<JSONRPCErrorResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for JSONRPCErrorResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("error"))
        return Utils::ResultError("Missing required field: error");
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    JSONRPCErrorResponse result;
    if (obj.contains("error") && obj["error"].isObject()) {
        const auto res0 = fromJson<Error>(obj["error"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._error = *res0;
    }
    if (obj.contains("id")) {
        const auto res1 = fromJson<RequestId>(obj["id"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._id = *res1;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    return result;
}

QJsonObject toJson(const JSONRPCErrorResponse &data)
{
    QJsonObject obj{
        {"error", toJson(data._error)},
        {"jsonrpc", QString("2.0")}
    };
    if (data._id.has_value())
        obj.insert("id", toJsonValue(*data._id));
    return obj;
}

template<>
Utils::Result<JSONRPCNotification> fromJson<JSONRPCNotification>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for JSONRPCNotification");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    JSONRPCNotification result;
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    result._method = obj.value("method").toString();
    if (obj.contains("params") && obj["params"].isObject()) {
        const QJsonObject mapObj_params = obj["params"].toObject();
        QMap<QString, QJsonValue> map_params;
        for (auto it = mapObj_params.constBegin(); it != mapObj_params.constEnd(); ++it)
            map_params.insert(it.key(), it.value());
        result._params = map_params;
    }
    return result;
}

QJsonObject toJson(const JSONRPCNotification &data)
{
    QJsonObject obj{
        {"jsonrpc", QString("2.0")},
        {"method", data._method}
    };
    if (data._params.has_value()) {
        QJsonObject map_params;
        for (auto it = data._params->constBegin(); it != data._params->constEnd(); ++it)
            map_params.insert(it.key(), it.value());
        obj.insert("params", map_params);
    }
    return obj;
}

template<>
Utils::Result<JSONRPCRequest> fromJson<JSONRPCRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for JSONRPCRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    JSONRPCRequest result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>(obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    result._method = obj.value("method").toString();
    if (obj.contains("params") && obj["params"].isObject()) {
        const QJsonObject mapObj_params = obj["params"].toObject();
        QMap<QString, QJsonValue> map_params;
        for (auto it = mapObj_params.constBegin(); it != mapObj_params.constEnd(); ++it)
            map_params.insert(it.key(), it.value());
        result._params = map_params;
    }
    return result;
}

QJsonObject toJson(const JSONRPCRequest &data)
{
    QJsonObject obj{
        {"id", toJsonValue(data._id)},
        {"jsonrpc", QString("2.0")},
        {"method", data._method}
    };
    if (data._params.has_value()) {
        QJsonObject map_params;
        for (auto it = data._params->constBegin(); it != data._params->constEnd(); ++it)
            map_params.insert(it.key(), it.value());
        obj.insert("params", map_params);
    }
    return obj;
}

template<>
Utils::Result<JSONRPCResultResponse> fromJson<JSONRPCResultResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for JSONRPCResultResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("result"))
        return Utils::ResultError("Missing required field: result");
    JSONRPCResultResponse result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>(obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.contains("result") && obj["result"].isObject()) {
        const auto res1 = fromJson<Result>(obj["result"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._result = *res1;
    }
    return result;
}

QJsonObject toJson(const JSONRPCResultResponse &data)
{
    QJsonObject obj{
        {"id", toJsonValue(data._id)},
        {"jsonrpc", QString("2.0")},
        {"result", toJson(data._result)}
    };
    return obj;
}

template<>
Utils::Result<JSONRPCMessage> fromJson<JSONRPCMessage>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid JSONRPCMessage: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("result")) {
        const auto res0 = fromJson<JSONRPCResultResponse>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return JSONRPCMessage(*res0);
    }
    if (obj.contains("error")) {
        const auto res1 = fromJson<JSONRPCErrorResponse>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        return JSONRPCMessage(*res1);
    }
    {
        auto result = fromJson<JSONRPCRequest>(val);
        if (result) return JSONRPCMessage(*result);
    }
    {
        auto result = fromJson<JSONRPCNotification>(val);
        if (result) return JSONRPCMessage(*result);
    }
    return Utils::ResultError("Invalid JSONRPCMessage");
}

QJsonObject toJson(const JSONRPCMessage &val)
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

QJsonValue toJsonValue(const JSONRPCMessage &val)
{
    return toJson(val);
}

template<>
Utils::Result<JSONRPCResponse> fromJson<JSONRPCResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid JSONRPCResponse: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("result")) {
        const auto res0 = fromJson<JSONRPCResultResponse>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return JSONRPCResponse(*res0);
    }
    if (obj.contains("error")) {
        const auto res1 = fromJson<JSONRPCErrorResponse>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        return JSONRPCResponse(*res1);
    }
    return Utils::ResultError("Invalid JSONRPCResponse");
}

QJsonObject toJson(const JSONRPCResponse &val)
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

QJsonValue toJsonValue(const JSONRPCResponse &val)
{
    return toJson(val);
}

template<>
Utils::Result<PromptArgument> fromJson<PromptArgument>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for PromptArgument");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    PromptArgument result;
    if (obj.contains("description"))
        result._description = obj.value("description").toString();
    result._name = obj.value("name").toString();
    if (obj.contains("required"))
        result._required = obj.value("required").toBool();
    if (obj.contains("title"))
        result._title = obj.value("title").toString();
    return result;
}

QJsonObject toJson(const PromptArgument &data)
{
    QJsonObject obj{{"name", data._name}};
    if (data._description.has_value())
        obj.insert("description", *data._description);
    if (data._required.has_value())
        obj.insert("required", *data._required);
    if (data._title.has_value())
        obj.insert("title", *data._title);
    return obj;
}

template<>
Utils::Result<Prompt> fromJson<Prompt>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Prompt");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    Prompt result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const QJsonObject mapObj__meta = obj["_meta"].toObject();
        QMap<QString, QJsonValue> map__meta;
        for (auto it = mapObj__meta.constBegin(); it != mapObj__meta.constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        result.__meta = map__meta;
    }
    if (obj.contains("arguments") && obj["arguments"].isArray()) {
        const QJsonArray arr = obj["arguments"].toArray();
        QList<PromptArgument> list_arguments;
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<PromptArgument>(v);
            if (!res0)
                return Utils::ResultError(res0.error());
            list_arguments.append(*res0);
        }
        result._arguments = list_arguments;
    }
    if (obj.contains("description"))
        result._description = obj.value("description").toString();
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
    result._name = obj.value("name").toString();
    if (obj.contains("title"))
        result._title = obj.value("title").toString();
    return result;
}

QJsonObject toJson(const Prompt &data)
{
    QJsonObject obj{{"name", data._name}};
    if (data.__meta.has_value()) {
        QJsonObject map__meta;
        for (auto it = data.__meta->constBegin(); it != data.__meta->constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        obj.insert("_meta", map__meta);
    }
    if (data._arguments.has_value()) {
        QJsonArray arr_arguments;
        for (const auto &v : *data._arguments) arr_arguments.append(toJson(v));
        obj.insert("arguments", arr_arguments);
    }
    if (data._description.has_value())
        obj.insert("description", *data._description);
    if (data._icons.has_value()) {
        QJsonArray arr_icons;
        for (const auto &v : *data._icons) arr_icons.append(toJson(v));
        obj.insert("icons", arr_icons);
    }
    if (data._title.has_value())
        obj.insert("title", *data._title);
    return obj;
}

template<>
Utils::Result<ListPromptsResult> fromJson<ListPromptsResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ListPromptsResult");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("prompts"))
        return Utils::ResultError("Missing required field: prompts");
    ListPromptsResult result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const QJsonObject mapObj__meta = obj["_meta"].toObject();
        QMap<QString, QJsonValue> map__meta;
        for (auto it = mapObj__meta.constBegin(); it != mapObj__meta.constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        result.__meta = map__meta;
    }
    if (obj.contains("nextCursor"))
        result._nextCursor = obj.value("nextCursor").toString();
    if (obj.contains("prompts") && obj["prompts"].isArray()) {
        const QJsonArray arr = obj["prompts"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<Prompt>(v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._prompts.append(*res0);
        }
    }
    return result;
}

QJsonObject toJson(const ListPromptsResult &data)
{
    QJsonObject obj;
    if (data.__meta.has_value()) {
        QJsonObject map__meta;
        for (auto it = data.__meta->constBegin(); it != data.__meta->constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        obj.insert("_meta", map__meta);
    }
    if (data._nextCursor.has_value())
        obj.insert("nextCursor", *data._nextCursor);
    QJsonArray arr_prompts;
    for (const auto &v : data._prompts) arr_prompts.append(toJson(v));
    obj.insert("prompts", arr_prompts);
    return obj;
}

template<>
Utils::Result<ResourceTemplate> fromJson<ResourceTemplate>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ResourceTemplate");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    if (!obj.contains("uriTemplate"))
        return Utils::ResultError("Missing required field: uriTemplate");
    ResourceTemplate result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const QJsonObject mapObj__meta = obj["_meta"].toObject();
        QMap<QString, QJsonValue> map__meta;
        for (auto it = mapObj__meta.constBegin(); it != mapObj__meta.constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        result.__meta = map__meta;
    }
    if (obj.contains("annotations") && obj["annotations"].isObject()) {
        const auto res0 = fromJson<Annotations>(obj["annotations"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._annotations = *res0;
    }
    if (obj.contains("description"))
        result._description = obj.value("description").toString();
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
    if (obj.contains("mimeType"))
        result._mimeType = obj.value("mimeType").toString();
    result._name = obj.value("name").toString();
    if (obj.contains("title"))
        result._title = obj.value("title").toString();
    result._uriTemplate = obj.value("uriTemplate").toString();
    return result;
}

QJsonObject toJson(const ResourceTemplate &data)
{
    QJsonObject obj{
        {"name", data._name},
        {"uriTemplate", data._uriTemplate}
    };
    if (data.__meta.has_value()) {
        QJsonObject map__meta;
        for (auto it = data.__meta->constBegin(); it != data.__meta->constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        obj.insert("_meta", map__meta);
    }
    if (data._annotations.has_value())
        obj.insert("annotations", toJson(*data._annotations));
    if (data._description.has_value())
        obj.insert("description", *data._description);
    if (data._icons.has_value()) {
        QJsonArray arr_icons;
        for (const auto &v : *data._icons) arr_icons.append(toJson(v));
        obj.insert("icons", arr_icons);
    }
    if (data._mimeType.has_value())
        obj.insert("mimeType", *data._mimeType);
    if (data._title.has_value())
        obj.insert("title", *data._title);
    return obj;
}

template<>
Utils::Result<ListResourceTemplatesResult> fromJson<ListResourceTemplatesResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ListResourceTemplatesResult");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("resourceTemplates"))
        return Utils::ResultError("Missing required field: resourceTemplates");
    ListResourceTemplatesResult result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const QJsonObject mapObj__meta = obj["_meta"].toObject();
        QMap<QString, QJsonValue> map__meta;
        for (auto it = mapObj__meta.constBegin(); it != mapObj__meta.constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        result.__meta = map__meta;
    }
    if (obj.contains("nextCursor"))
        result._nextCursor = obj.value("nextCursor").toString();
    if (obj.contains("resourceTemplates") && obj["resourceTemplates"].isArray()) {
        const QJsonArray arr = obj["resourceTemplates"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<ResourceTemplate>(v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._resourceTemplates.append(*res0);
        }
    }
    return result;
}

QJsonObject toJson(const ListResourceTemplatesResult &data)
{
    QJsonObject obj;
    if (data.__meta.has_value()) {
        QJsonObject map__meta;
        for (auto it = data.__meta->constBegin(); it != data.__meta->constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        obj.insert("_meta", map__meta);
    }
    if (data._nextCursor.has_value())
        obj.insert("nextCursor", *data._nextCursor);
    QJsonArray arr_resourceTemplates;
    for (const auto &v : data._resourceTemplates) arr_resourceTemplates.append(toJson(v));
    obj.insert("resourceTemplates", arr_resourceTemplates);
    return obj;
}

template<>
Utils::Result<Resource> fromJson<Resource>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Resource");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    Resource result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const QJsonObject mapObj__meta = obj["_meta"].toObject();
        QMap<QString, QJsonValue> map__meta;
        for (auto it = mapObj__meta.constBegin(); it != mapObj__meta.constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        result.__meta = map__meta;
    }
    if (obj.contains("annotations") && obj["annotations"].isObject()) {
        const auto res0 = fromJson<Annotations>(obj["annotations"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._annotations = *res0;
    }
    if (obj.contains("description"))
        result._description = obj.value("description").toString();
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
    if (obj.contains("mimeType"))
        result._mimeType = obj.value("mimeType").toString();
    result._name = obj.value("name").toString();
    if (obj.contains("size"))
        result._size = obj.value("size").toInt();
    if (obj.contains("title"))
        result._title = obj.value("title").toString();
    result._uri = obj.value("uri").toString();
    return result;
}

QJsonObject toJson(const Resource &data)
{
    QJsonObject obj{
        {"name", data._name},
        {"uri", data._uri}
    };
    if (data.__meta.has_value()) {
        QJsonObject map__meta;
        for (auto it = data.__meta->constBegin(); it != data.__meta->constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        obj.insert("_meta", map__meta);
    }
    if (data._annotations.has_value())
        obj.insert("annotations", toJson(*data._annotations));
    if (data._description.has_value())
        obj.insert("description", *data._description);
    if (data._icons.has_value()) {
        QJsonArray arr_icons;
        for (const auto &v : *data._icons) arr_icons.append(toJson(v));
        obj.insert("icons", arr_icons);
    }
    if (data._mimeType.has_value())
        obj.insert("mimeType", *data._mimeType);
    if (data._size.has_value())
        obj.insert("size", *data._size);
    if (data._title.has_value())
        obj.insert("title", *data._title);
    return obj;
}

template<>
Utils::Result<ListResourcesResult> fromJson<ListResourcesResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ListResourcesResult");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("resources"))
        return Utils::ResultError("Missing required field: resources");
    ListResourcesResult result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const QJsonObject mapObj__meta = obj["_meta"].toObject();
        QMap<QString, QJsonValue> map__meta;
        for (auto it = mapObj__meta.constBegin(); it != mapObj__meta.constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        result.__meta = map__meta;
    }
    if (obj.contains("nextCursor"))
        result._nextCursor = obj.value("nextCursor").toString();
    if (obj.contains("resources") && obj["resources"].isArray()) {
        const QJsonArray arr = obj["resources"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<Resource>(v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._resources.append(*res0);
        }
    }
    return result;
}

QJsonObject toJson(const ListResourcesResult &data)
{
    QJsonObject obj;
    if (data.__meta.has_value()) {
        QJsonObject map__meta;
        for (auto it = data.__meta->constBegin(); it != data.__meta->constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        obj.insert("_meta", map__meta);
    }
    if (data._nextCursor.has_value())
        obj.insert("nextCursor", *data._nextCursor);
    QJsonArray arr_resources;
    for (const auto &v : data._resources) arr_resources.append(toJson(v));
    obj.insert("resources", arr_resources);
    return obj;
}

template<>
Utils::Result<ListRootsRequest> fromJson<ListRootsRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ListRootsRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    ListRootsRequest result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>(obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "roots/list")
        return Utils::ResultError("Field 'method' must be 'roots/list', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res1 = fromJson<RequestParams>(obj["params"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._params = *res1;
    }
    return result;
}

QJsonObject toJson(const ListRootsRequest &data)
{
    QJsonObject obj{
        {"id", toJsonValue(data._id)},
        {"jsonrpc", QString("2.0")},
        {"method", QString("roots/list")}
    };
    if (data._params.has_value())
        obj.insert("params", toJson(*data._params));
    return obj;
}

template<>
Utils::Result<ListToolsResult> fromJson<ListToolsResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ListToolsResult");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("tools"))
        return Utils::ResultError("Missing required field: tools");
    ListToolsResult result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const QJsonObject mapObj__meta = obj["_meta"].toObject();
        QMap<QString, QJsonValue> map__meta;
        for (auto it = mapObj__meta.constBegin(); it != mapObj__meta.constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        result.__meta = map__meta;
    }
    if (obj.contains("nextCursor"))
        result._nextCursor = obj.value("nextCursor").toString();
    if (obj.contains("tools") && obj["tools"].isArray()) {
        const QJsonArray arr = obj["tools"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<Tool>(v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._tools.append(*res0);
        }
    }
    return result;
}

QJsonObject toJson(const ListToolsResult &data)
{
    QJsonObject obj;
    if (data.__meta.has_value()) {
        QJsonObject map__meta;
        for (auto it = data.__meta->constBegin(); it != data.__meta->constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        obj.insert("_meta", map__meta);
    }
    if (data._nextCursor.has_value())
        obj.insert("nextCursor", *data._nextCursor);
    QJsonArray arr_tools;
    for (const auto &v : data._tools) arr_tools.append(toJson(v));
    obj.insert("tools", arr_tools);
    return obj;
}

template<>
Utils::Result<LoggingMessageNotificationParams> fromJson<LoggingMessageNotificationParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for LoggingMessageNotificationParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("data"))
        return Utils::ResultError("Missing required field: data");
    if (!obj.contains("level"))
        return Utils::ResultError("Missing required field: level");
    LoggingMessageNotificationParams result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const QJsonObject mapObj__meta = obj["_meta"].toObject();
        QMap<QString, QJsonValue> map__meta;
        for (auto it = mapObj__meta.constBegin(); it != mapObj__meta.constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        result.__meta = map__meta;
    }
    result._data = obj.value("data");
    if (obj.contains("level") && obj["level"].isString()) {
        const auto res0 = fromJson<LoggingLevel>(obj["level"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._level = *res0;
    }
    if (obj.contains("logger"))
        result._logger = obj.value("logger").toString();
    return result;
}

QJsonObject toJson(const LoggingMessageNotificationParams &data)
{
    QJsonObject obj{
        {"data", data._data},
        {"level", toJsonValue(data._level)}
    };
    if (data.__meta.has_value()) {
        QJsonObject map__meta;
        for (auto it = data.__meta->constBegin(); it != data.__meta->constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        obj.insert("_meta", map__meta);
    }
    if (data._logger.has_value())
        obj.insert("logger", *data._logger);
    return obj;
}

template<>
Utils::Result<LoggingMessageNotification> fromJson<LoggingMessageNotification>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for LoggingMessageNotification");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    if (!obj.contains("params"))
        return Utils::ResultError("Missing required field: params");
    LoggingMessageNotification result;
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "notifications/message")
        return Utils::ResultError("Field 'method' must be 'notifications/message', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res0 = fromJson<LoggingMessageNotificationParams>(obj["params"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._params = *res0;
    }
    return result;
}

QJsonObject toJson(const LoggingMessageNotification &data)
{
    QJsonObject obj{
        {"jsonrpc", QString("2.0")},
        {"method", QString("notifications/message")},
        {"params", toJson(data._params)}
    };
    return obj;
}

template<>
Utils::Result<MultiSelectEnumSchema> fromJson<MultiSelectEnumSchema>(const QJsonValue &val)
{
    if (val.isObject()) {
        auto result = fromJson<UntitledMultiSelectEnumSchema>(val);
        if (result) return MultiSelectEnumSchema(*result);
    }
    if (val.isObject()) {
        auto result = fromJson<TitledMultiSelectEnumSchema>(val);
        if (result) return MultiSelectEnumSchema(*result);
    }
    return Utils::ResultError("Invalid MultiSelectEnumSchema");
}

QJsonObject toJson(const MultiSelectEnumSchema &val)
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

QJsonValue toJsonValue(const MultiSelectEnumSchema &val)
{
    return toJson(val);
}

template<>
Utils::Result<Notification> fromJson<Notification>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Notification");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    Notification result;
    result._method = obj.value("method").toString();
    if (obj.contains("params") && obj["params"].isObject()) {
        const QJsonObject mapObj_params = obj["params"].toObject();
        QMap<QString, QJsonValue> map_params;
        for (auto it = mapObj_params.constBegin(); it != mapObj_params.constEnd(); ++it)
            map_params.insert(it.key(), it.value());
        result._params = map_params;
    }
    return result;
}

QJsonObject toJson(const Notification &data)
{
    QJsonObject obj{{"method", data._method}};
    if (data._params.has_value()) {
        QJsonObject map_params;
        for (auto it = data._params->constBegin(); it != data._params->constEnd(); ++it)
            map_params.insert(it.key(), it.value());
        obj.insert("params", map_params);
    }
    return obj;
}

template<>
Utils::Result<PaginatedRequest> fromJson<PaginatedRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for PaginatedRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    PaginatedRequest result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>(obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    result._method = obj.value("method").toString();
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res1 = fromJson<PaginatedRequestParams>(obj["params"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._params = *res1;
    }
    return result;
}

QJsonObject toJson(const PaginatedRequest &data)
{
    QJsonObject obj{
        {"id", toJsonValue(data._id)},
        {"jsonrpc", QString("2.0")},
        {"method", data._method}
    };
    if (data._params.has_value())
        obj.insert("params", toJson(*data._params));
    return obj;
}

template<>
Utils::Result<PaginatedResult> fromJson<PaginatedResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for PaginatedResult");
    const QJsonObject obj = val.toObject();
    PaginatedResult result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const QJsonObject mapObj__meta = obj["_meta"].toObject();
        QMap<QString, QJsonValue> map__meta;
        for (auto it = mapObj__meta.constBegin(); it != mapObj__meta.constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        result.__meta = map__meta;
    }
    if (obj.contains("nextCursor"))
        result._nextCursor = obj.value("nextCursor").toString();
    return result;
}

QJsonObject toJson(const PaginatedResult &data)
{
    QJsonObject obj;
    if (data.__meta.has_value()) {
        QJsonObject map__meta;
        for (auto it = data.__meta->constBegin(); it != data.__meta->constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        obj.insert("_meta", map__meta);
    }
    if (data._nextCursor.has_value())
        obj.insert("nextCursor", *data._nextCursor);
    return obj;
}

template<>
Utils::Result<PromptListChangedNotification> fromJson<PromptListChangedNotification>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for PromptListChangedNotification");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    PromptListChangedNotification result;
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "notifications/prompts/list_changed")
        return Utils::ResultError("Field 'method' must be 'notifications/prompts/list_changed', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res0 = fromJson<NotificationParams>(obj["params"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._params = *res0;
    }
    return result;
}

QJsonObject toJson(const PromptListChangedNotification &data)
{
    QJsonObject obj{
        {"jsonrpc", QString("2.0")},
        {"method", QString("notifications/prompts/list_changed")}
    };
    if (data._params.has_value())
        obj.insert("params", toJson(*data._params));
    return obj;
}

template<>
Utils::Result<ReadResourceResult> fromJson<ReadResourceResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ReadResourceResult");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("contents"))
        return Utils::ResultError("Missing required field: contents");
    ReadResourceResult result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const QJsonObject mapObj__meta = obj["_meta"].toObject();
        QMap<QString, QJsonValue> map__meta;
        for (auto it = mapObj__meta.constBegin(); it != mapObj__meta.constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        result.__meta = map__meta;
    }
    if (obj.contains("contents") && obj["contents"].isArray()) {
        const QJsonArray arr = obj["contents"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<EmbeddedResourceResource>(v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._contents.append(*res0);
        }
    }
    return result;
}

QJsonObject toJson(const ReadResourceResult &data)
{
    QJsonObject obj;
    if (data.__meta.has_value()) {
        QJsonObject map__meta;
        for (auto it = data.__meta->constBegin(); it != data.__meta->constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        obj.insert("_meta", map__meta);
    }
    QJsonArray arr_contents;
    for (const auto &v : data._contents) arr_contents.append(toJsonValue(v));
    obj.insert("contents", arr_contents);
    return obj;
}

template<>
Utils::Result<RelatedTaskMetadata> fromJson<RelatedTaskMetadata>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for RelatedTaskMetadata");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("taskId"))
        return Utils::ResultError("Missing required field: taskId");
    RelatedTaskMetadata result;
    result._taskId = obj.value("taskId").toString();
    return result;
}

QJsonObject toJson(const RelatedTaskMetadata &data)
{
    QJsonObject obj{{"taskId", data._taskId}};
    return obj;
}

template<>
Utils::Result<Request> fromJson<Request>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Request");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    Request result;
    result._method = obj.value("method").toString();
    if (obj.contains("params") && obj["params"].isObject()) {
        const QJsonObject mapObj_params = obj["params"].toObject();
        QMap<QString, QJsonValue> map_params;
        for (auto it = mapObj_params.constBegin(); it != mapObj_params.constEnd(); ++it)
            map_params.insert(it.key(), it.value());
        result._params = map_params;
    }
    return result;
}

QJsonObject toJson(const Request &data)
{
    QJsonObject obj{{"method", data._method}};
    if (data._params.has_value()) {
        QJsonObject map_params;
        for (auto it = data._params->constBegin(); it != data._params->constEnd(); ++it)
            map_params.insert(it.key(), it.value());
        obj.insert("params", map_params);
    }
    return obj;
}

template<>
Utils::Result<ResourceContents> fromJson<ResourceContents>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ResourceContents");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    ResourceContents result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const QJsonObject mapObj__meta = obj["_meta"].toObject();
        QMap<QString, QJsonValue> map__meta;
        for (auto it = mapObj__meta.constBegin(); it != mapObj__meta.constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        result.__meta = map__meta;
    }
    if (obj.contains("mimeType"))
        result._mimeType = obj.value("mimeType").toString();
    result._uri = obj.value("uri").toString();
    return result;
}

QJsonObject toJson(const ResourceContents &data)
{
    QJsonObject obj{{"uri", data._uri}};
    if (data.__meta.has_value()) {
        QJsonObject map__meta;
        for (auto it = data.__meta->constBegin(); it != data.__meta->constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        obj.insert("_meta", map__meta);
    }
    if (data._mimeType.has_value())
        obj.insert("mimeType", *data._mimeType);
    return obj;
}

template<>
Utils::Result<ResourceListChangedNotification> fromJson<ResourceListChangedNotification>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ResourceListChangedNotification");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    ResourceListChangedNotification result;
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "notifications/resources/list_changed")
        return Utils::ResultError("Field 'method' must be 'notifications/resources/list_changed', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res0 = fromJson<NotificationParams>(obj["params"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._params = *res0;
    }
    return result;
}

QJsonObject toJson(const ResourceListChangedNotification &data)
{
    QJsonObject obj{
        {"jsonrpc", QString("2.0")},
        {"method", QString("notifications/resources/list_changed")}
    };
    if (data._params.has_value())
        obj.insert("params", toJson(*data._params));
    return obj;
}

template<>
Utils::Result<ResourceRequestParams::Meta> fromJson<ResourceRequestParams::Meta>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Meta");
    const QJsonObject obj = val.toObject();
    ResourceRequestParams::Meta result;
    if (obj.contains("progressToken")) {
        const auto res0 = fromJson<ProgressToken>(obj["progressToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._progressToken = *res0;
    }
    return result;
}

QJsonObject toJson(const ResourceRequestParams::Meta &data)
{
    QJsonObject obj;
    if (data._progressToken.has_value())
        obj.insert("progressToken", toJsonValue(*data._progressToken));
    return obj;
}

template<>
Utils::Result<ResourceRequestParams> fromJson<ResourceRequestParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ResourceRequestParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    ResourceRequestParams result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<ResourceRequestParams::Meta>(obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    result._uri = obj.value("uri").toString();
    return result;
}

QJsonObject toJson(const ResourceRequestParams &data)
{
    QJsonObject obj{{"uri", data._uri}};
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
    return obj;
}

template<>
Utils::Result<ResourceUpdatedNotificationParams> fromJson<ResourceUpdatedNotificationParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ResourceUpdatedNotificationParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    ResourceUpdatedNotificationParams result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const QJsonObject mapObj__meta = obj["_meta"].toObject();
        QMap<QString, QJsonValue> map__meta;
        for (auto it = mapObj__meta.constBegin(); it != mapObj__meta.constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        result.__meta = map__meta;
    }
    result._uri = obj.value("uri").toString();
    return result;
}

QJsonObject toJson(const ResourceUpdatedNotificationParams &data)
{
    QJsonObject obj{{"uri", data._uri}};
    if (data.__meta.has_value()) {
        QJsonObject map__meta;
        for (auto it = data.__meta->constBegin(); it != data.__meta->constEnd(); ++it)
            map__meta.insert(it.key(), it.value());
        obj.insert("_meta", map__meta);
    }
    return obj;
}

template<>
Utils::Result<ResourceUpdatedNotification> fromJson<ResourceUpdatedNotification>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ResourceUpdatedNotification");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    if (!obj.contains("params"))
        return Utils::ResultError("Missing required field: params");
    ResourceUpdatedNotification result;
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "notifications/resources/updated")
        return Utils::ResultError("Field 'method' must be 'notifications/resources/updated', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res0 = fromJson<ResourceUpdatedNotificationParams>(obj["params"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._params = *res0;
    }
    return result;
}

QJsonObject toJson(const ResourceUpdatedNotification &data)
{
    QJsonObject obj{
        {"jsonrpc", QString("2.0")},
        {"method", QString("notifications/resources/updated")},
        {"params", toJson(data._params)}
    };
    return obj;
}

template<>
Utils::Result<ToolListChangedNotification> fromJson<ToolListChangedNotification>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ToolListChangedNotification");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    ToolListChangedNotification result;
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "notifications/tools/list_changed")
        return Utils::ResultError("Field 'method' must be 'notifications/tools/list_changed', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res0 = fromJson<NotificationParams>(obj["params"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._params = *res0;
    }
    return result;
}

QJsonObject toJson(const ToolListChangedNotification &data)
{
    QJsonObject obj{
        {"jsonrpc", QString("2.0")},
        {"method", QString("notifications/tools/list_changed")}
    };
    if (data._params.has_value())
        obj.insert("params", toJson(*data._params));
    return obj;
}

template<>
Utils::Result<ServerNotification> fromJson<ServerNotification>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid ServerNotification: expected object");
    const QString dispatchValue = val.toObject().value("method").toString();
    if (dispatchValue == "notifications/cancelled") {
        const auto res0 = fromJson<CancelledNotification>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return ServerNotification(*res0);
    }
    else if (dispatchValue == "notifications/progress") {
        const auto res1 = fromJson<ProgressNotification>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        return ServerNotification(*res1);
    }
    else if (dispatchValue == "notifications/resources/list_changed") {
        const auto res2 = fromJson<ResourceListChangedNotification>(val);
        if (!res2)
            return Utils::ResultError(res2.error());
        return ServerNotification(*res2);
    }
    else if (dispatchValue == "notifications/resources/updated") {
        const auto res3 = fromJson<ResourceUpdatedNotification>(val);
        if (!res3)
            return Utils::ResultError(res3.error());
        return ServerNotification(*res3);
    }
    else if (dispatchValue == "notifications/prompts/list_changed") {
        const auto res4 = fromJson<PromptListChangedNotification>(val);
        if (!res4)
            return Utils::ResultError(res4.error());
        return ServerNotification(*res4);
    }
    else if (dispatchValue == "notifications/tools/list_changed") {
        const auto res5 = fromJson<ToolListChangedNotification>(val);
        if (!res5)
            return Utils::ResultError(res5.error());
        return ServerNotification(*res5);
    }
    else if (dispatchValue == "notifications/tasks/status") {
        const auto res6 = fromJson<TaskStatusNotification>(val);
        if (!res6)
            return Utils::ResultError(res6.error());
        return ServerNotification(*res6);
    }
    else if (dispatchValue == "notifications/message") {
        const auto res7 = fromJson<LoggingMessageNotification>(val);
        if (!res7)
            return Utils::ResultError(res7.error());
        return ServerNotification(*res7);
    }
    else if (dispatchValue == "notifications/elicitation/complete") {
        const auto res8 = fromJson<ElicitationCompleteNotification>(val);
        if (!res8)
            return Utils::ResultError(res8.error());
        return ServerNotification(*res8);
    }
    return Utils::ResultError("Invalid ServerNotification: unknown method \"" + dispatchValue + "\"");
}

QJsonObject toJson(const ServerNotification &val)
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

QJsonValue toJsonValue(const ServerNotification &val)
{
    return toJson(val);
}

QString dispatchValue(const ServerNotification &val)
{
    return std::visit([](const auto &v) -> QString {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, CancelledNotification>) return "notifications/cancelled";
        else if constexpr (std::is_same_v<T, ProgressNotification>) return "notifications/progress";
        else if constexpr (std::is_same_v<T, ResourceListChangedNotification>) return "notifications/resources/list_changed";
        else if constexpr (std::is_same_v<T, ResourceUpdatedNotification>) return "notifications/resources/updated";
        else if constexpr (std::is_same_v<T, PromptListChangedNotification>) return "notifications/prompts/list_changed";
        else if constexpr (std::is_same_v<T, ToolListChangedNotification>) return "notifications/tools/list_changed";
        else if constexpr (std::is_same_v<T, TaskStatusNotification>) return "notifications/tasks/status";
        else if constexpr (std::is_same_v<T, LoggingMessageNotification>) return "notifications/message";
        else if constexpr (std::is_same_v<T, ElicitationCompleteNotification>) return "notifications/elicitation/complete";
        return {};
    }, val);
}

template<>
Utils::Result<ServerRequest> fromJson<ServerRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid ServerRequest: expected object");
    const QString dispatchValue = val.toObject().value("method").toString();
    if (dispatchValue == "ping") {
        const auto res0 = fromJson<PingRequest>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return ServerRequest(*res0);
    }
    else if (dispatchValue == "tasks/get") {
        const auto res1 = fromJson<GetTaskRequest>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        return ServerRequest(*res1);
    }
    else if (dispatchValue == "tasks/result") {
        const auto res2 = fromJson<GetTaskPayloadRequest>(val);
        if (!res2)
            return Utils::ResultError(res2.error());
        return ServerRequest(*res2);
    }
    else if (dispatchValue == "tasks/cancel") {
        const auto res3 = fromJson<CancelTaskRequest>(val);
        if (!res3)
            return Utils::ResultError(res3.error());
        return ServerRequest(*res3);
    }
    else if (dispatchValue == "tasks/list") {
        const auto res4 = fromJson<ListTasksRequest>(val);
        if (!res4)
            return Utils::ResultError(res4.error());
        return ServerRequest(*res4);
    }
    else if (dispatchValue == "sampling/createMessage") {
        const auto res5 = fromJson<CreateMessageRequest>(val);
        if (!res5)
            return Utils::ResultError(res5.error());
        return ServerRequest(*res5);
    }
    else if (dispatchValue == "roots/list") {
        const auto res6 = fromJson<ListRootsRequest>(val);
        if (!res6)
            return Utils::ResultError(res6.error());
        return ServerRequest(*res6);
    }
    else if (dispatchValue == "elicitation/create") {
        const auto res7 = fromJson<ElicitRequest>(val);
        if (!res7)
            return Utils::ResultError(res7.error());
        return ServerRequest(*res7);
    }
    return Utils::ResultError("Invalid ServerRequest: unknown method \"" + dispatchValue + "\"");
}

QJsonObject toJson(const ServerRequest &val)
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

QJsonValue toJsonValue(const ServerRequest &val)
{
    return toJson(val);
}

QString dispatchValue(const ServerRequest &val)
{
    return std::visit([](const auto &v) -> QString {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, PingRequest>) return "ping";
        else if constexpr (std::is_same_v<T, GetTaskRequest>) return "tasks/get";
        else if constexpr (std::is_same_v<T, GetTaskPayloadRequest>) return "tasks/result";
        else if constexpr (std::is_same_v<T, CancelTaskRequest>) return "tasks/cancel";
        else if constexpr (std::is_same_v<T, ListTasksRequest>) return "tasks/list";
        else if constexpr (std::is_same_v<T, CreateMessageRequest>) return "sampling/createMessage";
        else if constexpr (std::is_same_v<T, ListRootsRequest>) return "roots/list";
        else if constexpr (std::is_same_v<T, ElicitRequest>) return "elicitation/create";
        return {};
    }, val);
}

RequestId id(const ServerRequest &val)
{
    return std::visit([](const auto &v) -> RequestId { return v._id; }, val);
}

template<>
Utils::Result<ServerResult> fromJson<ServerResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid ServerResult: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("capabilities")) {
        const auto res0 = fromJson<InitializeResult>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return ServerResult(*res0);
    }
    if (obj.contains("resources")) {
        const auto res1 = fromJson<ListResourcesResult>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        return ServerResult(*res1);
    }
    if (obj.contains("resourceTemplates")) {
        const auto res2 = fromJson<ListResourceTemplatesResult>(val);
        if (!res2)
            return Utils::ResultError(res2.error());
        return ServerResult(*res2);
    }
    if (obj.contains("contents")) {
        const auto res3 = fromJson<ReadResourceResult>(val);
        if (!res3)
            return Utils::ResultError(res3.error());
        return ServerResult(*res3);
    }
    if (obj.contains("prompts")) {
        const auto res4 = fromJson<ListPromptsResult>(val);
        if (!res4)
            return Utils::ResultError(res4.error());
        return ServerResult(*res4);
    }
    if (obj.contains("messages")) {
        const auto res5 = fromJson<GetPromptResult>(val);
        if (!res5)
            return Utils::ResultError(res5.error());
        return ServerResult(*res5);
    }
    if (obj.contains("tools")) {
        const auto res6 = fromJson<ListToolsResult>(val);
        if (!res6)
            return Utils::ResultError(res6.error());
        return ServerResult(*res6);
    }
    if (obj.contains("content")) {
        const auto res7 = fromJson<CallToolResult>(val);
        if (!res7)
            return Utils::ResultError(res7.error());
        return ServerResult(*res7);
    }
    if (obj.contains("tasks")) {
        const auto res8 = fromJson<ListTasksResult>(val);
        if (!res8)
            return Utils::ResultError(res8.error());
        return ServerResult(*res8);
    }
    if (obj.contains("completion")) {
        const auto res9 = fromJson<CompleteResult>(val);
        if (!res9)
            return Utils::ResultError(res9.error());
        return ServerResult(*res9);
    }
    {
        auto result = fromJson<Result>(val);
        if (result) return ServerResult(*result);
    }
    {
        auto result = fromJson<GetTaskResult>(val);
        if (result) return ServerResult(*result);
    }
    {
        auto result = fromJson<GetTaskPayloadResult>(val);
        if (result) return ServerResult(*result);
    }
    {
        auto result = fromJson<CancelTaskResult>(val);
        if (result) return ServerResult(*result);
    }
    return Utils::ResultError("Invalid ServerResult");
}

QJsonObject toJson(const ServerResult &val)
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

QJsonValue toJsonValue(const ServerResult &val)
{
    return toJson(val);
}

template<>
Utils::Result<SingleSelectEnumSchema> fromJson<SingleSelectEnumSchema>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid SingleSelectEnumSchema: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("enum")) {
        const auto res0 = fromJson<UntitledSingleSelectEnumSchema>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return SingleSelectEnumSchema(*res0);
    }
    if (obj.contains("oneOf")) {
        const auto res1 = fromJson<TitledSingleSelectEnumSchema>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        return SingleSelectEnumSchema(*res1);
    }
    return Utils::ResultError("Invalid SingleSelectEnumSchema");
}

QJsonObject toJson(const SingleSelectEnumSchema &val)
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

QJsonValue toJsonValue(const SingleSelectEnumSchema &val)
{
    return toJson(val);
}

template<>
Utils::Result<TaskAugmentedRequestParams::Meta> fromJson<TaskAugmentedRequestParams::Meta>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Meta");
    const QJsonObject obj = val.toObject();
    TaskAugmentedRequestParams::Meta result;
    if (obj.contains("progressToken")) {
        const auto res0 = fromJson<ProgressToken>(obj["progressToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._progressToken = *res0;
    }
    return result;
}

QJsonObject toJson(const TaskAugmentedRequestParams::Meta &data)
{
    QJsonObject obj;
    if (data._progressToken.has_value())
        obj.insert("progressToken", toJsonValue(*data._progressToken));
    return obj;
}

template<>
Utils::Result<TaskAugmentedRequestParams> fromJson<TaskAugmentedRequestParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TaskAugmentedRequestParams");
    const QJsonObject obj = val.toObject();
    TaskAugmentedRequestParams result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<TaskAugmentedRequestParams::Meta>(obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    if (obj.contains("task") && obj["task"].isObject()) {
        const auto res1 = fromJson<TaskMetadata>(obj["task"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._task = *res1;
    }
    return result;
}

QJsonObject toJson(const TaskAugmentedRequestParams &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
    if (data._task.has_value())
        obj.insert("task", toJson(*data._task));
    return obj;
}

template<>
Utils::Result<URLElicitationRequiredError> fromJson<URLElicitationRequiredError>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for URLElicitationRequiredError");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("error"))
        return Utils::ResultError("Missing required field: error");
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    URLElicitationRequiredError result;
    if (obj.contains("error") && obj["error"].isObject()) {
        const auto res0 = fromJson<Error>(obj["error"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._error = *res0;
    }
    if (obj.contains("id")) {
        const auto res1 = fromJson<RequestId>(obj["id"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._id = *res1;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    return result;
}

QJsonObject toJson(const URLElicitationRequiredError &data)
{
    QJsonObject obj{
        {"error", toJson(data._error)},
        {"jsonrpc", QString("2.0")}
    };
    if (data._id.has_value())
        obj.insert("id", toJsonValue(*data._id));
    return obj;
}

} // namespace Mcp::Generated::Schema::_2025_11_25
