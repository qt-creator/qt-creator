/*
 This file is auto-generated. Do not edit manually.
 Generated with:

 C:\dev\bin\Python\313\python.exe \
  scripts/generate_cpp_from_schema.py \
  src/libs/acp/schema/schema-v2.json src/libs/acp/acpv2.h --namespace Acp::V2 --cpp-output src/libs/acp/acpv2.cpp --export-macro ACPLIB_EXPORT --export-header acp_global.h --three-state
*/
#pragma once

#include "acp_global.h"

#include <utils/result.h>
#include <utils/co_result.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QMap>
#include <QSet>
#include <QString>
#include <QVariant>

#include <variant>

namespace Acp::V2 {

/**
 * Three-state field for upsert patch semantics: absent (leave unchanged),
 * null (explicit clear), or a concrete value.
 */
template<typename T>
class Patch
{
public:
    Patch() = default;
    Patch(std::nullopt_t) : m_null(true) {}
    Patch(const T &value) : m_value(value) {}

    bool isAbsent() const { return !m_null && !m_value.has_value(); }
    bool isNull() const { return m_null; }
    bool has_value() const { return m_value.has_value(); }
    const T &operator*() const { return *m_value; }
    const T *operator->() const { return &*m_value; }
    const std::optional<T> &asOptional() const { return m_value; }

    Patch &operator=(std::nullopt_t) { m_null = true; m_value.reset(); return *this; }
    Patch &operator=(const T &value) { m_null = false; m_value = value; return *this; }

private:
    bool m_null = false;
    std::optional<T> m_value;
};

template<typename T> Utils::Result<T> fromJson(const QJsonValue &val) = delete;

/**
 * JSON RPC Request Id
 *
 * An identifier established by the Client that MUST contain a String, Number, or NULL value if included. If it is not included it is assumed to be a notification. The value SHOULD normally not be Null \[1\] and Numbers SHOULD NOT contain fractional parts \[2\]
 *
 * The Server MUST reply with the same value in the Response object if included. This member is used to correlate the context between the two objects.
 *
 * \[1\] The use of Null as a value for the id member in a Request object is discouraged, because this specification uses a value of Null for Responses with an unknown id. Also, because JSON-RPC 1.0 uses an id value of Null for Notifications this could cause confusion in handling.
 *
 * \[2\] Fractional parts may be problematic, since many decimal fractions cannot be represented exactly as binary fractions.
 */
using RequestId = std::variant<std::monostate, int, QString>;

template<>
ACPLIB_EXPORT Utils::Result<RequestId> fromJson<RequestId>(const QJsonValue &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const RequestId &val);

/**
 * Request-scoped elicitation, tied to a specific JSON-RPC request outside of a session
 * (e.g., during auth/configuration phases before any session is started).
 */
struct ElicitationRequestScope {
    RequestId _requestId;  //!< The request this elicitation is tied to.

    ElicitationRequestScope& requestId(const RequestId & v) { _requestId = v; return *this; }

    const RequestId& requestId() const { return _requestId; }
};

template<>
ACPLIB_EXPORT Utils::Result<ElicitationRequestScope> fromJson<ElicitationRequestScope>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ElicitationRequestScope &data);

/** Schema for boolean properties in an elicitation form. */
struct BooleanPropertySchema {
    /**
     * Optional title for the property.
     *
     * Optional. Omitted and `null` are equivalent and mean no title is provided.
     */
    Patch<QString> _title;
    /**
     * Human-readable description.
     *
     * Optional. Omitted and `null` are equivalent and mean no description is provided.
     */
    Patch<QString> _description;
    /**
     * Default value.
     *
     * Optional. Omitted and `null` are equivalent and mean no default value is provided.
     */
    Patch<bool> _default_;
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * Optional. Omitted and `null` are equivalent and mean no metadata.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    BooleanPropertySchema& title(const Patch<QString> & v) { _title = v; return *this; }
    BooleanPropertySchema& title(const QString & v) { _title = v; return *this; }
    BooleanPropertySchema& description(const Patch<QString> & v) { _description = v; return *this; }
    BooleanPropertySchema& description(const QString & v) { _description = v; return *this; }
    BooleanPropertySchema& default_(const Patch<bool> & v) { _default_ = v; return *this; }
    BooleanPropertySchema& default_(bool v) { _default_ = v; return *this; }
    BooleanPropertySchema& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    BooleanPropertySchema& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QString>& title() const { return _title; }
    const Patch<QString>& description() const { return _description; }
    const Patch<bool>& default_() const { return _default_; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<BooleanPropertySchema> fromJson<BooleanPropertySchema>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const BooleanPropertySchema &data);

/** Schema for integer properties in an elicitation form. */
struct IntegerPropertySchema {
    /**
     * Optional title for the property.
     *
     * Optional. Omitted and `null` are equivalent and mean no title is provided.
     */
    Patch<QString> _title;
    /**
     * Human-readable description.
     *
     * Optional. Omitted and `null` are equivalent and mean no description is provided.
     */
    Patch<QString> _description;
    /**
     * Minimum value (inclusive).
     *
     * Optional. Omitted and `null` are equivalent and mean there is no inclusive lower bound.
     */
    Patch<int> _minimum;
    /**
     * Maximum value (inclusive).
     *
     * Optional. Omitted and `null` are equivalent and mean there is no inclusive upper bound.
     */
    Patch<int> _maximum;
    /**
     * Default value.
     *
     * Optional. Omitted and `null` are equivalent and mean no default value is provided.
     */
    Patch<int> _default_;
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * Optional. Omitted and `null` are equivalent and mean no metadata.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    IntegerPropertySchema& title(const Patch<QString> & v) { _title = v; return *this; }
    IntegerPropertySchema& title(const QString & v) { _title = v; return *this; }
    IntegerPropertySchema& description(const Patch<QString> & v) { _description = v; return *this; }
    IntegerPropertySchema& description(const QString & v) { _description = v; return *this; }
    IntegerPropertySchema& minimum(const Patch<int> & v) { _minimum = v; return *this; }
    IntegerPropertySchema& minimum(int v) { _minimum = v; return *this; }
    IntegerPropertySchema& maximum(const Patch<int> & v) { _maximum = v; return *this; }
    IntegerPropertySchema& maximum(int v) { _maximum = v; return *this; }
    IntegerPropertySchema& default_(const Patch<int> & v) { _default_ = v; return *this; }
    IntegerPropertySchema& default_(int v) { _default_ = v; return *this; }
    IntegerPropertySchema& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    IntegerPropertySchema& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QString>& title() const { return _title; }
    const Patch<QString>& description() const { return _description; }
    const Patch<int>& minimum() const { return _minimum; }
    const Patch<int>& maximum() const { return _maximum; }
    const Patch<int>& default_() const { return _default_; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<IntegerPropertySchema> fromJson<IntegerPropertySchema>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const IntegerPropertySchema &data);

/** String item schema for multi-select enum properties. */
struct StringMultiSelectItems {
    QStringList _enum_;  //!< Allowed enum values. Must contain at least one value.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * Optional. Omitted and `null` are equivalent and mean no metadata.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    StringMultiSelectItems& enum_(const QStringList & v) { _enum_ = v; return *this; }
    StringMultiSelectItems& addEnum(const QString & v) { _enum_.append(v); return *this; }
    StringMultiSelectItems& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    StringMultiSelectItems& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const QStringList& enum_() const { return _enum_; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<StringMultiSelectItems> fromJson<StringMultiSelectItems>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const StringMultiSelectItems &data);

/** A titled enum option with a const value, human-readable title, and optional description. */
struct EnumOption {
    QString _const_;  //!< The constant value for this option.
    QString _title;  //!< Human-readable title for this option.
    /**
     * Human-readable description.
     *
     * Optional. Omitted and `null` are equivalent and mean no description is provided.
     */
    Patch<QString> _description;
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * Optional. Omitted and `null` are equivalent and mean no metadata.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    EnumOption& const_(const QString & v) { _const_ = v; return *this; }
    EnumOption& title(const QString & v) { _title = v; return *this; }
    EnumOption& description(const Patch<QString> & v) { _description = v; return *this; }
    EnumOption& description(const QString & v) { _description = v; return *this; }
    EnumOption& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    EnumOption& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const QString& const_() const { return _const_; }
    const QString& title() const { return _title; }
    const Patch<QString>& description() const { return _description; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<EnumOption> fromJson<EnumOption>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const EnumOption &data);

/** Items definition for titled multi-select enum properties. */
struct TitledMultiSelectItems {
    QList<EnumOption> _anyOf;  //!< Titled enum options. Must contain at least one option.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * Optional. Omitted and `null` are equivalent and mean no metadata.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    TitledMultiSelectItems& anyOf(const QList<EnumOption> & v) { _anyOf = v; return *this; }
    TitledMultiSelectItems& addAnyOf(const EnumOption & v) { _anyOf.append(v); return *this; }
    TitledMultiSelectItems& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    TitledMultiSelectItems& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const QList<EnumOption>& anyOf() const { return _anyOf; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<TitledMultiSelectItems> fromJson<TitledMultiSelectItems>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const TitledMultiSelectItems &data);

/** Items for a multi-select (array) property schema. */
using MultiSelectItems = std::variant<StringMultiSelectItems, QJsonObject, TitledMultiSelectItems>;

template<>
ACPLIB_EXPORT Utils::Result<MultiSelectItems> fromJson<MultiSelectItems>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const MultiSelectItems &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const MultiSelectItems &val);

/** Schema for multi-select (array) properties in an elicitation form. */
struct MultiSelectPropertySchema {
    /**
     * Optional title for the property.
     *
     * Optional. Omitted and `null` are equivalent and mean no title is provided.
     */
    Patch<QString> _title;
    /**
     * Human-readable description.
     *
     * Optional. Omitted and `null` are equivalent and mean no description is provided.
     */
    Patch<QString> _description;
    /**
     * Minimum number of items to select.
     *
     * Optional. Omitted and `null` are equivalent and mean there is no minimum selection count.
     */
    Patch<int> _minItems;
    /**
     * Maximum number of items to select.
     *
     * Optional. Omitted and `null` are equivalent and mean there is no maximum selection count.
     */
    Patch<int> _maxItems;
    MultiSelectItems _items;  //!< The items definition describing allowed values.
    /**
     * Default selected values.
     *
     * Optional. Omitted and `null` are equivalent and mean no default selections are provided.
     */
    Patch<QJsonArray> _default_;
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * Optional. Omitted and `null` are equivalent and mean no metadata.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    MultiSelectPropertySchema& title(const Patch<QString> & v) { _title = v; return *this; }
    MultiSelectPropertySchema& title(const QString & v) { _title = v; return *this; }
    MultiSelectPropertySchema& description(const Patch<QString> & v) { _description = v; return *this; }
    MultiSelectPropertySchema& description(const QString & v) { _description = v; return *this; }
    MultiSelectPropertySchema& minItems(const Patch<int> & v) { _minItems = v; return *this; }
    MultiSelectPropertySchema& minItems(int v) { _minItems = v; return *this; }
    MultiSelectPropertySchema& maxItems(const Patch<int> & v) { _maxItems = v; return *this; }
    MultiSelectPropertySchema& maxItems(int v) { _maxItems = v; return *this; }
    MultiSelectPropertySchema& items(const MultiSelectItems & v) { _items = v; return *this; }
    MultiSelectPropertySchema& default_(const Patch<QJsonArray> & v) { _default_ = v; return *this; }
    MultiSelectPropertySchema& default_(const QJsonArray & v) { _default_ = v; return *this; }
    MultiSelectPropertySchema& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    MultiSelectPropertySchema& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QString>& title() const { return _title; }
    const Patch<QString>& description() const { return _description; }
    const Patch<int>& minItems() const { return _minItems; }
    const Patch<int>& maxItems() const { return _maxItems; }
    const MultiSelectItems& items() const { return _items; }
    const Patch<QJsonArray>& default_() const { return _default_; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<MultiSelectPropertySchema> fromJson<MultiSelectPropertySchema>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const MultiSelectPropertySchema &data);

/** Schema for number (floating-point) properties in an elicitation form. */
struct NumberPropertySchema {
    /**
     * Optional title for the property.
     *
     * Optional. Omitted and `null` are equivalent and mean no title is provided.
     */
    Patch<QString> _title;
    /**
     * Human-readable description.
     *
     * Optional. Omitted and `null` are equivalent and mean no description is provided.
     */
    Patch<QString> _description;
    /**
     * Minimum value (inclusive).
     *
     * Optional. Omitted and `null` are equivalent and mean there is no inclusive lower bound.
     */
    Patch<double> _minimum;
    /**
     * Maximum value (inclusive).
     *
     * Optional. Omitted and `null` are equivalent and mean there is no inclusive upper bound.
     */
    Patch<double> _maximum;
    /**
     * Default value.
     *
     * Optional. Omitted and `null` are equivalent and mean no default value is provided.
     */
    Patch<double> _default_;
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * Optional. Omitted and `null` are equivalent and mean no metadata.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    NumberPropertySchema& title(const Patch<QString> & v) { _title = v; return *this; }
    NumberPropertySchema& title(const QString & v) { _title = v; return *this; }
    NumberPropertySchema& description(const Patch<QString> & v) { _description = v; return *this; }
    NumberPropertySchema& description(const QString & v) { _description = v; return *this; }
    NumberPropertySchema& minimum(const Patch<double> & v) { _minimum = v; return *this; }
    NumberPropertySchema& minimum(double v) { _minimum = v; return *this; }
    NumberPropertySchema& maximum(const Patch<double> & v) { _maximum = v; return *this; }
    NumberPropertySchema& maximum(double v) { _maximum = v; return *this; }
    NumberPropertySchema& default_(const Patch<double> & v) { _default_ = v; return *this; }
    NumberPropertySchema& default_(double v) { _default_ = v; return *this; }
    NumberPropertySchema& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    NumberPropertySchema& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QString>& title() const { return _title; }
    const Patch<QString>& description() const { return _description; }
    const Patch<double>& minimum() const { return _minimum; }
    const Patch<double>& maximum() const { return _maximum; }
    const Patch<double>& default_() const { return _default_; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<NumberPropertySchema> fromJson<NumberPropertySchema>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const NumberPropertySchema &data);

/** String format types for string properties in elicitation schemas. */
enum class StringFormat {
    email,
    uri,
    date,
    dateminustime
};

ACPLIB_EXPORT QString toString(StringFormat v);

template<>
ACPLIB_EXPORT Utils::Result<StringFormat> fromJson<StringFormat>(const QJsonValue &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const StringFormat &v);

/**
 * Schema for string properties in an elicitation form.
 *
 * When `enum` or `oneOf` is set, this represents a single-select enum
 * with `"type": "string"`.
 */
struct StringPropertySchema {
    /**
     * Optional title for the property.
     *
     * Optional. Omitted and `null` are equivalent and mean no title is provided.
     */
    Patch<QString> _title;
    /**
     * Human-readable description.
     *
     * Optional. Omitted and `null` are equivalent and mean no description is provided.
     */
    Patch<QString> _description;
    /**
     * Minimum string length.
     *
     * Optional. Omitted and `null` are equivalent and mean there is no minimum length constraint.
     */
    Patch<int> _minLength;
    /**
     * Maximum string length.
     *
     * Optional. Omitted and `null` are equivalent and mean there is no maximum length constraint.
     */
    Patch<int> _maxLength;
    /**
     * Pattern the string must match.
     *
     * Optional. Omitted and `null` are equivalent and mean there is no pattern constraint.
     */
    Patch<QString> _pattern;
    /**
     * String format.
     *
     * Optional. Omitted and `null` are equivalent and mean there is no format constraint.
     */
    Patch<StringFormat> _format;
    /**
     * Default value.
     *
     * Optional. Omitted and `null` are equivalent and mean no default value is provided.
     */
    Patch<QString> _default_;
    /**
     * Enum values for untitled single-select enums.
     * Must contain at least one value when present.
     * Optional. Omitted and `null` are equivalent and mean no untitled single-select choices are
     * declared by `enum`.
     */
    Patch<QJsonArray> _enum_;
    /**
     * Titled enum options for titled single-select enums.
     * Must contain at least one option when present.
     * Optional. Omitted and `null` are equivalent and mean no titled single-select choices are
     * declared by `oneOf`.
     */
    Patch<QList<EnumOption>> _oneOf;
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * Optional. Omitted and `null` are equivalent and mean no metadata.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    StringPropertySchema& title(const Patch<QString> & v) { _title = v; return *this; }
    StringPropertySchema& title(const QString & v) { _title = v; return *this; }
    StringPropertySchema& description(const Patch<QString> & v) { _description = v; return *this; }
    StringPropertySchema& description(const QString & v) { _description = v; return *this; }
    StringPropertySchema& minLength(const Patch<int> & v) { _minLength = v; return *this; }
    StringPropertySchema& minLength(int v) { _minLength = v; return *this; }
    StringPropertySchema& maxLength(const Patch<int> & v) { _maxLength = v; return *this; }
    StringPropertySchema& maxLength(int v) { _maxLength = v; return *this; }
    StringPropertySchema& pattern(const Patch<QString> & v) { _pattern = v; return *this; }
    StringPropertySchema& pattern(const QString & v) { _pattern = v; return *this; }
    StringPropertySchema& format(const Patch<StringFormat> & v) { _format = v; return *this; }
    StringPropertySchema& format(const StringFormat & v) { _format = v; return *this; }
    StringPropertySchema& default_(const Patch<QString> & v) { _default_ = v; return *this; }
    StringPropertySchema& default_(const QString & v) { _default_ = v; return *this; }
    StringPropertySchema& enum_(const Patch<QJsonArray> & v) { _enum_ = v; return *this; }
    StringPropertySchema& enum_(const QJsonArray & v) { _enum_ = v; return *this; }
    StringPropertySchema& oneOf(const Patch<QList<EnumOption>> & v) { _oneOf = v; return *this; }
    StringPropertySchema& oneOf(const QList<EnumOption> & v) { _oneOf = v; return *this; }
    StringPropertySchema& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    StringPropertySchema& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QString>& title() const { return _title; }
    const Patch<QString>& description() const { return _description; }
    const Patch<int>& minLength() const { return _minLength; }
    const Patch<int>& maxLength() const { return _maxLength; }
    const Patch<QString>& pattern() const { return _pattern; }
    const Patch<StringFormat>& format() const { return _format; }
    const Patch<QString>& default_() const { return _default_; }
    const Patch<QJsonArray>& enum_() const { return _enum_; }
    const Patch<QList<EnumOption>>& oneOf() const { return _oneOf; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<StringPropertySchema> fromJson<StringPropertySchema>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const StringPropertySchema &data);

/**
 * Property schema for elicitation form fields.
 *
 * Each variant corresponds to a JSON Schema `"type"` value.
 * Single-select enums use the `String` variant with `enum` or `oneOf` set.
 * Multi-select enums use the `Array` variant.
 */
using ElicitationPropertySchema = std::variant<StringPropertySchema, NumberPropertySchema, IntegerPropertySchema, BooleanPropertySchema, MultiSelectPropertySchema, QJsonObject>;

template<>
ACPLIB_EXPORT Utils::Result<ElicitationPropertySchema> fromJson<ElicitationPropertySchema>(const QJsonValue &val);

/** Returns the 'type' dispatch field value for the active variant. */
ACPLIB_EXPORT QString dispatchValue(const ElicitationPropertySchema &val);

ACPLIB_EXPORT QJsonObject toJson(const ElicitationPropertySchema &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const ElicitationPropertySchema &val);

/** Type discriminator for elicitation schemas. */
enum class ElicitationSchemaType {
    object
};

ACPLIB_EXPORT QString toString(ElicitationSchemaType v);

template<>
ACPLIB_EXPORT Utils::Result<ElicitationSchemaType> fromJson<ElicitationSchemaType>(const QJsonValue &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const ElicitationSchemaType &v);

/**
 * Type-safe elicitation schema for requesting structured user input.
 *
 * This represents a JSON Schema object with primitive-typed properties,
 * as required by the elicitation specification.
 */
struct ElicitationSchema {
    std::optional<ElicitationSchemaType> _type;  //!< Type discriminator. Always `"object"`.
    /**
     * Optional title for the schema.
     *
     * Optional. Omitted and `null` are equivalent and mean no title is provided.
     */
    Patch<QString> _title;
    std::optional<QMap<QString, ElicitationPropertySchema>> _properties;  //!< Property definitions (must be primitive types).
    /**
     * List of required property names.
     *
     * Optional. Omitted and `null` are equivalent and mean no property names are required.
     */
    Patch<QJsonArray> _required;
    /**
     * Optional description of what this schema represents.
     *
     * Optional. Omitted and `null` are equivalent and mean no schema description is provided.
     */
    Patch<QString> _description;
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * Optional. Omitted and `null` are equivalent and mean no metadata.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    ElicitationSchema& type(const std::optional<ElicitationSchemaType> & v) { _type = v; return *this; }
    ElicitationSchema& title(const Patch<QString> & v) { _title = v; return *this; }
    ElicitationSchema& title(const QString & v) { _title = v; return *this; }
    ElicitationSchema& properties(const std::optional<QMap<QString, ElicitationPropertySchema>> & v) { _properties = v; return *this; }
    ElicitationSchema& addProperty(const QString &key, const ElicitationPropertySchema & v) { if (!_properties) _properties = QMap<QString, ElicitationPropertySchema>{}; (*_properties)[key] = v; return *this; }
    ElicitationSchema& required(const Patch<QJsonArray> & v) { _required = v; return *this; }
    ElicitationSchema& required(const QJsonArray & v) { _required = v; return *this; }
    ElicitationSchema& description(const Patch<QString> & v) { _description = v; return *this; }
    ElicitationSchema& description(const QString & v) { _description = v; return *this; }
    ElicitationSchema& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    ElicitationSchema& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const std::optional<ElicitationSchemaType>& type() const { return _type; }
    const Patch<QString>& title() const { return _title; }
    const std::optional<QMap<QString, ElicitationPropertySchema>>& properties() const { return _properties; }
    const Patch<QJsonArray>& required() const { return _required; }
    const Patch<QString>& description() const { return _description; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<ElicitationSchema> fromJson<ElicitationSchema>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ElicitationSchema &data);

using SessionId = QString;
template<>
ACPLIB_EXPORT Utils::Result<SessionId> fromJson<SessionId>(const QJsonValue &val);

using ToolCallId = QString;

/**
 * Session-scoped elicitation, optionally tied to a specific tool call.
 *
 * When `tool_call_id` is set, the elicitation is tied to a specific tool call.
 * This is useful when an agent receives an elicitation from an MCP server
 * during a tool call and needs to redirect it to the user.
 */
struct ElicitationSessionScope {
    SessionId _sessionId;  //!< The session this elicitation is tied to.
    /**
     * Optional tool call within the session.
     *
     * Optional. Omitted and `null` are equivalent and mean the elicitation is scoped to the
     * session without a specific tool call.
     */
    Patch<ToolCallId> _toolCallId;

