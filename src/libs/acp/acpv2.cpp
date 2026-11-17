// This file is auto-generated. Do not edit manually.
#include "acpv2.h"

namespace Acp::V2 {

template<>
Utils::Result<RequestId> fromJson<RequestId>(const QJsonValue &val)
{
    if (val.isDouble())
        return RequestId(static_cast<int>(val.toDouble()));
    if (val.isString())
        return RequestId(val.toString());
    return Utils::ResultError("Invalid RequestId");
}

QJsonValue toJsonValue(const RequestId &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
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
    const auto res0 = fromJson<RequestId>("requestId", obj["requestId"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._requestId = *res0;
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
    if (!obj["enum"].isArray())
        return Utils::ResultError("Invalid type for field: enum");
    StringMultiSelectItems result;
    {
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
    if (!obj["const"].isString())
        return Utils::ResultError("Invalid type for field: const");
    if (!obj.contains("title"))
        return Utils::ResultError("Missing required field: title");
    if (!obj["title"].isString())
        return Utils::ResultError("Invalid type for field: title");
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
        return Utils::ResultError("Expected JSON object for TitledMultiSelectItems");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("anyOf"))
        return Utils::ResultError("Missing required field: anyOf");
    if (!obj["anyOf"].isArray())
        return Utils::ResultError("Invalid type for field: anyOf");
    TitledMultiSelectItems result;
    {
        const QJsonArray arr = obj["anyOf"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<EnumOption>("anyOf", v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._anyOf.append(*res0);
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
        return Utils::ResultError("Invalid MultiSelectItems: expected object");
    const QString dispatchValue = val.toObject().value("type").toString();
    if (dispatchValue == "string") {
        const auto res0 = fromJson<StringMultiSelectItems>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return MultiSelectItems(*res0);
    }
    if (dispatchValue.isEmpty()) {
        const auto res1 = fromJson<TitledMultiSelectItems>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        return MultiSelectItems(*res1);
    }
    return MultiSelectItems(val.toObject());  // open union: preserve unknown variants raw
}

QString dispatchValue(const MultiSelectItems &val)
{
    return std::visit([](const auto &v) -> QString {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, StringMultiSelectItems>) return "string";
        else if constexpr (std::is_same_v<T, QJsonObject>) return v.value("type").toString();
        return {};
    }, val);
}

QJsonObject toJson(const MultiSelectItems &val)
{
    QJsonObject obj = std::visit([](const auto &v) -> QJsonObject {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
    if (const QString disc = dispatchValue(val); !disc.isEmpty())
        obj.insert("type", disc);
    return obj;
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
    const auto res0 = fromJson<MultiSelectItems>("items", obj["items"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._items = *res0;
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
    return result;
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
    if (!val.isString())
        return Utils::ResultError("Expected JSON string for StringFormat");
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
    if (obj.contains("format") && !obj["format"].isNull()) {
        const auto res0 = fromJson<StringFormat>("format", obj["format"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._format = *res0;
    }
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
            const auto res1 = fromJson<EnumOption>("oneOf", v);
            if (!res1)
                return Utils::ResultError(res1.error());
            list_oneOf.append(*res1);
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
    return result;
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
    if (!val.isString())
        return Utils::ResultError("Expected JSON string for ElicitationSchemaType");
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
    if (!obj.contains("type"))
        return Utils::ResultError("Missing required field: type");
    if (!obj.contains("properties"))
        return Utils::ResultError("Missing required field: properties");
    if (!obj["properties"].isObject())
        return Utils::ResultError("Invalid type for field: properties");
    ElicitationSchema result;
    const auto res0 = fromJson<ElicitationSchemaType>("type", obj["type"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._type = *res0;
    if (obj.contains("title")) {
        if (!obj["title"].isNull()) {
            result._title = obj.value("title").toString();
        } else {
            result._title = std::nullopt;
        }
    }
    {
        const QJsonObject mapObj_properties = obj["properties"].toObject();
        QMap<QString, ElicitationPropertySchema> map_properties;
        for (auto it = mapObj_properties.constBegin(); it != mapObj_properties.constEnd(); ++it) {
            const auto res1 = fromJson<ElicitationPropertySchema>("properties", it.value());
            if (!res1)
                return Utils::ResultError(res1.error());
            map_properties.insert(it.key(), *res1);
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
    return result;
}

QJsonObject toJson(const ElicitationSchema &data)
{
    QJsonObject obj{{"type", toJsonValue(data._type)}};
    if (data._title.has_value())
        obj.insert("title", *data._title);
    else if (data._title.isNull())
        obj.insert("title", QJsonValue::Null);
    QJsonObject map_properties;
    for (auto it = data._properties.constBegin(); it != data._properties.constEnd(); ++it)
        map_properties.insert(it.key(), toJsonValue(it.value()));
    obj.insert("properties", map_properties);
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
        return Utils::ResultError("Expected JSON object for ElicitationSessionScope");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    if (!obj["sessionId"].isString())
        return Utils::ResultError("Invalid type for field: sessionId");
    ElicitationSessionScope result;
    const auto res0 = fromJson<SessionId>("sessionId", obj["sessionId"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._sessionId = *res0;
    // "toolCallId": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("toolCallId") && !obj["toolCallId"].isNull()) {
            const auto toolCallIdRes0 = fromJson<ToolCallId>("toolCallId", obj["toolCallId"]);
            if (!toolCallIdRes0)
                return Utils::ResultError(toolCallIdRes0.error());
            result._toolCallId = *toolCallIdRes0;
        }
        else if (obj.contains("toolCallId"))
            result._toolCallId = std::nullopt;
        return {};
    }();
    return result;
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
        return Utils::ResultError("Expected JSON object for ElicitationFormMode");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("requestedSchema"))
        return Utils::ResultError("Missing required field: requestedSchema");
    if (!obj["requestedSchema"].isObject())
        return Utils::ResultError("Invalid type for field: requestedSchema");
    ElicitationFormMode result;
    const auto res0 = fromJson<ElicitationSchema>("requestedSchema", obj["requestedSchema"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._requestedSchema = *res0;
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
    if (!obj["elicitationId"].isString())
        return Utils::ResultError("Invalid type for field: elicitationId");
    if (!obj.contains("url"))
        return Utils::ResultError("Missing required field: url");
    if (!obj["url"].isString())
        return Utils::ResultError("Invalid type for field: url");
    ElicitationUrlMode result;
    const auto res0 = fromJson<ElicitationId>("elicitationId", obj["elicitationId"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._elicitationId = *res0;
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
    if (!obj["message"].isString())
        return Utils::ResultError("Invalid type for field: message");
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
    if (!val.isString())
        return Utils::ResultError("Expected JSON string for PermissionOptionKind");
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
    if (!obj["optionId"].isString())
        return Utils::ResultError("Invalid type for field: optionId");
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    if (!obj["name"].isString())
        return Utils::ResultError("Invalid type for field: name");
    if (!obj.contains("kind"))
        return Utils::ResultError("Missing required field: kind");
    PermissionOption result;
    const auto res0 = fromJson<PermissionOptionId>("optionId", obj["optionId"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._optionId = *res0;
    result._name = obj.value("name").toString();
    const auto res1 = fromJson<PermissionOptionKind>("kind", obj["kind"]);
    if (!res1)
        return Utils::ResultError(res1.error());
    result._kind = *res1;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
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
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<CommandPermissionSubject> fromJson<CommandPermissionSubject>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CommandPermissionSubject");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("command"))
        return Utils::ResultError("Missing required field: command");
    if (!obj["command"].isString())
        return Utils::ResultError("Invalid type for field: command");
    if (!obj.contains("cwd"))
        return Utils::ResultError("Missing required field: cwd");
    if (!obj["cwd"].isString())
        return Utils::ResultError("Invalid type for field: cwd");
    CommandPermissionSubject result;
    result._command = obj.value("command").toString();
    const auto res0 = fromJson<AbsolutePath>("cwd", obj["cwd"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._cwd = *res0;
    // "toolCallId": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("toolCallId") && !obj["toolCallId"].isNull()) {
            const auto toolCallIdRes0 = fromJson<ToolCallId>("toolCallId", obj["toolCallId"]);
            if (!toolCallIdRes0)
                return Utils::ResultError(toolCallIdRes0.error());
            result._toolCallId = *toolCallIdRes0;
        }
        else if (obj.contains("toolCallId"))
            result._toolCallId = std::nullopt;
        return {};
    }();
    // "terminalId": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("terminalId") && !obj["terminalId"].isNull()) {
            const auto terminalIdRes0 = fromJson<TerminalId>("terminalId", obj["terminalId"]);
            if (!terminalIdRes0)
                return Utils::ResultError(terminalIdRes0.error());
            result._terminalId = *terminalIdRes0;
        }
        else if (obj.contains("terminalId"))
            result._terminalId = std::nullopt;
        return {};
    }();
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
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

template<>
Utils::Result<ToolCallCancelCapabilities> fromJson<ToolCallCancelCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ToolCallCancelCapabilities");
    const QJsonObject obj = val.toObject();
    ToolCallCancelCapabilities result;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const ToolCallCancelCapabilities &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<ToolCallCapabilities> fromJson<ToolCallCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ToolCallCapabilities");
    const QJsonObject obj = val.toObject();
    ToolCallCapabilities result;
    // "cancel": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("cancel") && !obj["cancel"].isNull()) {
            const auto cancelRes0 = fromJson<ToolCallCancelCapabilities>("cancel", obj["cancel"]);
            if (!cancelRes0)
                return Utils::ResultError(cancelRes0.error());
            result._cancel = *cancelRes0;
        }
        else if (obj.contains("cancel"))
            result._cancel = std::nullopt;
        return {};
    }();
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const ToolCallCapabilities &data)
{
    QJsonObject obj;
    if (data._cancel.has_value())
        obj.insert("cancel", toJson(*data._cancel));
    else if (data._cancel.isNull())
        obj.insert("cancel", QJsonValue::Null);
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
            const auto audienceItem = fromJson<Role>("audience", v);
            if (!audienceItem)
                continue;
            list_audience.append(*audienceItem);
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
        return Utils::ResultError("Expected JSON object for AudioContent");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("data"))
        return Utils::ResultError("Missing required field: data");
    if (!obj["data"].isString())
        return Utils::ResultError("Invalid type for field: data");
    if (!obj.contains("mimeType"))
        return Utils::ResultError("Missing required field: mimeType");
    if (!obj["mimeType"].isString())
        return Utils::ResultError("Invalid type for field: mimeType");
    AudioContent result;
    result._data = obj.value("data").toString();
    const auto res0 = fromJson<MediaType>("mimeType", obj["mimeType"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._mimeType = *res0;
    // "annotations": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("annotations") && !obj["annotations"].isNull()) {
            const auto annotationsRes0 = fromJson<Annotations>("annotations", obj["annotations"]);
            if (!annotationsRes0)
                return Utils::ResultError(annotationsRes0.error());
            result._annotations = *annotationsRes0;
        }
        else if (obj.contains("annotations"))
            result._annotations = std::nullopt;
        return {};
    }();
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
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
        return Utils::ResultError("Expected JSON object for BlobResourceContents");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("blob"))
        return Utils::ResultError("Missing required field: blob");
    if (!obj["blob"].isString())
        return Utils::ResultError("Invalid type for field: blob");
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    if (!obj["uri"].isString())
        return Utils::ResultError("Invalid type for field: uri");
    BlobResourceContents result;
    result._blob = obj.value("blob").toString();
    result._uri = obj.value("uri").toString();
    // "mimeType": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("mimeType") && !obj["mimeType"].isNull()) {
            const auto mimeTypeRes0 = fromJson<MediaType>("mimeType", obj["mimeType"]);
            if (!mimeTypeRes0)
                return Utils::ResultError(mimeTypeRes0.error());
            result._mimeType = *mimeTypeRes0;
        }
        else if (obj.contains("mimeType"))
            result._mimeType = std::nullopt;
        return {};
    }();
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
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
        return Utils::ResultError("Expected JSON object for TextResourceContents");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("text"))
        return Utils::ResultError("Missing required field: text");
    if (!obj["text"].isString())
        return Utils::ResultError("Invalid type for field: text");
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    if (!obj["uri"].isString())
        return Utils::ResultError("Invalid type for field: uri");
    TextResourceContents result;
    result._text = obj.value("text").toString();
    result._uri = obj.value("uri").toString();
    // "mimeType": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("mimeType") && !obj["mimeType"].isNull()) {
            const auto mimeTypeRes0 = fromJson<MediaType>("mimeType", obj["mimeType"]);
            if (!mimeTypeRes0)
                return Utils::ResultError(mimeTypeRes0.error());
            result._mimeType = *mimeTypeRes0;
        }
        else if (obj.contains("mimeType"))
            result._mimeType = std::nullopt;
        return {};
    }();
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
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
    const auto res0 = fromJson<EmbeddedResourceResource>("resource", obj["resource"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._resource = *res0;
    // "annotations": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("annotations") && !obj["annotations"].isNull()) {
            const auto annotationsRes0 = fromJson<Annotations>("annotations", obj["annotations"]);
            if (!annotationsRes0)
                return Utils::ResultError(annotationsRes0.error());
            result._annotations = *annotationsRes0;
        }
        else if (obj.contains("annotations"))
            result._annotations = std::nullopt;
        return {};
    }();
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
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
        return Utils::ResultError("Expected JSON object for ImageContent");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("data"))
        return Utils::ResultError("Missing required field: data");
    if (!obj["data"].isString())
        return Utils::ResultError("Invalid type for field: data");
    if (!obj.contains("mimeType"))
        return Utils::ResultError("Missing required field: mimeType");
    if (!obj["mimeType"].isString())
        return Utils::ResultError("Invalid type for field: mimeType");
    ImageContent result;
    result._data = obj.value("data").toString();
    const auto res0 = fromJson<MediaType>("mimeType", obj["mimeType"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._mimeType = *res0;
    if (obj.contains("uri")) {
        if (!obj["uri"].isNull()) {
            result._uri = obj.value("uri").toString();
        } else {
            result._uri = std::nullopt;
        }
    }
    // "annotations": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("annotations") && !obj["annotations"].isNull()) {
            const auto annotationsRes0 = fromJson<Annotations>("annotations", obj["annotations"]);
            if (!annotationsRes0)
                return Utils::ResultError(annotationsRes0.error());
            result._annotations = *annotationsRes0;
        }
        else if (obj.contains("annotations"))
            result._annotations = std::nullopt;
        return {};
    }();
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
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
    if (!val.isString())
        return Utils::ResultError("Expected JSON string for IconTheme");
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
        return Utils::ResultError("Expected JSON object for Icon");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("src"))
        return Utils::ResultError("Missing required field: src");
    if (!obj["src"].isString())
        return Utils::ResultError("Invalid type for field: src");
    Icon result;
    result._src = obj.value("src").toString();
    // "mimeType": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("mimeType") && !obj["mimeType"].isNull()) {
            const auto mimeTypeRes0 = fromJson<MediaType>("mimeType", obj["mimeType"]);
            if (!mimeTypeRes0)
                return Utils::ResultError(mimeTypeRes0.error());
            result._mimeType = *mimeTypeRes0;
        }
        else if (obj.contains("mimeType"))
            result._mimeType = std::nullopt;
        return {};
    }();
    if (obj.contains("sizes")) {
        if (!obj["sizes"].isNull()) {
            result._sizes = obj.value("sizes").toArray();
        } else {
            result._sizes = std::nullopt;
        }
    }
    // "theme": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("theme") && !obj["theme"].isNull()) {
            const auto themeRes0 = fromJson<IconTheme>("theme", obj["theme"]);
            if (!themeRes0)
                return Utils::ResultError(themeRes0.error());
            result._theme = *themeRes0;
        }
        else if (obj.contains("theme"))
            result._theme = std::nullopt;
        return {};
    }();
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
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
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
    if (!obj["name"].isString())
        return Utils::ResultError("Invalid type for field: name");
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    if (!obj["uri"].isString())
        return Utils::ResultError("Invalid type for field: uri");
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
            const auto iconsItem = fromJson<Icon>("icons", v);
            if (!iconsItem)
                continue;
            list_icons.append(*iconsItem);
        }
        result._icons = list_icons;
    }
    else if (obj.contains("icons") && obj["icons"].isNull())
        result._icons = std::nullopt;
    // "mimeType": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("mimeType") && !obj["mimeType"].isNull()) {
            const auto mimeTypeRes0 = fromJson<MediaType>("mimeType", obj["mimeType"]);
            if (!mimeTypeRes0)
                return Utils::ResultError(mimeTypeRes0.error());
            result._mimeType = *mimeTypeRes0;
        }
        else if (obj.contains("mimeType"))
            result._mimeType = std::nullopt;
        return {};
    }();
    if (obj.contains("size")) {
        if (!obj["size"].isNull()) {
            result._size = obj.value("size").toInt();
        } else {
            result._size = std::nullopt;
        }
    }
    // "annotations": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("annotations") && !obj["annotations"].isNull()) {
            const auto annotationsRes0 = fromJson<Annotations>("annotations", obj["annotations"]);
            if (!annotationsRes0)
                return Utils::ResultError(annotationsRes0.error());
            result._annotations = *annotationsRes0;
        }
        else if (obj.contains("annotations"))
            result._annotations = std::nullopt;
        return {};
    }();
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
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
        return Utils::ResultError("Expected JSON object for TextContent");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("text"))
        return Utils::ResultError("Missing required field: text");
    if (!obj["text"].isString())
        return Utils::ResultError("Invalid type for field: text");
    TextContent result;
    result._text = obj.value("text").toString();
    // "annotations": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("annotations") && !obj["annotations"].isNull()) {
            const auto annotationsRes0 = fromJson<Annotations>("annotations", obj["annotations"]);
            if (!annotationsRes0)
                return Utils::ResultError(annotationsRes0.error());
            result._annotations = *annotationsRes0;
        }
        else if (obj.contains("annotations"))
            result._annotations = std::nullopt;
        return {};
    }();
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
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
    if (dispatchValue.isEmpty())
        return Utils::ResultError("Invalid ContentBlock: missing type");
    return ContentBlock(val.toObject());  // open union: preserve unknown variants raw
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
        return Utils::ResultError("Expected JSON object for Content");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("content"))
        return Utils::ResultError("Missing required field: content");
    Content result;
    const auto res0 = fromJson<ContentBlock>("content", obj["content"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._content = *res0;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
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
    if (!val.isString())
        return Utils::ResultError("Expected JSON string for DiffFileType");
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
        return Utils::ResultError("Expected JSON object for DiffPathChange");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("path"))
        return Utils::ResultError("Missing required field: path");
    if (!obj["path"].isString())
        return Utils::ResultError("Invalid type for field: path");
    DiffPathChange result;
    const auto res0 = fromJson<AbsolutePath>("path", obj["path"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._path = *res0;
    return result;
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
        return Utils::ResultError("Expected JSON object for DiffPathPairChange");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("oldPath"))
        return Utils::ResultError("Missing required field: oldPath");
    if (!obj["oldPath"].isString())
        return Utils::ResultError("Invalid type for field: oldPath");
    if (!obj.contains("path"))
        return Utils::ResultError("Missing required field: path");
    if (!obj["path"].isString())
        return Utils::ResultError("Invalid type for field: path");
    DiffPathPairChange result;
    const auto res0 = fromJson<AbsolutePath>("oldPath", obj["oldPath"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._oldPath = *res0;
    const auto res1 = fromJson<AbsolutePath>("path", obj["path"]);
    if (!res1)
        return Utils::ResultError(res1.error());
    result._path = *res1;
    return result;
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
        return Utils::ResultError("Expected JSON object for DiffChange");
    const QJsonObject obj = val.toObject();
    DiffChange result;
    // "fileType": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("fileType") && !obj["fileType"].isNull()) {
            const auto fileTypeRes0 = fromJson<DiffFileType>("fileType", obj["fileType"]);
            if (!fileTypeRes0)
                return Utils::ResultError(fileTypeRes0.error());
            result._fileType = *fileTypeRes0;
        }
        else if (obj.contains("fileType"))
            result._fileType = std::nullopt;
        return {};
    }();
    // "mimeType": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("mimeType") && !obj["mimeType"].isNull()) {
            const auto mimeTypeRes0 = fromJson<MediaType>("mimeType", obj["mimeType"]);
            if (!mimeTypeRes0)
                return Utils::ResultError(mimeTypeRes0.error());
            result._mimeType = *mimeTypeRes0;
        }
        else if (obj.contains("mimeType"))
            result._mimeType = std::nullopt;
        return {};
    }();
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
    return result;
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
    if (!val.isString())
        return Utils::ResultError("Expected JSON string for DiffPatchFormat");
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
        return Utils::ResultError("Expected JSON object for DiffPatch");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("format"))
        return Utils::ResultError("Missing required field: format");
    if (!obj.contains("text"))
        return Utils::ResultError("Missing required field: text");
    if (!obj["text"].isString())
        return Utils::ResultError("Invalid type for field: text");
    DiffPatch result;
    const auto res0 = fromJson<DiffPatchFormat>("format", obj["format"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._format = *res0;
    result._text = obj.value("text").toString();
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const DiffPatch &data)
{
    QJsonObject obj{
        {"format", toJsonValue(data._format)},
        {"text", data._text}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<Diff> fromJson<Diff>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Diff");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("changes"))
        return Utils::ResultError("Missing required field: changes");
    if (!obj["changes"].isArray())
        return Utils::ResultError("Invalid type for field: changes");
    Diff result;
    {
        const QJsonArray arr = obj["changes"].toArray();
        for (const QJsonValue &v : arr) {
            const auto changesItem = fromJson<DiffChange>("changes", v);
            if (!changesItem)
                continue;
            result._changes.append(*changesItem);
        }
    }
    // "patch": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("patch") && !obj["patch"].isNull()) {
            const auto patchRes0 = fromJson<DiffPatch>("patch", obj["patch"]);
            if (!patchRes0)
                return Utils::ResultError(patchRes0.error());
            result._patch = *patchRes0;
        }
        else if (obj.contains("patch"))
            result._patch = std::nullopt;
        return {};
    }();
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
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
        return Utils::ResultError("Expected JSON object for Terminal");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("terminalId"))
        return Utils::ResultError("Missing required field: terminalId");
    if (!obj["terminalId"].isString())
        return Utils::ResultError("Invalid type for field: terminalId");
    Terminal result;
    const auto res0 = fromJson<TerminalId>("terminalId", obj["terminalId"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._terminalId = *res0;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
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
    if (dispatchValue.isEmpty())
        return Utils::ResultError("Invalid ToolCallContent: missing type");
    return ToolCallContent(val.toObject());  // open union: preserve unknown variants raw
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
        return Utils::ResultError("Expected JSON object for ToolCallLocation");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("path"))
        return Utils::ResultError("Missing required field: path");
    if (!obj["path"].isString())
        return Utils::ResultError("Invalid type for field: path");
    ToolCallLocation result;
    const auto res0 = fromJson<AbsolutePath>("path", obj["path"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._path = *res0;
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
    return result;
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
        case ToolCallStatus::paused: return "paused";
        case ToolCallStatus::completed: return "completed";
        case ToolCallStatus::failed: return "failed";
        case ToolCallStatus::cancelled: return "cancelled";
    }
    return {};
}

template<>
Utils::Result<ToolCallStatus> fromJson<ToolCallStatus>(const QJsonValue &val)
{
    if (!val.isString())
        return Utils::ResultError("Expected JSON string for ToolCallStatus");
    const QString str = val.toString();
    if (str == "pending") return ToolCallStatus::pending;
    if (str == "in_progress") return ToolCallStatus::in_progress;
    if (str == "paused") return ToolCallStatus::paused;
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
    if (!val.isString())
        return Utils::ResultError("Expected JSON string for ToolKind");
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
    if (!obj["toolCallId"].isString())
        return Utils::ResultError("Invalid type for field: toolCallId");
    ToolCallUpdate result;
    const auto res0 = fromJson<ToolCallId>("toolCallId", obj["toolCallId"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._toolCallId = *res0;
    if (obj.contains("name")) {
        if (!obj["name"].isNull()) {
            result._name = obj.value("name").toString();
        } else {
            result._name = std::nullopt;
        }
    }
    if (obj.contains("title")) {
        if (!obj["title"].isNull()) {
            result._title = obj.value("title").toString();
        } else {
            result._title = std::nullopt;
        }
    }
    // "kind": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("kind") && !obj["kind"].isNull()) {
            const auto kindRes0 = fromJson<ToolKind>("kind", obj["kind"]);
            if (!kindRes0)
                return Utils::ResultError(kindRes0.error());
            result._kind = *kindRes0;
        }
        else if (obj.contains("kind"))
            result._kind = std::nullopt;
        return {};
    }();
    // "status": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("status") && !obj["status"].isNull()) {
            const auto statusRes0 = fromJson<ToolCallStatus>("status", obj["status"]);
            if (!statusRes0)
                return Utils::ResultError(statusRes0.error());
            result._status = *statusRes0;
        }
        else if (obj.contains("status"))
            result._status = std::nullopt;
        return {};
    }();
    if (obj.contains("content") && obj["content"].isArray()) {
        const QJsonArray arr = obj["content"].toArray();
        QList<ToolCallContent> list_content;
        for (const QJsonValue &v : arr) {
            const auto contentItem = fromJson<ToolCallContent>("content", v);
            if (!contentItem)
                continue;
            list_content.append(*contentItem);
        }
        result._content = list_content;
    }
    else if (obj.contains("content") && obj["content"].isNull())
        result._content = std::nullopt;
    if (obj.contains("locations") && obj["locations"].isArray()) {
        const QJsonArray arr = obj["locations"].toArray();
        QList<ToolCallLocation> list_locations;
        for (const QJsonValue &v : arr) {
            const auto locationsItem = fromJson<ToolCallLocation>("locations", v);
            if (!locationsItem)
                continue;
            list_locations.append(*locationsItem);
        }
        result._locations = list_locations;
    }
    else if (obj.contains("locations") && obj["locations"].isNull())
        result._locations = std::nullopt;
    if (obj.contains("rawInput"))
        result._rawInput = obj.value("rawInput");
    if (obj.contains("rawOutput"))
        result._rawOutput = obj.value("rawOutput");
    // "capabilities": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("capabilities") && !obj["capabilities"].isNull()) {
            const auto capabilitiesRes0 = fromJson<ToolCallCapabilities>("capabilities", obj["capabilities"]);
            if (!capabilitiesRes0)
                return Utils::ResultError(capabilitiesRes0.error());
            result._capabilities = *capabilitiesRes0;
        }
        else if (obj.contains("capabilities"))
            result._capabilities = std::nullopt;
        return {};
    }();
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const ToolCallUpdate &data)
{
    QJsonObject obj{{"toolCallId", data._toolCallId}};
    if (data._name.has_value())
        obj.insert("name", *data._name);
    else if (data._name.isNull())
        obj.insert("name", QJsonValue::Null);
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
    if (data._capabilities.has_value())
        obj.insert("capabilities", toJson(*data._capabilities));
    else if (data._capabilities.isNull())
        obj.insert("capabilities", QJsonValue::Null);
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
        return Utils::ResultError("Expected JSON object for ToolCallPermissionSubject");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("toolCall"))
        return Utils::ResultError("Missing required field: toolCall");
    if (!obj["toolCall"].isObject())
        return Utils::ResultError("Invalid type for field: toolCall");
    ToolCallPermissionSubject result;
    const auto res0 = fromJson<ToolCallUpdate>("toolCall", obj["toolCall"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._toolCall = *res0;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const ToolCallPermissionSubject &data)
{
    QJsonObject obj{{"toolCall", toJson(data._toolCall)}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<RequestPermissionSubject> fromJson<RequestPermissionSubject>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid RequestPermissionSubject: expected object");
    const QString dispatchValue = val.toObject().value("type").toString();
    if (dispatchValue == "tool_call") {
        const auto res0 = fromJson<ToolCallPermissionSubject>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return RequestPermissionSubject(*res0);
    }
    else if (dispatchValue == "command") {
        const auto res1 = fromJson<CommandPermissionSubject>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        return RequestPermissionSubject(*res1);
    }
    if (dispatchValue.isEmpty())
        return Utils::ResultError("Invalid RequestPermissionSubject: missing type");
    return RequestPermissionSubject(val.toObject());  // open union: preserve unknown variants raw
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
        return Utils::ResultError("Expected JSON object for RequestPermissionRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    if (!obj["sessionId"].isString())
        return Utils::ResultError("Invalid type for field: sessionId");
    if (!obj.contains("title"))
        return Utils::ResultError("Missing required field: title");
    if (!obj["title"].isString())
        return Utils::ResultError("Invalid type for field: title");
    if (!obj.contains("options"))
        return Utils::ResultError("Missing required field: options");
    if (!obj["options"].isArray())
        return Utils::ResultError("Invalid type for field: options");
    RequestPermissionRequest result;
    const auto res0 = fromJson<SessionId>("sessionId", obj["sessionId"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._sessionId = *res0;
    result._title = obj.value("title").toString();
    if (obj.contains("description")) {
        if (!obj["description"].isNull()) {
            result._description = obj.value("description").toString();
        } else {
            result._description = std::nullopt;
        }
    }
    if (obj.contains("subject") && !obj["subject"].isNull()) {
        const auto res1 = fromJson<RequestPermissionSubject>("subject", obj["subject"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._subject = *res1;
    }
    else if (obj.contains("subject"))
        result._subject = std::nullopt;
    {
        const QJsonArray arr = obj["options"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res2 = fromJson<PermissionOption>("options", v);
            if (!res2)
                return Utils::ResultError(res2.error());
            result._options.append(*res2);
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
        return Utils::ResultError("Expected JSON object for AgentRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    if (!obj["method"].isString())
        return Utils::ResultError("Invalid type for field: method");
    AgentRequest result;
    const auto res0 = fromJson<RequestId>("id", obj["id"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._id = *res0;
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
Utils::Result<CancelTerminalResponse> fromJson<CancelTerminalResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CancelTerminalResponse");
    const QJsonObject obj = val.toObject();
    CancelTerminalResponse result;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const CancelTerminalResponse &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<CancelToolCallResponse> fromJson<CancelToolCallResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CancelToolCallResponse");
    const QJsonObject obj = val.toObject();
    CancelToolCallResponse result;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const CancelToolCallResponse &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
Utils::Result<SessionNotFoundErrorData> fromJson<SessionNotFoundErrorData>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SessionNotFoundErrorData");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    if (!obj["sessionId"].isString())
        return Utils::ResultError("Invalid type for field: sessionId");
    SessionNotFoundErrorData result;
    const auto res0 = fromJson<SessionId>("sessionId", obj["sessionId"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._sessionId = *res0;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const SessionNotFoundErrorData &data)
{
    QJsonObject obj{{"sessionId", data._sessionId}};
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
Utils::Result<UnsupportedProtocolVersionErrorData> fromJson<UnsupportedProtocolVersionErrorData>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for UnsupportedProtocolVersionErrorData");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("requested"))
        return Utils::ResultError("Missing required field: requested");
    if (!obj["requested"].isDouble())
        return Utils::ResultError("Invalid type for field: requested");
    if (!obj.contains("supported"))
        return Utils::ResultError("Missing required field: supported");
    if (!obj["supported"].isArray())
        return Utils::ResultError("Invalid type for field: supported");
    UnsupportedProtocolVersionErrorData result;
    const auto res0 = fromJson<ProtocolVersion>("requested", obj["requested"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._requested = *res0;
    {
        const QJsonArray arr = obj["supported"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res1 = fromJson<ProtocolVersion>("supported", v);
            if (!res1)
                return Utils::ResultError(res1.error());
            result._supported.append(*res1);
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

QJsonObject toJson(const UnsupportedProtocolVersionErrorData &data)
{
    QJsonObject obj{{"requested", data._requested}};
    QJsonArray arr_supported;
    for (const auto &v : data._supported) arr_supported.append(v);
    obj.insert("supported", arr_supported);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<ErrorData> fromJson<ErrorData>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid ErrorData: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("requested")) {
        const auto res0 = fromJson<UnsupportedProtocolVersionErrorData>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return ErrorData(*res0);
    }
    if (obj.contains("sessionId")) {
        const auto res1 = fromJson<SessionNotFoundErrorData>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        return ErrorData(*res1);
    }
    return Utils::ResultError("Invalid ErrorData");
}

QJsonObject toJson(const ErrorData &val)
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

QJsonValue toJsonValue(const ErrorData &val)
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
    if (!obj["code"].isDouble())
        return Utils::ResultError("Invalid type for field: code");
    if (!obj.contains("message"))
        return Utils::ResultError("Missing required field: message");
    if (!obj["message"].isString())
        return Utils::ResultError("Invalid type for field: message");
    Error result;
    result._code = obj.value("code").toInt();
    result._message = obj.value("message").toString();
    // "data": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("data") && !obj["data"].isNull()) {
            const auto dataRes0 = fromJson<ErrorData>("data", obj["data"]);
            if (!dataRes0)
                return Utils::ResultError(dataRes0.error());
            result._data = *dataRes0;
        }
        else if (obj.contains("data"))
            result._data = std::nullopt;
        return {};
    }();
    return result;
}

QJsonObject toJson(const Error &data)
{
    QJsonObject obj{
        {"code", data._code},
        {"message", data._message}
    };
    if (data._data.has_value())
        obj.insert("data", toJsonValue(*data._data));
    else if (data._data.isNull())
        obj.insert("data", QJsonValue::Null);
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
        return Utils::ResultError("Expected JSON object for McpCapabilities");
    const QJsonObject obj = val.toObject();
    McpCapabilities result;
    // "stdio": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("stdio") && !obj["stdio"].isNull()) {
            const auto stdioRes0 = fromJson<McpStdioCapabilities>("stdio", obj["stdio"]);
            if (!stdioRes0)
                return Utils::ResultError(stdioRes0.error());
            result._stdio = *stdioRes0;
        }
        else if (obj.contains("stdio"))
            result._stdio = std::nullopt;
        return {};
    }();
    // "http": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("http") && !obj["http"].isNull()) {
            const auto httpRes0 = fromJson<McpHttpCapabilities>("http", obj["http"]);
            if (!httpRes0)
                return Utils::ResultError(httpRes0.error());
            result._http = *httpRes0;
        }
        else if (obj.contains("http"))
            result._http = std::nullopt;
        return {};
    }();
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
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
        return Utils::ResultError("Expected JSON object for PromptCapabilities");
    const QJsonObject obj = val.toObject();
    PromptCapabilities result;
    // "image": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("image") && !obj["image"].isNull()) {
            const auto imageRes0 = fromJson<PromptImageCapabilities>("image", obj["image"]);
            if (!imageRes0)
                return Utils::ResultError(imageRes0.error());
            result._image = *imageRes0;
        }
        else if (obj.contains("image"))
            result._image = std::nullopt;
        return {};
    }();
    // "audio": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("audio") && !obj["audio"].isNull()) {
            const auto audioRes0 = fromJson<PromptAudioCapabilities>("audio", obj["audio"]);
            if (!audioRes0)
                return Utils::ResultError(audioRes0.error());
            result._audio = *audioRes0;
        }
        else if (obj.contains("audio"))
            result._audio = std::nullopt;
        return {};
    }();
    // "embeddedContext": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("embeddedContext") && !obj["embeddedContext"].isNull()) {
            const auto embeddedContextRes0 = fromJson<PromptEmbeddedContextCapabilities>("embeddedContext", obj["embeddedContext"]);
            if (!embeddedContextRes0)
                return Utils::ResultError(embeddedContextRes0.error());
            result._embeddedContext = *embeddedContextRes0;
        }
        else if (obj.contains("embeddedContext"))
            result._embeddedContext = std::nullopt;
        return {};
    }();
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
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
Utils::Result<SessionSystemPromptOverrideCapabilities> fromJson<SessionSystemPromptOverrideCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SessionSystemPromptOverrideCapabilities");
    const QJsonObject obj = val.toObject();
    SessionSystemPromptOverrideCapabilities result;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const SessionSystemPromptOverrideCapabilities &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<SessionSystemPromptModeCapabilities> fromJson<SessionSystemPromptModeCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SessionSystemPromptModeCapabilities");
    const QJsonObject obj = val.toObject();
    SessionSystemPromptModeCapabilities result;
    // "override": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("override") && !obj["override"].isNull()) {
            const auto override_Res0 = fromJson<SessionSystemPromptOverrideCapabilities>("override", obj["override"]);
            if (!override_Res0)
                return Utils::ResultError(override_Res0.error());
            result._override_ = *override_Res0;
        }
        else if (obj.contains("override"))
            result._override_ = std::nullopt;
        return {};
    }();
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const SessionSystemPromptModeCapabilities &data)
{
    QJsonObject obj;
    if (data._override_.has_value())
        obj.insert("override", toJson(*data._override_));
    else if (data._override_.isNull())
        obj.insert("override", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<SessionSystemPromptCapabilities> fromJson<SessionSystemPromptCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SessionSystemPromptCapabilities");
    const QJsonObject obj = val.toObject();
    SessionSystemPromptCapabilities result;
    // "mode": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("mode") && !obj["mode"].isNull()) {
            const auto modeRes0 = fromJson<SessionSystemPromptModeCapabilities>("mode", obj["mode"]);
            if (!modeRes0)
                return Utils::ResultError(modeRes0.error());
            result._mode = *modeRes0;
        }
        else if (obj.contains("mode"))
            result._mode = std::nullopt;
        return {};
    }();
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const SessionSystemPromptCapabilities &data)
{
    QJsonObject obj;
    if (data._mode.has_value())
        obj.insert("mode", toJson(*data._mode));
    else if (data._mode.isNull())
        obj.insert("mode", QJsonValue::Null);
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
        return Utils::ResultError("Expected JSON object for SessionCapabilities");
    const QJsonObject obj = val.toObject();
    SessionCapabilities result;
    // "prompt": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("prompt") && !obj["prompt"].isNull()) {
            const auto promptRes0 = fromJson<PromptCapabilities>("prompt", obj["prompt"]);
            if (!promptRes0)
                return Utils::ResultError(promptRes0.error());
            result._prompt = *promptRes0;
        }
        else if (obj.contains("prompt"))
            result._prompt = std::nullopt;
        return {};
    }();
    // "mcp": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("mcp") && !obj["mcp"].isNull()) {
            const auto mcpRes0 = fromJson<McpCapabilities>("mcp", obj["mcp"]);
            if (!mcpRes0)
                return Utils::ResultError(mcpRes0.error());
            result._mcp = *mcpRes0;
        }
        else if (obj.contains("mcp"))
            result._mcp = std::nullopt;
        return {};
    }();
    // "delete": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("delete") && !obj["delete"].isNull()) {
            const auto delete_Res0 = fromJson<SessionDeleteCapabilities>("delete", obj["delete"]);
            if (!delete_Res0)
                return Utils::ResultError(delete_Res0.error());
            result._delete_ = *delete_Res0;
        }
        else if (obj.contains("delete"))
            result._delete_ = std::nullopt;
        return {};
    }();
    // "additionalDirectories": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("additionalDirectories") && !obj["additionalDirectories"].isNull()) {
            const auto additionalDirectoriesRes0 = fromJson<SessionAdditionalDirectoriesCapabilities>("additionalDirectories", obj["additionalDirectories"]);
            if (!additionalDirectoriesRes0)
                return Utils::ResultError(additionalDirectoriesRes0.error());
            result._additionalDirectories = *additionalDirectoriesRes0;
        }
        else if (obj.contains("additionalDirectories"))
            result._additionalDirectories = std::nullopt;
        return {};
    }();
    // "systemPrompt": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("systemPrompt") && !obj["systemPrompt"].isNull()) {
            const auto systemPromptRes0 = fromJson<SessionSystemPromptCapabilities>("systemPrompt", obj["systemPrompt"]);
            if (!systemPromptRes0)
                return Utils::ResultError(systemPromptRes0.error());
            result._systemPrompt = *systemPromptRes0;
        }
        else if (obj.contains("systemPrompt"))
            result._systemPrompt = std::nullopt;
        return {};
    }();
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
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
    if (data._systemPrompt.has_value())
        obj.insert("systemPrompt", toJson(*data._systemPrompt));
    else if (data._systemPrompt.isNull())
        obj.insert("systemPrompt", QJsonValue::Null);
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
        return Utils::ResultError("Expected JSON object for AgentCapabilities");
    const QJsonObject obj = val.toObject();
    AgentCapabilities result;
    // "session": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("session") && !obj["session"].isNull()) {
            const auto sessionRes0 = fromJson<SessionCapabilities>("session", obj["session"]);
            if (!sessionRes0)
                return Utils::ResultError(sessionRes0.error());
            result._session = *sessionRes0;
        }
        else if (obj.contains("session"))
            result._session = std::nullopt;
        return {};
    }();
    // "auth": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("auth") && !obj["auth"].isNull()) {
            const auto authRes0 = fromJson<AgentAuthCapabilities>("auth", obj["auth"]);
            if (!authRes0)
                return Utils::ResultError(authRes0.error());
            result._auth = *authRes0;
        }
        else if (obj.contains("auth"))
            result._auth = std::nullopt;
        return {};
    }();
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
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
        return Utils::ResultError("Expected JSON object for AuthMethodAgent");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("methodId"))
        return Utils::ResultError("Missing required field: methodId");
    if (!obj["methodId"].isString())
        return Utils::ResultError("Invalid type for field: methodId");
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    if (!obj["name"].isString())
        return Utils::ResultError("Invalid type for field: name");
    AuthMethodAgent result;
    const auto res0 = fromJson<AuthMethodId>("methodId", obj["methodId"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._methodId = *res0;
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
    return result;
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
Utils::Result<EnvVariable> fromJson<EnvVariable>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for EnvVariable");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    if (!obj["name"].isString())
        return Utils::ResultError("Invalid type for field: name");
    if (!obj.contains("value"))
        return Utils::ResultError("Missing required field: value");
    if (!obj["value"].isString())
        return Utils::ResultError("Invalid type for field: value");
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
Utils::Result<AuthMethodTerminal> fromJson<AuthMethodTerminal>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for AuthMethodTerminal");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("methodId"))
        return Utils::ResultError("Missing required field: methodId");
    if (!obj["methodId"].isString())
        return Utils::ResultError("Invalid type for field: methodId");
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    if (!obj["name"].isString())
        return Utils::ResultError("Invalid type for field: name");
    AuthMethodTerminal result;
    const auto res0 = fromJson<AuthMethodId>("methodId", obj["methodId"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._methodId = *res0;
    result._name = obj.value("name").toString();
    if (obj.contains("description")) {
        if (!obj["description"].isNull()) {
            result._description = obj.value("description").toString();
        } else {
            result._description = std::nullopt;
        }
    }
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
            const auto res1 = fromJson<EnvVariable>("env", v);
            if (!res1)
                return Utils::ResultError(res1.error());
            list_env.append(*res1);
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
    return result;
}

QJsonObject toJson(const AuthMethodTerminal &data)
{
    QJsonObject obj{
        {"methodId", data._methodId},
        {"name", data._name}
    };
    if (data._description.has_value())
        obj.insert("description", *data._description);
    else if (data._description.isNull())
        obj.insert("description", QJsonValue::Null);
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
    if (dispatchValue.isEmpty())
        return Utils::ResultError("Invalid AuthMethod: missing type");
    return AuthMethod(val.toObject());  // open union: preserve unknown variants raw
}

QString dispatchValue(const AuthMethod &val)
{
    return std::visit([](const auto &v) -> QString {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, AuthMethodTerminal>) return "terminal";
        else if constexpr (std::is_same_v<T, AuthMethodAgent>) return "agent";
        else if constexpr (std::is_same_v<T, QJsonObject>) return v.value("type").toString();
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

template<>
Utils::Result<Implementation> fromJson<Implementation>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Implementation");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    if (!obj["name"].isString())
        return Utils::ResultError("Invalid type for field: name");
    if (!obj.contains("version"))
        return Utils::ResultError("Missing required field: version");
    if (!obj["version"].isString())
        return Utils::ResultError("Invalid type for field: version");
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

template<>
Utils::Result<InitializeResponse> fromJson<InitializeResponse>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InitializeResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("protocolVersion"))
        return Utils::ResultError("Missing required field: protocolVersion");
    if (!obj["protocolVersion"].isDouble())
        return Utils::ResultError("Invalid type for field: protocolVersion");
    if (!obj.contains("info"))
        return Utils::ResultError("Missing required field: info");
    if (!obj["info"].isObject())
        return Utils::ResultError("Invalid type for field: info");
    InitializeResponse result;
    const auto res0 = fromJson<ProtocolVersion>("protocolVersion", obj["protocolVersion"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._protocolVersion = *res0;
    const auto res1 = fromJson<Implementation>("info", obj["info"]);
    if (!res1)
        return Utils::ResultError(res1.error());
    result._info = *res1;
    // "capabilities": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("capabilities") && obj["capabilities"].isObject()) {
            const auto capabilitiesRes0 = fromJson<AgentCapabilities>("capabilities", obj["capabilities"]);
            if (!capabilitiesRes0)
                return Utils::ResultError(capabilitiesRes0.error());
            result._capabilities = *capabilitiesRes0;
        }
        return {};
    }();
    if (obj.contains("authMethods") && obj["authMethods"].isArray()) {
        const QJsonArray arr = obj["authMethods"].toArray();
        QList<AuthMethod> list_authMethods;
        for (const QJsonValue &v : arr) {
            const auto authMethodsItem = fromJson<AuthMethod>("authMethods", v);
            if (!authMethodsItem)
                continue;
            list_authMethods.append(*authMethodsItem);
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
    return result;
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
        return Utils::ResultError("Expected JSON object for SessionInfo");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    if (!obj["sessionId"].isString())
        return Utils::ResultError("Invalid type for field: sessionId");
    if (!obj.contains("cwd"))
        return Utils::ResultError("Missing required field: cwd");
    if (!obj["cwd"].isString())
        return Utils::ResultError("Invalid type for field: cwd");
    SessionInfo result;
    const auto res0 = fromJson<SessionId>("sessionId", obj["sessionId"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._sessionId = *res0;
    const auto res1 = fromJson<AbsolutePath>("cwd", obj["cwd"]);
    if (!res1)
        return Utils::ResultError(res1.error());
    result._cwd = *res1;
    if (obj.contains("additionalDirectories") && obj["additionalDirectories"].isArray()) {
        const QJsonArray arr = obj["additionalDirectories"].toArray();
        QList<AbsolutePath> list_additionalDirectories;
        for (const QJsonValue &v : arr) {
            const auto res2 = fromJson<AbsolutePath>("additionalDirectories", v);
            if (!res2)
                return Utils::ResultError(res2.error());
            list_additionalDirectories.append(*res2);
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
        return Utils::ResultError("Expected JSON object for ListSessionsResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessions"))
        return Utils::ResultError("Missing required field: sessions");
    if (!obj["sessions"].isArray())
        return Utils::ResultError("Invalid type for field: sessions");
    ListSessionsResponse result;
    {
        const QJsonArray arr = obj["sessions"].toArray();
        for (const QJsonValue &v : arr) {
            const auto sessionsItem = fromJson<SessionInfo>("sessions", v);
            if (!sessionsItem)
                continue;
            result._sessions.append(*sessionsItem);
        }
    }
    if (obj.contains("nextCursor") && !obj["nextCursor"].isNull()) {
        const auto res0 = fromJson<SessionListCursor>("nextCursor", obj["nextCursor"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._nextCursor = *res0;
    }
    else if (obj.contains("nextCursor"))
        result._nextCursor = std::nullopt;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
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
Utils::Result<TextCommandInput> fromJson<TextCommandInput>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TextCommandInput");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("hint"))
        return Utils::ResultError("Missing required field: hint");
    if (!obj["hint"].isString())
        return Utils::ResultError("Invalid type for field: hint");
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
        return Utils::ResultError("Invalid AvailableCommandInput: expected object");
    const QString dispatchValue = val.toObject().value("type").toString();
    if (dispatchValue == "text") {
        const auto res0 = fromJson<TextCommandInput>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return AvailableCommandInput(*res0);
    }
    if (dispatchValue.isEmpty())
        return Utils::ResultError("Invalid AvailableCommandInput: missing type");
    return AvailableCommandInput(val.toObject());  // open union: preserve unknown variants raw
}

QString dispatchValue(const AvailableCommandInput &val)
{
    return std::visit([](const auto &v) -> QString {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, TextCommandInput>) return "text";
        else if constexpr (std::is_same_v<T, QJsonObject>) return v.value("type").toString();
        return {};
    }, val);
}

QJsonObject toJson(const AvailableCommandInput &val)
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

QJsonValue toJsonValue(const AvailableCommandInput &val)
{
    return toJson(val);
}

template<>
Utils::Result<Command> fromJson<Command>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Command");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    if (!obj["name"].isString())
        return Utils::ResultError("Invalid type for field: name");
    if (!obj.contains("description"))
        return Utils::ResultError("Missing required field: description");
    if (!obj["description"].isString())
        return Utils::ResultError("Invalid type for field: description");
    Command result;
    result._name = obj.value("name").toString();
    result._description = obj.value("description").toString();
    // "input": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("input") && !obj["input"].isNull()) {
            const auto inputRes0 = fromJson<AvailableCommandInput>("input", obj["input"]);
            if (!inputRes0)
                return Utils::ResultError(inputRes0.error());
            result._input = *inputRes0;
        }
        else if (obj.contains("input"))
            result._input = std::nullopt;
        return {};
    }();
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const Command &data)
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
Utils::Result<AvailableCommand> fromJson<AvailableCommand>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid AvailableCommand: expected object");
    const QString dispatchValue = val.toObject().value("type").toString();
    if (dispatchValue == "command") {
        const auto res0 = fromJson<Command>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return AvailableCommand(*res0);
    }
    if (dispatchValue.isEmpty())
        return Utils::ResultError("Invalid AvailableCommand: missing type");
    return AvailableCommand(val.toObject());  // open union: preserve unknown variants raw
}

QString dispatchValue(const AvailableCommand &val)
{
    return std::visit([](const auto &v) -> QString {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, Command>) return "command";
        else if constexpr (std::is_same_v<T, QJsonObject>) return v.value("type").toString();
        return {};
    }, val);
}

QJsonObject toJson(const AvailableCommand &val)
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

QJsonValue toJsonValue(const AvailableCommand &val)
{
    return toJson(val);
}

template<>
Utils::Result<SessionConfigBoolean> fromJson<SessionConfigBoolean>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SessionConfigBoolean");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("currentValue"))
        return Utils::ResultError("Missing required field: currentValue");
    if (!obj["currentValue"].isBool())
        return Utils::ResultError("Invalid type for field: currentValue");
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
    if (!val.isString())
        return Utils::ResultError("Expected JSON string for SessionConfigOptionCategory");
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
    if (!obj["value"].isString())
        return Utils::ResultError("Invalid type for field: value");
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    if (!obj["name"].isString())
        return Utils::ResultError("Invalid type for field: name");
    SessionConfigSelectOption result;
    const auto res0 = fromJson<SessionConfigValueId>("value", obj["value"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._value = *res0;
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
        return Utils::ResultError("Expected JSON object for SessionConfigSelectGroup");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("groupId"))
        return Utils::ResultError("Missing required field: groupId");
    if (!obj["groupId"].isString())
        return Utils::ResultError("Invalid type for field: groupId");
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    if (!obj["name"].isString())
        return Utils::ResultError("Invalid type for field: name");
    if (!obj.contains("options"))
        return Utils::ResultError("Missing required field: options");
    if (!obj["options"].isArray())
        return Utils::ResultError("Invalid type for field: options");
    SessionConfigSelectGroup result;
    const auto res0 = fromJson<SessionConfigGroupId>("groupId", obj["groupId"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._groupId = *res0;
    result._name = obj.value("name").toString();
    {
        const QJsonArray arr = obj["options"].toArray();
        for (const QJsonValue &v : arr) {
            const auto optionsItem = fromJson<SessionConfigSelectOption>("options", v);
            if (!optionsItem)
                continue;
            result._options.append(*optionsItem);
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
        return Utils::ResultError("Expected JSON object for SessionConfigSelect");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("currentValue"))
        return Utils::ResultError("Missing required field: currentValue");
    if (!obj["currentValue"].isString())
        return Utils::ResultError("Invalid type for field: currentValue");
    if (!obj.contains("options"))
        return Utils::ResultError("Missing required field: options");
    SessionConfigSelect result;
    const auto res0 = fromJson<SessionConfigValueId>("currentValue", obj["currentValue"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._currentValue = *res0;
    const auto res1 = fromJson<SessionConfigSelectOptions>("options", obj["options"]);
    if (!res1)
        return Utils::ResultError(res1.error());
    result._options = *res1;
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
    if (!obj.contains("configId"))
        return Utils::ResultError("Missing required field: configId");
    if (!obj["configId"].isString())
        return Utils::ResultError("Invalid type for field: configId");
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    if (!obj["name"].isString())
        return Utils::ResultError("Invalid type for field: name");
    SessionConfigOption result;
    const auto res0 = fromJson<SessionConfigId>("configId", obj["configId"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._configId = *res0;
    result._name = obj.value("name").toString();
    if (obj.contains("description")) {
        if (!obj["description"].isNull()) {
            result._description = obj.value("description").toString();
        } else {
            result._description = std::nullopt;
        }
    }
    // "category": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("category") && !obj["category"].isNull()) {
            const auto categoryRes0 = fromJson<SessionConfigOptionCategory>("category", obj["category"]);
            if (!categoryRes0)
                return Utils::ResultError(categoryRes0.error());
            result._category = *categoryRes0;
        }
        else if (obj.contains("category"))
            result._category = std::nullopt;
        return {};
    }();
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
    return result;
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
        return Utils::ResultError("Expected JSON object for NewSessionResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    if (!obj["sessionId"].isString())
        return Utils::ResultError("Invalid type for field: sessionId");
    NewSessionResponse result;
    const auto res0 = fromJson<SessionId>("sessionId", obj["sessionId"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._sessionId = *res0;
    if (obj.contains("configOptions") && obj["configOptions"].isArray()) {
        const QJsonArray arr = obj["configOptions"].toArray();
        QList<SessionConfigOption> list_configOptions;
        for (const QJsonValue &v : arr) {
            const auto configOptionsItem = fromJson<SessionConfigOption>("configOptions", v);
            if (!configOptionsItem)
                continue;
            list_configOptions.append(*configOptionsItem);
        }
        result._configOptions = list_configOptions;
    }
    if (obj.contains("availableCommands") && obj["availableCommands"].isArray()) {
        const QJsonArray arr = obj["availableCommands"].toArray();
        QList<AvailableCommand> list_availableCommands;
        for (const QJsonValue &v : arr) {
            const auto availableCommandsItem = fromJson<AvailableCommand>("availableCommands", v);
            if (!availableCommandsItem)
                continue;
            list_availableCommands.append(*availableCommandsItem);
        }
        result._availableCommands = list_availableCommands;
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

QJsonObject toJson(const NewSessionResponse &data)
{
    QJsonObject obj{{"sessionId", data._sessionId}};
    if (data._configOptions.has_value()) {
        QJsonArray arr_configOptions;
        for (const auto &v : *data._configOptions) arr_configOptions.append(toJson(v));
        obj.insert("configOptions", arr_configOptions);
    }
    if (data._availableCommands.has_value()) {
        QJsonArray arr_availableCommands;
        for (const auto &v : *data._availableCommands) arr_availableCommands.append(toJsonValue(v));
        obj.insert("availableCommands", arr_availableCommands);
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
    if (!obj.contains("messageId"))
        return Utils::ResultError("Missing required field: messageId");
    if (!obj["messageId"].isString())
        return Utils::ResultError("Invalid type for field: messageId");
    PromptResponse result;
    const auto res0 = fromJson<MessageId>("messageId", obj["messageId"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._messageId = *res0;
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
    QJsonObject obj{{"messageId", data._messageId}};
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
        return Utils::ResultError("Expected JSON object for ResumeSessionResponse");
    const QJsonObject obj = val.toObject();
    ResumeSessionResponse result;
    if (obj.contains("configOptions") && obj["configOptions"].isArray()) {
        const QJsonArray arr = obj["configOptions"].toArray();
        QList<SessionConfigOption> list_configOptions;
        for (const QJsonValue &v : arr) {
            const auto configOptionsItem = fromJson<SessionConfigOption>("configOptions", v);
            if (!configOptionsItem)
                continue;
            list_configOptions.append(*configOptionsItem);
        }
        result._configOptions = list_configOptions;
    }
    if (obj.contains("availableCommands") && obj["availableCommands"].isArray()) {
        const QJsonArray arr = obj["availableCommands"].toArray();
        QList<AvailableCommand> list_availableCommands;
        for (const QJsonValue &v : arr) {
            const auto availableCommandsItem = fromJson<AvailableCommand>("availableCommands", v);
            if (!availableCommandsItem)
                continue;
            list_availableCommands.append(*availableCommandsItem);
        }
        result._availableCommands = list_availableCommands;
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

QJsonObject toJson(const ResumeSessionResponse &data)
{
    QJsonObject obj;
    if (data._configOptions.has_value()) {
        QJsonArray arr_configOptions;
        for (const auto &v : *data._configOptions) arr_configOptions.append(toJson(v));
        obj.insert("configOptions", arr_configOptions);
    }
    if (data._availableCommands.has_value()) {
        QJsonArray arr_availableCommands;
        for (const auto &v : *data._availableCommands) arr_availableCommands.append(toJsonValue(v));
        obj.insert("availableCommands", arr_availableCommands);
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
        return Utils::ResultError("Expected JSON object for SetSessionConfigOptionResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("configOptions"))
        return Utils::ResultError("Missing required field: configOptions");
    if (!obj["configOptions"].isArray())
        return Utils::ResultError("Invalid type for field: configOptions");
    SetSessionConfigOptionResponse result;
    {
        const QJsonArray arr = obj["configOptions"].toArray();
        for (const QJsonValue &v : arr) {
            const auto configOptionsItem = fromJson<SessionConfigOption>("configOptions", v);
            if (!configOptionsItem)
                continue;
            result._configOptions.append(*configOptionsItem);
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
        return Utils::ResultError("Expected JSON object for CompleteElicitationNotification");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("elicitationId"))
        return Utils::ResultError("Missing required field: elicitationId");
    if (!obj["elicitationId"].isString())
        return Utils::ResultError("Invalid type for field: elicitationId");
    CompleteElicitationNotification result;
    const auto res0 = fromJson<ElicitationId>("elicitationId", obj["elicitationId"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._elicitationId = *res0;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
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
        return Utils::ResultError("Expected JSON object for AgentMessage");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("messageId"))
        return Utils::ResultError("Missing required field: messageId");
    if (!obj["messageId"].isString())
        return Utils::ResultError("Invalid type for field: messageId");
    AgentMessage result;
    const auto res0 = fromJson<MessageId>("messageId", obj["messageId"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._messageId = *res0;
    if (obj.contains("content") && obj["content"].isArray()) {
        const QJsonArray arr = obj["content"].toArray();
        QList<ContentBlock> list_content;
        for (const QJsonValue &v : arr) {
            const auto contentItem = fromJson<ContentBlock>("content", v);
            if (!contentItem)
                continue;
            list_content.append(*contentItem);
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
    return result;
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
        return Utils::ResultError("Expected JSON object for AgentThought");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("messageId"))
        return Utils::ResultError("Missing required field: messageId");
    if (!obj["messageId"].isString())
        return Utils::ResultError("Invalid type for field: messageId");
    AgentThought result;
    const auto res0 = fromJson<MessageId>("messageId", obj["messageId"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._messageId = *res0;
    if (obj.contains("content") && obj["content"].isArray()) {
        const QJsonArray arr = obj["content"].toArray();
        QList<ContentBlock> list_content;
        for (const QJsonValue &v : arr) {
            const auto contentItem = fromJson<ContentBlock>("content", v);
            if (!contentItem)
                continue;
            list_content.append(*contentItem);
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
    return result;
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
Utils::Result<AvailableCommandsUpdate> fromJson<AvailableCommandsUpdate>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for AvailableCommandsUpdate");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("availableCommands"))
        return Utils::ResultError("Missing required field: availableCommands");
    if (!obj["availableCommands"].isArray())
        return Utils::ResultError("Invalid type for field: availableCommands");
    AvailableCommandsUpdate result;
    {
        const QJsonArray arr = obj["availableCommands"].toArray();
        for (const QJsonValue &v : arr) {
            const auto availableCommandsItem = fromJson<AvailableCommand>("availableCommands", v);
            if (!availableCommandsItem)
                continue;
            result._availableCommands.append(*availableCommandsItem);
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

QJsonObject toJson(const AvailableCommandsUpdate &data)
{
    QJsonObject obj;
    QJsonArray arr_availableCommands;
    for (const auto &v : data._availableCommands) arr_availableCommands.append(toJsonValue(v));
    obj.insert("availableCommands", arr_availableCommands);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<CompactionSummaryChunk> fromJson<CompactionSummaryChunk>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CompactionSummaryChunk");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("compactionId"))
        return Utils::ResultError("Missing required field: compactionId");
    if (!obj["compactionId"].isString())
        return Utils::ResultError("Invalid type for field: compactionId");
    if (!obj.contains("content"))
        return Utils::ResultError("Missing required field: content");
    CompactionSummaryChunk result;
    const auto res0 = fromJson<CompactionId>("compactionId", obj["compactionId"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._compactionId = *res0;
    const auto res1 = fromJson<ContentBlock>("content", obj["content"]);
    if (!res1)
        return Utils::ResultError(res1.error());
    result._content = *res1;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const CompactionSummaryChunk &data)
{
    QJsonObject obj{
        {"compactionId", data._compactionId},
        {"content", toJsonValue(data._content)}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

QString toString(CompactionStatus v)
{
    switch(v) {
        case CompactionStatus::in_progress: return "in_progress";
        case CompactionStatus::completed: return "completed";
        case CompactionStatus::failed: return "failed";
        case CompactionStatus::cancelled: return "cancelled";
    }
    return {};
}

template<>
Utils::Result<CompactionStatus> fromJson<CompactionStatus>(const QJsonValue &val)
{
    if (!val.isString())
        return Utils::ResultError("Expected JSON string for CompactionStatus");
    const QString str = val.toString();
    if (str == "in_progress") return CompactionStatus::in_progress;
    if (str == "completed") return CompactionStatus::completed;
    if (str == "failed") return CompactionStatus::failed;
    if (str == "cancelled") return CompactionStatus::cancelled;
    return Utils::ResultError("Invalid CompactionStatus value: " + str);
}

QJsonValue toJsonValue(const CompactionStatus &v)
{
    return toString(v);
}

template<>
Utils::Result<CompactionUpdate> fromJson<CompactionUpdate>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CompactionUpdate");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("compactionId"))
        return Utils::ResultError("Missing required field: compactionId");
    if (!obj["compactionId"].isString())
        return Utils::ResultError("Invalid type for field: compactionId");
    if (!obj.contains("status"))
        return Utils::ResultError("Missing required field: status");
    CompactionUpdate result;
    const auto res0 = fromJson<CompactionId>("compactionId", obj["compactionId"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._compactionId = *res0;
    const auto res1 = fromJson<CompactionStatus>("status", obj["status"]);
    if (!res1)
        return Utils::ResultError(res1.error());
    result._status = *res1;
    if (obj.contains("summary") && obj["summary"].isArray()) {
        const QJsonArray arr = obj["summary"].toArray();
        QList<ContentBlock> list_summary;
        for (const QJsonValue &v : arr) {
            const auto summaryItem = fromJson<ContentBlock>("summary", v);
            if (!summaryItem)
                continue;
            list_summary.append(*summaryItem);
        }
        result._summary = list_summary;
    }
    else if (obj.contains("summary") && obj["summary"].isNull())
        result._summary = std::nullopt;
    if (obj.contains("error")) {
        if (!obj["error"].isNull()) {
            result._error = obj.value("error").toString();
        } else {
            result._error = std::nullopt;
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

QJsonObject toJson(const CompactionUpdate &data)
{
    QJsonObject obj{
        {"compactionId", data._compactionId},
        {"status", toJsonValue(data._status)}
    };
    if (data._summary.has_value()) {
        QJsonArray arr_summary;
        for (const auto &v : *data._summary) arr_summary.append(toJsonValue(v));
        obj.insert("summary", arr_summary);
    }
    else if (data._summary.isNull())
        obj.insert("summary", QJsonValue::Null);
    if (data._error.has_value())
        obj.insert("error", *data._error);
    else if (data._error.isNull())
        obj.insert("error", QJsonValue::Null);
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
        return Utils::ResultError("Expected JSON object for ConfigOptionUpdate");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("configOptions"))
        return Utils::ResultError("Missing required field: configOptions");
    if (!obj["configOptions"].isArray())
        return Utils::ResultError("Invalid type for field: configOptions");
    ConfigOptionUpdate result;
    {
        const QJsonArray arr = obj["configOptions"].toArray();
        for (const QJsonValue &v : arr) {
            const auto configOptionsItem = fromJson<SessionConfigOption>("configOptions", v);
            if (!configOptionsItem)
                continue;
            result._configOptions.append(*configOptionsItem);
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
        return Utils::ResultError("Expected JSON object for ContentChunk");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("messageId"))
        return Utils::ResultError("Missing required field: messageId");
    if (!obj["messageId"].isString())
        return Utils::ResultError("Invalid type for field: messageId");
    if (!obj.contains("content"))
        return Utils::ResultError("Missing required field: content");
    ContentChunk result;
    const auto res0 = fromJson<MessageId>("messageId", obj["messageId"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._messageId = *res0;
    const auto res1 = fromJson<ContentBlock>("content", obj["content"]);
    if (!res1)
        return Utils::ResultError(res1.error());
    result._content = *res1;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
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

QString toString(NoticeSeverity v)
{
    switch(v) {
        case NoticeSeverity::info: return "info";
        case NoticeSeverity::warning: return "warning";
        case NoticeSeverity::error: return "error";
    }
    return {};
}

template<>
Utils::Result<NoticeSeverity> fromJson<NoticeSeverity>(const QJsonValue &val)
{
    if (!val.isString())
        return Utils::ResultError("Expected JSON string for NoticeSeverity");
    const QString str = val.toString();
    if (str == "info") return NoticeSeverity::info;
    if (str == "warning") return NoticeSeverity::warning;
    if (str == "error") return NoticeSeverity::error;
    return Utils::ResultError("Invalid NoticeSeverity value: " + str);
}

QJsonValue toJsonValue(const NoticeSeverity &v)
{
    return toString(v);
}

template<>
Utils::Result<Notice> fromJson<Notice>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Notice");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("severity"))
        return Utils::ResultError("Missing required field: severity");
    if (!obj.contains("title"))
        return Utils::ResultError("Missing required field: title");
    if (!obj["title"].isString())
        return Utils::ResultError("Invalid type for field: title");
    Notice result;
    const auto res0 = fromJson<NoticeSeverity>("severity", obj["severity"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._severity = *res0;
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

QJsonObject toJson(const Notice &data)
{
    QJsonObject obj{
        {"severity", toJsonValue(data._severity)},
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
    if (!val.isString())
        return Utils::ResultError("Expected JSON string for PlanEntryPriority");
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
    if (!val.isString())
        return Utils::ResultError("Expected JSON string for PlanEntryStatus");
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
        return Utils::ResultError("Expected JSON object for PlanEntry");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("content"))
        return Utils::ResultError("Missing required field: content");
    if (!obj["content"].isString())
        return Utils::ResultError("Invalid type for field: content");
    if (!obj.contains("priority"))
        return Utils::ResultError("Missing required field: priority");
    if (!obj.contains("status"))
        return Utils::ResultError("Missing required field: status");
    PlanEntry result;
    result._content = obj.value("content").toString();
    const auto res0 = fromJson<PlanEntryPriority>("priority", obj["priority"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._priority = *res0;
    const auto res1 = fromJson<PlanEntryStatus>("status", obj["status"]);
    if (!res1)
        return Utils::ResultError(res1.error());
    result._status = *res1;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
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
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<PlanItems> fromJson<PlanItems>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for PlanItems");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("planId"))
        return Utils::ResultError("Missing required field: planId");
    if (!obj["planId"].isString())
        return Utils::ResultError("Invalid type for field: planId");
    if (!obj.contains("entries"))
        return Utils::ResultError("Missing required field: entries");
    if (!obj["entries"].isArray())
        return Utils::ResultError("Invalid type for field: entries");
    PlanItems result;
    const auto res0 = fromJson<PlanId>("planId", obj["planId"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._planId = *res0;
    {
        const QJsonArray arr = obj["entries"].toArray();
        for (const QJsonValue &v : arr) {
            const auto entriesItem = fromJson<PlanEntry>("entries", v);
            if (!entriesItem)
                continue;
            result._entries.append(*entriesItem);
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
        return Utils::ResultError("Invalid PlanUpdateContent: expected object");
    const QString dispatchValue = val.toObject().value("type").toString();
    if (dispatchValue == "items") {
        const auto res0 = fromJson<PlanItems>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return PlanUpdateContent(*res0);
    }
    if (dispatchValue.isEmpty())
        return Utils::ResultError("Invalid PlanUpdateContent: missing type");
    return PlanUpdateContent(val.toObject());  // open union: preserve unknown variants raw
}

QString dispatchValue(const PlanUpdateContent &val)
{
    return std::visit([](const auto &v) -> QString {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, PlanItems>) return "items";
        else if constexpr (std::is_same_v<T, QJsonObject>) return v.value("type").toString();
        return {};
    }, val);
}

QJsonObject toJson(const PlanUpdateContent &val)
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

QJsonValue toJsonValue(const PlanUpdateContent &val)
{
    return toJson(val);
}

template<>
Utils::Result<PlanUpdate> fromJson<PlanUpdate>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for PlanUpdate");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("plan"))
        return Utils::ResultError("Missing required field: plan");
    PlanUpdate result;
    const auto res0 = fromJson<PlanUpdateContent>("plan", obj["plan"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._plan = *res0;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
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

template<>
Utils::Result<BackgroundTerminal> fromJson<BackgroundTerminal>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for BackgroundTerminal");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("terminalId"))
        return Utils::ResultError("Missing required field: terminalId");
    if (!obj["terminalId"].isString())
        return Utils::ResultError("Invalid type for field: terminalId");
    BackgroundTerminal result;
    const auto res0 = fromJson<TerminalId>("terminalId", obj["terminalId"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._terminalId = *res0;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const BackgroundTerminal &data)
{
    QJsonObject obj{{"terminalId", data._terminalId}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<BackgroundToolCall> fromJson<BackgroundToolCall>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for BackgroundToolCall");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("toolCallId"))
        return Utils::ResultError("Missing required field: toolCallId");
    if (!obj["toolCallId"].isString())
        return Utils::ResultError("Invalid type for field: toolCallId");
    BackgroundToolCall result;
    const auto res0 = fromJson<ToolCallId>("toolCallId", obj["toolCallId"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._toolCallId = *res0;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const BackgroundToolCall &data)
{
    QJsonObject obj{{"toolCallId", data._toolCallId}};
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<BackgroundWork> fromJson<BackgroundWork>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid BackgroundWork: expected object");
    const QString dispatchValue = val.toObject().value("type").toString();
    if (dispatchValue == "terminal") {
        const auto res0 = fromJson<BackgroundTerminal>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return BackgroundWork(*res0);
    }
    else if (dispatchValue == "tool_call") {
        const auto res1 = fromJson<BackgroundToolCall>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        return BackgroundWork(*res1);
    }
    if (dispatchValue.isEmpty())
        return Utils::ResultError("Invalid BackgroundWork: missing type");
    return BackgroundWork(val.toObject());  // open union: preserve unknown variants raw
}

QString dispatchValue(const BackgroundWork &val)
{
    return std::visit([](const auto &v) -> QString {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, BackgroundTerminal>) return "terminal";
        else if constexpr (std::is_same_v<T, BackgroundToolCall>) return "tool_call";
        else if constexpr (std::is_same_v<T, QJsonObject>) return v.value("type").toString();
        return {};
    }, val);
}

QJsonObject toJson(const BackgroundWork &val)
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

QJsonValue toJsonValue(const BackgroundWork &val)
{
    return toJson(val);
}

template<>
Utils::Result<ErrorStopReason> fromJson<ErrorStopReason>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ErrorStopReason");
    const QJsonObject obj = val.toObject();
    ErrorStopReason result;
    // "error": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("error") && !obj["error"].isNull()) {
            const auto errorRes0 = fromJson<Error>("error", obj["error"]);
            if (!errorRes0)
                return Utils::ResultError(errorRes0.error());
            result._error = *errorRes0;
        }
        else if (obj.contains("error"))
            result._error = std::nullopt;
        return {};
    }();
    return result;
}

QJsonObject toJson(const ErrorStopReason &data)
{
    QJsonObject obj;
    if (data._error.has_value())
        obj.insert("error", toJson(*data._error));
    else if (data._error.isNull())
        obj.insert("error", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<IdleStateUpdate> fromJson<IdleStateUpdate>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for IdleStateUpdate");
    const QJsonObject obj = val.toObject();
    IdleStateUpdate result;
    // "background": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("background") && obj["background"].isArray()) {
            const QJsonArray arr = obj["background"].toArray();
            QList<BackgroundWork> list_background;
            for (const QJsonValue &v : arr) {
                const auto backgroundRes0 = fromJson<BackgroundWork>("background", v);
                if (!backgroundRes0)
                    return Utils::ResultError(backgroundRes0.error());
                list_background.append(*backgroundRes0);
            }
            result._background = list_background;
        }
        else if (obj.contains("background") && obj["background"].isNull())
            result._background = std::nullopt;
        return {};
    }();
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    if (obj.contains("stopReason")) {
        if (!obj["stopReason"].isNull()) {
            result._stopReason = obj.value("stopReason").toString();
        } else {
            result._stopReason = std::nullopt;
        }
    }
    {
        const QSet<QString> knownKeys{"background", "_meta", "stopReason"};
        for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
            if (!knownKeys.contains(it.key()))
                result._additionalProperties.insert(it.key(), it.value());
        }
    }
    return result;
}

QJsonObject toJson(const IdleStateUpdate &data)
{
    QJsonObject obj;
    if (data._background.has_value()) {
        QJsonArray arr_background;
        for (const auto &v : *data._background) arr_background.append(toJsonValue(v));
        obj.insert("background", arr_background);
    }
    else if (data._background.isNull())
        obj.insert("background", QJsonValue::Null);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    if (data._stopReason.has_value())
        obj.insert("stopReason", *data._stopReason);
    else if (data._stopReason.isNull())
        obj.insert("stopReason", QJsonValue::Null);
    for (auto it = data._additionalProperties.constBegin(); it != data._additionalProperties.constEnd(); ++it)
        obj.insert(it.key(), it.value());
    return obj;
}

template<>
Utils::Result<RequiresActionStateUpdate> fromJson<RequiresActionStateUpdate>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for RequiresActionStateUpdate");
    const QJsonObject obj = val.toObject();
    RequiresActionStateUpdate result;
    // "background": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("background") && obj["background"].isArray()) {
            const QJsonArray arr = obj["background"].toArray();
            QList<BackgroundWork> list_background;
            for (const QJsonValue &v : arr) {
                const auto backgroundRes0 = fromJson<BackgroundWork>("background", v);
                if (!backgroundRes0)
                    return Utils::ResultError(backgroundRes0.error());
                list_background.append(*backgroundRes0);
            }
            result._background = list_background;
        }
        else if (obj.contains("background") && obj["background"].isNull())
            result._background = std::nullopt;
        return {};
    }();
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
    if (data._background.has_value()) {
        QJsonArray arr_background;
        for (const auto &v : *data._background) arr_background.append(toJsonValue(v));
        obj.insert("background", arr_background);
    }
    else if (data._background.isNull())
        obj.insert("background", QJsonValue::Null);
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
    // "background": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("background") && obj["background"].isArray()) {
            const QJsonArray arr = obj["background"].toArray();
            QList<BackgroundWork> list_background;
            for (const QJsonValue &v : arr) {
                const auto backgroundRes0 = fromJson<BackgroundWork>("background", v);
                if (!backgroundRes0)
                    return Utils::ResultError(backgroundRes0.error());
                list_background.append(*backgroundRes0);
            }
            result._background = list_background;
        }
        else if (obj.contains("background") && obj["background"].isNull())
            result._background = std::nullopt;
        return {};
    }();
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
    if (data._background.has_value()) {
        QJsonArray arr_background;
        for (const auto &v : *data._background) arr_background.append(toJsonValue(v));
        obj.insert("background", arr_background);
    }
    else if (data._background.isNull())
        obj.insert("background", QJsonValue::Null);
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
        return Utils::ResultError("Invalid StateUpdate: expected object");
    const QString dispatchValue = val.toObject().value("state").toString();
    if (dispatchValue == "running") {
        const auto res0 = fromJson<RunningStateUpdate>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return StateUpdate(*res0);
    }
    else if (dispatchValue == "idle") {
        const auto res1 = fromJson<IdleStateUpdate>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        return StateUpdate(*res1);
    }
    else if (dispatchValue == "requires_action") {
        const auto res2 = fromJson<RequiresActionStateUpdate>(val);
        if (!res2)
            return Utils::ResultError(res2.error());
        return StateUpdate(*res2);
    }
    if (dispatchValue.isEmpty())
        return Utils::ResultError("Invalid StateUpdate: missing state");
    return StateUpdate(val.toObject());  // open union: preserve unknown variants raw
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
        return Utils::ResultError("Expected JSON object for TerminalOutputChunk");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("terminalId"))
        return Utils::ResultError("Missing required field: terminalId");
    if (!obj["terminalId"].isString())
        return Utils::ResultError("Invalid type for field: terminalId");
    if (!obj.contains("data"))
        return Utils::ResultError("Missing required field: data");
    if (!obj["data"].isString())
        return Utils::ResultError("Invalid type for field: data");
    TerminalOutputChunk result;
    const auto res0 = fromJson<TerminalId>("terminalId", obj["terminalId"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._terminalId = *res0;
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
Utils::Result<TerminalCancelCapabilities> fromJson<TerminalCancelCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TerminalCancelCapabilities");
    const QJsonObject obj = val.toObject();
    TerminalCancelCapabilities result;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const TerminalCancelCapabilities &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<TerminalCapabilities> fromJson<TerminalCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TerminalCapabilities");
    const QJsonObject obj = val.toObject();
    TerminalCapabilities result;
    // "cancel": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("cancel") && !obj["cancel"].isNull()) {
            const auto cancelRes0 = fromJson<TerminalCancelCapabilities>("cancel", obj["cancel"]);
            if (!cancelRes0)
                return Utils::ResultError(cancelRes0.error());
            result._cancel = *cancelRes0;
        }
        else if (obj.contains("cancel"))
            result._cancel = std::nullopt;
        return {};
    }();
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const TerminalCapabilities &data)
{
    QJsonObject obj;
    if (data._cancel.has_value())
        obj.insert("cancel", toJson(*data._cancel));
    else if (data._cancel.isNull())
        obj.insert("cancel", QJsonValue::Null);
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
    if (!obj["data"].isString())
        return Utils::ResultError("Invalid type for field: data");
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
Utils::Result<TerminalSize> fromJson<TerminalSize>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TerminalSize");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("cols"))
        return Utils::ResultError("Missing required field: cols");
    if (!obj["cols"].isDouble())
        return Utils::ResultError("Invalid type for field: cols");
    if (!obj.contains("rows"))
        return Utils::ResultError("Missing required field: rows");
    if (!obj["rows"].isDouble())
        return Utils::ResultError("Invalid type for field: rows");
    TerminalSize result;
    result._cols = obj.value("cols").toInt();
    result._rows = obj.value("rows").toInt();
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const TerminalSize &data)
{
    QJsonObject obj{
        {"cols", data._cols},
        {"rows", data._rows}
    };
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
        return Utils::ResultError("Expected JSON object for TerminalUpdate");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("terminalId"))
        return Utils::ResultError("Missing required field: terminalId");
    if (!obj["terminalId"].isString())
        return Utils::ResultError("Invalid type for field: terminalId");
    TerminalUpdate result;
    const auto res0 = fromJson<TerminalId>("terminalId", obj["terminalId"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._terminalId = *res0;
    if (obj.contains("command")) {
        if (!obj["command"].isNull()) {
            result._command = obj.value("command").toString();
        } else {
            result._command = std::nullopt;
        }
    }
    // "cwd": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("cwd") && !obj["cwd"].isNull()) {
            const auto cwdRes0 = fromJson<AbsolutePath>("cwd", obj["cwd"]);
            if (!cwdRes0)
                return Utils::ResultError(cwdRes0.error());
            result._cwd = *cwdRes0;
        }
        else if (obj.contains("cwd"))
            result._cwd = std::nullopt;
        return {};
    }();
    // "output": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("output") && !obj["output"].isNull()) {
            const auto outputRes0 = fromJson<TerminalOutput>("output", obj["output"]);
            if (!outputRes0)
                return Utils::ResultError(outputRes0.error());
            result._output = *outputRes0;
        }
        else if (obj.contains("output"))
            result._output = std::nullopt;
        return {};
    }();
    // "exitStatus": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("exitStatus") && !obj["exitStatus"].isNull()) {
            const auto exitStatusRes0 = fromJson<TerminalExitStatus>("exitStatus", obj["exitStatus"]);
            if (!exitStatusRes0)
                return Utils::ResultError(exitStatusRes0.error());
            result._exitStatus = *exitStatusRes0;
        }
        else if (obj.contains("exitStatus"))
            result._exitStatus = std::nullopt;
        return {};
    }();
    // "size": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("size") && !obj["size"].isNull()) {
            const auto sizeRes0 = fromJson<TerminalSize>("size", obj["size"]);
            if (!sizeRes0)
                return Utils::ResultError(sizeRes0.error());
            result._size = *sizeRes0;
        }
        else if (obj.contains("size"))
            result._size = std::nullopt;
        return {};
    }();
    // "capabilities": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("capabilities") && !obj["capabilities"].isNull()) {
            const auto capabilitiesRes0 = fromJson<TerminalCapabilities>("capabilities", obj["capabilities"]);
            if (!capabilitiesRes0)
                return Utils::ResultError(capabilitiesRes0.error());
            result._capabilities = *capabilitiesRes0;
        }
        else if (obj.contains("capabilities"))
            result._capabilities = std::nullopt;
        return {};
    }();
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
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
    if (data._size.has_value())
        obj.insert("size", toJson(*data._size));
    else if (data._size.isNull())
        obj.insert("size", QJsonValue::Null);
    if (data._capabilities.has_value())
        obj.insert("capabilities", toJson(*data._capabilities));
    else if (data._capabilities.isNull())
        obj.insert("capabilities", QJsonValue::Null);
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
        return Utils::ResultError("Expected JSON object for ToolCallContentChunk");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("toolCallId"))
        return Utils::ResultError("Missing required field: toolCallId");
    if (!obj["toolCallId"].isString())
        return Utils::ResultError("Invalid type for field: toolCallId");
    if (!obj.contains("content"))
        return Utils::ResultError("Missing required field: content");
    ToolCallContentChunk result;
    const auto res0 = fromJson<ToolCallId>("toolCallId", obj["toolCallId"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._toolCallId = *res0;
    const auto res1 = fromJson<ToolCallContent>("content", obj["content"]);
    if (!res1)
        return Utils::ResultError(res1.error());
    result._content = *res1;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
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
    if (!obj["amount"].isDouble())
        return Utils::ResultError("Invalid type for field: amount");
    if (!obj.contains("currency"))
        return Utils::ResultError("Missing required field: currency");
    if (!obj["currency"].isString())
        return Utils::ResultError("Invalid type for field: currency");
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
        return Utils::ResultError("Expected JSON object for UsageUpdate");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("used"))
        return Utils::ResultError("Missing required field: used");
    if (!obj["used"].isDouble())
        return Utils::ResultError("Invalid type for field: used");
    if (!obj.contains("size"))
        return Utils::ResultError("Missing required field: size");
    if (!obj["size"].isDouble())
        return Utils::ResultError("Invalid type for field: size");
    UsageUpdate result;
    result._used = obj.value("used").toInt();
    result._size = obj.value("size").toInt();
    // "cost": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("cost") && !obj["cost"].isNull()) {
            const auto costRes0 = fromJson<Cost>("cost", obj["cost"]);
            if (!costRes0)
                return Utils::ResultError(costRes0.error());
            result._cost = *costRes0;
        }
        else if (obj.contains("cost"))
            result._cost = std::nullopt;
        return {};
    }();
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
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
        return Utils::ResultError("Expected JSON object for UserMessage");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("messageId"))
        return Utils::ResultError("Missing required field: messageId");
    if (!obj["messageId"].isString())
        return Utils::ResultError("Invalid type for field: messageId");
    UserMessage result;
    const auto res0 = fromJson<MessageId>("messageId", obj["messageId"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._messageId = *res0;
    if (obj.contains("content") && obj["content"].isArray()) {
        const QJsonArray arr = obj["content"].toArray();
        QList<ContentBlock> list_content;
        for (const QJsonValue &v : arr) {
            const auto contentItem = fromJson<ContentBlock>("content", v);
            if (!contentItem)
                continue;
            list_content.append(*contentItem);
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
    return result;
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
    else if (kind == "user_message") {
        const auto res1 = fromJson<UserMessage>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._value = *res1;
    }
    else if (kind == "agent_message_chunk") {
        const auto res2 = fromJson<ContentChunk>(val);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._value = *res2;
    }
    else if (kind == "agent_message") {
        const auto res3 = fromJson<AgentMessage>(val);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._value = *res3;
    }
    else if (kind == "agent_thought_chunk") {
        const auto res4 = fromJson<ContentChunk>(val);
        if (!res4)
            return Utils::ResultError(res4.error());
        result._value = *res4;
    }
    else if (kind == "agent_thought") {
        const auto res5 = fromJson<AgentThought>(val);
        if (!res5)
            return Utils::ResultError(res5.error());
        result._value = *res5;
    }
    else if (kind == "state_update") {
        const auto res6 = fromJson<StateUpdate>(val);
        if (!res6)
            return Utils::ResultError(res6.error());
        result._value = *res6;
    }
    else if (kind == "tool_call_content_chunk") {
        const auto res7 = fromJson<ToolCallContentChunk>(val);
        if (!res7)
            return Utils::ResultError(res7.error());
        result._value = *res7;
    }
    else if (kind == "tool_call_update") {
        const auto res8 = fromJson<ToolCallUpdate>(val);
        if (!res8)
            return Utils::ResultError(res8.error());
        result._value = *res8;
    }
    else if (kind == "terminal_update") {
        const auto res9 = fromJson<TerminalUpdate>(val);
        if (!res9)
            return Utils::ResultError(res9.error());
        result._value = *res9;
    }
    else if (kind == "terminal_output_chunk") {
        const auto res10 = fromJson<TerminalOutputChunk>(val);
        if (!res10)
            return Utils::ResultError(res10.error());
        result._value = *res10;
    }
    else if (kind == "plan_update") {
        const auto res11 = fromJson<PlanUpdate>(val);
        if (!res11)
            return Utils::ResultError(res11.error());
        result._value = *res11;
    }
    else if (kind == "available_commands_update") {
        const auto res12 = fromJson<AvailableCommandsUpdate>(val);
        if (!res12)
            return Utils::ResultError(res12.error());
        result._value = *res12;
    }
    else if (kind == "config_option_update") {
        const auto res13 = fromJson<ConfigOptionUpdate>(val);
        if (!res13)
            return Utils::ResultError(res13.error());
        result._value = *res13;
    }
    else if (kind == "session_info_update") {
        const auto res14 = fromJson<SessionInfoUpdate>(val);
        if (!res14)
            return Utils::ResultError(res14.error());
        result._value = *res14;
    }
    else if (kind == "usage_update") {
        const auto res15 = fromJson<UsageUpdate>(val);
        if (!res15)
            return Utils::ResultError(res15.error());
        result._value = *res15;
    }
    else if (kind == "notice") {
        const auto res16 = fromJson<Notice>(val);
        if (!res16)
            return Utils::ResultError(res16.error());
        result._value = *res16;
    }
    else if (kind == "compaction_update") {
        const auto res17 = fromJson<CompactionUpdate>(val);
        if (!res17)
            return Utils::ResultError(res17.error());
        result._value = *res17;
    }
    else if (kind == "compaction_summary_chunk") {
        const auto res18 = fromJson<CompactionSummaryChunk>(val);
        if (!res18)
            return Utils::ResultError(res18.error());
        result._value = *res18;
    }
    else if (kind.isEmpty())
        return Utils::ResultError("Invalid SessionUpdate: missing sessionUpdate");
    else
        result._value = obj;  // open union: preserve unknown variants raw
    return result;
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
        return Utils::ResultError("Expected JSON object for UpdateSessionNotification");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    if (!obj["sessionId"].isString())
        return Utils::ResultError("Invalid type for field: sessionId");
    if (!obj.contains("update"))
        return Utils::ResultError("Missing required field: update");
    UpdateSessionNotification result;
    const auto res0 = fromJson<SessionId>("sessionId", obj["sessionId"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._sessionId = *res0;
    const auto res1 = fromJson<SessionUpdate>("update", obj["update"]);
    if (!res1)
        return Utils::ResultError(res1.error());
    result._update = *res1;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
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
    if (!obj["method"].isString())
        return Utils::ResultError("Invalid type for field: method");
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
Utils::Result<CancelTerminalRequest> fromJson<CancelTerminalRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CancelTerminalRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    if (!obj["sessionId"].isString())
        return Utils::ResultError("Invalid type for field: sessionId");
    if (!obj.contains("terminalId"))
        return Utils::ResultError("Missing required field: terminalId");
    if (!obj["terminalId"].isString())
        return Utils::ResultError("Invalid type for field: terminalId");
    CancelTerminalRequest result;
    const auto res0 = fromJson<SessionId>("sessionId", obj["sessionId"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._sessionId = *res0;
    const auto res1 = fromJson<TerminalId>("terminalId", obj["terminalId"]);
    if (!res1)
        return Utils::ResultError(res1.error());
    result._terminalId = *res1;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const CancelTerminalRequest &data)
{
    QJsonObject obj{
        {"sessionId", data._sessionId},
        {"terminalId", data._terminalId}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<CancelToolCallRequest> fromJson<CancelToolCallRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CancelToolCallRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    if (!obj["sessionId"].isString())
        return Utils::ResultError("Invalid type for field: sessionId");
    if (!obj.contains("toolCallId"))
        return Utils::ResultError("Missing required field: toolCallId");
    if (!obj["toolCallId"].isString())
        return Utils::ResultError("Invalid type for field: toolCallId");
    CancelToolCallRequest result;
    const auto res0 = fromJson<SessionId>("sessionId", obj["sessionId"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._sessionId = *res0;
    const auto res1 = fromJson<ToolCallId>("toolCallId", obj["toolCallId"]);
    if (!res1)
        return Utils::ResultError(res1.error());
    result._toolCallId = *res1;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const CancelToolCallRequest &data)
{
    QJsonObject obj{
        {"sessionId", data._sessionId},
        {"toolCallId", data._toolCallId}
    };
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
    if (!obj["sessionId"].isString())
        return Utils::ResultError("Invalid type for field: sessionId");
    CloseSessionRequest result;
    const auto res0 = fromJson<SessionId>("sessionId", obj["sessionId"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._sessionId = *res0;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
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
        return Utils::ResultError("Expected JSON object for DeleteSessionRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    if (!obj["sessionId"].isString())
        return Utils::ResultError("Invalid type for field: sessionId");
    DeleteSessionRequest result;
    const auto res0 = fromJson<SessionId>("sessionId", obj["sessionId"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._sessionId = *res0;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
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
Utils::Result<TerminalAuthCapabilities> fromJson<TerminalAuthCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TerminalAuthCapabilities");
    const QJsonObject obj = val.toObject();
    TerminalAuthCapabilities result;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const TerminalAuthCapabilities &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<ClientAuthCapabilities> fromJson<ClientAuthCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ClientAuthCapabilities");
    const QJsonObject obj = val.toObject();
    ClientAuthCapabilities result;
    // "terminal": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("terminal") && !obj["terminal"].isNull()) {
            const auto terminalRes0 = fromJson<TerminalAuthCapabilities>("terminal", obj["terminal"]);
            if (!terminalRes0)
                return Utils::ResultError(terminalRes0.error());
            result._terminal = *terminalRes0;
        }
        else if (obj.contains("terminal"))
            result._terminal = std::nullopt;
        return {};
    }();
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const ClientAuthCapabilities &data)
{
    QJsonObject obj;
    if (data._terminal.has_value())
        obj.insert("terminal", toJson(*data._terminal));
    else if (data._terminal.isNull())
        obj.insert("terminal", QJsonValue::Null);
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
        return Utils::ResultError("Expected JSON object for ElicitationCapabilities");
    const QJsonObject obj = val.toObject();
    ElicitationCapabilities result;
    // "form": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("form") && !obj["form"].isNull()) {
            const auto formRes0 = fromJson<ElicitationFormCapabilities>("form", obj["form"]);
            if (!formRes0)
                return Utils::ResultError(formRes0.error());
            result._form = *formRes0;
        }
        else if (obj.contains("form"))
            result._form = std::nullopt;
        return {};
    }();
    // "url": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("url") && !obj["url"].isNull()) {
            const auto urlRes0 = fromJson<ElicitationUrlCapabilities>("url", obj["url"]);
            if (!urlRes0)
                return Utils::ResultError(urlRes0.error());
            result._url = *urlRes0;
        }
        else if (obj.contains("url"))
            result._url = std::nullopt;
        return {};
    }();
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
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
        return Utils::ResultError("Expected JSON object for ClientCapabilities");
    const QJsonObject obj = val.toObject();
    ClientCapabilities result;
    // "auth": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("auth") && !obj["auth"].isNull()) {
            const auto authRes0 = fromJson<ClientAuthCapabilities>("auth", obj["auth"]);
            if (!authRes0)
                return Utils::ResultError(authRes0.error());
            result._auth = *authRes0;
        }
        else if (obj.contains("auth"))
            result._auth = std::nullopt;
        return {};
    }();
    // "elicitation": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("elicitation") && !obj["elicitation"].isNull()) {
            const auto elicitationRes0 = fromJson<ElicitationCapabilities>("elicitation", obj["elicitation"]);
            if (!elicitationRes0)
                return Utils::ResultError(elicitationRes0.error());
            result._elicitation = *elicitationRes0;
        }
        else if (obj.contains("elicitation"))
            result._elicitation = std::nullopt;
        return {};
    }();
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const ClientCapabilities &data)
{
    QJsonObject obj;
    if (data._auth.has_value())
        obj.insert("auth", toJson(*data._auth));
    else if (data._auth.isNull())
        obj.insert("auth", QJsonValue::Null);
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
        return Utils::ResultError("Expected JSON object for InitializeRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("protocolVersion"))
        return Utils::ResultError("Missing required field: protocolVersion");
    if (!obj["protocolVersion"].isDouble())
        return Utils::ResultError("Invalid type for field: protocolVersion");
    if (!obj.contains("info"))
        return Utils::ResultError("Missing required field: info");
    if (!obj["info"].isObject())
        return Utils::ResultError("Invalid type for field: info");
    InitializeRequest result;
    const auto res0 = fromJson<ProtocolVersion>("protocolVersion", obj["protocolVersion"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._protocolVersion = *res0;
    const auto res1 = fromJson<Implementation>("info", obj["info"]);
    if (!res1)
        return Utils::ResultError(res1.error());
    result._info = *res1;
    // "capabilities": a parse failure leaves the field at its default.
    (void) [&]() -> Utils::Result<> {
        if (obj.contains("capabilities") && obj["capabilities"].isObject()) {
            const auto capabilitiesRes0 = fromJson<ClientCapabilities>("capabilities", obj["capabilities"]);
            if (!capabilitiesRes0)
                return Utils::ResultError(capabilitiesRes0.error());
            result._capabilities = *capabilitiesRes0;
        }
        return {};
    }();
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
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
        return Utils::ResultError("Expected JSON object for ListSessionsRequest");
    const QJsonObject obj = val.toObject();
    ListSessionsRequest result;
    if (obj.contains("cwd") && !obj["cwd"].isNull()) {
        const auto res0 = fromJson<AbsolutePath>("cwd", obj["cwd"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._cwd = *res0;
    }
    else if (obj.contains("cwd"))
        result._cwd = std::nullopt;
    if (obj.contains("cursor") && !obj["cursor"].isNull()) {
        const auto res1 = fromJson<SessionListCursor>("cursor", obj["cursor"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._cursor = *res1;
    }
    else if (obj.contains("cursor"))
        result._cursor = std::nullopt;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
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
        return Utils::ResultError("Expected JSON object for LoginAuthRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("methodId"))
        return Utils::ResultError("Missing required field: methodId");
    if (!obj["methodId"].isString())
        return Utils::ResultError("Invalid type for field: methodId");
    LoginAuthRequest result;
    const auto res0 = fromJson<AuthMethodId>("methodId", obj["methodId"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._methodId = *res0;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
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
    if (!obj["name"].isString())
        return Utils::ResultError("Invalid type for field: name");
    if (!obj.contains("value"))
        return Utils::ResultError("Missing required field: value");
    if (!obj["value"].isString())
        return Utils::ResultError("Invalid type for field: value");
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
        return Utils::ResultError("Expected JSON object for McpServerHttp");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    if (!obj["name"].isString())
        return Utils::ResultError("Invalid type for field: name");
    if (!obj.contains("url"))
        return Utils::ResultError("Missing required field: url");
    if (!obj["url"].isString())
        return Utils::ResultError("Invalid type for field: url");
    McpServerHttp result;
    result._name = obj.value("name").toString();
    result._url = obj.value("url").toString();
    if (obj.contains("headers") && obj["headers"].isArray()) {
        const QJsonArray arr = obj["headers"].toArray();
        QList<HttpHeader> list_headers;
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<HttpHeader>("headers", v);
            if (!res0)
                return Utils::ResultError(res0.error());
            list_headers.append(*res0);
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
    return result;
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
Utils::Result<McpServerStdio> fromJson<McpServerStdio>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for McpServerStdio");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    if (!obj["name"].isString())
        return Utils::ResultError("Invalid type for field: name");
    if (!obj.contains("command"))
        return Utils::ResultError("Missing required field: command");
    if (!obj["command"].isString())
        return Utils::ResultError("Invalid type for field: command");
    McpServerStdio result;
    result._name = obj.value("name").toString();
    const auto res0 = fromJson<AbsolutePath>("command", obj["command"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._command = *res0;
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
            const auto res1 = fromJson<EnvVariable>("env", v);
            if (!res1)
                return Utils::ResultError(res1.error());
            list_env.append(*res1);
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
    return result;
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
        return Utils::ResultError("Invalid McpServer: expected object");
    const QString dispatchValue = val.toObject().value("type").toString();
    if (dispatchValue == "http") {
        const auto res0 = fromJson<McpServerHttp>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return McpServer(*res0);
    }
    else if (dispatchValue == "stdio") {
        const auto res1 = fromJson<McpServerStdio>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        return McpServer(*res1);
    }
    if (dispatchValue.isEmpty())
        return Utils::ResultError("Invalid McpServer: missing type");
    return McpServer(val.toObject());  // open union: preserve unknown variants raw
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

QString toString(SystemPromptMode v)
{
    switch(v) {
        case SystemPromptMode::append: return "append";
        case SystemPromptMode::override_: return "override";
    }
    return {};
}

template<>
Utils::Result<SystemPromptMode> fromJson<SystemPromptMode>(const QJsonValue &val)
{
    if (!val.isString())
        return Utils::ResultError("Expected JSON string for SystemPromptMode");
    const QString str = val.toString();
    if (str == "append") return SystemPromptMode::append;
    if (str == "override") return SystemPromptMode::override_;
    return Utils::ResultError("Invalid SystemPromptMode value: " + str);
}

QJsonValue toJsonValue(const SystemPromptMode &v)
{
    return toString(v);
}

template<>
Utils::Result<SystemPrompt> fromJson<SystemPrompt>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SystemPrompt");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("mode"))
        return Utils::ResultError("Missing required field: mode");
    if (!obj.contains("content"))
        return Utils::ResultError("Missing required field: content");
    if (!obj["content"].isArray())
        return Utils::ResultError("Invalid type for field: content");
    SystemPrompt result;
    const auto res0 = fromJson<SystemPromptMode>("mode", obj["mode"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._mode = *res0;
    {
        const QJsonArray arr = obj["content"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res1 = fromJson<ContentBlock>("content", v);
            if (!res1)
                return Utils::ResultError(res1.error());
            result._content.append(*res1);
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

QJsonObject toJson(const SystemPrompt &data)
{
    QJsonObject obj{{"mode", toJsonValue(data._mode)}};
    QJsonArray arr_content;
    for (const auto &v : data._content) arr_content.append(toJsonValue(v));
    obj.insert("content", arr_content);
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
    if (!obj["cwd"].isString())
        return Utils::ResultError("Invalid type for field: cwd");
    NewSessionRequest result;
    const auto res0 = fromJson<AbsolutePath>("cwd", obj["cwd"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._cwd = *res0;
    if (obj.contains("additionalDirectories") && obj["additionalDirectories"].isArray()) {
        const QJsonArray arr = obj["additionalDirectories"].toArray();
        QList<AbsolutePath> list_additionalDirectories;
        for (const QJsonValue &v : arr) {
            const auto res1 = fromJson<AbsolutePath>("additionalDirectories", v);
            if (!res1)
                return Utils::ResultError(res1.error());
            list_additionalDirectories.append(*res1);
        }
        result._additionalDirectories = list_additionalDirectories;
    }
    if (obj.contains("mcpServers") && obj["mcpServers"].isArray()) {
        const QJsonArray arr = obj["mcpServers"].toArray();
        QList<McpServer> list_mcpServers;
        for (const QJsonValue &v : arr) {
            const auto res2 = fromJson<McpServer>("mcpServers", v);
            if (!res2)
                return Utils::ResultError(res2.error());
            list_mcpServers.append(*res2);
        }
        result._mcpServers = list_mcpServers;
    }
    if (obj.contains("systemPrompt") && !obj["systemPrompt"].isNull()) {
        const auto res3 = fromJson<SystemPrompt>("systemPrompt", obj["systemPrompt"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._systemPrompt = *res3;
    }
    else if (obj.contains("systemPrompt"))
        result._systemPrompt = std::nullopt;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
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
    if (data._mcpServers.has_value()) {
        QJsonArray arr_mcpServers;
        for (const auto &v : *data._mcpServers) arr_mcpServers.append(toJsonValue(v));
        obj.insert("mcpServers", arr_mcpServers);
    }
    if (data._systemPrompt.has_value())
        obj.insert("systemPrompt", toJson(*data._systemPrompt));
    else if (data._systemPrompt.isNull())
        obj.insert("systemPrompt", QJsonValue::Null);
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
        return Utils::ResultError("Expected JSON object for PromptRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    if (!obj["sessionId"].isString())
        return Utils::ResultError("Invalid type for field: sessionId");
    if (!obj.contains("prompt"))
        return Utils::ResultError("Missing required field: prompt");
    if (!obj["prompt"].isArray())
        return Utils::ResultError("Invalid type for field: prompt");
    PromptRequest result;
    const auto res0 = fromJson<SessionId>("sessionId", obj["sessionId"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._sessionId = *res0;
    {
        const QJsonArray arr = obj["prompt"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res1 = fromJson<ContentBlock>("prompt", v);
            if (!res1)
                return Utils::ResultError(res1.error());
            result._prompt.append(*res1);
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
    if (!val.isObject())
        return Utils::ResultError("Invalid ReplayFrom: expected object");
    const QString dispatchValue = val.toObject().value("type").toString();
    if (dispatchValue == "start") {
        const auto res0 = fromJson<ReplayFromStart>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return ReplayFrom(*res0);
    }
    if (dispatchValue.isEmpty())
        return Utils::ResultError("Invalid ReplayFrom: missing type");
    return ReplayFrom(val.toObject());  // open union: preserve unknown variants raw
}

QString dispatchValue(const ReplayFrom &val)
{
    return std::visit([](const auto &v) -> QString {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, ReplayFromStart>) return "start";
        else if constexpr (std::is_same_v<T, QJsonObject>) return v.value("type").toString();
        return {};
    }, val);
}

QJsonObject toJson(const ReplayFrom &val)
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

QJsonValue toJsonValue(const ReplayFrom &val)
{
    return toJson(val);
}

template<>
Utils::Result<ResumeSessionRequest> fromJson<ResumeSessionRequest>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ResumeSessionRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    if (!obj["sessionId"].isString())
        return Utils::ResultError("Invalid type for field: sessionId");
    if (!obj.contains("cwd"))
        return Utils::ResultError("Missing required field: cwd");
    if (!obj["cwd"].isString())
        return Utils::ResultError("Invalid type for field: cwd");
    ResumeSessionRequest result;
    const auto res0 = fromJson<SessionId>("sessionId", obj["sessionId"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._sessionId = *res0;
    const auto res1 = fromJson<AbsolutePath>("cwd", obj["cwd"]);
    if (!res1)
        return Utils::ResultError(res1.error());
    result._cwd = *res1;
    if (obj.contains("additionalDirectories") && obj["additionalDirectories"].isArray()) {
        const QJsonArray arr = obj["additionalDirectories"].toArray();
        QList<AbsolutePath> list_additionalDirectories;
        for (const QJsonValue &v : arr) {
            const auto res2 = fromJson<AbsolutePath>("additionalDirectories", v);
            if (!res2)
                return Utils::ResultError(res2.error());
            list_additionalDirectories.append(*res2);
        }
        result._additionalDirectories = list_additionalDirectories;
    }
    if (obj.contains("mcpServers") && obj["mcpServers"].isArray()) {
        const QJsonArray arr = obj["mcpServers"].toArray();
        QList<McpServer> list_mcpServers;
        for (const QJsonValue &v : arr) {
            const auto res3 = fromJson<McpServer>("mcpServers", v);
            if (!res3)
                return Utils::ResultError(res3.error());
            list_mcpServers.append(*res3);
        }
        result._mcpServers = list_mcpServers;
    }
    if (obj.contains("replayFrom") && !obj["replayFrom"].isNull()) {
        const auto res4 = fromJson<ReplayFrom>("replayFrom", obj["replayFrom"]);
        if (!res4)
            return Utils::ResultError(res4.error());
        result._replayFrom = *res4;
    }
    else if (obj.contains("replayFrom"))
        result._replayFrom = std::nullopt;
    if (obj.contains("systemPrompt") && !obj["systemPrompt"].isNull()) {
        const auto res5 = fromJson<SystemPrompt>("systemPrompt", obj["systemPrompt"]);
        if (!res5)
            return Utils::ResultError(res5.error());
        result._systemPrompt = *res5;
    }
    else if (obj.contains("systemPrompt"))
        result._systemPrompt = std::nullopt;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
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
    if (data._replayFrom.has_value())
        obj.insert("replayFrom", toJsonValue(*data._replayFrom));
    else if (data._replayFrom.isNull())
        obj.insert("replayFrom", QJsonValue::Null);
    if (data._systemPrompt.has_value())
        obj.insert("systemPrompt", toJson(*data._systemPrompt));
    else if (data._systemPrompt.isNull())
        obj.insert("systemPrompt", QJsonValue::Null);
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
        return Utils::ResultError("Expected JSON object for SetSessionConfigOptionRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    if (!obj["sessionId"].isString())
        return Utils::ResultError("Invalid type for field: sessionId");
    if (!obj.contains("configId"))
        return Utils::ResultError("Missing required field: configId");
    if (!obj["configId"].isString())
        return Utils::ResultError("Invalid type for field: configId");
    SetSessionConfigOptionRequest result;
    const auto res0 = fromJson<SessionId>("sessionId", obj["sessionId"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._sessionId = *res0;
    const auto res1 = fromJson<SessionConfigId>("configId", obj["configId"]);
    if (!res1)
        return Utils::ResultError(res1.error());
    result._configId = *res1;
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
        return Utils::ResultError("Expected JSON object for ClientRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    if (!obj["method"].isString())
        return Utils::ResultError("Invalid type for field: method");
    ClientRequest result;
    const auto res0 = fromJson<RequestId>("id", obj["id"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._id = *res0;
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
Utils::Result<CancelledPermissionOutcome> fromJson<CancelledPermissionOutcome>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CancelledPermissionOutcome");
    const QJsonObject obj = val.toObject();
    CancelledPermissionOutcome result;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
}

QJsonObject toJson(const CancelledPermissionOutcome &data)
{
    QJsonObject obj;
    if (data.__meta.has_value())
        obj.insert("_meta", *data.__meta);
    else if (data.__meta.isNull())
        obj.insert("_meta", QJsonValue::Null);
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
    if (!obj["optionId"].isString())
        return Utils::ResultError("Invalid type for field: optionId");
    SelectedPermissionOutcome result;
    const auto res0 = fromJson<PermissionOptionId>("optionId", obj["optionId"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._optionId = *res0;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
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
        return Utils::ResultError("Invalid RequestPermissionOutcome: expected object");
    const QString dispatchValue = val.toObject().value("outcome").toString();
    if (dispatchValue == "cancelled") {
        const auto res0 = fromJson<CancelledPermissionOutcome>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return RequestPermissionOutcome(*res0);
    }
    else if (dispatchValue == "selected") {
        const auto res1 = fromJson<SelectedPermissionOutcome>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        return RequestPermissionOutcome(*res1);
    }
    if (dispatchValue.isEmpty())
        return Utils::ResultError("Invalid RequestPermissionOutcome: missing outcome");
    return RequestPermissionOutcome(val.toObject());  // open union: preserve unknown variants raw
}

QString dispatchValue(const RequestPermissionOutcome &val)
{
    return std::visit([](const auto &v) -> QString {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, CancelledPermissionOutcome>) return "cancelled";
        else if constexpr (std::is_same_v<T, SelectedPermissionOutcome>) return "selected";
        else if constexpr (std::is_same_v<T, QJsonObject>) return v.value("outcome").toString();
        return {};
    }, val);
}

QJsonObject toJson(const RequestPermissionOutcome &val)
{
    QJsonObject obj = std::visit([](const auto &v) -> QJsonObject {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
    obj.insert("outcome", dispatchValue(val));
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
    const auto res0 = fromJson<RequestPermissionOutcome>("outcome", obj["outcome"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._outcome = *res0;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
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
        return Utils::ResultError("Expected JSON object for CancelSessionNotification");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sessionId"))
        return Utils::ResultError("Missing required field: sessionId");
    if (!obj["sessionId"].isString())
        return Utils::ResultError("Invalid type for field: sessionId");
    CancelSessionNotification result;
    const auto res0 = fromJson<SessionId>("sessionId", obj["sessionId"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._sessionId = *res0;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
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
    if (!obj["method"].isString())
        return Utils::ResultError("Invalid type for field: method");
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
    const auto res0 = fromJson<RequestId>("requestId", obj["requestId"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._requestId = *res0;
    if (obj.contains("_meta")) {
        if (!obj["_meta"].isNull()) {
            result.__meta = obj.value("_meta").toObject();
        } else {
            result.__meta = std::nullopt;
        }
    }
    return result;
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

} // namespace Acp::V2
