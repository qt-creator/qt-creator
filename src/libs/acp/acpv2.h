/*
 This file is auto-generated. Do not edit manually.
 Generated with:

 python3 \
  scripts/generate_cpp_from_schema.py \
  src/libs/acp/schema/schema-v2.json src/libs/acp/acpv2.h --namespace Acp::V2 --cpp-output src/libs/acp/acpv2.cpp --export-macro ACPLIB_EXPORT --export-header acp_global.h --three-state --no-cxx20
*/
#pragma once

#include "acp_global.h"

#include <utils/result.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QMap>
#include <QSet>
#include <QString>
#include <QVariant>

#include <cmath>
#include <limits>
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

    bool operator==(const Patch &other) const = default;

private:
    bool m_null = false;
    std::optional<T> m_value;
};

template<typename T> Utils::Result<T> fromJson(const QJsonValue &val) = delete;

// Defs that carry no constraints beyond "an object" alias to QJsonObject; these
// let such aliases take part in the generated conversions unchanged.
template<> inline Utils::Result<QJsonObject> fromJson<QJsonObject>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError(QString("Expected JSON object"));
    return val.toObject();
}

inline QJsonObject toJson(const QJsonObject &data) { return data; }

template<typename T>
Utils::Result<T> fromJson(const QString &field, const QJsonValue &val)
{
    const Utils::Result<T> result = fromJson<T>(val);
    if (result)
        return result;
    return Utils::ResultError(field + ": " + result.error());
}

/**
 * A JSON-RPC request ID: a string or an integer. Unlike base JSON-RPC, it MUST NOT be `null`.
 *
 * The receiver MUST reply with the same value in the response. Each side chooses IDs for its own
 * requests, so the same ID can be in use in both directions. A sender MUST NOT reuse an ID while
 * a request with it is still awaiting a response.
 */
using RequestId = std::variant<int, QString>;

template<>
ACPLIB_EXPORT Utils::Result<RequestId> fromJson<RequestId>(const QJsonValue &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const RequestId &val);

/**
 * Request-scoped elicitation, tied to a specific JSON-RPC request outside of a session
 * (e.g., during auth/configuration phases before any session is started).
 */
struct ElicitationRequestScope {
    RequestId _requestId{};  //!< The pending Client-to-Agent request this elicitation is tied to.

    ElicitationRequestScope& requestId(const RequestId & v) { _requestId = v; return *this; }

    const RequestId& requestId() const { return _requestId; }