    ElicitationSessionScope& sessionId(const SessionId & v) { _sessionId = v; return *this; }
    ElicitationSessionScope& toolCallId(const Patch<ToolCallId> & v) { _toolCallId = v; return *this; }
    ElicitationSessionScope& toolCallId(const ToolCallId & v) { _toolCallId = v; return *this; }

    const SessionId& sessionId() const { return _sessionId; }
    const Patch<ToolCallId>& toolCallId() const { return _toolCallId; }
};

template<>
ACPLIB_EXPORT Utils::Result<ElicitationSessionScope> fromJson<ElicitationSessionScope>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ElicitationSessionScope &data);

/** Form-based elicitation mode where the client renders a form from the provided schema. */
struct ElicitationFormMode {
    ElicitationSchema _requestedSchema;  //!< A JSON Schema describing the form fields to present to the user.
    QJsonObject _additionalProperties;  //!< additional properties

    ElicitationFormMode& requestedSchema(const ElicitationSchema & v) { _requestedSchema = v; return *this; }
    ElicitationFormMode& additionalProperties(const QString &key, const QJsonValue &v) { _additionalProperties.insert(key, v); return *this; }
    ElicitationFormMode& additionalProperties(const QJsonObject &obj) { for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) _additionalProperties.insert(it.key(), it.value()); return *this; }

    const ElicitationSchema& requestedSchema() const { return _requestedSchema; }
    const QJsonObject& additionalProperties() const { return _additionalProperties; }
};

template<>
ACPLIB_EXPORT Utils::Result<ElicitationFormMode> fromJson<ElicitationFormMode>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ElicitationFormMode &data);

using ElicitationId = QString;

/** URL-based elicitation mode where the client directs the user to a URL. */
struct ElicitationUrlMode {
    ElicitationId _elicitationId;  //!< The unique identifier for this elicitation.
    QString _url;  //!< The URL to direct the user to.
    QJsonObject _additionalProperties;  //!< additional properties

    ElicitationUrlMode& elicitationId(const ElicitationId & v) { _elicitationId = v; return *this; }
    ElicitationUrlMode& url(const QString & v) { _url = v; return *this; }
    ElicitationUrlMode& additionalProperties(const QString &key, const QJsonValue &v) { _additionalProperties.insert(key, v); return *this; }
    ElicitationUrlMode& additionalProperties(const QJsonObject &obj) { for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) _additionalProperties.insert(it.key(), it.value()); return *this; }

    const ElicitationId& elicitationId() const { return _elicitationId; }
    const QString& url() const { return _url; }
    const QJsonObject& additionalProperties() const { return _additionalProperties; }
};

template<>
ACPLIB_EXPORT Utils::Result<ElicitationUrlMode> fromJson<ElicitationUrlMode>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ElicitationUrlMode &data);

/**
 * Request from the agent to elicit structured user input.
 *
 * The agent sends this to the client to request information from the user,
 * either via a form or by directing them to a URL.
 * Elicitations are tied to a session (optionally a tool call) or a request.
 */
struct CreateElicitationRequest {
    QString _message;  //!< A human-readable message describing what input is needed.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * Optional. Omitted and `null` are equivalent and mean no metadata.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;
    QJsonObject _additionalProperties;  //!< additional properties

    CreateElicitationRequest& message(const QString & v) { _message = v; return *this; }
    CreateElicitationRequest& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    CreateElicitationRequest& _meta(const QJsonObject & v) { __meta = v; return *this; }
    CreateElicitationRequest& additionalProperties(const QString &key, const QJsonValue &v) { _additionalProperties.insert(key, v); return *this; }
    CreateElicitationRequest& additionalProperties(const QJsonObject &obj) { for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) _additionalProperties.insert(it.key(), it.value()); return *this; }

    const QString& message() const { return _message; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
    const QJsonObject& additionalProperties() const { return _additionalProperties; }
};

template<>
ACPLIB_EXPORT Utils::Result<CreateElicitationRequest> fromJson<CreateElicitationRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const CreateElicitationRequest &data);

// Skipped unknown type alias: ExtRequest

using PermissionOptionId = QString;

/**
 * The type of permission option being presented to the user.
 *
 * Helps clients choose appropriate icons and UI treatment.
 */
enum class PermissionOptionKind {
    allow_once,
    allow_always,
    reject_once,
    reject_always
};

ACPLIB_EXPORT QString toString(PermissionOptionKind v);

template<>
ACPLIB_EXPORT Utils::Result<PermissionOptionKind> fromJson<PermissionOptionKind>(const QJsonValue &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const PermissionOptionKind &v);

/** An option presented to the user when requesting permission. */
struct PermissionOption {
    PermissionOptionId _optionId;  //!< Unique identifier for this permission option.
    QString _name;  //!< Human-readable label to display to the user.
    PermissionOptionKind _kind;  //!< Hint about the nature of this permission option.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    PermissionOption& optionId(const PermissionOptionId & v) { _optionId = v; return *this; }
    PermissionOption& name(const QString & v) { _name = v; return *this; }
    PermissionOption& kind(const PermissionOptionKind & v) { _kind = v; return *this; }
    PermissionOption& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    PermissionOption& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const PermissionOptionId& optionId() const { return _optionId; }
    const QString& name() const { return _name; }
    const PermissionOptionKind& kind() const { return _kind; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<PermissionOption> fromJson<PermissionOption>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const PermissionOption &data);

using AbsolutePath = QString;

using TerminalId = QString;

/** Permission request details for a command. */
struct CommandPermissionSubject {
    QString _command;  //!< The command that would be run if permission is granted.
    AbsolutePath _cwd;  //!< The absolute working directory for the command.
    Patch<ToolCallId> _toolCallId;  //!< The associated tool call, when known. Omitted and `null` are equivalent.
    Patch<TerminalId> _terminalId;  //!< The associated terminal, when already known. Omitted and `null` are equivalent.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys. Omitted and `null` are equivalent and mean no subject metadata was provided.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    CommandPermissionSubject& command(const QString & v) { _command = v; return *this; }
    CommandPermissionSubject& cwd(const AbsolutePath & v) { _cwd = v; return *this; }
    CommandPermissionSubject& toolCallId(const Patch<ToolCallId> & v) { _toolCallId = v; return *this; }
    CommandPermissionSubject& toolCallId(const ToolCallId & v) { _toolCallId = v; return *this; }
    CommandPermissionSubject& terminalId(const Patch<TerminalId> & v) { _terminalId = v; return *this; }
    CommandPermissionSubject& terminalId(const TerminalId & v) { _terminalId = v; return *this; }
    CommandPermissionSubject& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    CommandPermissionSubject& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const QString& command() const { return _command; }
    const AbsolutePath& cwd() const { return _cwd; }
    const Patch<ToolCallId>& toolCallId() const { return _toolCallId; }
    const Patch<TerminalId>& terminalId() const { return _terminalId; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<CommandPermissionSubject> fromJson<CommandPermissionSubject>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const CommandPermissionSubject &data);

/** The sender or recipient of messages and data in a conversation. */
enum class Role {
    assistant,
    user
};

ACPLIB_EXPORT QString toString(Role v);

template<>
ACPLIB_EXPORT Utils::Result<Role> fromJson<Role>(const QJsonValue &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const Role &v);

/**
 * Optional annotations for the client. The client can use annotations to inform how objects are used or displayed
 */
struct Annotations {
    Patch<QList<Role>> _audience;  //!< Intended recipients for this content, such as the user or assistant.
    /**
     * Timestamp indicating when the underlying resource was last modified.
     *
     * Must be an RFC 3339 formatted string (e.g., "2025-01-12T15:00:58Z").
     */
    Patch<QString> _lastModified;
    Patch<double> _priority;  //!< Relative importance of this content when clients choose what to surface.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    Annotations& audience(const Patch<QList<Role>> & v) { _audience = v; return *this; }
    Annotations& audience(const QList<Role> & v) { _audience = v; return *this; }
    Annotations& lastModified(const Patch<QString> & v) { _lastModified = v; return *this; }
    Annotations& lastModified(const QString & v) { _lastModified = v; return *this; }
    Annotations& priority(const Patch<double> & v) { _priority = v; return *this; }
    Annotations& priority(double v) { _priority = v; return *this; }
    Annotations& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    Annotations& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QList<Role>>& audience() const { return _audience; }
    const Patch<QString>& lastModified() const { return _lastModified; }
    const Patch<double>& priority() const { return _priority; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<Annotations> fromJson<Annotations>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const Annotations &data);

using MediaType = QString;

/** Audio provided to or from an LLM. */
struct AudioContent {
    QString _data;  //!< Base64-encoded media payload.
    MediaType _mimeType;  //!< MIME type describing the encoded media payload.
    Patch<Annotations> _annotations;  //!< Optional annotations that help clients decide how to display or route this content.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    AudioContent& data(const QString & v) { _data = v; return *this; }
    AudioContent& mimeType(const MediaType & v) { _mimeType = v; return *this; }
    AudioContent& annotations(const Patch<Annotations> & v) { _annotations = v; return *this; }
    AudioContent& annotations(const Annotations & v) { _annotations = v; return *this; }
    AudioContent& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    AudioContent& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const QString& data() const { return _data; }
    const MediaType& mimeType() const { return _mimeType; }
    const Patch<Annotations>& annotations() const { return _annotations; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<AudioContent> fromJson<AudioContent>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const AudioContent &data);

/** Binary resource contents. */
struct BlobResourceContents {
    QString _blob;  //!< Base64-encoded bytes for a binary resource payload.
    QString _uri;  //!< URI associated with this resource or media payload.
    Patch<MediaType> _mimeType;  //!< MIME type describing the encoded media payload.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    BlobResourceContents& blob(const QString & v) { _blob = v; return *this; }
    BlobResourceContents& uri(const QString & v) { _uri = v; return *this; }
    BlobResourceContents& mimeType(const Patch<MediaType> & v) { _mimeType = v; return *this; }
    BlobResourceContents& mimeType(const MediaType & v) { _mimeType = v; return *this; }
    BlobResourceContents& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    BlobResourceContents& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const QString& blob() const { return _blob; }
    const QString& uri() const { return _uri; }
    const Patch<MediaType>& mimeType() const { return _mimeType; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<BlobResourceContents> fromJson<BlobResourceContents>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const BlobResourceContents &data);

/** Text-based resource contents. */
struct TextResourceContents {
    QString _text;  //!< Text payload carried by this content block.
    QString _uri;  //!< URI associated with this resource or media payload.
    Patch<MediaType> _mimeType;  //!< MIME type describing the encoded media payload.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    TextResourceContents& text(const QString & v) { _text = v; return *this; }
    TextResourceContents& uri(const QString & v) { _uri = v; return *this; }
    TextResourceContents& mimeType(const Patch<MediaType> & v) { _mimeType = v; return *this; }
    TextResourceContents& mimeType(const MediaType & v) { _mimeType = v; return *this; }
    TextResourceContents& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    TextResourceContents& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const QString& text() const { return _text; }
    const QString& uri() const { return _uri; }
    const Patch<MediaType>& mimeType() const { return _mimeType; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<TextResourceContents> fromJson<TextResourceContents>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const TextResourceContents &data);

/** Resource content that can be embedded in a message. */
using EmbeddedResourceResource = std::variant<TextResourceContents, BlobResourceContents>;

template<>
ACPLIB_EXPORT Utils::Result<EmbeddedResourceResource> fromJson<EmbeddedResourceResource>(const QJsonValue &val);

/** Returns the 'uri' field from the active variant. */
ACPLIB_EXPORT QString uri(const EmbeddedResourceResource &val);

ACPLIB_EXPORT QJsonObject toJson(const EmbeddedResourceResource &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const EmbeddedResourceResource &val);

/** The contents of a resource, embedded into a prompt or tool call result. */
struct EmbeddedResource {
    EmbeddedResourceResource _resource;  //!< Embedded resource payload, either text or binary data.
    Patch<Annotations> _annotations;  //!< Optional annotations that help clients decide how to display or route this content.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    EmbeddedResource& resource(const EmbeddedResourceResource & v) { _resource = v; return *this; }
    EmbeddedResource& annotations(const Patch<Annotations> & v) { _annotations = v; return *this; }
    EmbeddedResource& annotations(const Annotations & v) { _annotations = v; return *this; }
    EmbeddedResource& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    EmbeddedResource& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const EmbeddedResourceResource& resource() const { return _resource; }
    const Patch<Annotations>& annotations() const { return _annotations; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<EmbeddedResource> fromJson<EmbeddedResource>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const EmbeddedResource &data);

/** An image provided to or from an LLM. */
struct ImageContent {
    QString _data;  //!< Base64-encoded media payload.
    MediaType _mimeType;  //!< MIME type describing the encoded media payload.
    Patch<QString> _uri;  //!< URI associated with this resource or media payload.
    Patch<Annotations> _annotations;  //!< Optional annotations that help clients decide how to display or route this content.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    ImageContent& data(const QString & v) { _data = v; return *this; }
    ImageContent& mimeType(const MediaType & v) { _mimeType = v; return *this; }
    ImageContent& uri(const Patch<QString> & v) { _uri = v; return *this; }
    ImageContent& uri(const QString & v) { _uri = v; return *this; }
    ImageContent& annotations(const Patch<Annotations> & v) { _annotations = v; return *this; }
    ImageContent& annotations(const Annotations & v) { _annotations = v; return *this; }
    ImageContent& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    ImageContent& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const QString& data() const { return _data; }
    const MediaType& mimeType() const { return _mimeType; }
    const Patch<QString>& uri() const { return _uri; }
    const Patch<Annotations>& annotations() const { return _annotations; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<ImageContent> fromJson<ImageContent>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ImageContent &data);

/** Theme an icon is designed for. */
enum class IconTheme {
    light,
    dark
};

ACPLIB_EXPORT QString toString(IconTheme v);

template<>
ACPLIB_EXPORT Utils::Result<IconTheme> fromJson<IconTheme>(const QJsonValue &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const IconTheme &v);

/** An optionally-sized icon that can be displayed in a user interface. */
struct Icon {
    QString _src;  //!< A standard URI pointing to an icon resource.
    Patch<MediaType> _mimeType;  //!< Optional MIME type override if the source MIME type is missing or generic.
    /**
     * Optional array of strings that specify sizes at which the icon can be used.
     * Each string should be in `WxH` format (e.g., `"48x48"`, `"96x96"`) or
     * `"any"` for scalable formats like SVG.
     *
     * If not provided, the client should assume that the icon can be used at any size.
     */
    Patch<QJsonArray> _sizes;
    Patch<IconTheme> _theme;  //!< Optional theme this icon is designed for.

