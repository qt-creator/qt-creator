// This file is auto-generated. Do not edit manually.
#include "acp.h"

namespace Acp {

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

template<>
Utils::Result<ElicitationRequestScope> fromJson<ElicitationRequestScope>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ElicitationRequestScope");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("requestId"))
        return Utils::ResultError("Missing required field: requestId");
    ElicitationRequestScope result;
    if (obj.contains("requestId")) {
        const auto res0 = fromJson<RequestId>(obj["requestId"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._requestId = *res0;
    }
    return result;
}

QJsonObject toJson(const ElicitationRequestScope &data)
{
    QJsonObject obj{{"requestId", toJsonValue(data._requestId)}};
    return obj;
}

template<>
Utils::Result<BooleanPropertySchema> fromJson<BooleanPropertySchema>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for BooleanPropertySchema");
    const QJsonObject obj = val.toObject();
    BooleanPropertySchema result;
    if (obj.contains("title"))
        if (!obj["title"].isNull()) {
            result._title = obj.value("title").toString();
        }
    if (obj.contains("description"))
        if (!obj["description"].isNull()) {
            result._description = obj.value("description").toString();
        }
    if (obj.contains("default"))
        if (!obj["default"].isNull()) {
            result._default_ = obj.value("default").toBool();
        }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const BooleanPropertySchema &data)
{
    QJsonObject obj;
    if (data._title.has_value())
        obj.insert("title", *data._title);
    if (data._description.has_value())
        obj.insert("description", *data._description);
    if (data._default_.has_value())
        obj.insert("default", *data._default_);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<IntegerPropertySchema> fromJson<IntegerPropertySchema>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for IntegerPropertySchema");
    const QJsonObject obj = val.toObject();
    IntegerPropertySchema result;
    if (obj.contains("title"))
        if (!obj["title"].isNull()) {
            result._title = obj.value("title").toString();
        }
    if (obj.contains("description"))
        if (!obj["description"].isNull()) {
            result._description = obj.value("description").toString();
        }
    if (obj.contains("minimum"))
        if (!obj["minimum"].isNull()) {
            result._minimum = obj.value("minimum").toInt();
        }
    if (obj.contains("maximum"))
        if (!obj["maximum"].isNull()) {
            result._maximum = obj.value("maximum").toInt();
        }
    if (obj.contains("default"))
        if (!obj["default"].isNull()) {
            result._default_ = obj.value("default").toInt();
        }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const IntegerPropertySchema &data)
{
    QJsonObject obj;
    if (data._title.has_value())
        obj.insert("title", *data._title);
    if (data._description.has_value())
        obj.insert("description", *data._description);
    if (data._minimum.has_value())
        obj.insert("minimum", *data._minimum);
    if (data._maximum.has_value())
        obj.insert("maximum", *data._maximum);
    if (data._default_.has_value())
        obj.insert("default", *data._default_);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<StringMultiSelectItems> fromJson<StringMultiSelectItems>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for StringMultiSelectItems");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("enum"))
        return Utils::ResultError("Missing required field: enum");
    StringMultiSelectItems result;
    if (obj.contains("enum") && obj["enum"].isArray()) {
        const QJsonArray arr = obj["enum"].toArray();
        for (const QJsonValue &v : arr) {
            result._enum_.append(v.toString());
        }
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const StringMultiSelectItems &data)
{
    QJsonObject obj;
    QJsonArray arr_enum_;
    for (const auto &v : data._enum_) arr_enum_.append(v);
    obj.insert("enum", arr_enum_);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<EnumOption> fromJson<EnumOption>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for EnumOption");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("const"))
        return Utils::ResultError("Missing required field: const");
    if (!obj.contains("title"))
        return Utils::ResultError("Missing required field: title");
    EnumOption result;
    result._const_ = obj.value("const").toString();
    result._title = obj.value("title").toString();
    if (obj.contains("description"))
        if (!obj["description"].isNull()) {
            result._description = obj.value("description").toString();
        }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const EnumOption &data)
{
    QJsonObject obj{
        {"const", data._const_},
        {"title", data._title}
    };
    if (data._description.has_value())
        obj.insert("description", *data._description);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<TitledMultiSelectItems> fromJson<TitledMultiSelectItems>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TitledMultiSelectItems");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("anyOf"))
        return Utils::ResultError("Missing required field: anyOf");
    TitledMultiSelectItems result;
    if (obj.contains("anyOf") && obj["anyOf"].isArray()) {
        const QJsonArray arr = obj["anyOf"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<EnumOption>(v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._anyOf.append(*res0);
        }
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const TitledMultiSelectItems &data)
{
    QJsonObject obj;
    QJsonArray arr_anyOf;
    for (const auto &v : data._anyOf) arr_anyOf.append(toJson(v));
    obj.insert("anyOf", arr_anyOf);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<MultiSelectItems> fromJson<MultiSelectItems>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid MultiSelectItems: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("enum")) {
        const auto res0 = fromJson<StringMultiSelectItems>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return MultiSelectItems(*res0);
    }
    if (obj.contains("anyOf")) {
        const auto res1 = fromJson<TitledMultiSelectItems>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        return MultiSelectItems(*res1);
    }
    if (val.isObject())
        return MultiSelectItems(val.toObject());  // open union: preserve unknown variants raw
    return Utils::ResultError("Invalid MultiSelectItems");
}

QJsonObject toJson(const MultiSelectItems &val)
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

QJsonValue toJsonValue(const MultiSelectItems &val)
{
    return toJson(val);
}

template<>
Utils::Result<MultiSelectPropertySchema> fromJson<MultiSelectPropertySchema>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for MultiSelectPropertySchema");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("items"))
        return Utils::ResultError("Missing required field: items");
    MultiSelectPropertySchema result;
    if (obj.contains("title"))
        if (!obj["title"].isNull()) {
            result._title = obj.value("title").toString();
        }
    if (obj.contains("description"))
        if (!obj["description"].isNull()) {
            result._description = obj.value("description").toString();
        }
    if (obj.contains("minItems"))
        if (!obj["minItems"].isNull()) {
            result._minItems = obj.value("minItems").toInt();
        }
    if (obj.contains("maxItems"))
        if (!obj["maxItems"].isNull()) {
            result._maxItems = obj.value("maxItems").toInt();
        }
    if (obj.contains("items")) {
        const auto res0 = fromJson<MultiSelectItems>(obj["items"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._items = *res0;
    }
    if (obj.contains("default"))
        if (!obj["default"].isNull()) {
            result._default_ = obj.value("default").toArray();
        }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const MultiSelectPropertySchema &data)
{
    QJsonObject obj{{"items", toJsonValue(data._items)}};
    if (data._title.has_value())
        obj.insert("title", *data._title);
    if (data._description.has_value())
        obj.insert("description", *data._description);
    if (data._minItems.has_value())
        obj.insert("minItems", *data._minItems);
    if (data._maxItems.has_value())
        obj.insert("maxItems", *data._maxItems);
    if (data._default_.has_value())
        obj.insert("default", *data._default_);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<NumberPropertySchema> fromJson<NumberPropertySchema>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for NumberPropertySchema");
    const QJsonObject obj = val.toObject();
    NumberPropertySchema result;
    if (obj.contains("title"))
        if (!obj["title"].isNull()) {
            result._title = obj.value("title").toString();
        }
    if (obj.contains("description"))
        if (!obj["description"].isNull()) {
            result._description = obj.value("description").toString();
        }
    if (obj.contains("minimum"))
        if (!obj["minimum"].isNull()) {
            result._minimum = obj.value("minimum").toDouble();
        }
    if (obj.contains("maximum"))
        if (!obj["maximum"].isNull()) {
            result._maximum = obj.value("maximum").toDouble();
        }
    if (obj.contains("default"))
        if (!obj["default"].isNull()) {
            result._default_ = obj.value("default").toDouble();
        }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const NumberPropertySchema &data)
{
    QJsonObject obj;
    if (data._title.has_value())
        obj.insert("title", *data._title);
    if (data._description.has_value())
        obj.insert("description", *data._description);
    if (data._minimum.has_value())
        obj.insert("minimum", *data._minimum);
    if (data._maximum.has_value())
        obj.insert("maximum", *data._maximum);
    if (data._default_.has_value())
        obj.insert("default", *data._default_);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

QString toString(StringFormat v)
{
    switch(v) {
        case StringFormat::email: return "email";
        case StringFormat::uri: return "uri";
        case StringFormat::date: return "date";
        case StringFormat::dateminustime: return "date-time";
    }
    return {};
}

template<>
Utils::Result<StringFormat> fromJson<StringFormat>(const QJsonValue &val)
{
    const QString str = val.toString();
    if (str == "email") return StringFormat::email;
    if (str == "uri") return StringFormat::uri;
    if (str == "date") return StringFormat::date;
    if (str == "date-time") return StringFormat::dateminustime;
    return Utils::ResultError("Invalid StringFormat value: " + str);
}

QJsonValue toJsonValue(const StringFormat &v)
{
    return toString(v);
}

template<>
Utils::Result<StringPropertySchema> fromJson<StringPropertySchema>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for StringPropertySchema");
    const QJsonObject obj = val.toObject();
    StringPropertySchema result;
    if (obj.contains("title"))
        if (!obj["title"].isNull()) {
            result._title = obj.value("title").toString();
        }
    if (obj.contains("description"))
        if (!obj["description"].isNull()) {
            result._description = obj.value("description").toString();
        }
    if (obj.contains("minLength"))
        if (!obj["minLength"].isNull()) {
            result._minLength = obj.value("minLength").toInt();
        }
    if (obj.contains("maxLength"))
        if (!obj["maxLength"].isNull()) {
            result._maxLength = obj.value("maxLength").toInt();
        }
    if (obj.contains("pattern"))
        if (!obj["pattern"].isNull()) {
            result._pattern = obj.value("pattern").toString();
        }
    if (obj.contains("format") && !obj["format"].isNull()) {
        const auto res0 = fromJson<StringFormat>(obj["format"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._format = *res0;
    }
    if (obj.contains("default"))
        if (!obj["default"].isNull()) {
            result._default_ = obj.value("default").toString();
        }
    if (obj.contains("enum"))
        if (!obj["enum"].isNull()) {
            result._enum_ = obj.value("enum").toArray();
        }
    if (obj.contains("oneOf"))
        if (!obj["oneOf"].isNull()) {
            result._oneOf = obj.value("oneOf").toArray();
        }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const StringPropertySchema &data)
{
    QJsonObject obj;
    if (data._title.has_value())
        obj.insert("title", *data._title);
    if (data._description.has_value())
        obj.insert("description", *data._description);
    if (data._minLength.has_value())
        obj.insert("minLength", *data._minLength);
    if (data._maxLength.has_value())
        obj.insert("maxLength", *data._maxLength);
    if (data._pattern.has_value())
        obj.insert("pattern", *data._pattern);
    if (data._format.has_value())
        obj.insert("format", toJsonValue(*data._format));
    if (data._default_.has_value())
        obj.insert("default", *data._default_);
    if (data._enum_.has_value())
        obj.insert("enum", *data._enum_);
    if (data._oneOf.has_value())
        obj.insert("oneOf", *data._oneOf);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<ElicitationPropertySchema> fromJson<ElicitationPropertySchema>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid ElicitationPropertySchema: expected object");
    const QString dispatchValue = val.toObject().value("type").toString();
    if (dispatchValue == "string") {
        const auto res0 = fromJson<StringPropertySchema>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return ElicitationPropertySchema(*res0);
    }
    else if (dispatchValue == "number") {
        const auto res1 = fromJson<NumberPropertySchema>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        return ElicitationPropertySchema(*res1);
    }
    else if (dispatchValue == "integer") {
        const auto res2 = fromJson<IntegerPropertySchema>(val);
        if (!res2)
            return Utils::ResultError(res2.error());
        return ElicitationPropertySchema(*res2);
    }
    else if (dispatchValue == "boolean") {
        const auto res3 = fromJson<BooleanPropertySchema>(val);
        if (!res3)
            return Utils::ResultError(res3.error());
        return ElicitationPropertySchema(*res3);
    }
    else if (dispatchValue == "array") {
        const auto res4 = fromJson<MultiSelectPropertySchema>(val);
        if (!res4)
            return Utils::ResultError(res4.error());
        return ElicitationPropertySchema(*res4);
    }
    if (dispatchValue.isEmpty())
        return Utils::ResultError("Invalid ElicitationPropertySchema: missing type");
    return ElicitationPropertySchema(val.toObject());  // open union: preserve unknown variants raw
}

QString dispatchValue(const ElicitationPropertySchema &val)
{
    return std::visit([](const auto &v) -> QString {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, StringPropertySchema>) return "string";
        else if constexpr (std::is_same_v<T, NumberPropertySchema>) return "number";
        else if constexpr (std::is_same_v<T, IntegerPropertySchema>) return "integer";
        else if constexpr (std::is_same_v<T, BooleanPropertySchema>) return "boolean";
        else if constexpr (std::is_same_v<T, MultiSelectPropertySchema>) return "array";
        else if constexpr (std::is_same_v<T, QJsonObject>) return v.value("type").toString();
        return {};
    }, val);
}

QJsonObject toJson(const ElicitationPropertySchema &val)
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

QJsonValue toJsonValue(const ElicitationPropertySchema &val)
{
    return toJson(val);
}

QString toString(ElicitationSchemaType v)
{
    switch(v) {
        case ElicitationSchemaType::object: return "object";
    }
    return {};
}

template<>
Utils::Result<ElicitationSchemaType> fromJson<ElicitationSchemaType>(const QJsonValue &val)
{
    const QString str = val.toString();
    if (str == "object") return ElicitationSchemaType::object;
    return Utils::ResultError("Invalid ElicitationSchemaType value: " + str);
}

QJsonValue toJsonValue(const ElicitationSchemaType &v)
{
    return toString(v);
}

template<>
Utils::Result<ElicitationSchema> fromJson<ElicitationSchema>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ElicitationSchema");
    const QJsonObject obj = val.toObject();
    ElicitationSchema result;
    if (obj.contains("type") && obj["type"].isString()) {
        const auto res0 = fromJson<ElicitationSchemaType>(obj["type"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._type = *res0;
    }
    if (obj.contains("title"))
        if (!obj["title"].isNull()) {
            result._title = obj.value("title").toString();
        }
    if (obj.contains("properties") && obj["properties"].isObject()) {
        const QJsonObject mapObj_properties = obj["properties"].toObject();
        QMap<QString, ElicitationPropertySchema> map_properties;
        for (auto it = mapObj_properties.constBegin(); it != mapObj_properties.constEnd(); ++it) {
            const auto res1 = fromJson<ElicitationPropertySchema>(it.value());
            if (!res1)
                return Utils::ResultError(res1.error());
            map_properties.insert(it.key(), *res1);
        }
        result._properties = map_properties;
    }
    if (obj.contains("required"))
        if (!obj["required"].isNull()) {
            result._required = obj.value("required").toArray();
        }
    if (obj.contains("description"))
        if (!obj["description"].isNull()) {
            result._description = obj.value("description").toString();
        }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const ElicitationSchema &data)
{
    QJsonObject obj;
    if (data._type.has_value())
        obj.insert("type", toJsonValue(*data._type));
    if (data._title.has_value())
        obj.insert("title", *data._title);
    if (data._properties.has_value()) {
        QJsonObject map_properties;
        for (auto it = data._properties->constBegin(); it != data._properties->constEnd(); ++it)
            map_properties.insert(it.key(), toJsonValue(it.value()));
        obj.insert("properties", map_properties);
    }
    if (data._required.has_value())
        obj.insert("required", *data._required);
    if (data._description.has_value())
        obj.insert("description", *data._description);
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
Utils::Result<ElicitationSessionScope> fromJson<ElicitationSessionScope>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ElicitationSessionScope");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    ElicitationSessionScope result;
    if (obj.contains("sessionId") && obj["sessionId"].isString()) {
        const auto res0 = fromJson<SessionId>(obj["sessionId"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._sessionId = *res0;
    }
    if (obj.contains("toolCallId") && !obj["toolCallId"].isNull()) {
        const auto res1 = fromJson<ToolCallId>(obj["toolCallId"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._toolCallId = *res1;
    }
    return result;
}

QJsonObject toJson(const ElicitationSessionScope &data)
{
    QJsonObject obj{{"sessionId", data._sessionId}};
    if (data._toolCallId.has_value())
        obj.insert("toolCallId", *data._toolCallId);
    return obj;
}

template<>
Utils::Result<ElicitationFormMode> fromJson<ElicitationFormMode>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ElicitationFormMode");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("requestedSchema"))
        return Utils::ResultError("Missing required field: requestedSchema");
    ElicitationFormMode result;
    if (obj.contains("requestedSchema") && obj["requestedSchema"].isObject()) {
        const auto res0 = fromJson<ElicitationSchema>(obj["requestedSchema"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._requestedSchema = *res0;
    }
    {
        const QSet<QString> knownKeys{"requestedSchema"};
        for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
            if (!knownKeys.contains(it.key()))
                result._additionalProperties.insert(it.key(), it.value());
        }
    }
    return result;
}

QJsonObject toJson(const ElicitationFormMode &data)
{
    QJsonObject obj{{"requestedSchema", toJson(data._requestedSchema)}};
    for (auto it = data._additionalProperties.constBegin(); it != data._additionalProperties.constEnd(); ++it)
        obj.insert(it.key(), it.value());
    return obj;
}

template<>
Utils::Result<ElicitationUrlMode> fromJson<ElicitationUrlMode>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ElicitationUrlMode");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("elicitationId"))
        return Utils::ResultError("Missing required field: elicitationId");
    if (!obj.contains("url"))
        return Utils::ResultError("Missing required field: url");
    ElicitationUrlMode result;
    if (obj.contains("elicitationId") && obj["elicitationId"].isString()) {
        const auto res0 = fromJson<ElicitationId>(obj["elicitationId"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._elicitationId = *res0;
    }
    result._url = obj.value("url").toString();
    {
        const QSet<QString> knownKeys{"elicitationId", "url"};
        for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
            if (!knownKeys.contains(it.key()))
                result._additionalProperties.insert(it.key(), it.value());
        }
    }
    return result;
}

QJsonObject toJson(const ElicitationUrlMode &data)
{
    QJsonObject obj{
        {"elicitationId", data._elicitationId},
        {"url", data._url}
    };
    for (auto it = data._additionalProperties.constBegin(); it != data._additionalProperties.constEnd(); ++it)
        obj.insert(it.key(), it.value());
    return obj;
}

template<>
Utils::Result<CreateElicitationRequest> fromJson<CreateElicitationRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CreateElicitationRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("message"))
        return Utils::ResultError("Missing required field: message");
    CreateElicitationRequest result;
    result._message = obj.value("message").toString();
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    {
        const QSet<QString> knownKeys{"message", "_meta"};
        for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
            if (!knownKeys.contains(it.key()))
                result._additionalProperties.insert(it.key(), it.value());
        }
    }
    return result;
}

QJsonObject toJson(const CreateElicitationRequest &data)
{
    QJsonObject obj{{"message", data._message}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    for (auto it = data._additionalProperties.constBegin(); it != data._additionalProperties.constEnd(); ++it)
        obj.insert(it.key(), it.value());
    return obj;
}

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

template<>
Utils::Result<CreateTerminalRequest> fromJson<CreateTerminalRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CreateTerminalRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    if (!obj.contains("command"))
        return Utils::ResultError("Missing required field: command");
    CreateTerminalRequest result;
    if (obj.contains("sessionId") && obj["sessionId"].isString()) {
        const auto res0 = fromJson<SessionId>(obj["sessionId"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._sessionId = *res0;
    }
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
            const auto res1 = fromJson<EnvVariable>(v);
            if (!res1)
                return Utils::ResultError(res1.error());
            list_env.append(*res1);
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
    return result;
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
        return Utils::ResultError("Expected JSON object for KillTerminalRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    if (!obj.contains("terminalId"))
        return Utils::ResultError("Missing required field: terminalId");
    KillTerminalRequest result;
    if (obj.contains("sessionId") && obj["sessionId"].isString()) {
        const auto res0 = fromJson<SessionId>(obj["sessionId"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._sessionId = *res0;
    }
    if (obj.contains("terminalId") && obj["terminalId"].isString()) {
        const auto res1 = fromJson<TerminalId>(obj["terminalId"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._terminalId = *res1;
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for ReadTextFileRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    if (!obj.contains("path"))
        return Utils::ResultError("Missing required field: path");
    ReadTextFileRequest result;
    if (obj.contains("sessionId") && obj["sessionId"].isString()) {
        const auto res0 = fromJson<SessionId>(obj["sessionId"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._sessionId = *res0;
    }
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
    return result;
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
        return Utils::ResultError("Expected JSON object for ReleaseTerminalRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    if (!obj.contains("terminalId"))
        return Utils::ResultError("Missing required field: terminalId");
    ReleaseTerminalRequest result;
    if (obj.contains("sessionId") && obj["sessionId"].isString()) {
        const auto res0 = fromJson<SessionId>(obj["sessionId"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._sessionId = *res0;
    }
    if (obj.contains("terminalId") && obj["terminalId"].isString()) {
        const auto res1 = fromJson<TerminalId>(obj["terminalId"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._terminalId = *res1;
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for PermissionOption");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("optionId"))
        return Utils::ResultError("Missing required field: optionId");
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    if (!obj.contains("kind"))
        return Utils::ResultError("Missing required field: kind");
    PermissionOption result;
    if (obj.contains("optionId") && obj["optionId"].isString()) {
        const auto res0 = fromJson<PermissionOptionId>(obj["optionId"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._optionId = *res0;
    }
    result._name = obj.value("name").toString();
    if (obj.contains("kind") && obj["kind"].isString()) {
        const auto res1 = fromJson<PermissionOptionKind>(obj["kind"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._kind = *res1;
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for AudioContent");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("data"))
        return Utils::ResultError("Missing required field: data");
    if (!obj.contains("mimeType"))
        return Utils::ResultError("Missing required field: mimeType");
    AudioContent result;
    if (obj.contains("annotations") && !obj["annotations"].isNull()) {
        const auto res0 = fromJson<Annotations>(obj["annotations"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._annotations = *res0;
    }
    result._data = obj.value("data").toString();
    result._mimeType = obj.value("mimeType").toString();
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Invalid EmbeddedResourceResource: expected object");
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
        return Utils::ResultError("Expected JSON object for EmbeddedResource");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("resource"))
        return Utils::ResultError("Missing required field: resource");
    EmbeddedResource result;
    if (obj.contains("annotations") && !obj["annotations"].isNull()) {
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
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for ImageContent");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("data"))
        return Utils::ResultError("Missing required field: data");
    if (!obj.contains("mimeType"))
        return Utils::ResultError("Missing required field: mimeType");
    ImageContent result;
    if (obj.contains("annotations") && !obj["annotations"].isNull()) {
        const auto res0 = fromJson<Annotations>(obj["annotations"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._annotations = *res0;
    }
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
    return result;
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
        return Utils::ResultError("Expected JSON object for ResourceLink");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    ResourceLink result;
    if (obj.contains("annotations") && !obj["annotations"].isNull()) {
        const auto res0 = fromJson<Annotations>(obj["annotations"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._annotations = *res0;
    }
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
    return result;
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
        return Utils::ResultError("Expected JSON object for TextContent");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("text"))
        return Utils::ResultError("Missing required field: text");
    TextContent result;
    if (obj.contains("annotations") && !obj["annotations"].isNull()) {
        const auto res0 = fromJson<Annotations>(obj["annotations"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._annotations = *res0;
    }
    result._text = obj.value("text").toString();
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for Content");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("content"))
        return Utils::ResultError("Missing required field: content");
    Content result;
    if (obj.contains("content")) {
        const auto res0 = fromJson<ContentBlock>(obj["content"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._content = *res0;
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for Terminal");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("terminalId"))
        return Utils::ResultError("Missing required field: terminalId");
    Terminal result;
    if (obj.contains("terminalId") && obj["terminalId"].isString()) {
        const auto res0 = fromJson<TerminalId>(obj["terminalId"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._terminalId = *res0;
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Invalid ToolCallContent: expected object");
    const QString dispatchValue = val.toObject().value("type").toString();
    if (dispatchValue == "content") {
        const auto res0 = fromJson<Content>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return ToolCallContent(*res0);
    }
    else if (dispatchValue == "diff") {
        const auto res1 = fromJson<Diff>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        return ToolCallContent(*res1);
    }
    else if (dispatchValue == "terminal") {
        const auto res2 = fromJson<Terminal>(val);
        if (!res2)
            return Utils::ResultError(res2.error());
        return ToolCallContent(*res2);
    }
    return Utils::ResultError("Invalid ToolCallContent: unknown type \"" + dispatchValue + "\"");
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
        return Utils::ResultError("Expected JSON object for ToolCallUpdate");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("toolCallId"))
        return Utils::ResultError("Missing required field: toolCallId");
    ToolCallUpdate result;
    if (obj.contains("toolCallId") && obj["toolCallId"].isString()) {
        const auto res0 = fromJson<ToolCallId>(obj["toolCallId"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._toolCallId = *res0;
    }
    if (obj.contains("kind") && !obj["kind"].isNull()) {
        const auto res1 = fromJson<ToolKind>(obj["kind"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._kind = *res1;
    }
    if (obj.contains("status") && !obj["status"].isNull()) {
        const auto res2 = fromJson<ToolCallStatus>(obj["status"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._status = *res2;
    }
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
    return result;
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
        return Utils::ResultError("Expected JSON object for RequestPermissionRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    if (!obj.contains("toolCall"))
        return Utils::ResultError("Missing required field: toolCall");
    if (!obj.contains("options"))
        return Utils::ResultError("Missing required field: options");
    RequestPermissionRequest result;
    if (obj.contains("sessionId") && obj["sessionId"].isString()) {
        const auto res0 = fromJson<SessionId>(obj["sessionId"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._sessionId = *res0;
    }
    if (obj.contains("toolCall") && obj["toolCall"].isObject()) {
        const auto res1 = fromJson<ToolCallUpdate>(obj["toolCall"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._toolCall = *res1;
    }
    if (obj.contains("options") && obj["options"].isArray()) {
        const QJsonArray arr = obj["options"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res2 = fromJson<PermissionOption>(v);
            if (!res2)
                return Utils::ResultError(res2.error());
            result._options.append(*res2);
        }
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for TerminalOutputRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    if (!obj.contains("terminalId"))
        return Utils::ResultError("Missing required field: terminalId");
    TerminalOutputRequest result;
    if (obj.contains("sessionId") && obj["sessionId"].isString()) {
        const auto res0 = fromJson<SessionId>(obj["sessionId"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._sessionId = *res0;
    }
    if (obj.contains("terminalId") && obj["terminalId"].isString()) {
        const auto res1 = fromJson<TerminalId>(obj["terminalId"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._terminalId = *res1;
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for WaitForTerminalExitRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    if (!obj.contains("terminalId"))
        return Utils::ResultError("Missing required field: terminalId");
    WaitForTerminalExitRequest result;
    if (obj.contains("sessionId") && obj["sessionId"].isString()) {
        const auto res0 = fromJson<SessionId>(obj["sessionId"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._sessionId = *res0;
    }
    if (obj.contains("terminalId") && obj["terminalId"].isString()) {
        const auto res1 = fromJson<TerminalId>(obj["terminalId"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._terminalId = *res1;
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for WriteTextFileRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    if (!obj.contains("path"))
        return Utils::ResultError("Missing required field: path");
    if (!obj.contains("content"))
        return Utils::ResultError("Missing required field: content");
    WriteTextFileRequest result;
    if (obj.contains("sessionId") && obj["sessionId"].isString()) {
        const auto res0 = fromJson<SessionId>(obj["sessionId"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._sessionId = *res0;
    }
    result._path = obj.value("path").toString();
    result._content = obj.value("content").toString();
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for AgentRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    AgentRequest result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>(obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    result._method = obj.value("method").toString();
    if (obj.contains("params"))
        result._params = obj.value("params").toString();
    return result;
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
        return Utils::ResultError("Expected JSON object for AgentAuthCapabilities");
    const QJsonObject obj = val.toObject();
    AgentAuthCapabilities result;
    if (obj.contains("logout") && !obj["logout"].isNull()) {
        const auto res0 = fromJson<LogoutCapabilities>(obj["logout"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._logout = *res0;
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for SessionCapabilities");
    const QJsonObject obj = val.toObject();
    SessionCapabilities result;
    if (obj.contains("list") && !obj["list"].isNull()) {
        const auto res0 = fromJson<SessionListCapabilities>(obj["list"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._list = *res0;
    }
    if (obj.contains("delete") && !obj["delete"].isNull()) {
        const auto res1 = fromJson<SessionDeleteCapabilities>(obj["delete"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._delete_ = *res1;
    }
    if (obj.contains("additionalDirectories") && !obj["additionalDirectories"].isNull()) {
        const auto res2 = fromJson<SessionAdditionalDirectoriesCapabilities>(obj["additionalDirectories"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._additionalDirectories = *res2;
    }
    if (obj.contains("resume") && !obj["resume"].isNull()) {
        const auto res3 = fromJson<SessionResumeCapabilities>(obj["resume"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._resume = *res3;
    }
    if (obj.contains("close") && !obj["close"].isNull()) {
        const auto res4 = fromJson<SessionCloseCapabilities>(obj["close"]);
        if (!res4)
            return Utils::ResultError(res4.error());
        result._close = *res4;
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for AgentCapabilities");
    const QJsonObject obj = val.toObject();
    AgentCapabilities result;
    if (obj.contains("loadSession"))
        result._loadSession = obj.value("loadSession").toBool();
    if (obj.contains("promptCapabilities") && obj["promptCapabilities"].isObject()) {
        const auto res0 = fromJson<PromptCapabilities>(obj["promptCapabilities"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._promptCapabilities = *res0;
    }
    if (obj.contains("mcpCapabilities") && obj["mcpCapabilities"].isObject()) {
        const auto res1 = fromJson<McpCapabilities>(obj["mcpCapabilities"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._mcpCapabilities = *res1;
    }
    if (obj.contains("sessionCapabilities") && obj["sessionCapabilities"].isObject()) {
        const auto res2 = fromJson<SessionCapabilities>(obj["sessionCapabilities"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._sessionCapabilities = *res2;
    }
    if (obj.contains("auth") && obj["auth"].isObject()) {
        const auto res3 = fromJson<AgentAuthCapabilities>(obj["auth"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._auth = *res3;
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for AuthMethodAgent");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    AuthMethodAgent result;
    if (obj.contains("id") && obj["id"].isString()) {
        const auto res0 = fromJson<AuthMethodId>(obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    result._name = obj.value("name").toString();
    if (obj.contains("description"))
        if (!obj["description"].isNull()) {
            result._description = obj.value("description").toString();
        }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
Utils::Result<AuthMethodTerminal> fromJson<AuthMethodTerminal>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for AuthMethodTerminal");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    AuthMethodTerminal result;
    if (obj.contains("id") && obj["id"].isString()) {
        const auto res0 = fromJson<AuthMethodId>(obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    result._name = obj.value("name").toString();
    if (obj.contains("description"))
        if (!obj["description"].isNull()) {
            result._description = obj.value("description").toString();
        }
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
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const AuthMethodTerminal &data)
{
    QJsonObject obj{
        {"id", data._id},
        {"name", data._name}
    };
    if (data._description.has_value())
        obj.insert("description", *data._description);
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
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<AuthMethod> fromJson<AuthMethod>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid AuthMethod: expected object");
    const QString dispatchValue = val.toObject().value("type").toString();
    if (dispatchValue == "terminal") {
        const auto res0 = fromJson<AuthMethodTerminal>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return AuthMethod(*res0);
    }
    else if (dispatchValue == "agent") {
        const auto res1 = fromJson<AuthMethodAgent>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        return AuthMethod(*res1);
    }
    if (dispatchValue.isEmpty()) {
        const auto res2 = fromJson<AuthMethodAgent>(val);
        if (!res2)
            return Utils::ResultError(res2.error());
        return AuthMethod(*res2);
    }
    return Utils::ResultError("Invalid AuthMethod: unknown type \"" + dispatchValue + "\"");
}

QString dispatchValue(const AuthMethod &val)
{
    return std::visit([](const auto &v) -> QString {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, AuthMethodTerminal>) return "terminal";
        else if constexpr (std::is_same_v<T, AuthMethodAgent>) return "agent";
        return {};
    }, val);
}

QJsonObject toJson(const AuthMethod &val)
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

QJsonValue toJsonValue(const AuthMethod &val)
{
    return toJson(val);
}

QString name(const AuthMethod &val)
{
    return std::visit([](const auto &v) -> QString { return v._name; }, val);
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
        return Utils::ResultError("Expected JSON object for InitializeResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("protocolVersion"))
        return Utils::ResultError("Missing required field: protocolVersion");
    InitializeResponse result;
    if (obj.contains("protocolVersion") && obj["protocolVersion"].isDouble()) {
        const auto res0 = fromJson<ProtocolVersion>(obj["protocolVersion"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._protocolVersion = *res0;
    }
    if (obj.contains("agentCapabilities") && obj["agentCapabilities"].isObject()) {
        const auto res1 = fromJson<AgentCapabilities>(obj["agentCapabilities"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._agentCapabilities = *res1;
    }
    if (obj.contains("authMethods") && obj["authMethods"].isArray()) {
        const QJsonArray arr = obj["authMethods"].toArray();
        QList<AuthMethod> list_authMethods;
        for (const QJsonValue &v : arr) {
            const auto res2 = fromJson<AuthMethod>(v);
            if (!res2)
                return Utils::ResultError(res2.error());
            list_authMethods.append(*res2);
        }
        result._authMethods = list_authMethods;
    }
    if (obj.contains("agentInfo") && !obj["agentInfo"].isNull()) {
        const auto res3 = fromJson<Implementation>(obj["agentInfo"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._agentInfo = *res3;
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for SessionInfo");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    if (!obj.contains("cwd"))
        return Utils::ResultError("Missing required field: cwd");
    SessionInfo result;
    if (obj.contains("sessionId") && obj["sessionId"].isString()) {
        const auto res0 = fromJson<SessionId>(obj["sessionId"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._sessionId = *res0;
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
        return Utils::ResultError("Expected JSON object for ListSessionsResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessions"))
        return Utils::ResultError("Missing required field: sessions");
    ListSessionsResponse result;
    if (obj.contains("sessions") && obj["sessions"].isArray()) {
        const QJsonArray arr = obj["sessions"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<SessionInfo>(v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._sessions.append(*res0);
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
    return result;
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

template<>
Utils::Result<SessionConfigBoolean> fromJson<SessionConfigBoolean>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SessionConfigBoolean");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("currentValue"))
        return Utils::ResultError("Missing required field: currentValue");
    SessionConfigBoolean result;
    result._currentValue = obj.value("currentValue").toBool();
    return result;
}

QJsonObject toJson(const SessionConfigBoolean &data)
{
    QJsonObject obj{{"currentValue", data._currentValue}};
    return obj;
}

QString toString(SessionConfigOptionCategory v)
{
    switch(v) {
        case SessionConfigOptionCategory::mode: return "mode";
        case SessionConfigOptionCategory::model: return "model";
        case SessionConfigOptionCategory::model_config: return "model_config";
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
    if (str == "model_config") return SessionConfigOptionCategory::model_config;
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
        return Utils::ResultError("Expected JSON object for SessionConfigSelectOption");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("value"))
        return Utils::ResultError("Missing required field: value");
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    SessionConfigSelectOption result;
    if (obj.contains("value") && obj["value"].isString()) {
        const auto res0 = fromJson<SessionConfigValueId>(obj["value"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._value = *res0;
    }
    result._name = obj.value("name").toString();
    if (obj.contains("description"))
        if (!obj["description"].isNull()) {
            result._description = obj.value("description").toString();
        }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for SessionConfigSelectGroup");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("group"))
        return Utils::ResultError("Missing required field: group");
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    if (!obj.contains("options"))
        return Utils::ResultError("Missing required field: options");
    SessionConfigSelectGroup result;
    if (obj.contains("group") && obj["group"].isString()) {
        const auto res0 = fromJson<SessionConfigGroupId>(obj["group"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._group = *res0;
    }
    result._name = obj.value("name").toString();
    if (obj.contains("options") && obj["options"].isArray()) {
        const QJsonArray arr = obj["options"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res1 = fromJson<SessionConfigSelectOption>(v);
            if (!res1)
                return Utils::ResultError(res1.error());
            result._options.append(*res1);
        }
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for SessionConfigSelect");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("currentValue"))
        return Utils::ResultError("Missing required field: currentValue");
    if (!obj.contains("options"))
        return Utils::ResultError("Missing required field: options");
    SessionConfigSelect result;
    if (obj.contains("currentValue") && obj["currentValue"].isString()) {
        const auto res0 = fromJson<SessionConfigValueId>(obj["currentValue"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._currentValue = *res0;
    }
    if (obj.contains("options")) {
        const auto res1 = fromJson<SessionConfigSelectOptions>(obj["options"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._options = *res1;
    }
    return result;
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
        return Utils::ResultError("Expected JSON object for SessionConfigOption");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    SessionConfigOption result;
    if (obj.contains("id") && obj["id"].isString()) {
        const auto res0 = fromJson<SessionConfigId>(obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    result._name = obj.value("name").toString();
    if (obj.contains("description"))
        if (!obj["description"].isNull()) {
            result._description = obj.value("description").toString();
        }
    if (obj.contains("category") && !obj["category"].isNull()) {
        const auto res1 = fromJson<SessionConfigOptionCategory>(obj["category"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._category = *res1;
    }
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
    return result;
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
        return Utils::ResultError("Expected JSON object for SessionMode");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    SessionMode result;
    if (obj.contains("id") && obj["id"].isString()) {
        const auto res0 = fromJson<SessionModeId>(obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    result._name = obj.value("name").toString();
    if (obj.contains("description"))
        if (!obj["description"].isNull()) {
            result._description = obj.value("description").toString();
        }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for SessionModeState");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("currentModeId"))
        return Utils::ResultError("Missing required field: currentModeId");
    if (!obj.contains("availableModes"))
        return Utils::ResultError("Missing required field: availableModes");
    SessionModeState result;
    if (obj.contains("currentModeId") && obj["currentModeId"].isString()) {
        const auto res0 = fromJson<SessionModeId>(obj["currentModeId"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._currentModeId = *res0;
    }
    if (obj.contains("availableModes") && obj["availableModes"].isArray()) {
        const QJsonArray arr = obj["availableModes"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res1 = fromJson<SessionMode>(v);
            if (!res1)
                return Utils::ResultError(res1.error());
            result._availableModes.append(*res1);
        }
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for LoadSessionResponse");
    const QJsonObject obj = val.toObject();
    LoadSessionResponse result;
    if (obj.contains("modes") && !obj["modes"].isNull()) {
        const auto res0 = fromJson<SessionModeState>(obj["modes"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._modes = *res0;
    }
    if (obj.contains("configOptions"))
        if (!obj["configOptions"].isNull()) {
            result._configOptions = obj.value("configOptions").toArray();
        }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for NewSessionResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    NewSessionResponse result;
    if (obj.contains("sessionId") && obj["sessionId"].isString()) {
        const auto res0 = fromJson<SessionId>(obj["sessionId"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._sessionId = *res0;
    }
    if (obj.contains("modes") && !obj["modes"].isNull()) {
        const auto res1 = fromJson<SessionModeState>(obj["modes"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._modes = *res1;
    }
    if (obj.contains("configOptions"))
        if (!obj["configOptions"].isNull()) {
            result._configOptions = obj.value("configOptions").toArray();
        }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for PromptResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("stopReason"))
        return Utils::ResultError("Missing required field: stopReason");
    PromptResponse result;
    if (obj.contains("stopReason") && obj["stopReason"].isString()) {
        const auto res0 = fromJson<StopReason>(obj["stopReason"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._stopReason = *res0;
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for ResumeSessionResponse");
    const QJsonObject obj = val.toObject();
    ResumeSessionResponse result;
    if (obj.contains("modes") && !obj["modes"].isNull()) {
        const auto res0 = fromJson<SessionModeState>(obj["modes"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._modes = *res0;
    }
    if (obj.contains("configOptions"))
        if (!obj["configOptions"].isNull()) {
            result._configOptions = obj.value("configOptions").toArray();
        }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for SetSessionConfigOptionResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("configOptions"))
        return Utils::ResultError("Missing required field: configOptions");
    SetSessionConfigOptionResponse result;
    if (obj.contains("configOptions") && obj["configOptions"].isArray()) {
        const QJsonArray arr = obj["configOptions"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<SessionConfigOption>(v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._configOptions.append(*res0);
        }
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
Utils::Result<AgentResponse> fromJson<AgentResponse>(const QJsonValue &val)
{
    if (val.isObject())
        return AgentResponse(val.toObject());  // open union: preserve unknown variants raw
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
Utils::Result<CompleteElicitationNotification> fromJson<CompleteElicitationNotification>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CompleteElicitationNotification");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("elicitationId"))
        return Utils::ResultError("Missing required field: elicitationId");
    CompleteElicitationNotification result;
    if (obj.contains("elicitationId") && obj["elicitationId"].isString()) {
        const auto res0 = fromJson<ElicitationId>(obj["elicitationId"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._elicitationId = *res0;
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const CompleteElicitationNotification &data)
{
    QJsonObject obj{{"elicitationId", data._elicitationId}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
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
        return Utils::ResultError("Invalid AvailableCommandInput: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("hint")) {
        const auto res0 = fromJson<UnstructuredCommandInput>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return AvailableCommandInput(*res0);
    }
    return Utils::ResultError("Invalid AvailableCommandInput");
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
        return Utils::ResultError("Expected JSON object for AvailableCommand");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    if (!obj.contains("description"))
        return Utils::ResultError("Missing required field: description");
    AvailableCommand result;
    result._name = obj.value("name").toString();
    result._description = obj.value("description").toString();
    if (obj.contains("input") && !obj["input"].isNull()) {
        const auto res0 = fromJson<AvailableCommandInput>(obj["input"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._input = *res0;
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for AvailableCommandsUpdate");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("availableCommands"))
        return Utils::ResultError("Missing required field: availableCommands");
    AvailableCommandsUpdate result;
    if (obj.contains("availableCommands") && obj["availableCommands"].isArray()) {
        const QJsonArray arr = obj["availableCommands"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<AvailableCommand>(v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._availableCommands.append(*res0);
        }
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for ConfigOptionUpdate");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("configOptions"))
        return Utils::ResultError("Missing required field: configOptions");
    ConfigOptionUpdate result;
    if (obj.contains("configOptions") && obj["configOptions"].isArray()) {
        const QJsonArray arr = obj["configOptions"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<SessionConfigOption>(v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._configOptions.append(*res0);
        }
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for ContentChunk");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("content"))
        return Utils::ResultError("Missing required field: content");
    ContentChunk result;
    if (obj.contains("content")) {
        const auto res0 = fromJson<ContentBlock>(obj["content"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._content = *res0;
    }
    if (obj.contains("messageId") && !obj["messageId"].isNull()) {
        const auto res1 = fromJson<MessageId>(obj["messageId"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._messageId = *res1;
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for CurrentModeUpdate");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("currentModeId"))
        return Utils::ResultError("Missing required field: currentModeId");
    CurrentModeUpdate result;
    if (obj.contains("currentModeId") && obj["currentModeId"].isString()) {
        const auto res0 = fromJson<SessionModeId>(obj["currentModeId"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._currentModeId = *res0;
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for PlanEntry");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("content"))
        return Utils::ResultError("Missing required field: content");
    if (!obj.contains("priority"))
        return Utils::ResultError("Missing required field: priority");
    if (!obj.contains("status"))
        return Utils::ResultError("Missing required field: status");
    PlanEntry result;
    result._content = obj.value("content").toString();
    if (obj.contains("priority") && obj["priority"].isString()) {
        const auto res0 = fromJson<PlanEntryPriority>(obj["priority"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._priority = *res0;
    }
    if (obj.contains("status") && obj["status"].isString()) {
        const auto res1 = fromJson<PlanEntryStatus>(obj["status"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._status = *res1;
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for Plan");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("entries"))
        return Utils::ResultError("Missing required field: entries");
    Plan result;
    if (obj.contains("entries") && obj["entries"].isArray()) {
        const QJsonArray arr = obj["entries"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<PlanEntry>(v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._entries.append(*res0);
        }
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for ToolCall");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("toolCallId"))
        return Utils::ResultError("Missing required field: toolCallId");
    if (!obj.contains("title"))
        return Utils::ResultError("Missing required field: title");
    ToolCall result;
    if (obj.contains("toolCallId") && obj["toolCallId"].isString()) {
        const auto res0 = fromJson<ToolCallId>(obj["toolCallId"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._toolCallId = *res0;
    }
    result._title = obj.value("title").toString();
    if (obj.contains("kind") && obj["kind"].isString()) {
        const auto res1 = fromJson<ToolKind>(obj["kind"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._kind = *res1;
    }
    if (obj.contains("status") && obj["status"].isString()) {
        const auto res2 = fromJson<ToolCallStatus>(obj["status"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._status = *res2;
    }
    if (obj.contains("content") && obj["content"].isArray()) {
        const QJsonArray arr = obj["content"].toArray();
        QList<ToolCallContent> list_content;
        for (const QJsonValue &v : arr) {
            const auto res3 = fromJson<ToolCallContent>(v);
            if (!res3)
                return Utils::ResultError(res3.error());
            list_content.append(*res3);
        }
        result._content = list_content;
    }
    if (obj.contains("locations") && obj["locations"].isArray()) {
        const QJsonArray arr = obj["locations"].toArray();
        QList<ToolCallLocation> list_locations;
        for (const QJsonValue &v : arr) {
            const auto res4 = fromJson<ToolCallLocation>(v);
            if (!res4)
                return Utils::ResultError(res4.error());
            list_locations.append(*res4);
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
    return result;
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
        return Utils::ResultError("Expected JSON object for UsageUpdate");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("used"))
        return Utils::ResultError("Missing required field: used");
    if (!obj.contains("size"))
        return Utils::ResultError("Missing required field: size");
    UsageUpdate result;
    result._used = obj.value("used").toInt();
    result._size = obj.value("size").toInt();
    if (obj.contains("cost") && !obj["cost"].isNull()) {
        const auto res0 = fromJson<Cost>(obj["cost"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._cost = *res0;
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Invalid SessionUpdate: expected object");
    const QJsonObject obj = val.toObject();
    const QString kind = obj.value("sessionUpdate").toString();
    SessionUpdate result;
    result._kind = kind;
    if (kind == "user_message_chunk") {
        const auto res0 = fromJson<ContentChunk>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._value = *res0;
    }
    else if (kind == "agent_message_chunk") {
        const auto res1 = fromJson<ContentChunk>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._value = *res1;
    }
    else if (kind == "agent_thought_chunk") {
        const auto res2 = fromJson<ContentChunk>(val);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._value = *res2;
    }
    else if (kind == "tool_call") {
        const auto res3 = fromJson<ToolCall>(val);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._value = *res3;
    }
    else if (kind == "tool_call_update") {
        const auto res4 = fromJson<ToolCallUpdate>(val);
        if (!res4)
            return Utils::ResultError(res4.error());
        result._value = *res4;
    }
    else if (kind == "plan") {
        const auto res5 = fromJson<Plan>(val);
        if (!res5)
            return Utils::ResultError(res5.error());
        result._value = *res5;
    }
    else if (kind == "available_commands_update") {
        const auto res6 = fromJson<AvailableCommandsUpdate>(val);
        if (!res6)
            return Utils::ResultError(res6.error());
        result._value = *res6;
    }
    else if (kind == "current_mode_update") {
        const auto res7 = fromJson<CurrentModeUpdate>(val);
        if (!res7)
            return Utils::ResultError(res7.error());
        result._value = *res7;
    }
    else if (kind == "config_option_update") {
        const auto res8 = fromJson<ConfigOptionUpdate>(val);
        if (!res8)
            return Utils::ResultError(res8.error());
        result._value = *res8;
    }
    else if (kind == "session_info_update") {
        const auto res9 = fromJson<SessionInfoUpdate>(val);
        if (!res9)
            return Utils::ResultError(res9.error());
        result._value = *res9;
    }
    else if (kind == "usage_update") {
        const auto res10 = fromJson<UsageUpdate>(val);
        if (!res10)
            return Utils::ResultError(res10.error());
        result._value = *res10;
    }
    else
        return Utils::ResultError("Invalid SessionUpdate: unknown sessionUpdate \"" + kind + "\"");
    return result;
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
        return Utils::ResultError("Expected JSON object for SessionNotification");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    if (!obj.contains("update"))
        return Utils::ResultError("Missing required field: update");
    SessionNotification result;
    if (obj.contains("sessionId") && obj["sessionId"].isString()) {
        const auto res0 = fromJson<SessionId>(obj["sessionId"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._sessionId = *res0;
    }
    if (obj.contains("update")) {
        const auto res1 = fromJson<SessionUpdate>(obj["update"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._update = *res1;
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for AuthenticateRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("methodId"))
        return Utils::ResultError("Missing required field: methodId");
    AuthenticateRequest result;
    if (obj.contains("methodId") && obj["methodId"].isString()) {
        const auto res0 = fromJson<AuthMethodId>(obj["methodId"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._methodId = *res0;
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for CloseSessionRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    CloseSessionRequest result;
    if (obj.contains("sessionId") && obj["sessionId"].isString()) {
        const auto res0 = fromJson<SessionId>(obj["sessionId"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._sessionId = *res0;
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for DeleteSessionRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    DeleteSessionRequest result;
    if (obj.contains("sessionId") && obj["sessionId"].isString()) {
        const auto res0 = fromJson<SessionId>(obj["sessionId"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._sessionId = *res0;
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const DeleteSessionRequest &data)
{
    QJsonObject obj{{"sessionId", data._sessionId}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<AuthCapabilities> fromJson<AuthCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for AuthCapabilities");
    const QJsonObject obj = val.toObject();
    AuthCapabilities result;
    if (obj.contains("terminal"))
        result._terminal = obj.value("terminal").toBool();
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const AuthCapabilities &data)
{
    QJsonObject obj;
    if (data._terminal.has_value())
        obj.insert("terminal", *data._terminal);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<BooleanConfigOptionCapabilities> fromJson<BooleanConfigOptionCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for BooleanConfigOptionCapabilities");
    const QJsonObject obj = val.toObject();
    BooleanConfigOptionCapabilities result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const BooleanConfigOptionCapabilities &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<SessionConfigOptionsCapabilities> fromJson<SessionConfigOptionsCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SessionConfigOptionsCapabilities");
    const QJsonObject obj = val.toObject();
    SessionConfigOptionsCapabilities result;
    if (obj.contains("boolean") && !obj["boolean"].isNull()) {
        const auto res0 = fromJson<BooleanConfigOptionCapabilities>(obj["boolean"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._boolean = *res0;
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const SessionConfigOptionsCapabilities &data)
{
    QJsonObject obj;
    if (data._boolean.has_value())
        obj.insert("boolean", toJson(*data._boolean));
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<ClientSessionCapabilities> fromJson<ClientSessionCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ClientSessionCapabilities");
    const QJsonObject obj = val.toObject();
    ClientSessionCapabilities result;
    if (obj.contains("configOptions") && !obj["configOptions"].isNull()) {
        const auto res0 = fromJson<SessionConfigOptionsCapabilities>(obj["configOptions"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._configOptions = *res0;
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const ClientSessionCapabilities &data)
{
    QJsonObject obj;
    if (data._configOptions.has_value())
        obj.insert("configOptions", toJson(*data._configOptions));
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<ElicitationFormCapabilities> fromJson<ElicitationFormCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ElicitationFormCapabilities");
    const QJsonObject obj = val.toObject();
    ElicitationFormCapabilities result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const ElicitationFormCapabilities &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<ElicitationUrlCapabilities> fromJson<ElicitationUrlCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ElicitationUrlCapabilities");
    const QJsonObject obj = val.toObject();
    ElicitationUrlCapabilities result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const ElicitationUrlCapabilities &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<ElicitationCapabilities> fromJson<ElicitationCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ElicitationCapabilities");
    const QJsonObject obj = val.toObject();
    ElicitationCapabilities result;
    if (obj.contains("form") && !obj["form"].isNull()) {
        const auto res0 = fromJson<ElicitationFormCapabilities>(obj["form"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._form = *res0;
    }
    if (obj.contains("url") && !obj["url"].isNull()) {
        const auto res1 = fromJson<ElicitationUrlCapabilities>(obj["url"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._url = *res1;
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const ElicitationCapabilities &data)
{
    QJsonObject obj;
    if (data._form.has_value())
        obj.insert("form", toJson(*data._form));
    if (data._url.has_value())
        obj.insert("url", toJson(*data._url));
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
        return Utils::ResultError("Expected JSON object for ClientCapabilities");
    const QJsonObject obj = val.toObject();
    ClientCapabilities result;
    if (obj.contains("fs") && obj["fs"].isObject()) {
        const auto res0 = fromJson<FileSystemCapabilities>(obj["fs"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._fs = *res0;
    }
    if (obj.contains("terminal"))
        result._terminal = obj.value("terminal").toBool();
    if (obj.contains("session") && !obj["session"].isNull()) {
        const auto res1 = fromJson<ClientSessionCapabilities>(obj["session"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._session = *res1;
    }
    if (obj.contains("auth") && obj["auth"].isObject()) {
        const auto res2 = fromJson<AuthCapabilities>(obj["auth"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._auth = *res2;
    }
    if (obj.contains("elicitation") && !obj["elicitation"].isNull()) {
        const auto res3 = fromJson<ElicitationCapabilities>(obj["elicitation"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._elicitation = *res3;
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const ClientCapabilities &data)
{
    QJsonObject obj;
    if (data._fs.has_value())
        obj.insert("fs", toJson(*data._fs));
    if (data._terminal.has_value())
        obj.insert("terminal", *data._terminal);
    if (data._session.has_value())
        obj.insert("session", toJson(*data._session));
    if (data._auth.has_value())
        obj.insert("auth", toJson(*data._auth));
    if (data._elicitation.has_value())
        obj.insert("elicitation", toJson(*data._elicitation));
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

template<>
Utils::Result<InitializeRequest> fromJson<InitializeRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InitializeRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("protocolVersion"))
        return Utils::ResultError("Missing required field: protocolVersion");
    InitializeRequest result;
    if (obj.contains("protocolVersion") && obj["protocolVersion"].isDouble()) {
        const auto res0 = fromJson<ProtocolVersion>(obj["protocolVersion"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._protocolVersion = *res0;
    }
    if (obj.contains("clientCapabilities") && obj["clientCapabilities"].isObject()) {
        const auto res1 = fromJson<ClientCapabilities>(obj["clientCapabilities"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._clientCapabilities = *res1;
    }
    if (obj.contains("clientInfo") && !obj["clientInfo"].isNull()) {
        const auto res2 = fromJson<Implementation>(obj["clientInfo"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._clientInfo = *res2;
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for McpServerHttp");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    if (!obj.contains("url"))
        return Utils::ResultError("Missing required field: url");
    if (!obj.contains("headers"))
        return Utils::ResultError("Missing required field: headers");
    McpServerHttp result;
    result._name = obj.value("name").toString();
    result._url = obj.value("url").toString();
    if (obj.contains("headers") && obj["headers"].isArray()) {
        const QJsonArray arr = obj["headers"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<HttpHeader>(v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._headers.append(*res0);
        }
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for McpServerSse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    if (!obj.contains("url"))
        return Utils::ResultError("Missing required field: url");
    if (!obj.contains("headers"))
        return Utils::ResultError("Missing required field: headers");
    McpServerSse result;
    result._name = obj.value("name").toString();
    result._url = obj.value("url").toString();
    if (obj.contains("headers") && obj["headers"].isArray()) {
        const QJsonArray arr = obj["headers"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<HttpHeader>(v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._headers.append(*res0);
        }
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for McpServerStdio");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    if (!obj.contains("command"))
        return Utils::ResultError("Missing required field: command");
    if (!obj.contains("args"))
        return Utils::ResultError("Missing required field: args");
    if (!obj.contains("env"))
        return Utils::ResultError("Missing required field: env");
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
            const auto res0 = fromJson<EnvVariable>(v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._env.append(*res0);
        }
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Invalid McpServer: expected object");
    const QString dispatchValue = val.toObject().value("type").toString();
    if (dispatchValue == "http") {
        const auto res0 = fromJson<McpServerHttp>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return McpServer(*res0);
    }
    else if (dispatchValue == "sse") {
        const auto res1 = fromJson<McpServerSse>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        return McpServer(*res1);
    }
    else if (dispatchValue == "stdio") {
        const auto res2 = fromJson<McpServerStdio>(val);
        if (!res2)
            return Utils::ResultError(res2.error());
        return McpServer(*res2);
    }
    return Utils::ResultError("Invalid McpServer: unknown type \"" + dispatchValue + "\"");
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
        return Utils::ResultError("Expected JSON object for LoadSessionRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("mcpServers"))
        return Utils::ResultError("Missing required field: mcpServers");
    if (!obj.contains("cwd"))
        return Utils::ResultError("Missing required field: cwd");
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    LoadSessionRequest result;
    if (obj.contains("mcpServers") && obj["mcpServers"].isArray()) {
        const QJsonArray arr = obj["mcpServers"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<McpServer>(v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._mcpServers.append(*res0);
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
    if (obj.contains("sessionId") && obj["sessionId"].isString()) {
        const auto res1 = fromJson<SessionId>(obj["sessionId"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._sessionId = *res1;
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for NewSessionRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("cwd"))
        return Utils::ResultError("Missing required field: cwd");
    if (!obj.contains("mcpServers"))
        return Utils::ResultError("Missing required field: mcpServers");
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
            const auto res0 = fromJson<McpServer>(v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._mcpServers.append(*res0);
        }
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for PromptRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    if (!obj.contains("prompt"))
        return Utils::ResultError("Missing required field: prompt");
    PromptRequest result;
    if (obj.contains("sessionId") && obj["sessionId"].isString()) {
        const auto res0 = fromJson<SessionId>(obj["sessionId"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._sessionId = *res0;
    }
    if (obj.contains("prompt") && obj["prompt"].isArray()) {
        const QJsonArray arr = obj["prompt"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res1 = fromJson<ContentBlock>(v);
            if (!res1)
                return Utils::ResultError(res1.error());
            result._prompt.append(*res1);
        }
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for ResumeSessionRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    if (!obj.contains("cwd"))
        return Utils::ResultError("Missing required field: cwd");
    ResumeSessionRequest result;
    if (obj.contains("sessionId") && obj["sessionId"].isString()) {
        const auto res0 = fromJson<SessionId>(obj["sessionId"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._sessionId = *res0;
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
    if (obj.contains("mcpServers") && obj["mcpServers"].isArray()) {
        const QJsonArray arr = obj["mcpServers"].toArray();
        QList<McpServer> list_mcpServers;
        for (const QJsonValue &v : arr) {
            const auto res1 = fromJson<McpServer>(v);
            if (!res1)
                return Utils::ResultError(res1.error());
            list_mcpServers.append(*res1);
        }
        result._mcpServers = list_mcpServers;
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for SetSessionConfigOptionRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    if (!obj.contains("configId"))
        return Utils::ResultError("Missing required field: configId");
    SetSessionConfigOptionRequest result;
    if (obj.contains("sessionId") && obj["sessionId"].isString()) {
        const auto res0 = fromJson<SessionId>(obj["sessionId"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._sessionId = *res0;
    }
    if (obj.contains("configId") && obj["configId"].isString()) {
        const auto res1 = fromJson<SessionConfigId>(obj["configId"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._configId = *res1;
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    {
        const QSet<QString> knownKeys{"sessionId", "configId", "_meta"};
        for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
            if (!knownKeys.contains(it.key()))
                result._additionalProperties.insert(it.key(), it.value());
        }
    }
    return result;
}

QJsonObject toJson(const SetSessionConfigOptionRequest &data)
{
    QJsonObject obj{
        {"sessionId", data._sessionId},
        {"configId", data._configId}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    for (auto it = data._additionalProperties.constBegin(); it != data._additionalProperties.constEnd(); ++it)
        obj.insert(it.key(), it.value());
    return obj;
}

template<>
Utils::Result<SetSessionModeRequest> fromJson<SetSessionModeRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SetSessionModeRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    if (!obj.contains("modeId"))
        return Utils::ResultError("Missing required field: modeId");
    SetSessionModeRequest result;
    if (obj.contains("sessionId") && obj["sessionId"].isString()) {
        const auto res0 = fromJson<SessionId>(obj["sessionId"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._sessionId = *res0;
    }
    if (obj.contains("modeId") && obj["modeId"].isString()) {
        const auto res1 = fromJson<SessionModeId>(obj["modeId"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._modeId = *res1;
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for ClientRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    ClientRequest result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<RequestId>(obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    result._method = obj.value("method").toString();
    if (obj.contains("params"))
        result._params = obj.value("params").toString();
    return result;
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
Utils::Result<ElicitationContentValue> fromJson<ElicitationContentValue>(const QJsonValue &val)
{
    if (val.isString())
        return ElicitationContentValue(val.toString());
    if (val.isDouble()) {
        const double d = val.toDouble();
        if (d == std::trunc(d)
                && d >= double(std::numeric_limits<int>::min())
                && d <= double(std::numeric_limits<int>::max())) {
            return ElicitationContentValue(static_cast<int>(d));
        }
    }
    if (val.isDouble())
        return ElicitationContentValue(val.toDouble());
    if (val.isBool())
        return ElicitationContentValue(val.toBool());
    if (val.isArray()) {
        QList<QString> list;
        for (const auto &elem : val.toArray())
            list.append(elem.toString());
        return ElicitationContentValue(std::move(list));
    }
    return Utils::ResultError("Invalid ElicitationContentValue");
}

QJsonValue toJsonValue(const ElicitationContentValue &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QList<QString>>) {
            QJsonArray arr;
            for (const auto &elem : v)
                arr.append(elem);
            return arr;
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<ElicitationAcceptAction> fromJson<ElicitationAcceptAction>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ElicitationAcceptAction");
    const QJsonObject obj = val.toObject();
    ElicitationAcceptAction result;
    if (obj.contains("content"))
        if (!obj["content"].isNull()) {
            result._content = obj.value("content").toObject();
        }
    return result;
}

QJsonObject toJson(const ElicitationAcceptAction &data)
{
    QJsonObject obj;
    if (data._content.has_value())
        obj.insert("content", *data._content);
    return obj;
}

template<>
Utils::Result<CreateElicitationResponse> fromJson<CreateElicitationResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CreateElicitationResponse");
    const QJsonObject obj = val.toObject();
    CreateElicitationResponse result;
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
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

QJsonObject toJson(const CreateElicitationResponse &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    for (auto it = data._additionalProperties.constBegin(); it != data._additionalProperties.constEnd(); ++it)
        obj.insert(it.key(), it.value());
    return obj;
}

template<>
Utils::Result<CreateTerminalResponse> fromJson<CreateTerminalResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CreateTerminalResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("terminalId"))
        return Utils::ResultError("Missing required field: terminalId");
    CreateTerminalResponse result;
    if (obj.contains("terminalId") && obj["terminalId"].isString()) {
        const auto res0 = fromJson<TerminalId>(obj["terminalId"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._terminalId = *res0;
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for SelectedPermissionOutcome");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("optionId"))
        return Utils::ResultError("Missing required field: optionId");
    SelectedPermissionOutcome result;
    if (obj.contains("optionId") && obj["optionId"].isString()) {
        const auto res0 = fromJson<PermissionOptionId>(obj["optionId"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._optionId = *res0;
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Invalid RequestPermissionOutcome: expected object");
    const QJsonObject obj = val.toObject();
    const QString kind = obj.value("outcome").toString();
    RequestPermissionOutcome result;
    result._kind = kind;
    if (kind == "cancelled")
        result._value = std::monostate{};
    else if (kind == "selected") {
        const auto res0 = fromJson<SelectedPermissionOutcome>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._value = *res0;
    }
    else
        return Utils::ResultError("Invalid RequestPermissionOutcome: unknown outcome \"" + kind + "\"");
    return result;
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
        return Utils::ResultError("Expected JSON object for RequestPermissionResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("outcome"))
        return Utils::ResultError("Missing required field: outcome");
    RequestPermissionResponse result;
    if (obj.contains("outcome")) {
        const auto res0 = fromJson<RequestPermissionOutcome>(obj["outcome"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._outcome = *res0;
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for TerminalOutputResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("output"))
        return Utils::ResultError("Missing required field: output");
    if (!obj.contains("truncated"))
        return Utils::ResultError("Missing required field: truncated");
    TerminalOutputResponse result;
    result._output = obj.value("output").toString();
    result._truncated = obj.value("truncated").toBool();
    if (obj.contains("exitStatus") && !obj["exitStatus"].isNull()) {
        const auto res0 = fromJson<TerminalExitStatus>(obj["exitStatus"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._exitStatus = *res0;
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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
        return Utils::ResultError("Expected JSON object for CancelNotification");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    CancelNotification result;
    if (obj.contains("sessionId") && obj["sessionId"].isString()) {
        const auto res0 = fromJson<SessionId>(obj["sessionId"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._sessionId = *res0;
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
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

template<>
Utils::Result<CancelRequestNotification> fromJson<CancelRequestNotification>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CancelRequestNotification");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("requestId"))
        return Utils::ResultError("Missing required field: requestId");
    CancelRequestNotification result;
    if (obj.contains("requestId")) {
        const auto res0 = fromJson<RequestId>(obj["requestId"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._requestId = *res0;
    }
    if (obj.contains("_meta"))
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        }
    return result;
}

QJsonObject toJson(const CancelRequestNotification &data)
{
    QJsonObject obj{{"requestId", toJsonValue(data._requestId)}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    return obj;
}

} // namespace Acp
