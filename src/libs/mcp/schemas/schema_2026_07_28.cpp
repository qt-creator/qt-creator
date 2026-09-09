// This file is auto-generated. Do not edit manually.
#include "schema_2026_07_28.h"

namespace Mcp::Generated::Schema::_2026_07_28 {

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
    if (!val.isString())
        return Utils::ResultError("Expected JSON string for Role");
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
            const auto res0 = fromJson<Role>("audience", v);
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
        const auto res0 = fromJson<MetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    if (obj.contains("annotations") && obj["annotations"].isObject()) {
        const auto res1 = fromJson<Annotations>("annotations", obj["annotations"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._annotations = *res1;
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
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
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
        const auto res0 = fromJson<MetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
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
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
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
    if (!val.isString())
        return Utils::ResultError("Expected JSON string for Icon::Theme");
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
    if (obj.contains("theme")) {
        const auto res0 = fromJson<Icon::Theme>("theme", obj["theme"]);
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
            const auto res0 = fromJson<Icon>("icons", v);
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
Utils::Result<ResultMetaObject> fromJson<ResultMetaObject>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ResultMetaObject");
    const QJsonObject obj = val.toObject();
    ResultMetaObject result;
    if (obj.contains("io.modelcontextprotocol/serverInfo") && obj["io.modelcontextprotocol/serverInfo"].isObject()) {
        const auto res0 = fromJson<Implementation>("io.modelcontextprotocol/serverInfo", obj["io.modelcontextprotocol/serverInfo"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._iodotmodelcontextprotocolslashserverInfo = *res0;
    }
    return result;
}

QJsonObject toJson(const ResultMetaObject &data)
{
    QJsonObject obj;
    if (data._iodotmodelcontextprotocolslashserverInfo.has_value())
        obj.insert("io.modelcontextprotocol/serverInfo", toJson(*data._iodotmodelcontextprotocolslashserverInfo));
    return obj;
}

QString toString(const CacheableResult::CacheScope &v)
{
    switch(v) {
        case CacheableResult::CacheScope::private_: return "private";
        case CacheableResult::CacheScope::public_: return "public";
    }
    return {};
}

template<>
Utils::Result<CacheableResult::CacheScope> fromJson<CacheableResult::CacheScope>(const QJsonValue &val)
{
    if (!val.isString())
        return Utils::ResultError("Expected JSON string for CacheableResult::CacheScope");
    const QString str = val.toString();
    if (str == "private") return CacheableResult::CacheScope::private_;
    if (str == "public") return CacheableResult::CacheScope::public_;
    return Utils::ResultError("Invalid CacheableResult::CacheScope value: " + str);
}

QJsonValue toJsonValue(const CacheableResult::CacheScope &v)
{
    return toString(v);
}

template<>
Utils::Result<CacheableResult> fromJson<CacheableResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CacheableResult");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("cacheScope"))
        return Utils::ResultError("Missing required field: cacheScope");
    if (!obj.contains("resultType"))
        return Utils::ResultError("Missing required field: resultType");
    if (!obj.contains("ttlMs"))
        return Utils::ResultError("Missing required field: ttlMs");
    CacheableResult result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<ResultMetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    const auto res1 = fromJson<CacheableResult::CacheScope>("cacheScope", obj["cacheScope"]);
    if (!res1)
        return Utils::ResultError(res1.error());
    result._cacheScope = *res1;
    result._resultType = obj.value("resultType").toString();
    result._ttlMs = obj.value("ttlMs").toInt();
    return result;
}

QJsonObject toJson(const CacheableResult &data)
{
    QJsonObject obj{
        {"cacheScope", toJsonValue(data._cacheScope)},
        {"resultType", data._resultType},
        {"ttlMs", data._ttlMs}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
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
        const auto res0 = fromJson<MetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    if (obj.contains("annotations") && obj["annotations"].isObject()) {
        const auto res1 = fromJson<Annotations>("annotations", obj["annotations"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._annotations = *res1;
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
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
    if (data._annotations.has_value())
        obj.insert("annotations", toJson(*data._annotations));
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
        const auto res0 = fromJson<MetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    if (obj.contains("annotations") && obj["annotations"].isObject()) {
        const auto res1 = fromJson<Annotations>("annotations", obj["annotations"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._annotations = *res1;
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
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
    if (data._annotations.has_value())
        obj.insert("annotations", toJson(*data._annotations));
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
        const auto res0 = fromJson<MetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
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
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
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
        const auto res0 = fromJson<MetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    if (obj.contains("annotations") && obj["annotations"].isObject()) {
        const auto res1 = fromJson<Annotations>("annotations", obj["annotations"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._annotations = *res1;
    }
    if (obj.contains("resource")) {
        const auto res2 = fromJson<EmbeddedResourceResource>("resource", obj["resource"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._resource = *res2;
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
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
    if (data._annotations.has_value())
        obj.insert("annotations", toJson(*data._annotations));
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
        const auto res0 = fromJson<MetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    if (obj.contains("annotations") && obj["annotations"].isObject()) {
        const auto res1 = fromJson<Annotations>("annotations", obj["annotations"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._annotations = *res1;
    }
    if (obj.contains("description"))
        result._description = obj.value("description").toString();
    if (obj.contains("icons") && obj["icons"].isArray()) {
        const QJsonArray arr = obj["icons"].toArray();
        QList<Icon> list_icons;
        for (const QJsonValue &v : arr) {
            const auto res2 = fromJson<Icon>("icons", v);
            if (!res2)
                return Utils::ResultError(res2.error());
            list_icons.append(*res2);
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
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
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
        const auto res0 = fromJson<MetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    if (obj.contains("content") && obj["content"].isArray()) {
        const QJsonArray arr = obj["content"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res1 = fromJson<ContentBlock>("content", v);
            if (!res1)
                return Utils::ResultError(res1.error());
            result._content.append(*res1);
        }
    }
    if (obj.contains("isError"))
        result._isError = obj.value("isError").toBool();
    if (obj.contains("structuredContent"))
        result._structuredContent = obj.value("structuredContent");
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
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
    QJsonArray arr_content;
    for (const auto &v : data._content) arr_content.append(toJsonValue(v));
    obj.insert("content", arr_content);
    if (data._isError.has_value())
        obj.insert("isError", *data._isError);
    if (data._structuredContent.has_value())
        obj.insert("structuredContent", *data._structuredContent);
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
        const auto res0 = fromJson<MetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
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
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
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
        const auto res0 = fromJson<MetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    if (obj.contains("content")) {
        const auto res1 = fromJson<CreateMessageResultContent>("content", obj["content"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._content = *res1;
    }
    result._model = obj.value("model").toString();
    const auto res2 = fromJson<Role>("role", obj["role"]);
    if (!res2)
        return Utils::ResultError(res2.error());
    result._role = *res2;
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
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
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
    if (!val.isString())
        return Utils::ResultError("Expected JSON string for ElicitResult::Action");
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
    const auto res0 = fromJson<ElicitResult::Action>("action", obj["action"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._action = *res0;
    if (obj.contains("content") && obj["content"].isObject()) {
        const QJsonObject mapObj_content = obj["content"].toObject();
        QMap<QString, ElicitResultContentValue> map_content;
        for (auto it = mapObj_content.constBegin(); it != mapObj_content.constEnd(); ++it) {
            const auto res1 = fromJson<ElicitResultContentValue>("content", it.value());
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
    if (data._content.has_value()) {
        QJsonObject map_content;
        for (auto it = data._content->constBegin(); it != data._content->constEnd(); ++it)
            map_content.insert(it.key(), toJsonValue(it.value()));
        obj.insert("content", map_content);
    }
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
        const auto res0 = fromJson<MetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    if (obj.contains("name"))
        result._name = obj.value("name").toString();
    result._uri = obj.value("uri").toString();
    return result;
}

QJsonObject toJson(const Root &data)
{
    QJsonObject obj{{"uri", data._uri}};
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
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
    if (obj.contains("roots") && obj["roots"].isArray()) {
        const QJsonArray arr = obj["roots"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<Root>("roots", v);
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
    QJsonArray arr_roots;
    for (const auto &v : data._roots) arr_roots.append(toJson(v));
    obj.insert("roots", arr_roots);
    return obj;
}

template<>
Utils::Result<InputResponse> fromJson<InputResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid InputResponse: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("model")) {
        const auto res0 = fromJson<CreateMessageResult>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return InputResponse(*res0);
    }
    if (obj.contains("roots")) {
        const auto res1 = fromJson<ListRootsResult>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        return InputResponse(*res1);
    }
    if (obj.contains("action")) {
        const auto res2 = fromJson<ElicitResult>(val);
        if (!res2)
            return Utils::ResultError(res2.error());
        return InputResponse(*res2);
    }
    return Utils::ResultError("Invalid InputResponse");
}

QJsonObject toJson(const InputResponse &val)
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

QJsonValue toJsonValue(const InputResponse &val)
{
    return toJson(val);
}

template<>
Utils::Result<InputResponses> fromJson<InputResponses>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InputResponses");
    const QJsonObject obj = val.toObject();
    InputResponses result;
    for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
        const auto res0 = fromJson<InputResponse>(it.value());
        if (!res0)
            return Utils::ResultError(res0.error());
        result.insert(it.key(), *res0);
    }
    return result;
}

QJsonObject toJson(const InputResponses &data)
{
    QJsonObject obj;
    for (auto it = data.constBegin(); it != data.constEnd(); ++it)
        obj.insert(it.key(), toJson(it.value()));
    return obj;
}

template<>
Utils::Result<ClientCapabilities::Elicitation> fromJson<ClientCapabilities::Elicitation>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Elicitation");
    const QJsonObject obj = val.toObject();
    ClientCapabilities::Elicitation result;
    if (obj.contains("form"))
        result._form = obj.value("form").toObject();
    if (obj.contains("url"))
        result._url = obj.value("url").toObject();
    return result;
}

QJsonObject toJson(const ClientCapabilities::Elicitation &data)
{
    QJsonObject obj;
    if (data._form.has_value())
        obj.insert("form", *data._form);
    if (data._url.has_value())
        obj.insert("url", *data._url);
    return obj;
}

template<>
Utils::Result<ClientCapabilities::Sampling> fromJson<ClientCapabilities::Sampling>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Sampling");
    const QJsonObject obj = val.toObject();
    ClientCapabilities::Sampling result;
    if (obj.contains("context"))
        result._context = obj.value("context").toObject();
    if (obj.contains("tools"))
        result._tools = obj.value("tools").toObject();
    return result;
}

QJsonObject toJson(const ClientCapabilities::Sampling &data)
{
    QJsonObject obj;
    if (data._context.has_value())
        obj.insert("context", *data._context);
    if (data._tools.has_value())
        obj.insert("tools", *data._tools);
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
        const auto res0 = fromJson<ClientCapabilities::Elicitation>("elicitation", obj["elicitation"]);
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
    if (obj.contains("extensions") && obj["extensions"].isObject()) {
        const QJsonObject mapObj_extensions = obj["extensions"].toObject();
        QMap<QString, QJsonObject> map_extensions;
        for (auto it = mapObj_extensions.constBegin(); it != mapObj_extensions.constEnd(); ++it)
            map_extensions.insert(it.key(), it.value().toObject());
        result._extensions = map_extensions;
    }
    if (obj.contains("roots"))
        result._roots = obj.value("roots").toObject();
    if (obj.contains("sampling") && obj["sampling"].isObject()) {
        const auto res1 = fromJson<ClientCapabilities::Sampling>("sampling", obj["sampling"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._sampling = *res1;
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
    if (data._extensions.has_value()) {
        QJsonObject map_extensions;
        for (auto it = data._extensions->constBegin(); it != data._extensions->constEnd(); ++it)
            map_extensions.insert(it.key(), QJsonValue(it.value()));
        obj.insert("extensions", map_extensions);
    }
    if (data._roots.has_value())
        obj.insert("roots", *data._roots);
    if (data._sampling.has_value())
        obj.insert("sampling", toJson(*data._sampling));
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
    if (!val.isString())
        return Utils::ResultError("Expected JSON string for LoggingLevel");
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
Utils::Result<RequestMetaObject> fromJson<RequestMetaObject>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for RequestMetaObject");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("io.modelcontextprotocol/clientCapabilities"))
        return Utils::ResultError("Missing required field: io.modelcontextprotocol/clientCapabilities");
    if (!obj.contains("io.modelcontextprotocol/protocolVersion"))
        return Utils::ResultError("Missing required field: io.modelcontextprotocol/protocolVersion");
    RequestMetaObject result;
    if (obj.contains("io.modelcontextprotocol/clientCapabilities") && obj["io.modelcontextprotocol/clientCapabilities"].isObject()) {
        const auto res0 = fromJson<ClientCapabilities>("io.modelcontextprotocol/clientCapabilities", obj["io.modelcontextprotocol/clientCapabilities"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._iodotmodelcontextprotocolslashclientCapabilities = *res0;
    }
    if (obj.contains("io.modelcontextprotocol/clientInfo") && obj["io.modelcontextprotocol/clientInfo"].isObject()) {
        const auto res1 = fromJson<Implementation>("io.modelcontextprotocol/clientInfo", obj["io.modelcontextprotocol/clientInfo"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._iodotmodelcontextprotocolslashclientInfo = *res1;
    }
    if (obj.contains("io.modelcontextprotocol/logLevel")) {
        const auto res2 = fromJson<LoggingLevel>("io.modelcontextprotocol/logLevel", obj["io.modelcontextprotocol/logLevel"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._iodotmodelcontextprotocolslashlogLevel = *res2;
    }
    result._iodotmodelcontextprotocolslashprotocolVersion = obj.value("io.modelcontextprotocol/protocolVersion").toString();
    if (obj.contains("progressToken")) {
        const auto res3 = fromJson<ProgressToken>("progressToken", obj["progressToken"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._progressToken = *res3;
    }
    return result;
}

QJsonObject toJson(const RequestMetaObject &data)
{
    QJsonObject obj{
        {"io.modelcontextprotocol/clientCapabilities", toJson(data._iodotmodelcontextprotocolslashclientCapabilities)},
        {"io.modelcontextprotocol/protocolVersion", data._iodotmodelcontextprotocolslashprotocolVersion}
    };
    if (data._iodotmodelcontextprotocolslashclientInfo.has_value())
        obj.insert("io.modelcontextprotocol/clientInfo", toJson(*data._iodotmodelcontextprotocolslashclientInfo));
    if (data._iodotmodelcontextprotocolslashlogLevel.has_value())
        obj.insert("io.modelcontextprotocol/logLevel", toJsonValue(*data._iodotmodelcontextprotocolslashlogLevel));
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
    if (!obj.contains("_meta"))
        return Utils::ResultError("Missing required field: _meta");
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    CallToolRequestParams result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<RequestMetaObject>("_meta", obj["_meta"]);
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
    if (obj.contains("inputResponses") && obj["inputResponses"].isObject()) {
        const auto res1 = fromJson<InputResponses>("inputResponses", obj["inputResponses"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._inputResponses = *res1;
    }
    result._name = obj.value("name").toString();
    if (obj.contains("requestState"))
        result._requestState = obj.value("requestState").toString();
    return result;
}

QJsonObject toJson(const CallToolRequestParams &data)
{
    QJsonObject obj{
        {"_meta", toJson(data.__meta)},
        {"name", data._name}
    };
    if (data._arguments.has_value()) {
        QJsonObject map_arguments;
        for (auto it = data._arguments->constBegin(); it != data._arguments->constEnd(); ++it)
            map_arguments.insert(it.key(), it.value());
        obj.insert("arguments", map_arguments);
    }
    if (data._inputResponses.has_value())
        obj.insert("inputResponses", toJson(*data._inputResponses));
    if (data._requestState.has_value())
        obj.insert("requestState", *data._requestState);
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
        const auto res0 = fromJson<RequestId>("id", obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "tools/call")
        return Utils::ResultError("Field 'method' must be 'tools/call', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res1 = fromJson<CallToolRequestParams>("params", obj["params"]);
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
Utils::Result<CallToolResult> fromJson<CallToolResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CallToolResult");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("content"))
        return Utils::ResultError("Missing required field: content");
    if (!obj.contains("resultType"))
        return Utils::ResultError("Missing required field: resultType");
    CallToolResult result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<ResultMetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    if (obj.contains("content") && obj["content"].isArray()) {
        const QJsonArray arr = obj["content"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res1 = fromJson<ContentBlock>("content", v);
            if (!res1)
                return Utils::ResultError(res1.error());
            result._content.append(*res1);
        }
    }
    if (obj.contains("isError"))
        result._isError = obj.value("isError").toBool();
    result._resultType = obj.value("resultType").toString();
    if (obj.contains("structuredContent"))
        result._structuredContent = obj.value("structuredContent");
    return result;
}

QJsonObject toJson(const CallToolResult &data)
{
    QJsonObject obj{{"resultType", data._resultType}};
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
    QJsonArray arr_content;
    for (const auto &v : data._content) arr_content.append(toJsonValue(v));
    obj.insert("content", arr_content);
    if (data._isError.has_value())
        obj.insert("isError", *data._isError);
    if (data._structuredContent.has_value())
        obj.insert("structuredContent", *data._structuredContent);
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
            const auto res0 = fromJson<ModelHint>("hints", v);
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
        const auto res0 = fromJson<MetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    if (obj.contains("content")) {
        const auto res1 = fromJson<CreateMessageResultContent>("content", obj["content"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._content = *res1;
    }
    const auto res2 = fromJson<Role>("role", obj["role"]);
    if (!res2)
        return Utils::ResultError(res2.error());
    result._role = *res2;
    return result;
}

QJsonObject toJson(const SamplingMessage &data)
{
    QJsonObject obj{
        {"content", toJsonValue(data._content)},
        {"role", toJsonValue(data._role)}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
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
    if (obj.value("type").toString() != "object")
        return Utils::ResultError("Field 'type' must be 'object', got: " + obj.value("type").toString());
    {
        const QSet<QString> knownKeys{"$schema", "type"};
        for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
            if (!knownKeys.contains(it.key()))
                result._additionalProperties.insert(it.key(), it.value());
        }
    }
    return result;
}

QJsonObject toJson(const Tool::InputSchema &data)
{
    QJsonObject obj{{"type", QString("object")}};
    if (data._dollarschema.has_value())
        obj.insert("$schema", *data._dollarschema);
    for (auto it = data._additionalProperties.constBegin(); it != data._additionalProperties.constEnd(); ++it)
        obj.insert(it.key(), it.value());
    return obj;
}

template<>
Utils::Result<Tool::OutputSchema> fromJson<Tool::OutputSchema>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for OutputSchema");
    const QJsonObject obj = val.toObject();
    Tool::OutputSchema result;
    if (obj.contains("$schema"))
        result._dollarschema = obj.value("$schema").toString();
    {
        const QSet<QString> knownKeys{"$schema"};
        for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
            if (!knownKeys.contains(it.key()))
                result._additionalProperties.insert(it.key(), it.value());
        }
    }
    return result;
}

QJsonObject toJson(const Tool::OutputSchema &data)
{
    QJsonObject obj;
    if (data._dollarschema.has_value())
        obj.insert("$schema", *data._dollarschema);
    for (auto it = data._additionalProperties.constBegin(); it != data._additionalProperties.constEnd(); ++it)
        obj.insert(it.key(), it.value());
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
        const auto res0 = fromJson<MetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    if (obj.contains("annotations") && obj["annotations"].isObject()) {
        const auto res1 = fromJson<ToolAnnotations>("annotations", obj["annotations"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._annotations = *res1;
    }
    if (obj.contains("description"))
        result._description = obj.value("description").toString();
    if (obj.contains("icons") && obj["icons"].isArray()) {
        const QJsonArray arr = obj["icons"].toArray();
        QList<Icon> list_icons;
        for (const QJsonValue &v : arr) {
            const auto res2 = fromJson<Icon>("icons", v);
            if (!res2)
                return Utils::ResultError(res2.error());
            list_icons.append(*res2);
        }
        result._icons = list_icons;
    }
    if (obj.contains("inputSchema") && obj["inputSchema"].isObject()) {
        const auto res3 = fromJson<Tool::InputSchema>("inputSchema", obj["inputSchema"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._inputSchema = *res3;
    }
    result._name = obj.value("name").toString();
    if (obj.contains("outputSchema") && obj["outputSchema"].isObject()) {
        const auto res4 = fromJson<Tool::OutputSchema>("outputSchema", obj["outputSchema"]);
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
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
    if (data._annotations.has_value())
        obj.insert("annotations", toJson(*data._annotations));
    if (data._description.has_value())
        obj.insert("description", *data._description);
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
    if (!val.isString())
        return Utils::ResultError("Expected JSON string for ToolChoice::Mode");
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
    if (obj.contains("mode")) {
        const auto res0 = fromJson<ToolChoice::Mode>("mode", obj["mode"]);
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
    if (!val.isString())
        return Utils::ResultError("Expected JSON string for CreateMessageRequestParams::IncludeContext");
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
    if (obj.contains("includeContext")) {
        const auto res0 = fromJson<CreateMessageRequestParams::IncludeContext>("includeContext", obj["includeContext"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._includeContext = *res0;
    }
    result._maxTokens = obj.value("maxTokens").toInt();
    if (obj.contains("messages") && obj["messages"].isArray()) {
        const QJsonArray arr = obj["messages"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res1 = fromJson<SamplingMessage>("messages", v);
            if (!res1)
                return Utils::ResultError(res1.error());
            result._messages.append(*res1);
        }
    }
    if (obj.contains("metadata"))
        result._metadata = obj.value("metadata").toObject();
    if (obj.contains("modelPreferences") && obj["modelPreferences"].isObject()) {
        const auto res2 = fromJson<ModelPreferences>("modelPreferences", obj["modelPreferences"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._modelPreferences = *res2;
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
    if (obj.contains("temperature"))
        result._temperature = obj.value("temperature").toDouble();
    if (obj.contains("toolChoice") && obj["toolChoice"].isObject()) {
        const auto res3 = fromJson<ToolChoice>("toolChoice", obj["toolChoice"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._toolChoice = *res3;
    }
    if (obj.contains("tools") && obj["tools"].isArray()) {
        const QJsonArray arr = obj["tools"].toArray();
        QList<Tool> list_tools;
        for (const QJsonValue &v : arr) {
            const auto res4 = fromJson<Tool>("tools", v);
            if (!res4)
                return Utils::ResultError(res4.error());
            list_tools.append(*res4);
        }
        result._tools = list_tools;
    }
    return result;
}

QJsonObject toJson(const CreateMessageRequestParams &data)
{
    QJsonObject obj{{"maxTokens", data._maxTokens}};
    if (data._includeContext.has_value())
        obj.insert("includeContext", toJsonValue(*data._includeContext));
    QJsonArray arr_messages;
    for (const auto &v : data._messages) arr_messages.append(toJson(v));
    obj.insert("messages", arr_messages);
    if (data._metadata.has_value())
        obj.insert("metadata", *data._metadata);
    if (data._modelPreferences.has_value())
        obj.insert("modelPreferences", toJson(*data._modelPreferences));
    if (data._stopSequences.has_value()) {
        QJsonArray arr_stopSequences;
        for (const auto &v : *data._stopSequences) arr_stopSequences.append(v);
        obj.insert("stopSequences", arr_stopSequences);
    }
    if (data._systemPrompt.has_value())
        obj.insert("systemPrompt", *data._systemPrompt);
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
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    if (!obj.contains("params"))
        return Utils::ResultError("Missing required field: params");
    CreateMessageRequest result;
    if (obj.value("method").toString() != "sampling/createMessage")
        return Utils::ResultError("Field 'method' must be 'sampling/createMessage', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res0 = fromJson<CreateMessageRequestParams>("params", obj["params"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._params = *res0;
    }
    return result;
}

QJsonObject toJson(const CreateMessageRequest &data)
{
    QJsonObject obj{
        {"method", QString("sampling/createMessage")},
        {"params", toJson(data._params)}
    };
    return obj;
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
    if (!val.isString())
        return Utils::ResultError("Expected JSON string for NumberSchema::Type");
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
        result._default_ = obj.value("default").toDouble();
    if (obj.contains("description"))
        result._description = obj.value("description").toString();
    if (obj.contains("maximum"))
        result._maximum = obj.value("maximum").toDouble();
    if (obj.contains("minimum"))
        result._minimum = obj.value("minimum").toDouble();
    if (obj.contains("title"))
        result._title = obj.value("title").toString();
    const auto res0 = fromJson<NumberSchema::Type>("type", obj["type"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._type = *res0;
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
    if (!val.isString())
        return Utils::ResultError("Expected JSON string for StringSchema::Format");
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
    if (obj.contains("format")) {
        const auto res0 = fromJson<StringSchema::Format>("format", obj["format"]);
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
            const auto res0 = fromJson<TitledMultiSelectEnumSchema::Items::AnyOfItem>("anyOf", v);
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
        const auto res0 = fromJson<TitledMultiSelectEnumSchema::Items>("items", obj["items"]);
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
            const auto res0 = fromJson<TitledSingleSelectEnumSchema::OneOfItem>("oneOf", v);
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
        const auto res0 = fromJson<UntitledMultiSelectEnumSchema::Items>("items", obj["items"]);
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
            const auto res0 = fromJson<PrimitiveSchemaDefinition>("properties", it.value());
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
    result._message = obj.value("message").toString();
    if (obj.value("mode").toString() != "form")
        return Utils::ResultError("Field 'mode' must be 'form', got: " + obj.value("mode").toString());
    if (obj.contains("requestedSchema") && obj["requestedSchema"].isObject()) {
        const auto res0 = fromJson<ElicitRequestFormParams::RequestedSchema>("requestedSchema", obj["requestedSchema"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._requestedSchema = *res0;
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
    return obj;
}

template<>
Utils::Result<ElicitRequestURLParams> fromJson<ElicitRequestURLParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ElicitRequestURLParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("message"))
        return Utils::ResultError("Missing required field: message");
    if (!obj.contains("mode"))
        return Utils::ResultError("Missing required field: mode");
    if (!obj.contains("url"))
        return Utils::ResultError("Missing required field: url");
    ElicitRequestURLParams result;
    result._message = obj.value("message").toString();
    if (obj.value("mode").toString() != "url")
        return Utils::ResultError("Field 'mode' must be 'url', got: " + obj.value("mode").toString());
    result._url = obj.value("url").toString();
    return result;
}

QJsonObject toJson(const ElicitRequestURLParams &data)
{
    QJsonObject obj{
        {"message", data._message},
        {"mode", QString("url")},
        {"url", data._url}
    };
    return obj;
}

template<>
Utils::Result<ElicitRequestParams> fromJson<ElicitRequestParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid ElicitRequestParams: expected object");
    const QString dispatchValue = val.toObject().value("mode").toString();
    if (dispatchValue == "form") {
        const auto res0 = fromJson<ElicitRequestFormParams>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return ElicitRequestParams(*res0);
    }
    else if (dispatchValue == "url") {
        const auto res1 = fromJson<ElicitRequestURLParams>(val);
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
        if constexpr (std::is_same_v<T, ElicitRequestFormParams>) return "form";
        else if constexpr (std::is_same_v<T, ElicitRequestURLParams>) return "url";
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
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    if (!obj.contains("params"))
        return Utils::ResultError("Missing required field: params");
    ElicitRequest result;
    if (obj.value("method").toString() != "elicitation/create")
        return Utils::ResultError("Field 'method' must be 'elicitation/create', got: " + obj.value("method").toString());
    if (obj.contains("params")) {
        const auto res0 = fromJson<ElicitRequestParams>("params", obj["params"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._params = *res0;
    }
    return result;
}

QJsonObject toJson(const ElicitRequest &data)
{
    QJsonObject obj{
        {"method", QString("elicitation/create")},
        {"params", toJsonValue(data._params)}
    };
    return obj;
}

template<>
Utils::Result<ListRootsRequest::Params> fromJson<ListRootsRequest::Params>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Params");
    const QJsonObject obj = val.toObject();
    ListRootsRequest::Params result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<MetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    return result;
}

QJsonObject toJson(const ListRootsRequest::Params &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
    return obj;
}

template<>
Utils::Result<ListRootsRequest> fromJson<ListRootsRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ListRootsRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    ListRootsRequest result;
    if (obj.value("method").toString() != "roots/list")
        return Utils::ResultError("Field 'method' must be 'roots/list', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res0 = fromJson<ListRootsRequest::Params>("params", obj["params"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._params = *res0;
    }
    return result;
}

QJsonObject toJson(const ListRootsRequest &data)
{
    QJsonObject obj{{"method", QString("roots/list")}};
    if (data._params.has_value())
        obj.insert("params", toJson(*data._params));
    return obj;
}

template<>
Utils::Result<InputRequest> fromJson<InputRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid InputRequest: expected object");
    const QString dispatchValue = val.toObject().value("method").toString();
    if (dispatchValue == "sampling/createMessage") {
        const auto res0 = fromJson<CreateMessageRequest>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return InputRequest(*res0);
    }
    else if (dispatchValue == "roots/list") {
        const auto res1 = fromJson<ListRootsRequest>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        return InputRequest(*res1);
    }
    else if (dispatchValue == "elicitation/create") {
        const auto res2 = fromJson<ElicitRequest>(val);
        if (!res2)
            return Utils::ResultError(res2.error());
        return InputRequest(*res2);
    }
    return Utils::ResultError("Invalid InputRequest: unknown method \"" + dispatchValue + "\"");
}

QJsonObject toJson(const InputRequest &val)
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

QJsonValue toJsonValue(const InputRequest &val)
{
    return toJson(val);
}

QString dispatchValue(const InputRequest &val)
{
    return std::visit([](const auto &v) -> QString {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, CreateMessageRequest>) return "sampling/createMessage";
        else if constexpr (std::is_same_v<T, ListRootsRequest>) return "roots/list";
        else if constexpr (std::is_same_v<T, ElicitRequest>) return "elicitation/create";
        return {};
    }, val);
}

template<>
Utils::Result<InputRequests> fromJson<InputRequests>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InputRequests");
    const QJsonObject obj = val.toObject();
    InputRequests result;
    for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
        const auto res0 = fromJson<InputRequest>(it.value());
        if (!res0)
            return Utils::ResultError(res0.error());
        result.insert(it.key(), *res0);
    }
    return result;
}

QJsonObject toJson(const InputRequests &data)
{
    QJsonObject obj;
    for (auto it = data.constBegin(); it != data.constEnd(); ++it)
        obj.insert(it.key(), toJson(it.value()));
    return obj;
}

template<>
Utils::Result<InputRequiredResult> fromJson<InputRequiredResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InputRequiredResult");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("resultType"))
        return Utils::ResultError("Missing required field: resultType");
    InputRequiredResult result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<ResultMetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    if (obj.contains("inputRequests") && obj["inputRequests"].isObject()) {
        const auto res1 = fromJson<InputRequests>("inputRequests", obj["inputRequests"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._inputRequests = *res1;
    }
    if (obj.contains("requestState"))
        result._requestState = obj.value("requestState").toString();
    result._resultType = obj.value("resultType").toString();
    return result;
}

QJsonObject toJson(const InputRequiredResult &data)
{
    QJsonObject obj{{"resultType", data._resultType}};
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
    if (data._inputRequests.has_value())
        obj.insert("inputRequests", toJson(*data._inputRequests));
    if (data._requestState.has_value())
        obj.insert("requestState", *data._requestState);
    return obj;
}

template<>
Utils::Result<CallToolResultResponseResult> fromJson<CallToolResultResponseResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid CallToolResultResponseResult: expected object or array");
    const QJsonObject obj = val.toObject();
    if (obj.contains("content")) {
        const auto res0 = fromJson<CallToolResult>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return CallToolResultResponseResult(*res0);
    }
    {
        auto result = fromJson<InputRequiredResult>(val);
        if (result) return CallToolResultResponseResult(*result);
    }
    return Utils::ResultError("Invalid CallToolResultResponseResult");
}

QJsonValue toJsonValue(const CallToolResultResponseResult &val)
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
Utils::Result<CallToolResultResponse> fromJson<CallToolResultResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CallToolResultResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("result"))
        return Utils::ResultError("Missing required field: result");
    CallToolResultResponse result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>("id", obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.contains("result")) {
        const auto res1 = fromJson<CallToolResultResponseResult>("result", obj["result"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._result = *res1;
    }
    return result;
}

QJsonObject toJson(const CallToolResultResponse &data)
{
    QJsonObject obj{
        {"id", toJsonValue(data._id)},
        {"jsonrpc", QString("2.0")},
        {"result", toJsonValue(data._result)}
    };
    return obj;
}

template<>
Utils::Result<NotificationMetaObject> fromJson<NotificationMetaObject>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for NotificationMetaObject");
    const QJsonObject obj = val.toObject();
    NotificationMetaObject result;
    if (obj.contains("io.modelcontextprotocol/subscriptionId")) {
        const auto res0 = fromJson<RequestId>("io.modelcontextprotocol/subscriptionId", obj["io.modelcontextprotocol/subscriptionId"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._iodotmodelcontextprotocolslashsubscriptionId = *res0;
    }
    return result;
}

QJsonObject toJson(const NotificationMetaObject &data)
{
    QJsonObject obj;
    if (data._iodotmodelcontextprotocolslashsubscriptionId.has_value())
        obj.insert("io.modelcontextprotocol/subscriptionId", toJsonValue(*data._iodotmodelcontextprotocolslashsubscriptionId));
    return obj;
}

template<>
Utils::Result<CancelledNotificationParams> fromJson<CancelledNotificationParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CancelledNotificationParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("requestId"))
        return Utils::ResultError("Missing required field: requestId");
    CancelledNotificationParams result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<NotificationMetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    if (obj.contains("reason"))
        result._reason = obj.value("reason").toString();
    if (obj.contains("requestId")) {
        const auto res1 = fromJson<RequestId>("requestId", obj["requestId"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._requestId = *res1;
    }
    return result;
}

QJsonObject toJson(const CancelledNotificationParams &data)
{
    QJsonObject obj{{"requestId", toJsonValue(data._requestId)}};
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
    if (data._reason.has_value())
        obj.insert("reason", *data._reason);
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
        const auto res0 = fromJson<CancelledNotificationParams>("params", obj["params"]);
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
Utils::Result<ClientNotification> fromJson<ClientNotification>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ClientNotification");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    if (!obj.contains("params"))
        return Utils::ResultError("Missing required field: params");
    ClientNotification result;
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "notifications/cancelled")
        return Utils::ResultError("Field 'method' must be 'notifications/cancelled', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res0 = fromJson<CancelledNotificationParams>("params", obj["params"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._params = *res0;
    }
    return result;
}

QJsonObject toJson(const ClientNotification &data)
{
    QJsonObject obj{
        {"jsonrpc", QString("2.0")},
        {"method", QString("notifications/cancelled")},
        {"params", toJson(data._params)}
    };
    return obj;
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
    if (!obj.contains("_meta"))
        return Utils::ResultError("Missing required field: _meta");
    if (!obj.contains("argument"))
        return Utils::ResultError("Missing required field: argument");
    if (!obj.contains("ref"))
        return Utils::ResultError("Missing required field: ref");
    CompleteRequestParams result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<RequestMetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    if (obj.contains("argument") && obj["argument"].isObject()) {
        const auto res1 = fromJson<CompleteRequestParams::Argument>("argument", obj["argument"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._argument = *res1;
    }
    if (obj.contains("context") && obj["context"].isObject()) {
        const auto res2 = fromJson<CompleteRequestParams::Context>("context", obj["context"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._context = *res2;
    }
    if (obj.contains("ref")) {
        const auto res3 = fromJson<CompleteRequestParamsRef>("ref", obj["ref"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._ref = *res3;
    }
    return result;
}

QJsonObject toJson(const CompleteRequestParams &data)
{
    QJsonObject obj{
        {"_meta", toJson(data.__meta)},
        {"argument", toJson(data._argument)},
        {"ref", toJsonValue(data._ref)}
    };
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
        const auto res0 = fromJson<RequestId>("id", obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "completion/complete")
        return Utils::ResultError("Field 'method' must be 'completion/complete', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res1 = fromJson<CompleteRequestParams>("params", obj["params"]);
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
Utils::Result<RequestParams> fromJson<RequestParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for RequestParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("_meta"))
        return Utils::ResultError("Missing required field: _meta");
    RequestParams result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<RequestMetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    return result;
}

QJsonObject toJson(const RequestParams &data)
{
    QJsonObject obj{{"_meta", toJson(data.__meta)}};
    return obj;
}

template<>
Utils::Result<DiscoverRequest> fromJson<DiscoverRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DiscoverRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    if (!obj.contains("params"))
        return Utils::ResultError("Missing required field: params");
    DiscoverRequest result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>("id", obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "server/discover")
        return Utils::ResultError("Field 'method' must be 'server/discover', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res1 = fromJson<RequestParams>("params", obj["params"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._params = *res1;
    }
    return result;
}

QJsonObject toJson(const DiscoverRequest &data)
{
    QJsonObject obj{
        {"id", toJsonValue(data._id)},
        {"jsonrpc", QString("2.0")},
        {"method", QString("server/discover")},
        {"params", toJson(data._params)}
    };
    return obj;
}

template<>
Utils::Result<GetPromptRequestParams> fromJson<GetPromptRequestParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for GetPromptRequestParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("_meta"))
        return Utils::ResultError("Missing required field: _meta");
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    GetPromptRequestParams result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<RequestMetaObject>("_meta", obj["_meta"]);
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
    if (obj.contains("inputResponses") && obj["inputResponses"].isObject()) {
        const auto res1 = fromJson<InputResponses>("inputResponses", obj["inputResponses"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._inputResponses = *res1;
    }
    result._name = obj.value("name").toString();
    if (obj.contains("requestState"))
        result._requestState = obj.value("requestState").toString();
    return result;
}

QJsonObject toJson(const GetPromptRequestParams &data)
{
    QJsonObject obj{
        {"_meta", toJson(data.__meta)},
        {"name", data._name}
    };
    if (data._arguments.has_value()) {
        QJsonObject map_arguments;
        for (auto it = data._arguments->constBegin(); it != data._arguments->constEnd(); ++it)
            map_arguments.insert(it.key(), QJsonValue(it.value()));
        obj.insert("arguments", map_arguments);
    }
    if (data._inputResponses.has_value())
        obj.insert("inputResponses", toJson(*data._inputResponses));
    if (data._requestState.has_value())
        obj.insert("requestState", *data._requestState);
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
        const auto res0 = fromJson<RequestId>("id", obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "prompts/get")
        return Utils::ResultError("Field 'method' must be 'prompts/get', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res1 = fromJson<GetPromptRequestParams>("params", obj["params"]);
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
Utils::Result<PaginatedRequestParams> fromJson<PaginatedRequestParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for PaginatedRequestParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("_meta"))
        return Utils::ResultError("Missing required field: _meta");
    PaginatedRequestParams result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<RequestMetaObject>("_meta", obj["_meta"]);
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
    QJsonObject obj{{"_meta", toJson(data.__meta)}};
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
    if (!obj.contains("params"))
        return Utils::ResultError("Missing required field: params");
    ListPromptsRequest result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>("id", obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "prompts/list")
        return Utils::ResultError("Field 'method' must be 'prompts/list', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res1 = fromJson<PaginatedRequestParams>("params", obj["params"]);
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
        {"method", QString("prompts/list")},
        {"params", toJson(data._params)}
    };
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
    if (!obj.contains("params"))
        return Utils::ResultError("Missing required field: params");
    ListResourceTemplatesRequest result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>("id", obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "resources/templates/list")
        return Utils::ResultError("Field 'method' must be 'resources/templates/list', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res1 = fromJson<PaginatedRequestParams>("params", obj["params"]);
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
        {"method", QString("resources/templates/list")},
        {"params", toJson(data._params)}
    };
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
    if (!obj.contains("params"))
        return Utils::ResultError("Missing required field: params");
    ListResourcesRequest result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>("id", obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "resources/list")
        return Utils::ResultError("Field 'method' must be 'resources/list', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res1 = fromJson<PaginatedRequestParams>("params", obj["params"]);
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
        {"method", QString("resources/list")},
        {"params", toJson(data._params)}
    };
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
    if (!obj.contains("params"))
        return Utils::ResultError("Missing required field: params");
    ListToolsRequest result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>("id", obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "tools/list")
        return Utils::ResultError("Field 'method' must be 'tools/list', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res1 = fromJson<PaginatedRequestParams>("params", obj["params"]);
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
        {"method", QString("tools/list")},
        {"params", toJson(data._params)}
    };
    return obj;
}

template<>
Utils::Result<ReadResourceRequestParams> fromJson<ReadResourceRequestParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ReadResourceRequestParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("_meta"))
        return Utils::ResultError("Missing required field: _meta");
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    ReadResourceRequestParams result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<RequestMetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    if (obj.contains("inputResponses") && obj["inputResponses"].isObject()) {
        const auto res1 = fromJson<InputResponses>("inputResponses", obj["inputResponses"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._inputResponses = *res1;
    }
    if (obj.contains("requestState"))
        result._requestState = obj.value("requestState").toString();
    result._uri = obj.value("uri").toString();
    return result;
}

QJsonObject toJson(const ReadResourceRequestParams &data)
{
    QJsonObject obj{
        {"_meta", toJson(data.__meta)},
        {"uri", data._uri}
    };
    if (data._inputResponses.has_value())
        obj.insert("inputResponses", toJson(*data._inputResponses));
    if (data._requestState.has_value())
        obj.insert("requestState", *data._requestState);
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
        const auto res0 = fromJson<RequestId>("id", obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "resources/read")
        return Utils::ResultError("Field 'method' must be 'resources/read', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res1 = fromJson<ReadResourceRequestParams>("params", obj["params"]);
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

template<>
Utils::Result<SubscriptionFilter> fromJson<SubscriptionFilter>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SubscriptionFilter");
    const QJsonObject obj = val.toObject();
    SubscriptionFilter result;
    if (obj.contains("promptsListChanged"))
        result._promptsListChanged = obj.value("promptsListChanged").toBool();
    if (obj.contains("resourceSubscriptions") && obj["resourceSubscriptions"].isArray()) {
        const QJsonArray arr = obj["resourceSubscriptions"].toArray();
        QStringList list_resourceSubscriptions;
        for (const QJsonValue &v : arr) {
            list_resourceSubscriptions.append(v.toString());
        }
        result._resourceSubscriptions = list_resourceSubscriptions;
    }
    if (obj.contains("resourcesListChanged"))
        result._resourcesListChanged = obj.value("resourcesListChanged").toBool();
    if (obj.contains("toolsListChanged"))
        result._toolsListChanged = obj.value("toolsListChanged").toBool();
    return result;
}

QJsonObject toJson(const SubscriptionFilter &data)
{
    QJsonObject obj;
    if (data._promptsListChanged.has_value())
        obj.insert("promptsListChanged", *data._promptsListChanged);
    if (data._resourceSubscriptions.has_value()) {
        QJsonArray arr_resourceSubscriptions;
        for (const auto &v : *data._resourceSubscriptions) arr_resourceSubscriptions.append(v);
        obj.insert("resourceSubscriptions", arr_resourceSubscriptions);
    }
    if (data._resourcesListChanged.has_value())
        obj.insert("resourcesListChanged", *data._resourcesListChanged);
    if (data._toolsListChanged.has_value())
        obj.insert("toolsListChanged", *data._toolsListChanged);
    return obj;
}

template<>
Utils::Result<SubscriptionsListenRequestParams> fromJson<SubscriptionsListenRequestParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SubscriptionsListenRequestParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("_meta"))
        return Utils::ResultError("Missing required field: _meta");
    if (!obj.contains("notifications"))
        return Utils::ResultError("Missing required field: notifications");
    SubscriptionsListenRequestParams result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<RequestMetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    if (obj.contains("notifications") && obj["notifications"].isObject()) {
        const auto res1 = fromJson<SubscriptionFilter>("notifications", obj["notifications"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._notifications = *res1;
    }
    return result;
}

QJsonObject toJson(const SubscriptionsListenRequestParams &data)
{
    QJsonObject obj{
        {"_meta", toJson(data.__meta)},
        {"notifications", toJson(data._notifications)}
    };
    return obj;
}

template<>
Utils::Result<SubscriptionsListenRequest> fromJson<SubscriptionsListenRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SubscriptionsListenRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    if (!obj.contains("params"))
        return Utils::ResultError("Missing required field: params");
    SubscriptionsListenRequest result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>("id", obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "subscriptions/listen")
        return Utils::ResultError("Field 'method' must be 'subscriptions/listen', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res1 = fromJson<SubscriptionsListenRequestParams>("params", obj["params"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._params = *res1;
    }
    return result;
}

QJsonObject toJson(const SubscriptionsListenRequest &data)
{
    QJsonObject obj{
        {"id", toJsonValue(data._id)},
        {"jsonrpc", QString("2.0")},
        {"method", QString("subscriptions/listen")},
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
    if (dispatchValue == "server/discover") {
        const auto res0 = fromJson<DiscoverRequest>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return ClientRequest(*res0);
    }
    else if (dispatchValue == "resources/list") {
        const auto res1 = fromJson<ListResourcesRequest>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        return ClientRequest(*res1);
    }
    else if (dispatchValue == "resources/templates/list") {
        const auto res2 = fromJson<ListResourceTemplatesRequest>(val);
        if (!res2)
            return Utils::ResultError(res2.error());
        return ClientRequest(*res2);
    }
    else if (dispatchValue == "resources/read") {
        const auto res3 = fromJson<ReadResourceRequest>(val);
        if (!res3)
            return Utils::ResultError(res3.error());
        return ClientRequest(*res3);
    }
    else if (dispatchValue == "subscriptions/listen") {
        const auto res4 = fromJson<SubscriptionsListenRequest>(val);
        if (!res4)
            return Utils::ResultError(res4.error());
        return ClientRequest(*res4);
    }
    else if (dispatchValue == "prompts/list") {
        const auto res5 = fromJson<ListPromptsRequest>(val);
        if (!res5)
            return Utils::ResultError(res5.error());
        return ClientRequest(*res5);
    }
    else if (dispatchValue == "prompts/get") {
        const auto res6 = fromJson<GetPromptRequest>(val);
        if (!res6)
            return Utils::ResultError(res6.error());
        return ClientRequest(*res6);
    }
    else if (dispatchValue == "tools/list") {
        const auto res7 = fromJson<ListToolsRequest>(val);
        if (!res7)
            return Utils::ResultError(res7.error());
        return ClientRequest(*res7);
    }
    else if (dispatchValue == "tools/call") {
        const auto res8 = fromJson<CallToolRequest>(val);
        if (!res8)
            return Utils::ResultError(res8.error());
        return ClientRequest(*res8);
    }
    else if (dispatchValue == "completion/complete") {
        const auto res9 = fromJson<CompleteRequest>(val);
        if (!res9)
            return Utils::ResultError(res9.error());
        return ClientRequest(*res9);
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
        if constexpr (std::is_same_v<T, DiscoverRequest>) return "server/discover";
        else if constexpr (std::is_same_v<T, ListResourcesRequest>) return "resources/list";
        else if constexpr (std::is_same_v<T, ListResourceTemplatesRequest>) return "resources/templates/list";
        else if constexpr (std::is_same_v<T, ReadResourceRequest>) return "resources/read";
        else if constexpr (std::is_same_v<T, SubscriptionsListenRequest>) return "subscriptions/listen";
        else if constexpr (std::is_same_v<T, ListPromptsRequest>) return "prompts/list";
        else if constexpr (std::is_same_v<T, GetPromptRequest>) return "prompts/get";
        else if constexpr (std::is_same_v<T, ListToolsRequest>) return "tools/list";
        else if constexpr (std::is_same_v<T, CallToolRequest>) return "tools/call";
        else if constexpr (std::is_same_v<T, CompleteRequest>) return "completion/complete";
        return {};
    }, val);
}

RequestId id(const ClientRequest &val)
{
    return std::visit([](const auto &v) -> RequestId { return v._id; }, val);
}

template<>
Utils::Result<Result> fromJson<Result>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Result");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("resultType"))
        return Utils::ResultError("Missing required field: resultType");
    Result result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<ResultMetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    result._resultType = obj.value("resultType").toString();
    {
        const QSet<QString> knownKeys{"_meta", "resultType"};
        for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
            if (!knownKeys.contains(it.key()))
                result._additionalProperties.insert(it.key(), it.value());
        }
    }
    return result;
}

QJsonObject toJson(const Result &data)
{
    QJsonObject obj{{"resultType", data._resultType}};
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
    for (auto it = data._additionalProperties.constBegin(); it != data._additionalProperties.constEnd(); ++it)
        obj.insert(it.key(), it.value());
    return obj;
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
    if (!obj.contains("resultType"))
        return Utils::ResultError("Missing required field: resultType");
    CompleteResult result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<ResultMetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    if (obj.contains("completion") && obj["completion"].isObject()) {
        const auto res1 = fromJson<CompleteResult::Completion>("completion", obj["completion"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._completion = *res1;
    }
    result._resultType = obj.value("resultType").toString();
    return result;
}

QJsonObject toJson(const CompleteResult &data)
{
    QJsonObject obj{
        {"completion", toJson(data._completion)},
        {"resultType", data._resultType}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
    return obj;
}

template<>
Utils::Result<CompleteResultResponse> fromJson<CompleteResultResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CompleteResultResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("result"))
        return Utils::ResultError("Missing required field: result");
    CompleteResultResponse result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>("id", obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.contains("result") && obj["result"].isObject()) {
        const auto res1 = fromJson<CompleteResult>("result", obj["result"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._result = *res1;
    }
    return result;
}

QJsonObject toJson(const CompleteResultResponse &data)
{
    QJsonObject obj{
        {"id", toJsonValue(data._id)},
        {"jsonrpc", QString("2.0")},
        {"result", toJson(data._result)}
    };
    return obj;
}

template<> Utils::Result<Cursor> fromJson<Cursor>(const QJsonValue &val)
{
    if (!val.isString()) return Utils::ResultError("Expected string");
    return val.toString();
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
    if (obj.contains("completions"))
        result._completions = obj.value("completions").toObject();
    if (obj.contains("experimental") && obj["experimental"].isObject()) {
        const QJsonObject mapObj_experimental = obj["experimental"].toObject();
        QMap<QString, QJsonObject> map_experimental;
        for (auto it = mapObj_experimental.constBegin(); it != mapObj_experimental.constEnd(); ++it)
            map_experimental.insert(it.key(), it.value().toObject());
        result._experimental = map_experimental;
    }
    if (obj.contains("extensions") && obj["extensions"].isObject()) {
        const QJsonObject mapObj_extensions = obj["extensions"].toObject();
        QMap<QString, QJsonObject> map_extensions;
        for (auto it = mapObj_extensions.constBegin(); it != mapObj_extensions.constEnd(); ++it)
            map_extensions.insert(it.key(), it.value().toObject());
        result._extensions = map_extensions;
    }
    if (obj.contains("logging"))
        result._logging = obj.value("logging").toObject();
    if (obj.contains("prompts") && obj["prompts"].isObject()) {
        const auto res0 = fromJson<ServerCapabilities::Prompts>("prompts", obj["prompts"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._prompts = *res0;
    }
    if (obj.contains("resources") && obj["resources"].isObject()) {
        const auto res1 = fromJson<ServerCapabilities::Resources>("resources", obj["resources"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._resources = *res1;
    }
    if (obj.contains("tools") && obj["tools"].isObject()) {
        const auto res2 = fromJson<ServerCapabilities::Tools>("tools", obj["tools"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._tools = *res2;
    }
    return result;
}

QJsonObject toJson(const ServerCapabilities &data)
{
    QJsonObject obj;
    if (data._completions.has_value())
        obj.insert("completions", *data._completions);
    if (data._experimental.has_value()) {
        QJsonObject map_experimental;
        for (auto it = data._experimental->constBegin(); it != data._experimental->constEnd(); ++it)
            map_experimental.insert(it.key(), QJsonValue(it.value()));
        obj.insert("experimental", map_experimental);
    }
    if (data._extensions.has_value()) {
        QJsonObject map_extensions;
        for (auto it = data._extensions->constBegin(); it != data._extensions->constEnd(); ++it)
            map_extensions.insert(it.key(), QJsonValue(it.value()));
        obj.insert("extensions", map_extensions);
    }
    if (data._logging.has_value())
        obj.insert("logging", *data._logging);
    if (data._prompts.has_value())
        obj.insert("prompts", toJson(*data._prompts));
    if (data._resources.has_value())
        obj.insert("resources", toJson(*data._resources));
    if (data._tools.has_value())
        obj.insert("tools", toJson(*data._tools));
    return obj;
}

QString toString(const DiscoverResult::CacheScope &v)
{
    switch(v) {
        case DiscoverResult::CacheScope::private_: return "private";
        case DiscoverResult::CacheScope::public_: return "public";
    }
    return {};
}

template<>
Utils::Result<DiscoverResult::CacheScope> fromJson<DiscoverResult::CacheScope>(const QJsonValue &val)
{
    if (!val.isString())
        return Utils::ResultError("Expected JSON string for DiscoverResult::CacheScope");
    const QString str = val.toString();
    if (str == "private") return DiscoverResult::CacheScope::private_;
    if (str == "public") return DiscoverResult::CacheScope::public_;
    return Utils::ResultError("Invalid DiscoverResult::CacheScope value: " + str);
}

QJsonValue toJsonValue(const DiscoverResult::CacheScope &v)
{
    return toString(v);
}

template<>
Utils::Result<DiscoverResult> fromJson<DiscoverResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DiscoverResult");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("cacheScope"))
        return Utils::ResultError("Missing required field: cacheScope");
    if (!obj.contains("capabilities"))
        return Utils::ResultError("Missing required field: capabilities");
    if (!obj.contains("resultType"))
        return Utils::ResultError("Missing required field: resultType");
    if (!obj.contains("supportedVersions"))
        return Utils::ResultError("Missing required field: supportedVersions");
    if (!obj.contains("ttlMs"))
        return Utils::ResultError("Missing required field: ttlMs");
    DiscoverResult result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<ResultMetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    const auto res1 = fromJson<DiscoverResult::CacheScope>("cacheScope", obj["cacheScope"]);
    if (!res1)
        return Utils::ResultError(res1.error());
    result._cacheScope = *res1;
    if (obj.contains("capabilities") && obj["capabilities"].isObject()) {
        const auto res2 = fromJson<ServerCapabilities>("capabilities", obj["capabilities"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._capabilities = *res2;
    }
    if (obj.contains("instructions"))
        result._instructions = obj.value("instructions").toString();
    result._resultType = obj.value("resultType").toString();
    if (obj.contains("supportedVersions") && obj["supportedVersions"].isArray()) {
        const QJsonArray arr = obj["supportedVersions"].toArray();
        for (const QJsonValue &v : arr) {
            result._supportedVersions.append(v.toString());
        }
    }
    result._ttlMs = obj.value("ttlMs").toInt();
    return result;
}

QJsonObject toJson(const DiscoverResult &data)
{
    QJsonObject obj{
        {"cacheScope", toJsonValue(data._cacheScope)},
        {"capabilities", toJson(data._capabilities)},
        {"resultType", data._resultType},
        {"ttlMs", data._ttlMs}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
    if (data._instructions.has_value())
        obj.insert("instructions", *data._instructions);
    QJsonArray arr_supportedVersions;
    for (const auto &v : data._supportedVersions) arr_supportedVersions.append(v);
    obj.insert("supportedVersions", arr_supportedVersions);
    return obj;
}

template<>
Utils::Result<DiscoverResultResponse> fromJson<DiscoverResultResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DiscoverResultResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("result"))
        return Utils::ResultError("Missing required field: result");
    DiscoverResultResponse result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>("id", obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.contains("result") && obj["result"].isObject()) {
        const auto res1 = fromJson<DiscoverResult>("result", obj["result"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._result = *res1;
    }
    return result;
}

QJsonObject toJson(const DiscoverResultResponse &data)
{
    QJsonObject obj{
        {"id", toJsonValue(data._id)},
        {"jsonrpc", QString("2.0")},
        {"result", toJson(data._result)}
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
        const auto res0 = fromJson<ContentBlock>("content", obj["content"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._content = *res0;
    }
    const auto res1 = fromJson<Role>("role", obj["role"]);
    if (!res1)
        return Utils::ResultError(res1.error());
    result._role = *res1;
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
    if (!obj.contains("resultType"))
        return Utils::ResultError("Missing required field: resultType");
    GetPromptResult result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<ResultMetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    if (obj.contains("description"))
        result._description = obj.value("description").toString();
    if (obj.contains("messages") && obj["messages"].isArray()) {
        const QJsonArray arr = obj["messages"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res1 = fromJson<PromptMessage>("messages", v);
            if (!res1)
                return Utils::ResultError(res1.error());
            result._messages.append(*res1);
        }
    }
    result._resultType = obj.value("resultType").toString();
    return result;
}

QJsonObject toJson(const GetPromptResult &data)
{
    QJsonObject obj{{"resultType", data._resultType}};
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
    if (data._description.has_value())
        obj.insert("description", *data._description);
    QJsonArray arr_messages;
    for (const auto &v : data._messages) arr_messages.append(toJson(v));
    obj.insert("messages", arr_messages);
    return obj;
}

template<>
Utils::Result<GetPromptResultResponseResult> fromJson<GetPromptResultResponseResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid GetPromptResultResponseResult: expected object or array");
    const QJsonObject obj = val.toObject();
    if (obj.contains("messages")) {
        const auto res0 = fromJson<GetPromptResult>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return GetPromptResultResponseResult(*res0);
    }
    {
        auto result = fromJson<InputRequiredResult>(val);
        if (result) return GetPromptResultResponseResult(*result);
    }
    return Utils::ResultError("Invalid GetPromptResultResponseResult");
}

QJsonValue toJsonValue(const GetPromptResultResponseResult &val)
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
Utils::Result<GetPromptResultResponse> fromJson<GetPromptResultResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for GetPromptResultResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("result"))
        return Utils::ResultError("Missing required field: result");
    GetPromptResultResponse result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>("id", obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.contains("result")) {
        const auto res1 = fromJson<GetPromptResultResponseResult>("result", obj["result"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._result = *res1;
    }
    return result;
}

QJsonObject toJson(const GetPromptResultResponse &data)
{
    QJsonObject obj{
        {"id", toJsonValue(data._id)},
        {"jsonrpc", QString("2.0")},
        {"result", toJsonValue(data._result)}
    };
    return obj;
}

template<>
Utils::Result<HeaderMismatchError> fromJson<HeaderMismatchError>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for HeaderMismatchError");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("error"))
        return Utils::ResultError("Missing required field: error");
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    HeaderMismatchError result;
    if (obj.contains("error") && obj["error"].isObject()) {
        const auto res0 = fromJson<Error>("error", obj["error"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._error = *res0;
    }
    if (obj.contains("id")) {
        const auto res1 = fromJson<RequestId>("id", obj["id"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._id = *res1;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    return result;
}

QJsonObject toJson(const HeaderMismatchError &data)
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
            const auto res0 = fromJson<Icon>("icons", v);
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
Utils::Result<InputResponseRequestParams> fromJson<InputResponseRequestParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InputResponseRequestParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("_meta"))
        return Utils::ResultError("Missing required field: _meta");
    InputResponseRequestParams result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<RequestMetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    if (obj.contains("inputResponses") && obj["inputResponses"].isObject()) {
        const auto res1 = fromJson<InputResponses>("inputResponses", obj["inputResponses"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._inputResponses = *res1;
    }
    if (obj.contains("requestState"))
        result._requestState = obj.value("requestState").toString();
    return result;
}

QJsonObject toJson(const InputResponseRequestParams &data)
{
    QJsonObject obj{{"_meta", toJson(data.__meta)}};
    if (data._inputResponses.has_value())
        obj.insert("inputResponses", toJson(*data._inputResponses));
    if (data._requestState.has_value())
        obj.insert("requestState", *data._requestState);
    return obj;
}

template<>
Utils::Result<InternalError> fromJson<InternalError>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InternalError");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("code"))
        return Utils::ResultError("Missing required field: code");
    if (!obj.contains("message"))
        return Utils::ResultError("Missing required field: message");
    InternalError result;
    result._code = obj.value("code").toInt();
    if (obj.contains("data"))
        result._data = obj.value("data");
    result._message = obj.value("message").toString();
    return result;
}

QJsonObject toJson(const InternalError &data)
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
Utils::Result<InvalidParamsError> fromJson<InvalidParamsError>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InvalidParamsError");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("code"))
        return Utils::ResultError("Missing required field: code");
    if (!obj.contains("message"))
        return Utils::ResultError("Missing required field: message");
    InvalidParamsError result;
    result._code = obj.value("code").toInt();
    if (obj.contains("data"))
        result._data = obj.value("data");
    result._message = obj.value("message").toString();
    return result;
}

QJsonObject toJson(const InvalidParamsError &data)
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
Utils::Result<InvalidRequestError> fromJson<InvalidRequestError>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InvalidRequestError");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("code"))
        return Utils::ResultError("Missing required field: code");
    if (!obj.contains("message"))
        return Utils::ResultError("Missing required field: message");
    InvalidRequestError result;
    result._code = obj.value("code").toInt();
    if (obj.contains("data"))
        result._data = obj.value("data");
    result._message = obj.value("message").toString();
    return result;
}

QJsonObject toJson(const InvalidRequestError &data)
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
        const auto res0 = fromJson<Error>("error", obj["error"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._error = *res0;
    }
    if (obj.contains("id")) {
        const auto res1 = fromJson<RequestId>("id", obj["id"]);
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
        const auto res0 = fromJson<RequestId>("id", obj["id"]);
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
        const auto res0 = fromJson<RequestId>("id", obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.contains("result") && obj["result"].isObject()) {
        const auto res1 = fromJson<Result>("result", obj["result"]);
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
        const auto res0 = fromJson<MetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    if (obj.contains("arguments") && obj["arguments"].isArray()) {
        const QJsonArray arr = obj["arguments"].toArray();
        QList<PromptArgument> list_arguments;
        for (const QJsonValue &v : arr) {
            const auto res1 = fromJson<PromptArgument>("arguments", v);
            if (!res1)
                return Utils::ResultError(res1.error());
            list_arguments.append(*res1);
        }
        result._arguments = list_arguments;
    }
    if (obj.contains("description"))
        result._description = obj.value("description").toString();
    if (obj.contains("icons") && obj["icons"].isArray()) {
        const QJsonArray arr = obj["icons"].toArray();
        QList<Icon> list_icons;
        for (const QJsonValue &v : arr) {
            const auto res2 = fromJson<Icon>("icons", v);
            if (!res2)
                return Utils::ResultError(res2.error());
            list_icons.append(*res2);
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
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
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

QString toString(const ListPromptsResult::CacheScope &v)
{
    switch(v) {
        case ListPromptsResult::CacheScope::private_: return "private";
        case ListPromptsResult::CacheScope::public_: return "public";
    }
    return {};
}

template<>
Utils::Result<ListPromptsResult::CacheScope> fromJson<ListPromptsResult::CacheScope>(const QJsonValue &val)
{
    if (!val.isString())
        return Utils::ResultError("Expected JSON string for ListPromptsResult::CacheScope");
    const QString str = val.toString();
    if (str == "private") return ListPromptsResult::CacheScope::private_;
    if (str == "public") return ListPromptsResult::CacheScope::public_;
    return Utils::ResultError("Invalid ListPromptsResult::CacheScope value: " + str);
}

QJsonValue toJsonValue(const ListPromptsResult::CacheScope &v)
{
    return toString(v);
}

template<>
Utils::Result<ListPromptsResult> fromJson<ListPromptsResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ListPromptsResult");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("cacheScope"))
        return Utils::ResultError("Missing required field: cacheScope");
    if (!obj.contains("prompts"))
        return Utils::ResultError("Missing required field: prompts");
    if (!obj.contains("resultType"))
        return Utils::ResultError("Missing required field: resultType");
    if (!obj.contains("ttlMs"))
        return Utils::ResultError("Missing required field: ttlMs");
    ListPromptsResult result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<ResultMetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    const auto res1 = fromJson<ListPromptsResult::CacheScope>("cacheScope", obj["cacheScope"]);
    if (!res1)
        return Utils::ResultError(res1.error());
    result._cacheScope = *res1;
    if (obj.contains("nextCursor"))
        result._nextCursor = obj.value("nextCursor").toString();
    if (obj.contains("prompts") && obj["prompts"].isArray()) {
        const QJsonArray arr = obj["prompts"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res2 = fromJson<Prompt>("prompts", v);
            if (!res2)
                return Utils::ResultError(res2.error());
            result._prompts.append(*res2);
        }
    }
    result._resultType = obj.value("resultType").toString();
    result._ttlMs = obj.value("ttlMs").toInt();
    return result;
}

QJsonObject toJson(const ListPromptsResult &data)
{
    QJsonObject obj{
        {"cacheScope", toJsonValue(data._cacheScope)},
        {"resultType", data._resultType},
        {"ttlMs", data._ttlMs}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
    if (data._nextCursor.has_value())
        obj.insert("nextCursor", *data._nextCursor);
    QJsonArray arr_prompts;
    for (const auto &v : data._prompts) arr_prompts.append(toJson(v));
    obj.insert("prompts", arr_prompts);
    return obj;
}

template<>
Utils::Result<ListPromptsResultResponse> fromJson<ListPromptsResultResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ListPromptsResultResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("result"))
        return Utils::ResultError("Missing required field: result");
    ListPromptsResultResponse result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>("id", obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.contains("result") && obj["result"].isObject()) {
        const auto res1 = fromJson<ListPromptsResult>("result", obj["result"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._result = *res1;
    }
    return result;
}

QJsonObject toJson(const ListPromptsResultResponse &data)
{
    QJsonObject obj{
        {"id", toJsonValue(data._id)},
        {"jsonrpc", QString("2.0")},
        {"result", toJson(data._result)}
    };
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
        const auto res0 = fromJson<MetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    if (obj.contains("annotations") && obj["annotations"].isObject()) {
        const auto res1 = fromJson<Annotations>("annotations", obj["annotations"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._annotations = *res1;
    }
    if (obj.contains("description"))
        result._description = obj.value("description").toString();
    if (obj.contains("icons") && obj["icons"].isArray()) {
        const QJsonArray arr = obj["icons"].toArray();
        QList<Icon> list_icons;
        for (const QJsonValue &v : arr) {
            const auto res2 = fromJson<Icon>("icons", v);
            if (!res2)
                return Utils::ResultError(res2.error());
            list_icons.append(*res2);
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
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
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

QString toString(const ListResourceTemplatesResult::CacheScope &v)
{
    switch(v) {
        case ListResourceTemplatesResult::CacheScope::private_: return "private";
        case ListResourceTemplatesResult::CacheScope::public_: return "public";
    }
    return {};
}

template<>
Utils::Result<ListResourceTemplatesResult::CacheScope> fromJson<ListResourceTemplatesResult::CacheScope>(const QJsonValue &val)
{
    if (!val.isString())
        return Utils::ResultError("Expected JSON string for ListResourceTemplatesResult::CacheScope");
    const QString str = val.toString();
    if (str == "private") return ListResourceTemplatesResult::CacheScope::private_;
    if (str == "public") return ListResourceTemplatesResult::CacheScope::public_;
    return Utils::ResultError("Invalid ListResourceTemplatesResult::CacheScope value: " + str);
}

QJsonValue toJsonValue(const ListResourceTemplatesResult::CacheScope &v)
{
    return toString(v);
}

template<>
Utils::Result<ListResourceTemplatesResult> fromJson<ListResourceTemplatesResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ListResourceTemplatesResult");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("cacheScope"))
        return Utils::ResultError("Missing required field: cacheScope");
    if (!obj.contains("resourceTemplates"))
        return Utils::ResultError("Missing required field: resourceTemplates");
    if (!obj.contains("resultType"))
        return Utils::ResultError("Missing required field: resultType");
    if (!obj.contains("ttlMs"))
        return Utils::ResultError("Missing required field: ttlMs");
    ListResourceTemplatesResult result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<ResultMetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    const auto res1 = fromJson<ListResourceTemplatesResult::CacheScope>("cacheScope", obj["cacheScope"]);
    if (!res1)
        return Utils::ResultError(res1.error());
    result._cacheScope = *res1;
    if (obj.contains("nextCursor"))
        result._nextCursor = obj.value("nextCursor").toString();
    if (obj.contains("resourceTemplates") && obj["resourceTemplates"].isArray()) {
        const QJsonArray arr = obj["resourceTemplates"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res2 = fromJson<ResourceTemplate>("resourceTemplates", v);
            if (!res2)
                return Utils::ResultError(res2.error());
            result._resourceTemplates.append(*res2);
        }
    }
    result._resultType = obj.value("resultType").toString();
    result._ttlMs = obj.value("ttlMs").toInt();
    return result;
}

QJsonObject toJson(const ListResourceTemplatesResult &data)
{
    QJsonObject obj{
        {"cacheScope", toJsonValue(data._cacheScope)},
        {"resultType", data._resultType},
        {"ttlMs", data._ttlMs}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
    if (data._nextCursor.has_value())
        obj.insert("nextCursor", *data._nextCursor);
    QJsonArray arr_resourceTemplates;
    for (const auto &v : data._resourceTemplates) arr_resourceTemplates.append(toJson(v));
    obj.insert("resourceTemplates", arr_resourceTemplates);
    return obj;
}

template<>
Utils::Result<ListResourceTemplatesResultResponse> fromJson<ListResourceTemplatesResultResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ListResourceTemplatesResultResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("result"))
        return Utils::ResultError("Missing required field: result");
    ListResourceTemplatesResultResponse result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>("id", obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.contains("result") && obj["result"].isObject()) {
        const auto res1 = fromJson<ListResourceTemplatesResult>("result", obj["result"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._result = *res1;
    }
    return result;
}

QJsonObject toJson(const ListResourceTemplatesResultResponse &data)
{
    QJsonObject obj{
        {"id", toJsonValue(data._id)},
        {"jsonrpc", QString("2.0")},
        {"result", toJson(data._result)}
    };
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
        const auto res0 = fromJson<MetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    if (obj.contains("annotations") && obj["annotations"].isObject()) {
        const auto res1 = fromJson<Annotations>("annotations", obj["annotations"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._annotations = *res1;
    }
    if (obj.contains("description"))
        result._description = obj.value("description").toString();
    if (obj.contains("icons") && obj["icons"].isArray()) {
        const QJsonArray arr = obj["icons"].toArray();
        QList<Icon> list_icons;
        for (const QJsonValue &v : arr) {
            const auto res2 = fromJson<Icon>("icons", v);
            if (!res2)
                return Utils::ResultError(res2.error());
            list_icons.append(*res2);
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
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
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

QString toString(const ListResourcesResult::CacheScope &v)
{
    switch(v) {
        case ListResourcesResult::CacheScope::private_: return "private";
        case ListResourcesResult::CacheScope::public_: return "public";
    }
    return {};
}

template<>
Utils::Result<ListResourcesResult::CacheScope> fromJson<ListResourcesResult::CacheScope>(const QJsonValue &val)
{
    if (!val.isString())
        return Utils::ResultError("Expected JSON string for ListResourcesResult::CacheScope");
    const QString str = val.toString();
    if (str == "private") return ListResourcesResult::CacheScope::private_;
    if (str == "public") return ListResourcesResult::CacheScope::public_;
    return Utils::ResultError("Invalid ListResourcesResult::CacheScope value: " + str);
}

QJsonValue toJsonValue(const ListResourcesResult::CacheScope &v)
{
    return toString(v);
}

template<>
Utils::Result<ListResourcesResult> fromJson<ListResourcesResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ListResourcesResult");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("cacheScope"))
        return Utils::ResultError("Missing required field: cacheScope");
    if (!obj.contains("resources"))
        return Utils::ResultError("Missing required field: resources");
    if (!obj.contains("resultType"))
        return Utils::ResultError("Missing required field: resultType");
    if (!obj.contains("ttlMs"))
        return Utils::ResultError("Missing required field: ttlMs");
    ListResourcesResult result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<ResultMetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    const auto res1 = fromJson<ListResourcesResult::CacheScope>("cacheScope", obj["cacheScope"]);
    if (!res1)
        return Utils::ResultError(res1.error());
    result._cacheScope = *res1;
    if (obj.contains("nextCursor"))
        result._nextCursor = obj.value("nextCursor").toString();
    if (obj.contains("resources") && obj["resources"].isArray()) {
        const QJsonArray arr = obj["resources"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res2 = fromJson<Resource>("resources", v);
            if (!res2)
                return Utils::ResultError(res2.error());
            result._resources.append(*res2);
        }
    }
    result._resultType = obj.value("resultType").toString();
    result._ttlMs = obj.value("ttlMs").toInt();
    return result;
}

QJsonObject toJson(const ListResourcesResult &data)
{
    QJsonObject obj{
        {"cacheScope", toJsonValue(data._cacheScope)},
        {"resultType", data._resultType},
        {"ttlMs", data._ttlMs}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
    if (data._nextCursor.has_value())
        obj.insert("nextCursor", *data._nextCursor);
    QJsonArray arr_resources;
    for (const auto &v : data._resources) arr_resources.append(toJson(v));
    obj.insert("resources", arr_resources);
    return obj;
}

template<>
Utils::Result<ListResourcesResultResponse> fromJson<ListResourcesResultResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ListResourcesResultResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("result"))
        return Utils::ResultError("Missing required field: result");
    ListResourcesResultResponse result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>("id", obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.contains("result") && obj["result"].isObject()) {
        const auto res1 = fromJson<ListResourcesResult>("result", obj["result"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._result = *res1;
    }
    return result;
}

QJsonObject toJson(const ListResourcesResultResponse &data)
{
    QJsonObject obj{
        {"id", toJsonValue(data._id)},
        {"jsonrpc", QString("2.0")},
        {"result", toJson(data._result)}
    };
    return obj;
}

QString toString(const ListToolsResult::CacheScope &v)
{
    switch(v) {
        case ListToolsResult::CacheScope::private_: return "private";
        case ListToolsResult::CacheScope::public_: return "public";
    }
    return {};
}

template<>
Utils::Result<ListToolsResult::CacheScope> fromJson<ListToolsResult::CacheScope>(const QJsonValue &val)
{
    if (!val.isString())
        return Utils::ResultError("Expected JSON string for ListToolsResult::CacheScope");
    const QString str = val.toString();
    if (str == "private") return ListToolsResult::CacheScope::private_;
    if (str == "public") return ListToolsResult::CacheScope::public_;
    return Utils::ResultError("Invalid ListToolsResult::CacheScope value: " + str);
}

QJsonValue toJsonValue(const ListToolsResult::CacheScope &v)
{
    return toString(v);
}

template<>
Utils::Result<ListToolsResult> fromJson<ListToolsResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ListToolsResult");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("cacheScope"))
        return Utils::ResultError("Missing required field: cacheScope");
    if (!obj.contains("resultType"))
        return Utils::ResultError("Missing required field: resultType");
    if (!obj.contains("tools"))
        return Utils::ResultError("Missing required field: tools");
    if (!obj.contains("ttlMs"))
        return Utils::ResultError("Missing required field: ttlMs");
    ListToolsResult result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<ResultMetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    const auto res1 = fromJson<ListToolsResult::CacheScope>("cacheScope", obj["cacheScope"]);
    if (!res1)
        return Utils::ResultError(res1.error());
    result._cacheScope = *res1;
    if (obj.contains("nextCursor"))
        result._nextCursor = obj.value("nextCursor").toString();
    result._resultType = obj.value("resultType").toString();
    if (obj.contains("tools") && obj["tools"].isArray()) {
        const QJsonArray arr = obj["tools"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res2 = fromJson<Tool>("tools", v);
            if (!res2)
                return Utils::ResultError(res2.error());
            result._tools.append(*res2);
        }
    }
    result._ttlMs = obj.value("ttlMs").toInt();
    return result;
}

QJsonObject toJson(const ListToolsResult &data)
{
    QJsonObject obj{
        {"cacheScope", toJsonValue(data._cacheScope)},
        {"resultType", data._resultType},
        {"ttlMs", data._ttlMs}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
    if (data._nextCursor.has_value())
        obj.insert("nextCursor", *data._nextCursor);
    QJsonArray arr_tools;
    for (const auto &v : data._tools) arr_tools.append(toJson(v));
    obj.insert("tools", arr_tools);
    return obj;
}

template<>
Utils::Result<ListToolsResultResponse> fromJson<ListToolsResultResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ListToolsResultResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("result"))
        return Utils::ResultError("Missing required field: result");
    ListToolsResultResponse result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>("id", obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.contains("result") && obj["result"].isObject()) {
        const auto res1 = fromJson<ListToolsResult>("result", obj["result"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._result = *res1;
    }
    return result;
}

QJsonObject toJson(const ListToolsResultResponse &data)
{
    QJsonObject obj{
        {"id", toJsonValue(data._id)},
        {"jsonrpc", QString("2.0")},
        {"result", toJson(data._result)}
    };
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
        const auto res0 = fromJson<NotificationMetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    result._data = obj.value("data");
    const auto res1 = fromJson<LoggingLevel>("level", obj["level"]);
    if (!res1)
        return Utils::ResultError(res1.error());
    result._level = *res1;
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
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
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
        const auto res0 = fromJson<LoggingMessageNotificationParams>("params", obj["params"]);
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
Utils::Result<MethodNotFoundError> fromJson<MethodNotFoundError>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for MethodNotFoundError");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("code"))
        return Utils::ResultError("Missing required field: code");
    if (!obj.contains("message"))
        return Utils::ResultError("Missing required field: message");
    MethodNotFoundError result;
    result._code = obj.value("code").toInt();
    if (obj.contains("data"))
        result._data = obj.value("data");
    result._message = obj.value("message").toString();
    return result;
}

QJsonObject toJson(const MethodNotFoundError &data)
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
Utils::Result<MissingRequiredClientCapabilityError> fromJson<MissingRequiredClientCapabilityError>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for MissingRequiredClientCapabilityError");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("error"))
        return Utils::ResultError("Missing required field: error");
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    MissingRequiredClientCapabilityError result;
    if (obj.contains("error") && obj["error"].isObject()) {
        const auto res0 = fromJson<Error>("error", obj["error"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._error = *res0;
    }
    if (obj.contains("id")) {
        const auto res1 = fromJson<RequestId>("id", obj["id"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._id = *res1;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    return result;
}

QJsonObject toJson(const MissingRequiredClientCapabilityError &data)
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
Utils::Result<NotificationParams> fromJson<NotificationParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for NotificationParams");
    const QJsonObject obj = val.toObject();
    NotificationParams result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<NotificationMetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    return result;
}

QJsonObject toJson(const NotificationParams &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
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
    if (!obj.contains("params"))
        return Utils::ResultError("Missing required field: params");
    PaginatedRequest result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>("id", obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    result._method = obj.value("method").toString();
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res1 = fromJson<PaginatedRequestParams>("params", obj["params"]);
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
        {"method", data._method},
        {"params", toJson(data._params)}
    };
    return obj;
}

template<>
Utils::Result<PaginatedResult> fromJson<PaginatedResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for PaginatedResult");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("resultType"))
        return Utils::ResultError("Missing required field: resultType");
    PaginatedResult result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<ResultMetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    if (obj.contains("nextCursor"))
        result._nextCursor = obj.value("nextCursor").toString();
    result._resultType = obj.value("resultType").toString();
    return result;
}

QJsonObject toJson(const PaginatedResult &data)
{
    QJsonObject obj{{"resultType", data._resultType}};
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
    if (data._nextCursor.has_value())
        obj.insert("nextCursor", *data._nextCursor);
    return obj;
}

template<>
Utils::Result<ParseError> fromJson<ParseError>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ParseError");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("code"))
        return Utils::ResultError("Missing required field: code");
    if (!obj.contains("message"))
        return Utils::ResultError("Missing required field: message");
    ParseError result;
    result._code = obj.value("code").toInt();
    if (obj.contains("data"))
        result._data = obj.value("data");
    result._message = obj.value("message").toString();
    return result;
}

QJsonObject toJson(const ParseError &data)
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
        const auto res0 = fromJson<NotificationMetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    if (obj.contains("message"))
        result._message = obj.value("message").toString();
    result._progress = obj.value("progress").toDouble();
    if (obj.contains("progressToken")) {
        const auto res1 = fromJson<ProgressToken>("progressToken", obj["progressToken"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._progressToken = *res1;
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
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
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
        const auto res0 = fromJson<ProgressNotificationParams>("params", obj["params"]);
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
        const auto res0 = fromJson<NotificationParams>("params", obj["params"]);
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

QString toString(const ReadResourceResult::CacheScope &v)
{
    switch(v) {
        case ReadResourceResult::CacheScope::private_: return "private";
        case ReadResourceResult::CacheScope::public_: return "public";
    }
    return {};
}

template<>
Utils::Result<ReadResourceResult::CacheScope> fromJson<ReadResourceResult::CacheScope>(const QJsonValue &val)
{
    if (!val.isString())
        return Utils::ResultError("Expected JSON string for ReadResourceResult::CacheScope");
    const QString str = val.toString();
    if (str == "private") return ReadResourceResult::CacheScope::private_;
    if (str == "public") return ReadResourceResult::CacheScope::public_;
    return Utils::ResultError("Invalid ReadResourceResult::CacheScope value: " + str);
}

QJsonValue toJsonValue(const ReadResourceResult::CacheScope &v)
{
    return toString(v);
}

template<>
Utils::Result<ReadResourceResult> fromJson<ReadResourceResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ReadResourceResult");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("cacheScope"))
        return Utils::ResultError("Missing required field: cacheScope");
    if (!obj.contains("contents"))
        return Utils::ResultError("Missing required field: contents");
    if (!obj.contains("resultType"))
        return Utils::ResultError("Missing required field: resultType");
    if (!obj.contains("ttlMs"))
        return Utils::ResultError("Missing required field: ttlMs");
    ReadResourceResult result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<ResultMetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    const auto res1 = fromJson<ReadResourceResult::CacheScope>("cacheScope", obj["cacheScope"]);
    if (!res1)
        return Utils::ResultError(res1.error());
    result._cacheScope = *res1;
    if (obj.contains("contents") && obj["contents"].isArray()) {
        const QJsonArray arr = obj["contents"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res2 = fromJson<EmbeddedResourceResource>("contents", v);
            if (!res2)
                return Utils::ResultError(res2.error());
            result._contents.append(*res2);
        }
    }
    result._resultType = obj.value("resultType").toString();
    result._ttlMs = obj.value("ttlMs").toInt();
    return result;
}

QJsonObject toJson(const ReadResourceResult &data)
{
    QJsonObject obj{
        {"cacheScope", toJsonValue(data._cacheScope)},
        {"resultType", data._resultType},
        {"ttlMs", data._ttlMs}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
    QJsonArray arr_contents;
    for (const auto &v : data._contents) arr_contents.append(toJsonValue(v));
    obj.insert("contents", arr_contents);
    return obj;
}

template<>
Utils::Result<ReadResourceResultResponseResult> fromJson<ReadResourceResultResponseResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid ReadResourceResultResponseResult: expected object or array");
    const QJsonObject obj = val.toObject();
    if (obj.contains("cacheScope")) {
        const auto res0 = fromJson<ReadResourceResult>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return ReadResourceResultResponseResult(*res0);
    }
    {
        auto result = fromJson<InputRequiredResult>(val);
        if (result) return ReadResourceResultResponseResult(*result);
    }
    return Utils::ResultError("Invalid ReadResourceResultResponseResult");
}

QJsonValue toJsonValue(const ReadResourceResultResponseResult &val)
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
Utils::Result<ReadResourceResultResponse> fromJson<ReadResourceResultResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ReadResourceResultResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("result"))
        return Utils::ResultError("Missing required field: result");
    ReadResourceResultResponse result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>("id", obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.contains("result")) {
        const auto res1 = fromJson<ReadResourceResultResponseResult>("result", obj["result"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._result = *res1;
    }
    return result;
}

QJsonObject toJson(const ReadResourceResultResponse &data)
{
    QJsonObject obj{
        {"id", toJsonValue(data._id)},
        {"jsonrpc", QString("2.0")},
        {"result", toJsonValue(data._result)}
    };
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
        const auto res0 = fromJson<MetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    if (obj.contains("mimeType"))
        result._mimeType = obj.value("mimeType").toString();
    result._uri = obj.value("uri").toString();
    return result;
}

QJsonObject toJson(const ResourceContents &data)
{
    QJsonObject obj{{"uri", data._uri}};
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
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
        const auto res0 = fromJson<NotificationParams>("params", obj["params"]);
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
Utils::Result<ResourceRequestParams> fromJson<ResourceRequestParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ResourceRequestParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("_meta"))
        return Utils::ResultError("Missing required field: _meta");
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    ResourceRequestParams result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<RequestMetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    result._uri = obj.value("uri").toString();
    return result;
}

QJsonObject toJson(const ResourceRequestParams &data)
{
    QJsonObject obj{
        {"_meta", toJson(data.__meta)},
        {"uri", data._uri}
    };
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
        const auto res0 = fromJson<NotificationMetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    result._uri = obj.value("uri").toString();
    return result;
}

QJsonObject toJson(const ResourceUpdatedNotificationParams &data)
{
    QJsonObject obj{{"uri", data._uri}};
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
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
        const auto res0 = fromJson<ResourceUpdatedNotificationParams>("params", obj["params"]);
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
Utils::Result<SubscriptionsAcknowledgedNotificationParams> fromJson<SubscriptionsAcknowledgedNotificationParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SubscriptionsAcknowledgedNotificationParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("notifications"))
        return Utils::ResultError("Missing required field: notifications");
    SubscriptionsAcknowledgedNotificationParams result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<NotificationMetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    if (obj.contains("notifications") && obj["notifications"].isObject()) {
        const auto res1 = fromJson<SubscriptionFilter>("notifications", obj["notifications"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._notifications = *res1;
    }
    return result;
}

QJsonObject toJson(const SubscriptionsAcknowledgedNotificationParams &data)
{
    QJsonObject obj{{"notifications", toJson(data._notifications)}};
    if (data.__meta.has_value())
        obj.insert("_meta", toJson(*data.__meta));
    return obj;
}

template<>
Utils::Result<SubscriptionsAcknowledgedNotification> fromJson<SubscriptionsAcknowledgedNotification>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SubscriptionsAcknowledgedNotification");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    if (!obj.contains("params"))
        return Utils::ResultError("Missing required field: params");
    SubscriptionsAcknowledgedNotification result;
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "notifications/subscriptions/acknowledged")
        return Utils::ResultError("Field 'method' must be 'notifications/subscriptions/acknowledged', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject()) {
        const auto res0 = fromJson<SubscriptionsAcknowledgedNotificationParams>("params", obj["params"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._params = *res0;
    }
    return result;
}

QJsonObject toJson(const SubscriptionsAcknowledgedNotification &data)
{
    QJsonObject obj{
        {"jsonrpc", QString("2.0")},
        {"method", QString("notifications/subscriptions/acknowledged")},
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
        const auto res0 = fromJson<NotificationParams>("params", obj["params"]);
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
    else if (dispatchValue == "notifications/subscriptions/acknowledged") {
        const auto res3 = fromJson<SubscriptionsAcknowledgedNotification>(val);
        if (!res3)
            return Utils::ResultError(res3.error());
        return ServerNotification(*res3);
    }
    else if (dispatchValue == "notifications/resources/updated") {
        const auto res4 = fromJson<ResourceUpdatedNotification>(val);
        if (!res4)
            return Utils::ResultError(res4.error());
        return ServerNotification(*res4);
    }
    else if (dispatchValue == "notifications/prompts/list_changed") {
        const auto res5 = fromJson<PromptListChangedNotification>(val);
        if (!res5)
            return Utils::ResultError(res5.error());
        return ServerNotification(*res5);
    }
    else if (dispatchValue == "notifications/tools/list_changed") {
        const auto res6 = fromJson<ToolListChangedNotification>(val);
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
        else if constexpr (std::is_same_v<T, SubscriptionsAcknowledgedNotification>) return "notifications/subscriptions/acknowledged";
        else if constexpr (std::is_same_v<T, ResourceUpdatedNotification>) return "notifications/resources/updated";
        else if constexpr (std::is_same_v<T, PromptListChangedNotification>) return "notifications/prompts/list_changed";
        else if constexpr (std::is_same_v<T, ToolListChangedNotification>) return "notifications/tools/list_changed";
        else if constexpr (std::is_same_v<T, LoggingMessageNotification>) return "notifications/message";
        return {};
    }, val);
}

template<>
Utils::Result<SubscriptionsListenResultMetaObject> fromJson<SubscriptionsListenResultMetaObject>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SubscriptionsListenResultMetaObject");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("io.modelcontextprotocol/subscriptionId"))
        return Utils::ResultError("Missing required field: io.modelcontextprotocol/subscriptionId");
    SubscriptionsListenResultMetaObject result;
    if (obj.contains("io.modelcontextprotocol/serverInfo") && obj["io.modelcontextprotocol/serverInfo"].isObject()) {
        const auto res0 = fromJson<Implementation>("io.modelcontextprotocol/serverInfo", obj["io.modelcontextprotocol/serverInfo"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._iodotmodelcontextprotocolslashserverInfo = *res0;
    }
    if (obj.contains("io.modelcontextprotocol/subscriptionId")) {
        const auto res1 = fromJson<RequestId>("io.modelcontextprotocol/subscriptionId", obj["io.modelcontextprotocol/subscriptionId"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._iodotmodelcontextprotocolslashsubscriptionId = *res1;
    }
    return result;
}

QJsonObject toJson(const SubscriptionsListenResultMetaObject &data)
{
    QJsonObject obj{{"io.modelcontextprotocol/subscriptionId", toJsonValue(data._iodotmodelcontextprotocolslashsubscriptionId)}};
    if (data._iodotmodelcontextprotocolslashserverInfo.has_value())
        obj.insert("io.modelcontextprotocol/serverInfo", toJson(*data._iodotmodelcontextprotocolslashserverInfo));
    return obj;
}

template<>
Utils::Result<SubscriptionsListenResult> fromJson<SubscriptionsListenResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SubscriptionsListenResult");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("_meta"))
        return Utils::ResultError("Missing required field: _meta");
    if (!obj.contains("resultType"))
        return Utils::ResultError("Missing required field: resultType");
    SubscriptionsListenResult result;
    if (obj.contains("_meta") && obj["_meta"].isObject()) {
        const auto res0 = fromJson<SubscriptionsListenResultMetaObject>("_meta", obj["_meta"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.__meta = *res0;
    }
    result._resultType = obj.value("resultType").toString();
    return result;
}

QJsonObject toJson(const SubscriptionsListenResult &data)
{
    QJsonObject obj{
        {"_meta", toJson(data.__meta)},
        {"resultType", data._resultType}
    };
    return obj;
}

template<>
Utils::Result<ServerResult> fromJson<ServerResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid ServerResult: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("capabilities")) {
        const auto res0 = fromJson<DiscoverResult>(val);
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
    if (obj.contains("completion")) {
        const auto res8 = fromJson<CompleteResult>(val);
        if (!res8)
            return Utils::ResultError(res8.error());
        return ServerResult(*res8);
    }
    {
        auto result = fromJson<Result>(val);
        if (result) return ServerResult(*result);
    }
    {
        auto result = fromJson<InputRequiredResult>(val);
        if (result) return ServerResult(*result);
    }
    {
        auto result = fromJson<SubscriptionsListenResult>(val);
        if (result) return ServerResult(*result);
    }
    return Utils::ResultError("Invalid ServerResult");
}

QString resultType(const ServerResult &val)
{
    return std::visit([](const auto &v) -> QString { return v._resultType; }, val);
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
Utils::Result<SubscriptionsListenResultResponse> fromJson<SubscriptionsListenResultResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SubscriptionsListenResultResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("result"))
        return Utils::ResultError("Missing required field: result");
    SubscriptionsListenResultResponse result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>("id", obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.contains("result") && obj["result"].isObject()) {
        const auto res1 = fromJson<SubscriptionsListenResult>("result", obj["result"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._result = *res1;
    }
    return result;
}

QJsonObject toJson(const SubscriptionsListenResultResponse &data)
{
    QJsonObject obj{
        {"id", toJsonValue(data._id)},
        {"jsonrpc", QString("2.0")},
        {"result", toJson(data._result)}
    };
    return obj;
}

template<>
Utils::Result<UnsupportedProtocolVersionError> fromJson<UnsupportedProtocolVersionError>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for UnsupportedProtocolVersionError");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("error"))
        return Utils::ResultError("Missing required field: error");
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    UnsupportedProtocolVersionError result;
    if (obj.contains("error") && obj["error"].isObject()) {
        const auto res0 = fromJson<Error>("error", obj["error"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._error = *res0;
    }
    if (obj.contains("id")) {
        const auto res1 = fromJson<RequestId>("id", obj["id"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._id = *res1;
    }
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    return result;
}

QJsonObject toJson(const UnsupportedProtocolVersionError &data)
{
    QJsonObject obj{
        {"error", toJson(data._error)},
        {"jsonrpc", QString("2.0")}
    };
    if (data._id.has_value())
        obj.insert("id", toJsonValue(*data._id));
    return obj;
}

} // namespace Mcp::Generated::Schema::_2026_07_28