    Icon& src(const QString & v) { _src = v; return *this; }
    Icon& mimeType(const Patch<MediaType> & v) { _mimeType = v; return *this; }
    Icon& mimeType(const MediaType & v) { _mimeType = v; return *this; }
    Icon& sizes(const Patch<QJsonArray> & v) { _sizes = v; return *this; }
    Icon& sizes(const QJsonArray & v) { _sizes = v; return *this; }
    Icon& theme(const Patch<IconTheme> & v) { _theme = v; return *this; }
    Icon& theme(const IconTheme & v) { _theme = v; return *this; }

    const QString& src() const { return _src; }
    const Patch<MediaType>& mimeType() const { return _mimeType; }
    const Patch<QJsonArray>& sizes() const { return _sizes; }
    const Patch<IconTheme>& theme() const { return _theme; }
};

template<>
ACPLIB_EXPORT Utils::Result<Icon> fromJson<Icon>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const Icon &data);

/** A resource that the server is capable of reading, included in a prompt or tool call result. */
struct ResourceLink {
    QString _name;  //!< Human-readable name shown for this protocol object.
    QString _uri;  //!< URI associated with this resource or media payload.
    Patch<QString> _title;  //!< Optional display title for end-user UI.
    Patch<QString> _description;  //!< Optional human-readable details shown with this protocol object.
    Patch<QList<Icon>> _icons;  //!< Optional set of sized icons that the client can display in a user interface.
    Patch<MediaType> _mimeType;  //!< MIME type describing the encoded media payload.
    Patch<int> _size;  //!< Optional size of the linked resource in bytes, if known.
    Patch<Annotations> _annotations;  //!< Optional annotations that help clients decide how to display or route this content.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    ResourceLink& name(const QString & v) { _name = v; return *this; }
    ResourceLink& uri(const QString & v) { _uri = v; return *this; }
    ResourceLink& title(const Patch<QString> & v) { _title = v; return *this; }
    ResourceLink& title(const QString & v) { _title = v; return *this; }
    ResourceLink& description(const Patch<QString> & v) { _description = v; return *this; }
    ResourceLink& description(const QString & v) { _description = v; return *this; }
    ResourceLink& icons(const Patch<QList<Icon>> & v) { _icons = v; return *this; }
    ResourceLink& icons(const QList<Icon> & v) { _icons = v; return *this; }
    ResourceLink& mimeType(const Patch<MediaType> & v) { _mimeType = v; return *this; }
    ResourceLink& mimeType(const MediaType & v) { _mimeType = v; return *this; }
    ResourceLink& size(const Patch<int> & v) { _size = v; return *this; }
    ResourceLink& size(int v) { _size = v; return *this; }
    ResourceLink& annotations(const Patch<Annotations> & v) { _annotations = v; return *this; }
    ResourceLink& annotations(const Annotations & v) { _annotations = v; return *this; }
    ResourceLink& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    ResourceLink& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const QString& name() const { return _name; }
    const QString& uri() const { return _uri; }
    const Patch<QString>& title() const { return _title; }
    const Patch<QString>& description() const { return _description; }
    const Patch<QList<Icon>>& icons() const { return _icons; }
    const Patch<MediaType>& mimeType() const { return _mimeType; }
    const Patch<int>& size() const { return _size; }
    const Patch<Annotations>& annotations() const { return _annotations; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<ResourceLink> fromJson<ResourceLink>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ResourceLink &data);

/** Text provided to or from an LLM. */
struct TextContent {
    QString _text;  //!< Text payload carried by this content block.
    Patch<Annotations> _annotations;  //!< Optional annotations that help clients decide how to display or route this content.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    TextContent& text(const QString & v) { _text = v; return *this; }
    TextContent& annotations(const Patch<Annotations> & v) { _annotations = v; return *this; }
    TextContent& annotations(const Annotations & v) { _annotations = v; return *this; }
    TextContent& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    TextContent& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const QString& text() const { return _text; }
    const Patch<Annotations>& annotations() const { return _annotations; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<TextContent> fromJson<TextContent>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const TextContent &data);

/**
 * Content blocks represent displayable information in the Agent Client Protocol.
 *
 * They provide a structured way to handle various types of user-facing content—whether
 * it's text from language models, images for analysis, or embedded resources for context.
 *
 * Content blocks appear in:
 * - User prompts sent via `session/prompt`
 * - Language model output reported through `session/update` notifications as
 * message updates or streamed chunks
 * - Progress updates and results from tool calls
 *
 * This structure is compatible with the Model Context Protocol (MCP), enabling
 * agents to seamlessly forward content from MCP tool outputs without transformation.
 *
 * See protocol docs: [Content](https://agentclientprotocol.com/protocol/v2/content)
 */
using ContentBlock = std::variant<TextContent, ImageContent, AudioContent, ResourceLink, EmbeddedResource, QJsonObject>;

template<>
ACPLIB_EXPORT Utils::Result<ContentBlock> fromJson<ContentBlock>(const QJsonValue &val);

/** Returns the 'type' dispatch field value for the active variant. */
ACPLIB_EXPORT QString dispatchValue(const ContentBlock &val);

ACPLIB_EXPORT QJsonObject toJson(const ContentBlock &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const ContentBlock &val);

/** Standard content block (text, images, resources). */
struct Content {
    ContentBlock _content;  //!< The actual content block.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    Content& content(const ContentBlock & v) { _content = v; return *this; }
    Content& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    Content& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const ContentBlock& content() const { return _content; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<Content> fromJson<Content>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const Content &data);

/** Kind of file content represented by a diff change. */
enum class DiffFileType {
    text,
    binary,
    directory,
    symlink
};

ACPLIB_EXPORT QString toString(DiffFileType v);

template<>
ACPLIB_EXPORT Utils::Result<DiffFileType> fromJson<DiffFileType>(const QJsonValue &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const DiffFileType &v);

/** Operation metadata for add, delete, and modify changes. */
struct DiffPathChange {
    AbsolutePath _path;  //!< Absolute path for the operation.

    DiffPathChange& path(const AbsolutePath & v) { _path = v; return *this; }

    const AbsolutePath& path() const { return _path; }
};

template<>
ACPLIB_EXPORT Utils::Result<DiffPathChange> fromJson<DiffPathChange>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const DiffPathChange &data);

/** Operation metadata for move and copy changes. */
struct DiffPathPairChange {
    AbsolutePath _oldPath;  //!< Absolute path before the operation.
    AbsolutePath _path;  //!< Absolute path after the operation.

    DiffPathPairChange& oldPath(const AbsolutePath & v) { _oldPath = v; return *this; }
    DiffPathPairChange& path(const AbsolutePath & v) { _path = v; return *this; }

    const AbsolutePath& oldPath() const { return _oldPath; }
    const AbsolutePath& path() const { return _path; }
};

template<>
ACPLIB_EXPORT Utils::Result<DiffPathPairChange> fromJson<DiffPathPairChange>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const DiffPathPairChange &data);

/**
 * One file-level change described by a [`Diff`].
 *
 * Structured change metadata lets clients identify affected files and
 * operations without parsing the text patch.
 */
struct DiffChange {
    /**
     * File content kind.
     *
     * Omitted or `null` means the content kind is unknown.
     */
    Patch<DiffFileType> _fileType;
    /**
     * MIME type of the file contents.
     *
     * Omitted or `null` means the MIME type is unknown.
     */
    Patch<MediaType> _mimeType;
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;
    QJsonObject _additionalProperties;  //!< additional properties

    DiffChange& fileType(const Patch<DiffFileType> & v) { _fileType = v; return *this; }
    DiffChange& fileType(const DiffFileType & v) { _fileType = v; return *this; }
    DiffChange& mimeType(const Patch<MediaType> & v) { _mimeType = v; return *this; }
    DiffChange& mimeType(const MediaType & v) { _mimeType = v; return *this; }
    DiffChange& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    DiffChange& _meta(const QJsonObject & v) { __meta = v; return *this; }
    DiffChange& additionalProperties(const QString &key, const QJsonValue &v) { _additionalProperties.insert(key, v); return *this; }
    DiffChange& additionalProperties(const QJsonObject &obj) { for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) _additionalProperties.insert(it.key(), it.value()); return *this; }

    const Patch<DiffFileType>& fileType() const { return _fileType; }
    const Patch<MediaType>& mimeType() const { return _mimeType; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
    const QJsonObject& additionalProperties() const { return _additionalProperties; }
};

template<>
ACPLIB_EXPORT Utils::Result<DiffChange> fromJson<DiffChange>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const DiffChange &data);

/** Text patch format used by [`DiffPatch`]. */
enum class DiffPatchFormat {
    git_patch
};

ACPLIB_EXPORT QString toString(DiffPatchFormat v);

template<>
ACPLIB_EXPORT Utils::Result<DiffPatchFormat> fromJson<DiffPatchFormat>(const QJsonValue &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const DiffPatchFormat &v);

/** Renderable patch text and its format. */
struct DiffPatch {
    DiffPatchFormat _format;  //!< Patch format. The only ACP-defined value is `git_patch`.
    QString _text;  //!< Patch text in the format named by `format`.

    DiffPatch& format(const DiffPatchFormat & v) { _format = v; return *this; }
    DiffPatch& text(const QString & v) { _text = v; return *this; }

    const DiffPatchFormat& format() const { return _format; }
    const QString& text() const { return _text; }
};

template<>
ACPLIB_EXPORT Utils::Result<DiffPatch> fromJson<DiffPatch>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const DiffPatch &data);

/**
 * File changes produced by a tool call.
 *
 * `changes` is authoritative for affected absolute paths and operations.
 * `patch` optionally carries renderable text for some or all of those changes
 * and MUST be consistent with `changes`. Agents SHOULD provide `patch` whenever
 * feasible. Clients MUST handle diffs where `patch` is omitted or `null`.
 *
 * See protocol docs: [Content](https://agentclientprotocol.com/protocol/v2/tool-calls#content)
 */
struct Diff {
    /**
     * Structured file changes described by this diff.
     *
     * Clients can use this field without parsing patch text to determine affected paths.
     */
    QList<DiffChange> _changes;
    /**
     * Renderable patch text for some or all of the structured changes.
     *
     * Agents SHOULD provide patch text whenever feasible. Omitted or `null`
     * means no renderable patch text was provided.
     */
    Patch<DiffPatch> _patch;
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    Diff& changes(const QList<DiffChange> & v) { _changes = v; return *this; }
    Diff& addChange(const DiffChange & v) { _changes.append(v); return *this; }
    Diff& patch(const Patch<DiffPatch> & v) { _patch = v; return *this; }
    Diff& patch(const DiffPatch & v) { _patch = v; return *this; }
    Diff& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    Diff& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const QList<DiffChange>& changes() const { return _changes; }
    const Patch<DiffPatch>& patch() const { return _patch; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<Diff> fromJson<Diff>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const Diff &data);

/**
 * A display-only reference to an agent-owned terminal.
 *
 * Terminal state and output are delivered separately through
 * [`TerminalUpdate`] and [`TerminalOutputChunk`].
 */
struct Terminal {
    TerminalId _terminalId;  //!< The ID of the terminal to display.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys. This metadata is scoped to the content reference. Omitted
     * and `null` are equivalent and mean no item metadata was provided.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    Terminal& terminalId(const TerminalId & v) { _terminalId = v; return *this; }
    Terminal& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    Terminal& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const TerminalId& terminalId() const { return _terminalId; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<Terminal> fromJson<Terminal>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const Terminal &data);

/**
 * Content produced by a tool call.
 *
 * Tool calls can produce different types of content including standard
 * content blocks (text, images), file diffs, or display-only terminals.
 *
 * See protocol docs: [Content](https://agentclientprotocol.com/protocol/v2/tool-calls#content)
 */
using ToolCallContent = std::variant<Content, Diff, Terminal, QJsonObject>;

template<>
ACPLIB_EXPORT Utils::Result<ToolCallContent> fromJson<ToolCallContent>(const QJsonValue &val);

/** Returns the 'type' dispatch field value for the active variant. */
ACPLIB_EXPORT QString dispatchValue(const ToolCallContent &val);

ACPLIB_EXPORT QJsonObject toJson(const ToolCallContent &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const ToolCallContent &val);

/**
 * A file location being accessed or modified by a tool.
 *
 * Enables clients to implement "follow-along" features that track
 * which files the agent is working with in real-time.
 *
 * See protocol docs: [Following the Agent](https://agentclientprotocol.com/protocol/v2/tool-calls#following-the-agent)
 */
struct ToolCallLocation {
    AbsolutePath _path;  //!< The absolute file path being accessed or modified.
    Patch<int> _line;  //!< Optional line number within the file.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    ToolCallLocation& path(const AbsolutePath & v) { _path = v; return *this; }
    ToolCallLocation& line(const Patch<int> & v) { _line = v; return *this; }
    ToolCallLocation& line(int v) { _line = v; return *this; }
    ToolCallLocation& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    ToolCallLocation& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const AbsolutePath& path() const { return _path; }
    const Patch<int>& line() const { return _line; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<ToolCallLocation> fromJson<ToolCallLocation>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ToolCallLocation &data);

/**
 * Execution status of a tool call.
 *
 * Tool calls progress through different statuses during their lifecycle.
 *
 * See protocol docs: [Status](https://agentclientprotocol.com/protocol/v2/tool-calls#status)
 */
enum class ToolCallStatus {
    pending,
    in_progress,
    completed,
    failed,
    cancelled
};

ACPLIB_EXPORT QString toString(ToolCallStatus v);

template<>
ACPLIB_EXPORT Utils::Result<ToolCallStatus> fromJson<ToolCallStatus>(const QJsonValue &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const ToolCallStatus &v);

/**
 * Categories of tools that can be invoked.
 *
 * Tool kinds help clients choose appropriate icons and optimize how they
 * display tool execution progress.
 *
 * See protocol docs: [Creating](https://agentclientprotocol.com/protocol/v2/tool-calls#creating)
 */
enum class ToolKind {
    read,
    edit,
    delete_,
    move,
    search,
    execute,
    think,
    fetch,
    switch_mode,
    other
};

ACPLIB_EXPORT QString toString(ToolKind v);

template<>
ACPLIB_EXPORT Utils::Result<ToolKind> fromJson<ToolKind>(const QJsonValue &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const ToolKind &v);

/**
 * Represents an upsert for a tool call that the language model has requested.
 *
 * Tool calls are actions that the agent executes on behalf of the language model,
 * such as reading files, executing code, or fetching data from external sources.
 *
 * Only [`ToolCallUpdate::tool_call_id`] is required. Other fields have patch semantics:
 * omitted fields leave the existing tool call value unchanged, `null` clears or
 * unsets the value, and concrete values replace the previous value. For
 * collection fields, concrete arrays replace the previous collection, and both
 * `null` and `[]` clear the collection. When a client receives a tool call ID it
 * has not seen before, omitted fields use client defaults.
 *
 * See protocol docs: [Tool Calls](https://agentclientprotocol.com/protocol/v2/tool-calls)
 */
struct ToolCallUpdate {
    ToolCallId _toolCallId;  //!< Unique identifier for this tool call within the session.
    Patch<QString> _title;  //!< Human-readable title describing what the tool is doing.
    /**
     * The category of tool being invoked.
     * Helps clients choose appropriate icons and UI treatment.
     */
    Patch<ToolKind> _kind;
    Patch<ToolCallStatus> _status;  //!< Current execution status of the tool call.
    Patch<QList<ToolCallContent>> _content;  //!< Content produced by the tool call.
    /**
     * File locations affected by this tool call.
     * Enables "follow-along" features in clients.
     */
    Patch<QList<ToolCallLocation>> _locations;
    std::optional<QJsonValue> _rawInput;  //!< Raw input parameters sent to the tool.
    std::optional<QJsonValue> _rawOutput;  //!< Raw output returned by the tool.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted means no metadata update; `null` is an
     * explicit clear signal. Implementations MUST NOT make assumptions about values at these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    ToolCallUpdate& toolCallId(const ToolCallId & v) { _toolCallId = v; return *this; }
    ToolCallUpdate& title(const Patch<QString> & v) { _title = v; return *this; }
    ToolCallUpdate& title(const QString & v) { _title = v; return *this; }
    ToolCallUpdate& kind(const Patch<ToolKind> & v) { _kind = v; return *this; }
    ToolCallUpdate& kind(const ToolKind & v) { _kind = v; return *this; }
    ToolCallUpdate& status(const Patch<ToolCallStatus> & v) { _status = v; return *this; }
    ToolCallUpdate& status(const ToolCallStatus & v) { _status = v; return *this; }
    ToolCallUpdate& content(const Patch<QList<ToolCallContent>> & v) { _content = v; return *this; }
    ToolCallUpdate& content(const QList<ToolCallContent> & v) { _content = v; return *this; }
    ToolCallUpdate& locations(const Patch<QList<ToolCallLocation>> & v) { _locations = v; return *this; }
    ToolCallUpdate& locations(const QList<ToolCallLocation> & v) { _locations = v; return *this; }
    ToolCallUpdate& rawInput(const std::optional<QJsonValue> & v) { _rawInput = v; return *this; }
    ToolCallUpdate& rawOutput(const std::optional<QJsonValue> & v) { _rawOutput = v; return *this; }
    ToolCallUpdate& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    ToolCallUpdate& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const ToolCallId& toolCallId() const { return _toolCallId; }
    const Patch<QString>& title() const { return _title; }
    const Patch<ToolKind>& kind() const { return _kind; }
    const Patch<ToolCallStatus>& status() const { return _status; }
    const Patch<QList<ToolCallContent>>& content() const { return _content; }
    const Patch<QList<ToolCallLocation>>& locations() const { return _locations; }
    const std::optional<QJsonValue>& rawInput() const { return _rawInput; }
    const std::optional<QJsonValue>& rawOutput() const { return _rawOutput; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<ToolCallUpdate> fromJson<ToolCallUpdate>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ToolCallUpdate &data);

/** Permission request details for a tool call. */
struct ToolCallPermissionSubject {
    ToolCallUpdate _toolCall;  //!< Details about the tool call requiring permission.