    bool operator==(const ElicitationRequestScope &other) const = default;
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
    Patch<QString> _title{};
    /**
     * Human-readable description.
     *
     * Optional. Omitted and `null` are equivalent and mean no description is provided.
     */
    Patch<QString> _description{};
    /**
     * Default value.
     *
     * Optional. Omitted and `null` are equivalent and mean no default value is provided.
     */
    Patch<bool> _default_{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

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

    bool operator==(const BooleanPropertySchema &other) const = default;
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
    Patch<QString> _title{};
    /**
     * Human-readable description.
     *
     * Optional. Omitted and `null` are equivalent and mean no description is provided.
     */
    Patch<QString> _description{};
    /**
     * Minimum value (inclusive).
     *
     * Optional. Omitted and `null` are equivalent and mean there is no inclusive lower bound.
     */
    Patch<int> _minimum{};
    /**
     * Maximum value (inclusive).
     *
     * Optional. Omitted and `null` are equivalent and mean there is no inclusive upper bound.
     */
    Patch<int> _maximum{};
    /**
     * Default value.
     *
     * Optional. Omitted and `null` are equivalent and mean no default value is provided.
     */
    Patch<int> _default_{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

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

    bool operator==(const IntegerPropertySchema &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<IntegerPropertySchema> fromJson<IntegerPropertySchema>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const IntegerPropertySchema &data);

/** String item schema for multi-select enum properties. */
struct StringMultiSelectItems {
    QStringList _enum_{};  //!< Allowed enum values. Must contain at least one value.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    StringMultiSelectItems& enum_(const QStringList & v) { _enum_ = v; return *this; }
    StringMultiSelectItems& addEnum(const QString & v) { _enum_.append(v); return *this; }
    StringMultiSelectItems& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    StringMultiSelectItems& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const QStringList& enum_() const { return _enum_; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const StringMultiSelectItems &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<StringMultiSelectItems> fromJson<StringMultiSelectItems>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const StringMultiSelectItems &data);

/** A titled enum option with a const value, human-readable title, and optional description. */
struct EnumOption {
    QString _const_{};  //!< The constant value for this option.
    QString _title{};  //!< Human-readable title for this option.
    /**
     * Human-readable description.
     *
     * Optional. Omitted and `null` are equivalent and mean no description is provided.
     */
    Patch<QString> _description{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

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

    bool operator==(const EnumOption &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<EnumOption> fromJson<EnumOption>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const EnumOption &data);

/** Items definition for titled multi-select enum properties. */
struct TitledMultiSelectItems {
    QList<EnumOption> _anyOf{};  //!< Titled enum options. Must contain at least one option.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    TitledMultiSelectItems& anyOf(const QList<EnumOption> & v) { _anyOf = v; return *this; }
    TitledMultiSelectItems& addAnyOf(const EnumOption & v) { _anyOf.append(v); return *this; }
    TitledMultiSelectItems& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    TitledMultiSelectItems& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const QList<EnumOption>& anyOf() const { return _anyOf; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const TitledMultiSelectItems &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<TitledMultiSelectItems> fromJson<TitledMultiSelectItems>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const TitledMultiSelectItems &data);

/** Items for a multi-select (array) property schema. */
using MultiSelectItems = std::variant<StringMultiSelectItems, TitledMultiSelectItems, QJsonObject>;

template<>
ACPLIB_EXPORT Utils::Result<MultiSelectItems> fromJson<MultiSelectItems>(const QJsonValue &val);

/** Returns the 'type' dispatch field value for the active variant. */
ACPLIB_EXPORT QString dispatchValue(const MultiSelectItems &val);

ACPLIB_EXPORT QJsonObject toJson(const MultiSelectItems &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const MultiSelectItems &val);

/** Schema for multi-select (array) properties in an elicitation form. */
struct MultiSelectPropertySchema {
    /**
     * Optional title for the property.
     *
     * Optional. Omitted and `null` are equivalent and mean no title is provided.
     */
    Patch<QString> _title{};
    /**
     * Human-readable description.
     *
     * Optional. Omitted and `null` are equivalent and mean no description is provided.
     */
    Patch<QString> _description{};
    /**
     * Minimum number of items to select.
     *
     * Optional. Omitted and `null` are equivalent and mean there is no minimum selection count.
     */
    Patch<int> _minItems{};
    /**
     * Maximum number of items to select.
     *
     * Optional. Omitted and `null` are equivalent and mean there is no maximum selection count.
     */
    Patch<int> _maxItems{};
    MultiSelectItems _items{};  //!< The items definition describing allowed values.
    /**
     * Default selected values.
     *
     * Optional. Omitted and `null` are equivalent and mean no default selections are provided.
     */
    Patch<QJsonArray> _default_{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

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

    bool operator==(const MultiSelectPropertySchema &other) const = default;
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
    Patch<QString> _title{};
    /**
     * Human-readable description.
     *
     * Optional. Omitted and `null` are equivalent and mean no description is provided.
     */
    Patch<QString> _description{};
    /**
     * Minimum value (inclusive).
     *
     * Optional. Omitted and `null` are equivalent and mean there is no inclusive lower bound.
     */
    Patch<double> _minimum{};
    /**
     * Maximum value (inclusive).
     *
     * Optional. Omitted and `null` are equivalent and mean there is no inclusive upper bound.
     */
    Patch<double> _maximum{};
    /**
     * Default value.
     *
     * Optional. Omitted and `null` are equivalent and mean no default value is provided.
     */
    Patch<double> _default_{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

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

    bool operator==(const NumberPropertySchema &other) const = default;
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
    Patch<QString> _title{};
    /**
     * Human-readable description.
     *
     * Optional. Omitted and `null` are equivalent and mean no description is provided.
     */
    Patch<QString> _description{};
    /**
     * Minimum string length.
     *
     * Optional. Omitted and `null` are equivalent and mean there is no minimum length constraint.
     */
    Patch<int> _minLength{};
    /**
     * Maximum string length.
     *
     * Optional. Omitted and `null` are equivalent and mean there is no maximum length constraint.
     */
    Patch<int> _maxLength{};
    /**
     * Pattern the string must match, in ECMA-262 regular expression syntax.
     *
     * Optional. Omitted and `null` are equivalent and mean there is no pattern constraint.
     */
    Patch<QString> _pattern{};
    /**
     * String format.
     *
     * Optional. Omitted and `null` are equivalent and mean there is no format constraint.
     */
    Patch<StringFormat> _format{};
    /**
     * Default value.
     *
     * Optional. Omitted and `null` are equivalent and mean no default value is provided.
     */
    Patch<QString> _default_{};
    /**
     * Enum values for untitled single-select enums.
     * Must contain at least one value when present.
     * Optional. Omitted and `null` are equivalent and mean no untitled single-select choices are
     * declared by `enum`.
     */
    Patch<QJsonArray> _enum_{};
    /**
     * Titled enum options for titled single-select enums.
     * Must contain at least one option when present.
     * Optional. Omitted and `null` are equivalent and mean no titled single-select choices are
     * declared by `oneOf`.
     */
    Patch<QList<EnumOption>> _oneOf{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

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

    bool operator==(const StringPropertySchema &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<StringPropertySchema> fromJson<StringPropertySchema>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const StringPropertySchema &data);

/**
 * Property schema for elicitation form fields.
 *
 * Each variant corresponds to a JSON Schema `"type"` value.
 * Single-select enums use `type: "string"` with `enum` or `oneOf` set.
 * Multi-select enums use `type: "array"`.
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
    ElicitationSchemaType _type{};  //!< Type discriminator. Always `"object"`. Any other value is rejected.
    /**
     * Optional title for the schema.
     *
     * Optional. Omitted and `null` are equivalent and mean no title is provided.
     */
    Patch<QString> _title{};
    QMap<QString, ElicitationPropertySchema> _properties{};  //!< Property definitions (must be primitive types).
    /**
     * List of required property names.
     *
     * Optional. Omitted and `null` are equivalent and mean no property names are required.
     */
    Patch<QJsonArray> _required{};
    /**
     * Optional description of what this schema represents.
     *
     * Optional. Omitted and `null` are equivalent and mean no schema description is provided.
     */
    Patch<QString> _description{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    ElicitationSchema& type(const ElicitationSchemaType & v) { _type = v; return *this; }
    ElicitationSchema& title(const Patch<QString> & v) { _title = v; return *this; }
    ElicitationSchema& title(const QString & v) { _title = v; return *this; }
    ElicitationSchema& properties(const QMap<QString, ElicitationPropertySchema> & v) { _properties = v; return *this; }
    ElicitationSchema& addProperty(const QString &key, const ElicitationPropertySchema & v) { _properties[key] = v; return *this; }
    ElicitationSchema& required(const Patch<QJsonArray> & v) { _required = v; return *this; }
    ElicitationSchema& required(const QJsonArray & v) { _required = v; return *this; }
    ElicitationSchema& description(const Patch<QString> & v) { _description = v; return *this; }
    ElicitationSchema& description(const QString & v) { _description = v; return *this; }
    ElicitationSchema& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    ElicitationSchema& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const ElicitationSchemaType& type() const { return _type; }
    const Patch<QString>& title() const { return _title; }
    const QMap<QString, ElicitationPropertySchema>& properties() const { return _properties; }
    const Patch<QJsonArray>& required() const { return _required; }
    const Patch<QString>& description() const { return _description; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const ElicitationSchema &other) const = default;
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
 * When `toolCallId` is set, the elicitation is tied to a specific tool call.
 * This is useful when an agent receives an elicitation from an MCP server
 * during a tool call and needs to redirect it to the user.
 */
struct ElicitationSessionScope {
    SessionId _sessionId{};  //!< The session this elicitation is tied to.
    /**
     * Optional tool call within the session.
     *
     * Optional. Omitted and `null` are equivalent and mean the elicitation is scoped to the
     * session without a specific tool call.
     */
    Patch<ToolCallId> _toolCallId{};

    ElicitationSessionScope& sessionId(const SessionId & v) { _sessionId = v; return *this; }
    ElicitationSessionScope& toolCallId(const Patch<ToolCallId> & v) { _toolCallId = v; return *this; }
    ElicitationSessionScope& toolCallId(const ToolCallId & v) { _toolCallId = v; return *this; }

    const SessionId& sessionId() const { return _sessionId; }
    const Patch<ToolCallId>& toolCallId() const { return _toolCallId; }

    bool operator==(const ElicitationSessionScope &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<ElicitationSessionScope> fromJson<ElicitationSessionScope>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ElicitationSessionScope &data);

/** Form-based elicitation mode where the client renders a form from the provided schema. */
struct ElicitationFormMode {
    ElicitationSchema _requestedSchema{};  //!< A JSON Schema describing the form fields to present to the user.
    QJsonObject _additionalProperties;  //!< additional properties

    ElicitationFormMode& requestedSchema(const ElicitationSchema & v) { _requestedSchema = v; return *this; }
    ElicitationFormMode& additionalProperties(const QString &key, const QJsonValue &v) { _additionalProperties.insert(key, v); return *this; }
    ElicitationFormMode& additionalProperties(const QJsonObject &obj) { for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) _additionalProperties.insert(it.key(), it.value()); return *this; }

    const ElicitationSchema& requestedSchema() const { return _requestedSchema; }
    const QJsonObject& additionalProperties() const { return _additionalProperties; }

    bool operator==(const ElicitationFormMode &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<ElicitationFormMode> fromJson<ElicitationFormMode>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ElicitationFormMode &data);

using ElicitationId = QString;

/** URL-based elicitation mode where the client directs the user to a URL. */
struct ElicitationUrlMode {
    ElicitationId _elicitationId{};  //!< The unique identifier for this elicitation.
    QString _url{};  //!< The URL to direct the user to.
    QJsonObject _additionalProperties;  //!< additional properties

    ElicitationUrlMode& elicitationId(const ElicitationId & v) { _elicitationId = v; return *this; }
    ElicitationUrlMode& url(const QString & v) { _url = v; return *this; }
    ElicitationUrlMode& additionalProperties(const QString &key, const QJsonValue &v) { _additionalProperties.insert(key, v); return *this; }
    ElicitationUrlMode& additionalProperties(const QJsonObject &obj) { for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) _additionalProperties.insert(it.key(), it.value()); return *this; }

    const ElicitationId& elicitationId() const { return _elicitationId; }
    const QString& url() const { return _url; }
    const QJsonObject& additionalProperties() const { return _additionalProperties; }

    bool operator==(const ElicitationUrlMode &other) const = default;
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
 *
 * A session-scoped request follows the aggregate foreground-state contract:
 * agents SHOULD report `requires_action` when the whole foreground workflow is
 * blocked on user action. Once reported, they MUST report `running` when progress
 * resumes, or `idle` if work ends without resuming. Independent background requests
 * and request-scoped elicitations do not themselves change session state.
 * Accepting a URL does not mean the external interaction completed.
 *
 * Agents SHOULD cancel pending elicitations no longer needed by cancelled work.
 * The request's scope alone does not establish that dependency.
 *
 * See protocol docs: [Elicitation](https://agentclientprotocol.com/protocol/v2/elicitation)
 */
struct CreateElicitationRequest {
    QString _message{};  //!< A human-readable message describing what input is needed.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};
    QJsonObject _additionalProperties;  //!< additional properties

    CreateElicitationRequest& message(const QString & v) { _message = v; return *this; }
    CreateElicitationRequest& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    CreateElicitationRequest& _meta(const QJsonObject & v) { __meta = v; return *this; }
    CreateElicitationRequest& additionalProperties(const QString &key, const QJsonValue &v) { _additionalProperties.insert(key, v); return *this; }
    CreateElicitationRequest& additionalProperties(const QJsonObject &obj) { for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) _additionalProperties.insert(it.key(), it.value()); return *this; }

    const QString& message() const { return _message; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
    const QJsonObject& additionalProperties() const { return _additionalProperties; }

    bool operator==(const CreateElicitationRequest &other) const = default;
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
    PermissionOptionId _optionId{};  //!< Identifier for this permission option. MUST be unique within the request.
    QString _name{};  //!< Human-readable label to display to the user.
    PermissionOptionKind _kind{};  //!< Hint about the nature of this permission option.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    PermissionOption& optionId(const PermissionOptionId & v) { _optionId = v; return *this; }
    PermissionOption& name(const QString & v) { _name = v; return *this; }
    PermissionOption& kind(const PermissionOptionKind & v) { _kind = v; return *this; }
    PermissionOption& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    PermissionOption& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const PermissionOptionId& optionId() const { return _optionId; }
    const QString& name() const { return _name; }
    const PermissionOptionKind& kind() const { return _kind; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const PermissionOption &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<PermissionOption> fromJson<PermissionOption>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const PermissionOption &data);

using AbsolutePath = QString;

using TerminalId = QString;

/** Permission request details for a command. */
struct CommandPermissionSubject {
    QString _command{};  //!< The command that would be run if permission is granted.
    AbsolutePath _cwd{};  //!< The absolute working directory for the command.
    Patch<ToolCallId> _toolCallId{};  //!< The associated tool call, when known. Omitted and `null` are equivalent.
    Patch<TerminalId> _terminalId{};  //!< The associated terminal, when already known. Omitted and `null` are equivalent.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

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

    bool operator==(const CommandPermissionSubject &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<CommandPermissionSubject> fromJson<CommandPermissionSubject>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const CommandPermissionSubject &data);

/**
 * Capability to cancel a tool call with `tool_call/cancel`.
 *
 * Supplying `{}` advertises support. An omitted or `null` `cancel` does not.
 */
struct ToolCallCancelCapabilities {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    ToolCallCancelCapabilities& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    ToolCallCancelCapabilities& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const ToolCallCancelCapabilities &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<ToolCallCancelCapabilities> fromJson<ToolCallCancelCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ToolCallCancelCapabilities &data);

/**
 * Client-initiated controls a tool call currently supports.
 *
 * A malformed value falls back to no controls, so the Client never sends a request the
 * Agent did not advertise.
 *
 * See protocol docs: [Cancelling Tool Calls](https://agentclientprotocol.com/protocol/v2/tool-calls#cancelling-tool-calls)
 */
struct ToolCallCapabilities {
    /**
     * Permits the Client to cancel this tool call with `tool_call/cancel`.
     *
     * Omitted and `null` are equivalent and mean unsupported. An object (including `{}`)
     * means supported. Agents SHOULD stop advertising it once the tool call reaches a final status.
     */
    Patch<ToolCallCancelCapabilities> _cancel{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    ToolCallCapabilities& cancel(const Patch<ToolCallCancelCapabilities> & v) { _cancel = v; return *this; }
    ToolCallCapabilities& cancel(const ToolCallCancelCapabilities & v) { _cancel = v; return *this; }
    ToolCallCapabilities& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    ToolCallCapabilities& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<ToolCallCancelCapabilities>& cancel() const { return _cancel; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const ToolCallCapabilities &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<ToolCallCapabilities> fromJson<ToolCallCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ToolCallCapabilities &data);

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
    /**
     * Intended recipients for this content, such as the user or assistant.
     *
     * Omitted and `null` are equivalent and mean no audience is specified.
     */
    Patch<QList<Role>> _audience{};
    /**
     * Timestamp indicating when the underlying resource was last modified.
     *
     * Should be an ISO 8601 formatted string (e.g., "2025-01-12T15:00:58Z"), as in MCP.
     *
     * Omitted and `null` are equivalent and mean the modification time is not reported.
     */
    Patch<QString> _lastModified{};
    /**
     * Relative importance of this content when clients choose what to surface, from 0 (least
     * important) to 1 (most important).
     *
     * Omitted and `null` are equivalent and mean no priority is specified.
     */
    Patch<double> _priority{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

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

    bool operator==(const Annotations &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<Annotations> fromJson<Annotations>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const Annotations &data);

using MediaType = QString;

/** Audio provided to or from an LLM. */
struct AudioContent {
    QString _data{};  //!< Base64-encoded media payload.
    MediaType _mimeType{};  //!< MIME type describing the encoded media payload.
    /**
     * Optional annotations that help clients decide how to display or route this content.
     *
     * Omitted and `null` are equivalent and mean no annotations are provided.
     */
    Patch<Annotations> _annotations{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

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

    bool operator==(const AudioContent &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<AudioContent> fromJson<AudioContent>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const AudioContent &data);

/** Binary resource contents. */
struct BlobResourceContents {
    QString _blob{};  //!< Base64-encoded bytes for a binary resource payload.
    QString _uri{};  //!< URI associated with this resource or media payload.
    /**
     * MIME type describing the encoded media payload.
     *
     * Omitted and `null` are equivalent and mean the MIME type is not specified.
     */
    Patch<MediaType> _mimeType{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

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

    bool operator==(const BlobResourceContents &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<BlobResourceContents> fromJson<BlobResourceContents>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const BlobResourceContents &data);

/** Text-based resource contents. */
struct TextResourceContents {
    QString _text{};  //!< Text payload carried by this content block.
    QString _uri{};  //!< URI associated with this resource or media payload.
    /**
     * MIME type describing the encoded media payload.
     *
     * Omitted and `null` are equivalent and mean the MIME type is not specified.
     */
    Patch<MediaType> _mimeType{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

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

    bool operator==(const TextResourceContents &other) const = default;
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
    EmbeddedResourceResource _resource{};  //!< Embedded resource payload, either text or binary data.
    /**
     * Optional annotations that help clients decide how to display or route this content.
     *
     * Omitted and `null` are equivalent and mean no annotations are provided.
     */
    Patch<Annotations> _annotations{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    EmbeddedResource& resource(const EmbeddedResourceResource & v) { _resource = v; return *this; }
    EmbeddedResource& annotations(const Patch<Annotations> & v) { _annotations = v; return *this; }
    EmbeddedResource& annotations(const Annotations & v) { _annotations = v; return *this; }
    EmbeddedResource& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    EmbeddedResource& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const EmbeddedResourceResource& resource() const { return _resource; }
    const Patch<Annotations>& annotations() const { return _annotations; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const EmbeddedResource &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<EmbeddedResource> fromJson<EmbeddedResource>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const EmbeddedResource &data);

/** An image provided to or from an LLM. */
struct ImageContent {
    QString _data{};  //!< Base64-encoded media payload.
    MediaType _mimeType{};  //!< MIME type describing the encoded media payload.
    /**
     * URI associated with this resource or media payload.
     *
     * Omitted and `null` are equivalent and mean no URI is associated with the image.
     */
    Patch<QString> _uri{};
    /**
     * Optional annotations that help clients decide how to display or route this content.
     *
     * Omitted and `null` are equivalent and mean no annotations are provided.
     */
    Patch<Annotations> _annotations{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

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

    bool operator==(const ImageContent &other) const = default;
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
    /**
     * A standard URI pointing to an icon resource. MUST be an `https:` or `data:` URI.
     * Clients reject other schemes, fetch without credentials, and do not follow
     * scheme- or origin-changing redirects.
     */
    QString _src{};
    /**
     * Optional MIME type override if the source MIME type is missing or generic.
     *
     * Omitted and `null` are equivalent and mean the source MIME type is used.
     */
    Patch<MediaType> _mimeType{};
    /**
     * Optional array of strings that specify sizes at which the icon can be used.
     * Each string should be in `WxH` format (e.g., `"48x48"`, `"96x96"`) or
     * `"any"` for scalable formats like SVG.
     *
     * If not provided, the client should assume that the icon can be used at any size.
     *
     * Omitted and `null` are equivalent and mean no sizes are specified.
     */
    Patch<QJsonArray> _sizes{};
    /**
     * Optional theme this icon is designed for.
     *
     * Omitted and `null` are equivalent and mean the icon is not theme-specific.
     */
    Patch<IconTheme> _theme{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    Icon& src(const QString & v) { _src = v; return *this; }
    Icon& mimeType(const Patch<MediaType> & v) { _mimeType = v; return *this; }
    Icon& mimeType(const MediaType & v) { _mimeType = v; return *this; }
    Icon& sizes(const Patch<QJsonArray> & v) { _sizes = v; return *this; }
    Icon& sizes(const QJsonArray & v) { _sizes = v; return *this; }
    Icon& theme(const Patch<IconTheme> & v) { _theme = v; return *this; }
    Icon& theme(const IconTheme & v) { _theme = v; return *this; }
    Icon& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    Icon& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const QString& src() const { return _src; }
    const Patch<MediaType>& mimeType() const { return _mimeType; }
    const Patch<QJsonArray>& sizes() const { return _sizes; }
    const Patch<IconTheme>& theme() const { return _theme; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const Icon &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<Icon> fromJson<Icon>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const Icon &data);

/**
 * A link to a resource that the receiver may be able to read, included in a prompt or tool call result.
 */
struct ResourceLink {
    QString _name{};  //!< Human-readable name shown for this protocol object.
    QString _uri{};  //!< URI associated with this resource or media payload.
    /**
     * Optional display title for end-user UI.
     *
     * Omitted and `null` are equivalent and mean no title is provided.
     */
    Patch<QString> _title{};
    /**
     * Optional human-readable details shown with this protocol object.
     *
     * Omitted and `null` are equivalent and mean no description is provided.
     */
    Patch<QString> _description{};
    /**
     * Optional set of sized icons that the client can display in a user interface.
     *
     * Omitted and `null` are equivalent and mean no icons are provided.
     */
    Patch<QList<Icon>> _icons{};
    /**
     * MIME type describing the encoded media payload.
     *
     * Omitted and `null` are equivalent and mean the MIME type is not specified.
     */
    Patch<MediaType> _mimeType{};
    /**
     * Optional size of the linked resource in bytes, if known.
     *
     * Omitted and `null` are equivalent and mean the size is not known.
     */
    Patch<int> _size{};
    /**
     * Optional annotations that help clients decide how to display or route this content.
     *
     * Omitted and `null` are equivalent and mean no annotations are provided.
     */
    Patch<Annotations> _annotations{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

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

    bool operator==(const ResourceLink &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<ResourceLink> fromJson<ResourceLink>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ResourceLink &data);

/** Text provided to or from an LLM. */
struct TextContent {
    QString _text{};  //!< Text payload carried by this content block.
    /**
     * Optional annotations that help clients decide how to display or route this content.
     *
     * Omitted and `null` are equivalent and mean no annotations are provided.
     */
    Patch<Annotations> _annotations{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    TextContent& text(const QString & v) { _text = v; return *this; }
    TextContent& annotations(const Patch<Annotations> & v) { _annotations = v; return *this; }
    TextContent& annotations(const Annotations & v) { _annotations = v; return *this; }
    TextContent& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    TextContent& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const QString& text() const { return _text; }
    const Patch<Annotations>& annotations() const { return _annotations; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const TextContent &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<TextContent> fromJson<TextContent>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const TextContent &data);

/**
 * Content blocks represent displayable information in the Agent Client Protocol.
 *
 * Content blocks appear in:
 * - User prompts sent via `session/prompt`
 * - Language model output reported through `session/update` notifications as
 * message updates or streamed chunks
 * - Progress updates and results from tool calls
 *
 * This structure is compatible with the Model Context Protocol (MCP), so agents can
 * forward content from MCP tool outputs without transformation.
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
    ContentBlock _content{};  //!< The actual content block.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    Content& content(const ContentBlock & v) { _content = v; return *this; }
    Content& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    Content& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const ContentBlock& content() const { return _content; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const Content &other) const = default;
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
    AbsolutePath _path{};  //!< Absolute path for the operation.

    DiffPathChange& path(const AbsolutePath & v) { _path = v; return *this; }

    const AbsolutePath& path() const { return _path; }

    bool operator==(const DiffPathChange &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<DiffPathChange> fromJson<DiffPathChange>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const DiffPathChange &data);

/** Operation metadata for move and copy changes. */
struct DiffPathPairChange {
    AbsolutePath _oldPath{};  //!< Absolute path before the operation.
    AbsolutePath _path{};  //!< Absolute path after the operation.

    DiffPathPairChange& oldPath(const AbsolutePath & v) { _oldPath = v; return *this; }
    DiffPathPairChange& path(const AbsolutePath & v) { _path = v; return *this; }

    const AbsolutePath& oldPath() const { return _oldPath; }
    const AbsolutePath& path() const { return _path; }

    bool operator==(const DiffPathPairChange &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<DiffPathPairChange> fromJson<DiffPathPairChange>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const DiffPathPairChange &data);

/**
 * One file-level change described by a `Diff`.
 *
 * Structured change metadata lets clients identify affected files and
 * operations without parsing the text patch.
 */
struct DiffChange {
    /**
     * File content kind after the operation (for `delete`, of the deleted file).
     *
     * Omitted and `null` are equivalent and mean the content kind is unknown.
     */
    Patch<DiffFileType> _fileType{};
    /**
     * MIME type of the file contents after the operation (for `delete`, of the deleted file).
     *
     * Omitted and `null` are equivalent and mean the MIME type is unknown.
     */
    Patch<MediaType> _mimeType{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};
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

    bool operator==(const DiffChange &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<DiffChange> fromJson<DiffChange>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const DiffChange &data);

/** Text patch format used by `DiffPatch`. */
enum class DiffPatchFormat {
    git_patch
};

ACPLIB_EXPORT QString toString(DiffPatchFormat v);

template<>
ACPLIB_EXPORT Utils::Result<DiffPatchFormat> fromJson<DiffPatchFormat>(const QJsonValue &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const DiffPatchFormat &v);

/** Renderable patch text and its format. */
struct DiffPatch {
    DiffPatchFormat _format{};  //!< Patch format. The only ACP-defined value is `git_patch`.
    QString _text{};  //!< Patch text in the format named by `format`.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    DiffPatch& format(const DiffPatchFormat & v) { _format = v; return *this; }
    DiffPatch& text(const QString & v) { _text = v; return *this; }
    DiffPatch& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    DiffPatch& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const DiffPatchFormat& format() const { return _format; }
    const QString& text() const { return _text; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const DiffPatch &other) const = default;
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
    QList<DiffChange> _changes{};
    /**
     * Renderable patch text for some or all of the structured changes.
     *
     * Agents SHOULD provide patch text whenever feasible. Omitted and `null` are
     * equivalent and mean no renderable patch text was provided.
     */
    Patch<DiffPatch> _patch{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    Diff& changes(const QList<DiffChange> & v) { _changes = v; return *this; }
    Diff& addChange(const DiffChange & v) { _changes.append(v); return *this; }
    Diff& patch(const Patch<DiffPatch> & v) { _patch = v; return *this; }
    Diff& patch(const DiffPatch & v) { _patch = v; return *this; }
    Diff& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    Diff& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const QList<DiffChange>& changes() const { return _changes; }
    const Patch<DiffPatch>& patch() const { return _patch; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const Diff &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<Diff> fromJson<Diff>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const Diff &data);

/**
 * A display-only reference to an agent-owned terminal.
 *
 * Terminal state and output are delivered separately through
 * `terminal_update` and `terminal_output_chunk` session updates.
 */
struct Terminal {
    TerminalId _terminalId{};  //!< The ID of the terminal to display.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    Terminal& terminalId(const TerminalId & v) { _terminalId = v; return *this; }
    Terminal& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    Terminal& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const TerminalId& terminalId() const { return _terminalId; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const Terminal &other) const = default;
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
    AbsolutePath _path{};  //!< The absolute file path being accessed or modified.
    /**
     * Optional 1-based line number within the file. Omitted and `null` are equivalent and
     * mean no specific line. `0` is invalid.
     */
    Patch<int> _line{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    ToolCallLocation& path(const AbsolutePath & v) { _path = v; return *this; }
    ToolCallLocation& line(const Patch<int> & v) { _line = v; return *this; }
    ToolCallLocation& line(int v) { _line = v; return *this; }
    ToolCallLocation& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    ToolCallLocation& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const AbsolutePath& path() const { return _path; }
    const Patch<int>& line() const { return _line; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const ToolCallLocation &other) const = default;
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
    paused,
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
 * See protocol docs: [Reporting](https://agentclientprotocol.com/protocol/v2/tool-calls#reporting)
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
 * Only `toolCallId` is required. Other fields have patch semantics:
 * omitted fields leave the existing tool call value unchanged, `null` clears or
 * unsets the value, and concrete values replace the previous value. For
 * collection fields, concrete arrays replace the previous collection, and both
 * `null` and `[]` clear the collection. A concrete `_meta` object instead
 * patches the stored metadata by top-level key. When a client receives a tool
 * call ID it has not seen before, omitted fields start at their protocol defaults.
 *
 * See protocol docs: [Tool Calls](https://agentclientprotocol.com/protocol/v2/tool-calls)
 */
struct ToolCallUpdate {
    ToolCallId _toolCallId{};  //!< Unique identifier for this tool call within the session.
    /**
     * Programmatic name of the tool being invoked.
     *
     * This field is optional and has patch semantics. Omission means no
     * change, `null` clears the name, and a string replaces it. For a tool
     * call ID the client has not seen before, omission or `null` means that no
     * tool name is available.
     */
    Patch<QString> _name{};
    /**
     * Human-readable title describing what the tool is doing.
     *
     * Omission leaves the title unchanged, `null` unsets it, and a string replaces it.
     * A new tool call starts without a title.
     */
    Patch<QString> _title{};
    /**
     * The category of tool being invoked.
     * Helps clients choose appropriate icons and UI treatment.
     *
     * Omission leaves the kind unchanged, `null` resets it to `other`, and a value replaces it.
     * A new tool call starts as `other`.
     */
    Patch<ToolKind> _kind{};
    /**
     * Current execution status of the tool call.
     *
     * Omission leaves the status unchanged, `null` resets it to `pending`, and a value
     * replaces it. A new tool call starts as `pending`.
     */
    Patch<ToolCallStatus> _status{};
    Patch<QList<ToolCallContent>> _content{};  //!< Content produced by the tool call.
    /**
     * File locations affected by this tool call.
     * Enables "follow-along" features in clients.
     */
    Patch<QList<ToolCallLocation>> _locations{};
    std::optional<QJsonValue> _rawInput{};  //!< Raw input parameters sent to the tool.
    std::optional<QJsonValue> _rawOutput{};  //!< Raw output returned by the tool.
    /**
     * Client-initiated controls this tool call currently supports, such as cancellation.
     *
     * Omission leaves the capabilities unchanged, `null` clears them, and an object replaces
     * the whole previous value. A new tool call starts with none.
     */
    Patch<ToolCallCapabilities> _capabilities{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted leaves the stored metadata unchanged. `null`
     * clears it. An object patches the stored metadata by top-level key: keys it omits are
     * unchanged, a key set to `null` is removed, and any other value replaces that key's
     * stored value without merging nested objects. Beyond this patch rule, implementations
     * MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Metadata Patches](https://agentclientprotocol.com/protocol/v2/extensibility#metadata-patches)
     */
    Patch<QJsonObject> __meta{};

    ToolCallUpdate& toolCallId(const ToolCallId & v) { _toolCallId = v; return *this; }
    ToolCallUpdate& name(const Patch<QString> & v) { _name = v; return *this; }
    ToolCallUpdate& name(const QString & v) { _name = v; return *this; }
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
    ToolCallUpdate& capabilities(const Patch<ToolCallCapabilities> & v) { _capabilities = v; return *this; }
    ToolCallUpdate& capabilities(const ToolCallCapabilities & v) { _capabilities = v; return *this; }
    ToolCallUpdate& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    ToolCallUpdate& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const ToolCallId& toolCallId() const { return _toolCallId; }
    const Patch<QString>& name() const { return _name; }
    const Patch<QString>& title() const { return _title; }
    const Patch<ToolKind>& kind() const { return _kind; }
    const Patch<ToolCallStatus>& status() const { return _status; }
    const Patch<QList<ToolCallContent>>& content() const { return _content; }
    const Patch<QList<ToolCallLocation>>& locations() const { return _locations; }
    const std::optional<QJsonValue>& rawInput() const { return _rawInput; }
    const std::optional<QJsonValue>& rawOutput() const { return _rawOutput; }
    const Patch<ToolCallCapabilities>& capabilities() const { return _capabilities; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const ToolCallUpdate &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<ToolCallUpdate> fromJson<ToolCallUpdate>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ToolCallUpdate &data);

/** Permission request details for a tool call. */
struct ToolCallPermissionSubject {
    /**
     * Details about the tool call requiring permission.
     *
     * Clients apply this to their stored tool call exactly like a `tool_call_update`
     * notification, in the order it was received, before showing the prompt. A first-seen
     * `toolCallId` creates the tool call.
     */
    ToolCallUpdate _toolCall{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    ToolCallPermissionSubject& toolCall(const ToolCallUpdate & v) { _toolCall = v; return *this; }
    ToolCallPermissionSubject& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    ToolCallPermissionSubject& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const ToolCallUpdate& toolCall() const { return _toolCall; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const ToolCallPermissionSubject &other) const = default;
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
    SessionId _sessionId{};  //!< The session ID for this request.
    /**
     * Human-readable title for the permission prompt.
     *
     * This title is specific to the permission prompt and does not update any
     * subject's displayed title.
     */
    QString _title{};
    /**
     * Optional human-readable explanation of why permission is needed.
     *
     * This text is specific to the permission prompt and does not update any
     * subject's displayed content. Omitted and `null` are equivalent and mean no separate
     * permission description was provided.
     */
    Patch<QString> _description{};
    /**
     * Optional structured context about the operation requiring permission.
     *
     * Omitted and `null` are equivalent and mean no structured subject was provided.
     */
    Patch<RequestPermissionSubject> _subject{};
    /**
     * Available permission options for the user to choose from.
     * Must contain at least one option.
     */
    QList<PermissionOption> _options{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

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

    bool operator==(const RequestPermissionRequest &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<RequestPermissionRequest> fromJson<RequestPermissionRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const RequestPermissionRequest &data);

/** A JSON-RPC request object. */
struct AgentRequest {
    RequestId _id{};  //!< The request id used to correlate the matching response.
    QString _method{};  //!< The method name to invoke.
    std::optional<QString> _params{};  //!< Method-specific request parameters.

    AgentRequest& id(const RequestId & v) { _id = v; return *this; }
    AgentRequest& method(const QString & v) { _method = v; return *this; }
    AgentRequest& params(const std::optional<QString> & v) { _params = v; return *this; }

    const RequestId& id() const { return _id; }
    const QString& method() const { return _method; }
    const std::optional<QString>& params() const { return _params; }

    bool operator==(const AgentRequest &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<AgentRequest> fromJson<AgentRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const AgentRequest &data);

/** Response from cancelling a terminal. */
struct CancelTerminalResponse {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    CancelTerminalResponse& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    CancelTerminalResponse& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const CancelTerminalResponse &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<CancelTerminalResponse> fromJson<CancelTerminalResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const CancelTerminalResponse &data);

/** Response from cancelling a tool call. */
struct CancelToolCallResponse {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    CancelToolCallResponse& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    CancelToolCallResponse& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const CancelToolCallResponse &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<CancelToolCallResponse> fromJson<CancelToolCallResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const CancelToolCallResponse &data);

/** Response from closing a session. */
struct CloseSessionResponse {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    CloseSessionResponse& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    CloseSessionResponse& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const CloseSessionResponse &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<CloseSessionResponse> fromJson<CloseSessionResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const CloseSessionResponse &data);

/** Response from deleting a session. */
struct DeleteSessionResponse {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    DeleteSessionResponse& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    DeleteSessionResponse& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const DeleteSessionResponse &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<DeleteSessionResponse> fromJson<DeleteSessionResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const DeleteSessionResponse &data);

/**
 * Predefined error codes for common JSON-RPC and ACP-specific errors.
 *
 * Standard errors use the JSON-RPC 2.0 codes. ACP-specific errors use the JSON-RPC
 * server-error range (-32000 to -32099), and request cancellation uses -32800 from the
 * range LSP reserves (-32899 to -32800).
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
    constexpr int Unsupported_protocol_version = -32022;
} // namespace ErrorCode
/**
 * Data for the `-32002` (Resource not found) error when a session-scoped request names a
 * session that does not exist or is not active.
 *
 * See protocol docs: [Unknown Sessions](https://agentclientprotocol.com/protocol/v2/error#unknown-sessions)
 */
struct SessionNotFoundErrorData {
    SessionId _sessionId{};  //!< The session ID from the request.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    SessionNotFoundErrorData& sessionId(const SessionId & v) { _sessionId = v; return *this; }
    SessionNotFoundErrorData& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    SessionNotFoundErrorData& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const SessionId& sessionId() const { return _sessionId; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const SessionNotFoundErrorData &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<SessionNotFoundErrorData> fromJson<SessionNotFoundErrorData>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionNotFoundErrorData &data);

using ProtocolVersion = int;
template<>
ACPLIB_EXPORT Utils::Result<ProtocolVersion> fromJson<ProtocolVersion>(const QJsonValue &val);

/**
 * Data for the `-32022` (Unsupported protocol version) error.
 *
 * See protocol docs: [Version Negotiation](https://agentclientprotocol.com/protocol/v2/initialization#version-negotiation)
 */
struct UnsupportedProtocolVersionErrorData {
    ProtocolVersion _requested{};  //!< The protocol version the Client requested.
    QList<ProtocolVersion> _supported{};  //!< The protocol versions the Agent supports.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    UnsupportedProtocolVersionErrorData& requested(const ProtocolVersion & v) { _requested = v; return *this; }
    UnsupportedProtocolVersionErrorData& supported(const QList<ProtocolVersion> & v) { _supported = v; return *this; }
    UnsupportedProtocolVersionErrorData& addSupported(const ProtocolVersion & v) { _supported.append(v); return *this; }
    UnsupportedProtocolVersionErrorData& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    UnsupportedProtocolVersionErrorData& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const ProtocolVersion& requested() const { return _requested; }
    const QList<ProtocolVersion>& supported() const { return _supported; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const UnsupportedProtocolVersionErrorData &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<UnsupportedProtocolVersionErrorData> fromJson<UnsupportedProtocolVersionErrorData>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const UnsupportedProtocolVersionErrorData &data);

/** Schema for [`Error::data`]: the shapes ACP defines, or any other value. */
using ErrorData = std::variant<UnsupportedProtocolVersionErrorData, SessionNotFoundErrorData>;

template<>
ACPLIB_EXPORT Utils::Result<ErrorData> fromJson<ErrorData>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ErrorData &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const ErrorData &val);

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
    int _code{};
    /**
     * A string providing a short description of the error.
     * The message should be limited to a concise single sentence.
     */
    QString _message{};
    /**
     * Optional primitive or structured value that contains additional information about the error.
     * This may include debugging information or context-specific details.
     *
     * Omitted and `null` are equivalent and mean no additional data is provided. Some codes
     * define a shape: [`UnsupportedProtocolVersionErrorData`] for `-32022`, and
     * [`SessionNotFoundErrorData`] for `-32002` when a session is not found.
     */
    Patch<ErrorData> _data{};

    Error& code(int v) { _code = v; return *this; }
    Error& message(const QString & v) { _message = v; return *this; }
    Error& data(const Patch<ErrorData> & v) { _data = v; return *this; }
    Error& data(const ErrorData & v) { _data = v; return *this; }

    const int& code() const { return _code; }
    const QString& message() const { return _message; }
    const Patch<ErrorData>& data() const { return _data; }

    bool operator==(const Error &other) const = default;
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
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    AgentAuthCapabilities& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    AgentAuthCapabilities& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const AgentAuthCapabilities &other) const = default;
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
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    McpHttpCapabilities& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    McpHttpCapabilities& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const McpHttpCapabilities &other) const = default;
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
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    McpStdioCapabilities& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    McpStdioCapabilities& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const McpStdioCapabilities &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<McpStdioCapabilities> fromJson<McpStdioCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const McpStdioCapabilities &data);

/** MCP capabilities supported by the agent for session lifecycle requests. */
struct McpCapabilities {
    /**
     * Agent supports `stdio` MCP servers.
     *
     * Optional. Omitted and `null` are equivalent and mean the agent does not advertise support.
     * Supplying `{}` means the agent supports stdio MCP server transports.
     */
    Patch<McpStdioCapabilities> _stdio{};
    /**
     * Agent supports `http` MCP servers.
     *
     * Optional. Omitted and `null` are equivalent and mean the agent does not advertise support.
     * Supplying `{}` means the agent supports HTTP MCP server transports.
     */
    Patch<McpHttpCapabilities> _http{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    McpCapabilities& stdio(const Patch<McpStdioCapabilities> & v) { _stdio = v; return *this; }
    McpCapabilities& stdio(const McpStdioCapabilities & v) { _stdio = v; return *this; }
    McpCapabilities& http(const Patch<McpHttpCapabilities> & v) { _http = v; return *this; }
    McpCapabilities& http(const McpHttpCapabilities & v) { _http = v; return *this; }
    McpCapabilities& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    McpCapabilities& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<McpStdioCapabilities>& stdio() const { return _stdio; }
    const Patch<McpHttpCapabilities>& http() const { return _http; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const McpCapabilities &other) const = default;
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
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    PromptAudioCapabilities& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    PromptAudioCapabilities& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const PromptAudioCapabilities &other) const = default;
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
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    PromptEmbeddedContextCapabilities& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    PromptEmbeddedContextCapabilities& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const PromptEmbeddedContextCapabilities &other) const = default;
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
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    PromptImageCapabilities& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    PromptImageCapabilities& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const PromptImageCapabilities &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<PromptImageCapabilities> fromJson<PromptImageCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const PromptImageCapabilities &data);

/**
 * Prompt capabilities supported by the agent in `session/prompt` requests.
 *
 * Baseline agent functionality requires support for `text`
 * and `resource_link` content blocks in prompt requests.
 *
 * Other variants must be explicitly opted in to.
 * Capabilities for different types of content in prompt requests.
 *
 * Indicates which content types beyond the baseline (text and resource links)
 * the agent can process.
 *
 * See protocol docs: [Session Prompt Capabilities](https://agentclientprotocol.com/protocol/v2/initialization#session-prompt-capabilities)
 */
struct PromptCapabilities {
    /**
     * Agent supports `image` content blocks.
     *
     * Optional. Omitted and `null` are equivalent and mean the agent does not advertise support.
     * Supplying `{}` means the agent supports image content in prompts.
     */
    Patch<PromptImageCapabilities> _image{};
    /**
     * Agent supports `audio` content blocks.
     *
     * Optional. Omitted and `null` are equivalent and mean the agent does not advertise support.
     * Supplying `{}` means the agent supports audio content in prompts.
     */
    Patch<PromptAudioCapabilities> _audio{};
    /**
     * Agent supports embedded context in `session/prompt` requests.
     *
     * When enabled, the Client is allowed to include `resource` content blocks
     * in prompt requests for pieces of context that are referenced in the message.
     *
     * Optional. Omitted and `null` are equivalent and mean the agent does not advertise support.
     * Supplying `{}` means the agent supports embedded context in prompts.
     */
    Patch<PromptEmbeddedContextCapabilities> _embeddedContext{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

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

    bool operator==(const PromptCapabilities &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<PromptCapabilities> fromJson<PromptCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const PromptCapabilities &data);

/**
 * Capabilities for additional session directories support.
 *
 * Supplying `{}` means the agent supports the `additionalDirectories` field on
 * supported session lifecycle requests. Agents may return
 * `SessionInfo.additionalDirectories` in `session/list` to report the
 * complete ordered additional-root list associated with a listed session.
 */
struct SessionAdditionalDirectoriesCapabilities {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    SessionAdditionalDirectoriesCapabilities& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    SessionAdditionalDirectoriesCapabilities& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const SessionAdditionalDirectoriesCapabilities &other) const = default;
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
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    SessionDeleteCapabilities& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    SessionDeleteCapabilities& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const SessionDeleteCapabilities &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<SessionDeleteCapabilities> fromJson<SessionDeleteCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionDeleteCapabilities &data);

/**
 * Capabilities for the `override` system prompt mode.
 *
 * Supplying `{}` means the agent supports replacing its built-in default
 * instructions with the client-provided system prompt.
 */
struct SessionSystemPromptOverrideCapabilities {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    SessionSystemPromptOverrideCapabilities& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    SessionSystemPromptOverrideCapabilities& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const SessionSystemPromptOverrideCapabilities &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<SessionSystemPromptOverrideCapabilities> fromJson<SessionSystemPromptOverrideCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionSystemPromptOverrideCapabilities &data);

/**
 * System prompt delivery modes the agent supports in addition to `append`.
 *
 * `append` has no entry because every agent advertising `session.systemPrompt`
 * supports it.
 */
struct SessionSystemPromptModeCapabilities {
    /**
     * Whether the agent supports `override` mode.
     *
     * Optional. Omitted and `null` are equivalent and mean the agent does not advertise support.
     * Supplying `{}` means the agent supports replacing its built-in default
     * instructions with the client-provided system prompt.
     */
    Patch<SessionSystemPromptOverrideCapabilities> _override_{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    SessionSystemPromptModeCapabilities& override_(const Patch<SessionSystemPromptOverrideCapabilities> & v) { _override_ = v; return *this; }
    SessionSystemPromptModeCapabilities& override_(const SessionSystemPromptOverrideCapabilities & v) { _override_ = v; return *this; }
    SessionSystemPromptModeCapabilities& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    SessionSystemPromptModeCapabilities& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<SessionSystemPromptOverrideCapabilities>& override_() const { return _override_; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const SessionSystemPromptModeCapabilities &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<SessionSystemPromptModeCapabilities> fromJson<SessionSystemPromptModeCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionSystemPromptModeCapabilities &data);

/**
 * Capabilities for client-provided system prompts.
 *
 * Supplying `{}` means the agent supports `systemPrompt` on session lifecycle
 * requests in `append` mode.
 */
struct SessionSystemPromptCapabilities {
    /**
     * Delivery modes the agent supports in addition to `append`.
     *
     * Optional. Omitted and `null` are equivalent and mean the agent supports only `append`.
     */
    Patch<SessionSystemPromptModeCapabilities> _mode{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    SessionSystemPromptCapabilities& mode(const Patch<SessionSystemPromptModeCapabilities> & v) { _mode = v; return *this; }
    SessionSystemPromptCapabilities& mode(const SessionSystemPromptModeCapabilities & v) { _mode = v; return *this; }
    SessionSystemPromptCapabilities& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    SessionSystemPromptCapabilities& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<SessionSystemPromptModeCapabilities>& mode() const { return _mode; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const SessionSystemPromptCapabilities &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<SessionSystemPromptCapabilities> fromJson<SessionSystemPromptCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionSystemPromptCapabilities &data);

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
     * Optional. Omitted and `null` are equivalent and mean the agent does not advertise any
     * prompt extensions beyond the baseline text and resource-link content
     * required by `session/prompt`.
     */
    Patch<PromptCapabilities> _prompt{};
    /**
     * MCP capabilities supported by the agent for session lifecycle requests.
     *
     * Optional. Omitted and `null` are equivalent and mean the agent does not advertise MCP
     * server transport support for sessions.
     */
    Patch<McpCapabilities> _mcp{};
    /**
     * Whether the agent supports `session/delete`.
     *
     * Optional. Omitted and `null` are equivalent and mean the agent does not advertise support.
     * Supplying `{}` means the agent supports deleting sessions from `session/list`.
     */
    Patch<SessionDeleteCapabilities> _delete_{};
    /**
     * Whether the agent supports `additionalDirectories` on supported session lifecycle requests.
     *
     * Optional. Omitted and `null` are equivalent and mean the agent does not advertise support.
     * Supplying `{}` means the agent supports `additionalDirectories` on
     * supported session lifecycle requests.
     *
     * Agents may return `SessionInfo.additionalDirectories` to report the
     * complete ordered additional-root list associated with a listed session.
     */
    Patch<SessionAdditionalDirectoriesCapabilities> _additionalDirectories{};
    /**
     * Whether the agent supports `systemPrompt` on session lifecycle requests.
     *
     * Optional. Omitted and `null` are equivalent and mean the agent does not advertise support.
     * Supplying `{}` means the agent supports `systemPrompt` in `append` mode.
     */
    Patch<SessionSystemPromptCapabilities> _systemPrompt{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    SessionCapabilities& prompt(const Patch<PromptCapabilities> & v) { _prompt = v; return *this; }
    SessionCapabilities& prompt(const PromptCapabilities & v) { _prompt = v; return *this; }
    SessionCapabilities& mcp(const Patch<McpCapabilities> & v) { _mcp = v; return *this; }
    SessionCapabilities& mcp(const McpCapabilities & v) { _mcp = v; return *this; }
    SessionCapabilities& delete_(const Patch<SessionDeleteCapabilities> & v) { _delete_ = v; return *this; }
    SessionCapabilities& delete_(const SessionDeleteCapabilities & v) { _delete_ = v; return *this; }
    SessionCapabilities& additionalDirectories(const Patch<SessionAdditionalDirectoriesCapabilities> & v) { _additionalDirectories = v; return *this; }
    SessionCapabilities& additionalDirectories(const SessionAdditionalDirectoriesCapabilities & v) { _additionalDirectories = v; return *this; }
    SessionCapabilities& systemPrompt(const Patch<SessionSystemPromptCapabilities> & v) { _systemPrompt = v; return *this; }
    SessionCapabilities& systemPrompt(const SessionSystemPromptCapabilities & v) { _systemPrompt = v; return *this; }
    SessionCapabilities& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    SessionCapabilities& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<PromptCapabilities>& prompt() const { return _prompt; }
    const Patch<McpCapabilities>& mcp() const { return _mcp; }
    const Patch<SessionDeleteCapabilities>& delete_() const { return _delete_; }
    const Patch<SessionAdditionalDirectoriesCapabilities>& additionalDirectories() const { return _additionalDirectories; }
    const Patch<SessionSystemPromptCapabilities>& systemPrompt() const { return _systemPrompt; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const SessionCapabilities &other) const = default;
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
     * Optional. Omitted and `null` are equivalent and mean the agent does not support the
     * `session/\*` methods. Supplying `{}` means the agent supports the
     * baseline session methods: `session/new`, `session/list`,
     * `session/resume`, `session/close`, `session/prompt`, `session/cancel`,
     * and `session/update`.
     */
    Patch<SessionCapabilities> _session{};
    /**
     * Authentication-related extension capabilities supported by the agent.
     *
     * Optional. Omitted and `null` are equivalent and mean the agent does not advertise any
     * authentication-related extensions. This field does not advertise support
     * for `auth/login` or `auth/logout`. Those methods are advertised by a
     * non-empty `authMethods` list in the `initialize` response.
     */
    Patch<AgentAuthCapabilities> _auth{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    AgentCapabilities& session(const Patch<SessionCapabilities> & v) { _session = v; return *this; }
    AgentCapabilities& session(const SessionCapabilities & v) { _session = v; return *this; }
    AgentCapabilities& auth(const Patch<AgentAuthCapabilities> & v) { _auth = v; return *this; }
    AgentCapabilities& auth(const AgentAuthCapabilities & v) { _auth = v; return *this; }
    AgentCapabilities& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    AgentCapabilities& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<SessionCapabilities>& session() const { return _session; }
    const Patch<AgentAuthCapabilities>& auth() const { return _auth; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const AgentCapabilities &other) const = default;
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
    AuthMethodId _methodId{};  //!< Unique identifier for this authentication method.
    QString _name{};  //!< Human-readable name of the authentication method.
    /**
     * Optional human-readable description of this authentication method.
     *
     * Omitted and `null` are equivalent and mean no description is provided.
     */
    Patch<QString> _description{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

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

    bool operator==(const AuthMethodAgent &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<AuthMethodAgent> fromJson<AuthMethodAgent>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const AuthMethodAgent &data);

/** An environment variable to set when launching a process. */
struct EnvVariable {
    QString _name{};  //!< The name of the environment variable.
    QString _value{};  //!< The value to set for the environment variable.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    EnvVariable& name(const QString & v) { _name = v; return *this; }
    EnvVariable& value(const QString & v) { _value = v; return *this; }
    EnvVariable& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    EnvVariable& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const QString& name() const { return _name; }
    const QString& value() const { return _value; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const EnvVariable &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<EnvVariable> fromJson<EnvVariable>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const EnvVariable &data);

/**
 * Terminal-based authentication method.
 *
 * The client runs the configured agent program as a separate interactive
 * process for the user to authenticate via a TUI. Agents MUST advertise this
 * method only when the client enabled its terminal authentication capability.
 * A zero exit status signals success. Any other termination signals failure.
 * The client MUST NOT pass this method to `auth/login`.
 */
struct AuthMethodTerminal {
    AuthMethodId _methodId{};  //!< Unique identifier for this authentication method.
    QString _name{};  //!< Human-readable name of the authentication method.
    /**
     * Optional human-readable description of this authentication method.
     *
     * Omitted and `null` are equivalent and mean no description is provided.
     */
    Patch<QString> _description{};
    std::optional<QStringList> _args{};  //!< Additional arguments to append to the configured agent invocation for terminal auth.
    /**
     * Additional environment variables to set on the configured agent invocation for terminal auth.
     * Names MUST be unique. These values override same-named variables in the
     * base launch configuration.
     */
    std::optional<QList<EnvVariable>> _env{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    AuthMethodTerminal& methodId(const AuthMethodId & v) { _methodId = v; return *this; }
    AuthMethodTerminal& name(const QString & v) { _name = v; return *this; }
    AuthMethodTerminal& description(const Patch<QString> & v) { _description = v; return *this; }
    AuthMethodTerminal& description(const QString & v) { _description = v; return *this; }
    AuthMethodTerminal& args(const std::optional<QStringList> & v) { _args = v; return *this; }
    AuthMethodTerminal& addArg(const QString & v) { if (!_args) _args = QStringList{}; (*_args).append(v); return *this; }
    AuthMethodTerminal& env(const std::optional<QList<EnvVariable>> & v) { _env = v; return *this; }
    AuthMethodTerminal& addEnv(const EnvVariable & v) { if (!_env) _env = QList<EnvVariable>{}; (*_env).append(v); return *this; }
    AuthMethodTerminal& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    AuthMethodTerminal& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const AuthMethodId& methodId() const { return _methodId; }
    const QString& name() const { return _name; }
    const Patch<QString>& description() const { return _description; }
    const std::optional<QStringList>& args() const { return _args; }
    const std::optional<QList<EnvVariable>>& env() const { return _env; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const AuthMethodTerminal &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<AuthMethodTerminal> fromJson<AuthMethodTerminal>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const AuthMethodTerminal &data);

/**
 * Describes an available authentication method.
 *
 * The `type` field acts as the discriminator in the serialized JSON form.
 */
using AuthMethod = std::variant<AuthMethodTerminal, AuthMethodAgent, QJsonObject>;

template<>
ACPLIB_EXPORT Utils::Result<AuthMethod> fromJson<AuthMethod>(const QJsonValue &val);

/** Returns the 'type' dispatch field value for the active variant. */
ACPLIB_EXPORT QString dispatchValue(const AuthMethod &val);

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
     * name fallback if title isn't present.
     */
    QString _name{};
    /**
     * Intended for UI and end-user contexts and optimized to be human-readable
     * and easily understood.
     *
     * If not provided, the name should be used for display.
     *
     * Omitted and `null` are equivalent and mean the `name` is used for display.
     */
    Patch<QString> _title{};
    /**
     * Version of the implementation. Can be displayed to the user or used
     * for debugging or metrics purposes. (e.g. "1.0.0").
     */
    QString _version{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

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

    bool operator==(const Implementation &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<Implementation> fromJson<Implementation>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const Implementation &data);

/**
 * Response to the `initialize` method.
 *
 * Contains the negotiated protocol version and agent capabilities.
 *
 * See protocol docs: [Initialization](https://agentclientprotocol.com/protocol/v2/initialization)
 */
struct InitializeResponse {
    /**
     * The highest protocol version supported by the agent that is at or below
     * the version requested by the client. If no such version exists,
     * initialization fails with error code -32022.
     *
     * The client must check that it supports this version and should disconnect
     * if it does not.
     */
    ProtocolVersion _protocolVersion{};
    Implementation _info{};  //!< Information about the implementation sending this initialize response.
    std::optional<AgentCapabilities> _capabilities{};  //!< Capabilities supported by the agent.
    /**
     * Authentication methods supported by the agent.
     *
     * Optional. Omitted or empty means the agent does not advertise the
     * `auth/login` and `auth/logout` methods. Supplying one or more methods means
     * the agent MUST support both `auth/login` and `auth/logout`.
     */
    std::optional<QList<AuthMethod>> _authMethods{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

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

    bool operator==(const InitializeResponse &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<InitializeResponse> fromJson<InitializeResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const InitializeResponse &data);

/** Information about a session returned by session/list */
struct SessionInfo {
    SessionId _sessionId{};  //!< Unique identifier for the session
    AbsolutePath _cwd{};  //!< The working directory for this session. Must be an absolute path.
    /**
     * Additional workspace roots reported for this session. Each path must be absolute.
     *
     * When present, this is the complete ordered additional-root list reported
     * by the Agent. Omitted and empty values are equivalent: the response
     * reports no additional roots.
     */
    std::optional<QList<AbsolutePath>> _additionalDirectories{};
    /**
     * Human-readable title for the session
     *
     * Omitted and `null` are equivalent and mean no title is provided.
     */
    Patch<QString> _title{};
    /**
     * RFC 3339 timestamp of last activity.
     *
     * Omitted and `null` are equivalent and mean the last activity time is not reported.
     */
    Patch<QString> _updatedAt{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

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

    bool operator==(const SessionInfo &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<SessionInfo> fromJson<SessionInfo>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionInfo &data);

using SessionListCursor = QString;

/** Response from listing sessions. */
struct ListSessionsResponse {
    QList<SessionInfo> _sessions{};  //!< Array of session information objects.
    /**
     * Opaque cursor token. If present, pass this in the next request's cursor parameter
     * to fetch the next page. Omitted and `null` are equivalent and mean there are no more
     * results. An empty string is a valid cursor, not the end of results.
     */
    Patch<SessionListCursor> _nextCursor{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    ListSessionsResponse& sessions(const QList<SessionInfo> & v) { _sessions = v; return *this; }
    ListSessionsResponse& addSession(const SessionInfo & v) { _sessions.append(v); return *this; }
    ListSessionsResponse& nextCursor(const Patch<SessionListCursor> & v) { _nextCursor = v; return *this; }
    ListSessionsResponse& nextCursor(const SessionListCursor & v) { _nextCursor = v; return *this; }
    ListSessionsResponse& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    ListSessionsResponse& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const QList<SessionInfo>& sessions() const { return _sessions; }
    const Patch<SessionListCursor>& nextCursor() const { return _nextCursor; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const ListSessionsResponse &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<ListSessionsResponse> fromJson<ListSessionsResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ListSessionsResponse &data);

/** Response to the `auth/login` method. */
struct LoginAuthResponse {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    LoginAuthResponse& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    LoginAuthResponse& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const LoginAuthResponse &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<LoginAuthResponse> fromJson<LoginAuthResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const LoginAuthResponse &data);

/** Response to the `auth/logout` method. */
struct LogoutAuthResponse {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    LogoutAuthResponse& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    LogoutAuthResponse& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const LogoutAuthResponse &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<LogoutAuthResponse> fromJson<LogoutAuthResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const LogoutAuthResponse &data);

/** All text that was typed after the command name is provided as input. */
struct TextCommandInput {
    QString _hint{};  //!< A hint to display when the input hasn't been provided yet
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    TextCommandInput& hint(const QString & v) { _hint = v; return *this; }
    TextCommandInput& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    TextCommandInput& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const QString& hint() const { return _hint; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const TextCommandInput &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<TextCommandInput> fromJson<TextCommandInput>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const TextCommandInput &data);

/** The input specification for a command. */
using AvailableCommandInput = std::variant<TextCommandInput, QJsonObject>;

template<>
ACPLIB_EXPORT Utils::Result<AvailableCommandInput> fromJson<AvailableCommandInput>(const QJsonValue &val);

/** Returns the 'type' dispatch field value for the active variant. */
ACPLIB_EXPORT QString dispatchValue(const AvailableCommandInput &val);

ACPLIB_EXPORT QJsonObject toJson(const AvailableCommandInput &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const AvailableCommandInput &val);

/**
 * A slash command the user can invoke by including it in a prompt.
 *
 * The `type` discriminator value is `command`.
 */
struct Command {
    /**
     * Command name without the leading `/` (e.g., `create_plan`, `research_codebase`).
     *
     * MUST NOT contain whitespace and MUST be unique within the command list.
     */
    QString _name{};
    QString _description{};  //!< Human-readable description of what the command does.
    /**
     * Input for the command if required
     *
     * Omitted and `null` are equivalent and mean the command takes no input.
     */
    Patch<AvailableCommandInput> _input{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    Command& name(const QString & v) { _name = v; return *this; }
    Command& description(const QString & v) { _description = v; return *this; }
    Command& input(const Patch<AvailableCommandInput> & v) { _input = v; return *this; }
    Command& input(const AvailableCommandInput & v) { _input = v; return *this; }
    Command& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    Command& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const QString& name() const { return _name; }
    const QString& description() const { return _description; }
    const Patch<AvailableCommandInput>& input() const { return _input; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const Command &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<Command> fromJson<Command>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const Command &data);

/**
 * A command the agent advertises to the client.
 *
 * The `type` field acts as the discriminator in the serialized JSON form.
 */
using AvailableCommand = std::variant<Command, QJsonObject>;

template<>
ACPLIB_EXPORT Utils::Result<AvailableCommand> fromJson<AvailableCommand>(const QJsonValue &val);

/** Returns the 'type' dispatch field value for the active variant. */
ACPLIB_EXPORT QString dispatchValue(const AvailableCommand &val);

ACPLIB_EXPORT QJsonObject toJson(const AvailableCommand &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const AvailableCommand &val);

/** A boolean on/off toggle session configuration option payload. */
struct SessionConfigBoolean {
    bool _currentValue{};  //!< The current value of the boolean option.

    SessionConfigBoolean& currentValue(bool v) { _currentValue = v; return *this; }

    const bool& currentValue() const { return _currentValue; }

    bool operator==(const SessionConfigBoolean &other) const = default;
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
    SessionConfigValueId _value{};  //!< Unique identifier for this option value.
    QString _name{};  //!< Human-readable label for this option value.
    /**
     * Optional description for this option value.
     *
     * Omitted and `null` are equivalent and mean no description is provided.
     */
    Patch<QString> _description{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

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

    bool operator==(const SessionConfigSelectOption &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<SessionConfigSelectOption> fromJson<SessionConfigSelectOption>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionConfigSelectOption &data);

/** A group of possible values for a session configuration option. */
struct SessionConfigSelectGroup {
    SessionConfigGroupId _groupId{};  //!< Unique identifier for this group.
    QString _name{};  //!< Human-readable label for this group.
    QList<SessionConfigSelectOption> _options{};  //!< The set of option values in this group. MUST contain at least one value.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

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

    bool operator==(const SessionConfigSelectGroup &other) const = default;
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
    SessionConfigValueId _currentValue{};  //!< The currently selected value. MUST be one of the `value`s in `options`.
    /**
     * The set of selectable options. MUST contain at least one value, and each `value` MUST be
     * unique within the option, including across groups.
     */
    SessionConfigSelectOptions _options{};

    SessionConfigSelect& currentValue(const SessionConfigValueId & v) { _currentValue = v; return *this; }
    SessionConfigSelect& options(const SessionConfigSelectOptions & v) { _options = v; return *this; }

    const SessionConfigValueId& currentValue() const { return _currentValue; }
    const SessionConfigSelectOptions& options() const { return _options; }

    bool operator==(const SessionConfigSelect &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<SessionConfigSelect> fromJson<SessionConfigSelect>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionConfigSelect &data);

/** A session configuration option selector and its current state. */
struct SessionConfigOption {
    SessionConfigId _configId{};  //!< Unique identifier for the configuration option.
    QString _name{};  //!< Human-readable label for the option.
    /**
     * Optional description for the Client to display to the user.
     *
     * Omitted and `null` are equivalent and mean no description is provided.
     */
    Patch<QString> _description{};
    /**
     * Optional semantic category for this option (UX only).
     *
     * Omitted and `null` are equivalent and mean no category is provided.
     */
    Patch<SessionConfigOptionCategory> _category{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};
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

    bool operator==(const SessionConfigOption &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<SessionConfigOption> fromJson<SessionConfigOption>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionConfigOption &data);

/**
 * Response from creating a new session.
 *
 * New sessions default to `idle` until a state update reports otherwise.
 * Updates may arrive before or after this response. Clients MUST NOT overwrite
 * an already-reported state when the response arrives.
 *
 * See protocol docs: [Creating a Session](https://agentclientprotocol.com/protocol/v2/session-setup#creating-a-session)
 */
struct NewSessionResponse {
    /**
     * Unique identifier for the created session.
     *
     * Used in all subsequent requests for this conversation.
     */
    SessionId _sessionId{};
    /**
     * Initial session configuration options.
     *
     * Optional. Omitted or empty means no config options are advertised.
     * Senders MUST use an array, not `null`. Receivers treat `null` like omission.
     * Later `config_option_update` notifications replace this list.
     */
    std::optional<QList<SessionConfigOption>> _configOptions{};
    /**
     * Initial commands the agent can execute in this session.
     *
     * Optional. Omitted or empty means no initial commands are advertised.
     * Senders MUST use an array, not `null`. Receivers treat `null` like omission.
     * Later `available_commands_update` notifications replace this list.
     */
    std::optional<QList<AvailableCommand>> _availableCommands{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    NewSessionResponse& sessionId(const SessionId & v) { _sessionId = v; return *this; }
    NewSessionResponse& configOptions(const std::optional<QList<SessionConfigOption>> & v) { _configOptions = v; return *this; }
    NewSessionResponse& addConfigOption(const SessionConfigOption & v) { if (!_configOptions) _configOptions = QList<SessionConfigOption>{}; (*_configOptions).append(v); return *this; }
    NewSessionResponse& availableCommands(const std::optional<QList<AvailableCommand>> & v) { _availableCommands = v; return *this; }
    NewSessionResponse& addAvailableCommand(const AvailableCommand & v) { if (!_availableCommands) _availableCommands = QList<AvailableCommand>{}; (*_availableCommands).append(v); return *this; }
    NewSessionResponse& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    NewSessionResponse& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const SessionId& sessionId() const { return _sessionId; }
    const std::optional<QList<SessionConfigOption>>& configOptions() const { return _configOptions; }
    const std::optional<QList<AvailableCommand>>& availableCommands() const { return _availableCommands; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const NewSessionResponse &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<NewSessionResponse> fromJson<NewSessionResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const NewSessionResponse &data);

using MessageId = QString;

/**
 * Response acknowledging that a user prompt was inserted into the ACP conversation.
 *
 * This response does not indicate that the prompt was merely received or held, nor that the
 * agent has finished processing it.
 * Processing and completion are reported through `state_update` session updates.
 *
 * Insertion is the logical addition of a user message with a message ID and a position
 * relative to other messages in the ACP conversation.
 *
 * See protocol docs: [Prompt Accepted](https://agentclientprotocol.com/protocol/v2/prompt-lifecycle#2-prompt-accepted)
 */
struct PromptResponse {
    /**
     * Identifies the user message inserted into the ACP conversation.
     *
     * Required and non-null. Omission and explicit `null` are both invalid.
     *
     * The corresponding user-message session update carries this same identifier and may arrive
     * before or after this response. Agents must echo the message during the live session, but are
     * not required to retain it. If retained and replayed, the message keeps this identifier.
     */
    MessageId _messageId{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    PromptResponse& messageId(const MessageId & v) { _messageId = v; return *this; }
    PromptResponse& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    PromptResponse& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const MessageId& messageId() const { return _messageId; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const PromptResponse &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<PromptResponse> fromJson<PromptResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const PromptResponse &data);

/**
 * Response from resuming an existing session.
 *
 * The agent MUST send a current `state_update` after any requested history
 * replay and before this response, even without replay. The update reports
 * foreground state after resuming. The response alone does not imply `idle`.
 *
 * See protocol docs: [Current-State Synchronization](https://agentclientprotocol.com/protocol/v2/session-setup#current-state-synchronization)
 */
struct ResumeSessionResponse {
    /**
     * Initial session configuration options.
     *
     * Optional. Omitted or empty means no config options are advertised.
     * Senders MUST use an array, not `null`. Receivers treat `null` like omission.
     * Later `config_option_update` notifications replace this list.
     */
    std::optional<QList<SessionConfigOption>> _configOptions{};
    /**
     * Initial commands the agent can execute in this session.
     *
     * Optional. Omitted or empty means no initial commands are advertised.
     * Senders MUST use an array, not `null`. Receivers treat `null` like omission.
     * Later `available_commands_update` notifications replace this list.
     */
    std::optional<QList<AvailableCommand>> _availableCommands{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    ResumeSessionResponse& configOptions(const std::optional<QList<SessionConfigOption>> & v) { _configOptions = v; return *this; }
    ResumeSessionResponse& addConfigOption(const SessionConfigOption & v) { if (!_configOptions) _configOptions = QList<SessionConfigOption>{}; (*_configOptions).append(v); return *this; }
    ResumeSessionResponse& availableCommands(const std::optional<QList<AvailableCommand>> & v) { _availableCommands = v; return *this; }
    ResumeSessionResponse& addAvailableCommand(const AvailableCommand & v) { if (!_availableCommands) _availableCommands = QList<AvailableCommand>{}; (*_availableCommands).append(v); return *this; }
    ResumeSessionResponse& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    ResumeSessionResponse& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const std::optional<QList<SessionConfigOption>>& configOptions() const { return _configOptions; }
    const std::optional<QList<AvailableCommand>>& availableCommands() const { return _availableCommands; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const ResumeSessionResponse &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<ResumeSessionResponse> fromJson<ResumeSessionResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ResumeSessionResponse &data);

/** Response to `session/set_config_option` method. */
struct SetSessionConfigOptionResponse {
    QList<SessionConfigOption> _configOptions{};  //!< The full set of configuration options and their current values.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    SetSessionConfigOptionResponse& configOptions(const QList<SessionConfigOption> & v) { _configOptions = v; return *this; }
    SetSessionConfigOptionResponse& addConfigOption(const SessionConfigOption & v) { _configOptions.append(v); return *this; }
    SetSessionConfigOptionResponse& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    SetSessionConfigOptionResponse& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const QList<SessionConfigOption>& configOptions() const { return _configOptions; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const SetSessionConfigOptionResponse &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<SetSessionConfigOptionResponse> fromJson<SetSessionConfigOptionResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SetSessionConfigOptionResponse &data);

/** A JSON-RPC response object. */
using AgentResponse = std::variant<QJsonObject>;

template<>
ACPLIB_EXPORT Utils::Result<AgentResponse> fromJson<AgentResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const AgentResponse &val);

/**
 * Notification sent by the agent when a URL-based elicitation is complete.
 *
 * This notification is not a foreground state update.
 */
struct CompleteElicitationNotification {
    ElicitationId _elicitationId{};  //!< The ID of the elicitation that completed.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    CompleteElicitationNotification& elicitationId(const ElicitationId & v) { _elicitationId = v; return *this; }
    CompleteElicitationNotification& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    CompleteElicitationNotification& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const ElicitationId& elicitationId() const { return _elicitationId; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const CompleteElicitationNotification &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<CompleteElicitationNotification> fromJson<CompleteElicitationNotification>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const CompleteElicitationNotification &data);

// Skipped unknown type alias: ExtNotification

/**
 * An agent message upsert.
 *
 * Only `messageId` is required. `content` has patch semantics:
 * an omitted field leaves existing message content unchanged, `null` clears the
 * value, and a concrete array replaces the previous value. For a new
 * `messageId`, omitted `content` starts empty. `content` is replaced as a
 * whole array. Send `[]` or `null` to clear it.
 *
 * Message updates and chunks are applied in the order they are received. When
 * an `agent_message` update includes `content`, that array replaces any
 * content previously accumulated for the message, including content from
 * earlier chunks. Later chunks with the same `messageId` append to the current
 * content.
 */
struct AgentMessage {
    MessageId _messageId{};  //!< A unique identifier for the message.
    Patch<QList<ContentBlock>> _content{};  //!< Complete replacement content for this message.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted leaves the stored metadata unchanged. `null`
     * clears it. An object patches the stored metadata by top-level key: keys it omits are
     * unchanged, a key set to `null` is removed, and any other value replaces that key's
     * stored value without merging nested objects. Beyond this patch rule, implementations
     * MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Metadata Patches](https://agentclientprotocol.com/protocol/v2/extensibility#metadata-patches)
     */
    Patch<QJsonObject> __meta{};

    AgentMessage& messageId(const MessageId & v) { _messageId = v; return *this; }
    AgentMessage& content(const Patch<QList<ContentBlock>> & v) { _content = v; return *this; }
    AgentMessage& content(const QList<ContentBlock> & v) { _content = v; return *this; }
    AgentMessage& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    AgentMessage& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const MessageId& messageId() const { return _messageId; }
    const Patch<QList<ContentBlock>>& content() const { return _content; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const AgentMessage &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<AgentMessage> fromJson<AgentMessage>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const AgentMessage &data);

/**
 * An agent thought or reasoning message upsert.
 *
 * Only `messageId` is required. `content` has patch semantics:
 * an omitted field leaves existing thought content unchanged, `null` clears the
 * value, and a concrete array replaces the previous value. For a new
 * `messageId`, omitted `content` starts empty. `content` is replaced as a
 * whole array. Send `[]` or `null` to clear it.
 *
 * Message updates and chunks are applied in the order they are received. When
 * an `agent_thought` update includes `content`, that array replaces any
 * content previously accumulated for the thought, including content from
 * earlier chunks. Later chunks with the same `messageId` append to the current
 * content.
 */
struct AgentThought {
    MessageId _messageId{};  //!< A unique identifier for the thought message.
    Patch<QList<ContentBlock>> _content{};  //!< Complete replacement content for this thought message.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted leaves the stored metadata unchanged. `null`
     * clears it. An object patches the stored metadata by top-level key: keys it omits are
     * unchanged, a key set to `null` is removed, and any other value replaces that key's
     * stored value without merging nested objects. Beyond this patch rule, implementations
     * MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Metadata Patches](https://agentclientprotocol.com/protocol/v2/extensibility#metadata-patches)
     */
    Patch<QJsonObject> __meta{};

    AgentThought& messageId(const MessageId & v) { _messageId = v; return *this; }
    AgentThought& content(const Patch<QList<ContentBlock>> & v) { _content = v; return *this; }
    AgentThought& content(const QList<ContentBlock> & v) { _content = v; return *this; }
    AgentThought& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    AgentThought& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const MessageId& messageId() const { return _messageId; }
    const Patch<QList<ContentBlock>>& content() const { return _content; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const AgentThought &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<AgentThought> fromJson<AgentThought>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const AgentThought &data);

/** Available commands are ready or have changed */
struct AvailableCommandsUpdate {
    QList<AvailableCommand> _availableCommands{};  //!< Commands the agent can execute.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    AvailableCommandsUpdate& availableCommands(const QList<AvailableCommand> & v) { _availableCommands = v; return *this; }
    AvailableCommandsUpdate& addAvailableCommand(const AvailableCommand & v) { _availableCommands.append(v); return *this; }
    AvailableCommandsUpdate& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    AvailableCommandsUpdate& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const QList<AvailableCommand>& availableCommands() const { return _availableCommands; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const AvailableCommandsUpdate &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<AvailableCommandsUpdate> fromJson<AvailableCommandsUpdate>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const AvailableCommandsUpdate &data);

using CompactionId = QString;

/**
 * A content block appended to a compaction's summary. A first-seen ID creates
 * an in-progress compaction. Chunks append in receive order.
 */
struct CompactionSummaryChunk {
    CompactionId _compactionId{};  //!< ID of the compaction whose summary receives this content.
    ContentBlock _content{};  //!< One content block to append.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     * This metadata is scoped to the chunk and does not patch stored metadata.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    CompactionSummaryChunk& compactionId(const CompactionId & v) { _compactionId = v; return *this; }
    CompactionSummaryChunk& content(const ContentBlock & v) { _content = v; return *this; }
    CompactionSummaryChunk& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    CompactionSummaryChunk& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const CompactionId& compactionId() const { return _compactionId; }
    const ContentBlock& content() const { return _content; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const CompactionSummaryChunk &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<CompactionSummaryChunk> fromJson<CompactionSummaryChunk>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const CompactionSummaryChunk &data);

/** Lifecycle state of a context compaction. */
enum class CompactionStatus {
    in_progress,
    completed,
    failed,
    cancelled
};

ACPLIB_EXPORT QString toString(CompactionStatus v);

template<>
ACPLIB_EXPORT Utils::Result<CompactionStatus> fromJson<CompactionStatus>(const QJsonValue &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const CompactionStatus &v);

/**
 * A context compaction upsert. The first notification fixes the compaction's
 * timeline position. Later updates with the same ID patch that entity in place.
 *
 * `summary`, `error`, and `_meta` have patch semantics: omission leaves the
 * stored value unchanged, `null` clears it, and a concrete value replaces it.
 * `summary: []` also clears the summary. A concrete `_meta` object patches the
 * stored metadata by top-level key instead of replacing it.
 */
struct CompactionUpdate {
    CompactionId _compactionId{};  //!< The Agent-owned ID of this compaction, unique within the session.
    CompactionStatus _status{};  //!< Current lifecycle status.
    Patch<QList<ContentBlock>> _summary{};  //!< Complete replacement user-displayable summary content for the compaction.
    Patch<QString> _error{};  //!< Human-readable error details for the compaction.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted leaves the stored metadata unchanged. `null`
     * clears it. An object patches the stored metadata by top-level key: keys it omits are
     * unchanged, a key set to `null` is removed, and any other value replaces that key's
     * stored value without merging nested objects. Beyond this patch rule, implementations
     * MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Metadata Patches](https://agentclientprotocol.com/protocol/v2/extensibility#metadata-patches)
     */
    Patch<QJsonObject> __meta{};

    CompactionUpdate& compactionId(const CompactionId & v) { _compactionId = v; return *this; }
    CompactionUpdate& status(const CompactionStatus & v) { _status = v; return *this; }
    CompactionUpdate& summary(const Patch<QList<ContentBlock>> & v) { _summary = v; return *this; }
    CompactionUpdate& summary(const QList<ContentBlock> & v) { _summary = v; return *this; }
    CompactionUpdate& error(const Patch<QString> & v) { _error = v; return *this; }
    CompactionUpdate& error(const QString & v) { _error = v; return *this; }
    CompactionUpdate& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    CompactionUpdate& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const CompactionId& compactionId() const { return _compactionId; }
    const CompactionStatus& status() const { return _status; }
    const Patch<QList<ContentBlock>>& summary() const { return _summary; }
    const Patch<QString>& error() const { return _error; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const CompactionUpdate &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<CompactionUpdate> fromJson<CompactionUpdate>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const CompactionUpdate &data);

/** Session configuration options have been updated. */
struct ConfigOptionUpdate {
    QList<SessionConfigOption> _configOptions{};  //!< The full set of configuration options and their current values.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    ConfigOptionUpdate& configOptions(const QList<SessionConfigOption> & v) { _configOptions = v; return *this; }
    ConfigOptionUpdate& addConfigOption(const SessionConfigOption & v) { _configOptions.append(v); return *this; }
    ConfigOptionUpdate& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    ConfigOptionUpdate& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const QList<SessionConfigOption>& configOptions() const { return _configOptions; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const ConfigOptionUpdate &other) const = default;
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
     * Chunks for different messages can interleave.
     */
    MessageId _messageId{};
    ContentBlock _content{};  //!< A single item of content
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     * This metadata is scoped to the chunk and does not patch stored metadata.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    ContentChunk& messageId(const MessageId & v) { _messageId = v; return *this; }
    ContentChunk& content(const ContentBlock & v) { _content = v; return *this; }
    ContentChunk& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    ContentChunk& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const MessageId& messageId() const { return _messageId; }
    const ContentBlock& content() const { return _content; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const ContentChunk &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<ContentChunk> fromJson<ContentChunk>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ContentChunk &data);

/** Severity hint for a session notice. */
enum class NoticeSeverity {
    info,
    warning,
    error
};

ACPLIB_EXPORT QString toString(NoticeSeverity v);

template<>
ACPLIB_EXPORT Utils::Result<NoticeSeverity> fromJson<NoticeSeverity>(const QJsonValue &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const NoticeSeverity &v);

/**
 * Fire-and-forget information for the user.
 *
 * Notices are live events rather than session history. Agents must not rely on
 * a notice being received, displayed, or seen by the user.
 * No Client capability is required, and unsupported Clients may ignore notices.
 *
 * See protocol docs: [Session Notices](https://agentclientprotocol.com/protocol/v2/prompt-lifecycle#session-notices)
 */
struct Notice {
    NoticeSeverity _severity{};  //!< Presentation severity hint.
    QString _title{};  //!< Required non-empty plain-text title that can stand alone.
    /**
     * Optional plain-text detail or guidance.
     *
     * Omitted and `null` are equivalent and mean no description was supplied.
     */
    Patch<QString> _description{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    Notice& severity(const NoticeSeverity & v) { _severity = v; return *this; }
    Notice& title(const QString & v) { _title = v; return *this; }
    Notice& description(const Patch<QString> & v) { _description = v; return *this; }
    Notice& description(const QString & v) { _description = v; return *this; }
    Notice& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    Notice& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const NoticeSeverity& severity() const { return _severity; }
    const QString& title() const { return _title; }
    const Patch<QString>& description() const { return _description; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const Notice &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<Notice> fromJson<Notice>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const Notice &data);

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
    QString _content{};  //!< Human-readable description of what this task aims to accomplish.
    /**
     * The relative importance of this task.
     * Used to indicate which tasks are most critical to the overall goal.
     */
    PlanEntryPriority _priority{};
    PlanEntryStatus _status{};  //!< Current execution status of this task.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    PlanEntry& content(const QString & v) { _content = v; return *this; }
    PlanEntry& priority(const PlanEntryPriority & v) { _priority = v; return *this; }
    PlanEntry& status(const PlanEntryStatus & v) { _status = v; return *this; }
    PlanEntry& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    PlanEntry& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const QString& content() const { return _content; }
    const PlanEntryPriority& priority() const { return _priority; }
    const PlanEntryStatus& status() const { return _status; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const PlanEntry &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<PlanEntry> fromJson<PlanEntry>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const PlanEntry &data);

/** A plan represented as structured entries. */
struct PlanItems {
    PlanId _planId{};  //!< The plan ID to update.
    /**
     * The list of tasks to be accomplished.
     *
     * When updating an item-based plan, the agent must send a complete list of all entries
     * with their current status. The client replaces that plan with each update.
     */
    QList<PlanEntry> _entries{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    PlanItems& planId(const PlanId & v) { _planId = v; return *this; }
    PlanItems& entries(const QList<PlanEntry> & v) { _entries = v; return *this; }
    PlanItems& addEntry(const PlanEntry & v) { _entries.append(v); return *this; }
    PlanItems& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    PlanItems& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const PlanId& planId() const { return _planId; }
    const QList<PlanEntry>& entries() const { return _entries; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const PlanItems &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<PlanItems> fromJson<PlanItems>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const PlanItems &data);

/** Updated content for a plan. */
using PlanUpdateContent = std::variant<PlanItems, QJsonObject>;

template<>
ACPLIB_EXPORT Utils::Result<PlanUpdateContent> fromJson<PlanUpdateContent>(const QJsonValue &val);

/** Returns the 'type' dispatch field value for the active variant. */
ACPLIB_EXPORT QString dispatchValue(const PlanUpdateContent &val);

ACPLIB_EXPORT QJsonObject toJson(const PlanUpdateContent &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const PlanUpdateContent &val);

/** A content update for a plan identified by ID. */
struct PlanUpdate {
    PlanUpdateContent _plan{};  //!< The updated plan content.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    PlanUpdate& plan(const PlanUpdateContent & v) { _plan = v; return *this; }
    PlanUpdate& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    PlanUpdate& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const PlanUpdateContent& plan() const { return _plan; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const PlanUpdate &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<PlanUpdate> fromJson<PlanUpdate>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const PlanUpdate &data);

/**
 * Update to session metadata. All fields are optional to support partial updates.
 *
 * Agents send this notification to update session information like title or custom metadata.
 * Clients can use it to keep session titles and state current.
 *
 * Omitted fields leave the existing session info unchanged. `null` clears the
 * corresponding value. A concrete `_meta` object patches the stored metadata by
 * top-level key.
 */
struct SessionInfoUpdate {
    Patch<QString> _title{};  //!< Human-readable title for the session. Set to null to clear.
    Patch<QString> _updatedAt{};  //!< RFC 3339 timestamp of last activity. Set to null to clear.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted leaves the stored metadata unchanged. `null`
     * clears it. An object patches the stored metadata by top-level key: keys it omits are
     * unchanged, a key set to `null` is removed, and any other value replaces that key's
     * stored value without merging nested objects. Beyond this patch rule, implementations
     * MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Metadata Patches](https://agentclientprotocol.com/protocol/v2/extensibility#metadata-patches)
     */
    Patch<QJsonObject> __meta{};

    SessionInfoUpdate& title(const Patch<QString> & v) { _title = v; return *this; }
    SessionInfoUpdate& title(const QString & v) { _title = v; return *this; }
    SessionInfoUpdate& updatedAt(const Patch<QString> & v) { _updatedAt = v; return *this; }
    SessionInfoUpdate& updatedAt(const QString & v) { _updatedAt = v; return *this; }
    SessionInfoUpdate& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    SessionInfoUpdate& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QString>& title() const { return _title; }
    const Patch<QString>& updatedAt() const { return _updatedAt; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const SessionInfoUpdate &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<SessionInfoUpdate> fromJson<SessionInfoUpdate>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionInfoUpdate &data);

/** A listed terminal running in the background. */
struct BackgroundTerminal {
    TerminalId _terminalId{};  //!< The ID of the listed terminal.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    BackgroundTerminal& terminalId(const TerminalId & v) { _terminalId = v; return *this; }
    BackgroundTerminal& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    BackgroundTerminal& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const TerminalId& terminalId() const { return _terminalId; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const BackgroundTerminal &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<BackgroundTerminal> fromJson<BackgroundTerminal>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const BackgroundTerminal &data);

/** A listed tool call whose work continues in the background. */
struct BackgroundToolCall {
    ToolCallId _toolCallId{};  //!< The ID of the listed tool call.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    BackgroundToolCall& toolCallId(const ToolCallId & v) { _toolCallId = v; return *this; }
    BackgroundToolCall& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    BackgroundToolCall& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const ToolCallId& toolCallId() const { return _toolCallId; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const BackgroundToolCall &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<BackgroundToolCall> fromJson<BackgroundToolCall>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const BackgroundToolCall &data);

/**
 * One piece of work running independently of the session's foreground work.
 *
 * Items reference entities the Client already tracks. The Agent MUST announce each item
 * before the first snapshot that lists it, and MUST report a terminal's `exitStatus` or a
 * tool call's final status before the snapshot that removes it. Each piece of work is
 * listed once: process-backed work is listed as its terminal, not also as its tool call.
 *
 * See protocol docs: [Background Work](https://agentclientprotocol.com/protocol/v2/prompt-lifecycle#background-work)
 */
using BackgroundWork = std::variant<BackgroundTerminal, BackgroundToolCall, QJsonObject>;

template<>
ACPLIB_EXPORT Utils::Result<BackgroundWork> fromJson<BackgroundWork>(const QJsonValue &val);

/** Returns the 'type' dispatch field value for the active variant. */
ACPLIB_EXPORT QString dispatchValue(const BackgroundWork &val);

ACPLIB_EXPORT QJsonObject toJson(const BackgroundWork &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const BackgroundWork &val);

/** Details of a failure that ended active work. */
struct ErrorStopReason {
    /**
     * The failure, as a JSON-RPC error object.
     *
     * Optional. Omitted and `null` are equivalent and mean the agent is not reporting failure details.
     * Agents SHOULD include it.
     */
    Patch<Error> _error{};

    ErrorStopReason& error(const Patch<Error> & v) { _error = v; return *this; }
    ErrorStopReason& error(const Error & v) { _error = v; return *this; }

    const Patch<Error>& error() const { return _error; }

    bool operator==(const ErrorStopReason &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<ErrorStopReason> fromJson<ErrorStopReason>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ErrorStopReason &data);

/**
 * No foreground work is active. Background activity may continue.
 *
 * Agents SHOULD include a `stopReason` when the idle transition ends foreground
 * work. An idle update that does not end foreground work need not include one.
 * Receivers treat an omitted, `null`, or malformed `stopReason` as the agent not
 * reporting one.
 * Repeated or resume snapshots may carry the last stop reason for context.
 * They MUST NOT be counted as new completions, cancellations, or additional usage.
 */
struct IdleStateUpdate {
    /**
     * Work running independently of foreground work, as a complete snapshot.
     *
     * Optional. Omitted and `null` are equivalent and mean the Agent is not reporting
     * background work, not that nothing is running. `[]` means no background work is
     * running. An Agent that reports background work MUST include it on every state
     * update for the session. A malformed list is treated as not reported.
     *
     * See protocol docs: [Background Work](https://agentclientprotocol.com/protocol/v2/prompt-lifecycle#background-work)
     */
    Patch<QList<BackgroundWork>> _background{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};
    /**
     * Why foreground work stopped. The value selects one of this type's variants, which may add fields of their own.
     *
     * Optional. Omitted and `null` are equivalent and mean the agent is not reporting a stop reason; a malformed value is treated the same way.
     *
     * See protocol docs: [Stop Reasons](https://agentclientprotocol.com/protocol/v2/prompt-lifecycle#stop-reasons)
     */
    Patch<QString> _stopReason{};
    QJsonObject _additionalProperties;  //!< additional properties

    IdleStateUpdate& background(const Patch<QList<BackgroundWork>> & v) { _background = v; return *this; }
    IdleStateUpdate& background(const QList<BackgroundWork> & v) { _background = v; return *this; }
    IdleStateUpdate& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    IdleStateUpdate& _meta(const QJsonObject & v) { __meta = v; return *this; }
    IdleStateUpdate& stopReason(const Patch<QString> & v) { _stopReason = v; return *this; }
    IdleStateUpdate& stopReason(const QString & v) { _stopReason = v; return *this; }
    IdleStateUpdate& additionalProperties(const QString &key, const QJsonValue &v) { _additionalProperties.insert(key, v); return *this; }
    IdleStateUpdate& additionalProperties(const QJsonObject &obj) { for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) _additionalProperties.insert(it.key(), it.value()); return *this; }

    const Patch<QList<BackgroundWork>>& background() const { return _background; }
    const Patch<QJsonObject>& _meta() const { return __meta; }
    const Patch<QString>& stopReason() const { return _stopReason; }
    const QJsonObject& additionalProperties() const { return _additionalProperties; }

    bool operator==(const IdleStateUpdate &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<IdleStateUpdate> fromJson<IdleStateUpdate>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const IdleStateUpdate &data);

/**
 * The foreground workflow as a whole is blocked on user action.
 *
 * Agents SHOULD report this state when foreground work cannot progress without
 * user action. Pending permissions or elicitations alone do not imply this
 * state if other foreground work can progress, or if the requests belong only
 * to independent background activity. Once reported, agents MUST report
 * `running` when progress resumes, or `idle` if the work ends without resuming.
 * Responding to one request does not itself imply `running`.
 */
struct RequiresActionStateUpdate {
    /**
     * Work running independently of foreground work, as a complete snapshot.
     *
     * Optional. Omitted and `null` are equivalent and mean the Agent is not reporting
     * background work, not that nothing is running. `[]` means no background work is
     * running. An Agent that reports background work MUST include it on every state
     * update for the session. A malformed list is treated as not reported.
     *
     * See protocol docs: [Background Work](https://agentclientprotocol.com/protocol/v2/prompt-lifecycle#background-work)
     */
    Patch<QList<BackgroundWork>> _background{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    RequiresActionStateUpdate& background(const Patch<QList<BackgroundWork>> & v) { _background = v; return *this; }
    RequiresActionStateUpdate& background(const QList<BackgroundWork> & v) { _background = v; return *this; }
    RequiresActionStateUpdate& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    RequiresActionStateUpdate& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QList<BackgroundWork>>& background() const { return _background; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const RequiresActionStateUpdate &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<RequiresActionStateUpdate> fromJson<RequiresActionStateUpdate>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const RequiresActionStateUpdate &data);

/** Foreground work is in progress. */
struct RunningStateUpdate {
    /**
     * Work running independently of foreground work, as a complete snapshot.
     *
     * Optional. Omitted and `null` are equivalent and mean the Agent is not reporting
     * background work, not that nothing is running. `[]` means no background work is
     * running. An Agent that reports background work MUST include it on every state
     * update for the session. A malformed list is treated as not reported.
     *
     * See protocol docs: [Background Work](https://agentclientprotocol.com/protocol/v2/prompt-lifecycle#background-work)
     */
    Patch<QList<BackgroundWork>> _background{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    RunningStateUpdate& background(const Patch<QList<BackgroundWork>> & v) { _background = v; return *this; }
    RunningStateUpdate& background(const QList<BackgroundWork> & v) { _background = v; return *this; }
    RunningStateUpdate& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    RunningStateUpdate& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QList<BackgroundWork>>& background() const { return _background; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const RunningStateUpdate &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<RunningStateUpdate> fromJson<RunningStateUpdate>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const RunningStateUpdate &data);

/**
 * A snapshot of the session's aggregate foreground activity.
 *
 * Foreground work is the agent's active response or workflow in the session,
 * including model processing, tool execution, and waiting for user action needed
 * to continue that work. It can start with a prompt or be agent-initiated, for
 * example to pursue an ongoing goal or follow up on a completed background task.
 * Either way, the agent MUST report `running` when foreground work starts,
 * or `requires_action` if it starts blocked on user action, and `idle` when it
 * ends. The agent SHOULD include a stop reason on that idle update.
 * Agent-initiated work does not require a prompt request or a synthetic user
 * message.
 *
 * `running` may precede user-message insertion, for example during a foreground
 * hook. It is not acknowledgment of insertion, and rejecting a prompt does not
 * itself end foreground work.
 *
 * Background activity can continue and emit other `session/update` notifications
 * while `idle`. Those notifications, including a detached task's progress or
 * completion, do not by themselves change this state. A permission request only
 * warrants `requires_action` when foreground work is blocked on user action.
 *
 * The agent SHOULD report `requires_action` when the foreground workflow as a
 * whole cannot progress without user action, including permissions or elicitations.
 * Once reported, it MUST report `running` when progress resumes, or `idle` when
 * work ends without resuming.
 * If other foreground work can progress, the state remains `running`. Resolving
 * one of several requests does not itself change the state. Agents SHOULD ignore
 * responses for work that has already ended rather than use them to resume it.
 *
 * A new session defaults to idle until a state update reports otherwise.
 * Resume MUST send a current-state snapshot after any replay and before its
 * response. The snapshot reports foreground state after resuming.
 * The response alone does not imply idle.
 * Repeated snapshots are idempotent, not additional work, completions, or usage.
 * A repeated state may carry updated information without another state transition.
 * The agent MUST report `idle` when no foreground activity remains. Completing
 * an individual activity while other foreground work continues does not require
 * an idle boundary. Local commands use the same lifecycle even without model work.
 *
 * See protocol docs: [Session States](https://agentclientprotocol.com/protocol/v2/prompt-lifecycle#session-states)
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
    TerminalId _terminalId{};  //!< The terminal receiving these bytes.
    QString _data{};  //!< Independently base64-encoded terminal output bytes.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     * This metadata is scoped to the chunk and does not patch stored metadata.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    TerminalOutputChunk& terminalId(const TerminalId & v) { _terminalId = v; return *this; }
    TerminalOutputChunk& data(const QString & v) { _data = v; return *this; }
    TerminalOutputChunk& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    TerminalOutputChunk& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const TerminalId& terminalId() const { return _terminalId; }
    const QString& data() const { return _data; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const TerminalOutputChunk &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<TerminalOutputChunk> fromJson<TerminalOutputChunk>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const TerminalOutputChunk &data);

/**
 * Capability to cancel a terminal with `terminal/cancel`.
 *
 * Supplying `{}` advertises support. An omitted or `null` `cancel` does not.
 */
struct TerminalCancelCapabilities {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    TerminalCancelCapabilities& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    TerminalCancelCapabilities& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const TerminalCancelCapabilities &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<TerminalCancelCapabilities> fromJson<TerminalCancelCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const TerminalCancelCapabilities &data);

/**
 * Client-initiated controls a terminal currently supports.
 *
 * A malformed value falls back to no controls, so the Client never sends a request the
 * Agent did not advertise.
 *
 * See protocol docs: [Cancelling Terminals](https://agentclientprotocol.com/protocol/v2/tool-calls#cancelling-terminals)
 */
struct TerminalCapabilities {
    /**
     * Permits the Client to cancel this terminal with `terminal/cancel`.
     *
     * Omitted and `null` are equivalent and mean unsupported. An object (including `{}`)
     * means supported. Agents SHOULD stop advertising it once the terminal reports `exitStatus`.
     */
    Patch<TerminalCancelCapabilities> _cancel{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    TerminalCapabilities& cancel(const Patch<TerminalCancelCapabilities> & v) { _cancel = v; return *this; }
    TerminalCapabilities& cancel(const TerminalCancelCapabilities & v) { _cancel = v; return *this; }
    TerminalCapabilities& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    TerminalCapabilities& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<TerminalCancelCapabilities>& cancel() const { return _cancel; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const TerminalCapabilities &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<TerminalCapabilities> fromJson<TerminalCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const TerminalCapabilities &data);

/**
 * Exit information for an agent-owned terminal.
 *
 * The presence of this object marks the terminal as exited, even when neither
 * an exit code nor a signal is known.
 */
struct TerminalExitStatus {
    /**
     * Process exit code as the platform reports it, when known. Negative values are
     * allowed, for example signed Windows exit codes. Omitted and `null` are equivalent.
     */
    Patch<int> _exitCode{};
    /**
     * Signal that terminated the process, when known.
     *
     * Agents should use the conventional platform signal name. POSIX examples
     * include `SIGTERM`, `SIGKILL`, and `SIGINT`. Other platforms may use a
     * platform-specific name. Omitted and `null` are equivalent.
     */
    Patch<QString> _signal{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    TerminalExitStatus& exitCode(const Patch<int> & v) { _exitCode = v; return *this; }
    TerminalExitStatus& exitCode(int v) { _exitCode = v; return *this; }
    TerminalExitStatus& signal(const Patch<QString> & v) { _signal = v; return *this; }
    TerminalExitStatus& signal(const QString & v) { _signal = v; return *this; }
    TerminalExitStatus& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    TerminalExitStatus& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<int>& exitCode() const { return _exitCode; }
    const Patch<QString>& signal() const { return _signal; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const TerminalExitStatus &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<TerminalExitStatus> fromJson<TerminalExitStatus>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const TerminalExitStatus &data);

/** An authoritative replacement snapshot of terminal output bytes. */
struct TerminalOutput {
    QString _data{};  //!< Base64-encoded replacement terminal output bytes.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    TerminalOutput& data(const QString & v) { _data = v; return *this; }
    TerminalOutput& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    TerminalOutput& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const QString& data() const { return _data; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const TerminalOutput &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<TerminalOutput> fromJson<TerminalOutput>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const TerminalOutput &data);

/** The size of an agent-owned terminal in character cells. */
struct TerminalSize {
    int _cols{};  //!< Number of columns.
    int _rows{};  //!< Number of rows.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    TerminalSize& cols(int v) { _cols = v; return *this; }
    TerminalSize& rows(int v) { _rows = v; return *this; }
    TerminalSize& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    TerminalSize& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const int& cols() const { return _cols; }
    const int& rows() const { return _rows; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const TerminalSize &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<TerminalSize> fromJson<TerminalSize>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const TerminalSize &data);

/**
 * An upsert for the stored state of an agent-owned terminal.
 *
 * Only `terminalId` is required. Other fields have patch
 * semantics: omitted fields leave the stored value unchanged, `null` clears
 * it, and concrete values replace it. A concrete `_meta` object instead
 * patches the stored metadata by top-level key. When the terminal ID is new,
 * omitted fields start unknown.
 */
struct TerminalUpdate {
    TerminalId _terminalId{};  //!< Unique identifier for this terminal within the session.
    Patch<QString> _command{};  //!< The command being run.
    Patch<AbsolutePath> _cwd{};  //!< The absolute working directory of the command.
    Patch<TerminalOutput> _output{};  //!< An authoritative replacement snapshot of terminal output bytes.
    Patch<TerminalExitStatus> _exitStatus{};  //!< Exit information. A concrete object marks the terminal as exited.
    /**
     * The terminal size the command runs with, when known.
     *
     * Clients can render output at this size so that column layouts, progress bars,
     * and cursor-positioned output display as the program wrote them.
     */
    Patch<TerminalSize> _size{};
    /**
     * Client-initiated controls this terminal currently supports, such as cancellation.
     *
     * Omission leaves the capabilities unchanged, `null` clears them, and an object replaces
     * the whole previous value. A new terminal starts with none.
     */
    Patch<TerminalCapabilities> _capabilities{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted leaves the stored metadata unchanged. `null`
     * clears it. An object patches the stored metadata by top-level key: keys it omits are
     * unchanged, a key set to `null` is removed, and any other value replaces that key's
     * stored value without merging nested objects. Beyond this patch rule, implementations
     * MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Metadata Patches](https://agentclientprotocol.com/protocol/v2/extensibility#metadata-patches)
     */
    Patch<QJsonObject> __meta{};

    TerminalUpdate& terminalId(const TerminalId & v) { _terminalId = v; return *this; }
    TerminalUpdate& command(const Patch<QString> & v) { _command = v; return *this; }
    TerminalUpdate& command(const QString & v) { _command = v; return *this; }
    TerminalUpdate& cwd(const Patch<AbsolutePath> & v) { _cwd = v; return *this; }
    TerminalUpdate& cwd(const AbsolutePath & v) { _cwd = v; return *this; }
    TerminalUpdate& output(const Patch<TerminalOutput> & v) { _output = v; return *this; }
    TerminalUpdate& output(const TerminalOutput & v) { _output = v; return *this; }
    TerminalUpdate& exitStatus(const Patch<TerminalExitStatus> & v) { _exitStatus = v; return *this; }
    TerminalUpdate& exitStatus(const TerminalExitStatus & v) { _exitStatus = v; return *this; }
    TerminalUpdate& size(const Patch<TerminalSize> & v) { _size = v; return *this; }
    TerminalUpdate& size(const TerminalSize & v) { _size = v; return *this; }
    TerminalUpdate& capabilities(const Patch<TerminalCapabilities> & v) { _capabilities = v; return *this; }
    TerminalUpdate& capabilities(const TerminalCapabilities & v) { _capabilities = v; return *this; }
    TerminalUpdate& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    TerminalUpdate& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const TerminalId& terminalId() const { return _terminalId; }
    const Patch<QString>& command() const { return _command; }
    const Patch<AbsolutePath>& cwd() const { return _cwd; }
    const Patch<TerminalOutput>& output() const { return _output; }
    const Patch<TerminalExitStatus>& exitStatus() const { return _exitStatus; }
    const Patch<TerminalSize>& size() const { return _size; }
    const Patch<TerminalCapabilities>& capabilities() const { return _capabilities; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const TerminalUpdate &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<TerminalUpdate> fromJson<TerminalUpdate>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const TerminalUpdate &data);

/**
 * A streamed item of tool-call content.
 *
 * Tool-call content chunks append one `ToolCallContent` item to the current
 * content for the matching `toolCallId`. Agents can send a `tool_call_update`
 * with `content` when they need to replace the whole content collection
 * instead.
 */
struct ToolCallContentChunk {
    ToolCallId _toolCallId{};  //!< The ID of the tool call this content belongs to.
    ToolCallContent _content{};  //!< A single item of content produced by the tool call.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     * This metadata is scoped to the chunk and does not patch stored metadata.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    ToolCallContentChunk& toolCallId(const ToolCallId & v) { _toolCallId = v; return *this; }
    ToolCallContentChunk& content(const ToolCallContent & v) { _content = v; return *this; }
    ToolCallContentChunk& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    ToolCallContentChunk& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const ToolCallId& toolCallId() const { return _toolCallId; }
    const ToolCallContent& content() const { return _content; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const ToolCallContentChunk &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<ToolCallContentChunk> fromJson<ToolCallContentChunk>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ToolCallContentChunk &data);

/** Cost information for a session. */
struct Cost {
    double _amount{};  //!< Total cumulative cost for session.
    /**
     * Unit of `amount`: an ISO 4217 currency code (e.g., "USD", "EUR") for money, or another
     * unit such as "credits".
     */
    QString _currency{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    Cost& amount(double v) { _amount = v; return *this; }
    Cost& currency(const QString & v) { _currency = v; return *this; }
    Cost& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    Cost& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const double& amount() const { return _amount; }
    const QString& currency() const { return _currency; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const Cost &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<Cost> fromJson<Cost>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const Cost &data);

/** Context window and cost update for a session. */
struct UsageUpdate {
    int _used{};  //!< Tokens currently in context.
    int _size{};  //!< Total context window size in tokens.
    /**
     * Cumulative session cost.
     *
     * Omitted leaves the last reported cost unchanged. `null` clears it. A value replaces it.
     */
    Patch<Cost> _cost{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

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

    bool operator==(const UsageUpdate &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<UsageUpdate> fromJson<UsageUpdate>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const UsageUpdate &data);

/**
 * A user message upsert.
 *
 * Only `messageId` is required. `content` has patch semantics:
 * an omitted field leaves existing message content unchanged, `null` clears the
 * value, and a concrete array replaces the previous value. For a new
 * `messageId`, omitted `content` starts empty. `content` is replaced as a
 * whole array. Send `[]` or `null` to clear it.
 *
 * Message updates and chunks are applied in the order they are received. When
 * a `user_message` update includes `content`, that array replaces any content
 * previously accumulated for the message, including content from earlier
 * chunks. Later chunks with the same `messageId` append to the current
 * content.
 */
struct UserMessage {
    MessageId _messageId{};  //!< A unique identifier for the message.
    Patch<QList<ContentBlock>> _content{};  //!< Complete replacement content for this message.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted leaves the stored metadata unchanged. `null`
     * clears it. An object patches the stored metadata by top-level key: keys it omits are
     * unchanged, a key set to `null` is removed, and any other value replaces that key's
     * stored value without merging nested objects. Beyond this patch rule, implementations
     * MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Metadata Patches](https://agentclientprotocol.com/protocol/v2/extensibility#metadata-patches)
     */
    Patch<QJsonObject> __meta{};

    UserMessage& messageId(const MessageId & v) { _messageId = v; return *this; }
    UserMessage& content(const Patch<QList<ContentBlock>> & v) { _content = v; return *this; }
    UserMessage& content(const QList<ContentBlock> & v) { _content = v; return *this; }
    UserMessage& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    UserMessage& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const MessageId& messageId() const { return _messageId; }
    const Patch<QList<ContentBlock>>& content() const { return _content; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const UserMessage &other) const = default;
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
    using Variant = std::variant<ContentChunk, UserMessage, AgentMessage, AgentThought, StateUpdate, ToolCallContentChunk, ToolCallUpdate, TerminalUpdate, TerminalOutputChunk, PlanUpdate, AvailableCommandsUpdate, ConfigOptionUpdate, SessionInfoUpdate, UsageUpdate, Notice, CompactionUpdate, CompactionSummaryChunk, QJsonObject>;
    Variant _value;
    QString _kind;  //!< discriminator value (sessionUpdate)

    template<typename T> const T* get() const { return std::get_if<T>(&_value); }
    const QString& kind() const { return _kind; }

    bool operator==(const SessionUpdate &other) const = default;
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
    SessionId _sessionId{};  //!< The ID of the session this update pertains to.
    SessionUpdate _update{};  //!< The actual update content.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    UpdateSessionNotification& sessionId(const SessionId & v) { _sessionId = v; return *this; }
    UpdateSessionNotification& update(const SessionUpdate & v) { _update = v; return *this; }
    UpdateSessionNotification& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    UpdateSessionNotification& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const SessionId& sessionId() const { return _sessionId; }
    const SessionUpdate& update() const { return _update; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const UpdateSessionNotification &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<UpdateSessionNotification> fromJson<UpdateSessionNotification>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const UpdateSessionNotification &data);

/** A JSON-RPC notification object. */
struct AgentNotification {
    QString _method{};  //!< The notification method name.
    std::optional<QString> _params{};  //!< Method-specific notification parameters.

    AgentNotification& method(const QString & v) { _method = v; return *this; }
    AgentNotification& params(const std::optional<QString> & v) { _params = v; return *this; }

    const QString& method() const { return _method; }
    const std::optional<QString>& params() const { return _params; }

    bool operator==(const AgentNotification &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<AgentNotification> fromJson<AgentNotification>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const AgentNotification &data);

/**
 * Request parameters for cancelling one terminal.
 *
 * Clients MUST NOT send it unless the terminal currently advertises `capabilities.cancel`.
 *
 * The Agent responds once it has accepted the cancellation. The response does not mean
 * the work has stopped. The Agent confirms it with a `terminal_update` that sets
 * `exitStatus`. Cancelling stops the process but does not release the terminal, and does
 * not change the status of a tool call that references it.
 *
 * The Agent responds with `-32602` if the terminal is unknown, has already exited, or is not
 * cancellable. Cancelling a terminal does not change the foreground state by itself.
 *
 * See protocol docs: [Cancelling Terminals](https://agentclientprotocol.com/protocol/v2/tool-calls#cancelling-terminals)
 */
struct CancelTerminalRequest {
    SessionId _sessionId{};  //!< The ID of the session that owns the terminal.
    TerminalId _terminalId{};  //!< The ID of the terminal to cancel.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    CancelTerminalRequest& sessionId(const SessionId & v) { _sessionId = v; return *this; }
    CancelTerminalRequest& terminalId(const TerminalId & v) { _terminalId = v; return *this; }
    CancelTerminalRequest& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    CancelTerminalRequest& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const SessionId& sessionId() const { return _sessionId; }
    const TerminalId& terminalId() const { return _terminalId; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const CancelTerminalRequest &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<CancelTerminalRequest> fromJson<CancelTerminalRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const CancelTerminalRequest &data);

/**
 * Request parameters for cancelling one tool call.
 *
 * Clients MUST NOT send it unless the tool call currently advertises `capabilities.cancel`.
 *
 * The Agent responds once it has accepted the cancellation. The response does not mean
 * the work has stopped. The Agent confirms it with a `tool_call_update` to a final
 * status: normally `cancelled`, or `completed` or `failed` if the work finished first.
 *
 * The Agent responds with `-32602` if the tool call is unknown, already final, or not
 * cancellable. Cancelling a tool call does not change the foreground state by itself.
 *
 * See protocol docs: [Cancelling Tool Calls](https://agentclientprotocol.com/protocol/v2/tool-calls#cancelling-tool-calls)
 */
struct CancelToolCallRequest {
    SessionId _sessionId{};  //!< The ID of the session that owns the tool call.
    ToolCallId _toolCallId{};  //!< The ID of the tool call to cancel.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    CancelToolCallRequest& sessionId(const SessionId & v) { _sessionId = v; return *this; }
    CancelToolCallRequest& toolCallId(const ToolCallId & v) { _toolCallId = v; return *this; }
    CancelToolCallRequest& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    CancelToolCallRequest& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const SessionId& sessionId() const { return _sessionId; }
    const ToolCallId& toolCallId() const { return _toolCallId; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const CancelToolCallRequest &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<CancelToolCallRequest> fromJson<CancelToolCallRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const CancelToolCallRequest &data);

/**
 * Request parameters for closing an active session.
 *
 * The Agent MUST cancel any ongoing work related to the session as if `session/cancel`
 * was called, then free the resources associated with the session. Unlike `session/cancel`,
 * closing also stops the session's background work. After the close response, the Agent
 * sends no other messages for the session on that connection.
 *
 * Closing a session that does not exist or is not active SHOULD succeed, and Clients SHOULD
 * treat a `-32002` response as a successful close.
 *
 * When closing cancels active foreground work, the Agent MUST send the final
 * updates for that work and the idle `state_update` with `stopReason: "cancelled"`
 * before responding successfully to `session/close`. Closing an idle session
 * does not itself require another idle update.
 *
 * Closing also cancels pending `session/prompt` requests whose messages have not
 * been inserted, including held prompts. If closing prevents a prompt's user message from being inserted, the
 * Agent MUST answer that request with `-32800` (Request Cancelled). If the message
 * has already been inserted, the Agent MUST respond successfully with its
 * `messageId`. Prompt responses need not precede the close response.
 */
struct CloseSessionRequest {
    SessionId _sessionId{};  //!< The ID of the session to close.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    CloseSessionRequest& sessionId(const SessionId & v) { _sessionId = v; return *this; }
    CloseSessionRequest& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    CloseSessionRequest& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const SessionId& sessionId() const { return _sessionId; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const CloseSessionRequest &other) const = default;
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
    SessionId _sessionId{};  //!< The ID of the session to delete.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    DeleteSessionRequest& sessionId(const SessionId & v) { _sessionId = v; return *this; }
    DeleteSessionRequest& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    DeleteSessionRequest& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const SessionId& sessionId() const { return _sessionId; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const DeleteSessionRequest &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<DeleteSessionRequest> fromJson<DeleteSessionRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const DeleteSessionRequest &data);

/**
 * Capabilities for terminal authentication methods.
 *
 * Supplying `{}` means the client can reproduce the configured agent
 * invocation in an interactive terminal and supports terminal authentication
 * methods.
 */
struct TerminalAuthCapabilities {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    TerminalAuthCapabilities& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    TerminalAuthCapabilities& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const TerminalAuthCapabilities &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<TerminalAuthCapabilities> fromJson<TerminalAuthCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const TerminalAuthCapabilities &data);

/**
 * Authentication capabilities supported by the client.
 *
 * Advertised during initialization to inform the agent which authentication
 * method types the client can handle. This governs opt-in types that require
 * additional client-side support.
 */
struct ClientAuthCapabilities {
    /**
     * Whether the client supports `terminal` authentication methods.
     *
     * Optional. Omitted and `null` are equivalent and mean the client does not advertise support.
     * The client should supply `{}` only when it can reproduce the configured
     * agent invocation in an interactive terminal. Supplying `{}` means the
     * agent may include `terminal` entries in its authentication methods.
     */
    Patch<TerminalAuthCapabilities> _terminal{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    ClientAuthCapabilities& terminal(const Patch<TerminalAuthCapabilities> & v) { _terminal = v; return *this; }
    ClientAuthCapabilities& terminal(const TerminalAuthCapabilities & v) { _terminal = v; return *this; }
    ClientAuthCapabilities& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    ClientAuthCapabilities& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<TerminalAuthCapabilities>& terminal() const { return _terminal; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const ClientAuthCapabilities &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<ClientAuthCapabilities> fromJson<ClientAuthCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ClientAuthCapabilities &data);

/**
 * Form-based elicitation capabilities.
 *
 * Supplying `{}` means the client supports form-based elicitation.
 */
struct ElicitationFormCapabilities {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    ElicitationFormCapabilities& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    ElicitationFormCapabilities& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const ElicitationFormCapabilities &other) const = default;
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
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    ElicitationUrlCapabilities& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    ElicitationUrlCapabilities& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const ElicitationUrlCapabilities &other) const = default;
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
    Patch<ElicitationFormCapabilities> _form{};
    /**
     * Whether the client supports URL-based elicitation.
     *
     * Optional. Omitted and `null` are equivalent and mean the client does not advertise support.
     * Supplying `{}` means the client supports URL-based elicitation.
     */
    Patch<ElicitationUrlCapabilities> _url{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    ElicitationCapabilities& form(const Patch<ElicitationFormCapabilities> & v) { _form = v; return *this; }
    ElicitationCapabilities& form(const ElicitationFormCapabilities & v) { _form = v; return *this; }
    ElicitationCapabilities& url(const Patch<ElicitationUrlCapabilities> & v) { _url = v; return *this; }
    ElicitationCapabilities& url(const ElicitationUrlCapabilities & v) { _url = v; return *this; }
    ElicitationCapabilities& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    ElicitationCapabilities& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<ElicitationFormCapabilities>& form() const { return _form; }
    const Patch<ElicitationUrlCapabilities>& url() const { return _url; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const ElicitationCapabilities &other) const = default;
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
     * Authentication capabilities supported by the client.
     * Determines which authentication method types the agent may include
     * in its `InitializeResponse`.
     *
     * Optional. Omitted and `null` are equivalent and mean the client does not advertise any
     * authentication-method extensions.
     */
    Patch<ClientAuthCapabilities> _auth{};
    /**
     * Elicitation capabilities supported by the client.
     * Determines which elicitation modes the agent may use.
     *
     * Optional. Omitted and `null` are equivalent and mean the client does not advertise
     * elicitation support.
     */
    Patch<ElicitationCapabilities> _elicitation{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    ClientCapabilities& auth(const Patch<ClientAuthCapabilities> & v) { _auth = v; return *this; }
    ClientCapabilities& auth(const ClientAuthCapabilities & v) { _auth = v; return *this; }
    ClientCapabilities& elicitation(const Patch<ElicitationCapabilities> & v) { _elicitation = v; return *this; }
    ClientCapabilities& elicitation(const ElicitationCapabilities & v) { _elicitation = v; return *this; }
    ClientCapabilities& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    ClientCapabilities& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<ClientAuthCapabilities>& auth() const { return _auth; }
    const Patch<ElicitationCapabilities>& elicitation() const { return _elicitation; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const ClientCapabilities &other) const = default;
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
    ProtocolVersion _protocolVersion{};  //!< The latest protocol version supported by the client.
    Implementation _info{};  //!< Information about the implementation sending this initialize request.
    std::optional<ClientCapabilities> _capabilities{};  //!< Capabilities supported by the client.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    InitializeRequest& protocolVersion(const ProtocolVersion & v) { _protocolVersion = v; return *this; }
    InitializeRequest& info(const Implementation & v) { _info = v; return *this; }
    InitializeRequest& capabilities(const std::optional<ClientCapabilities> & v) { _capabilities = v; return *this; }
    InitializeRequest& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    InitializeRequest& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const ProtocolVersion& protocolVersion() const { return _protocolVersion; }
    const Implementation& info() const { return _info; }
    const std::optional<ClientCapabilities>& capabilities() const { return _capabilities; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const InitializeRequest &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<InitializeRequest> fromJson<InitializeRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const InitializeRequest &data);

/** Request parameters for listing existing sessions. */
struct ListSessionsRequest {
    /**
     * Filter sessions by working directory. Must be an absolute path.
     *
     * Only sessions whose `cwd` is this same directory match. Subdirectories and
     * `additionalDirectories` don't. Agents MAY normalize paths when comparing.
     *
     * Omitted and `null` are equivalent and mean sessions are not filtered by working directory.
     */
    Patch<AbsolutePath> _cwd{};
    /**
     * Opaque cursor token from a previous response's nextCursor field for cursor-based pagination
     *
     * Omitted and `null` are equivalent and mean the first page is requested.
     */
    Patch<SessionListCursor> _cursor{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    ListSessionsRequest& cwd(const Patch<AbsolutePath> & v) { _cwd = v; return *this; }
    ListSessionsRequest& cwd(const AbsolutePath & v) { _cwd = v; return *this; }
    ListSessionsRequest& cursor(const Patch<SessionListCursor> & v) { _cursor = v; return *this; }
    ListSessionsRequest& cursor(const SessionListCursor & v) { _cursor = v; return *this; }
    ListSessionsRequest& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    ListSessionsRequest& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<AbsolutePath>& cwd() const { return _cwd; }
    const Patch<SessionListCursor>& cursor() const { return _cursor; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const ListSessionsRequest &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<ListSessionsRequest> fromJson<ListSessionsRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ListSessionsRequest &data);

/**
 * Request parameters for the `auth/login` method.
 *
 * Specifies which authentication method to use.
 *
 * Agents MUST support this method when their `initialize` response included
 * a non-empty `authMethods` list. Clients MUST NOT call this method
 * when `authMethods` was omitted or empty.
 */
struct LoginAuthRequest {
    /**
     * The ID of the authentication method to use.
     * Must be one of the methods advertised in the initialize response.
     */
    AuthMethodId _methodId{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    LoginAuthRequest& methodId(const AuthMethodId & v) { _methodId = v; return *this; }
    LoginAuthRequest& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    LoginAuthRequest& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const AuthMethodId& methodId() const { return _methodId; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const LoginAuthRequest &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<LoginAuthRequest> fromJson<LoginAuthRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const LoginAuthRequest &data);

/**
 * Request parameters for the `auth/logout` method.
 *
 * Terminates the current authenticated session.
 *
 * Agents MUST support this method when their `initialize` response included
 * a non-empty `authMethods` list. Clients MUST NOT call this method
 * when `authMethods` was omitted or empty.
 */
struct LogoutAuthRequest {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    LogoutAuthRequest& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    LogoutAuthRequest& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const LogoutAuthRequest &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<LogoutAuthRequest> fromJson<LogoutAuthRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const LogoutAuthRequest &data);

/** An HTTP header to set when making requests to the MCP server. */
struct HttpHeader {
    QString _name{};  //!< The name of the HTTP header.
    QString _value{};  //!< The value to set for the HTTP header.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    HttpHeader& name(const QString & v) { _name = v; return *this; }
    HttpHeader& value(const QString & v) { _value = v; return *this; }
    HttpHeader& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    HttpHeader& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const QString& name() const { return _name; }
    const QString& value() const { return _value; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const HttpHeader &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<HttpHeader> fromJson<HttpHeader>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const HttpHeader &data);

/** HTTP transport configuration for MCP. */
struct McpServerHttp {
    QString _name{};  //!< Human-readable name identifying this MCP server.
    QString _url{};  //!< URL to the MCP server.
    /**
     * HTTP headers to set when making requests to the MCP server. Headers the MCP transport
     * defines are owned by the transport, and Agents use the transport's values for them.
     */
    std::optional<QList<HttpHeader>> _headers{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

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

    bool operator==(const McpServerHttp &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<McpServerHttp> fromJson<McpServerHttp>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const McpServerHttp &data);

/** Stdio transport configuration for MCP. */
struct McpServerStdio {
    QString _name{};  //!< Human-readable name identifying this MCP server.
    AbsolutePath _command{};  //!< Absolute path to the MCP server executable.
    std::optional<QStringList> _args{};  //!< Command-line arguments to pass to the MCP server.
    /**
     * Environment variables to set when launching the MCP server. Names SHOULD be unique.
     * Agents MAY reject duplicates with `-32602`, and otherwise the last entry wins.
     */
    std::optional<QList<EnvVariable>> _env{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

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

    bool operator==(const McpServerStdio &other) const = default;
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
 * How the Agent combines a client-provided system prompt with its own instructions.
 *
 * Unknown modes are preserved so they can be stored,
 * proxied, or forwarded. An Agent MUST return an error for a mode it does not
 * support rather than treating it as `append` or `override`.
 */
enum class SystemPromptMode {
    append,
    override_
};

ACPLIB_EXPORT QString toString(SystemPromptMode v);

template<>
ACPLIB_EXPORT Utils::Result<SystemPromptMode> fromJson<SystemPromptMode>(const QJsonValue &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const SystemPromptMode &v);

/**
 * Client-provided system-level instructions for a session.
 *
 * Clients MUST only send this when the Agent advertises
 * `session.systemPrompt`. Agents that cannot apply it, for example because of
 * an unadvertised mode or an unsupported content block type, MUST return an
 * error instead of creating or resuming the session.
 */
struct SystemPrompt {
    /**
     * How the Agent combines `content` with its own instructions.
     *
     * Required. Agents MUST return an error for a mode they do not support rather than
     * treating it as `append` or `override`.
     */
    SystemPromptMode _mode{};
    /**
     * The system prompt content.
     *
     * Agents advertising the capability MUST accept `text` content blocks.
     * Clients MUST NOT send other content block types until a future
     * capability allows them, and Agents MUST return an error for content
     * blocks they do not support, including unknown types.
     *
     * Clients MUST NOT send an empty array. Agents MAY reject one with
     * `-32602` without creating or resuming the session.
     */
    QList<ContentBlock> _content{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    SystemPrompt& mode(const SystemPromptMode & v) { _mode = v; return *this; }
    SystemPrompt& content(const QList<ContentBlock> & v) { _content = v; return *this; }
    SystemPrompt& addContent(const ContentBlock & v) { _content.append(v); return *this; }
    SystemPrompt& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    SystemPrompt& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const SystemPromptMode& mode() const { return _mode; }
    const QList<ContentBlock>& content() const { return _content; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const SystemPrompt &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<SystemPrompt> fromJson<SystemPrompt>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SystemPrompt &data);

/**
 * Request parameters for creating a new session.
 *
 * See protocol docs: [Creating a Session](https://agentclientprotocol.com/protocol/v2/session-setup#creating-a-session)
 */
struct NewSessionRequest {
    AbsolutePath _cwd{};  //!< The working directory for this session. Must be an absolute path.
    /**
     * Additional workspace roots for this session. Each path must be absolute.
     *
     * These expand the session's workspace scope without changing `cwd`, which
     * remains the base for relative paths. When omitted or empty, no
     * additional roots are activated for the new session.
     */
    std::optional<QList<AbsolutePath>> _additionalDirectories{};
    std::optional<QList<McpServer>> _mcpServers{};  //!< List of MCP (Model Context Protocol) servers the agent should connect to.
    /**
     * System-level instructions for the new session.
     *
     * Optional. Omitted and `null` are equivalent and mean the Agent uses its
     * own default instructions. Clients MUST only send this when the Agent
     * advertises `session.systemPrompt`.
     */
    Patch<SystemPrompt> _systemPrompt{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    NewSessionRequest& cwd(const AbsolutePath & v) { _cwd = v; return *this; }
    NewSessionRequest& additionalDirectories(const std::optional<QList<AbsolutePath>> & v) { _additionalDirectories = v; return *this; }
    NewSessionRequest& addAdditionalDirectory(const AbsolutePath & v) { if (!_additionalDirectories) _additionalDirectories = QList<AbsolutePath>{}; (*_additionalDirectories).append(v); return *this; }
    NewSessionRequest& mcpServers(const std::optional<QList<McpServer>> & v) { _mcpServers = v; return *this; }
    NewSessionRequest& addMcpServer(const McpServer & v) { if (!_mcpServers) _mcpServers = QList<McpServer>{}; (*_mcpServers).append(v); return *this; }
    NewSessionRequest& systemPrompt(const Patch<SystemPrompt> & v) { _systemPrompt = v; return *this; }
    NewSessionRequest& systemPrompt(const SystemPrompt & v) { _systemPrompt = v; return *this; }
    NewSessionRequest& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    NewSessionRequest& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const AbsolutePath& cwd() const { return _cwd; }
    const std::optional<QList<AbsolutePath>>& additionalDirectories() const { return _additionalDirectories; }
    const std::optional<QList<McpServer>>& mcpServers() const { return _mcpServers; }
    const Patch<SystemPrompt>& systemPrompt() const { return _systemPrompt; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const NewSessionRequest &other) const = default;
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
    SessionId _sessionId{};  //!< The ID of the session to send this user message to
    /**
     * The blocks of content that compose the user's message.
     *
     * As a baseline, the Agent MUST support `text` and `resource_link` content blocks,
     * while other variants are optionally enabled via `PromptCapabilities`.
     *
     * The Client MUST adapt its interface according to `PromptCapabilities`.
     *
     * The client MAY include referenced pieces of context as either
     * `resource` or `resource_link` content blocks.
     *
     * When available, `resource` is preferred
     * as it avoids extra round-trips and allows the message to include
     * pieces of context from sources the agent may not have access to.
     *
     * Clients MUST NOT send an empty array. Agents MAY reject one with `-32602` without
     * inserting it.
     */
    QList<ContentBlock> _prompt{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    PromptRequest& sessionId(const SessionId & v) { _sessionId = v; return *this; }
    PromptRequest& prompt(const QList<ContentBlock> & v) { _prompt = v; return *this; }
    PromptRequest& addPrompt(const ContentBlock & v) { _prompt.append(v); return *this; }
    PromptRequest& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    PromptRequest& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const SessionId& sessionId() const { return _sessionId; }
    const QList<ContentBlock>& prompt() const { return _prompt; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const PromptRequest &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<PromptRequest> fromJson<PromptRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const PromptRequest &data);

/** Inclusive replay cursor requesting replay from the start of retained conversation history. */
struct ReplayFromStart {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    ReplayFromStart& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    ReplayFromStart& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const ReplayFromStart &other) const = default;
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

/** Returns the 'type' dispatch field value for the active variant. */
ACPLIB_EXPORT QString dispatchValue(const ReplayFrom &val);

ACPLIB_EXPORT QJsonObject toJson(const ReplayFrom &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const ReplayFrom &val);

/**
 * Request parameters for resuming an existing session.
 *
 * Resumes an existing session and optionally replays retained conversation
 * history according to `replayFrom`.
 */
struct ResumeSessionRequest {
    SessionId _sessionId{};  //!< The ID of the session to resume.
    /**
     * The working directory for this session. Must be an absolute path.
     *
     * Clients MUST send the session's working directory. Agents MAY reject a different directory
     * with `-32602`, and MAY normalize paths when comparing.
     */
    AbsolutePath _cwd{};
    /**
     * Additional workspace roots to activate for this session. Each path must be absolute.
     *
     * When omitted or empty, no additional roots are activated. When non-empty,
     * this is the complete resulting additional-root list for the resumed
     * session. It may differ from any previously used or reported list as long as
     * the request `cwd` matches the session's `cwd`.
     */
    std::optional<QList<AbsolutePath>> _additionalDirectories{};
    /**
     * List of MCP servers to connect to for this session.
     *
     * The complete set of Client-provided servers. Omitted and `[]` mean none. Agents SHOULD
     * NOT restore servers from earlier requests.
     */
    std::optional<QList<McpServer>> _mcpServers{};
    /**
     * Inclusive cursor describing where conversation replay should begin.
     *
     * Optional. Omitted and `null` are equivalent and mean the Agent should resume without
     * replaying previous conversation history. Replay cursors are inclusive:
     * replay includes the position identified by the cursor. Supplying
     * `{ "type": "start" }` means the Agent should replay all retained
     * conversation history before responding.
     *
     * Agents MUST reject unsupported cursor types or malformed cursor values
     * with `-32602` (Invalid params), rather than substituting another cursor
     * or resuming without replay.
     */
    Patch<ReplayFrom> _replayFrom{};
    /**
     * System-level instructions for the resumed session.
     *
     * Optional. Omitted and `null` are equivalent. The Agent may then keep a
     * system prompt it stored with the session or use its default instructions.
     * Clients MUST only send this when the Agent advertises
     * `session.systemPrompt`, and SHOULD send the same value they used when the
     * session was created.
     */
    Patch<SystemPrompt> _systemPrompt{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    ResumeSessionRequest& sessionId(const SessionId & v) { _sessionId = v; return *this; }
    ResumeSessionRequest& cwd(const AbsolutePath & v) { _cwd = v; return *this; }
    ResumeSessionRequest& additionalDirectories(const std::optional<QList<AbsolutePath>> & v) { _additionalDirectories = v; return *this; }
    ResumeSessionRequest& addAdditionalDirectory(const AbsolutePath & v) { if (!_additionalDirectories) _additionalDirectories = QList<AbsolutePath>{}; (*_additionalDirectories).append(v); return *this; }
    ResumeSessionRequest& mcpServers(const std::optional<QList<McpServer>> & v) { _mcpServers = v; return *this; }
    ResumeSessionRequest& addMcpServer(const McpServer & v) { if (!_mcpServers) _mcpServers = QList<McpServer>{}; (*_mcpServers).append(v); return *this; }
    ResumeSessionRequest& replayFrom(const Patch<ReplayFrom> & v) { _replayFrom = v; return *this; }
    ResumeSessionRequest& replayFrom(const ReplayFrom & v) { _replayFrom = v; return *this; }
    ResumeSessionRequest& systemPrompt(const Patch<SystemPrompt> & v) { _systemPrompt = v; return *this; }
    ResumeSessionRequest& systemPrompt(const SystemPrompt & v) { _systemPrompt = v; return *this; }
    ResumeSessionRequest& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    ResumeSessionRequest& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const SessionId& sessionId() const { return _sessionId; }
    const AbsolutePath& cwd() const { return _cwd; }
    const std::optional<QList<AbsolutePath>>& additionalDirectories() const { return _additionalDirectories; }
    const std::optional<QList<McpServer>>& mcpServers() const { return _mcpServers; }
    const Patch<ReplayFrom>& replayFrom() const { return _replayFrom; }
    const Patch<SystemPrompt>& systemPrompt() const { return _systemPrompt; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const ResumeSessionRequest &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<ResumeSessionRequest> fromJson<ResumeSessionRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ResumeSessionRequest &data);

/**
 * Request parameters for setting a session configuration option.
 *
 * Agents MUST support this method for any session where they advertise config options.
 * Agents MUST reject an unknown `configId` or invalid value with `-32602` and leave the
 * configuration unchanged.
 *
 * See protocol docs: [Session Config Options](https://agentclientprotocol.com/protocol/v2/session-config-options#setting-a-config-option)
 */
struct SetSessionConfigOptionRequest {
    SessionId _sessionId{};  //!< The ID of the session to set the configuration option for.
    SessionConfigId _configId{};  //!< The ID of the configuration option to set.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};
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

    bool operator==(const SetSessionConfigOptionRequest &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<SetSessionConfigOptionRequest> fromJson<SetSessionConfigOptionRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SetSessionConfigOptionRequest &data);

/** A JSON-RPC request object. */
struct ClientRequest {
    RequestId _id{};  //!< The request id used to correlate the matching response.
    QString _method{};  //!< The method name to invoke.
    std::optional<QString> _params{};  //!< Method-specific request parameters.

    ClientRequest& id(const RequestId & v) { _id = v; return *this; }
    ClientRequest& method(const QString & v) { _method = v; return *this; }
    ClientRequest& params(const std::optional<QString> & v) { _params = v; return *this; }

    const RequestId& id() const { return _id; }
    const QString& method() const { return _method; }
    const std::optional<QString>& params() const { return _params; }

    bool operator==(const ClientRequest &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<ClientRequest> fromJson<ClientRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ClientRequest &data);

/** Allowed wire representations for `ElicitationContentValue`. */
using ElicitationContentValue = std::variant<QString, int, double, bool, QList<QString>>;

template<>
ACPLIB_EXPORT Utils::Result<ElicitationContentValue> fromJson<ElicitationContentValue>(const QJsonValue &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const ElicitationContentValue &val);

/** The user accepted the elicitation and provided content. */
struct ElicitationAcceptAction {
    /**
     * The user-provided content, if any, as an object matching the requested schema.
     *
     * Omitted and `null` are equivalent and mean no content is provided.
     */
    Patch<QJsonObject> _content{};

    ElicitationAcceptAction& content(const Patch<QJsonObject> & v) { _content = v; return *this; }
    ElicitationAcceptAction& content(const QJsonObject & v) { _content = v; return *this; }

    const Patch<QJsonObject>& content() const { return _content; }

    bool operator==(const ElicitationAcceptAction &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<ElicitationAcceptAction> fromJson<ElicitationAcceptAction>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ElicitationAcceptAction &data);

/** Response from the client to an elicitation request. */
struct CreateElicitationResponse {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};
    QJsonObject _additionalProperties;  //!< additional properties

    CreateElicitationResponse& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    CreateElicitationResponse& _meta(const QJsonObject & v) { __meta = v; return *this; }
    CreateElicitationResponse& additionalProperties(const QString &key, const QJsonValue &v) { _additionalProperties.insert(key, v); return *this; }
    CreateElicitationResponse& additionalProperties(const QJsonObject &obj) { for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) _additionalProperties.insert(it.key(), it.value()); return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }
    const QJsonObject& additionalProperties() const { return _additionalProperties; }

    bool operator==(const CreateElicitationResponse &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<CreateElicitationResponse> fromJson<CreateElicitationResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const CreateElicitationResponse &data);

/** The request was not approved and no option was selected. */
struct CancelledPermissionOutcome {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    CancelledPermissionOutcome& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    CancelledPermissionOutcome& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const CancelledPermissionOutcome &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<CancelledPermissionOutcome> fromJson<CancelledPermissionOutcome>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const CancelledPermissionOutcome &data);

/** The user selected one of the provided options. */
struct SelectedPermissionOutcome {
    PermissionOptionId _optionId{};  //!< The ID of the option the user selected.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    SelectedPermissionOutcome& optionId(const PermissionOptionId & v) { _optionId = v; return *this; }
    SelectedPermissionOutcome& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    SelectedPermissionOutcome& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const PermissionOptionId& optionId() const { return _optionId; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const SelectedPermissionOutcome &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<SelectedPermissionOutcome> fromJson<SelectedPermissionOutcome>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SelectedPermissionOutcome &data);

/** The outcome of a permission request. */
using RequestPermissionOutcome = std::variant<CancelledPermissionOutcome, SelectedPermissionOutcome, QJsonObject>;

template<>
ACPLIB_EXPORT Utils::Result<RequestPermissionOutcome> fromJson<RequestPermissionOutcome>(const QJsonValue &val);

/** Returns the 'outcome' dispatch field value for the active variant. */
ACPLIB_EXPORT QString dispatchValue(const RequestPermissionOutcome &val);

ACPLIB_EXPORT QJsonObject toJson(const RequestPermissionOutcome &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const RequestPermissionOutcome &val);

/** Response to a permission request. */
struct RequestPermissionResponse {
    RequestPermissionOutcome _outcome{};  //!< The user's decision on the permission request.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    RequestPermissionResponse& outcome(const RequestPermissionOutcome & v) { _outcome = v; return *this; }
    RequestPermissionResponse& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    RequestPermissionResponse& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const RequestPermissionOutcome& outcome() const { return _outcome; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const RequestPermissionResponse &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<RequestPermissionResponse> fromJson<RequestPermissionResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const RequestPermissionResponse &data);

/** A JSON-RPC response object. */
using ClientResponse = std::variant<QJsonObject>;

/**
 * Notification to cancel a session's foreground work and its prompts not yet inserted.
 *
 * See protocol docs: [Cancellation](https://agentclientprotocol.com/protocol/v2/prompt-lifecycle#cancellation)
 */
struct CancelSessionNotification {
    SessionId _sessionId{};  //!< The ID of the session whose foreground work to cancel.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    CancelSessionNotification& sessionId(const SessionId & v) { _sessionId = v; return *this; }
    CancelSessionNotification& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    CancelSessionNotification& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const SessionId& sessionId() const { return _sessionId; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const CancelSessionNotification &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<CancelSessionNotification> fromJson<CancelSessionNotification>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const CancelSessionNotification &data);

/** A JSON-RPC notification object. */
struct ClientNotification {
    QString _method{};  //!< The notification method name.
    std::optional<QString> _params{};  //!< Method-specific notification parameters.

    ClientNotification& method(const QString & v) { _method = v; return *this; }
    ClientNotification& params(const std::optional<QString> & v) { _params = v; return *this; }

    const QString& method() const { return _method; }
    const std::optional<QString>& params() const { return _params; }

    bool operator==(const ClientNotification &other) const = default;
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
    /**
     * The ID of the request to cancel: a request that the sender of this notification issued.
     * Receivers match it only against requests they received from that sender.
     */
    RequestId _requestId{};
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Omitted and `null` are equivalent and mean no metadata.
     * Implementations MUST NOT assign meaning to keys they do not recognize.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/v2/extensibility)
     */
    Patch<QJsonObject> __meta{};

    CancelRequestNotification& requestId(const RequestId & v) { _requestId = v; return *this; }
    CancelRequestNotification& _meta(const Patch<QJsonObject> & v) { __meta = v; return *this; }
    CancelRequestNotification& _meta(const QJsonObject & v) { __meta = v; return *this; }

    const RequestId& requestId() const { return _requestId; }
    const Patch<QJsonObject>& _meta() const { return __meta; }

    bool operator==(const CancelRequestNotification &other) const = default;
};

template<>
ACPLIB_EXPORT Utils::Result<CancelRequestNotification> fromJson<CancelRequestNotification>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const CancelRequestNotification &data);

} // namespace Acp::V2
