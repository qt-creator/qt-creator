// This file is auto-generated. Do not edit manually.
#include "acpv2.h"

namespace Acp::V2 {

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
        co_return Utils::ResultError("Expected JSON object for ElicitationRequestScope");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("requestId"))
        co_return Utils::ResultError("Missing required field: requestId");
    ElicitationRequestScope result;
    if (obj.contains("requestId"))
        result._requestId = co_await fromJson<RequestId>(obj["requestId"]);
    co_return result;
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
    if (obj.contains("title")) {
        if (!obj["title"].isNull()) {
            result._title = obj.value("title").toString();
        } else {
            result._title = std::nullopt;
        }
    }
    if (obj.contains("description")) {
        if (!obj["description"].isNull()) {
            result._description = obj.value("description").toString();
        } else {
            result._description = std::nullopt;
        }
    }
    if (obj.contains("default")) {
        if (!obj["default"].isNull()) {
            result._default_ = obj.value("default").toBool();
        } else {
            result._default_ = std::nullopt;
        }
    }
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const BooleanPropertySchema &data)
{
    QJsonObject obj;
    if (data._title.has_value())
        obj.insert("title", *data._title);
    else if (data._title.isNull())
        obj.insert("title", QJsonValue::Null);
    if (data._description.has_value())
        obj.insert("description", *data._description);
    else if (data._description.isNull())
        obj.insert("description", QJsonValue::Null);
    if (data._default_.has_value())
        obj.insert("default", *data._default_);
    else if (data._default_.isNull())
        obj.insert("default", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<IntegerPropertySchema> fromJson<IntegerPropertySchema>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for IntegerPropertySchema");
    const QJsonObject obj = val.toObject();
    IntegerPropertySchema result;
    if (obj.contains("title")) {
        if (!obj["title"].isNull()) {
            result._title = obj.value("title").toString();
        } else {
            result._title = std::nullopt;
        }
    }
    if (obj.contains("description")) {
        if (!obj["description"].isNull()) {
            result._description = obj.value("description").toString();
        } else {
            result._description = std::nullopt;
        }
    }
    if (obj.contains("minimum")) {
        if (!obj["minimum"].isNull()) {
            result._minimum = obj.value("minimum").toInt();
        } else {
            result._minimum = std::nullopt;
        }
    }
    if (obj.contains("maximum")) {
        if (!obj["maximum"].isNull()) {
            result._maximum = obj.value("maximum").toInt();
        } else {
            result._maximum = std::nullopt;
        }
    }
    if (obj.contains("default")) {
        if (!obj["default"].isNull()) {
            result._default_ = obj.value("default").toInt();
        } else {
            result._default_ = std::nullopt;
        }
    }
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const IntegerPropertySchema &data)
{
    QJsonObject obj;
    if (data._title.has_value())
        obj.insert("title", *data._title);
    else if (data._title.isNull())
        obj.insert("title", QJsonValue::Null);
    if (data._description.has_value())
        obj.insert("description", *data._description);
    else if (data._description.isNull())
        obj.insert("description", QJsonValue::Null);
    if (data._minimum.has_value())
        obj.insert("minimum", *data._minimum);
    else if (data._minimum.isNull())
        obj.insert("minimum", QJsonValue::Null);
    if (data._maximum.has_value())
        obj.insert("maximum", *data._maximum);
    else if (data._maximum.isNull())
        obj.insert("maximum", QJsonValue::Null);
    if (data._default_.has_value())
        obj.insert("default", *data._default_);
    else if (data._default_.isNull())
        obj.insert("default", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
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
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
    if (obj.contains("description")) {
        if (!obj["description"].isNull()) {
            result._description = obj.value("description").toString();
        } else {
            result._description = std::nullopt;
        }
    }
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
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
    else if (data._description.isNull())
        obj.insert("description", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<TitledMultiSelectItems> fromJson<TitledMultiSelectItems>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for TitledMultiSelectItems");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("anyOf"))
        co_return Utils::ResultError("Missing required field: anyOf");
    TitledMultiSelectItems result;
    if (obj.contains("anyOf") && obj["anyOf"].isArray()) {
        const QJsonArray arr = obj["anyOf"].toArray();
        for (const QJsonValue &v : arr) {
            result._anyOf.append(co_await fromJson<EnumOption>(v));
        }
    }
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const TitledMultiSelectItems &data)
{
    QJsonObject obj;
    QJsonArray arr_anyOf;
    for (const auto &v : data._anyOf) arr_anyOf.append(toJson(v));
    obj.insert("anyOf", arr_anyOf);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<MultiSelectItems> fromJson<MultiSelectItems>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Invalid MultiSelectItems: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("enum"))
        co_return MultiSelectItems(co_await fromJson<StringMultiSelectItems>(val));
    if (obj.contains("anyOf"))
        co_return MultiSelectItems(co_await fromJson<TitledMultiSelectItems>(val));
    if (val.isObject())
        co_return MultiSelectItems(val.toObject());  // open union: preserve unknown variants raw
    co_return Utils::ResultError("Invalid MultiSelectItems");
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
        co_return Utils::ResultError("Expected JSON object for MultiSelectPropertySchema");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("items"))
        co_return Utils::ResultError("Missing required field: items");
    MultiSelectPropertySchema result;
    if (obj.contains("title")) {
        if (!obj["title"].isNull()) {
            result._title = obj.value("title").toString();
        } else {
            result._title = std::nullopt;
        }
    }
    if (obj.contains("description")) {
        if (!obj["description"].isNull()) {
            result._description = obj.value("description").toString();
        } else {
            result._description = std::nullopt;
        }
    }
    if (obj.contains("minItems")) {
        if (!obj["minItems"].isNull()) {
            result._minItems = obj.value("minItems").toInt();
        } else {
            result._minItems = std::nullopt;
        }
    }
    if (obj.contains("maxItems")) {
        if (!obj["maxItems"].isNull()) {
            result._maxItems = obj.value("maxItems").toInt();
        } else {
            result._maxItems = std::nullopt;
        }
    }
    if (obj.contains("items"))
        result._items = co_await fromJson<MultiSelectItems>(obj["items"]);
    if (obj.contains("default")) {
        if (!obj["default"].isNull()) {
            result._default_ = obj.value("default").toArray();
        } else {
            result._default_ = std::nullopt;
        }
    }
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const MultiSelectPropertySchema &data)
{
    QJsonObject obj{{"items", toJsonValue(data._items)}};
    if (data._title.has_value())
        obj.insert("title", *data._title);
    else if (data._title.isNull())
        obj.insert("title", QJsonValue::Null);
    if (data._description.has_value())
        obj.insert("description", *data._description);
    else if (data._description.isNull())
        obj.insert("description", QJsonValue::Null);
    if (data._minItems.has_value())
        obj.insert("minItems", *data._minItems);
    else if (data._minItems.isNull())
        obj.insert("minItems", QJsonValue::Null);
    if (data._maxItems.has_value())
        obj.insert("maxItems", *data._maxItems);
    else if (data._maxItems.isNull())
        obj.insert("maxItems", QJsonValue::Null);
    if (data._default_.has_value())
        obj.insert("default", *data._default_);
    else if (data._default_.isNull())
        obj.insert("default", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<NumberPropertySchema> fromJson<NumberPropertySchema>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for NumberPropertySchema");
    const QJsonObject obj = val.toObject();
    NumberPropertySchema result;
    if (obj.contains("title")) {
        if (!obj["title"].isNull()) {
            result._title = obj.value("title").toString();
        } else {
            result._title = std::nullopt;
        }
    }
    if (obj.contains("description")) {
        if (!obj["description"].isNull()) {
            result._description = obj.value("description").toString();
        } else {
            result._description = std::nullopt;
        }
    }
    if (obj.contains("minimum")) {
        if (!obj["minimum"].isNull()) {
            result._minimum = obj.value("minimum").toDouble();
        } else {
            result._minimum = std::nullopt;
        }
    }
    if (obj.contains("maximum")) {
        if (!obj["maximum"].isNull()) {
            result._maximum = obj.value("maximum").toDouble();
        } else {
            result._maximum = std::nullopt;
        }
    }
    if (obj.contains("default")) {
        if (!obj["default"].isNull()) {
            result._default_ = obj.value("default").toDouble();
        } else {
            result._default_ = std::nullopt;
        }
    }
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const NumberPropertySchema &data)
{
    QJsonObject obj;
    if (data._title.has_value())
        obj.insert("title", *data._title);
    else if (data._title.isNull())
        obj.insert("title", QJsonValue::Null);
    if (data._description.has_value())
        obj.insert("description", *data._description);
    else if (data._description.isNull())
        obj.insert("description", QJsonValue::Null);
    if (data._minimum.has_value())
        obj.insert("minimum", *data._minimum);
    else if (data._minimum.isNull())
        obj.insert("minimum", QJsonValue::Null);
    if (data._maximum.has_value())
        obj.insert("maximum", *data._maximum);
    else if (data._maximum.isNull())
        obj.insert("maximum", QJsonValue::Null);
    if (data._default_.has_value())
        obj.insert("default", *data._default_);
    else if (data._default_.isNull())
        obj.insert("default", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
        co_return Utils::ResultError("Expected JSON object for StringPropertySchema");
    const QJsonObject obj = val.toObject();
    StringPropertySchema result;
    if (obj.contains("title")) {
        if (!obj["title"].isNull()) {
            result._title = obj.value("title").toString();
        } else {
            result._title = std::nullopt;
        }
    }
    if (obj.contains("description")) {
        if (!obj["description"].isNull()) {
            result._description = obj.value("description").toString();
        } else {
            result._description = std::nullopt;
        }
    }
    if (obj.contains("minLength")) {
        if (!obj["minLength"].isNull()) {
            result._minLength = obj.value("minLength").toInt();
        } else {
            result._minLength = std::nullopt;
        }
    }
    if (obj.contains("maxLength")) {
        if (!obj["maxLength"].isNull()) {
            result._maxLength = obj.value("maxLength").toInt();
        } else {
            result._maxLength = std::nullopt;
        }
    }
    if (obj.contains("pattern")) {
        if (!obj["pattern"].isNull()) {
            result._pattern = obj.value("pattern").toString();
        } else {
            result._pattern = std::nullopt;
        }
    }
    if (obj.contains("format") && !obj["format"].isNull())
        result._format = co_await fromJson<StringFormat>(obj["format"]);
    else if (obj.contains("format"))
        result._format = std::nullopt;
    if (obj.contains("default")) {
        if (!obj["default"].isNull()) {
            result._default_ = obj.value("default").toString();
        } else {
            result._default_ = std::nullopt;
        }
    }
    if (obj.contains("enum")) {
        if (!obj["enum"].isNull()) {
            result._enum_ = obj.value("enum").toArray();
        } else {
            result._enum_ = std::nullopt;
        }
    }
    if (obj.contains("oneOf") && obj["oneOf"].isArray()) {
        const QJsonArray arr = obj["oneOf"].toArray();
        QList<EnumOption> list_oneOf;
        for (const QJsonValue &v : arr) {
            list_oneOf.append(co_await fromJson<EnumOption>(v));
        }
        result._oneOf = list_oneOf;
    }
    else if (obj.contains("oneOf") && obj["oneOf"].isNull())
        result._oneOf = std::nullopt;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const StringPropertySchema &data)
{
    QJsonObject obj;
    if (data._title.has_value())
        obj.insert("title", *data._title);
    else if (data._title.isNull())
        obj.insert("title", QJsonValue::Null);
    if (data._description.has_value())
        obj.insert("description", *data._description);
    else if (data._description.isNull())
        obj.insert("description", QJsonValue::Null);
    if (data._minLength.has_value())
        obj.insert("minLength", *data._minLength);
    else if (data._minLength.isNull())
        obj.insert("minLength", QJsonValue::Null);
    if (data._maxLength.has_value())
        obj.insert("maxLength", *data._maxLength);
    else if (data._maxLength.isNull())
        obj.insert("maxLength", QJsonValue::Null);
    if (data._pattern.has_value())
        obj.insert("pattern", *data._pattern);
    else if (data._pattern.isNull())
        obj.insert("pattern", QJsonValue::Null);
    if (data._format.has_value())
        obj.insert("format", toJsonValue(*data._format));
    else if (data._format.isNull())
        obj.insert("format", QJsonValue::Null);
    if (data._default_.has_value())
        obj.insert("default", *data._default_);
    else if (data._default_.isNull())
        obj.insert("default", QJsonValue::Null);
    if (data._enum_.has_value())
        obj.insert("enum", *data._enum_);
    else if (data._enum_.isNull())
        obj.insert("enum", QJsonValue::Null);
    if (data._oneOf.has_value()) {
        QJsonArray arr_oneOf;
        for (const auto &v : *data._oneOf) arr_oneOf.append(toJson(v));
        obj.insert("oneOf", arr_oneOf);
    }
    else if (data._oneOf.isNull())
        obj.insert("oneOf", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<ElicitationPropertySchema> fromJson<ElicitationPropertySchema>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Invalid ElicitationPropertySchema: expected object");
    const QString dispatchValue = val.toObject().value("type").toString();
    if (dispatchValue == "string")
        co_return ElicitationPropertySchema(co_await fromJson<StringPropertySchema>(val));
    else if (dispatchValue == "number")
        co_return ElicitationPropertySchema(co_await fromJson<NumberPropertySchema>(val));
    else if (dispatchValue == "integer")
        co_return ElicitationPropertySchema(co_await fromJson<IntegerPropertySchema>(val));
    else if (dispatchValue == "boolean")
        co_return ElicitationPropertySchema(co_await fromJson<BooleanPropertySchema>(val));
    else if (dispatchValue == "array")
        co_return ElicitationPropertySchema(co_await fromJson<MultiSelectPropertySchema>(val));
    if (dispatchValue.isEmpty())
        co_return Utils::ResultError("Invalid ElicitationPropertySchema: missing type");
    co_return ElicitationPropertySchema(val.toObject());  // open union: preserve unknown variants raw
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
        co_return Utils::ResultError("Expected JSON object for ElicitationSchema");
    const QJsonObject obj = val.toObject();
    ElicitationSchema result;
    if (obj.contains("type") && obj["type"].isString())
        result._type = co_await fromJson<ElicitationSchemaType>(obj["type"]);
    if (obj.contains("title")) {
        if (!obj["title"].isNull()) {
            result._title = obj.value("title").toString();
        } else {
            result._title = std::nullopt;
        }
    }
    if (obj.contains("properties") && obj["properties"].isObject()) {
        const QJsonObject mapObj_properties = obj["properties"].toObject();
        QMap<QString, ElicitationPropertySchema> map_properties;
        for (auto it = mapObj_properties.constBegin(); it != mapObj_properties.constEnd(); ++it) {
            map_properties.insert(it.key(), co_await fromJson<ElicitationPropertySchema>(it.value()));
        }
        result._properties = map_properties;
    }
    if (obj.contains("required")) {
        if (!obj["required"].isNull()) {
            result._required = obj.value("required").toArray();
        } else {
            result._required = std::nullopt;
        }
    }
    if (obj.contains("description")) {
        if (!obj["description"].isNull()) {
            result._description = obj.value("description").toString();
        } else {
            result._description = std::nullopt;
        }
    }
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const ElicitationSchema &data)
{
    QJsonObject obj;
    if (data._type.has_value())
        obj.insert("type", toJsonValue(*data._type));
    if (data._title.has_value())
        obj.insert("title", *data._title);
    else if (data._title.isNull())
        obj.insert("title", QJsonValue::Null);
    if (data._properties.has_value()) {
        QJsonObject map_properties;
        for (auto it = data._properties->constBegin(); it != data._properties->constEnd(); ++it)
            map_properties.insert(it.key(), toJsonValue(it.value()));
        obj.insert("properties", map_properties);
    }
    if (data._required.has_value())
        obj.insert("required", *data._required);
    else if (data._required.isNull())
        obj.insert("required", QJsonValue::Null);
    if (data._description.has_value())
        obj.insert("description", *data._description);
    else if (data._description.isNull())
        obj.insert("description", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
        co_return Utils::ResultError("Expected JSON object for ElicitationSessionScope");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        co_return Utils::ResultError("Missing required field: sessionId");
    ElicitationSessionScope result;
    if (obj.contains("sessionId") && obj["sessionId"].isString())
        result._sessionId = co_await fromJson<SessionId>(obj["sessionId"]);
    if (obj.contains("toolCallId") && !obj["toolCallId"].isNull())
        result._toolCallId = co_await fromJson<ToolCallId>(obj["toolCallId"]);
    else if (obj.contains("toolCallId"))
        result._toolCallId = std::nullopt;
    co_return result;
}

QJsonObject toJson(const ElicitationSessionScope &data)
{
    QJsonObject obj{{"sessionId", data._sessionId}};
    if (data._toolCallId.has_value())
        obj.insert("toolCallId", *data._toolCallId);
    else if (data._toolCallId.isNull())
        obj.insert("toolCallId", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<ElicitationFormMode> fromJson<ElicitationFormMode>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for ElicitationFormMode");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("requestedSchema"))
        co_return Utils::ResultError("Missing required field: requestedSchema");
    ElicitationFormMode result;
    if (obj.contains("requestedSchema") && obj["requestedSchema"].isObject())
        result._requestedSchema = co_await fromJson<ElicitationSchema>(obj["requestedSchema"]);
    {
        const QSet<QString> knownKeys{"requestedSchema"};
        for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
            if (!knownKeys.contains(it.key()))
                result._additionalProperties.insert(it.key(), it.value());
        }
    }
    co_return result;
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
        co_return Utils::ResultError("Expected JSON object for ElicitationUrlMode");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("elicitationId"))
        co_return Utils::ResultError("Missing required field: elicitationId");
    if (!obj.contains("url"))
        co_return Utils::ResultError("Missing required field: url");
    ElicitationUrlMode result;
    if (obj.contains("elicitationId") && obj["elicitationId"].isString())
        result._elicitationId = co_await fromJson<ElicitationId>(obj["elicitationId"]);
    result._url = obj.value("url").toString();
    {
        const QSet<QString> knownKeys{"elicitationId", "url"};
        for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
            if (!knownKeys.contains(it.key()))
                result._additionalProperties.insert(it.key(), it.value());
        }
    }
    co_return result;
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
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
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
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    for (auto it = data._additionalProperties.constBegin(); it != data._additionalProperties.constEnd(); ++it)
        obj.insert(it.key(), it.value());
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
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
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
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<CommandPermissionSubject> fromJson<CommandPermissionSubject>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for CommandPermissionSubject");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("command"))
        co_return Utils::ResultError("Missing required field: command");
    if (!obj.contains("cwd"))
        co_return Utils::ResultError("Missing required field: cwd");
    CommandPermissionSubject result;
    result._command = obj.value("command").toString();
    if (obj.contains("cwd") && obj["cwd"].isString())
        result._cwd = co_await fromJson<AbsolutePath>(obj["cwd"]);
    if (obj.contains("toolCallId") && !obj["toolCallId"].isNull())
        result._toolCallId = co_await fromJson<ToolCallId>(obj["toolCallId"]);
    else if (obj.contains("toolCallId"))
        result._toolCallId = std::nullopt;
    if (obj.contains("terminalId") && !obj["terminalId"].isNull())
        result._terminalId = co_await fromJson<TerminalId>(obj["terminalId"]);
    else if (obj.contains("terminalId"))
        result._terminalId = std::nullopt;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const CommandPermissionSubject &data)
{
    QJsonObject obj{
        {"command", data._command},
        {"cwd", data._cwd}
    };
    if (data._toolCallId.has_value())
        obj.insert("toolCallId", *data._toolCallId);
    else if (data._toolCallId.isNull())
        obj.insert("toolCallId", QJsonValue::Null);
    if (data._terminalId.has_value())
        obj.insert("terminalId", *data._terminalId);
    else if (data._terminalId.isNull())
        obj.insert("terminalId", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
        co_return Utils::ResultError("Expected JSON object for Annotations");
    const QJsonObject obj = val.toObject();
    Annotations result;
    if (obj.contains("audience") && obj["audience"].isArray()) {
        const QJsonArray arr = obj["audience"].toArray();
        QList<Role> list_audience;
        for (const QJsonValue &v : arr) {
            list_audience.append(co_await fromJson<Role>(v));
        }
        result._audience = list_audience;
    }
    else if (obj.contains("audience") && obj["audience"].isNull())
        result._audience = std::nullopt;
    if (obj.contains("lastModified")) {
        if (!obj["lastModified"].isNull()) {
            result._lastModified = obj.value("lastModified").toString();
        } else {
            result._lastModified = std::nullopt;
        }
    }
    if (obj.contains("priority")) {
        if (!obj["priority"].isNull()) {
            result._priority = obj.value("priority").toDouble();
        } else {
            result._priority = std::nullopt;
        }
    }
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const Annotations &data)
{
    QJsonObject obj;
    if (data._audience.has_value()) {
        QJsonArray arr_audience;
        for (const auto &v : *data._audience) arr_audience.append(toJsonValue(v));
        obj.insert("audience", arr_audience);
    }
    else if (data._audience.isNull())
        obj.insert("audience", QJsonValue::Null);
    if (data._lastModified.has_value())
        obj.insert("lastModified", *data._lastModified);
    else if (data._lastModified.isNull())
        obj.insert("lastModified", QJsonValue::Null);
    if (data._priority.has_value())
        obj.insert("priority", *data._priority);
    else if (data._priority.isNull())
        obj.insert("priority", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
    result._data = obj.value("data").toString();
    if (obj.contains("mimeType") && obj["mimeType"].isString())
        result._mimeType = co_await fromJson<MediaType>(obj["mimeType"]);
    if (obj.contains("annotations") && !obj["annotations"].isNull())
        result._annotations = co_await fromJson<Annotations>(obj["annotations"]);
    else if (obj.contains("annotations"))
        result._annotations = std::nullopt;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
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
    else if (data._annotations.isNull())
        obj.insert("annotations", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<BlobResourceContents> fromJson<BlobResourceContents>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for BlobResourceContents");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("blob"))
        co_return Utils::ResultError("Missing required field: blob");
    if (!obj.contains("uri"))
        co_return Utils::ResultError("Missing required field: uri");
    BlobResourceContents result;
    result._blob = obj.value("blob").toString();
    result._uri = obj.value("uri").toString();
    if (obj.contains("mimeType") && !obj["mimeType"].isNull())
        result._mimeType = co_await fromJson<MediaType>(obj["mimeType"]);
    else if (obj.contains("mimeType"))
        result._mimeType = std::nullopt;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const BlobResourceContents &data)
{
    QJsonObject obj{
        {"blob", data._blob},
        {"uri", data._uri}
    };
    if (data._mimeType.has_value())
        obj.insert("mimeType", *data._mimeType);
    else if (data._mimeType.isNull())
        obj.insert("mimeType", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<TextResourceContents> fromJson<TextResourceContents>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for TextResourceContents");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("text"))
        co_return Utils::ResultError("Missing required field: text");
    if (!obj.contains("uri"))
        co_return Utils::ResultError("Missing required field: uri");
    TextResourceContents result;
    result._text = obj.value("text").toString();
    result._uri = obj.value("uri").toString();
    if (obj.contains("mimeType") && !obj["mimeType"].isNull())
        result._mimeType = co_await fromJson<MediaType>(obj["mimeType"]);
    else if (obj.contains("mimeType"))
        result._mimeType = std::nullopt;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const TextResourceContents &data)
{
    QJsonObject obj{
        {"text", data._text},
        {"uri", data._uri}
    };
    if (data._mimeType.has_value())
        obj.insert("mimeType", *data._mimeType);
    else if (data._mimeType.isNull())
        obj.insert("mimeType", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
    if (obj.contains("resource"))
        result._resource = co_await fromJson<EmbeddedResourceResource>(obj["resource"]);
    if (obj.contains("annotations") && !obj["annotations"].isNull())
        result._annotations = co_await fromJson<Annotations>(obj["annotations"]);
    else if (obj.contains("annotations"))
        result._annotations = std::nullopt;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const EmbeddedResource &data)
{
    QJsonObject obj{{"resource", toJsonValue(data._resource)}};
    if (data._annotations.has_value())
        obj.insert("annotations", toJson(*data._annotations));
    else if (data._annotations.isNull())
        obj.insert("annotations", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
    result._data = obj.value("data").toString();
    if (obj.contains("mimeType") && obj["mimeType"].isString())
        result._mimeType = co_await fromJson<MediaType>(obj["mimeType"]);
    if (obj.contains("uri")) {
        if (!obj["uri"].isNull()) {
            result._uri = obj.value("uri").toString();
        } else {
            result._uri = std::nullopt;
        }
    }
    if (obj.contains("annotations") && !obj["annotations"].isNull())
        result._annotations = co_await fromJson<Annotations>(obj["annotations"]);
    else if (obj.contains("annotations"))
        result._annotations = std::nullopt;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const ImageContent &data)
{
    QJsonObject obj{
        {"data", data._data},
        {"mimeType", data._mimeType}
    };
    if (data._uri.has_value())
        obj.insert("uri", *data._uri);
    else if (data._uri.isNull())
        obj.insert("uri", QJsonValue::Null);
    if (data._annotations.has_value())
        obj.insert("annotations", toJson(*data._annotations));
    else if (data._annotations.isNull())
        obj.insert("annotations", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

QString toString(IconTheme v)
{
    switch(v) {
        case IconTheme::light: return "light";
        case IconTheme::dark: return "dark";
    }
    return {};
}

template<>
Utils::Result<IconTheme> fromJson<IconTheme>(const QJsonValue &val)
{
    const QString str = val.toString();
    if (str == "light") return IconTheme::light;
    if (str == "dark") return IconTheme::dark;
    return Utils::ResultError("Invalid IconTheme value: " + str);
}

QJsonValue toJsonValue(const IconTheme &v)
{
    return toString(v);
}

template<>
Utils::Result<Icon> fromJson<Icon>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for Icon");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("src"))
        co_return Utils::ResultError("Missing required field: src");
    Icon result;
    result._src = obj.value("src").toString();
    if (obj.contains("mimeType") && !obj["mimeType"].isNull())
        result._mimeType = co_await fromJson<MediaType>(obj["mimeType"]);
    else if (obj.contains("mimeType"))
        result._mimeType = std::nullopt;
    if (obj.contains("sizes")) {
        if (!obj["sizes"].isNull()) {
            result._sizes = obj.value("sizes").toArray();
        } else {
            result._sizes = std::nullopt;
        }
    }
    if (obj.contains("theme") && !obj["theme"].isNull())
        result._theme = co_await fromJson<IconTheme>(obj["theme"]);
    else if (obj.contains("theme"))
        result._theme = std::nullopt;
    co_return result;
}

QJsonObject toJson(const Icon &data)
{
    QJsonObject obj{{"src", data._src}};
    if (data._mimeType.has_value())
        obj.insert("mimeType", *data._mimeType);
    else if (data._mimeType.isNull())
        obj.insert("mimeType", QJsonValue::Null);
    if (data._sizes.has_value())
        obj.insert("sizes", *data._sizes);
    else if (data._sizes.isNull())
        obj.insert("sizes", QJsonValue::Null);
    if (data._theme.has_value())
        obj.insert("theme", toJsonValue(*data._theme));
    else if (data._theme.isNull())
        obj.insert("theme", QJsonValue::Null);
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
    result._name = obj.value("name").toString();
    result._uri = obj.value("uri").toString();
    if (obj.contains("title")) {
        if (!obj["title"].isNull()) {
            result._title = obj.value("title").toString();
        } else {
            result._title = std::nullopt;
        }
    }
    if (obj.contains("description")) {
        if (!obj["description"].isNull()) {
            result._description = obj.value("description").toString();
        } else {
            result._description = std::nullopt;
        }
    }
    if (obj.contains("icons") && obj["icons"].isArray()) {
        const QJsonArray arr = obj["icons"].toArray();
        QList<Icon> list_icons;
        for (const QJsonValue &v : arr) {
            list_icons.append(co_await fromJson<Icon>(v));
        }
        result._icons = list_icons;
    }
    else if (obj.contains("icons") && obj["icons"].isNull())
        result._icons = std::nullopt;
    if (obj.contains("mimeType") && !obj["mimeType"].isNull())
        result._mimeType = co_await fromJson<MediaType>(obj["mimeType"]);
    else if (obj.contains("mimeType"))
        result._mimeType = std::nullopt;
    if (obj.contains("size")) {
        if (!obj["size"].isNull()) {
            result._size = obj.value("size").toInt();
        } else {
            result._size = std::nullopt;
        }
    }
    if (obj.contains("annotations") && !obj["annotations"].isNull())
        result._annotations = co_await fromJson<Annotations>(obj["annotations"]);
    else if (obj.contains("annotations"))
        result._annotations = std::nullopt;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const ResourceLink &data)
{
    QJsonObject obj{
        {"name", data._name},
        {"uri", data._uri}
    };
    if (data._title.has_value())
        obj.insert("title", *data._title);
    else if (data._title.isNull())
        obj.insert("title", QJsonValue::Null);
    if (data._description.has_value())
        obj.insert("description", *data._description);
    else if (data._description.isNull())
        obj.insert("description", QJsonValue::Null);
    if (data._icons.has_value()) {
        QJsonArray arr_icons;
        for (const auto &v : *data._icons) arr_icons.append(toJson(v));
        obj.insert("icons", arr_icons);
    }
    else if (data._icons.isNull())
        obj.insert("icons", QJsonValue::Null);
    if (data._mimeType.has_value())
        obj.insert("mimeType", *data._mimeType);
    else if (data._mimeType.isNull())
        obj.insert("mimeType", QJsonValue::Null);
    if (data._size.has_value())
        obj.insert("size", *data._size);
    else if (data._size.isNull())
        obj.insert("size", QJsonValue::Null);
    if (data._annotations.has_value())
        obj.insert("annotations", toJson(*data._annotations));
    else if (data._annotations.isNull())
        obj.insert("annotations", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
    result._text = obj.value("text").toString();
    if (obj.contains("annotations") && !obj["annotations"].isNull())
        result._annotations = co_await fromJson<Annotations>(obj["annotations"]);
    else if (obj.contains("annotations"))
        result._annotations = std::nullopt;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const TextContent &data)
{
    QJsonObject obj{{"text", data._text}};
    if (data._annotations.has_value())
        obj.insert("annotations", toJson(*data._annotations));
    else if (data._annotations.isNull())
        obj.insert("annotations", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
    if (dispatchValue.isEmpty())
        co_return Utils::ResultError("Invalid ContentBlock: missing type");
    co_return ContentBlock(val.toObject());  // open union: preserve unknown variants raw
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
        else if constexpr (std::is_same_v<T, QJsonObject>) return v.value("type").toString();
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
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const Content &data)
{
    QJsonObject obj{{"content", toJsonValue(data._content)}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

QString toString(DiffFileType v)
{
    switch(v) {
        case DiffFileType::text: return "text";
        case DiffFileType::binary: return "binary";
        case DiffFileType::directory: return "directory";
        case DiffFileType::symlink: return "symlink";
    }
    return {};
}

template<>
Utils::Result<DiffFileType> fromJson<DiffFileType>(const QJsonValue &val)
{
    const QString str = val.toString();
    if (str == "text") return DiffFileType::text;
    if (str == "binary") return DiffFileType::binary;
    if (str == "directory") return DiffFileType::directory;
    if (str == "symlink") return DiffFileType::symlink;
    return Utils::ResultError("Invalid DiffFileType value: " + str);
}

QJsonValue toJsonValue(const DiffFileType &v)
{
    return toString(v);
}

template<>
Utils::Result<DiffPathChange> fromJson<DiffPathChange>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for DiffPathChange");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("path"))
        co_return Utils::ResultError("Missing required field: path");
    DiffPathChange result;
    if (obj.contains("path") && obj["path"].isString())
        result._path = co_await fromJson<AbsolutePath>(obj["path"]);
    co_return result;
}

QJsonObject toJson(const DiffPathChange &data)
{
    QJsonObject obj{{"path", data._path}};
    return obj;
}

template<>
Utils::Result<DiffPathPairChange> fromJson<DiffPathPairChange>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for DiffPathPairChange");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("oldPath"))
        co_return Utils::ResultError("Missing required field: oldPath");
    if (!obj.contains("path"))
        co_return Utils::ResultError("Missing required field: path");
    DiffPathPairChange result;
    if (obj.contains("oldPath") && obj["oldPath"].isString())
        result._oldPath = co_await fromJson<AbsolutePath>(obj["oldPath"]);
    if (obj.contains("path") && obj["path"].isString())
        result._path = co_await fromJson<AbsolutePath>(obj["path"]);
    co_return result;
}

QJsonObject toJson(const DiffPathPairChange &data)
{
    QJsonObject obj{
        {"oldPath", data._oldPath},
        {"path", data._path}
    };
    return obj;
}

template<>
Utils::Result<DiffChange> fromJson<DiffChange>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for DiffChange");
    const QJsonObject obj = val.toObject();
    DiffChange result;
    if (obj.contains("fileType") && !obj["fileType"].isNull())
        result._fileType = co_await fromJson<DiffFileType>(obj["fileType"]);
    else if (obj.contains("fileType"))
        result._fileType = std::nullopt;
    if (obj.contains("mimeType") && !obj["mimeType"].isNull())
        result._mimeType = co_await fromJson<MediaType>(obj["mimeType"]);
    else if (obj.contains("mimeType"))
        result._mimeType = std::nullopt;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    {
        const QSet<QString> knownKeys{"fileType", "mimeType", "_meta"};
        for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
            if (!knownKeys.contains(it.key()))
                result._additionalProperties.insert(it.key(), it.value());
        }
    }
    co_return result;
}

QJsonObject toJson(const DiffChange &data)
{
    QJsonObject obj;
    if (data._fileType.has_value())
        obj.insert("fileType", toJsonValue(*data._fileType));
    else if (data._fileType.isNull())
        obj.insert("fileType", QJsonValue::Null);
    if (data._mimeType.has_value())
        obj.insert("mimeType", *data._mimeType);
    else if (data._mimeType.isNull())
        obj.insert("mimeType", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    for (auto it = data._additionalProperties.constBegin(); it != data._additionalProperties.constEnd(); ++it)
        obj.insert(it.key(), it.value());
    return obj;
}

QString toString(DiffPatchFormat v)
{
    switch(v) {
        case DiffPatchFormat::git_patch: return "git_patch";
    }
    return {};
}

template<>
Utils::Result<DiffPatchFormat> fromJson<DiffPatchFormat>(const QJsonValue &val)
{
    const QString str = val.toString();
    if (str == "git_patch") return DiffPatchFormat::git_patch;
    return Utils::ResultError("Invalid DiffPatchFormat value: " + str);
}

QJsonValue toJsonValue(const DiffPatchFormat &v)
{
    return toString(v);
}

template<>
Utils::Result<DiffPatch> fromJson<DiffPatch>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for DiffPatch");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("format"))
        co_return Utils::ResultError("Missing required field: format");
    if (!obj.contains("text"))
        co_return Utils::ResultError("Missing required field: text");
    DiffPatch result;
    if (obj.contains("format") && obj["format"].isString())
        result._format = co_await fromJson<DiffPatchFormat>(obj["format"]);
    result._text = obj.value("text").toString();
    co_return result;
}

QJsonObject toJson(const DiffPatch &data)
{
    QJsonObject obj{
        {"format", toJsonValue(data._format)},
        {"text", data._text}
    };
    return obj;
}

template<>
Utils::Result<Diff> fromJson<Diff>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for Diff");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("changes"))
        co_return Utils::ResultError("Missing required field: changes");
    Diff result;
    if (obj.contains("changes") && obj["changes"].isArray()) {
        const QJsonArray arr = obj["changes"].toArray();
        for (const QJsonValue &v : arr) {
            result._changes.append(co_await fromJson<DiffChange>(v));
        }
    }
    if (obj.contains("patch") && !obj["patch"].isNull())
        result._patch = co_await fromJson<DiffPatch>(obj["patch"]);
    else if (obj.contains("patch"))
        result._patch = std::nullopt;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const Diff &data)
{
    QJsonObject obj;
    QJsonArray arr_changes;
    for (const auto &v : data._changes) arr_changes.append(toJson(v));
    obj.insert("changes", arr_changes);
    if (data._patch.has_value())
        obj.insert("patch", toJson(*data._patch));
    else if (data._patch.isNull())
        obj.insert("patch", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const Terminal &data)
{
    QJsonObject obj{{"terminalId", data._terminalId}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
    if (dispatchValue.isEmpty())
        co_return Utils::ResultError("Invalid ToolCallContent: missing type");
    co_return ToolCallContent(val.toObject());  // open union: preserve unknown variants raw
}

QString dispatchValue(const ToolCallContent &val)
{
    return std::visit([](const auto &v) -> QString {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, Content>) return "content";
        else if constexpr (std::is_same_v<T, Diff>) return "diff";
        else if constexpr (std::is_same_v<T, Terminal>) return "terminal";
        else if constexpr (std::is_same_v<T, QJsonObject>) return v.value("type").toString();
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
        co_return Utils::ResultError("Expected JSON object for ToolCallLocation");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("path"))
        co_return Utils::ResultError("Missing required field: path");
    ToolCallLocation result;
    if (obj.contains("path") && obj["path"].isString())
        result._path = co_await fromJson<AbsolutePath>(obj["path"]);
    if (obj.contains("line")) {
        if (!obj["line"].isNull()) {
            result._line = obj.value("line").toInt();
        } else {
            result._line = std::nullopt;
        }
    }
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const ToolCallLocation &data)
{
    QJsonObject obj{{"path", data._path}};
    if (data._line.has_value())
        obj.insert("line", *data._line);
    else if (data._line.isNull())
        obj.insert("line", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

QString toString(ToolCallStatus v)
{
    switch(v) {
        case ToolCallStatus::pending: return "pending";
        case ToolCallStatus::in_progress: return "in_progress";
        case ToolCallStatus::completed: return "completed";
        case ToolCallStatus::failed: return "failed";
        case ToolCallStatus::cancelled: return "cancelled";
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
    if (str == "cancelled") return ToolCallStatus::cancelled;
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
    if (obj.contains("title")) {
        if (!obj["title"].isNull()) {
            result._title = obj.value("title").toString();
        } else {
            result._title = std::nullopt;
        }
    }
    if (obj.contains("kind") && !obj["kind"].isNull())
        result._kind = co_await fromJson<ToolKind>(obj["kind"]);
    else if (obj.contains("kind"))
        result._kind = std::nullopt;
    if (obj.contains("status") && !obj["status"].isNull())
        result._status = co_await fromJson<ToolCallStatus>(obj["status"]);
    else if (obj.contains("status"))
        result._status = std::nullopt;
    if (obj.contains("content") && obj["content"].isArray()) {
        const QJsonArray arr = obj["content"].toArray();
        QList<ToolCallContent> list_content;
        for (const QJsonValue &v : arr) {
            list_content.append(co_await fromJson<ToolCallContent>(v));
        }
        result._content = list_content;
    }
    else if (obj.contains("content") && obj["content"].isNull())
        result._content = std::nullopt;
    if (obj.contains("locations") && obj["locations"].isArray()) {
        const QJsonArray arr = obj["locations"].toArray();
        QList<ToolCallLocation> list_locations;
        for (const QJsonValue &v : arr) {
            list_locations.append(co_await fromJson<ToolCallLocation>(v));
        }
        result._locations = list_locations;
    }
    else if (obj.contains("locations") && obj["locations"].isNull())
        result._locations = std::nullopt;
    if (obj.contains("rawInput"))
        result._rawInput = obj.value("rawInput");
    if (obj.contains("rawOutput"))
        result._rawOutput = obj.value("rawOutput");
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const ToolCallUpdate &data)
{
    QJsonObject obj{{"toolCallId", data._toolCallId}};
    if (data._title.has_value())
        obj.insert("title", *data._title);
    else if (data._title.isNull())
        obj.insert("title", QJsonValue::Null);
    if (data._kind.has_value())
        obj.insert("kind", toJsonValue(*data._kind));
    else if (data._kind.isNull())
        obj.insert("kind", QJsonValue::Null);
    if (data._status.has_value())
        obj.insert("status", toJsonValue(*data._status));
    else if (data._status.isNull())
        obj.insert("status", QJsonValue::Null);
    if (data._content.has_value()) {
        QJsonArray arr_content;
        for (const auto &v : *data._content) arr_content.append(toJsonValue(v));
        obj.insert("content", arr_content);
    }
    else if (data._content.isNull())
        obj.insert("content", QJsonValue::Null);
    if (data._locations.has_value()) {
        QJsonArray arr_locations;
        for (const auto &v : *data._locations) arr_locations.append(toJson(v));
        obj.insert("locations", arr_locations);
    }
    else if (data._locations.isNull())
        obj.insert("locations", QJsonValue::Null);
    if (data._rawInput.has_value())
        obj.insert("rawInput", *data._rawInput);
    if (data._rawOutput.has_value())
        obj.insert("rawOutput", *data._rawOutput);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<ToolCallPermissionSubject> fromJson<ToolCallPermissionSubject>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for ToolCallPermissionSubject");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("toolCall"))
        co_return Utils::ResultError("Missing required field: toolCall");
    ToolCallPermissionSubject result;
    if (obj.contains("toolCall") && obj["toolCall"].isObject())
        result._toolCall = co_await fromJson<ToolCallUpdate>(obj["toolCall"]);
    co_return result;
}

QJsonObject toJson(const ToolCallPermissionSubject &data)
{
    QJsonObject obj{{"toolCall", toJson(data._toolCall)}};
    return obj;
}

template<>
Utils::Result<RequestPermissionSubject> fromJson<RequestPermissionSubject>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Invalid RequestPermissionSubject: expected object");
    const QString dispatchValue = val.toObject().value("type").toString();
    if (dispatchValue == "tool_call")
        co_return RequestPermissionSubject(co_await fromJson<ToolCallPermissionSubject>(val));
    else if (dispatchValue == "command")
        co_return RequestPermissionSubject(co_await fromJson<CommandPermissionSubject>(val));
    if (dispatchValue.isEmpty())
        co_return Utils::ResultError("Invalid RequestPermissionSubject: missing type");
    co_return RequestPermissionSubject(val.toObject());  // open union: preserve unknown variants raw
}

QString dispatchValue(const RequestPermissionSubject &val)
{
    return std::visit([](const auto &v) -> QString {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, ToolCallPermissionSubject>) return "tool_call";
        else if constexpr (std::is_same_v<T, CommandPermissionSubject>) return "command";
        else if constexpr (std::is_same_v<T, QJsonObject>) return v.value("type").toString();
        return {};
    }, val);
}

QJsonObject toJson(const RequestPermissionSubject &val)
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

QJsonValue toJsonValue(const RequestPermissionSubject &val)
{
    return toJson(val);
}

template<>
Utils::Result<RequestPermissionRequest> fromJson<RequestPermissionRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for RequestPermissionRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        co_return Utils::ResultError("Missing required field: sessionId");
    if (!obj.contains("title"))
        co_return Utils::ResultError("Missing required field: title");
    if (!obj.contains("options"))
        co_return Utils::ResultError("Missing required field: options");
    RequestPermissionRequest result;
    if (obj.contains("sessionId") && obj["sessionId"].isString())
        result._sessionId = co_await fromJson<SessionId>(obj["sessionId"]);
    result._title = obj.value("title").toString();
    if (obj.contains("description")) {
        if (!obj["description"].isNull()) {
            result._description = obj.value("description").toString();
        } else {
            result._description = std::nullopt;
        }
    }
    if (obj.contains("subject") && !obj["subject"].isNull())
        result._subject = co_await fromJson<RequestPermissionSubject>(obj["subject"]);
    else if (obj.contains("subject"))
        result._subject = std::nullopt;
    if (obj.contains("options") && obj["options"].isArray()) {
        const QJsonArray arr = obj["options"].toArray();
        for (const QJsonValue &v : arr) {
            result._options.append(co_await fromJson<PermissionOption>(v));
        }
    }
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const RequestPermissionRequest &data)
{
    QJsonObject obj{
        {"sessionId", data._sessionId},
        {"title", data._title}
    };
    if (data._description.has_value())
        obj.insert("description", *data._description);
    else if (data._description.isNull())
        obj.insert("description", QJsonValue::Null);
    if (data._subject.has_value())
        obj.insert("subject", toJsonValue(*data._subject));
    else if (data._subject.isNull())
        obj.insert("subject", QJsonValue::Null);
    QJsonArray arr_options;
    for (const auto &v : data._options) arr_options.append(toJson(v));
    obj.insert("options", arr_options);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
Utils::Result<CloseSessionResponse> fromJson<CloseSessionResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CloseSessionResponse");
    const QJsonObject obj = val.toObject();
    CloseSessionResponse result;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const CloseSessionResponse &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<DeleteSessionResponse> fromJson<DeleteSessionResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DeleteSessionResponse");
    const QJsonObject obj = val.toObject();
    DeleteSessionResponse result;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const DeleteSessionResponse &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
Utils::Result<AgentAuthCapabilities> fromJson<AgentAuthCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for AgentAuthCapabilities");
    const QJsonObject obj = val.toObject();
    AgentAuthCapabilities result;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const AgentAuthCapabilities &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<McpHttpCapabilities> fromJson<McpHttpCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for McpHttpCapabilities");
    const QJsonObject obj = val.toObject();
    McpHttpCapabilities result;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const McpHttpCapabilities &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<McpStdioCapabilities> fromJson<McpStdioCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for McpStdioCapabilities");
    const QJsonObject obj = val.toObject();
    McpStdioCapabilities result;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const McpStdioCapabilities &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<McpCapabilities> fromJson<McpCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for McpCapabilities");
    const QJsonObject obj = val.toObject();
    McpCapabilities result;
    if (obj.contains("stdio") && !obj["stdio"].isNull())
        result._stdio = co_await fromJson<McpStdioCapabilities>(obj["stdio"]);
    else if (obj.contains("stdio"))
        result._stdio = std::nullopt;
    if (obj.contains("http") && !obj["http"].isNull())
        result._http = co_await fromJson<McpHttpCapabilities>(obj["http"]);
    else if (obj.contains("http"))
        result._http = std::nullopt;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const McpCapabilities &data)
{
    QJsonObject obj;
    if (data._stdio.has_value())
        obj.insert("stdio", toJson(*data._stdio));
    else if (data._stdio.isNull())
        obj.insert("stdio", QJsonValue::Null);
    if (data._http.has_value())
        obj.insert("http", toJson(*data._http));
    else if (data._http.isNull())
        obj.insert("http", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<PromptAudioCapabilities> fromJson<PromptAudioCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for PromptAudioCapabilities");
    const QJsonObject obj = val.toObject();
    PromptAudioCapabilities result;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const PromptAudioCapabilities &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<PromptEmbeddedContextCapabilities> fromJson<PromptEmbeddedContextCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for PromptEmbeddedContextCapabilities");
    const QJsonObject obj = val.toObject();
    PromptEmbeddedContextCapabilities result;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const PromptEmbeddedContextCapabilities &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<PromptImageCapabilities> fromJson<PromptImageCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for PromptImageCapabilities");
    const QJsonObject obj = val.toObject();
    PromptImageCapabilities result;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const PromptImageCapabilities &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<PromptCapabilities> fromJson<PromptCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for PromptCapabilities");
    const QJsonObject obj = val.toObject();
    PromptCapabilities result;
    if (obj.contains("image") && !obj["image"].isNull())
        result._image = co_await fromJson<PromptImageCapabilities>(obj["image"]);
    else if (obj.contains("image"))
        result._image = std::nullopt;
    if (obj.contains("audio") && !obj["audio"].isNull())
        result._audio = co_await fromJson<PromptAudioCapabilities>(obj["audio"]);
    else if (obj.contains("audio"))
        result._audio = std::nullopt;
    if (obj.contains("embeddedContext") && !obj["embeddedContext"].isNull())
        result._embeddedContext = co_await fromJson<PromptEmbeddedContextCapabilities>(obj["embeddedContext"]);
    else if (obj.contains("embeddedContext"))
        result._embeddedContext = std::nullopt;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const PromptCapabilities &data)
{
    QJsonObject obj;
    if (data._image.has_value())
        obj.insert("image", toJson(*data._image));
    else if (data._image.isNull())
        obj.insert("image", QJsonValue::Null);
    if (data._audio.has_value())
        obj.insert("audio", toJson(*data._audio));
    else if (data._audio.isNull())
        obj.insert("audio", QJsonValue::Null);
    if (data._embeddedContext.has_value())
        obj.insert("embeddedContext", toJson(*data._embeddedContext));
    else if (data._embeddedContext.isNull())
        obj.insert("embeddedContext", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<SessionAdditionalDirectoriesCapabilities> fromJson<SessionAdditionalDirectoriesCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SessionAdditionalDirectoriesCapabilities");
    const QJsonObject obj = val.toObject();
    SessionAdditionalDirectoriesCapabilities result;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const SessionAdditionalDirectoriesCapabilities &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<SessionDeleteCapabilities> fromJson<SessionDeleteCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SessionDeleteCapabilities");
    const QJsonObject obj = val.toObject();
    SessionDeleteCapabilities result;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const SessionDeleteCapabilities &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<SessionCapabilities> fromJson<SessionCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for SessionCapabilities");
    const QJsonObject obj = val.toObject();
    SessionCapabilities result;
    if (obj.contains("prompt") && !obj["prompt"].isNull())
        result._prompt = co_await fromJson<PromptCapabilities>(obj["prompt"]);
    else if (obj.contains("prompt"))
        result._prompt = std::nullopt;
    if (obj.contains("mcp") && !obj["mcp"].isNull())
        result._mcp = co_await fromJson<McpCapabilities>(obj["mcp"]);
    else if (obj.contains("mcp"))
        result._mcp = std::nullopt;
    if (obj.contains("delete") && !obj["delete"].isNull())
        result._delete_ = co_await fromJson<SessionDeleteCapabilities>(obj["delete"]);
    else if (obj.contains("delete"))
        result._delete_ = std::nullopt;
    if (obj.contains("additionalDirectories") && !obj["additionalDirectories"].isNull())
        result._additionalDirectories = co_await fromJson<SessionAdditionalDirectoriesCapabilities>(obj["additionalDirectories"]);
    else if (obj.contains("additionalDirectories"))
        result._additionalDirectories = std::nullopt;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const SessionCapabilities &data)
{
    QJsonObject obj;
    if (data._prompt.has_value())
        obj.insert("prompt", toJson(*data._prompt));
    else if (data._prompt.isNull())
        obj.insert("prompt", QJsonValue::Null);
    if (data._mcp.has_value())
        obj.insert("mcp", toJson(*data._mcp));
    else if (data._mcp.isNull())
        obj.insert("mcp", QJsonValue::Null);
    if (data._delete_.has_value())
        obj.insert("delete", toJson(*data._delete_));
    else if (data._delete_.isNull())
        obj.insert("delete", QJsonValue::Null);
    if (data._additionalDirectories.has_value())
        obj.insert("additionalDirectories", toJson(*data._additionalDirectories));
    else if (data._additionalDirectories.isNull())
        obj.insert("additionalDirectories", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<AgentCapabilities> fromJson<AgentCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for AgentCapabilities");
    const QJsonObject obj = val.toObject();
    AgentCapabilities result;
    if (obj.contains("session") && !obj["session"].isNull())
        result._session = co_await fromJson<SessionCapabilities>(obj["session"]);
    else if (obj.contains("session"))
        result._session = std::nullopt;
    if (obj.contains("auth") && !obj["auth"].isNull())
        result._auth = co_await fromJson<AgentAuthCapabilities>(obj["auth"]);
    else if (obj.contains("auth"))
        result._auth = std::nullopt;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const AgentCapabilities &data)
{
    QJsonObject obj;
    if (data._session.has_value())
        obj.insert("session", toJson(*data._session));
    else if (data._session.isNull())
        obj.insert("session", QJsonValue::Null);
    if (data._auth.has_value())
        obj.insert("auth", toJson(*data._auth));
    else if (data._auth.isNull())
        obj.insert("auth", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<AuthMethodAgent> fromJson<AuthMethodAgent>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for AuthMethodAgent");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("methodId"))
        co_return Utils::ResultError("Missing required field: methodId");
    if (!obj.contains("name"))
        co_return Utils::ResultError("Missing required field: name");
    AuthMethodAgent result;
    if (obj.contains("methodId") && obj["methodId"].isString())
        result._methodId = co_await fromJson<AuthMethodId>(obj["methodId"]);
    result._name = obj.value("name").toString();
    if (obj.contains("description")) {
        if (!obj["description"].isNull()) {
            result._description = obj.value("description").toString();
        } else {
            result._description = std::nullopt;
        }
    }
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const AuthMethodAgent &data)
{
    QJsonObject obj{
        {"methodId", data._methodId},
        {"name", data._name}
    };
    if (data._description.has_value())
        obj.insert("description", *data._description);
    else if (data._description.isNull())
        obj.insert("description", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<AuthMethod> fromJson<AuthMethod>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Invalid AuthMethod: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("methodId"))
        co_return AuthMethod(co_await fromJson<AuthMethodAgent>(val));
    if (val.isObject())
        co_return AuthMethod(val.toObject());  // open union: preserve unknown variants raw
    co_return Utils::ResultError("Invalid AuthMethod");
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
    if (obj.contains("title")) {
        if (!obj["title"].isNull()) {
            result._title = obj.value("title").toString();
        } else {
            result._title = std::nullopt;
        }
    }
    result._version = obj.value("version").toString();
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
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
    else if (data._title.isNull())
        obj.insert("title", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
    if (!obj.contains("info"))
        co_return Utils::ResultError("Missing required field: info");
    InitializeResponse result;
    if (obj.contains("protocolVersion") && obj["protocolVersion"].isDouble())
        result._protocolVersion = co_await fromJson<ProtocolVersion>(obj["protocolVersion"]);
    if (obj.contains("info") && obj["info"].isObject())
        result._info = co_await fromJson<Implementation>(obj["info"]);
    if (obj.contains("capabilities") && obj["capabilities"].isObject())
        result._capabilities = co_await fromJson<AgentCapabilities>(obj["capabilities"]);
    if (obj.contains("authMethods") && obj["authMethods"].isArray()) {
        const QJsonArray arr = obj["authMethods"].toArray();
        QList<AuthMethod> list_authMethods;
        for (const QJsonValue &v : arr) {
            list_authMethods.append(co_await fromJson<AuthMethod>(v));
        }
        result._authMethods = list_authMethods;
    }
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const InitializeResponse &data)
{
    QJsonObject obj{
        {"protocolVersion", data._protocolVersion},
        {"info", toJson(data._info)}
    };
    if (data._capabilities.has_value())
        obj.insert("capabilities", toJson(*data._capabilities));
    if (data._authMethods.has_value()) {
        QJsonArray arr_authMethods;
        for (const auto &v : *data._authMethods) arr_authMethods.append(toJsonValue(v));
        obj.insert("authMethods", arr_authMethods);
    }
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
    if (obj.contains("cwd") && obj["cwd"].isString())
        result._cwd = co_await fromJson<AbsolutePath>(obj["cwd"]);
    if (obj.contains("additionalDirectories") && obj["additionalDirectories"].isArray()) {
        const QJsonArray arr = obj["additionalDirectories"].toArray();
        QList<AbsolutePath> list_additionalDirectories;
        for (const QJsonValue &v : arr) {
            list_additionalDirectories.append(co_await fromJson<AbsolutePath>(v));
        }
        result._additionalDirectories = list_additionalDirectories;
    }
    if (obj.contains("title")) {
        if (!obj["title"].isNull()) {
            result._title = obj.value("title").toString();
        } else {
            result._title = std::nullopt;
        }
    }
    if (obj.contains("updatedAt")) {
        if (!obj["updatedAt"].isNull()) {
            result._updatedAt = obj.value("updatedAt").toString();
        } else {
            result._updatedAt = std::nullopt;
        }
    }
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
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
    else if (data._title.isNull())
        obj.insert("title", QJsonValue::Null);
    if (data._updatedAt.has_value())
        obj.insert("updatedAt", *data._updatedAt);
    else if (data._updatedAt.isNull())
        obj.insert("updatedAt", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
    if (obj.contains("nextCursor") && !obj["nextCursor"].isNull())
        result._nextCursor = co_await fromJson<SessionListCursor>(obj["nextCursor"]);
    else if (obj.contains("nextCursor"))
        result._nextCursor = std::nullopt;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
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
    else if (data._nextCursor.isNull())
        obj.insert("nextCursor", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<LoginAuthResponse> fromJson<LoginAuthResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for LoginAuthResponse");
    const QJsonObject obj = val.toObject();
    LoginAuthResponse result;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const LoginAuthResponse &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<LogoutAuthResponse> fromJson<LogoutAuthResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for LogoutAuthResponse");
    const QJsonObject obj = val.toObject();
    LogoutAuthResponse result;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const LogoutAuthResponse &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
    if (obj.contains("description")) {
        if (!obj["description"].isNull()) {
            result._description = obj.value("description").toString();
        } else {
            result._description = std::nullopt;
        }
    }
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
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
    else if (data._description.isNull())
        obj.insert("description", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<SessionConfigSelectGroup> fromJson<SessionConfigSelectGroup>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for SessionConfigSelectGroup");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("groupId"))
        co_return Utils::ResultError("Missing required field: groupId");
    if (!obj.contains("name"))
        co_return Utils::ResultError("Missing required field: name");
    if (!obj.contains("options"))
        co_return Utils::ResultError("Missing required field: options");
    SessionConfigSelectGroup result;
    if (obj.contains("groupId") && obj["groupId"].isString())
        result._groupId = co_await fromJson<SessionConfigGroupId>(obj["groupId"]);
    result._name = obj.value("name").toString();
    if (obj.contains("options") && obj["options"].isArray()) {
        const QJsonArray arr = obj["options"].toArray();
        for (const QJsonValue &v : arr) {
            result._options.append(co_await fromJson<SessionConfigSelectOption>(v));
        }
    }
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const SessionConfigSelectGroup &data)
{
    QJsonObject obj{
        {"groupId", data._groupId},
        {"name", data._name}
    };
    QJsonArray arr_options;
    for (const auto &v : data._options) arr_options.append(toJson(v));
    obj.insert("options", arr_options);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
    if (!obj.contains("configId"))
        co_return Utils::ResultError("Missing required field: configId");
    if (!obj.contains("name"))
        co_return Utils::ResultError("Missing required field: name");
    SessionConfigOption result;
    if (obj.contains("configId") && obj["configId"].isString())
        result._configId = co_await fromJson<SessionConfigId>(obj["configId"]);
    result._name = obj.value("name").toString();
    if (obj.contains("description")) {
        if (!obj["description"].isNull()) {
            result._description = obj.value("description").toString();
        } else {
            result._description = std::nullopt;
        }
    }
    if (obj.contains("category") && !obj["category"].isNull())
        result._category = co_await fromJson<SessionConfigOptionCategory>(obj["category"]);
    else if (obj.contains("category"))
        result._category = std::nullopt;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    {
        const QSet<QString> knownKeys{"configId", "name", "description", "category", "_meta"};
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
        {"configId", data._configId},
        {"name", data._name}
    };
    if (data._description.has_value())
        obj.insert("description", *data._description);
    else if (data._description.isNull())
        obj.insert("description", QJsonValue::Null);
    if (data._category.has_value())
        obj.insert("category", toJsonValue(*data._category));
    else if (data._category.isNull())
        obj.insert("category", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    for (auto it = data._additionalProperties.constBegin(); it != data._additionalProperties.constEnd(); ++it)
        obj.insert(it.key(), it.value());
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
    if (obj.contains("configOptions") && obj["configOptions"].isArray()) {
        const QJsonArray arr = obj["configOptions"].toArray();
        QList<SessionConfigOption> list_configOptions;
        for (const QJsonValue &v : arr) {
            list_configOptions.append(co_await fromJson<SessionConfigOption>(v));
        }
        result._configOptions = list_configOptions;
    }
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const NewSessionResponse &data)
{
    QJsonObject obj{{"sessionId", data._sessionId}};
    if (data._configOptions.has_value()) {
        QJsonArray arr_configOptions;
        for (const auto &v : *data._configOptions) arr_configOptions.append(toJson(v));
        obj.insert("configOptions", arr_configOptions);
    }
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<PromptResponse> fromJson<PromptResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for PromptResponse");
    const QJsonObject obj = val.toObject();
    PromptResponse result;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const PromptResponse &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<ResumeSessionResponse> fromJson<ResumeSessionResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for ResumeSessionResponse");
    const QJsonObject obj = val.toObject();
    ResumeSessionResponse result;
    if (obj.contains("configOptions") && obj["configOptions"].isArray()) {
        const QJsonArray arr = obj["configOptions"].toArray();
        QList<SessionConfigOption> list_configOptions;
        for (const QJsonValue &v : arr) {
            list_configOptions.append(co_await fromJson<SessionConfigOption>(v));
        }
        result._configOptions = list_configOptions;
    }
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const ResumeSessionResponse &data)
{
    QJsonObject obj;
    if (data._configOptions.has_value()) {
        QJsonArray arr_configOptions;
        for (const auto &v : *data._configOptions) arr_configOptions.append(toJson(v));
        obj.insert("configOptions", arr_configOptions);
    }
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
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
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
        co_return Utils::ResultError("Expected JSON object for CompleteElicitationNotification");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("elicitationId"))
        co_return Utils::ResultError("Missing required field: elicitationId");
    CompleteElicitationNotification result;
    if (obj.contains("elicitationId") && obj["elicitationId"].isString())
        result._elicitationId = co_await fromJson<ElicitationId>(obj["elicitationId"]);
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const CompleteElicitationNotification &data)
{
    QJsonObject obj{{"elicitationId", data._elicitationId}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<AgentMessage> fromJson<AgentMessage>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for AgentMessage");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("messageId"))
        co_return Utils::ResultError("Missing required field: messageId");
    AgentMessage result;
    if (obj.contains("messageId") && obj["messageId"].isString())
        result._messageId = co_await fromJson<MessageId>(obj["messageId"]);
    if (obj.contains("content") && obj["content"].isArray()) {
        const QJsonArray arr = obj["content"].toArray();
        QList<ContentBlock> list_content;
        for (const QJsonValue &v : arr) {
            list_content.append(co_await fromJson<ContentBlock>(v));
        }
        result._content = list_content;
    }
    else if (obj.contains("content") && obj["content"].isNull())
        result._content = std::nullopt;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const AgentMessage &data)
{
    QJsonObject obj{{"messageId", data._messageId}};
    if (data._content.has_value()) {
        QJsonArray arr_content;
        for (const auto &v : *data._content) arr_content.append(toJsonValue(v));
        obj.insert("content", arr_content);
    }
    else if (data._content.isNull())
        obj.insert("content", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<AgentThought> fromJson<AgentThought>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for AgentThought");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("messageId"))
        co_return Utils::ResultError("Missing required field: messageId");
    AgentThought result;
    if (obj.contains("messageId") && obj["messageId"].isString())
        result._messageId = co_await fromJson<MessageId>(obj["messageId"]);
    if (obj.contains("content") && obj["content"].isArray()) {
        const QJsonArray arr = obj["content"].toArray();
        QList<ContentBlock> list_content;
        for (const QJsonValue &v : arr) {
            list_content.append(co_await fromJson<ContentBlock>(v));
        }
        result._content = list_content;
    }
    else if (obj.contains("content") && obj["content"].isNull())
        result._content = std::nullopt;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const AgentThought &data)
{
    QJsonObject obj{{"messageId", data._messageId}};
    if (data._content.has_value()) {
        QJsonArray arr_content;
        for (const auto &v : *data._content) arr_content.append(toJsonValue(v));
        obj.insert("content", arr_content);
    }
    else if (data._content.isNull())
        obj.insert("content", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<TextCommandInput> fromJson<TextCommandInput>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TextCommandInput");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("hint"))
        return Utils::ResultError("Missing required field: hint");
    TextCommandInput result;
    result._hint = obj.value("hint").toString();
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const TextCommandInput &data)
{
    QJsonObject obj{{"hint", data._hint}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<AvailableCommandInput> fromJson<AvailableCommandInput>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Invalid AvailableCommandInput: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("hint"))
        co_return AvailableCommandInput(co_await fromJson<TextCommandInput>(val));
    if (val.isObject())
        co_return AvailableCommandInput(val.toObject());  // open union: preserve unknown variants raw
    co_return Utils::ResultError("Invalid AvailableCommandInput");
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
    else if (obj.contains("input"))
        result._input = std::nullopt;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
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
    else if (data._input.isNull())
        obj.insert("input", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
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
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
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
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<ContentChunk> fromJson<ContentChunk>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for ContentChunk");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("messageId"))
        co_return Utils::ResultError("Missing required field: messageId");
    if (!obj.contains("content"))
        co_return Utils::ResultError("Missing required field: content");
    ContentChunk result;
    if (obj.contains("messageId") && obj["messageId"].isString())
        result._messageId = co_await fromJson<MessageId>(obj["messageId"]);
    if (obj.contains("content"))
        result._content = co_await fromJson<ContentBlock>(obj["content"]);
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const ContentChunk &data)
{
    QJsonObject obj{
        {"messageId", data._messageId},
        {"content", toJsonValue(data._content)}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
        case PlanEntryStatus::cancelled: return "cancelled";
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
    if (str == "cancelled") return PlanEntryStatus::cancelled;
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
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
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
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<PlanItems> fromJson<PlanItems>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for PlanItems");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("planId"))
        co_return Utils::ResultError("Missing required field: planId");
    if (!obj.contains("entries"))
        co_return Utils::ResultError("Missing required field: entries");
    PlanItems result;
    if (obj.contains("planId") && obj["planId"].isString())
        result._planId = co_await fromJson<PlanId>(obj["planId"]);
    if (obj.contains("entries") && obj["entries"].isArray()) {
        const QJsonArray arr = obj["entries"].toArray();
        for (const QJsonValue &v : arr) {
            result._entries.append(co_await fromJson<PlanEntry>(v));
        }
    }
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const PlanItems &data)
{
    QJsonObject obj{{"planId", data._planId}};
    QJsonArray arr_entries;
    for (const auto &v : data._entries) arr_entries.append(toJson(v));
    obj.insert("entries", arr_entries);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<PlanUpdateContent> fromJson<PlanUpdateContent>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Invalid PlanUpdateContent: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("entries"))
        co_return PlanUpdateContent(co_await fromJson<PlanItems>(val));
    if (val.isObject())
        co_return PlanUpdateContent(val.toObject());  // open union: preserve unknown variants raw
    co_return Utils::ResultError("Invalid PlanUpdateContent");
}

QJsonObject toJson(const PlanUpdateContent &val)
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

QJsonValue toJsonValue(const PlanUpdateContent &val)
{
    return toJson(val);
}

template<>
Utils::Result<PlanUpdate> fromJson<PlanUpdate>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for PlanUpdate");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("plan"))
        co_return Utils::ResultError("Missing required field: plan");
    PlanUpdate result;
    if (obj.contains("plan"))
        result._plan = co_await fromJson<PlanUpdateContent>(obj["plan"]);
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const PlanUpdate &data)
{
    QJsonObject obj{{"plan", toJsonValue(data._plan)}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<SessionInfoUpdate> fromJson<SessionInfoUpdate>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SessionInfoUpdate");
    const QJsonObject obj = val.toObject();
    SessionInfoUpdate result;
    if (obj.contains("title")) {
        if (!obj["title"].isNull()) {
            result._title = obj.value("title").toString();
        } else {
            result._title = std::nullopt;
        }
    }
    if (obj.contains("updatedAt")) {
        if (!obj["updatedAt"].isNull()) {
            result._updatedAt = obj.value("updatedAt").toString();
        } else {
            result._updatedAt = std::nullopt;
        }
    }
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const SessionInfoUpdate &data)
{
    QJsonObject obj;
    if (data._title.has_value())
        obj.insert("title", *data._title);
    else if (data._title.isNull())
        obj.insert("title", QJsonValue::Null);
    if (data._updatedAt.has_value())
        obj.insert("updatedAt", *data._updatedAt);
    else if (data._updatedAt.isNull())
        obj.insert("updatedAt", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
Utils::Result<IdleStateUpdate> fromJson<IdleStateUpdate>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for IdleStateUpdate");
    const QJsonObject obj = val.toObject();
    IdleStateUpdate result;
    if (obj.contains("stopReason") && !obj["stopReason"].isNull())
        result._stopReason = co_await fromJson<StopReason>(obj["stopReason"]);
    else if (obj.contains("stopReason"))
        result._stopReason = std::nullopt;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const IdleStateUpdate &data)
{
    QJsonObject obj;
    if (data._stopReason.has_value())
        obj.insert("stopReason", toJsonValue(*data._stopReason));
    else if (data._stopReason.isNull())
        obj.insert("stopReason", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<RequiresActionStateUpdate> fromJson<RequiresActionStateUpdate>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for RequiresActionStateUpdate");
    const QJsonObject obj = val.toObject();
    RequiresActionStateUpdate result;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const RequiresActionStateUpdate &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<RunningStateUpdate> fromJson<RunningStateUpdate>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for RunningStateUpdate");
    const QJsonObject obj = val.toObject();
    RunningStateUpdate result;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const RunningStateUpdate &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<StateUpdate> fromJson<StateUpdate>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Invalid StateUpdate: expected object");
    const QString dispatchValue = val.toObject().value("state").toString();
    if (dispatchValue == "running")
        co_return StateUpdate(co_await fromJson<RunningStateUpdate>(val));
    else if (dispatchValue == "idle")
        co_return StateUpdate(co_await fromJson<IdleStateUpdate>(val));
    else if (dispatchValue == "requires_action")
        co_return StateUpdate(co_await fromJson<RequiresActionStateUpdate>(val));
    if (dispatchValue.isEmpty())
        co_return Utils::ResultError("Invalid StateUpdate: missing state");
    co_return StateUpdate(val.toObject());  // open union: preserve unknown variants raw
}

QString dispatchValue(const StateUpdate &val)
{
    return std::visit([](const auto &v) -> QString {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, RunningStateUpdate>) return "running";
        else if constexpr (std::is_same_v<T, IdleStateUpdate>) return "idle";
        else if constexpr (std::is_same_v<T, RequiresActionStateUpdate>) return "requires_action";
        else if constexpr (std::is_same_v<T, QJsonObject>) return v.value("state").toString();
        return {};
    }, val);
}

QJsonObject toJson(const StateUpdate &val)
{
    QJsonObject obj = std::visit([](const auto &v) -> QJsonObject {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
    obj.insert("state", dispatchValue(val));
    return obj;
}

QJsonValue toJsonValue(const StateUpdate &val)
{
    return toJson(val);
}

template<>
Utils::Result<TerminalOutputChunk> fromJson<TerminalOutputChunk>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for TerminalOutputChunk");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("terminalId"))
        co_return Utils::ResultError("Missing required field: terminalId");
    if (!obj.contains("data"))
        co_return Utils::ResultError("Missing required field: data");
    TerminalOutputChunk result;
    if (obj.contains("terminalId") && obj["terminalId"].isString())
        result._terminalId = co_await fromJson<TerminalId>(obj["terminalId"]);
    result._data = obj.value("data").toString();
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const TerminalOutputChunk &data)
{
    QJsonObject obj{
        {"terminalId", data._terminalId},
        {"data", data._data}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<TerminalExitStatus> fromJson<TerminalExitStatus>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TerminalExitStatus");
    const QJsonObject obj = val.toObject();
    TerminalExitStatus result;
    if (obj.contains("exitCode")) {
        if (!obj["exitCode"].isNull()) {
            result._exitCode = obj.value("exitCode").toInt();
        } else {
            result._exitCode = std::nullopt;
        }
    }
    if (obj.contains("signal")) {
        if (!obj["signal"].isNull()) {
            result._signal = obj.value("signal").toString();
        } else {
            result._signal = std::nullopt;
        }
    }
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const TerminalExitStatus &data)
{
    QJsonObject obj;
    if (data._exitCode.has_value())
        obj.insert("exitCode", *data._exitCode);
    else if (data._exitCode.isNull())
        obj.insert("exitCode", QJsonValue::Null);
    if (data._signal.has_value())
        obj.insert("signal", *data._signal);
    else if (data._signal.isNull())
        obj.insert("signal", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<TerminalOutput> fromJson<TerminalOutput>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TerminalOutput");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("data"))
        return Utils::ResultError("Missing required field: data");
    TerminalOutput result;
    result._data = obj.value("data").toString();
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const TerminalOutput &data)
{
    QJsonObject obj{{"data", data._data}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<TerminalUpdate> fromJson<TerminalUpdate>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for TerminalUpdate");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("terminalId"))
        co_return Utils::ResultError("Missing required field: terminalId");
    TerminalUpdate result;
    if (obj.contains("terminalId") && obj["terminalId"].isString())
        result._terminalId = co_await fromJson<TerminalId>(obj["terminalId"]);
    if (obj.contains("command")) {
        if (!obj["command"].isNull()) {
            result._command = obj.value("command").toString();
        } else {
            result._command = std::nullopt;
        }
    }
    if (obj.contains("cwd") && !obj["cwd"].isNull())
        result._cwd = co_await fromJson<AbsolutePath>(obj["cwd"]);
    else if (obj.contains("cwd"))
        result._cwd = std::nullopt;
    if (obj.contains("output") && !obj["output"].isNull())
        result._output = co_await fromJson<TerminalOutput>(obj["output"]);
    else if (obj.contains("output"))
        result._output = std::nullopt;
    if (obj.contains("exitStatus") && !obj["exitStatus"].isNull())
        result._exitStatus = co_await fromJson<TerminalExitStatus>(obj["exitStatus"]);
    else if (obj.contains("exitStatus"))
        result._exitStatus = std::nullopt;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const TerminalUpdate &data)
{
    QJsonObject obj{{"terminalId", data._terminalId}};
    if (data._command.has_value())
        obj.insert("command", *data._command);
    else if (data._command.isNull())
        obj.insert("command", QJsonValue::Null);
    if (data._cwd.has_value())
        obj.insert("cwd", *data._cwd);
    else if (data._cwd.isNull())
        obj.insert("cwd", QJsonValue::Null);
    if (data._output.has_value())
        obj.insert("output", toJson(*data._output));
    else if (data._output.isNull())
        obj.insert("output", QJsonValue::Null);
    if (data._exitStatus.has_value())
        obj.insert("exitStatus", toJson(*data._exitStatus));
    else if (data._exitStatus.isNull())
        obj.insert("exitStatus", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<ToolCallContentChunk> fromJson<ToolCallContentChunk>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for ToolCallContentChunk");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("toolCallId"))
        co_return Utils::ResultError("Missing required field: toolCallId");
    if (!obj.contains("content"))
        co_return Utils::ResultError("Missing required field: content");
    ToolCallContentChunk result;
    if (obj.contains("toolCallId") && obj["toolCallId"].isString())
        result._toolCallId = co_await fromJson<ToolCallId>(obj["toolCallId"]);
    if (obj.contains("content"))
        result._content = co_await fromJson<ToolCallContent>(obj["content"]);
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const ToolCallContentChunk &data)
{
    QJsonObject obj{
        {"toolCallId", data._toolCallId},
        {"content", toJsonValue(data._content)}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
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
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
    else if (obj.contains("cost"))
        result._cost = std::nullopt;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
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
    else if (data._cost.isNull())
        obj.insert("cost", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<UserMessage> fromJson<UserMessage>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for UserMessage");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("messageId"))
        co_return Utils::ResultError("Missing required field: messageId");
    UserMessage result;
    if (obj.contains("messageId") && obj["messageId"].isString())
        result._messageId = co_await fromJson<MessageId>(obj["messageId"]);
    if (obj.contains("content") && obj["content"].isArray()) {
        const QJsonArray arr = obj["content"].toArray();
        QList<ContentBlock> list_content;
        for (const QJsonValue &v : arr) {
            list_content.append(co_await fromJson<ContentBlock>(v));
        }
        result._content = list_content;
    }
    else if (obj.contains("content") && obj["content"].isNull())
        result._content = std::nullopt;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const UserMessage &data)
{
    QJsonObject obj{{"messageId", data._messageId}};
    if (data._content.has_value()) {
        QJsonArray arr_content;
        for (const auto &v : *data._content) arr_content.append(toJsonValue(v));
        obj.insert("content", arr_content);
    }
    else if (data._content.isNull())
        obj.insert("content", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
    else if (kind == "user_message")
        result._value = co_await fromJson<UserMessage>(val);
    else if (kind == "agent_message_chunk")
        result._value = co_await fromJson<ContentChunk>(val);
    else if (kind == "agent_message")
        result._value = co_await fromJson<AgentMessage>(val);
    else if (kind == "agent_thought_chunk")
        result._value = co_await fromJson<ContentChunk>(val);
    else if (kind == "agent_thought")
        result._value = co_await fromJson<AgentThought>(val);
    else if (kind == "state_update")
        result._value = co_await fromJson<StateUpdate>(val);
    else if (kind == "tool_call_content_chunk")
        result._value = co_await fromJson<ToolCallContentChunk>(val);
    else if (kind == "tool_call_update")
        result._value = co_await fromJson<ToolCallUpdate>(val);
    else if (kind == "terminal_update")
        result._value = co_await fromJson<TerminalUpdate>(val);
    else if (kind == "terminal_output_chunk")
        result._value = co_await fromJson<TerminalOutputChunk>(val);
    else if (kind == "plan_update")
        result._value = co_await fromJson<PlanUpdate>(val);
    else if (kind == "available_commands_update")
        result._value = co_await fromJson<AvailableCommandsUpdate>(val);
    else if (kind == "config_option_update")
        result._value = co_await fromJson<ConfigOptionUpdate>(val);
    else if (kind == "session_info_update")
        result._value = co_await fromJson<SessionInfoUpdate>(val);
    else if (kind == "usage_update")
        result._value = co_await fromJson<UsageUpdate>(val);
    else if (kind.isEmpty())
        co_return Utils::ResultError("Invalid SessionUpdate: missing sessionUpdate");
    else
        result._value = obj;  // open union: preserve unknown variants raw
    co_return result;
}

QJsonObject toJson(const SessionUpdate &data)
{
    QJsonObject obj = std::visit([](const auto &v) -> QJsonObject {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) return v;
        else return toJson(v);
    }, data._value);
    obj.insert("sessionUpdate", data._kind);
    return obj;
}

QJsonValue toJsonValue(const SessionUpdate &val)
{
    return toJson(val);
}

template<>
Utils::Result<UpdateSessionNotification> fromJson<UpdateSessionNotification>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for UpdateSessionNotification");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        co_return Utils::ResultError("Missing required field: sessionId");
    if (!obj.contains("update"))
        co_return Utils::ResultError("Missing required field: update");
    UpdateSessionNotification result;
    if (obj.contains("sessionId") && obj["sessionId"].isString())
        result._sessionId = co_await fromJson<SessionId>(obj["sessionId"]);
    if (obj.contains("update"))
        result._update = co_await fromJson<SessionUpdate>(obj["update"]);
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const UpdateSessionNotification &data)
{
    QJsonObject obj{
        {"sessionId", data._sessionId},
        {"update", toJsonValue(data._update)}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const CloseSessionRequest &data)
{
    QJsonObject obj{{"sessionId", data._sessionId}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const DeleteSessionRequest &data)
{
    QJsonObject obj{{"sessionId", data._sessionId}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<ElicitationFormCapabilities> fromJson<ElicitationFormCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ElicitationFormCapabilities");
    const QJsonObject obj = val.toObject();
    ElicitationFormCapabilities result;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const ElicitationFormCapabilities &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<ElicitationUrlCapabilities> fromJson<ElicitationUrlCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ElicitationUrlCapabilities");
    const QJsonObject obj = val.toObject();
    ElicitationUrlCapabilities result;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const ElicitationUrlCapabilities &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<ElicitationCapabilities> fromJson<ElicitationCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for ElicitationCapabilities");
    const QJsonObject obj = val.toObject();
    ElicitationCapabilities result;
    if (obj.contains("form") && !obj["form"].isNull())
        result._form = co_await fromJson<ElicitationFormCapabilities>(obj["form"]);
    else if (obj.contains("form"))
        result._form = std::nullopt;
    if (obj.contains("url") && !obj["url"].isNull())
        result._url = co_await fromJson<ElicitationUrlCapabilities>(obj["url"]);
    else if (obj.contains("url"))
        result._url = std::nullopt;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const ElicitationCapabilities &data)
{
    QJsonObject obj;
    if (data._form.has_value())
        obj.insert("form", toJson(*data._form));
    else if (data._form.isNull())
        obj.insert("form", QJsonValue::Null);
    if (data._url.has_value())
        obj.insert("url", toJson(*data._url));
    else if (data._url.isNull())
        obj.insert("url", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<ClientCapabilities> fromJson<ClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for ClientCapabilities");
    const QJsonObject obj = val.toObject();
    ClientCapabilities result;
    if (obj.contains("elicitation") && !obj["elicitation"].isNull())
        result._elicitation = co_await fromJson<ElicitationCapabilities>(obj["elicitation"]);
    else if (obj.contains("elicitation"))
        result._elicitation = std::nullopt;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const ClientCapabilities &data)
{
    QJsonObject obj;
    if (data._elicitation.has_value())
        obj.insert("elicitation", toJson(*data._elicitation));
    else if (data._elicitation.isNull())
        obj.insert("elicitation", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
    if (!obj.contains("info"))
        co_return Utils::ResultError("Missing required field: info");
    InitializeRequest result;
    if (obj.contains("protocolVersion") && obj["protocolVersion"].isDouble())
        result._protocolVersion = co_await fromJson<ProtocolVersion>(obj["protocolVersion"]);
    if (obj.contains("info") && obj["info"].isObject())
        result._info = co_await fromJson<Implementation>(obj["info"]);
    if (obj.contains("capabilities") && obj["capabilities"].isObject())
        result._capabilities = co_await fromJson<ClientCapabilities>(obj["capabilities"]);
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const InitializeRequest &data)
{
    QJsonObject obj{
        {"protocolVersion", data._protocolVersion},
        {"info", toJson(data._info)}
    };
    if (data._capabilities.has_value())
        obj.insert("capabilities", toJson(*data._capabilities));
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<ListSessionsRequest> fromJson<ListSessionsRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for ListSessionsRequest");
    const QJsonObject obj = val.toObject();
    ListSessionsRequest result;
    if (obj.contains("cwd") && !obj["cwd"].isNull())
        result._cwd = co_await fromJson<AbsolutePath>(obj["cwd"]);
    else if (obj.contains("cwd"))
        result._cwd = std::nullopt;
    if (obj.contains("cursor") && !obj["cursor"].isNull())
        result._cursor = co_await fromJson<SessionListCursor>(obj["cursor"]);
    else if (obj.contains("cursor"))
        result._cursor = std::nullopt;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const ListSessionsRequest &data)
{
    QJsonObject obj;
    if (data._cwd.has_value())
        obj.insert("cwd", *data._cwd);
    else if (data._cwd.isNull())
        obj.insert("cwd", QJsonValue::Null);
    if (data._cursor.has_value())
        obj.insert("cursor", *data._cursor);
    else if (data._cursor.isNull())
        obj.insert("cursor", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<LoginAuthRequest> fromJson<LoginAuthRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for LoginAuthRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("methodId"))
        co_return Utils::ResultError("Missing required field: methodId");
    LoginAuthRequest result;
    if (obj.contains("methodId") && obj["methodId"].isString())
        result._methodId = co_await fromJson<AuthMethodId>(obj["methodId"]);
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const LoginAuthRequest &data)
{
    QJsonObject obj{{"methodId", data._methodId}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<LogoutAuthRequest> fromJson<LogoutAuthRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for LogoutAuthRequest");
    const QJsonObject obj = val.toObject();
    LogoutAuthRequest result;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const LogoutAuthRequest &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
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
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
    McpServerHttp result;
    result._name = obj.value("name").toString();
    result._url = obj.value("url").toString();
    if (obj.contains("headers") && obj["headers"].isArray()) {
        const QJsonArray arr = obj["headers"].toArray();
        QList<HttpHeader> list_headers;
        for (const QJsonValue &v : arr) {
            list_headers.append(co_await fromJson<HttpHeader>(v));
        }
        result._headers = list_headers;
    }
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const McpServerHttp &data)
{
    QJsonObject obj{
        {"name", data._name},
        {"url", data._url}
    };
    if (data._headers.has_value()) {
        QJsonArray arr_headers;
        for (const auto &v : *data._headers) arr_headers.append(toJson(v));
        obj.insert("headers", arr_headers);
    }
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
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
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
    McpServerStdio result;
    result._name = obj.value("name").toString();
    if (obj.contains("command") && obj["command"].isString())
        result._command = co_await fromJson<AbsolutePath>(obj["command"]);
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
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const McpServerStdio &data)
{
    QJsonObject obj{
        {"name", data._name},
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
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
    else if (dispatchValue == "stdio")
        co_return McpServer(co_await fromJson<McpServerStdio>(val));
    if (dispatchValue.isEmpty())
        co_return Utils::ResultError("Invalid McpServer: missing type");
    co_return McpServer(val.toObject());  // open union: preserve unknown variants raw
}

QString dispatchValue(const McpServer &val)
{
    return std::visit([](const auto &v) -> QString {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, McpServerHttp>) return "http";
        else if constexpr (std::is_same_v<T, McpServerStdio>) return "stdio";
        else if constexpr (std::is_same_v<T, QJsonObject>) return v.value("type").toString();
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

template<>
Utils::Result<NewSessionRequest> fromJson<NewSessionRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for NewSessionRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("cwd"))
        co_return Utils::ResultError("Missing required field: cwd");
    NewSessionRequest result;
    if (obj.contains("cwd") && obj["cwd"].isString())
        result._cwd = co_await fromJson<AbsolutePath>(obj["cwd"]);
    if (obj.contains("additionalDirectories") && obj["additionalDirectories"].isArray()) {
        const QJsonArray arr = obj["additionalDirectories"].toArray();
        QList<AbsolutePath> list_additionalDirectories;
        for (const QJsonValue &v : arr) {
            list_additionalDirectories.append(co_await fromJson<AbsolutePath>(v));
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
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
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
    if (data._mcpServers.has_value()) {
        QJsonArray arr_mcpServers;
        for (const auto &v : *data._mcpServers) arr_mcpServers.append(toJsonValue(v));
        obj.insert("mcpServers", arr_mcpServers);
    }
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
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
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<ReplayFromStart> fromJson<ReplayFromStart>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ReplayFromStart");
    const QJsonObject obj = val.toObject();
    ReplayFromStart result;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const ReplayFromStart &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<ReplayFrom> fromJson<ReplayFrom>(const QJsonValue &val)
{
    if (val.isObject()) {
        auto result = fromJson<ReplayFromStart>(val);
        if (result) return ReplayFrom(*result);
    }
    if (val.isObject())
        return ReplayFrom(val.toObject());  // open union: preserve unknown variants raw
    return Utils::ResultError("Invalid ReplayFrom");
}

QJsonObject toJson(const ReplayFrom &val)
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

QJsonValue toJsonValue(const ReplayFrom &val)
{
    return toJson(val);
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
    if (obj.contains("cwd") && obj["cwd"].isString())
        result._cwd = co_await fromJson<AbsolutePath>(obj["cwd"]);
    if (obj.contains("additionalDirectories") && obj["additionalDirectories"].isArray()) {
        const QJsonArray arr = obj["additionalDirectories"].toArray();
        QList<AbsolutePath> list_additionalDirectories;
        for (const QJsonValue &v : arr) {
            list_additionalDirectories.append(co_await fromJson<AbsolutePath>(v));
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
    if (obj.contains("replayFrom") && !obj["replayFrom"].isNull())
        result._replayFrom = co_await fromJson<ReplayFrom>(obj["replayFrom"]);
    else if (obj.contains("replayFrom"))
        result._replayFrom = std::nullopt;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
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
    if (data._replayFrom.has_value())
        obj.insert("replayFrom", toJsonValue(*data._replayFrom));
    else if (data._replayFrom.isNull())
        obj.insert("replayFrom", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
    SetSessionConfigOptionRequest result;
    if (obj.contains("sessionId") && obj["sessionId"].isString())
        result._sessionId = co_await fromJson<SessionId>(obj["sessionId"]);
    if (obj.contains("configId") && obj["configId"].isString())
        result._configId = co_await fromJson<SessionConfigId>(obj["configId"]);
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    {
        const QSet<QString> knownKeys{"sessionId", "configId", "_meta"};
        for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
            if (!knownKeys.contains(it.key()))
                result._additionalProperties.insert(it.key(), it.value());
        }
    }
    co_return result;
}

QJsonObject toJson(const SetSessionConfigOptionRequest &data)
{
    QJsonObject obj{
        {"sessionId", data._sessionId},
        {"configId", data._configId}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    for (auto it = data._additionalProperties.constBegin(); it != data._additionalProperties.constEnd(); ++it)
        obj.insert(it.key(), it.value());
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
Utils::Result<ElicitationContentValue> fromJson<ElicitationContentValue>(const QJsonValue &val)
{
    if (val.isString())
        return ElicitationContentValue(val.toString());
    if (val.isDouble())
        return ElicitationContentValue(static_cast<int>(val.toDouble()));
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
    if (obj.contains("content")) {
        if (!obj["content"].isNull()) {
            result._content = obj.value("content").toObject();
        } else {
            result._content = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const ElicitationAcceptAction &data)
{
    QJsonObject obj;
    if (data._content.has_value())
        obj.insert("content", *data._content);
    else if (data._content.isNull())
        obj.insert("content", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<CreateElicitationResponse> fromJson<CreateElicitationResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CreateElicitationResponse");
    const QJsonObject obj = val.toObject();
    CreateElicitationResponse result;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
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
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    for (auto it = data._additionalProperties.constBegin(); it != data._additionalProperties.constEnd(); ++it)
        obj.insert(it.key(), it.value());
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
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const SelectedPermissionOutcome &data)
{
    QJsonObject obj{{"optionId", data._optionId}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
    else if (kind.isEmpty())
        co_return Utils::ResultError("Invalid RequestPermissionOutcome: missing outcome");
    else
        result._value = obj;  // open union: preserve unknown variants raw
    co_return result;
}

QJsonObject toJson(const RequestPermissionOutcome &data)
{
    QJsonObject obj = std::visit([](const auto &v) -> QJsonObject {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) return {};
        else if constexpr (std::is_same_v<T, QJsonObject>) return v;
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
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const RequestPermissionResponse &data)
{
    QJsonObject obj{{"outcome", toJsonValue(data._outcome)}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<CancelSessionNotification> fromJson<CancelSessionNotification>(const QJsonValue &val)
{
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for CancelSessionNotification");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        co_return Utils::ResultError("Missing required field: sessionId");
    CancelSessionNotification result;
    if (obj.contains("sessionId") && obj["sessionId"].isString())
        result._sessionId = co_await fromJson<SessionId>(obj["sessionId"]);
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const CancelSessionNotification &data)
{
    QJsonObject obj{{"sessionId", data._sessionId}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
        co_return Utils::ResultError("Expected JSON object for CancelRequestNotification");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("requestId"))
        co_return Utils::ResultError("Missing required field: requestId");
    CancelRequestNotification result;
    if (obj.contains("requestId"))
        result._requestId = co_await fromJson<RequestId>(obj["requestId"]);
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    co_return result;
}

QJsonObject toJson(const CancelRequestNotification &data)
{
    QJsonObject obj{{"requestId", toJsonValue(data._requestId)}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<ProtocolLevelNotification> fromJson<ProtocolLevelNotification>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ProtocolLevelNotification");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    ProtocolLevelNotification result;
    result._method = obj.value("method").toString();
    if (obj.contains("params"))
        result._params = obj.value("params").toString();
    return result;
}

QJsonObject toJson(const ProtocolLevelNotification &data)
{
    QJsonObject obj{{"method", data._method}};
    if (data._params.has_value())
        obj.insert("params", *data._params);
    return obj;
}

} // namespace Acp::V2