    ToolCallPermissionSubject& toolCall(const ToolCallUpdate & v) { _toolCall = v; return *this; }

    const ToolCallUpdate& toolCall() const { return _toolCall; }
};

template<>
ACPLIB_EXPORT Utils::Result<ToolCallPermissionSubject> fromJson<ToolCallPermissionSubject>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ToolCallPermissionSubject &data);

/** The operation requiring permission. */
using RequestPermissionSubject = std::variant<ToolCallPermissionSubject, CommandPermissionSubject, QJsonObject>;

template<>
ACPLIB_EXPORT Utils::Result<RequestPermissionSubject> fromJson<RequestPermissionSubject>(const QJsonValue &val);

/** Returns the 'type' dispatch field value for the active variant. */
ACPLIB_EXPORT QString dispatchValue(const RequestPermissionSubject &val);

ACPLIB_EXPORT QJsonObject toJson(const RequestPermissionSubject &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const RequestPermissionSubject &val);

/**
 * Request for user permission to proceed with an operation.
 *
 * Sent when the agent needs authorization before performing a sensitive operation.
 *
 * See protocol docs: [Requesting Permission](https://agentclientprotocol.com/protocol/v2/tool-calls#requesting-permission)
 */
struct RequestPermissionRequest {
    SessionId _sessionId;  //!< The session ID for this request.
    /**
     * Human-readable title for the permission prompt.
     *
     * This title is specific to the permission prompt and does not update any
     * subject's displayed title.
     */
    QString _title;
    /**
     * Optional human-readable explanation of why permission is needed.
     *
     * This text is specific to the permission prompt and does not update any
     * subject's displayed content. Omitted or `null` both mean no separate
     * permission description was provided.
     */
    Patch<QString> _description;
    /**
     * Optional structured context about the operation requiring permission.
     *
     * Omitted or `null` both mean no structured subject was provided.
     */
    Patch<RequestPermissionSubject> _subject;
    /**
     * Available permission options for the user to choose from.
     * Must contain at least one option.
     */
    QList<PermissionOption> _options;
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    RequestPermissionRequest& sessionId(const SessionId & v) { _sessionId = v; return *this; }
    RequestPermissionRequest& title(const QString & v) { _title = v; return *this; }
    RequestPermissionRequest& description(const Patch<QString> & v) { _description = v; return *this; }
    RequestPermissionRequest& description(const QString & v) { _description = v; return *this; }
    RequestPermissionRequest& subject(const Patch<RequestPermissionSubject> & v) { _subject = v; return *this; }
    RequestPermissionRequest& subject(const RequestPermissionSubject & v) { _subject = v; return *this; }
    RequestPermissionRequest& options(const QList<PermissionOption> & v) { _options = v; return *this; }
    RequestPermissionRequest& addOption(const PermissionOption & v) { _options.append(v); return *this; }
    RequestPermissionRequest& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    RequestPermissionRequest& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const SessionId& sessionId() const { return _sessionId; }
    const QString& title() const { return _title; }
    const Patch<QString>& description() const { return _description; }
    const Patch<RequestPermissionSubject>& subject() const { return _subject; }
    const QList<PermissionOption>& options() const { return _options; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<RequestPermissionRequest> fromJson<RequestPermissionRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const RequestPermissionRequest &data);

/** A JSON-RPC request object. */
struct AgentRequest {
    RequestId _id;  //!< The request id used to correlate the matching response.
    QString _method;  //!< The method name to invoke.
    std::optional<QString> _params;  //!< Method-specific request parameters.

    AgentRequest& id(const RequestId & v) { _id = v; return *this; }
    AgentRequest& method(const QString & v) { _method = v; return *this; }
    AgentRequest& params(const std::optional<QString> & v) { _params = v; return *this; }

    const RequestId& id() const { return _id; }
    const QString& method() const { return _method; }
    const std::optional<QString>& params() const { return _params; }
};

template<>
ACPLIB_EXPORT Utils::Result<AgentRequest> fromJson<AgentRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const AgentRequest &data);

/** Response from closing a session. */
struct CloseSessionResponse {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    CloseSessionResponse& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    CloseSessionResponse& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<CloseSessionResponse> fromJson<CloseSessionResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const CloseSessionResponse &data);

/** Response from deleting a session. */
struct DeleteSessionResponse {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    DeleteSessionResponse& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    DeleteSessionResponse& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<DeleteSessionResponse> fromJson<DeleteSessionResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const DeleteSessionResponse &data);

/**
 * Predefined error codes for common JSON-RPC and ACP-specific errors.
 *
 * These codes follow the JSON-RPC 2.0 specification for standard errors
 * and use the reserved range (-32000 to -32099) for protocol-specific errors.
 */
namespace ErrorCode {
    constexpr int Parse_error = -32700;
    constexpr int Invalid_request = -32600;
    constexpr int Method_not_found = -32601;
    constexpr int Invalid_params = -32602;
    constexpr int Internal_error = -32603;
    constexpr int Request_cancelled = -32800;
    constexpr int Authentication_required = -32000;
    constexpr int Resource_not_found = -32002;
} // namespace ErrorCode
/**
 * JSON-RPC error object.
 *
 * Represents an error that occurred during method execution, following the
 * JSON-RPC 2.0 error object specification with optional additional data.
 *
 * See protocol docs: [JSON-RPC Error Object](https://www.jsonrpc.org/specification#error_object)
 */
struct Error {
    /**
     * A number indicating the error type that occurred.
     * This must be an integer as defined in the JSON-RPC specification.
     */
    int _code;
    /**
     * A string providing a short description of the error.
     * The message should be limited to a concise single sentence.
     */
    QString _message;
    /**
     * Optional primitive or structured value that contains additional information about the error.
     * This may include debugging information or context-specific details.
     */
    std::optional<QJsonValue> _data;

    Error& code(int v) { _code = v; return *this; }
    Error& message(const QString & v) { _message = v; return *this; }
    Error& data(const std::optional<QJsonValue> & v) { _data = v; return *this; }

    const int& code() const { return _code; }
    const QString& message() const { return _message; }
    const std::optional<QJsonValue>& data() const { return _data; }
};

template<>
ACPLIB_EXPORT Utils::Result<Error> fromJson<Error>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const Error &data);

// Skipped unknown type alias: ExtResponse

/**
 * Authentication-related extension capabilities supported by the agent.
 *
 * This object does not advertise support for `auth/login` or `auth/logout`.
 * Those methods are advertised by a non-empty `authMethods` list in the
 * `initialize` response.
 */
struct AgentAuthCapabilities {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    AgentAuthCapabilities& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    AgentAuthCapabilities& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<AgentAuthCapabilities> fromJson<AgentAuthCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const AgentAuthCapabilities &data);

/**
 * Capabilities for HTTP MCP server transports.
 *
 * Supplying `{}` means the agent supports HTTP MCP server transports.
 */
struct McpHttpCapabilities {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    McpHttpCapabilities& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    McpHttpCapabilities& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<McpHttpCapabilities> fromJson<McpHttpCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const McpHttpCapabilities &data);

/**
 * Capabilities for stdio MCP server transports.
 *
 * Supplying `{}` means the agent supports stdio MCP server transports.
 */
struct McpStdioCapabilities {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    McpStdioCapabilities& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    McpStdioCapabilities& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<McpStdioCapabilities> fromJson<McpStdioCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const McpStdioCapabilities &data);

/** MCP capabilities supported by the agent for session lifecycle requests. */
struct McpCapabilities {
    /**
     * Agent supports [`McpServer::Stdio`].
     *
     * Optional. Omitted or `null` both mean the agent does not advertise support.
     * Supplying `{}` means the agent supports stdio MCP server transports.
     */
    Patch<McpStdioCapabilities> _stdio;
    /**
     * Agent supports [`McpServer::Http`].
     *
     * Optional. Omitted or `null` both mean the agent does not advertise support.
     * Supplying `{}` means the agent supports HTTP MCP server transports.
     */
    Patch<McpHttpCapabilities> _http;
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    McpCapabilities& stdio(const Patch<McpStdioCapabilities> & v) { _stdio = v; return *this; }
    McpCapabilities& stdio(const McpStdioCapabilities & v) { _stdio = v; return *this; }
    McpCapabilities& http(const Patch<McpHttpCapabilities> & v) { _http = v; return *this; }
    McpCapabilities& http(const McpHttpCapabilities & v) { _http = v; return *this; }
    McpCapabilities& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    McpCapabilities& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<McpStdioCapabilities>& stdio() const { return _stdio; }
    const Patch<McpHttpCapabilities>& http() const { return _http; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<McpCapabilities> fromJson<McpCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const McpCapabilities &data);

/**
 * Capabilities for audio content in prompt requests.
 *
 * Supplying `{}` means the agent supports audio content in prompts.
 */
struct PromptAudioCapabilities {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    PromptAudioCapabilities& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    PromptAudioCapabilities& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<PromptAudioCapabilities> fromJson<PromptAudioCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const PromptAudioCapabilities &data);

/**
 * Capabilities for embedded context in prompt requests.
 *
 * Supplying `{}` means the agent supports embedded context in prompts.
 */
struct PromptEmbeddedContextCapabilities {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    PromptEmbeddedContextCapabilities& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    PromptEmbeddedContextCapabilities& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<PromptEmbeddedContextCapabilities> fromJson<PromptEmbeddedContextCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const PromptEmbeddedContextCapabilities &data);

/**
 * Capabilities for image content in prompt requests.
 *
 * Supplying `{}` means the agent supports image content in prompts.
 */
struct PromptImageCapabilities {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    PromptImageCapabilities& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    PromptImageCapabilities& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<PromptImageCapabilities> fromJson<PromptImageCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const PromptImageCapabilities &data);

/**
 * Prompt capabilities supported by the agent in `session/prompt` requests.
 *
 * Baseline agent functionality requires support for [`ContentBlock::Text`]
 * and [`ContentBlock::ResourceLink`] in prompt requests.
 *
 * Other variants must be explicitly opted in to.
 * Capabilities for different types of content in prompt requests.
 *
 * Indicates which content types beyond the baseline (text and resource links)
 * the agent can process.
 *
 * See protocol docs: [Prompt Capabilities](https://agentclientprotocol.com/protocol/v2/initialization#prompt-capabilities)
 */
struct PromptCapabilities {
    /**
     * Agent supports [`ContentBlock::Image`].
     *
     * Optional. Omitted or `null` both mean the agent does not advertise support.
     * Supplying `{}` means the agent supports image content in prompts.
     */
    Patch<PromptImageCapabilities> _image;
    /**
     * Agent supports [`ContentBlock::Audio`].
     *
     * Optional. Omitted or `null` both mean the agent does not advertise support.
     * Supplying `{}` means the agent supports audio content in prompts.
     */
    Patch<PromptAudioCapabilities> _audio;
    /**
     * Agent supports embedded context in `session/prompt` requests.
     *
     * When enabled, the Client is allowed to include [`ContentBlock::Resource`]
     * in prompt requests for pieces of context that are referenced in the message.
     *
     * Optional. Omitted or `null` both mean the agent does not advertise support.
     * Supplying `{}` means the agent supports embedded context in prompts.
     */
    Patch<PromptEmbeddedContextCapabilities> _embeddedContext;
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    PromptCapabilities& image(const Patch<PromptImageCapabilities> & v) { _image = v; return *this; }
    PromptCapabilities& image(const PromptImageCapabilities & v) { _image = v; return *this; }
    PromptCapabilities& audio(const Patch<PromptAudioCapabilities> & v) { _audio = v; return *this; }
    PromptCapabilities& audio(const PromptAudioCapabilities & v) { _audio = v; return *this; }
    PromptCapabilities& embeddedContext(const Patch<PromptEmbeddedContextCapabilities> & v) { _embeddedContext = v; return *this; }
    PromptCapabilities& embeddedContext(const PromptEmbeddedContextCapabilities & v) { _embeddedContext = v; return *this; }
    PromptCapabilities& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    PromptCapabilities& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<PromptImageCapabilities>& image() const { return _image; }
    const Patch<PromptAudioCapabilities>& audio() const { return _audio; }
    const Patch<PromptEmbeddedContextCapabilities>& embeddedContext() const { return _embeddedContext; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<PromptCapabilities> fromJson<PromptCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const PromptCapabilities &data);

/**
 * Capabilities for additional session directories support.
 *
 * Supplying `{}` means the agent supports the `additionalDirectories` field on
 * supported session lifecycle requests. Agents that also support
 * `session/list` may return `SessionInfo.additionalDirectories` to report the
 * complete ordered additional-root list associated with a listed session.
 */
struct SessionAdditionalDirectoriesCapabilities {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    SessionAdditionalDirectoriesCapabilities& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    SessionAdditionalDirectoriesCapabilities& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<SessionAdditionalDirectoriesCapabilities> fromJson<SessionAdditionalDirectoriesCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionAdditionalDirectoriesCapabilities &data);

/**
 * Capabilities for the `session/delete` method.
 *
 * Supplying `{}` means the agent supports deleting sessions from `session/list`.
 */
struct SessionDeleteCapabilities {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    SessionDeleteCapabilities& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    SessionDeleteCapabilities& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<SessionDeleteCapabilities> fromJson<SessionDeleteCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionDeleteCapabilities &data);

/**
 * Session capabilities supported by the agent.
 *
 * Supplying `{}` means the agent supports the baseline session methods:
 * `session/new`, `session/list`, `session/resume`, `session/close`,
 * `session/prompt`, `session/cancel`, and `session/update`.
 *
 * Agents that support sessions **MAY** support additional session methods,
 * prompt content types, and MCP transports by specifying additional
 * capabilities.
 *
 * See protocol docs: [Session Capabilities](https://agentclientprotocol.com/protocol/v2/initialization#session-capabilities)
 */
struct SessionCapabilities {
    /**
     * Prompt capabilities supported by the agent in `session/prompt` requests.
     *
     * Optional. Omitted or `null` both mean the agent does not advertise any
     * prompt extensions beyond the baseline text and resource-link content
     * required by `session/prompt`.
     */
    Patch<PromptCapabilities> _prompt;
    /**
     * MCP capabilities supported by the agent for session lifecycle requests.
     *
     * Optional. Omitted or `null` both mean the agent does not advertise MCP
     * server transport support for sessions.
     */
    Patch<McpCapabilities> _mcp;
    /**
     * Whether the agent supports `session/delete`.
     *
     * Optional. Omitted or `null` both mean the agent does not advertise support.
     * Supplying `{}` means the agent supports deleting sessions from `session/list`.
     */
    Patch<SessionDeleteCapabilities> _delete_;
    /**
     * Whether the agent supports `additionalDirectories` on supported session lifecycle requests.
     *
     * Optional. Omitted or `null` both mean the agent does not advertise support.
     * Supplying `{}` means the agent supports `additionalDirectories` on
     * supported session lifecycle requests.
     *
     * Agents may return `SessionInfo.additionalDirectories` to report the
     * complete ordered additional-root list associated with a listed session.
     */
    Patch<SessionAdditionalDirectoriesCapabilities> _additionalDirectories;
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    SessionCapabilities& prompt(const Patch<PromptCapabilities> & v) { _prompt = v; return *this; }
    SessionCapabilities& prompt(const PromptCapabilities & v) { _prompt = v; return *this; }
    SessionCapabilities& mcp(const Patch<McpCapabilities> & v) { _mcp = v; return *this; }
    SessionCapabilities& mcp(const McpCapabilities & v) { _mcp = v; return *this; }
    SessionCapabilities& delete_(const Patch<SessionDeleteCapabilities> & v) { _delete_ = v; return *this; }
    SessionCapabilities& delete_(const SessionDeleteCapabilities & v) { _delete_ = v; return *this; }
    SessionCapabilities& additionalDirectories(const Patch<SessionAdditionalDirectoriesCapabilities> & v) { _additionalDirectories = v; return *this; }
    SessionCapabilities& additionalDirectories(const SessionAdditionalDirectoriesCapabilities & v) { _additionalDirectories = v; return *this; }
    SessionCapabilities& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    SessionCapabilities& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<PromptCapabilities>& prompt() const { return _prompt; }
    const Patch<McpCapabilities>& mcp() const { return _mcp; }
    const Patch<SessionDeleteCapabilities>& delete_() const { return _delete_; }
    const Patch<SessionAdditionalDirectoriesCapabilities>& additionalDirectories() const { return _additionalDirectories; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<SessionCapabilities> fromJson<SessionCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionCapabilities &data);

/**
 * Capabilities supported by the agent.
 *
 * Advertised during initialization to inform the client about
 * available features and content types.
 *
 * See protocol docs: [Agent Capabilities](https://agentclientprotocol.com/protocol/v2/initialization#agent-capabilities)
 */
struct AgentCapabilities {
    /**
     * Session capabilities supported by the agent.
     *
     * Optional. Omitted or `null` both mean the agent does not support the
     * `session/\*` method surface. Supplying `{}` means the agent supports the
     * baseline session methods: `session/new`, `session/prompt`,
     * `session/cancel`, and `session/update`.
     */
    Patch<SessionCapabilities> _session;
    /**
     * Authentication-related extension capabilities supported by the agent.
     *
     * Optional. Omitted or `null` both mean the agent does not advertise any
     * authentication-related extensions. This field does not advertise support
     * for `auth/login` or `auth/logout`; those methods are advertised by a
     * non-empty `authMethods` list in the `initialize` response.
     */
    Patch<AgentAuthCapabilities> _auth;
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    AgentCapabilities& session(const Patch<SessionCapabilities> & v) { _session = v; return *this; }
    AgentCapabilities& session(const SessionCapabilities & v) { _session = v; return *this; }
    AgentCapabilities& auth(const Patch<AgentAuthCapabilities> & v) { _auth = v; return *this; }
    AgentCapabilities& auth(const AgentAuthCapabilities & v) { _auth = v; return *this; }
    AgentCapabilities& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    AgentCapabilities& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<SessionCapabilities>& session() const { return _session; }
    const Patch<AgentAuthCapabilities>& auth() const { return _auth; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<AgentCapabilities> fromJson<AgentCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const AgentCapabilities &data);

using AuthMethodId = QString;

/**
 * Agent handles authentication itself through `auth/login`.
 *
 * The `type` discriminator value is `agent`.
 */
struct AuthMethodAgent {
    AuthMethodId _methodId;  //!< Unique identifier for this authentication method.
    QString _name;  //!< Human-readable name of the authentication method.
    Patch<QString> _description;  //!< Optional description providing more details about this authentication method.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    AuthMethodAgent& methodId(const AuthMethodId & v) { _methodId = v; return *this; }
    AuthMethodAgent& name(const QString & v) { _name = v; return *this; }
    AuthMethodAgent& description(const Patch<QString> & v) { _description = v; return *this; }
    AuthMethodAgent& description(const QString & v) { _description = v; return *this; }
    AuthMethodAgent& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    AuthMethodAgent& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const AuthMethodId& methodId() const { return _methodId; }
    const QString& name() const { return _name; }
    const Patch<QString>& description() const { return _description; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<AuthMethodAgent> fromJson<AuthMethodAgent>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const AuthMethodAgent &data);

/**
 * Describes an available authentication method.
 *
 * The `type` field acts as the discriminator in the serialized JSON form.
 */
using AuthMethod = std::variant<AuthMethodAgent, QJsonObject>;

template<>
ACPLIB_EXPORT Utils::Result<AuthMethod> fromJson<AuthMethod>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const AuthMethod &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const AuthMethod &val);

/**
 * Metadata about the implementation of the client or agent.
 * Describes the name and version of an ACP implementation, with an optional
 * title for UI representation.
 */
struct Implementation {
    /**
     * Intended for programmatic or logical use, but can be used as a display
     * name fallback if title isn’t present.
     */
    QString _name;
    /**
     * Intended for UI and end-user contexts — optimized to be human-readable
     * and easily understood.
     *
     * If not provided, the name should be used for display.
     */
    Patch<QString> _title;
    /**
     * Version of the implementation. Can be displayed to the user or used
     * for debugging or metrics purposes. (e.g. "1.0.0").
     */
    QString _version;
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    Implementation& name(const QString & v) { _name = v; return *this; }
    Implementation& title(const Patch<QString> & v) { _title = v; return *this; }
    Implementation& title(const QString & v) { _title = v; return *this; }
    Implementation& version(const QString & v) { _version = v; return *this; }
    Implementation& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    Implementation& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const QString& name() const { return _name; }
    const Patch<QString>& title() const { return _title; }
    const QString& version() const { return _version; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<Implementation> fromJson<Implementation>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const Implementation &data);

using ProtocolVersion = int;
template<>
ACPLIB_EXPORT Utils::Result<ProtocolVersion> fromJson<ProtocolVersion>(const QJsonValue &val);

/**
 * Response to the `initialize` method.
 *
 * Contains the negotiated protocol version and agent capabilities.
 *
 * See protocol docs: [Initialization](https://agentclientprotocol.com/protocol/v2/initialization)
 */
struct InitializeResponse {
    /**
     * The protocol version the client specified if supported by the agent,
     * or the latest protocol version supported by the agent.
     *
     * The client should disconnect, if it doesn't support this version.
     */
    ProtocolVersion _protocolVersion;
    Implementation _info;  //!< Information about the implementation sending this initialize response.
    std::optional<AgentCapabilities> _capabilities;  //!< Capabilities supported by the agent.
    /**
     * Authentication methods supported by the agent.
     *
     * Optional. Omitted or empty means the agent does not advertise the
     * authentication method surface. Supplying one or more valid methods means
     * the agent MUST support both `auth/login` and `auth/logout`.
     */
    std::optional<QList<AuthMethod>> _authMethods;
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    InitializeResponse& protocolVersion(const ProtocolVersion & v) { _protocolVersion = v; return *this; }
    InitializeResponse& info(const Implementation & v) { _info = v; return *this; }
    InitializeResponse& capabilities(const std::optional<AgentCapabilities> & v) { _capabilities = v; return *this; }
    InitializeResponse& authMethods(const std::optional<QList<AuthMethod>> & v) { _authMethods = v; return *this; }
    InitializeResponse& addAuthMethod(const AuthMethod & v) { if (!_authMethods) _authMethods = QList<AuthMethod>{}; (*_authMethods).append(v); return *this; }
    InitializeResponse& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    InitializeResponse& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const ProtocolVersion& protocolVersion() const { return _protocolVersion; }
    const Implementation& info() const { return _info; }
    const std::optional<AgentCapabilities>& capabilities() const { return _capabilities; }
    const std::optional<QList<AuthMethod>>& authMethods() const { return _authMethods; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<InitializeResponse> fromJson<InitializeResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const InitializeResponse &data);

/** Information about a session returned by session/list */
struct SessionInfo {
    SessionId _sessionId;  //!< Unique identifier for the session
    AbsolutePath _cwd;  //!< The working directory for this session. Must be an absolute path.
    /**
     * Additional workspace roots reported for this session. Each path must be absolute.
     *
     * When present, this is the complete ordered additional-root list reported
     * by the Agent. Omitted and empty values are equivalent: the response
     * reports no additional roots.
     */
    std::optional<QList<AbsolutePath>> _additionalDirectories;
    Patch<QString> _title;  //!< Human-readable title for the session
    Patch<QString> _updatedAt;  //!< RFC 3339 timestamp of last activity.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    SessionInfo& sessionId(const SessionId & v) { _sessionId = v; return *this; }
    SessionInfo& cwd(const AbsolutePath & v) { _cwd = v; return *this; }
    SessionInfo& additionalDirectories(const std::optional<QList<AbsolutePath>> & v) { _additionalDirectories = v; return *this; }
    SessionInfo& addAdditionalDirectory(const AbsolutePath & v) { if (!_additionalDirectories) _additionalDirectories = QList<AbsolutePath>{}; (*_additionalDirectories).append(v); return *this; }
    SessionInfo& title(const Patch<QString> & v) { _title = v; return *this; }
    SessionInfo& title(const QString & v) { _title = v; return *this; }
    SessionInfo& updatedAt(const Patch<QString> & v) { _updatedAt = v; return *this; }
    SessionInfo& updatedAt(const QString & v) { _updatedAt = v; return *this; }
    SessionInfo& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    SessionInfo& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const SessionId& sessionId() const { return _sessionId; }
    const AbsolutePath& cwd() const { return _cwd; }
    const std::optional<QList<AbsolutePath>>& additionalDirectories() const { return _additionalDirectories; }
    const Patch<QString>& title() const { return _title; }
    const Patch<QString>& updatedAt() const { return _updatedAt; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<SessionInfo> fromJson<SessionInfo>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionInfo &data);

using SessionListCursor = QString;

/** Response from listing sessions. */
struct ListSessionsResponse {
    QList<SessionInfo> _sessions;  //!< Array of session information objects.
    /**
     * Opaque cursor token. If present, pass this in the next request's cursor parameter
     * to fetch the next page. If absent, there are no more results.
     */
    Patch<SessionListCursor> _nextCursor;
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    ListSessionsResponse& sessions(const QList<SessionInfo> & v) { _sessions = v; return *this; }
    ListSessionsResponse& addSession(const SessionInfo & v) { _sessions.append(v); return *this; }
    ListSessionsResponse& nextCursor(const Patch<SessionListCursor> & v) { _nextCursor = v; return *this; }
    ListSessionsResponse& nextCursor(const SessionListCursor & v) { _nextCursor = v; return *this; }
    ListSessionsResponse& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    ListSessionsResponse& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const QList<SessionInfo>& sessions() const { return _sessions; }
    const Patch<SessionListCursor>& nextCursor() const { return _nextCursor; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<ListSessionsResponse> fromJson<ListSessionsResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ListSessionsResponse &data);

/** Response to the `auth/login` method. */
struct LoginAuthResponse {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    LoginAuthResponse& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    LoginAuthResponse& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<LoginAuthResponse> fromJson<LoginAuthResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const LoginAuthResponse &data);

/** Response to the `auth/logout` method. */
struct LogoutAuthResponse {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    LogoutAuthResponse& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    LogoutAuthResponse& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<LogoutAuthResponse> fromJson<LogoutAuthResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const LogoutAuthResponse &data);

/** A boolean on/off toggle session configuration option payload. */
struct SessionConfigBoolean {
    bool _currentValue;  //!< The current value of the boolean option.

    SessionConfigBoolean& currentValue(bool v) { _currentValue = v; return *this; }

    const bool& currentValue() const { return _currentValue; }
};

template<>
ACPLIB_EXPORT Utils::Result<SessionConfigBoolean> fromJson<SessionConfigBoolean>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionConfigBoolean &data);

using SessionConfigId = QString;

/**
 * Semantic category for a session configuration option.
 *
 * This is intended to help Clients distinguish broadly common selectors (e.g. model selector vs
 * session mode selector vs thought/reasoning level) for UX purposes (keyboard shortcuts, icons,
 * placement). It MUST NOT be required for correctness. Clients MUST handle missing or unknown
 * categories gracefully.
 *
 * Category names beginning with `_` are free for custom use, like other ACP extension methods.
 * Category names that do not begin with `_` are reserved for the ACP spec.
 */
enum class SessionConfigOptionCategory {
    mode,
    model,
    model_config,
    thought_level
};

ACPLIB_EXPORT QString toString(SessionConfigOptionCategory v);

template<>
ACPLIB_EXPORT Utils::Result<SessionConfigOptionCategory> fromJson<SessionConfigOptionCategory>(const QJsonValue &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const SessionConfigOptionCategory &v);

using SessionConfigGroupId = QString;

using SessionConfigValueId = QString;

/** A possible value for a session configuration option. */
struct SessionConfigSelectOption {
    SessionConfigValueId _value;  //!< Unique identifier for this option value.
    QString _name;  //!< Human-readable label for this option value.
    Patch<QString> _description;  //!< Optional description for this option value.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    SessionConfigSelectOption& value(const SessionConfigValueId & v) { _value = v; return *this; }
    SessionConfigSelectOption& name(const QString & v) { _name = v; return *this; }
    SessionConfigSelectOption& description(const Patch<QString> & v) { _description = v; return *this; }
    SessionConfigSelectOption& description(const QString & v) { _description = v; return *this; }
    SessionConfigSelectOption& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    SessionConfigSelectOption& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const SessionConfigValueId& value() const { return _value; }
    const QString& name() const { return _name; }
    const Patch<QString>& description() const { return _description; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<SessionConfigSelectOption> fromJson<SessionConfigSelectOption>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionConfigSelectOption &data);

/** A group of possible values for a session configuration option. */
struct SessionConfigSelectGroup {
    SessionConfigGroupId _groupId;  //!< Unique identifier for this group.
    QString _name;  //!< Human-readable label for this group.
    QList<SessionConfigSelectOption> _options;  //!< The set of option values in this group.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    SessionConfigSelectGroup& groupId(const SessionConfigGroupId & v) { _groupId = v; return *this; }
    SessionConfigSelectGroup& name(const QString & v) { _name = v; return *this; }
    SessionConfigSelectGroup& options(const QList<SessionConfigSelectOption> & v) { _options = v; return *this; }
    SessionConfigSelectGroup& addOption(const SessionConfigSelectOption & v) { _options.append(v); return *this; }
    SessionConfigSelectGroup& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    SessionConfigSelectGroup& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const SessionConfigGroupId& groupId() const { return _groupId; }
    const QString& name() const { return _name; }
    const QList<SessionConfigSelectOption>& options() const { return _options; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<SessionConfigSelectGroup> fromJson<SessionConfigSelectGroup>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionConfigSelectGroup &data);

/** Possible values for a session configuration option. */
using SessionConfigSelectOptions = std::variant<QList<SessionConfigSelectOption>, QList<SessionConfigSelectGroup>>;

template<>
ACPLIB_EXPORT Utils::Result<SessionConfigSelectOptions> fromJson<SessionConfigSelectOptions>(const QJsonValue &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const SessionConfigSelectOptions &val);

/** A single-value selector (dropdown) session configuration option payload. */
struct SessionConfigSelect {
    SessionConfigValueId _currentValue;  //!< The currently selected value.
    SessionConfigSelectOptions _options;  //!< The set of selectable options.

    SessionConfigSelect& currentValue(const SessionConfigValueId & v) { _currentValue = v; return *this; }
    SessionConfigSelect& options(const SessionConfigSelectOptions & v) { _options = v; return *this; }

    const SessionConfigValueId& currentValue() const { return _currentValue; }
    const SessionConfigSelectOptions& options() const { return _options; }
};

template<>
ACPLIB_EXPORT Utils::Result<SessionConfigSelect> fromJson<SessionConfigSelect>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionConfigSelect &data);

/** A session configuration option selector and its current state. */
struct SessionConfigOption {
    SessionConfigId _configId;  //!< Unique identifier for the configuration option.
    QString _name;  //!< Human-readable label for the option.
    Patch<QString> _description;  //!< Optional description for the Client to display to the user.
    Patch<SessionConfigOptionCategory> _category;  //!< Optional semantic category for this option (UX only).
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;
    QJsonObject _additionalProperties;  //!< additional properties

    SessionConfigOption& configId(const SessionConfigId & v) { _configId = v; return *this; }
    SessionConfigOption& name(const QString & v) { _name = v; return *this; }
    SessionConfigOption& description(const Patch<QString> & v) { _description = v; return *this; }
    SessionConfigOption& description(const QString & v) { _description = v; return *this; }
    SessionConfigOption& category(const Patch<SessionConfigOptionCategory> & v) { _category = v; return *this; }
    SessionConfigOption& category(const SessionConfigOptionCategory & v) { _category = v; return *this; }
    SessionConfigOption& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    SessionConfigOption& _meta(const QJsonObject & v) { __meta = v; return *this; }
    SessionConfigOption& additionalProperties(const QString &key, const QJsonValue &v) { _additionalProperties.insert(key, v); return *this; }
    SessionConfigOption& additionalProperties(const QJsonObject &obj) { for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) _additionalProperties.insert(it.key(), it.value()); return *this; }

    const SessionConfigId& configId() const { return _configId; }
    const QString& name() const { return _name; }
    const Patch<QString>& description() const { return _description; }
    const Patch<SessionConfigOptionCategory>& category() const { return _category; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
    const QJsonObject& additionalProperties() const { return _additionalProperties; }
};

template<>
ACPLIB_EXPORT Utils::Result<SessionConfigOption> fromJson<SessionConfigOption>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionConfigOption &data);

/**
 * Response from creating a new session.
 *
 * See protocol docs: [Creating a Session](https://agentclientprotocol.com/protocol/v2/session-setup#creating-a-session)
 */
struct NewSessionResponse {
    /**
     * Unique identifier for the created session.
     *
     * Used in all subsequent requests for this conversation.
     */
    SessionId _sessionId;
    std::optional<QList<SessionConfigOption>> _configOptions;  //!< Initial session configuration options.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    NewSessionResponse& sessionId(const SessionId & v) { _sessionId = v; return *this; }
    NewSessionResponse& configOptions(const std::optional<QList<SessionConfigOption>> & v) { _configOptions = v; return *this; }
    NewSessionResponse& addConfigOption(const SessionConfigOption & v) { if (!_configOptions) _configOptions = QList<SessionConfigOption>{}; (*_configOptions).append(v); return *this; }
    NewSessionResponse& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    NewSessionResponse& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const SessionId& sessionId() const { return _sessionId; }
    const std::optional<QList<SessionConfigOption>>& configOptions() const { return _configOptions; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<NewSessionResponse> fromJson<NewSessionResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const NewSessionResponse &data);

/**
 * Response acknowledging that a user prompt was accepted.
 *
 * This response does not indicate that the agent has finished processing.
 * Processing and completion are reported through `state_update` session updates.
 *
 * See protocol docs: [Prompt Accepted](https://agentclientprotocol.com/protocol/v2/prompt-lifecycle#2-prompt-accepted)
 */
struct PromptResponse {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    PromptResponse& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    PromptResponse& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<PromptResponse> fromJson<PromptResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const PromptResponse &data);

/** Response from resuming an existing session. */
struct ResumeSessionResponse {
    std::optional<QList<SessionConfigOption>> _configOptions;  //!< Initial session configuration options.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    ResumeSessionResponse& configOptions(const std::optional<QList<SessionConfigOption>> & v) { _configOptions = v; return *this; }
    ResumeSessionResponse& addConfigOption(const SessionConfigOption & v) { if (!_configOptions) _configOptions = QList<SessionConfigOption>{}; (*_configOptions).append(v); return *this; }
    ResumeSessionResponse& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    ResumeSessionResponse& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const std::optional<QList<SessionConfigOption>>& configOptions() const { return _configOptions; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<ResumeSessionResponse> fromJson<ResumeSessionResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ResumeSessionResponse &data);

/** Response to `session/set_config_option` method. */
struct SetSessionConfigOptionResponse {
    QList<SessionConfigOption> _configOptions;  //!< The full set of configuration options and their current values.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    SetSessionConfigOptionResponse& configOptions(const QList<SessionConfigOption> & v) { _configOptions = v; return *this; }
    SetSessionConfigOptionResponse& addConfigOption(const SessionConfigOption & v) { _configOptions.append(v); return *this; }
    SetSessionConfigOptionResponse& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    SetSessionConfigOptionResponse& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const QList<SessionConfigOption>& configOptions() const { return _configOptions; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<SetSessionConfigOptionResponse> fromJson<SetSessionConfigOptionResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SetSessionConfigOptionResponse &data);

/** A JSON-RPC response object. */
using AgentResponse = std::variant<QJsonObject>;

template<>
ACPLIB_EXPORT Utils::Result<AgentResponse> fromJson<AgentResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const AgentResponse &val);

/** Notification sent by the agent when a URL-based elicitation is complete. */
struct CompleteElicitationNotification {
    ElicitationId _elicitationId;  //!< The ID of the elicitation that completed.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * Optional. Omitted and `null` are equivalent and mean no metadata.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    CompleteElicitationNotification& elicitationId(const ElicitationId & v) { _elicitationId = v; return *this; }
    CompleteElicitationNotification& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    CompleteElicitationNotification& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const ElicitationId& elicitationId() const { return _elicitationId; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<CompleteElicitationNotification> fromJson<CompleteElicitationNotification>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const CompleteElicitationNotification &data);

// Skipped unknown type alias: ExtNotification

using MessageId = QString;

/**
 * An agent message upsert.
 *
 * Only [`AgentMessage::message_id`] is required. `content` has patch semantics:
 * an omitted field leaves existing message content unchanged, `null` clears the
 * value, and a concrete array replaces the previous value. For a new
 * `messageId`, omitted fields use client defaults. `content` is replaced as a
 * whole array; send `[]` or `null` to clear it.
 *
 * Message updates and chunks are applied in the order they are received. When
 * an `agent_message` update includes `content`, that array replaces any
 * content previously accumulated for the message, including content from
 * earlier chunks. Later chunks with the same `messageId` append to the current
 * content.
 */
struct AgentMessage {
    MessageId _messageId;  //!< A unique identifier for the message.
    Patch<QList<ContentBlock>> _content;  //!< Complete replacement content for this message.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys. Omitted means no metadata update; `null` is an explicit clear signal.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    AgentMessage& messageId(const MessageId & v) { _messageId = v; return *this; }
    AgentMessage& content(const Patch<QList<ContentBlock>> & v) { _content = v; return *this; }
    AgentMessage& content(const QList<ContentBlock> & v) { _content = v; return *this; }
    AgentMessage& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    AgentMessage& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const MessageId& messageId() const { return _messageId; }
    const Patch<QList<ContentBlock>>& content() const { return _content; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<AgentMessage> fromJson<AgentMessage>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const AgentMessage &data);

/**
 * An agent thought or reasoning message upsert.
 *
 * Only [`AgentThought::message_id`] is required. `content` has patch semantics:
 * an omitted field leaves existing thought content unchanged, `null` clears the
 * value, and a concrete array replaces the previous value. For a new
 * `messageId`, omitted fields use client defaults. `content` is replaced as a
 * whole array; send `[]` or `null` to clear it.
 *
 * Message updates and chunks are applied in the order they are received. When
 * an `agent_thought` update includes `content`, that array replaces any
 * content previously accumulated for the thought, including content from
 * earlier chunks. Later chunks with the same `messageId` append to the current
 * content.
 */
struct AgentThought {
    MessageId _messageId;  //!< A unique identifier for the thought message.
    Patch<QList<ContentBlock>> _content;  //!< Complete replacement content for this thought message.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys. Omitted means no metadata update; `null` is an explicit clear signal.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    AgentThought& messageId(const MessageId & v) { _messageId = v; return *this; }
    AgentThought& content(const Patch<QList<ContentBlock>> & v) { _content = v; return *this; }
    AgentThought& content(const QList<ContentBlock> & v) { _content = v; return *this; }
    AgentThought& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    AgentThought& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const MessageId& messageId() const { return _messageId; }
    const Patch<QList<ContentBlock>>& content() const { return _content; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<AgentThought> fromJson<AgentThought>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const AgentThought &data);

/** All text that was typed after the command name is provided as input. */
struct TextCommandInput {
    QString _hint;  //!< A hint to display when the input hasn't been provided yet
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    TextCommandInput& hint(const QString & v) { _hint = v; return *this; }
    TextCommandInput& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    TextCommandInput& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const QString& hint() const { return _hint; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<TextCommandInput> fromJson<TextCommandInput>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const TextCommandInput &data);

/** The input specification for a command. */
using AvailableCommandInput = std::variant<TextCommandInput, QJsonObject>;

template<>
ACPLIB_EXPORT Utils::Result<AvailableCommandInput> fromJson<AvailableCommandInput>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const AvailableCommandInput &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const AvailableCommandInput &val);

/** Information about a command. */
struct AvailableCommand {
    QString _name;  //!< Command name (e.g., `create_plan`, `research_codebase`).
    QString _description;  //!< Human-readable description of what the command does.
    Patch<AvailableCommandInput> _input;  //!< Input for the command if required
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    AvailableCommand& name(const QString & v) { _name = v; return *this; }
    AvailableCommand& description(const QString & v) { _description = v; return *this; }
    AvailableCommand& input(const Patch<AvailableCommandInput> & v) { _input = v; return *this; }
    AvailableCommand& input(const AvailableCommandInput & v) { _input = v; return *this; }
    AvailableCommand& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    AvailableCommand& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const QString& name() const { return _name; }
    const QString& description() const { return _description; }
    const Patch<AvailableCommandInput>& input() const { return _input; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<AvailableCommand> fromJson<AvailableCommand>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const AvailableCommand &data);

/** Available commands are ready or have changed */
struct AvailableCommandsUpdate {
    QList<AvailableCommand> _availableCommands;  //!< Commands the agent can execute.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    AvailableCommandsUpdate& availableCommands(const QList<AvailableCommand> & v) { _availableCommands = v; return *this; }
    AvailableCommandsUpdate& addAvailableCommand(const AvailableCommand & v) { _availableCommands.append(v); return *this; }
    AvailableCommandsUpdate& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    AvailableCommandsUpdate& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const QList<AvailableCommand>& availableCommands() const { return _availableCommands; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<AvailableCommandsUpdate> fromJson<AvailableCommandsUpdate>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const AvailableCommandsUpdate &data);

/** Session configuration options have been updated. */
struct ConfigOptionUpdate {
    QList<SessionConfigOption> _configOptions;  //!< The full set of configuration options and their current values.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    ConfigOptionUpdate& configOptions(const QList<SessionConfigOption> & v) { _configOptions = v; return *this; }
    ConfigOptionUpdate& addConfigOption(const SessionConfigOption & v) { _configOptions.append(v); return *this; }
    ConfigOptionUpdate& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    ConfigOptionUpdate& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const QList<SessionConfigOption>& configOptions() const { return _configOptions; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<ConfigOptionUpdate> fromJson<ConfigOptionUpdate>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ConfigOptionUpdate &data);

/** A streamed item of message content. */
struct ContentChunk {
    /**
     * A unique identifier for the message this chunk belongs to.
     *
     * All chunks belonging to the same message share the same `messageId`.
     * A change in `messageId` indicates a new message has started.
     */
    MessageId _messageId;
    ContentBlock _content;  //!< A single item of content
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys. This field is chunk-scoped.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    ContentChunk& messageId(const MessageId & v) { _messageId = v; return *this; }
    ContentChunk& content(const ContentBlock & v) { _content = v; return *this; }
    ContentChunk& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    ContentChunk& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const MessageId& messageId() const { return _messageId; }
    const ContentBlock& content() const { return _content; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<ContentChunk> fromJson<ContentChunk>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ContentChunk &data);

using PlanId = QString;

/**
 * Priority levels for plan entries.
 *
 * Used to indicate the relative importance or urgency of different
 * tasks in the execution plan.
 * See protocol docs: [Plan Entries](https://agentclientprotocol.com/protocol/v2/agent-plan#plan-entries)
 */
enum class PlanEntryPriority {
    high,
    medium,
    low
};

ACPLIB_EXPORT QString toString(PlanEntryPriority v);

template<>
ACPLIB_EXPORT Utils::Result<PlanEntryPriority> fromJson<PlanEntryPriority>(const QJsonValue &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const PlanEntryPriority &v);

/**
 * Status of a plan entry in the execution flow.
 *
 * Tracks the lifecycle of each task from planning through completion.
 * See protocol docs: [Plan Entries](https://agentclientprotocol.com/protocol/v2/agent-plan#plan-entries)
 */
enum class PlanEntryStatus {
    pending,
    in_progress,
    completed,
    cancelled
};

ACPLIB_EXPORT QString toString(PlanEntryStatus v);

template<>
ACPLIB_EXPORT Utils::Result<PlanEntryStatus> fromJson<PlanEntryStatus>(const QJsonValue &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const PlanEntryStatus &v);

/**
 * A single entry in the execution plan.
 *
 * Represents a task or goal that the assistant intends to accomplish
 * as part of fulfilling the user's request.
 * See protocol docs: [Plan Entries](https://agentclientprotocol.com/protocol/v2/agent-plan#plan-entries)
 */
struct PlanEntry {
    QString _content;  //!< Human-readable description of what this task aims to accomplish.
    /**
     * The relative importance of this task.
     * Used to indicate which tasks are most critical to the overall goal.
     */
    PlanEntryPriority _priority;
    PlanEntryStatus _status;  //!< Current execution status of this task.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    PlanEntry& content(const QString & v) { _content = v; return *this; }
    PlanEntry& priority(const PlanEntryPriority & v) { _priority = v; return *this; }
    PlanEntry& status(const PlanEntryStatus & v) { _status = v; return *this; }
    PlanEntry& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    PlanEntry& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const QString& content() const { return _content; }
    const PlanEntryPriority& priority() const { return _priority; }
    const PlanEntryStatus& status() const { return _status; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<PlanEntry> fromJson<PlanEntry>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const PlanEntry &data);

/** A plan represented as structured entries. */
struct PlanItems {
    PlanId _planId;  //!< The plan ID to update.
    /**
     * The list of tasks to be accomplished.
     *
     * When updating an item-based plan, the agent must send a complete list of all entries
     * with their current status. The client replaces that plan with each update.
     */
    QList<PlanEntry> _entries;
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    PlanItems& planId(const PlanId & v) { _planId = v; return *this; }
    PlanItems& entries(const QList<PlanEntry> & v) { _entries = v; return *this; }
    PlanItems& addEntry(const PlanEntry & v) { _entries.append(v); return *this; }
    PlanItems& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    PlanItems& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const PlanId& planId() const { return _planId; }
    const QList<PlanEntry>& entries() const { return _entries; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<PlanItems> fromJson<PlanItems>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const PlanItems &data);

/** Updated content for a plan. */
using PlanUpdateContent = std::variant<PlanItems, QJsonObject>;

template<>
ACPLIB_EXPORT Utils::Result<PlanUpdateContent> fromJson<PlanUpdateContent>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const PlanUpdateContent &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const PlanUpdateContent &val);

/** A content update for a plan identified by ID. */
struct PlanUpdate {
    PlanUpdateContent _plan;  //!< The updated plan content.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    PlanUpdate& plan(const PlanUpdateContent & v) { _plan = v; return *this; }
    PlanUpdate& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    PlanUpdate& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const PlanUpdateContent& plan() const { return _plan; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<PlanUpdate> fromJson<PlanUpdate>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const PlanUpdate &data);

/**
 * Update to session metadata. All fields are optional to support partial updates.
 *
 * Agents send this notification to update session information like title or custom metadata.
 * This allows clients to display dynamic session names and track session state changes.
 *
 * Omitted fields leave the existing session info unchanged. `null` clears the
 * corresponding value.
 */
struct SessionInfoUpdate {
    Patch<QString> _title;  //!< Human-readable title for the session. Set to null to clear.
    Patch<QString> _updatedAt;  //!< RFC 3339 timestamp of last activity. Set to null to clear.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted means no metadata update; `null` is an
     * explicit clear signal. Implementations MUST NOT make assumptions about values at these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    SessionInfoUpdate& title(const Patch<QString> & v) { _title = v; return *this; }
    SessionInfoUpdate& title(const QString & v) { _title = v; return *this; }
    SessionInfoUpdate& updatedAt(const Patch<QString> & v) { _updatedAt = v; return *this; }
    SessionInfoUpdate& updatedAt(const QString & v) { _updatedAt = v; return *this; }
    SessionInfoUpdate& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    SessionInfoUpdate& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QString>& title() const { return _title; }
    const Patch<QString>& updatedAt() const { return _updatedAt; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<SessionInfoUpdate> fromJson<SessionInfoUpdate>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionInfoUpdate &data);

/**
 * Reasons why an agent stops active session work.
 *
 * See protocol docs: [Stop Reasons](https://agentclientprotocol.com/protocol/v2/prompt-lifecycle#stop-reasons)
 */
enum class StopReason {
    end_turn,
    max_tokens,
    max_turn_requests,
    refusal,
    cancelled
};

ACPLIB_EXPORT QString toString(StopReason v);

template<>
ACPLIB_EXPORT Utils::Result<StopReason> fromJson<StopReason>(const QJsonValue &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const StopReason &v);

/** The agent is ready to process a new prompt. */
struct IdleStateUpdate {
    /**
     * Indicates why foreground work stopped.
     *
     * Optional. Omitted or `null` both mean the agent is not reporting a stop reason.
     * Agents SHOULD include this when the idle transition ends foreground work.
     */
    Patch<StopReason> _stopReason;
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    IdleStateUpdate& stopReason(const Patch<StopReason> & v) { _stopReason = v; return *this; }
    IdleStateUpdate& stopReason(const StopReason & v) { _stopReason = v; return *this; }
    IdleStateUpdate& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    IdleStateUpdate& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<StopReason>& stopReason() const { return _stopReason; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<IdleStateUpdate> fromJson<IdleStateUpdate>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const IdleStateUpdate &data);

/** Foreground work is blocked on user action. */
struct RequiresActionStateUpdate {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    RequiresActionStateUpdate& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    RequiresActionStateUpdate& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<RequiresActionStateUpdate> fromJson<RequiresActionStateUpdate>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const RequiresActionStateUpdate &data);

/** Foreground work is in progress. */
struct RunningStateUpdate {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    RunningStateUpdate& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    RunningStateUpdate& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<RunningStateUpdate> fromJson<RunningStateUpdate>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const RunningStateUpdate &data);

/**
 * The state of the agent's foreground work has changed.
 *
 * Background activity can continue and emit other `session/update` notifications
 * while `idle`. Those notifications do not change this state.
 */
using StateUpdate = std::variant<RunningStateUpdate, IdleStateUpdate, RequiresActionStateUpdate, QJsonObject>;

template<>
ACPLIB_EXPORT Utils::Result<StateUpdate> fromJson<StateUpdate>(const QJsonValue &val);

/** Returns the 'state' dispatch field value for the active variant. */
ACPLIB_EXPORT QString dispatchValue(const StateUpdate &val);

ACPLIB_EXPORT QJsonObject toJson(const StateUpdate &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const StateUpdate &val);

/** A chunk of bytes appended to an agent-owned terminal's output. */
struct TerminalOutputChunk {
    TerminalId _terminalId;  //!< The terminal receiving these bytes.
    QString _data;  //!< Independently base64-encoded terminal output bytes.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys. This field is chunk-scoped. Omitted and `null` are
     * equivalent and mean no chunk metadata was provided.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    TerminalOutputChunk& terminalId(const TerminalId & v) { _terminalId = v; return *this; }
    TerminalOutputChunk& data(const QString & v) { _data = v; return *this; }
    TerminalOutputChunk& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    TerminalOutputChunk& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const TerminalId& terminalId() const { return _terminalId; }
    const QString& data() const { return _data; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<TerminalOutputChunk> fromJson<TerminalOutputChunk>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const TerminalOutputChunk &data);

/**
 * Exit information for an agent-owned terminal.
 *
 * The presence of this object marks the terminal as exited, even when neither
 * an exit code nor a signal is known.
 */
struct TerminalExitStatus {
    Patch<int> _exitCode;  //!< Process exit code, when known. Omitted and `null` are equivalent.
    /**
     * Signal that terminated the process, when known.
     *
     * Agents should use the conventional platform signal name. POSIX examples
     * include `SIGTERM`, `SIGKILL`, and `SIGINT`. Other platforms may use a
     * platform-specific name. Omitted and `null` are equivalent.
     */
    Patch<QString> _signal;
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys. This metadata is scoped to the exit information. Omitted
     * and `null` are equivalent and mean no exit metadata was provided.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    TerminalExitStatus& exitCode(const Patch<int> & v) { _exitCode = v; return *this; }
    TerminalExitStatus& exitCode(int v) { _exitCode = v; return *this; }
    TerminalExitStatus& signal(const Patch<QString> & v) { _signal = v; return *this; }
    TerminalExitStatus& signal(const QString & v) { _signal = v; return *this; }
    TerminalExitStatus& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    TerminalExitStatus& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<int>& exitCode() const { return _exitCode; }
    const Patch<QString>& signal() const { return _signal; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<TerminalExitStatus> fromJson<TerminalExitStatus>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const TerminalExitStatus &data);

/** An authoritative replacement snapshot of terminal output bytes. */
struct TerminalOutput {
    QString _data;  //!< Base64-encoded replacement terminal output bytes.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys. This metadata is scoped to the replacement snapshot. Omitted
     * and `null` are equivalent and mean no snapshot metadata was provided.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    TerminalOutput& data(const QString & v) { _data = v; return *this; }
    TerminalOutput& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    TerminalOutput& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const QString& data() const { return _data; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<TerminalOutput> fromJson<TerminalOutput>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const TerminalOutput &data);

/**
 * An upsert for the stored state of an agent-owned terminal.
 *
 * Only [`TerminalUpdate::terminal_id`] is required. Other fields have patch
 * semantics: omitted fields leave the stored value unchanged, `null` clears
 * it, and concrete values replace it. When the terminal ID is new, omitted
 * fields start unknown.
 */
struct TerminalUpdate {
    TerminalId _terminalId;  //!< Unique identifier for this terminal within the session.
    Patch<QString> _command;  //!< The command being run.
    Patch<AbsolutePath> _cwd;  //!< The absolute working directory of the command.
    Patch<TerminalOutput> _output;  //!< An authoritative replacement snapshot of terminal output bytes.
    Patch<TerminalExitStatus> _exitStatus;  //!< Exit information. A concrete object marks the terminal as exited.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted means no metadata update; `null` is an
     * explicit clear signal. Implementations MUST NOT make assumptions about values at these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    TerminalUpdate& terminalId(const TerminalId & v) { _terminalId = v; return *this; }
    TerminalUpdate& command(const Patch<QString> & v) { _command = v; return *this; }
    TerminalUpdate& command(const QString & v) { _command = v; return *this; }
    TerminalUpdate& cwd(const Patch<AbsolutePath> & v) { _cwd = v; return *this; }
    TerminalUpdate& cwd(const AbsolutePath & v) { _cwd = v; return *this; }
    TerminalUpdate& output(const Patch<TerminalOutput> & v) { _output = v; return *this; }
    TerminalUpdate& output(const TerminalOutput & v) { _output = v; return *this; }
    TerminalUpdate& exitStatus(const Patch<TerminalExitStatus> & v) { _exitStatus = v; return *this; }
    TerminalUpdate& exitStatus(const TerminalExitStatus & v) { _exitStatus = v; return *this; }
    TerminalUpdate& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    TerminalUpdate& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const TerminalId& terminalId() const { return _terminalId; }
    const Patch<QString>& command() const { return _command; }
    const Patch<AbsolutePath>& cwd() const { return _cwd; }
    const Patch<TerminalOutput>& output() const { return _output; }
    const Patch<TerminalExitStatus>& exitStatus() const { return _exitStatus; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<TerminalUpdate> fromJson<TerminalUpdate>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const TerminalUpdate &data);

/**
 * A streamed item of tool-call content.
 *
 * Tool-call content chunks append one [`ToolCallContent`] item to the current
 * content for the matching [`ToolCallId`]. Agents can use
 * [`ToolCallUpdate::content`] when they need to replace the whole content
 * collection instead.
 */
struct ToolCallContentChunk {
    ToolCallId _toolCallId;  //!< The ID of the tool call this content belongs to.
    ToolCallContent _content;  //!< A single item of content produced by the tool call.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys. This field is chunk-scoped.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    ToolCallContentChunk& toolCallId(const ToolCallId & v) { _toolCallId = v; return *this; }
    ToolCallContentChunk& content(const ToolCallContent & v) { _content = v; return *this; }
    ToolCallContentChunk& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    ToolCallContentChunk& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const ToolCallId& toolCallId() const { return _toolCallId; }
    const ToolCallContent& content() const { return _content; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<ToolCallContentChunk> fromJson<ToolCallContentChunk>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ToolCallContentChunk &data);

/** Cost information for a session. */
struct Cost {
    double _amount;  //!< Total cumulative cost for session.
    QString _currency;  //!< ISO 4217 currency code (e.g., "USD", "EUR").
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    Cost& amount(double v) { _amount = v; return *this; }
    Cost& currency(const QString & v) { _currency = v; return *this; }
    Cost& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    Cost& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const double& amount() const { return _amount; }
    const QString& currency() const { return _currency; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<Cost> fromJson<Cost>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const Cost &data);

/** Context window and cost update for a session. */
struct UsageUpdate {
    int _used;  //!< Tokens currently in context.
    int _size;  //!< Total context window size in tokens.
    Patch<Cost> _cost;  //!< Cumulative session cost (optional).
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    UsageUpdate& used(int v) { _used = v; return *this; }
    UsageUpdate& size(int v) { _size = v; return *this; }
    UsageUpdate& cost(const Patch<Cost> & v) { _cost = v; return *this; }
    UsageUpdate& cost(const Cost & v) { _cost = v; return *this; }
    UsageUpdate& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    UsageUpdate& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const int& used() const { return _used; }
    const int& size() const { return _size; }
    const Patch<Cost>& cost() const { return _cost; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<UsageUpdate> fromJson<UsageUpdate>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const UsageUpdate &data);

/**
 * A user message upsert.
 *
 * Only [`UserMessage::message_id`] is required. `content` has patch semantics:
 * an omitted field leaves existing message content unchanged, `null` clears the
 * value, and a concrete array replaces the previous value. For a new
 * `messageId`, omitted fields use client defaults. `content` is replaced as a
 * whole array; send `[]` or `null` to clear it.
 *
 * Message updates and chunks are applied in the order they are received. When
 * a `user_message` update includes `content`, that array replaces any content
 * previously accumulated for the message, including content from earlier
 * chunks. Later chunks with the same `messageId` append to the current
 * content.
 */
struct UserMessage {
    MessageId _messageId;  //!< A unique identifier for the message.
    Patch<QList<ContentBlock>> _content;  //!< Complete replacement content for this message.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys. Omitted means no metadata update; `null` is an explicit clear signal.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    UserMessage& messageId(const MessageId & v) { _messageId = v; return *this; }
    UserMessage& content(const Patch<QList<ContentBlock>> & v) { _content = v; return *this; }
    UserMessage& content(const QList<ContentBlock> & v) { _content = v; return *this; }
    UserMessage& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    UserMessage& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const MessageId& messageId() const { return _messageId; }
    const Patch<QList<ContentBlock>>& content() const { return _content; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<UserMessage> fromJson<UserMessage>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const UserMessage &data);

/**
 * Different types of updates that can be sent while a session exists.
 *
 * These updates report messages, progress, and other session activity.
 *
 * See protocol docs: [Agent Reports Output](https://agentclientprotocol.com/protocol/v2/prompt-lifecycle#3-agent-reports-output)
 */
struct SessionUpdate {
    using Variant = std::variant<ContentChunk, UserMessage, AgentMessage, AgentThought, StateUpdate, ToolCallContentChunk, ToolCallUpdate, TerminalUpdate, TerminalOutputChunk, PlanUpdate, AvailableCommandsUpdate, ConfigOptionUpdate, SessionInfoUpdate, UsageUpdate, QJsonObject>;
    Variant _value;
    QString _kind;  //!< discriminator value (sessionUpdate)

    template<typename T> const T* get() const { return std::get_if<T>(&_value); }
    const QString& kind() const { return _kind; }
};

template<>
ACPLIB_EXPORT Utils::Result<SessionUpdate> fromJson<SessionUpdate>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionUpdate &data);

ACPLIB_EXPORT QJsonValue toJsonValue(const SessionUpdate &val);

/**
 * Notification containing a session update from the agent.
 *
 * Agents can send session updates at any point while the session exists.
 *
 * See protocol docs: [Agent Reports Output](https://agentclientprotocol.com/protocol/v2/prompt-lifecycle#3-agent-reports-output)
 */
struct UpdateSessionNotification {
    SessionId _sessionId;  //!< The ID of the session this update pertains to.
    SessionUpdate _update;  //!< The actual update content.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    UpdateSessionNotification& sessionId(const SessionId & v) { _sessionId = v; return *this; }
    UpdateSessionNotification& update(const SessionUpdate & v) { _update = v; return *this; }
    UpdateSessionNotification& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    UpdateSessionNotification& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const SessionId& sessionId() const { return _sessionId; }
    const SessionUpdate& update() const { return _update; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<UpdateSessionNotification> fromJson<UpdateSessionNotification>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const UpdateSessionNotification &data);

/** A JSON-RPC notification object. */
struct AgentNotification {
    QString _method;  //!< The notification method name.
    std::optional<QString> _params;  //!< Method-specific notification parameters.

    AgentNotification& method(const QString & v) { _method = v; return *this; }
    AgentNotification& params(const std::optional<QString> & v) { _params = v; return *this; }

    const QString& method() const { return _method; }
    const std::optional<QString>& params() const { return _params; }
};

template<>
ACPLIB_EXPORT Utils::Result<AgentNotification> fromJson<AgentNotification>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const AgentNotification &data);

/**
 * Request parameters for closing an active session.
 *
 * The agent **must** cancel any ongoing work related to the session (treat it
 * as if `session/cancel` was called) and then free up any resources associated
 * with the session.
 */
struct CloseSessionRequest {
    SessionId _sessionId;  //!< The ID of the session to close.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    CloseSessionRequest& sessionId(const SessionId & v) { _sessionId = v; return *this; }
    CloseSessionRequest& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    CloseSessionRequest& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const SessionId& sessionId() const { return _sessionId; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<CloseSessionRequest> fromJson<CloseSessionRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const CloseSessionRequest &data);

/**
 * Request parameters for deleting an existing session from `session/list`.
 *
 * Only available if the Agent supports the `session.delete` capability.
 */
struct DeleteSessionRequest {
    SessionId _sessionId;  //!< The ID of the session to delete.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    DeleteSessionRequest& sessionId(const SessionId & v) { _sessionId = v; return *this; }
    DeleteSessionRequest& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    DeleteSessionRequest& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const SessionId& sessionId() const { return _sessionId; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<DeleteSessionRequest> fromJson<DeleteSessionRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const DeleteSessionRequest &data);

/**
 * Form-based elicitation capabilities.
 *
 * Supplying `{}` means the client supports form-based elicitation.
 */
struct ElicitationFormCapabilities {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * Optional. Omitted and `null` are equivalent and mean no metadata.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    ElicitationFormCapabilities& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    ElicitationFormCapabilities& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<ElicitationFormCapabilities> fromJson<ElicitationFormCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ElicitationFormCapabilities &data);

/**
 * URL-based elicitation capabilities.
 *
 * Supplying `{}` means the client supports URL-based elicitation.
 */
struct ElicitationUrlCapabilities {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * Optional. Omitted and `null` are equivalent and mean no metadata.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    ElicitationUrlCapabilities& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    ElicitationUrlCapabilities& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<ElicitationUrlCapabilities> fromJson<ElicitationUrlCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ElicitationUrlCapabilities &data);

/** Elicitation capabilities supported by the client. */
struct ElicitationCapabilities {
    /**
     * Whether the client supports form-based elicitation.
     *
     * Optional. Omitted and `null` are equivalent and mean form support is not advertised.
     * Supplying `{}` explicitly advertises form support.
     */
    Patch<ElicitationFormCapabilities> _form;
    /**
     * Whether the client supports URL-based elicitation.
     *
     * Optional. Omitted or `null` both mean the client does not advertise support.
     * Supplying `{}` means the client supports URL-based elicitation.
     */
    Patch<ElicitationUrlCapabilities> _url;
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * Optional. Omitted and `null` are equivalent and mean no metadata.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    ElicitationCapabilities& form(const Patch<ElicitationFormCapabilities> & v) { _form = v; return *this; }
    ElicitationCapabilities& form(const ElicitationFormCapabilities & v) { _form = v; return *this; }
    ElicitationCapabilities& url(const Patch<ElicitationUrlCapabilities> & v) { _url = v; return *this; }
    ElicitationCapabilities& url(const ElicitationUrlCapabilities & v) { _url = v; return *this; }
    ElicitationCapabilities& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    ElicitationCapabilities& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<ElicitationFormCapabilities>& form() const { return _form; }
    const Patch<ElicitationUrlCapabilities>& url() const { return _url; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<ElicitationCapabilities> fromJson<ElicitationCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ElicitationCapabilities &data);

/**
 * Capabilities supported by the client.
 *
 * Advertised during initialization to inform the agent about
 * available features and methods.
 *
 * See protocol docs: [Client Capabilities](https://agentclientprotocol.com/protocol/v2/initialization#client-capabilities)
 */
struct ClientCapabilities {
    /**
     * Elicitation capabilities supported by the client.
     * Determines which elicitation modes the agent may use.
     *
     * Optional. Omitted or `null` both mean the client does not advertise
     * elicitation support.
     */
    Patch<ElicitationCapabilities> _elicitation;
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    ClientCapabilities& elicitation(const Patch<ElicitationCapabilities> & v) { _elicitation = v; return *this; }
    ClientCapabilities& elicitation(const ElicitationCapabilities & v) { _elicitation = v; return *this; }
    ClientCapabilities& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    ClientCapabilities& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<ElicitationCapabilities>& elicitation() const { return _elicitation; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<ClientCapabilities> fromJson<ClientCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ClientCapabilities &data);

/**
 * Request parameters for the initialize method.
 *
 * Sent by the client to establish connection and negotiate capabilities.
 *
 * See protocol docs: [Initialization](https://agentclientprotocol.com/protocol/v2/initialization)
 */
struct InitializeRequest {
    ProtocolVersion _protocolVersion;  //!< The latest protocol version supported by the client.
    Implementation _info;  //!< Information about the implementation sending this initialize request.
    std::optional<ClientCapabilities> _capabilities;  //!< Capabilities supported by the client.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    InitializeRequest& protocolVersion(const ProtocolVersion & v) { _protocolVersion = v; return *this; }
    InitializeRequest& info(const Implementation & v) { _info = v; return *this; }
    InitializeRequest& capabilities(const std::optional<ClientCapabilities> & v) { _capabilities = v; return *this; }
    InitializeRequest& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    InitializeRequest& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const ProtocolVersion& protocolVersion() const { return _protocolVersion; }
    const Implementation& info() const { return _info; }
    const std::optional<ClientCapabilities>& capabilities() const { return _capabilities; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<InitializeRequest> fromJson<InitializeRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const InitializeRequest &data);

/** Request parameters for listing existing sessions. */
struct ListSessionsRequest {
    Patch<AbsolutePath> _cwd;  //!< Filter sessions by working directory. Must be an absolute path.
    Patch<SessionListCursor> _cursor;  //!< Opaque cursor token from a previous response's nextCursor field for cursor-based pagination
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    ListSessionsRequest& cwd(const Patch<AbsolutePath> & v) { _cwd = v; return *this; }
    ListSessionsRequest& cwd(const AbsolutePath & v) { _cwd = v; return *this; }
    ListSessionsRequest& cursor(const Patch<SessionListCursor> & v) { _cursor = v; return *this; }
    ListSessionsRequest& cursor(const SessionListCursor & v) { _cursor = v; return *this; }
    ListSessionsRequest& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    ListSessionsRequest& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<AbsolutePath>& cwd() const { return _cwd; }
    const Patch<SessionListCursor>& cursor() const { return _cursor; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<ListSessionsRequest> fromJson<ListSessionsRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ListSessionsRequest &data);

/**
 * Request parameters for the `auth/login` method.
 *
 * Specifies which authentication method to use.
 *
 * Agents MUST support this method when their `initialize` response advertised
 * at least one valid authentication method. Clients MUST NOT call this method
 * when `authMethods` was omitted or empty.
 */
struct LoginAuthRequest {
    /**
     * The ID of the authentication method to use.
     * Must be one of the methods advertised in the initialize response.
     */
    AuthMethodId _methodId;
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    LoginAuthRequest& methodId(const AuthMethodId & v) { _methodId = v; return *this; }
    LoginAuthRequest& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    LoginAuthRequest& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const AuthMethodId& methodId() const { return _methodId; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<LoginAuthRequest> fromJson<LoginAuthRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const LoginAuthRequest &data);

/**
 * Request parameters for the `auth/logout` method.
 *
 * Terminates the current authenticated session.
 *
 * Agents MUST support this method when their `initialize` response advertised
 * at least one valid authentication method. Clients MUST NOT call this method
 * when `authMethods` was omitted or empty.
 */
struct LogoutAuthRequest {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    LogoutAuthRequest& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    LogoutAuthRequest& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<LogoutAuthRequest> fromJson<LogoutAuthRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const LogoutAuthRequest &data);

/** An HTTP header to set when making requests to the MCP server. */
struct HttpHeader {
    QString _name;  //!< The name of the HTTP header.
    QString _value;  //!< The value to set for the HTTP header.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    HttpHeader& name(const QString & v) { _name = v; return *this; }
    HttpHeader& value(const QString & v) { _value = v; return *this; }
    HttpHeader& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    HttpHeader& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const QString& name() const { return _name; }
    const QString& value() const { return _value; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<HttpHeader> fromJson<HttpHeader>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const HttpHeader &data);

/** HTTP transport configuration for MCP. */
struct McpServerHttp {
    QString _name;  //!< Human-readable name identifying this MCP server.
    QString _url;  //!< URL to the MCP server.
    std::optional<QList<HttpHeader>> _headers;  //!< HTTP headers to set when making requests to the MCP server.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    McpServerHttp& name(const QString & v) { _name = v; return *this; }
    McpServerHttp& url(const QString & v) { _url = v; return *this; }
    McpServerHttp& headers(const std::optional<QList<HttpHeader>> & v) { _headers = v; return *this; }
    McpServerHttp& addHeader(const HttpHeader & v) { if (!_headers) _headers = QList<HttpHeader>{}; (*_headers).append(v); return *this; }
    McpServerHttp& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    McpServerHttp& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const QString& name() const { return _name; }
    const QString& url() const { return _url; }
    const std::optional<QList<HttpHeader>>& headers() const { return _headers; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<McpServerHttp> fromJson<McpServerHttp>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const McpServerHttp &data);

/** An environment variable to set when launching a process. */
struct EnvVariable {
    QString _name;  //!< The name of the environment variable.
    QString _value;  //!< The value to set for the environment variable.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    EnvVariable& name(const QString & v) { _name = v; return *this; }
    EnvVariable& value(const QString & v) { _value = v; return *this; }
    EnvVariable& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    EnvVariable& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const QString& name() const { return _name; }
    const QString& value() const { return _value; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<EnvVariable> fromJson<EnvVariable>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const EnvVariable &data);

/** Stdio transport configuration for MCP. */
struct McpServerStdio {
    QString _name;  //!< Human-readable name identifying this MCP server.
    AbsolutePath _command;  //!< Absolute path to the MCP server executable.
    std::optional<QStringList> _args;  //!< Command-line arguments to pass to the MCP server.
    std::optional<QList<EnvVariable>> _env;  //!< Environment variables to set when launching the MCP server.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    McpServerStdio& name(const QString & v) { _name = v; return *this; }
    McpServerStdio& command(const AbsolutePath & v) { _command = v; return *this; }
    McpServerStdio& args(const std::optional<QStringList> & v) { _args = v; return *this; }
    McpServerStdio& addArg(const QString & v) { if (!_args) _args = QStringList{}; (*_args).append(v); return *this; }
    McpServerStdio& env(const std::optional<QList<EnvVariable>> & v) { _env = v; return *this; }
    McpServerStdio& addEnv(const EnvVariable & v) { if (!_env) _env = QList<EnvVariable>{}; (*_env).append(v); return *this; }
    McpServerStdio& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    McpServerStdio& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const QString& name() const { return _name; }
    const AbsolutePath& command() const { return _command; }
    const std::optional<QStringList>& args() const { return _args; }
    const std::optional<QList<EnvVariable>>& env() const { return _env; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<McpServerStdio> fromJson<McpServerStdio>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const McpServerStdio &data);

/**
 * Configuration for connecting to an MCP (Model Context Protocol) server.
 *
 * MCP servers provide tools and context that the agent can use when
 * processing prompts.
 *
 * See protocol docs: [MCP Servers](https://agentclientprotocol.com/protocol/v2/session-setup#mcp-servers)
 */
using McpServer = std::variant<McpServerHttp, McpServerStdio, QJsonObject>;

template<>
ACPLIB_EXPORT Utils::Result<McpServer> fromJson<McpServer>(const QJsonValue &val);

/** Returns the 'type' dispatch field value for the active variant. */
ACPLIB_EXPORT QString dispatchValue(const McpServer &val);

ACPLIB_EXPORT QJsonObject toJson(const McpServer &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const McpServer &val);

/**
 * Request parameters for creating a new session.
 *
 * See protocol docs: [Creating a Session](https://agentclientprotocol.com/protocol/v2/session-setup#creating-a-session)
 */
struct NewSessionRequest {
    AbsolutePath _cwd;  //!< The working directory for this session. Must be an absolute path.
    /**
     * Additional workspace roots for this session. Each path must be absolute.
     *
     * These expand the session's workspace scope without changing `cwd`, which
     * remains the base for relative paths. When omitted or empty, no
     * additional roots are activated for the new session.
     */
    std::optional<QList<AbsolutePath>> _additionalDirectories;
    std::optional<QList<McpServer>> _mcpServers;  //!< List of MCP (Model Context Protocol) servers the agent should connect to.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    NewSessionRequest& cwd(const AbsolutePath & v) { _cwd = v; return *this; }
    NewSessionRequest& additionalDirectories(const std::optional<QList<AbsolutePath>> & v) { _additionalDirectories = v; return *this; }
    NewSessionRequest& addAdditionalDirectory(const AbsolutePath & v) { if (!_additionalDirectories) _additionalDirectories = QList<AbsolutePath>{}; (*_additionalDirectories).append(v); return *this; }
    NewSessionRequest& mcpServers(const std::optional<QList<McpServer>> & v) { _mcpServers = v; return *this; }
    NewSessionRequest& addMcpServer(const McpServer & v) { if (!_mcpServers) _mcpServers = QList<McpServer>{}; (*_mcpServers).append(v); return *this; }
    NewSessionRequest& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    NewSessionRequest& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const AbsolutePath& cwd() const { return _cwd; }
    const std::optional<QList<AbsolutePath>>& additionalDirectories() const { return _additionalDirectories; }
    const std::optional<QList<McpServer>>& mcpServers() const { return _mcpServers; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<NewSessionRequest> fromJson<NewSessionRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const NewSessionRequest &data);

/**
 * Request parameters for sending a user prompt to the agent.
 *
 * Contains the user's message and any additional context.
 *
 * See protocol docs: [User Message](https://agentclientprotocol.com/protocol/v2/prompt-lifecycle#1-user-message)
 */
struct PromptRequest {
    SessionId _sessionId;  //!< The ID of the session to send this user message to
    /**
     * The blocks of content that compose the user's message.
     *
     * As a baseline, the Agent MUST support [`ContentBlock::Text`] and [`ContentBlock::ResourceLink`],
     * while other variants are optionally enabled via [`PromptCapabilities`].
     *
     * The Client MUST adapt its interface according to [`PromptCapabilities`].
     *
     * The client MAY include referenced pieces of context as either
     * [`ContentBlock::Resource`] or [`ContentBlock::ResourceLink`].
     *
     * When available, [`ContentBlock::Resource`] is preferred
     * as it avoids extra round-trips and allows the message to include
     * pieces of context from sources the agent may not have access to.
     */
    QList<ContentBlock> _prompt;
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    PromptRequest& sessionId(const SessionId & v) { _sessionId = v; return *this; }
    PromptRequest& prompt(const QList<ContentBlock> & v) { _prompt = v; return *this; }
    PromptRequest& addPrompt(const ContentBlock & v) { _prompt.append(v); return *this; }
    PromptRequest& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    PromptRequest& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const SessionId& sessionId() const { return _sessionId; }
    const QList<ContentBlock>& prompt() const { return _prompt; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<PromptRequest> fromJson<PromptRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const PromptRequest &data);

/** Inclusive replay cursor requesting replay from the start of the conversation. */
struct ReplayFromStart {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    ReplayFromStart& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    ReplayFromStart& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<ReplayFromStart> fromJson<ReplayFromStart>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ReplayFromStart &data);

/**
 * Inclusive cursor describing where replayed session history should begin.
 *
 * Replay includes the position identified by the cursor.
 */
using ReplayFrom = std::variant<ReplayFromStart, QJsonObject>;

template<>
ACPLIB_EXPORT Utils::Result<ReplayFrom> fromJson<ReplayFrom>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ReplayFrom &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const ReplayFrom &val);

/**
 * Request parameters for resuming an existing session.
 *
 * Resumes an existing session and optionally replays prior conversation
 * history according to `replayFrom`.
 */
struct ResumeSessionRequest {
    SessionId _sessionId;  //!< The ID of the session to resume.
    AbsolutePath _cwd;  //!< The working directory for this session. Must be an absolute path.
    /**
     * Additional workspace roots to activate for this session. Each path must be absolute.
     *
     * When omitted or empty, no additional roots are activated. When non-empty,
     * this is the complete resulting additional-root list for the resumed
     * session. It may differ from any previously used or reported list as long as
     * the request `cwd` matches the session's `cwd`.
     */
    std::optional<QList<AbsolutePath>> _additionalDirectories;
    std::optional<QList<McpServer>> _mcpServers;  //!< List of MCP servers to connect to for this session.
    /**
     * Inclusive cursor describing where conversation replay should begin.
     *
     * Optional. Omitted or `null` both mean the Agent should resume without
     * replaying previous conversation history. Replay cursors are inclusive:
     * replay includes the position identified by the cursor. Supplying
     * `{ "type": "start" }` means the Agent should replay the whole
     * conversation before responding.
     */
    Patch<ReplayFrom> _replayFrom;
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    ResumeSessionRequest& sessionId(const SessionId & v) { _sessionId = v; return *this; }
    ResumeSessionRequest& cwd(const AbsolutePath & v) { _cwd = v; return *this; }
    ResumeSessionRequest& additionalDirectories(const std::optional<QList<AbsolutePath>> & v) { _additionalDirectories = v; return *this; }
    ResumeSessionRequest& addAdditionalDirectory(const AbsolutePath & v) { if (!_additionalDirectories) _additionalDirectories = QList<AbsolutePath>{}; (*_additionalDirectories).append(v); return *this; }
    ResumeSessionRequest& mcpServers(const std::optional<QList<McpServer>> & v) { _mcpServers = v; return *this; }
    ResumeSessionRequest& addMcpServer(const McpServer & v) { if (!_mcpServers) _mcpServers = QList<McpServer>{}; (*_mcpServers).append(v); return *this; }
    ResumeSessionRequest& replayFrom(const Patch<ReplayFrom> & v) { _replayFrom = v; return *this; }
    ResumeSessionRequest& replayFrom(const ReplayFrom & v) { _replayFrom = v; return *this; }
    ResumeSessionRequest& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    ResumeSessionRequest& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const SessionId& sessionId() const { return _sessionId; }
    const AbsolutePath& cwd() const { return _cwd; }
    const std::optional<QList<AbsolutePath>>& additionalDirectories() const { return _additionalDirectories; }
    const std::optional<QList<McpServer>>& mcpServers() const { return _mcpServers; }
    const Patch<ReplayFrom>& replayFrom() const { return _replayFrom; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<ResumeSessionRequest> fromJson<ResumeSessionRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ResumeSessionRequest &data);

/** Request parameters for setting a session configuration option. */
struct SetSessionConfigOptionRequest {
    SessionId _sessionId;  //!< The ID of the session to set the configuration option for.
    SessionConfigId _configId;  //!< The ID of the configuration option to set.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;
    QJsonObject _additionalProperties;  //!< additional properties

    SetSessionConfigOptionRequest& sessionId(const SessionId & v) { _sessionId = v; return *this; }
    SetSessionConfigOptionRequest& configId(const SessionConfigId & v) { _configId = v; return *this; }
    SetSessionConfigOptionRequest& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    SetSessionConfigOptionRequest& _meta(const QJsonObject & v) { __meta = v; return *this; }
    SetSessionConfigOptionRequest& additionalProperties(const QString &key, const QJsonValue &v) { _additionalProperties.insert(key, v); return *this; }
    SetSessionConfigOptionRequest& additionalProperties(const QJsonObject &obj) { for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) _additionalProperties.insert(it.key(), it.value()); return *this; }

    const SessionId& sessionId() const { return _sessionId; }
    const SessionConfigId& configId() const { return _configId; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
    const QJsonObject& additionalProperties() const { return _additionalProperties; }
};

template<>
ACPLIB_EXPORT Utils::Result<SetSessionConfigOptionRequest> fromJson<SetSessionConfigOptionRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SetSessionConfigOptionRequest &data);

/** A JSON-RPC request object. */
struct ClientRequest {
    RequestId _id;  //!< The request id used to correlate the matching response.
    QString _method;  //!< The method name to invoke.
    std::optional<QString> _params;  //!< Method-specific request parameters.

    ClientRequest& id(const RequestId & v) { _id = v; return *this; }
    ClientRequest& method(const QString & v) { _method = v; return *this; }
    ClientRequest& params(const std::optional<QString> & v) { _params = v; return *this; }

    const RequestId& id() const { return _id; }
    const QString& method() const { return _method; }
    const std::optional<QString>& params() const { return _params; }
};

template<>
ACPLIB_EXPORT Utils::Result<ClientRequest> fromJson<ClientRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ClientRequest &data);

/** Allowed wire representations for [`ElicitationContentValue`]. */
using ElicitationContentValue = std::variant<QString, int, double, bool, QList<QString>>;

template<>
ACPLIB_EXPORT Utils::Result<ElicitationContentValue> fromJson<ElicitationContentValue>(const QJsonValue &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const ElicitationContentValue &val);

/** The user accepted the elicitation and provided content. */
struct ElicitationAcceptAction {
    Patch<QJsonObject> _content;  //!< The user-provided content, if any, as an object matching the requested schema.

    ElicitationAcceptAction& content(const Patch<QJsonObject> & v) { _content = v; return *this; }
    ElicitationAcceptAction& content(const QJsonObject & v) { _content = v; return *this; }

    const Patch<QJsonObject>& content() const { return _content; }
};

template<>
ACPLIB_EXPORT Utils::Result<ElicitationAcceptAction> fromJson<ElicitationAcceptAction>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ElicitationAcceptAction &data);

/** Response from the client to an elicitation request. */
struct CreateElicitationResponse {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * Optional. Omitted and `null` are equivalent and mean no metadata.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;
    QJsonObject _additionalProperties;  //!< additional properties

    CreateElicitationResponse& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    CreateElicitationResponse& _meta(const QJsonObject & v) { __meta = v; return *this; }
    CreateElicitationResponse& additionalProperties(const QString &key, const QJsonValue &v) { _additionalProperties.insert(key, v); return *this; }
    CreateElicitationResponse& additionalProperties(const QJsonObject &obj) { for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) _additionalProperties.insert(it.key(), it.value()); return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }
    const QJsonObject& additionalProperties() const { return _additionalProperties; }
};

template<>
ACPLIB_EXPORT Utils::Result<CreateElicitationResponse> fromJson<CreateElicitationResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const CreateElicitationResponse &data);

/** The user selected one of the provided options. */
struct SelectedPermissionOutcome {
    PermissionOptionId _optionId;  //!< The ID of the option the user selected.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    SelectedPermissionOutcome& optionId(const PermissionOptionId & v) { _optionId = v; return *this; }
    SelectedPermissionOutcome& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    SelectedPermissionOutcome& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const PermissionOptionId& optionId() const { return _optionId; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<SelectedPermissionOutcome> fromJson<SelectedPermissionOutcome>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SelectedPermissionOutcome &data);

/** The outcome of a permission request. */
struct RequestPermissionOutcome {
    using Variant = std::variant<std::monostate, SelectedPermissionOutcome, QJsonObject>;
    Variant _value;
    QString _kind;  //!< discriminator value (outcome)

    template<typename T> const T* get() const { return std::get_if<T>(&_value); }
    const QString& kind() const { return _kind; }
};

template<>
ACPLIB_EXPORT Utils::Result<RequestPermissionOutcome> fromJson<RequestPermissionOutcome>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const RequestPermissionOutcome &data);

ACPLIB_EXPORT QJsonValue toJsonValue(const RequestPermissionOutcome &val);

/** Response to a permission request. */
struct RequestPermissionResponse {
    RequestPermissionOutcome _outcome;  //!< The user's decision on the permission request.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    RequestPermissionResponse& outcome(const RequestPermissionOutcome & v) { _outcome = v; return *this; }
    RequestPermissionResponse& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    RequestPermissionResponse& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const RequestPermissionOutcome& outcome() const { return _outcome; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<RequestPermissionResponse> fromJson<RequestPermissionResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const RequestPermissionResponse &data);

/** A JSON-RPC response object. */
using ClientResponse = std::variant<QJsonObject>;

/**
 * Notification to cancel ongoing operations for a session.
 *
 * See protocol docs: [Cancellation](https://agentclientprotocol.com/protocol/v2/prompt-lifecycle#cancellation)
 */
struct CancelSessionNotification {
    SessionId _sessionId;  //!< The ID of the session to cancel operations for.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    CancelSessionNotification& sessionId(const SessionId & v) { _sessionId = v; return *this; }
    CancelSessionNotification& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    CancelSessionNotification& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const SessionId& sessionId() const { return _sessionId; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<CancelSessionNotification> fromJson<CancelSessionNotification>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const CancelSessionNotification &data);

/** A JSON-RPC notification object. */
struct ClientNotification {
    QString _method;  //!< The notification method name.
    std::optional<QString> _params;  //!< Method-specific notification parameters.

    ClientNotification& method(const QString & v) { _method = v; return *this; }
    ClientNotification& params(const std::optional<QString> & v) { _params = v; return *this; }

    const QString& method() const { return _method; }
    const std::optional<QString>& params() const { return _params; }
};

template<>
ACPLIB_EXPORT Utils::Result<ClientNotification> fromJson<ClientNotification>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ClientNotification &data);

/**
 * Notification to cancel an ongoing request.
 *
 * See protocol docs: [Cancellation](https://agentclientprotocol.com/protocol/v2/cancellation)
 */
struct CancelRequestNotification {
    RequestId _requestId;  //!< The ID of the request to cancel.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta;

    CancelRequestNotification& requestId(const RequestId & v) { _requestId = v; return *this; }
    CancelRequestNotification& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    CancelRequestNotification& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const RequestId& requestId() const { return _requestId; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<CancelRequestNotification> fromJson<CancelRequestNotification>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const CancelRequestNotification &data);

/** A JSON-RPC notification object. */
struct ProtocolLevelNotification {
    QString _method;  //!< The notification method name.
    std::optional<QString> _params;  //!< Method-specific notification parameters.

    ProtocolLevelNotification& method(const QString & v) { _method = v; return *this; }
    ProtocolLevelNotification& params(const std::optional<QString> & v) { _params = v; return *this; }

    const QString& method() const { return _method; }
    const std::optional<QString>& params() const { return _params; }
};

template<>
ACPLIB_EXPORT Utils::Result<ProtocolLevelNotification> fromJson<ProtocolLevelNotification>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ProtocolLevelNotification &data);

} // namespace Acp::V2
