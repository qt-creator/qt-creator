/*
 * Copyright (C) 2022-current by Axivion GmbH
 * https://www.axivion.com/
 *
 * SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
 *
 * Purpose: Dashboard C++ API Implementation
 *
 * !!!!!! GENERATED, DO NOT EDIT !!!!!!
 *
 * This file was generated with the script at
 * <AxivionSuiteRepo>/projects/libs/dashboard_cpp_api/generator/generate_dashboard_cpp_api.py
 */

#undef QT_RESTRICTED_CAST_FROM_ASCII
#define QT_NO_CAST_FROM_ASCII 1
#define QT_NO_CAST_TO_ASCII 1
#define QT_NO_CAST_FROM_BYTEARRAY 1

#include "dashboard/dto.h"

#include "dashboard/concat.h"

#include <QJsonValue>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <string>
#include <typeinfo>
#include <utility>

namespace Axivion::Internal::Dto {

    template<typename T>
    static std::string to_std_string(const T &e)
    {
        return std::to_string(e);
    }

    template<>
    std::string to_std_string(const QString &qstr)
    {
        return qstr.toStdString();
    }

    template<>
    std::string to_std_string(const QAnyStringView &qasv)
    {
        return to_std_string(qasv.toString());
    }

    // exceptions

    invalid_dto_exception::invalid_dto_exception(const std::string_view type_name, const std::exception &ex) :
        std::runtime_error(concat({ type_name, u8": ",  ex.what() }))
    {}

    invalid_dto_exception::invalid_dto_exception(const std::string_view type_name, const std::string_view message) :
        std::runtime_error(concat({ type_name, u8": ", message }))
    {}

    // throws Axivion::Internal::Dto::invalid_dto_exception
    template<typename T>
    [[noreturn]] static void throw_invalid_dto_exception(const std::exception &ex)
    {
        throw invalid_dto_exception(typeid(T).name(), ex);
    }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    template<typename T>
    [[noreturn]] static void throw_invalid_dto_exception(std::string_view message)
    {
        throw invalid_dto_exception(typeid(T).name(), message);
    }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    template<typename T>
    [[noreturn]] static void throw_json_type_conversion(QJsonValue::Type type) {
        throw_invalid_dto_exception<T>(concat({
            u8"Error parsing JSON: Cannot convert type ",
            to_std_string(type)
            }));
    }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    template<typename T, typename V>
    [[noreturn]] static void throw_json_value_conversion(const V &raw_value) {
        throw_invalid_dto_exception<T>(concat({
            u8"Error parsing JSON: Cannot convert raw value ",
            to_std_string(raw_value)
            }));
    }

    // basic json (de)serialization

    // throws Axivion::Internal::Dto::invalid_dto_exception
    template<typename T>
    static QJsonObject toJsonObject(const QJsonValue &json)
    {
        if (json.isObject())
        {
            return json.toObject();
        }
        throw_json_type_conversion<std::map<QString, T>>(json.type());
    }

    template<typename T>
    class de_serializer final
    {
    public:
        // Require usage of template specializations.
        // This static members have to be implemented:
        //
        // // throws Axivion::Internal::Dto::invalid_dto_exception
        // static T deserialize(const QJsonValue &json);
        //
        // static QJsonValue serialize(const T &value);

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    template<typename T>
    static T deserialize_json(const QJsonValue &json)
    {
        return de_serializer<T>::deserialize(json);
    }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    template<typename T>
    static T deserialize_bytes(const QByteArray &json)
    {
        QJsonValue qjv;
        {
            QJsonParseError error;
            const QJsonDocument qjd = QJsonDocument::fromJson(json, &error);
            if (error.error != QJsonParseError::ParseError::NoError)
            {
                throw_invalid_dto_exception<T>(concat({
                    u8"Error parsing JSON - ",
                    to_std_string(error.error),
                    u8" at ",
                    to_std_string(error.offset),
                    u8": ",
                    to_std_string(error.errorString())
                    }));
            }
            if (!qjd.isObject())
            {
                throw_invalid_dto_exception<T>(u8"Error parsing JSON: parsed data is no JSON object");
            }
            qjv = QJsonValue(qjd.object());
        }
        return deserialize_json<T>(qjv);
    }

    template<typename T>
    static QJsonValue serialize_json(const T &value)
    {
        return de_serializer<T>::serialize(value);
    }

    template<typename T>
    static QByteArray serialize_bytes(const T &value)
    {
        QJsonDocument qjd;
        {
            QJsonValue qjv = serialize_json(value);
            if (qjv.isObject())
            {
                qjd = QJsonDocument(qjv.toObject());
            }
            else if (qjv.isArray())
            {
                qjd = QJsonDocument(qjv.toArray());
            }
            else
            {
                throw std::domain_error(concat({
                    u8"Error serializing JSON - value is not an object or array:",
                    to_std_string(qjv.type())
                    }));
            }
        }
        return qjd.toJson(QJsonDocument::JsonFormat::Indented);
    }

    template<>
    class de_serializer<std::nullptr_t> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static std::nullptr_t deserialize(const QJsonValue &json)
        {
            if (json.isNull())
            {
                return nullptr;
            }
            throw_json_type_conversion<std::nullptr_t>(json.type());
        }

        static QJsonValue serialize(const std::nullptr_t&)
        {
            return {};
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    template<>
    class de_serializer<QString> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static QString deserialize(const QJsonValue &json)
        {
            if (json.isString())
            {
                return json.toString();
            }
            throw_json_type_conversion<QString>(json.type());
        }

        static QJsonValue serialize(const QString &value)
        {
            return { value };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    template<>
    class de_serializer<bool> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static bool deserialize(const QJsonValue &json)
        {
            if (json.isBool())
            {
                return json.toBool();
            }
            throw_json_type_conversion<bool>(json.type());
        }

        static QJsonValue serialize(const bool &value)
        {
            return { value };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    template<>
    class de_serializer<qint32> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static qint32 deserialize(const QJsonValue &json)
        {
            if (!json.isDouble())
            {
                throw_json_type_conversion<qint32>(json.type());
            }
            const double rawValue = json.toDouble();
            const qint32 value = static_cast<qint32>(rawValue);
            if (static_cast<double>(value) != rawValue)
            {
                throw_json_value_conversion<qint32>(rawValue);
            }
            return value;
        }

        static QJsonValue serialize(const qint32 &value)
        {
            return { static_cast<qint64>(value) };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    template<>
    class de_serializer<qint64> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static qint64 deserialize(const QJsonValue &json)
        {
            if (!json.isDouble())
            {
                throw_json_type_conversion<qint64>(json.type());
            }
            const double rawValue = json.toDouble();
            const qint64 value = static_cast<qint64>(rawValue);
            if (static_cast<double>(value) != rawValue)
            {
                throw_json_value_conversion<qint64>(rawValue);
            }
            return value;
        }

        static QJsonValue serialize(const qint64 &value)
        {
            return { value };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    static const QLatin1String deSerializerDoublePositiveInfinity{"Infinity"};
    static const QLatin1String deSerializerDoubleNegativeInfinity{"-Infinity"};
    static const QLatin1String deSerializerDoubleNAN{"NaN"};

    template<>
    class de_serializer<double> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static double deserialize(const QJsonValue &json)
        {
            if (json.isDouble())
            {
                return json.toDouble();
            }
            if (json.isString())
            {
                const QString rawValue = json.toString();
                if (rawValue == deSerializerDoublePositiveInfinity)
                {
                    return std::numeric_limits<double>::infinity();
                }
                if (rawValue == deSerializerDoubleNegativeInfinity)
                {
                    return -std::numeric_limits<double>::infinity();
                }
                if (rawValue == deSerializerDoubleNAN)
                {
                    return std::numeric_limits<double>::quiet_NaN();
                }
                throw_json_value_conversion<double>(rawValue);
            }
            throw_json_type_conversion<double>(json.type());
        }

        static QJsonValue serialize(const double &value)
        {
            if (value == std::numeric_limits<double>::infinity())
            {
                return { deSerializerDoublePositiveInfinity };
            }
            if (value == -std::numeric_limits<double>::infinity())
            {
                return { deSerializerDoubleNegativeInfinity };
            }
            if (std::isnan(value))
            {
                return { deSerializerDoubleNAN };
            }
            return { value };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    template<typename T>
    class de_serializer<std::optional<T>> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static std::optional<T> deserialize(const QJsonValue &json)
        {
            if (json.isNull())
            {
                return std::nullopt;
            }
            return deserialize_json<T>(json);
        }

        static QJsonValue serialize(const std::optional<T> &value)
        {
            if (value.has_value()) {
                return serialize_json(*value);
            }
            return serialize_json(nullptr);
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    template<typename T>
    class de_serializer<std::vector<T>> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static std::vector<T> deserialize(const QJsonValue &json)
        {
            if (!json.isArray())
            {
                throw_json_type_conversion<std::vector<T>>(json.type());
            }
            const QJsonArray ja = json.toArray();
            std::vector<T> value;
            value.reserve(ja.size());
            for (const auto item : ja)
            {
                value.push_back(deserialize_json<T>(item));
            }
            return value;
        }

        static QJsonValue serialize(const std::vector<T> &value)
        {
            QJsonArray ja;
            for (const T &e : value)
            {
                ja.push_back(serialize_json(e));
            }
            return { ja };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    template<typename T>
    class de_serializer<std::unordered_set<T>> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static std::unordered_set<T> deserialize(const QJsonValue &json)
        {
            if (!json.isArray())
            {
                throw_json_type_conversion<std::unordered_set<T>>(json.type());
            }
            const QJsonArray ja = json.toArray();
            std::unordered_set<T> value;
            value.reserve(ja.size());
            for (const auto item : ja)
            {
                value.insert(deserialize_json<T>(item));
            }
            return value;
        }

        static QJsonValue serialize(const std::unordered_set<T> &value)
        {
            QJsonArray ja;
            for (const T &e : value)
            {
                ja.push_back(serialize_json(e));
            }
            return { ja };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    template<typename T>
    class de_serializer<std::map<QString, T>> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static std::map<QString, T> deserialize(const QJsonValue &json)
        {
            const QJsonObject jo = toJsonObject<std::map<QString, T>>(json);
            std::map<QString, T> value;
            // value.reserve(jo.size());
            for (auto it = jo.constBegin(), end = jo.constEnd(); it != end; ++it)
            {
                value[it.key()] = deserialize_json<T>(it.value());
            }
            return value;
        }

        static QJsonValue serialize(const std::map<QString, T> &value)
        {
            QJsonObject jo;
            for (const auto &[key, val] : value)
            {
                jo.insert(key, serialize_json(val));
            }
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // field (de)serialization

    template<typename T>
    class field_de_serializer final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static T deserialize(const QJsonObject &jo, const QString &key)
        {
            const auto it = jo.constFind(key);
            if (it == jo.constEnd())
            {
                throw_invalid_dto_exception<T>(concat({
                    u8"Error parsing JSON: key not found ",
                    to_std_string(key)
                    }));
            }
            return deserialize_json<T>(it.value());
        }

        static void serialize(QJsonObject &json, const QString &key, const T &value)
        {
            json.insert(key, serialize_json(value));
        }

        field_de_serializer() = delete;
        ~field_de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    template<typename T>
    static T deserialize_field(const QJsonObject &jo, const QString &key)
    {
        return field_de_serializer<T>::deserialize(jo, key);
    }

    template<typename T>
    static void serialize_field(QJsonObject &jo, const QString &key, const T &value)
    {
        field_de_serializer<T>::serialize(jo, key, value);
    }

    template<typename T>
    class field_de_serializer<std::optional<T>> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static std::optional<T> deserialize(const QJsonObject &jo, const QString &key)
        {
            const auto it = jo.constFind(key);
            if (it == jo.constEnd())
            {
                return std::nullopt;
            }
            const auto value = it.value();
            if (value.isNull())
            {
                return std::nullopt;
            }
            return deserialize_json<T>(value);
        }

        static void serialize(QJsonObject &json, const QString &key, const std::optional<T> &value)
        {
            if (value.has_value())
            {
                serialize_field(json, key, *value);
            }
        }

        field_de_serializer() = delete;
        ~field_de_serializer() = delete;
    };

    // any

    template<>
    class de_serializer<Any> final
    {
    public:
        static Any deserialize(const QJsonValue &json)
        {
            if (json.isNull())
            {
                return Any();
            }
            if (json.isString())
            {
                return Any(deserialize_json<QString>(json));
            }
            if (json.isDouble())
            {
                return Any(deserialize_json<double>(json));
            }
            if (json.isObject())
            {
                return Any(deserialize_json<Any::Map>(json));
            }
            if (json.isArray())
            {
                return Any(deserialize_json<Any::Vector>(json));
            }
            if (json.isBool())
            {
                return Any(deserialize_json<bool>(json));
            }
            throw std::domain_error(concat({
                u8"Unknown json value type: ",
                to_std_string(json.type())
                }));
        }

        static QJsonValue serialize(const Any &value) {
            if (value.isNull())
            {
                return serialize_json(nullptr);
            }
            if (value.isString())
            {
                return serialize_json(value.getString());
            }
            if (value.isDouble())
            {
                return serialize_json(value.getDouble());
            }
            if (value.isMap())
            {
                return serialize_json(value.getMap());
            }
            if (value.isList())
            {
                return serialize_json(value.getList());
            }
            if (value.isBool())
            {
                return serialize_json(value.getBool());
            }
            throw std::domain_error(u8"Unknown Axivion::Internal::Dto::any variant");
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    Any Any::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<Any>(json);
    }

    Any::Any() {}

    Any::Any(QString value) : data(std::move(value)) {}

    Any::Any(double value) : data(std::move(value)) {}

    Any::Any(Map value) : data(std::move(value)) {}

    Any::Any(Vector value) : data(std::move(value)) {}

    Any::Any(bool value) : data(std::move(value)) {}

    bool Any::isNull() const
    {
        return this->data.index() == 0;
    }

    bool Any::isString() const
    {
        return this->data.index() == 1;
    }

    QString &Any::getString()
    {
        return std::get<1>(this->data);
    }

    const QString &Any::getString() const {
        return std::get<1>(this->data);
    }

    bool Any::isDouble() const
    {
        return this->data.index() == 2;
    }

    double &Any::getDouble()
    {
        return std::get<2>(this->data);
    }

    const double &Any::getDouble() const
    {
        return std::get<2>(this->data);
    }

    bool Any::isMap() const
    {
        return this->data.index() == 3;
    }

    Any::Map &Any::getMap()
    {
        return std::get<3>(this->data);
    }

    const Any::Map &Any::getMap() const
    {
        return std::get<3>(this->data);
    }

    bool Any::isList() const
    {
        return this->data.index() == 4;
    }

    Any::Vector &Any::getList()
    {
        return std::get<4>(this->data);
    }

    const Any::Vector &Any::getList() const
    {
        return std::get<4>(this->data);
    }

    bool Any::isBool() const
    {
        return this->data.index() == 5;
    }

    bool &Any::getBool()
    {
        return std::get<5>(this->data);
    }

    const bool &Any::getBool() const
    {
        return std::get<5>(this->data);
    }

    QByteArray Any::serialize() const
    {
        return serialize_bytes(*this);
    }

    // version

    constexpr std::array<qint32, 4> ApiVersion::number{7,6,3,12797};
    const QLatin1String ApiVersion::string{"7.6.3.12797"};
    const QLatin1String ApiVersion::name{"7.6.3"};
    const QLatin1String ApiVersion::timestamp{"2023-08-30 15:49:00 +00:00"};

    // AnalyzedFileDto

    static const QLatin1String analyzedFileKeyPath{"path"};
    static const QLatin1String analyzedFileKeyIsSystemHeader{"isSystemHeader"};
    static const QLatin1String analyzedFileKeyLanguageName{"languageName"};

    template<>
    class de_serializer<AnalyzedFileDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static AnalyzedFileDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<AnalyzedFileDto>(json);
            return {
                deserialize_field<QString>(jo, analyzedFileKeyPath),
                deserialize_field<std::optional<bool>>(jo, analyzedFileKeyIsSystemHeader),
                deserialize_field<std::optional<QString>>(jo, analyzedFileKeyLanguageName)
            };
        }

        static QJsonValue serialize(const AnalyzedFileDto &value) {
            QJsonObject jo;
            serialize_field(jo, analyzedFileKeyPath, value.path);
            serialize_field(jo, analyzedFileKeyIsSystemHeader, value.isSystemHeader);
            serialize_field(jo, analyzedFileKeyLanguageName, value.languageName);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    AnalyzedFileDto AnalyzedFileDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<AnalyzedFileDto>(json);
    }

    AnalyzedFileDto::AnalyzedFileDto(
        QString path,
        std::optional<bool> isSystemHeader,
        std::optional<QString> languageName
    ) :
        path(std::move(path)),
        isSystemHeader(std::move(isSystemHeader)),
        languageName(std::move(languageName))
    { }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray AnalyzedFileDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // ApiTokenType

    const QLatin1String ApiTokenTypeMeta::sourcefetch{"SourceFetch"};
    const QLatin1String ApiTokenTypeMeta::general{"General"};
    const QLatin1String ApiTokenTypeMeta::ideplugin{"IdePlugin"};
    const QLatin1String ApiTokenTypeMeta::login{"LogIn"};
    const QLatin1String ApiTokenTypeMeta::continuousintegration{"ContinuousIntegration"};

    // throws std::range_error
    ApiTokenType ApiTokenTypeMeta::strToEnum(QAnyStringView str)
    {
        if (str == ApiTokenTypeMeta::sourcefetch)
        {
            return ApiTokenType::sourcefetch;
        }
        if (str == ApiTokenTypeMeta::general)
        {
            return ApiTokenType::general;
        }
        if (str == ApiTokenTypeMeta::ideplugin)
        {
            return ApiTokenType::ideplugin;
        }
        if (str == ApiTokenTypeMeta::login)
        {
            return ApiTokenType::login;
        }
        if (str == ApiTokenTypeMeta::continuousintegration)
        {
            return ApiTokenType::continuousintegration;
        }
        throw std::range_error(concat({ u8"Unknown ApiTokenType str: ", to_std_string(str) }));
    }

    QLatin1String ApiTokenTypeMeta::enumToStr(ApiTokenType e)
    {
        switch (e)
        {
        case ApiTokenType::sourcefetch:
            return ApiTokenTypeMeta::sourcefetch;
        case ApiTokenType::general:
            return ApiTokenTypeMeta::general;
        case ApiTokenType::ideplugin:
            return ApiTokenTypeMeta::ideplugin;
        case ApiTokenType::login:
            return ApiTokenTypeMeta::login;
        case ApiTokenType::continuousintegration:
            return ApiTokenTypeMeta::continuousintegration;;
        default:
            throw std::domain_error(concat({
                u8"Unknown ApiTokenType enum: ",
                to_std_string(static_cast<int>(e))
                }));
        }
    }

    // ChangePasswordFormDto

    static const QLatin1String changePasswordFormKeyCurrentPassword{"currentPassword"};
    static const QLatin1String changePasswordFormKeyNewPassword{"newPassword"};

    template<>
    class de_serializer<ChangePasswordFormDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static ChangePasswordFormDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<ChangePasswordFormDto>(json);
            return {
                deserialize_field<QString>(jo, changePasswordFormKeyCurrentPassword),
                deserialize_field<QString>(jo, changePasswordFormKeyNewPassword)
            };
        }

        static QJsonValue serialize(const ChangePasswordFormDto &value) {
            QJsonObject jo;
            serialize_field(jo, changePasswordFormKeyCurrentPassword, value.currentPassword);
            serialize_field(jo, changePasswordFormKeyNewPassword, value.newPassword);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    ChangePasswordFormDto ChangePasswordFormDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<ChangePasswordFormDto>(json);
    }

    ChangePasswordFormDto::ChangePasswordFormDto(
        QString currentPassword,
        QString newPassword
    ) :
        currentPassword(std::move(currentPassword)),
        newPassword(std::move(newPassword))
    { }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray ChangePasswordFormDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // ColumnTypeOptionDto

    static const QLatin1String columnTypeOptionKeyKey{"key"};
    static const QLatin1String columnTypeOptionKeyDisplayName{"displayName"};
    static const QLatin1String columnTypeOptionKeyDisplayColor{"displayColor"};

    template<>
    class de_serializer<ColumnTypeOptionDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static ColumnTypeOptionDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<ColumnTypeOptionDto>(json);
            return {
                deserialize_field<QString>(jo, columnTypeOptionKeyKey),
                deserialize_field<std::optional<QString>>(jo, columnTypeOptionKeyDisplayName),
                deserialize_field<QString>(jo, columnTypeOptionKeyDisplayColor)
            };
        }

        static QJsonValue serialize(const ColumnTypeOptionDto &value) {
            QJsonObject jo;
            serialize_field(jo, columnTypeOptionKeyKey, value.key);
            serialize_field(jo, columnTypeOptionKeyDisplayName, value.displayName);
            serialize_field(jo, columnTypeOptionKeyDisplayColor, value.displayColor);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    ColumnTypeOptionDto ColumnTypeOptionDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<ColumnTypeOptionDto>(json);
    }

    ColumnTypeOptionDto::ColumnTypeOptionDto(
        QString key,
        std::optional<QString> displayName,
        QString displayColor
    ) :
        key(std::move(key)),
        displayName(std::move(displayName)),
        displayColor(std::move(displayColor))
    { }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray ColumnTypeOptionDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // CommentRequestDto

    static const QLatin1String commentRequestKeyText{"text"};

    template<>
    class de_serializer<CommentRequestDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static CommentRequestDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<CommentRequestDto>(json);
            return {
                deserialize_field<QString>(jo, commentRequestKeyText)
            };
        }

        static QJsonValue serialize(const CommentRequestDto &value) {
            QJsonObject jo;
            serialize_field(jo, commentRequestKeyText, value.text);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    CommentRequestDto CommentRequestDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<CommentRequestDto>(json);
    }

    CommentRequestDto::CommentRequestDto(
        QString text
    ) :
        text(std::move(text))
    { }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray CommentRequestDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // CsrfTokenDto

    static const QLatin1String csrfTokenKeyCsrfToken{"csrfToken"};

    template<>
    class de_serializer<CsrfTokenDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static CsrfTokenDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<CsrfTokenDto>(json);
            return {
                deserialize_field<QString>(jo, csrfTokenKeyCsrfToken)
            };
        }

        static QJsonValue serialize(const CsrfTokenDto &value) {
            QJsonObject jo;
            serialize_field(jo, csrfTokenKeyCsrfToken, value.csrfToken);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    CsrfTokenDto CsrfTokenDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<CsrfTokenDto>(json);
    }

    CsrfTokenDto::CsrfTokenDto(
        QString csrfToken
    ) :
        csrfToken(std::move(csrfToken))
    { }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray CsrfTokenDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // EntityDto

    static const QLatin1String entityKeyId{"id"};
    static const QLatin1String entityKeyName{"name"};
    static const QLatin1String entityKeyType{"type"};
    static const QLatin1String entityKeyPath{"path"};
    static const QLatin1String entityKeyLine{"line"};

    template<>
    class de_serializer<EntityDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static EntityDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<EntityDto>(json);
            return {
                deserialize_field<QString>(jo, entityKeyId),
                deserialize_field<QString>(jo, entityKeyName),
                deserialize_field<QString>(jo, entityKeyType),
                deserialize_field<std::optional<QString>>(jo, entityKeyPath),
                deserialize_field<std::optional<qint32>>(jo, entityKeyLine)
            };
        }

        static QJsonValue serialize(const EntityDto &value) {
            QJsonObject jo;
            serialize_field(jo, entityKeyId, value.id);
            serialize_field(jo, entityKeyName, value.name);
            serialize_field(jo, entityKeyType, value.type);
            serialize_field(jo, entityKeyPath, value.path);
            serialize_field(jo, entityKeyLine, value.line);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    EntityDto EntityDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<EntityDto>(json);
    }

    EntityDto::EntityDto(
        QString id,
        QString name,
        QString type,
        std::optional<QString> path,
        std::optional<qint32> line
    ) :
        id(std::move(id)),
        name(std::move(name)),
        type(std::move(type)),
        path(std::move(path)),
        line(std::move(line))
    { }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray EntityDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // ErrorDto

    static const QLatin1String errorKeyDashboardVersionNumber{"dashboardVersionNumber"};
    static const QLatin1String errorKeyType{"type"};
    static const QLatin1String errorKeyMessage{"message"};
    static const QLatin1String errorKeyLocalizedMessage{"localizedMessage"};
    static const QLatin1String errorKeyDetails{"details"};
    static const QLatin1String errorKeyLocalizedDetails{"localizedDetails"};
    static const QLatin1String errorKeySupportAddress{"supportAddress"};
    static const QLatin1String errorKeyDisplayServerBugHint{"displayServerBugHint"};
    static const QLatin1String errorKeyData{"data"};

    template<>
    class de_serializer<ErrorDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static ErrorDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<ErrorDto>(json);
            return {
                deserialize_field<std::optional<QString>>(jo, errorKeyDashboardVersionNumber),
                deserialize_field<QString>(jo, errorKeyType),
                deserialize_field<QString>(jo, errorKeyMessage),
                deserialize_field<QString>(jo, errorKeyLocalizedMessage),
                deserialize_field<std::optional<QString>>(jo, errorKeyDetails),
                deserialize_field<std::optional<QString>>(jo, errorKeyLocalizedDetails),
                deserialize_field<std::optional<QString>>(jo, errorKeySupportAddress),
                deserialize_field<std::optional<bool>>(jo, errorKeyDisplayServerBugHint),
                deserialize_field<std::optional<std::map<QString, Any>>>(jo, errorKeyData)
            };
        }

        static QJsonValue serialize(const ErrorDto &value) {
            QJsonObject jo;
            serialize_field(jo, errorKeyDashboardVersionNumber, value.dashboardVersionNumber);
            serialize_field(jo, errorKeyType, value.type);
            serialize_field(jo, errorKeyMessage, value.message);
            serialize_field(jo, errorKeyLocalizedMessage, value.localizedMessage);
            serialize_field(jo, errorKeyDetails, value.details);
            serialize_field(jo, errorKeyLocalizedDetails, value.localizedDetails);
            serialize_field(jo, errorKeySupportAddress, value.supportAddress);
            serialize_field(jo, errorKeyDisplayServerBugHint, value.displayServerBugHint);
            serialize_field(jo, errorKeyData, value.data);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    ErrorDto ErrorDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<ErrorDto>(json);
    }

    ErrorDto::ErrorDto(
        std::optional<QString> dashboardVersionNumber,
        QString type,
        QString message,
        QString localizedMessage,
        std::optional<QString> details,
        std::optional<QString> localizedDetails,
        std::optional<QString> supportAddress,
        std::optional<bool> displayServerBugHint,
        std::optional<std::map<QString, Any>> data
    ) :
        dashboardVersionNumber(std::move(dashboardVersionNumber)),
        type(std::move(type)),
        message(std::move(message)),
        localizedMessage(std::move(localizedMessage)),
        details(std::move(details)),
        localizedDetails(std::move(localizedDetails)),
        supportAddress(std::move(supportAddress)),
        displayServerBugHint(std::move(displayServerBugHint)),
        data(std::move(data))
    { }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray ErrorDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // IssueCommentDto

    static const QLatin1String issueCommentKeyUsername{"username"};
    static const QLatin1String issueCommentKeyUserDisplayName{"userDisplayName"};
    static const QLatin1String issueCommentKeyDate{"date"};
    static const QLatin1String issueCommentKeyDisplayDate{"displayDate"};
    static const QLatin1String issueCommentKeyText{"text"};
    static const QLatin1String issueCommentKeyHtml{"html"};
    static const QLatin1String issueCommentKeyCommentDeletionId{"commentDeletionId"};

    template<>
    class de_serializer<IssueCommentDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static IssueCommentDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<IssueCommentDto>(json);
            return {
                deserialize_field<QString>(jo, issueCommentKeyUsername),
                deserialize_field<QString>(jo, issueCommentKeyUserDisplayName),
                deserialize_field<QString>(jo, issueCommentKeyDate),
                deserialize_field<QString>(jo, issueCommentKeyDisplayDate),
                deserialize_field<QString>(jo, issueCommentKeyText),
                deserialize_field<std::optional<QString>>(jo, issueCommentKeyHtml),
                deserialize_field<std::optional<QString>>(jo, issueCommentKeyCommentDeletionId)
            };
        }

        static QJsonValue serialize(const IssueCommentDto &value) {
            QJsonObject jo;
            serialize_field(jo, issueCommentKeyUsername, value.username);
            serialize_field(jo, issueCommentKeyUserDisplayName, value.userDisplayName);
            serialize_field(jo, issueCommentKeyDate, value.date);
            serialize_field(jo, issueCommentKeyDisplayDate, value.displayDate);
            serialize_field(jo, issueCommentKeyText, value.text);
            serialize_field(jo, issueCommentKeyHtml, value.html);
            serialize_field(jo, issueCommentKeyCommentDeletionId, value.commentDeletionId);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    IssueCommentDto IssueCommentDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<IssueCommentDto>(json);
    }

    IssueCommentDto::IssueCommentDto(
        QString username,
        QString userDisplayName,
        QString date,
        QString displayDate,
        QString text,
        std::optional<QString> html,
        std::optional<QString> commentDeletionId
    ) :
        username(std::move(username)),
        userDisplayName(std::move(userDisplayName)),
        date(std::move(date)),
        displayDate(std::move(displayDate)),
        text(std::move(text)),
        html(std::move(html)),
        commentDeletionId(std::move(commentDeletionId))
    { }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray IssueCommentDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // IssueKind

    const QLatin1String IssueKindMeta::av{"AV"};
    const QLatin1String IssueKindMeta::cl{"CL"};
    const QLatin1String IssueKindMeta::cy{"CY"};
    const QLatin1String IssueKindMeta::de{"DE"};
    const QLatin1String IssueKindMeta::mv{"MV"};
    const QLatin1String IssueKindMeta::sv{"SV"};

    // throws std::range_error
    IssueKind IssueKindMeta::strToEnum(QAnyStringView str)
    {
        if (str == IssueKindMeta::av)
        {
            return IssueKind::av;
        }
        if (str == IssueKindMeta::cl)
        {
            return IssueKind::cl;
        }
        if (str == IssueKindMeta::cy)
        {
            return IssueKind::cy;
        }
        if (str == IssueKindMeta::de)
        {
            return IssueKind::de;
        }
        if (str == IssueKindMeta::mv)
        {
            return IssueKind::mv;
        }
        if (str == IssueKindMeta::sv)
        {
            return IssueKind::sv;
        }
        throw std::range_error(concat({ u8"Unknown IssueKind str: ", to_std_string(str) }));
    }

    QLatin1String IssueKindMeta::enumToStr(IssueKind e)
    {
        switch (e)
        {
        case IssueKind::av:
            return IssueKindMeta::av;
        case IssueKind::cl:
            return IssueKindMeta::cl;
        case IssueKind::cy:
            return IssueKindMeta::cy;
        case IssueKind::de:
            return IssueKindMeta::de;
        case IssueKind::mv:
            return IssueKindMeta::mv;
        case IssueKind::sv:
            return IssueKindMeta::sv;;
        default:
            throw std::domain_error(concat({
                u8"Unknown IssueKind enum: ",
                to_std_string(static_cast<int>(e))
                }));
        }
    }

    // IssueKindForNamedFilterCreation

    const QLatin1String IssueKindForNamedFilterCreationMeta::av{"AV"};
    const QLatin1String IssueKindForNamedFilterCreationMeta::cl{"CL"};
    const QLatin1String IssueKindForNamedFilterCreationMeta::cy{"CY"};
    const QLatin1String IssueKindForNamedFilterCreationMeta::de{"DE"};
    const QLatin1String IssueKindForNamedFilterCreationMeta::mv{"MV"};
    const QLatin1String IssueKindForNamedFilterCreationMeta::sv{"SV"};
    const QLatin1String IssueKindForNamedFilterCreationMeta::universal{"UNIVERSAL"};

    // throws std::range_error
    IssueKindForNamedFilterCreation IssueKindForNamedFilterCreationMeta::strToEnum(QAnyStringView str)
    {
        if (str == IssueKindForNamedFilterCreationMeta::av)
        {
            return IssueKindForNamedFilterCreation::av;
        }
        if (str == IssueKindForNamedFilterCreationMeta::cl)
        {
            return IssueKindForNamedFilterCreation::cl;
        }
        if (str == IssueKindForNamedFilterCreationMeta::cy)
        {
            return IssueKindForNamedFilterCreation::cy;
        }
        if (str == IssueKindForNamedFilterCreationMeta::de)
        {
            return IssueKindForNamedFilterCreation::de;
        }
        if (str == IssueKindForNamedFilterCreationMeta::mv)
        {
            return IssueKindForNamedFilterCreation::mv;
        }
        if (str == IssueKindForNamedFilterCreationMeta::sv)
        {
            return IssueKindForNamedFilterCreation::sv;
        }
        if (str == IssueKindForNamedFilterCreationMeta::universal)
        {
            return IssueKindForNamedFilterCreation::universal;
        }
        throw std::range_error(concat({ u8"Unknown IssueKindForNamedFilterCreation str: ", to_std_string(str) }));
    }

    QLatin1String IssueKindForNamedFilterCreationMeta::enumToStr(IssueKindForNamedFilterCreation e)
    {
        switch (e)
        {
        case IssueKindForNamedFilterCreation::av:
            return IssueKindForNamedFilterCreationMeta::av;
        case IssueKindForNamedFilterCreation::cl:
            return IssueKindForNamedFilterCreationMeta::cl;
        case IssueKindForNamedFilterCreation::cy:
            return IssueKindForNamedFilterCreationMeta::cy;
        case IssueKindForNamedFilterCreation::de:
            return IssueKindForNamedFilterCreationMeta::de;
        case IssueKindForNamedFilterCreation::mv:
            return IssueKindForNamedFilterCreationMeta::mv;
        case IssueKindForNamedFilterCreation::sv:
            return IssueKindForNamedFilterCreationMeta::sv;
        case IssueKindForNamedFilterCreation::universal:
            return IssueKindForNamedFilterCreationMeta::universal;;
        default:
            throw std::domain_error(concat({
                u8"Unknown IssueKindForNamedFilterCreation enum: ",
                to_std_string(static_cast<int>(e))
                }));
        }
    }

    // IssueSourceLocationDto

    static const QLatin1String issueSourceLocationKeyFileName{"fileName"};
    static const QLatin1String issueSourceLocationKeyRole{"role"};
    static const QLatin1String issueSourceLocationKeySourceCodeUrl{"sourceCodeUrl"};
    static const QLatin1String issueSourceLocationKeyStartLine{"startLine"};
    static const QLatin1String issueSourceLocationKeyStartColumn{"startColumn"};
    static const QLatin1String issueSourceLocationKeyEndLine{"endLine"};
    static const QLatin1String issueSourceLocationKeyEndColumn{"endColumn"};

    template<>
    class de_serializer<IssueSourceLocationDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static IssueSourceLocationDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<IssueSourceLocationDto>(json);
            return {
                deserialize_field<QString>(jo, issueSourceLocationKeyFileName),
                deserialize_field<std::optional<QString>>(jo, issueSourceLocationKeyRole),
                deserialize_field<QString>(jo, issueSourceLocationKeySourceCodeUrl),
                deserialize_field<qint32>(jo, issueSourceLocationKeyStartLine),
                deserialize_field<qint32>(jo, issueSourceLocationKeyStartColumn),
                deserialize_field<qint32>(jo, issueSourceLocationKeyEndLine),
                deserialize_field<qint32>(jo, issueSourceLocationKeyEndColumn)
            };
        }

        static QJsonValue serialize(const IssueSourceLocationDto &value) {
            QJsonObject jo;
            serialize_field(jo, issueSourceLocationKeyFileName, value.fileName);
            serialize_field(jo, issueSourceLocationKeyRole, value.role);
            serialize_field(jo, issueSourceLocationKeySourceCodeUrl, value.sourceCodeUrl);
            serialize_field(jo, issueSourceLocationKeyStartLine, value.startLine);
            serialize_field(jo, issueSourceLocationKeyStartColumn, value.startColumn);
            serialize_field(jo, issueSourceLocationKeyEndLine, value.endLine);
            serialize_field(jo, issueSourceLocationKeyEndColumn, value.endColumn);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    IssueSourceLocationDto IssueSourceLocationDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<IssueSourceLocationDto>(json);
    }

    IssueSourceLocationDto::IssueSourceLocationDto(
        QString fileName,
        std::optional<QString> role,
        QString sourceCodeUrl,
        qint32 startLine,
        qint32 startColumn,
        qint32 endLine,
        qint32 endColumn
    ) :
        fileName(std::move(fileName)),
        role(std::move(role)),
        sourceCodeUrl(std::move(sourceCodeUrl)),
        startLine(std::move(startLine)),
        startColumn(std::move(startColumn)),
        endLine(std::move(endLine)),
        endColumn(std::move(endColumn))
    { }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray IssueSourceLocationDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // IssueTagDto

    static const QLatin1String issueTagKeyTag{"tag"};
    static const QLatin1String issueTagKeyColor{"color"};

    template<>
    class de_serializer<IssueTagDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static IssueTagDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<IssueTagDto>(json);
            return {
                deserialize_field<QString>(jo, issueTagKeyTag),
                deserialize_field<QString>(jo, issueTagKeyColor)
            };
        }

        static QJsonValue serialize(const IssueTagDto &value) {
            QJsonObject jo;
            serialize_field(jo, issueTagKeyTag, value.tag);
            serialize_field(jo, issueTagKeyColor, value.color);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    IssueTagDto IssueTagDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<IssueTagDto>(json);
    }

    IssueTagDto::IssueTagDto(
        QString tag,
        QString color
    ) :
        tag(std::move(tag)),
        color(std::move(color))
    { }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray IssueTagDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // IssueTagTypeDto

    static const QLatin1String issueTagTypeKeyId{"id"};
    static const QLatin1String issueTagTypeKeyText{"text"};
    static const QLatin1String issueTagTypeKeyTag{"tag"};
    static const QLatin1String issueTagTypeKeyColor{"color"};
    static const QLatin1String issueTagTypeKeyDescription{"description"};
    static const QLatin1String issueTagTypeKeySelected{"selected"};

    template<>
    class de_serializer<IssueTagTypeDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static IssueTagTypeDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<IssueTagTypeDto>(json);
            return {
                deserialize_field<QString>(jo, issueTagTypeKeyId),
                deserialize_field<std::optional<QString>>(jo, issueTagTypeKeyText),
                deserialize_field<std::optional<QString>>(jo, issueTagTypeKeyTag),
                deserialize_field<QString>(jo, issueTagTypeKeyColor),
                deserialize_field<std::optional<QString>>(jo, issueTagTypeKeyDescription),
                deserialize_field<std::optional<bool>>(jo, issueTagTypeKeySelected)
            };
        }

        static QJsonValue serialize(const IssueTagTypeDto &value) {
            QJsonObject jo;
            serialize_field(jo, issueTagTypeKeyId, value.id);
            serialize_field(jo, issueTagTypeKeyText, value.text);
            serialize_field(jo, issueTagTypeKeyTag, value.tag);
            serialize_field(jo, issueTagTypeKeyColor, value.color);
            serialize_field(jo, issueTagTypeKeyDescription, value.description);
            serialize_field(jo, issueTagTypeKeySelected, value.selected);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    IssueTagTypeDto IssueTagTypeDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<IssueTagTypeDto>(json);
    }

    IssueTagTypeDto::IssueTagTypeDto(
        QString id,
        std::optional<QString> text,
        std::optional<QString> tag,
        QString color,
        std::optional<QString> description,
        std::optional<bool> selected
    ) :
        id(std::move(id)),
        text(std::move(text)),
        tag(std::move(tag)),
        color(std::move(color)),
        description(std::move(description)),
        selected(std::move(selected))
    { }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray IssueTagTypeDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // MessageSeverity

    const QLatin1String MessageSeverityMeta::debug{"DEBUG"};
    const QLatin1String MessageSeverityMeta::info{"INFO"};
    const QLatin1String MessageSeverityMeta::warning{"WARNING"};
    const QLatin1String MessageSeverityMeta::error{"ERROR"};
    const QLatin1String MessageSeverityMeta::fatal{"FATAL"};

    // throws std::range_error
    MessageSeverity MessageSeverityMeta::strToEnum(QAnyStringView str)
    {
        if (str == MessageSeverityMeta::debug)
        {
            return MessageSeverity::debug;
        }
        if (str == MessageSeverityMeta::info)
        {
            return MessageSeverity::info;
        }
        if (str == MessageSeverityMeta::warning)
        {
            return MessageSeverity::warning;
        }
        if (str == MessageSeverityMeta::error)
        {
            return MessageSeverity::error;
        }
        if (str == MessageSeverityMeta::fatal)
        {
            return MessageSeverity::fatal;
        }
        throw std::range_error(concat({ u8"Unknown MessageSeverity str: ", to_std_string(str) }));
    }

    QLatin1String MessageSeverityMeta::enumToStr(MessageSeverity e)
    {
        switch (e)
        {
        case MessageSeverity::debug:
            return MessageSeverityMeta::debug;
        case MessageSeverity::info:
            return MessageSeverityMeta::info;
        case MessageSeverity::warning:
            return MessageSeverityMeta::warning;
        case MessageSeverity::error:
            return MessageSeverityMeta::error;
        case MessageSeverity::fatal:
            return MessageSeverityMeta::fatal;;
        default:
            throw std::domain_error(concat({
                u8"Unknown MessageSeverity enum: ",
                to_std_string(static_cast<int>(e))
                }));
        }
    }

    // MetricDto

    static const QLatin1String metricKeyName{"name"};
    static const QLatin1String metricKeyDisplayName{"displayName"};
    static const QLatin1String metricKeyMinValue{"minValue"};
    static const QLatin1String metricKeyMaxValue{"maxValue"};

    template<>
    class de_serializer<MetricDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static MetricDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<MetricDto>(json);
            return {
                deserialize_field<QString>(jo, metricKeyName),
                deserialize_field<QString>(jo, metricKeyDisplayName),
                deserialize_field<Any>(jo, metricKeyMinValue),
                deserialize_field<Any>(jo, metricKeyMaxValue)
            };
        }

        static QJsonValue serialize(const MetricDto &value) {
            QJsonObject jo;
            serialize_field(jo, metricKeyName, value.name);
            serialize_field(jo, metricKeyDisplayName, value.displayName);
            serialize_field(jo, metricKeyMinValue, value.minValue);
            serialize_field(jo, metricKeyMaxValue, value.maxValue);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    MetricDto MetricDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<MetricDto>(json);
    }

    MetricDto::MetricDto(
        QString name,
        QString displayName,
        Any minValue,
        Any maxValue
    ) :
        name(std::move(name)),
        displayName(std::move(displayName)),
        minValue(std::move(minValue)),
        maxValue(std::move(maxValue))
    { }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray MetricDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // MetricValueTableRowDto

    static const QLatin1String metricValueTableRowKeyMetric{"metric"};
    static const QLatin1String metricValueTableRowKeyPath{"path"};
    static const QLatin1String metricValueTableRowKeyLine{"line"};
    static const QLatin1String metricValueTableRowKeyValue{"value"};
    static const QLatin1String metricValueTableRowKeyEntity{"entity"};
    static const QLatin1String metricValueTableRowKeyEntityType{"entityType"};
    static const QLatin1String metricValueTableRowKeyEntityId{"entityId"};

    template<>
    class de_serializer<MetricValueTableRowDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static MetricValueTableRowDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<MetricValueTableRowDto>(json);
            return {
                deserialize_field<QString>(jo, metricValueTableRowKeyMetric),
                deserialize_field<std::optional<QString>>(jo, metricValueTableRowKeyPath),
                deserialize_field<std::optional<qint32>>(jo, metricValueTableRowKeyLine),
                deserialize_field<std::optional<double>>(jo, metricValueTableRowKeyValue),
                deserialize_field<QString>(jo, metricValueTableRowKeyEntity),
                deserialize_field<QString>(jo, metricValueTableRowKeyEntityType),
                deserialize_field<QString>(jo, metricValueTableRowKeyEntityId)
            };
        }

        static QJsonValue serialize(const MetricValueTableRowDto &value) {
            QJsonObject jo;
            serialize_field(jo, metricValueTableRowKeyMetric, value.metric);
            serialize_field(jo, metricValueTableRowKeyPath, value.path);
            serialize_field(jo, metricValueTableRowKeyLine, value.line);
            serialize_field(jo, metricValueTableRowKeyValue, value.value);
            serialize_field(jo, metricValueTableRowKeyEntity, value.entity);
            serialize_field(jo, metricValueTableRowKeyEntityType, value.entityType);
            serialize_field(jo, metricValueTableRowKeyEntityId, value.entityId);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    MetricValueTableRowDto MetricValueTableRowDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<MetricValueTableRowDto>(json);
    }

    MetricValueTableRowDto::MetricValueTableRowDto(
        QString metric,
        std::optional<QString> path,
        std::optional<qint32> line,
        std::optional<double> value,
        QString entity,
        QString entityType,
        QString entityId
    ) :
        metric(std::move(metric)),
        path(std::move(path)),
        line(std::move(line)),
        value(std::move(value)),
        entity(std::move(entity)),
        entityType(std::move(entityType)),
        entityId(std::move(entityId))
    { }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray MetricValueTableRowDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // NamedFilterType

    const QLatin1String NamedFilterTypeMeta::predefined{"PREDEFINED"};
    const QLatin1String NamedFilterTypeMeta::global{"GLOBAL"};
    const QLatin1String NamedFilterTypeMeta::custom{"CUSTOM"};

    // throws std::range_error
    NamedFilterType NamedFilterTypeMeta::strToEnum(QAnyStringView str)
    {
        if (str == NamedFilterTypeMeta::predefined)
        {
            return NamedFilterType::predefined;
        }
        if (str == NamedFilterTypeMeta::global)
        {
            return NamedFilterType::global;
        }
        if (str == NamedFilterTypeMeta::custom)
        {
            return NamedFilterType::custom;
        }
        throw std::range_error(concat({ u8"Unknown NamedFilterType str: ", to_std_string(str) }));
    }

    QLatin1String NamedFilterTypeMeta::enumToStr(NamedFilterType e)
    {
        switch (e)
        {
        case NamedFilterType::predefined:
            return NamedFilterTypeMeta::predefined;
        case NamedFilterType::global:
            return NamedFilterTypeMeta::global;
        case NamedFilterType::custom:
            return NamedFilterTypeMeta::custom;;
        default:
            throw std::domain_error(concat({
                u8"Unknown NamedFilterType enum: ",
                to_std_string(static_cast<int>(e))
                }));
        }
    }

    // NamedFilterVisibilityDto

    static const QLatin1String namedFilterVisibilityKeyGroups{"groups"};

    template<>
    class de_serializer<NamedFilterVisibilityDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static NamedFilterVisibilityDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<NamedFilterVisibilityDto>(json);
            return {
                deserialize_field<std::optional<std::vector<QString>>>(jo, namedFilterVisibilityKeyGroups)
            };
        }

        static QJsonValue serialize(const NamedFilterVisibilityDto &value) {
            QJsonObject jo;
            serialize_field(jo, namedFilterVisibilityKeyGroups, value.groups);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    NamedFilterVisibilityDto NamedFilterVisibilityDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<NamedFilterVisibilityDto>(json);
    }

    NamedFilterVisibilityDto::NamedFilterVisibilityDto(
        std::optional<std::vector<QString>> groups
    ) :
        groups(std::move(groups))
    { }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray NamedFilterVisibilityDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // ProjectReferenceDto

    static const QLatin1String projectReferenceKeyName{"name"};
    static const QLatin1String projectReferenceKeyUrl{"url"};

    template<>
    class de_serializer<ProjectReferenceDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static ProjectReferenceDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<ProjectReferenceDto>(json);
            return {
                deserialize_field<QString>(jo, projectReferenceKeyName),
                deserialize_field<QString>(jo, projectReferenceKeyUrl)
            };
        }

        static QJsonValue serialize(const ProjectReferenceDto &value) {
            QJsonObject jo;
            serialize_field(jo, projectReferenceKeyName, value.name);
            serialize_field(jo, projectReferenceKeyUrl, value.url);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    ProjectReferenceDto ProjectReferenceDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<ProjectReferenceDto>(json);
    }

    ProjectReferenceDto::ProjectReferenceDto(
        QString name,
        QString url
    ) :
        name(std::move(name)),
        url(std::move(url))
    { }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray ProjectReferenceDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // RuleDto

    static const QLatin1String ruleKeyName{"name"};
    static const QLatin1String ruleKeyOriginal_name{"original_name"};
    static const QLatin1String ruleKeyDisabled{"disabled"};

    template<>
    class de_serializer<RuleDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static RuleDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<RuleDto>(json);
            return {
                deserialize_field<QString>(jo, ruleKeyName),
                deserialize_field<QString>(jo, ruleKeyOriginal_name),
                deserialize_field<std::optional<bool>>(jo, ruleKeyDisabled)
            };
        }

        static QJsonValue serialize(const RuleDto &value) {
            QJsonObject jo;
            serialize_field(jo, ruleKeyName, value.name);
            serialize_field(jo, ruleKeyOriginal_name, value.original_name);
            serialize_field(jo, ruleKeyDisabled, value.disabled);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    RuleDto RuleDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<RuleDto>(json);
    }

    RuleDto::RuleDto(
        QString name,
        QString original_name,
        std::optional<bool> disabled
    ) :
        name(std::move(name)),
        original_name(std::move(original_name)),
        disabled(std::move(disabled))
    { }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray RuleDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // SortDirection

    const QLatin1String SortDirectionMeta::asc{"ASC"};
    const QLatin1String SortDirectionMeta::desc{"DESC"};

    // throws std::range_error
    SortDirection SortDirectionMeta::strToEnum(QAnyStringView str)
    {
        if (str == SortDirectionMeta::asc)
        {
            return SortDirection::asc;
        }
        if (str == SortDirectionMeta::desc)
        {
            return SortDirection::desc;
        }
        throw std::range_error(concat({ u8"Unknown SortDirection str: ", to_std_string(str) }));
    }

    QLatin1String SortDirectionMeta::enumToStr(SortDirection e)
    {
        switch (e)
        {
        case SortDirection::asc:
            return SortDirectionMeta::asc;
        case SortDirection::desc:
            return SortDirectionMeta::desc;;
        default:
            throw std::domain_error(concat({
                u8"Unknown SortDirection enum: ",
                to_std_string(static_cast<int>(e))
                }));
        }
    }

    // TableCellAlignment

    const QLatin1String TableCellAlignmentMeta::left{"left"};
    const QLatin1String TableCellAlignmentMeta::right{"right"};
    const QLatin1String TableCellAlignmentMeta::center{"center"};

    // throws std::range_error
    TableCellAlignment TableCellAlignmentMeta::strToEnum(QAnyStringView str)
    {
        if (str == TableCellAlignmentMeta::left)
        {
            return TableCellAlignment::left;
        }
        if (str == TableCellAlignmentMeta::right)
        {
            return TableCellAlignment::right;
        }
        if (str == TableCellAlignmentMeta::center)
        {
            return TableCellAlignment::center;
        }
        throw std::range_error(concat({ u8"Unknown TableCellAlignment str: ", to_std_string(str) }));
    }

    QLatin1String TableCellAlignmentMeta::enumToStr(TableCellAlignment e)
    {
        switch (e)
        {
        case TableCellAlignment::left:
            return TableCellAlignmentMeta::left;
        case TableCellAlignment::right:
            return TableCellAlignmentMeta::right;
        case TableCellAlignment::center:
            return TableCellAlignmentMeta::center;;
        default:
            throw std::domain_error(concat({
                u8"Unknown TableCellAlignment enum: ",
                to_std_string(static_cast<int>(e))
                }));
        }
    }

    // ToolsVersionDto

    static const QLatin1String toolsVersionKeyName{"name"};
    static const QLatin1String toolsVersionKeyNumber{"number"};
    static const QLatin1String toolsVersionKeyBuildDate{"buildDate"};

    template<>
    class de_serializer<ToolsVersionDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static ToolsVersionDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<ToolsVersionDto>(json);
            return {
                deserialize_field<QString>(jo, toolsVersionKeyName),
                deserialize_field<QString>(jo, toolsVersionKeyNumber),
                deserialize_field<QString>(jo, toolsVersionKeyBuildDate)
            };
        }

        static QJsonValue serialize(const ToolsVersionDto &value) {
            QJsonObject jo;
            serialize_field(jo, toolsVersionKeyName, value.name);
            serialize_field(jo, toolsVersionKeyNumber, value.number);
            serialize_field(jo, toolsVersionKeyBuildDate, value.buildDate);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    ToolsVersionDto ToolsVersionDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<ToolsVersionDto>(json);
    }

    ToolsVersionDto::ToolsVersionDto(
        QString name,
        QString number,
        QString buildDate
    ) :
        name(std::move(name)),
        number(std::move(number)),
        buildDate(std::move(buildDate))
    { }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray ToolsVersionDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // UserRefType

    const QLatin1String UserRefTypeMeta::virtual_user{"VIRTUAL_USER"};
    const QLatin1String UserRefTypeMeta::dashboard_user{"DASHBOARD_USER"};
    const QLatin1String UserRefTypeMeta::unmapped_user{"UNMAPPED_USER"};

    // throws std::range_error
    UserRefType UserRefTypeMeta::strToEnum(QAnyStringView str)
    {
        if (str == UserRefTypeMeta::virtual_user)
        {
            return UserRefType::virtual_user;
        }
        if (str == UserRefTypeMeta::dashboard_user)
        {
            return UserRefType::dashboard_user;
        }
        if (str == UserRefTypeMeta::unmapped_user)
        {
            return UserRefType::unmapped_user;
        }
        throw std::range_error(concat({ u8"Unknown UserRefType str: ", to_std_string(str) }));
    }

    QLatin1String UserRefTypeMeta::enumToStr(UserRefType e)
    {
        switch (e)
        {
        case UserRefType::virtual_user:
            return UserRefTypeMeta::virtual_user;
        case UserRefType::dashboard_user:
            return UserRefTypeMeta::dashboard_user;
        case UserRefType::unmapped_user:
            return UserRefTypeMeta::unmapped_user;;
        default:
            throw std::domain_error(concat({
                u8"Unknown UserRefType enum: ",
                to_std_string(static_cast<int>(e))
                }));
        }
    }

    // VersionKindCountDto

    static const QLatin1String versionKindCountKeyTotal{"Total"};
    static const QLatin1String versionKindCountKeyAdded{"Added"};
    static const QLatin1String versionKindCountKeyRemoved{"Removed"};

    template<>
    class de_serializer<VersionKindCountDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static VersionKindCountDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<VersionKindCountDto>(json);
            return {
                deserialize_field<qint32>(jo, versionKindCountKeyTotal),
                deserialize_field<qint32>(jo, versionKindCountKeyAdded),
                deserialize_field<qint32>(jo, versionKindCountKeyRemoved)
            };
        }

        static QJsonValue serialize(const VersionKindCountDto &value) {
            QJsonObject jo;
            serialize_field(jo, versionKindCountKeyTotal, value.Total);
            serialize_field(jo, versionKindCountKeyAdded, value.Added);
            serialize_field(jo, versionKindCountKeyRemoved, value.Removed);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    VersionKindCountDto VersionKindCountDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<VersionKindCountDto>(json);
    }

    VersionKindCountDto::VersionKindCountDto(
        qint32 Total,
        qint32 Added,
        qint32 Removed
    ) :
        Total(std::move(Total)),
        Added(std::move(Added)),
        Removed(std::move(Removed))
    { }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray VersionKindCountDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // AnalysisVersionDto

    static const QLatin1String analysisVersionKeyDate{"date"};
    static const QLatin1String analysisVersionKeyLabel{"label"};
    static const QLatin1String analysisVersionKeyIndex{"index"};
    static const QLatin1String analysisVersionKeyName{"name"};
    static const QLatin1String analysisVersionKeyMillis{"millis"};
    static const QLatin1String analysisVersionKeyIssueCounts{"issueCounts"};
    static const QLatin1String analysisVersionKeyToolsVersion{"toolsVersion"};
    static const QLatin1String analysisVersionKeyLinesOfCode{"linesOfCode"};
    static const QLatin1String analysisVersionKeyCloneRatio{"cloneRatio"};

    template<>
    class de_serializer<AnalysisVersionDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static AnalysisVersionDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<AnalysisVersionDto>(json);
            return {
                deserialize_field<QString>(jo, analysisVersionKeyDate),
                deserialize_field<std::optional<QString>>(jo, analysisVersionKeyLabel),
                deserialize_field<qint32>(jo, analysisVersionKeyIndex),
                deserialize_field<QString>(jo, analysisVersionKeyName),
                deserialize_field<qint64>(jo, analysisVersionKeyMillis),
                deserialize_field<Any>(jo, analysisVersionKeyIssueCounts),
                deserialize_field<std::optional<ToolsVersionDto>>(jo, analysisVersionKeyToolsVersion),
                deserialize_field<std::optional<qint64>>(jo, analysisVersionKeyLinesOfCode),
                deserialize_field<std::optional<double>>(jo, analysisVersionKeyCloneRatio)
            };
        }

        static QJsonValue serialize(const AnalysisVersionDto &value) {
            QJsonObject jo;
            serialize_field(jo, analysisVersionKeyDate, value.date);
            serialize_field(jo, analysisVersionKeyLabel, value.label);
            serialize_field(jo, analysisVersionKeyIndex, value.index);
            serialize_field(jo, analysisVersionKeyName, value.name);
            serialize_field(jo, analysisVersionKeyMillis, value.millis);
            serialize_field(jo, analysisVersionKeyIssueCounts, value.issueCounts);
            serialize_field(jo, analysisVersionKeyToolsVersion, value.toolsVersion);
            serialize_field(jo, analysisVersionKeyLinesOfCode, value.linesOfCode);
            serialize_field(jo, analysisVersionKeyCloneRatio, value.cloneRatio);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    AnalysisVersionDto AnalysisVersionDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<AnalysisVersionDto>(json);
    }

    AnalysisVersionDto::AnalysisVersionDto(
        QString date,
        std::optional<QString> label,
        qint32 index,
        QString name,
        qint64 millis,
        Any issueCounts,
        std::optional<ToolsVersionDto> toolsVersion,
        std::optional<qint64> linesOfCode,
        std::optional<double> cloneRatio
    ) :
        date(std::move(date)),
        label(std::move(label)),
        index(std::move(index)),
        name(std::move(name)),
        millis(std::move(millis)),
        issueCounts(std::move(issueCounts)),
        toolsVersion(std::move(toolsVersion)),
        linesOfCode(std::move(linesOfCode)),
        cloneRatio(std::move(cloneRatio))
    { }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray AnalysisVersionDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // ApiTokenCreationRequestDto

    static const QLatin1String apiTokenCreationRequestKeyPassword{"password"};
    static const QLatin1String apiTokenCreationRequestKeyType{"type"};
    static const QLatin1String apiTokenCreationRequestKeyDescription{"description"};
    static const QLatin1String apiTokenCreationRequestKeyMaxAgeMillis{"maxAgeMillis"};

    template<>
    class de_serializer<ApiTokenCreationRequestDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static ApiTokenCreationRequestDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<ApiTokenCreationRequestDto>(json);
            return {
                deserialize_field<QString>(jo, apiTokenCreationRequestKeyPassword),
                deserialize_field<QString>(jo, apiTokenCreationRequestKeyType),
                deserialize_field<QString>(jo, apiTokenCreationRequestKeyDescription),
                deserialize_field<qint64>(jo, apiTokenCreationRequestKeyMaxAgeMillis)
            };
        }

        static QJsonValue serialize(const ApiTokenCreationRequestDto &value) {
            QJsonObject jo;
            serialize_field(jo, apiTokenCreationRequestKeyPassword, value.password);
            serialize_field(jo, apiTokenCreationRequestKeyType, value.type);
            serialize_field(jo, apiTokenCreationRequestKeyDescription, value.description);
            serialize_field(jo, apiTokenCreationRequestKeyMaxAgeMillis, value.maxAgeMillis);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    ApiTokenCreationRequestDto ApiTokenCreationRequestDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<ApiTokenCreationRequestDto>(json);
    }

    ApiTokenCreationRequestDto::ApiTokenCreationRequestDto(
        QString password,
        QString type,
        QString description,
        qint64 maxAgeMillis
    ) :
        password(std::move(password)),
        type(std::move(type)),
        description(std::move(description)),
        maxAgeMillis(std::move(maxAgeMillis))
    { }

    ApiTokenCreationRequestDto::ApiTokenCreationRequestDto(
        QString password,
        ApiTokenType type,
        QString description,
        qint64 maxAgeMillis
    ) : ApiTokenCreationRequestDto(
        std::move(password),
        ApiTokenTypeMeta::enumToStr(type),
        std::move(description),
        std::move(maxAgeMillis))
    { }

    // throws std::range_error
    ApiTokenType ApiTokenCreationRequestDto::getTypeEnum() const
    {
        return ApiTokenTypeMeta::strToEnum(type);
    }

    void ApiTokenCreationRequestDto::setTypeEnum(ApiTokenType newValue)
    {
        type = ApiTokenTypeMeta::enumToStr(newValue);
    }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray ApiTokenCreationRequestDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // ApiTokenInfoDto

    static const QLatin1String apiTokenInfoKeyId{"id"};
    static const QLatin1String apiTokenInfoKeyUrl{"url"};
    static const QLatin1String apiTokenInfoKeyIsValid{"isValid"};
    static const QLatin1String apiTokenInfoKeyType{"type"};
    static const QLatin1String apiTokenInfoKeyDescription{"description"};
    static const QLatin1String apiTokenInfoKeyToken{"token"};
    static const QLatin1String apiTokenInfoKeyCreationDate{"creationDate"};
    static const QLatin1String apiTokenInfoKeyDisplayCreationDate{"displayCreationDate"};
    static const QLatin1String apiTokenInfoKeyExpirationDate{"expirationDate"};
    static const QLatin1String apiTokenInfoKeyDisplayExpirationDate{"displayExpirationDate"};
    static const QLatin1String apiTokenInfoKeyLastUseDate{"lastUseDate"};
    static const QLatin1String apiTokenInfoKeyDisplayLastUseDate{"displayLastUseDate"};
    static const QLatin1String apiTokenInfoKeyUsedByCurrentRequest{"usedByCurrentRequest"};

    template<>
    class de_serializer<ApiTokenInfoDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static ApiTokenInfoDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<ApiTokenInfoDto>(json);
            return {
                deserialize_field<QString>(jo, apiTokenInfoKeyId),
                deserialize_field<QString>(jo, apiTokenInfoKeyUrl),
                deserialize_field<bool>(jo, apiTokenInfoKeyIsValid),
                deserialize_field<QString>(jo, apiTokenInfoKeyType),
                deserialize_field<QString>(jo, apiTokenInfoKeyDescription),
                deserialize_field<std::optional<QString>>(jo, apiTokenInfoKeyToken),
                deserialize_field<QString>(jo, apiTokenInfoKeyCreationDate),
                deserialize_field<QString>(jo, apiTokenInfoKeyDisplayCreationDate),
                deserialize_field<QString>(jo, apiTokenInfoKeyExpirationDate),
                deserialize_field<QString>(jo, apiTokenInfoKeyDisplayExpirationDate),
                deserialize_field<std::optional<QString>>(jo, apiTokenInfoKeyLastUseDate),
                deserialize_field<QString>(jo, apiTokenInfoKeyDisplayLastUseDate),
                deserialize_field<bool>(jo, apiTokenInfoKeyUsedByCurrentRequest)
            };
        }

        static QJsonValue serialize(const ApiTokenInfoDto &value) {
            QJsonObject jo;
            serialize_field(jo, apiTokenInfoKeyId, value.id);
            serialize_field(jo, apiTokenInfoKeyUrl, value.url);
            serialize_field(jo, apiTokenInfoKeyIsValid, value.isValid);
            serialize_field(jo, apiTokenInfoKeyType, value.type);
            serialize_field(jo, apiTokenInfoKeyDescription, value.description);
            serialize_field(jo, apiTokenInfoKeyToken, value.token);
            serialize_field(jo, apiTokenInfoKeyCreationDate, value.creationDate);
            serialize_field(jo, apiTokenInfoKeyDisplayCreationDate, value.displayCreationDate);
            serialize_field(jo, apiTokenInfoKeyExpirationDate, value.expirationDate);
            serialize_field(jo, apiTokenInfoKeyDisplayExpirationDate, value.displayExpirationDate);
            serialize_field(jo, apiTokenInfoKeyLastUseDate, value.lastUseDate);
            serialize_field(jo, apiTokenInfoKeyDisplayLastUseDate, value.displayLastUseDate);
            serialize_field(jo, apiTokenInfoKeyUsedByCurrentRequest, value.usedByCurrentRequest);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    ApiTokenInfoDto ApiTokenInfoDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<ApiTokenInfoDto>(json);
    }

    ApiTokenInfoDto::ApiTokenInfoDto(
        QString id,
        QString url,
        bool isValid,
        QString type,
        QString description,
        std::optional<QString> token,
        QString creationDate,
        QString displayCreationDate,
        QString expirationDate,
        QString displayExpirationDate,
        std::optional<QString> lastUseDate,
        QString displayLastUseDate,
        bool usedByCurrentRequest
    ) :
        id(std::move(id)),
        url(std::move(url)),
        isValid(std::move(isValid)),
        type(std::move(type)),
        description(std::move(description)),
        token(std::move(token)),
        creationDate(std::move(creationDate)),
        displayCreationDate(std::move(displayCreationDate)),
        expirationDate(std::move(expirationDate)),
        displayExpirationDate(std::move(displayExpirationDate)),
        lastUseDate(std::move(lastUseDate)),
        displayLastUseDate(std::move(displayLastUseDate)),
        usedByCurrentRequest(std::move(usedByCurrentRequest))
    { }

    ApiTokenInfoDto::ApiTokenInfoDto(
        QString id,
        QString url,
        bool isValid,
        ApiTokenType type,
        QString description,
        std::optional<QString> token,
        QString creationDate,
        QString displayCreationDate,
        QString expirationDate,
        QString displayExpirationDate,
        std::optional<QString> lastUseDate,
        QString displayLastUseDate,
        bool usedByCurrentRequest
    ) : ApiTokenInfoDto(
        std::move(id),
        std::move(url),
        std::move(isValid),
        ApiTokenTypeMeta::enumToStr(type),
        std::move(description),
        std::move(token),
        std::move(creationDate),
        std::move(displayCreationDate),
        std::move(expirationDate),
        std::move(displayExpirationDate),
        std::move(lastUseDate),
        std::move(displayLastUseDate),
        std::move(usedByCurrentRequest))
    { }

    // throws std::range_error
    ApiTokenType ApiTokenInfoDto::getTypeEnum() const
    {
        return ApiTokenTypeMeta::strToEnum(type);
    }

    void ApiTokenInfoDto::setTypeEnum(ApiTokenType newValue)
    {
        type = ApiTokenTypeMeta::enumToStr(newValue);
    }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray ApiTokenInfoDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // ColumnInfoDto

    static const QLatin1String columnInfoKeyKey{"key"};
    static const QLatin1String columnInfoKeyHeader{"header"};
    static const QLatin1String columnInfoKeyCanSort{"canSort"};
    static const QLatin1String columnInfoKeyCanFilter{"canFilter"};
    static const QLatin1String columnInfoKeyAlignment{"alignment"};
    static const QLatin1String columnInfoKeyType{"type"};
    static const QLatin1String columnInfoKeyTypeOptions{"typeOptions"};
    static const QLatin1String columnInfoKeyWidth{"width"};
    static const QLatin1String columnInfoKeyShowByDefault{"showByDefault"};
    static const QLatin1String columnInfoKeyLinkKey{"linkKey"};

    template<>
    class de_serializer<ColumnInfoDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static ColumnInfoDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<ColumnInfoDto>(json);
            return {
                deserialize_field<QString>(jo, columnInfoKeyKey),
                deserialize_field<std::optional<QString>>(jo, columnInfoKeyHeader),
                deserialize_field<bool>(jo, columnInfoKeyCanSort),
                deserialize_field<bool>(jo, columnInfoKeyCanFilter),
                deserialize_field<QString>(jo, columnInfoKeyAlignment),
                deserialize_field<QString>(jo, columnInfoKeyType),
                deserialize_field<std::optional<std::vector<ColumnTypeOptionDto>>>(jo, columnInfoKeyTypeOptions),
                deserialize_field<qint32>(jo, columnInfoKeyWidth),
                deserialize_field<bool>(jo, columnInfoKeyShowByDefault),
                deserialize_field<std::optional<QString>>(jo, columnInfoKeyLinkKey)
            };
        }

        static QJsonValue serialize(const ColumnInfoDto &value) {
            QJsonObject jo;
            serialize_field(jo, columnInfoKeyKey, value.key);
            serialize_field(jo, columnInfoKeyHeader, value.header);
            serialize_field(jo, columnInfoKeyCanSort, value.canSort);
            serialize_field(jo, columnInfoKeyCanFilter, value.canFilter);
            serialize_field(jo, columnInfoKeyAlignment, value.alignment);
            serialize_field(jo, columnInfoKeyType, value.type);
            serialize_field(jo, columnInfoKeyTypeOptions, value.typeOptions);
            serialize_field(jo, columnInfoKeyWidth, value.width);
            serialize_field(jo, columnInfoKeyShowByDefault, value.showByDefault);
            serialize_field(jo, columnInfoKeyLinkKey, value.linkKey);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    ColumnInfoDto ColumnInfoDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<ColumnInfoDto>(json);
    }

    ColumnInfoDto::ColumnInfoDto(
        QString key,
        std::optional<QString> header,
        bool canSort,
        bool canFilter,
        QString alignment,
        QString type,
        std::optional<std::vector<ColumnTypeOptionDto>> typeOptions,
        qint32 width,
        bool showByDefault,
        std::optional<QString> linkKey
    ) :
        key(std::move(key)),
        header(std::move(header)),
        canSort(std::move(canSort)),
        canFilter(std::move(canFilter)),
        alignment(std::move(alignment)),
        type(std::move(type)),
        typeOptions(std::move(typeOptions)),
        width(std::move(width)),
        showByDefault(std::move(showByDefault)),
        linkKey(std::move(linkKey))
    { }

    ColumnInfoDto::ColumnInfoDto(
        QString key,
        std::optional<QString> header,
        bool canSort,
        bool canFilter,
        TableCellAlignment alignment,
        QString type,
        std::optional<std::vector<ColumnTypeOptionDto>> typeOptions,
        qint32 width,
        bool showByDefault,
        std::optional<QString> linkKey
    ) : ColumnInfoDto(
        std::move(key),
        std::move(header),
        std::move(canSort),
        std::move(canFilter),
        TableCellAlignmentMeta::enumToStr(alignment),
        std::move(type),
        std::move(typeOptions),
        std::move(width),
        std::move(showByDefault),
        std::move(linkKey))
    { }

    // throws std::range_error
    TableCellAlignment ColumnInfoDto::getAlignmentEnum() const
    {
        return TableCellAlignmentMeta::strToEnum(alignment);
    }

    void ColumnInfoDto::setAlignmentEnum(TableCellAlignment newValue)
    {
        alignment = TableCellAlignmentMeta::enumToStr(newValue);
    }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray ColumnInfoDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // DashboardInfoDto

    static const QLatin1String dashboardInfoKeyMainUrl{"mainUrl"};
    static const QLatin1String dashboardInfoKeyDashboardVersion{"dashboardVersion"};
    static const QLatin1String dashboardInfoKeyDashboardVersionNumber{"dashboardVersionNumber"};
    static const QLatin1String dashboardInfoKeyDashboardBuildDate{"dashboardBuildDate"};
    static const QLatin1String dashboardInfoKeyUsername{"username"};
    static const QLatin1String dashboardInfoKeyCsrfTokenHeader{"csrfTokenHeader"};
    static const QLatin1String dashboardInfoKeyCsrfToken{"csrfToken"};
    static const QLatin1String dashboardInfoKeyCheckCredentialsUrl{"checkCredentialsUrl"};
    static const QLatin1String dashboardInfoKeyNamedFiltersUrl{"namedFiltersUrl"};
    static const QLatin1String dashboardInfoKeyProjects{"projects"};
    static const QLatin1String dashboardInfoKeyUserApiTokenUrl{"userApiTokenUrl"};
    static const QLatin1String dashboardInfoKeyUserNamedFiltersUrl{"userNamedFiltersUrl"};
    static const QLatin1String dashboardInfoKeySupportAddress{"supportAddress"};
    static const QLatin1String dashboardInfoKeyIssueFilterHelp{"issueFilterHelp"};
    static const QLatin1String dashboardInfoKeyCsrfTokenUrl{"csrfTokenUrl"};

    template<>
    class de_serializer<DashboardInfoDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static DashboardInfoDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<DashboardInfoDto>(json);
            return {
                deserialize_field<std::optional<QString>>(jo, dashboardInfoKeyMainUrl),
                deserialize_field<QString>(jo, dashboardInfoKeyDashboardVersion),
                deserialize_field<std::optional<QString>>(jo, dashboardInfoKeyDashboardVersionNumber),
                deserialize_field<QString>(jo, dashboardInfoKeyDashboardBuildDate),
                deserialize_field<std::optional<QString>>(jo, dashboardInfoKeyUsername),
                deserialize_field<std::optional<QString>>(jo, dashboardInfoKeyCsrfTokenHeader),
                deserialize_field<QString>(jo, dashboardInfoKeyCsrfToken),
                deserialize_field<std::optional<QString>>(jo, dashboardInfoKeyCheckCredentialsUrl),
                deserialize_field<std::optional<QString>>(jo, dashboardInfoKeyNamedFiltersUrl),
                deserialize_field<std::optional<std::vector<ProjectReferenceDto>>>(jo, dashboardInfoKeyProjects),
                deserialize_field<std::optional<QString>>(jo, dashboardInfoKeyUserApiTokenUrl),
                deserialize_field<std::optional<QString>>(jo, dashboardInfoKeyUserNamedFiltersUrl),
                deserialize_field<std::optional<QString>>(jo, dashboardInfoKeySupportAddress),
                deserialize_field<std::optional<QString>>(jo, dashboardInfoKeyIssueFilterHelp),
                deserialize_field<std::optional<QString>>(jo, dashboardInfoKeyCsrfTokenUrl)
            };
        }

        static QJsonValue serialize(const DashboardInfoDto &value) {
            QJsonObject jo;
            serialize_field(jo, dashboardInfoKeyMainUrl, value.mainUrl);
            serialize_field(jo, dashboardInfoKeyDashboardVersion, value.dashboardVersion);
            serialize_field(jo, dashboardInfoKeyDashboardVersionNumber, value.dashboardVersionNumber);
            serialize_field(jo, dashboardInfoKeyDashboardBuildDate, value.dashboardBuildDate);
            serialize_field(jo, dashboardInfoKeyUsername, value.username);
            serialize_field(jo, dashboardInfoKeyCsrfTokenHeader, value.csrfTokenHeader);
            serialize_field(jo, dashboardInfoKeyCsrfToken, value.csrfToken);
            serialize_field(jo, dashboardInfoKeyCheckCredentialsUrl, value.checkCredentialsUrl);
            serialize_field(jo, dashboardInfoKeyNamedFiltersUrl, value.namedFiltersUrl);
            serialize_field(jo, dashboardInfoKeyProjects, value.projects);
            serialize_field(jo, dashboardInfoKeyUserApiTokenUrl, value.userApiTokenUrl);
            serialize_field(jo, dashboardInfoKeyUserNamedFiltersUrl, value.userNamedFiltersUrl);
            serialize_field(jo, dashboardInfoKeySupportAddress, value.supportAddress);
            serialize_field(jo, dashboardInfoKeyIssueFilterHelp, value.issueFilterHelp);
            serialize_field(jo, dashboardInfoKeyCsrfTokenUrl, value.csrfTokenUrl);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    DashboardInfoDto DashboardInfoDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<DashboardInfoDto>(json);
    }

    DashboardInfoDto::DashboardInfoDto(
        std::optional<QString> mainUrl,
        QString dashboardVersion,
        std::optional<QString> dashboardVersionNumber,
        QString dashboardBuildDate,
        std::optional<QString> username,
        std::optional<QString> csrfTokenHeader,
        QString csrfToken,
        std::optional<QString> checkCredentialsUrl,
        std::optional<QString> namedFiltersUrl,
        std::optional<std::vector<ProjectReferenceDto>> projects,
        std::optional<QString> userApiTokenUrl,
        std::optional<QString> userNamedFiltersUrl,
        std::optional<QString> supportAddress,
        std::optional<QString> issueFilterHelp,
        std::optional<QString> csrfTokenUrl
    ) :
        mainUrl(std::move(mainUrl)),
        dashboardVersion(std::move(dashboardVersion)),
        dashboardVersionNumber(std::move(dashboardVersionNumber)),
        dashboardBuildDate(std::move(dashboardBuildDate)),
        username(std::move(username)),
        csrfTokenHeader(std::move(csrfTokenHeader)),
        csrfToken(std::move(csrfToken)),
        checkCredentialsUrl(std::move(checkCredentialsUrl)),
        namedFiltersUrl(std::move(namedFiltersUrl)),
        projects(std::move(projects)),
        userApiTokenUrl(std::move(userApiTokenUrl)),
        userNamedFiltersUrl(std::move(userNamedFiltersUrl)),
        supportAddress(std::move(supportAddress)),
        issueFilterHelp(std::move(issueFilterHelp)),
        csrfTokenUrl(std::move(csrfTokenUrl))
    { }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray DashboardInfoDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // IssueCommentListDto

    static const QLatin1String issueCommentListKeyComments{"comments"};

    template<>
    class de_serializer<IssueCommentListDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static IssueCommentListDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<IssueCommentListDto>(json);
            return {
                deserialize_field<std::vector<IssueCommentDto>>(jo, issueCommentListKeyComments)
            };
        }

        static QJsonValue serialize(const IssueCommentListDto &value) {
            QJsonObject jo;
            serialize_field(jo, issueCommentListKeyComments, value.comments);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    IssueCommentListDto IssueCommentListDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<IssueCommentListDto>(json);
    }

    IssueCommentListDto::IssueCommentListDto(
        std::vector<IssueCommentDto> comments
    ) :
        comments(std::move(comments))
    { }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray IssueCommentListDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // IssueKindInfoDto

    static const QLatin1String issueKindInfoKeyPrefix{"prefix"};
    static const QLatin1String issueKindInfoKeyNiceSingularName{"niceSingularName"};
    static const QLatin1String issueKindInfoKeyNicePluralName{"nicePluralName"};

    template<>
    class de_serializer<IssueKindInfoDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static IssueKindInfoDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<IssueKindInfoDto>(json);
            return {
                deserialize_field<QString>(jo, issueKindInfoKeyPrefix),
                deserialize_field<QString>(jo, issueKindInfoKeyNiceSingularName),
                deserialize_field<QString>(jo, issueKindInfoKeyNicePluralName)
            };
        }

        static QJsonValue serialize(const IssueKindInfoDto &value) {
            QJsonObject jo;
            serialize_field(jo, issueKindInfoKeyPrefix, value.prefix);
            serialize_field(jo, issueKindInfoKeyNiceSingularName, value.niceSingularName);
            serialize_field(jo, issueKindInfoKeyNicePluralName, value.nicePluralName);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    IssueKindInfoDto IssueKindInfoDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<IssueKindInfoDto>(json);
    }

    IssueKindInfoDto::IssueKindInfoDto(
        QString prefix,
        QString niceSingularName,
        QString nicePluralName
    ) :
        prefix(std::move(prefix)),
        niceSingularName(std::move(niceSingularName)),
        nicePluralName(std::move(nicePluralName))
    { }

    IssueKindInfoDto::IssueKindInfoDto(
        IssueKind prefix,
        QString niceSingularName,
        QString nicePluralName
    ) : IssueKindInfoDto(
        IssueKindMeta::enumToStr(prefix),
        std::move(niceSingularName),
        std::move(nicePluralName))
    { }

    // throws std::range_error
    IssueKind IssueKindInfoDto::getPrefixEnum() const
    {
        return IssueKindMeta::strToEnum(prefix);
    }

    void IssueKindInfoDto::setPrefixEnum(IssueKind newValue)
    {
        prefix = IssueKindMeta::enumToStr(newValue);
    }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray IssueKindInfoDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // IssueTagTypeListDto

    static const QLatin1String issueTagTypeListKeyTags{"tags"};

    template<>
    class de_serializer<IssueTagTypeListDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static IssueTagTypeListDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<IssueTagTypeListDto>(json);
            return {
                deserialize_field<std::vector<IssueTagTypeDto>>(jo, issueTagTypeListKeyTags)
            };
        }

        static QJsonValue serialize(const IssueTagTypeListDto &value) {
            QJsonObject jo;
            serialize_field(jo, issueTagTypeListKeyTags, value.tags);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    IssueTagTypeListDto IssueTagTypeListDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<IssueTagTypeListDto>(json);
    }

    IssueTagTypeListDto::IssueTagTypeListDto(
        std::vector<IssueTagTypeDto> tags
    ) :
        tags(std::move(tags))
    { }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray IssueTagTypeListDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // LineMarkerDto

    static const QLatin1String lineMarkerKeyKind{"kind"};
    static const QLatin1String lineMarkerKeyId{"id"};
    static const QLatin1String lineMarkerKeyStartLine{"startLine"};
    static const QLatin1String lineMarkerKeyStartColumn{"startColumn"};
    static const QLatin1String lineMarkerKeyEndLine{"endLine"};
    static const QLatin1String lineMarkerKeyEndColumn{"endColumn"};
    static const QLatin1String lineMarkerKeyDescription{"description"};
    static const QLatin1String lineMarkerKeyIssueUrl{"issueUrl"};
    static const QLatin1String lineMarkerKeyIsNew{"isNew"};

    template<>
    class de_serializer<LineMarkerDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static LineMarkerDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<LineMarkerDto>(json);
            return {
                deserialize_field<QString>(jo, lineMarkerKeyKind),
                deserialize_field<std::optional<qint64>>(jo, lineMarkerKeyId),
                deserialize_field<qint32>(jo, lineMarkerKeyStartLine),
                deserialize_field<qint32>(jo, lineMarkerKeyStartColumn),
                deserialize_field<qint32>(jo, lineMarkerKeyEndLine),
                deserialize_field<qint32>(jo, lineMarkerKeyEndColumn),
                deserialize_field<QString>(jo, lineMarkerKeyDescription),
                deserialize_field<std::optional<QString>>(jo, lineMarkerKeyIssueUrl),
                deserialize_field<std::optional<bool>>(jo, lineMarkerKeyIsNew)
            };
        }

        static QJsonValue serialize(const LineMarkerDto &value) {
            QJsonObject jo;
            serialize_field(jo, lineMarkerKeyKind, value.kind);
            serialize_field(jo, lineMarkerKeyId, value.id);
            serialize_field(jo, lineMarkerKeyStartLine, value.startLine);
            serialize_field(jo, lineMarkerKeyStartColumn, value.startColumn);
            serialize_field(jo, lineMarkerKeyEndLine, value.endLine);
            serialize_field(jo, lineMarkerKeyEndColumn, value.endColumn);
            serialize_field(jo, lineMarkerKeyDescription, value.description);
            serialize_field(jo, lineMarkerKeyIssueUrl, value.issueUrl);
            serialize_field(jo, lineMarkerKeyIsNew, value.isNew);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    LineMarkerDto LineMarkerDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<LineMarkerDto>(json);
    }

    LineMarkerDto::LineMarkerDto(
        QString kind,
        std::optional<qint64> id,
        qint32 startLine,
        qint32 startColumn,
        qint32 endLine,
        qint32 endColumn,
        QString description,
        std::optional<QString> issueUrl,
        std::optional<bool> isNew
    ) :
        kind(std::move(kind)),
        id(std::move(id)),
        startLine(std::move(startLine)),
        startColumn(std::move(startColumn)),
        endLine(std::move(endLine)),
        endColumn(std::move(endColumn)),
        description(std::move(description)),
        issueUrl(std::move(issueUrl)),
        isNew(std::move(isNew))
    { }

    LineMarkerDto::LineMarkerDto(
        IssueKind kind,
        std::optional<qint64> id,
        qint32 startLine,
        qint32 startColumn,
        qint32 endLine,
        qint32 endColumn,
        QString description,
        std::optional<QString> issueUrl,
        std::optional<bool> isNew
    ) : LineMarkerDto(
        IssueKindMeta::enumToStr(kind),
        std::move(id),
        std::move(startLine),
        std::move(startColumn),
        std::move(endLine),
        std::move(endColumn),
        std::move(description),
        std::move(issueUrl),
        std::move(isNew))
    { }

    // throws std::range_error
    IssueKind LineMarkerDto::getKindEnum() const
    {
        return IssueKindMeta::strToEnum(kind);
    }

    void LineMarkerDto::setKindEnum(IssueKind newValue)
    {
        kind = IssueKindMeta::enumToStr(newValue);
    }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray LineMarkerDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // RepositoryUpdateMessageDto

    static const QLatin1String repositoryUpdateMessageKeySeverity{"severity"};
    static const QLatin1String repositoryUpdateMessageKeyMessage{"message"};

    template<>
    class de_serializer<RepositoryUpdateMessageDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static RepositoryUpdateMessageDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<RepositoryUpdateMessageDto>(json);
            return {
                deserialize_field<std::optional<QString>>(jo, repositoryUpdateMessageKeySeverity),
                deserialize_field<std::optional<QString>>(jo, repositoryUpdateMessageKeyMessage)
            };
        }

        static QJsonValue serialize(const RepositoryUpdateMessageDto &value) {
            QJsonObject jo;
            serialize_field(jo, repositoryUpdateMessageKeySeverity, value.severity);
            serialize_field(jo, repositoryUpdateMessageKeyMessage, value.message);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    RepositoryUpdateMessageDto RepositoryUpdateMessageDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<RepositoryUpdateMessageDto>(json);
    }

    RepositoryUpdateMessageDto::RepositoryUpdateMessageDto(
        std::optional<QString> severity,
        std::optional<QString> message
    ) :
        severity(std::move(severity)),
        message(std::move(message))
    { }

    RepositoryUpdateMessageDto::RepositoryUpdateMessageDto(
        std::optional<MessageSeverity> severity,
        std::optional<QString> message
    ) : RepositoryUpdateMessageDto(
        optionalTransform<QString, MessageSeverity>(severity, MessageSeverityMeta::enumToStr),
        std::move(message))
    { }

    // throws std::range_error
    std::optional<MessageSeverity> RepositoryUpdateMessageDto::getSeverityEnum() const
    {
        return optionalTransform<MessageSeverity, QString>(severity, MessageSeverityMeta::strToEnum);
    }

    void RepositoryUpdateMessageDto::setSeverityEnum(std::optional<MessageSeverity> newValue)
    {
        severity = optionalTransform<QString, MessageSeverity>(newValue, MessageSeverityMeta::enumToStr);
    }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray RepositoryUpdateMessageDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // RuleListDto

    static const QLatin1String ruleListKeyRules{"rules"};

    template<>
    class de_serializer<RuleListDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static RuleListDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<RuleListDto>(json);
            return {
                deserialize_field<std::vector<RuleDto>>(jo, ruleListKeyRules)
            };
        }

        static QJsonValue serialize(const RuleListDto &value) {
            QJsonObject jo;
            serialize_field(jo, ruleListKeyRules, value.rules);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    RuleListDto RuleListDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<RuleListDto>(json);
    }

    RuleListDto::RuleListDto(
        std::vector<RuleDto> rules
    ) :
        rules(std::move(rules))
    { }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray RuleListDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // SortInfoDto

    static const QLatin1String sortInfoKeyKey{"key"};
    static const QLatin1String sortInfoKeyDirection{"direction"};

    template<>
    class de_serializer<SortInfoDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static SortInfoDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<SortInfoDto>(json);
            return {
                deserialize_field<QString>(jo, sortInfoKeyKey),
                deserialize_field<QString>(jo, sortInfoKeyDirection)
            };
        }

        static QJsonValue serialize(const SortInfoDto &value) {
            QJsonObject jo;
            serialize_field(jo, sortInfoKeyKey, value.key);
            serialize_field(jo, sortInfoKeyDirection, value.direction);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    SortInfoDto SortInfoDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<SortInfoDto>(json);
    }

    SortInfoDto::SortInfoDto(
        QString key,
        QString direction
    ) :
        key(std::move(key)),
        direction(std::move(direction))
    { }

    SortInfoDto::SortInfoDto(
        QString key,
        SortDirection direction
    ) : SortInfoDto(
        std::move(key),
        SortDirectionMeta::enumToStr(direction))
    { }

    // throws std::range_error
    SortDirection SortInfoDto::getDirectionEnum() const
    {
        return SortDirectionMeta::strToEnum(direction);
    }

    void SortInfoDto::setDirectionEnum(SortDirection newValue)
    {
        direction = SortDirectionMeta::enumToStr(newValue);
    }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray SortInfoDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // UserRefDto

    static const QLatin1String userRefKeyName{"name"};
    static const QLatin1String userRefKeyDisplayName{"displayName"};
    static const QLatin1String userRefKeyType{"type"};
    static const QLatin1String userRefKeyIsPublic{"isPublic"};

    template<>
    class de_serializer<UserRefDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static UserRefDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<UserRefDto>(json);
            return {
                deserialize_field<QString>(jo, userRefKeyName),
                deserialize_field<QString>(jo, userRefKeyDisplayName),
                deserialize_field<std::optional<QString>>(jo, userRefKeyType),
                deserialize_field<std::optional<bool>>(jo, userRefKeyIsPublic)
            };
        }

        static QJsonValue serialize(const UserRefDto &value) {
            QJsonObject jo;
            serialize_field(jo, userRefKeyName, value.name);
            serialize_field(jo, userRefKeyDisplayName, value.displayName);
            serialize_field(jo, userRefKeyType, value.type);
            serialize_field(jo, userRefKeyIsPublic, value.isPublic);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    UserRefDto UserRefDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<UserRefDto>(json);
    }

    UserRefDto::UserRefDto(
        QString name,
        QString displayName,
        std::optional<QString> type,
        std::optional<bool> isPublic
    ) :
        name(std::move(name)),
        displayName(std::move(displayName)),
        type(std::move(type)),
        isPublic(std::move(isPublic))
    { }

    UserRefDto::UserRefDto(
        QString name,
        QString displayName,
        std::optional<UserRefType> type,
        std::optional<bool> isPublic
    ) : UserRefDto(
        std::move(name),
        std::move(displayName),
        optionalTransform<QString, UserRefType>(type, UserRefTypeMeta::enumToStr),
        std::move(isPublic))
    { }

    // throws std::range_error
    std::optional<UserRefType> UserRefDto::getTypeEnum() const
    {
        return optionalTransform<UserRefType, QString>(type, UserRefTypeMeta::strToEnum);
    }

    void UserRefDto::setTypeEnum(std::optional<UserRefType> newValue)
    {
        type = optionalTransform<QString, UserRefType>(newValue, UserRefTypeMeta::enumToStr);
    }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray UserRefDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // AnalyzedFileListDto

    static const QLatin1String analyzedFileListKeyVersion{"version"};
    static const QLatin1String analyzedFileListKeyRows{"rows"};

    template<>
    class de_serializer<AnalyzedFileListDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static AnalyzedFileListDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<AnalyzedFileListDto>(json);
            return {
                deserialize_field<AnalysisVersionDto>(jo, analyzedFileListKeyVersion),
                deserialize_field<std::vector<AnalyzedFileDto>>(jo, analyzedFileListKeyRows)
            };
        }

        static QJsonValue serialize(const AnalyzedFileListDto &value) {
            QJsonObject jo;
            serialize_field(jo, analyzedFileListKeyVersion, value.version);
            serialize_field(jo, analyzedFileListKeyRows, value.rows);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    AnalyzedFileListDto AnalyzedFileListDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<AnalyzedFileListDto>(json);
    }

    AnalyzedFileListDto::AnalyzedFileListDto(
        AnalysisVersionDto version,
        std::vector<AnalyzedFileDto> rows
    ) :
        version(std::move(version)),
        rows(std::move(rows))
    { }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray AnalyzedFileListDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // EntityListDto

    static const QLatin1String entityListKeyVersion{"version"};
    static const QLatin1String entityListKeyEntities{"entities"};

    template<>
    class de_serializer<EntityListDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static EntityListDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<EntityListDto>(json);
            return {
                deserialize_field<std::optional<AnalysisVersionDto>>(jo, entityListKeyVersion),
                deserialize_field<std::vector<EntityDto>>(jo, entityListKeyEntities)
            };
        }

        static QJsonValue serialize(const EntityListDto &value) {
            QJsonObject jo;
            serialize_field(jo, entityListKeyVersion, value.version);
            serialize_field(jo, entityListKeyEntities, value.entities);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    EntityListDto EntityListDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<EntityListDto>(json);
    }

    EntityListDto::EntityListDto(
        std::optional<AnalysisVersionDto> version,
        std::vector<EntityDto> entities
    ) :
        version(std::move(version)),
        entities(std::move(entities))
    { }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray EntityListDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // FileViewDto

    static const QLatin1String fileViewKeyFileName{"fileName"};
    static const QLatin1String fileViewKeyVersion{"version"};
    static const QLatin1String fileViewKeySourceCodeUrl{"sourceCodeUrl"};
    static const QLatin1String fileViewKeyLineMarkers{"lineMarkers"};

    template<>
    class de_serializer<FileViewDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static FileViewDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<FileViewDto>(json);
            return {
                deserialize_field<QString>(jo, fileViewKeyFileName),
                deserialize_field<std::optional<QString>>(jo, fileViewKeyVersion),
                deserialize_field<std::optional<QString>>(jo, fileViewKeySourceCodeUrl),
                deserialize_field<std::vector<LineMarkerDto>>(jo, fileViewKeyLineMarkers)
            };
        }

        static QJsonValue serialize(const FileViewDto &value) {
            QJsonObject jo;
            serialize_field(jo, fileViewKeyFileName, value.fileName);
            serialize_field(jo, fileViewKeyVersion, value.version);
            serialize_field(jo, fileViewKeySourceCodeUrl, value.sourceCodeUrl);
            serialize_field(jo, fileViewKeyLineMarkers, value.lineMarkers);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    FileViewDto FileViewDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<FileViewDto>(json);
    }

    FileViewDto::FileViewDto(
        QString fileName,
        std::optional<QString> version,
        std::optional<QString> sourceCodeUrl,
        std::vector<LineMarkerDto> lineMarkers
    ) :
        fileName(std::move(fileName)),
        version(std::move(version)),
        sourceCodeUrl(std::move(sourceCodeUrl)),
        lineMarkers(std::move(lineMarkers))
    { }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray FileViewDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // IssueDto

    static const QLatin1String issueKeyKind{"kind"};
    static const QLatin1String issueKeyId{"id"};
    static const QLatin1String issueKeyParentProject{"parentProject"};
    static const QLatin1String issueKeySourceLocations{"sourceLocations"};
    static const QLatin1String issueKeyIssueKind{"issueKind"};
    static const QLatin1String issueKeyIsHidden{"isHidden"};
    static const QLatin1String issueKeyIssueViewUrl{"issueViewUrl"};

    template<>
    class de_serializer<IssueDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static IssueDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<IssueDto>(json);
            return {
                deserialize_field<QString>(jo, issueKeyKind),
                deserialize_field<qint64>(jo, issueKeyId),
                deserialize_field<ProjectReferenceDto>(jo, issueKeyParentProject),
                deserialize_field<std::vector<IssueSourceLocationDto>>(jo, issueKeySourceLocations),
                deserialize_field<IssueKindInfoDto>(jo, issueKeyIssueKind),
                deserialize_field<bool>(jo, issueKeyIsHidden),
                deserialize_field<std::optional<QString>>(jo, issueKeyIssueViewUrl)
            };
        }

        static QJsonValue serialize(const IssueDto &value) {
            QJsonObject jo;
            serialize_field(jo, issueKeyKind, value.kind);
            serialize_field(jo, issueKeyId, value.id);
            serialize_field(jo, issueKeyParentProject, value.parentProject);
            serialize_field(jo, issueKeySourceLocations, value.sourceLocations);
            serialize_field(jo, issueKeyIssueKind, value.issueKind);
            serialize_field(jo, issueKeyIsHidden, value.isHidden);
            serialize_field(jo, issueKeyIssueViewUrl, value.issueViewUrl);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    IssueDto IssueDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<IssueDto>(json);
    }

    IssueDto::IssueDto(
        QString kind,
        qint64 id,
        ProjectReferenceDto parentProject,
        std::vector<IssueSourceLocationDto> sourceLocations,
        IssueKindInfoDto issueKind,
        bool isHidden,
        std::optional<QString> issueViewUrl
    ) :
        kind(std::move(kind)),
        id(std::move(id)),
        parentProject(std::move(parentProject)),
        sourceLocations(std::move(sourceLocations)),
        issueKind(std::move(issueKind)),
        isHidden(std::move(isHidden)),
        issueViewUrl(std::move(issueViewUrl))
    { }

    IssueDto::IssueDto(
        IssueKind kind,
        qint64 id,
        ProjectReferenceDto parentProject,
        std::vector<IssueSourceLocationDto> sourceLocations,
        IssueKindInfoDto issueKind,
        bool isHidden,
        std::optional<QString> issueViewUrl
    ) : IssueDto(
        IssueKindMeta::enumToStr(kind),
        std::move(id),
        std::move(parentProject),
        std::move(sourceLocations),
        std::move(issueKind),
        std::move(isHidden),
        std::move(issueViewUrl))
    { }

    // throws std::range_error
    IssueKind IssueDto::getKindEnum() const
    {
        return IssueKindMeta::strToEnum(kind);
    }

    void IssueDto::setKindEnum(IssueKind newValue)
    {
        kind = IssueKindMeta::enumToStr(newValue);
    }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray IssueDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // IssueTableDto

    static const QLatin1String issueTableKeyStartVersion{"startVersion"};
    static const QLatin1String issueTableKeyEndVersion{"endVersion"};
    static const QLatin1String issueTableKeyTableViewUrl{"tableViewUrl"};
    static const QLatin1String issueTableKeyColumns{"columns"};
    static const QLatin1String issueTableKeyRows{"rows"};
    static const QLatin1String issueTableKeyTotalRowCount{"totalRowCount"};
    static const QLatin1String issueTableKeyTotalAddedCount{"totalAddedCount"};
    static const QLatin1String issueTableKeyTotalRemovedCount{"totalRemovedCount"};

    template<>
    class de_serializer<IssueTableDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static IssueTableDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<IssueTableDto>(json);
            return {
                deserialize_field<std::optional<AnalysisVersionDto>>(jo, issueTableKeyStartVersion),
                deserialize_field<AnalysisVersionDto>(jo, issueTableKeyEndVersion),
                deserialize_field<std::optional<QString>>(jo, issueTableKeyTableViewUrl),
                deserialize_field<std::optional<std::vector<ColumnInfoDto>>>(jo, issueTableKeyColumns),
                deserialize_field<std::vector<std::map<QString, Any>>>(jo, issueTableKeyRows),
                deserialize_field<std::optional<qint32>>(jo, issueTableKeyTotalRowCount),
                deserialize_field<std::optional<qint32>>(jo, issueTableKeyTotalAddedCount),
                deserialize_field<std::optional<qint32>>(jo, issueTableKeyTotalRemovedCount)
            };
        }

        static QJsonValue serialize(const IssueTableDto &value) {
            QJsonObject jo;
            serialize_field(jo, issueTableKeyStartVersion, value.startVersion);
            serialize_field(jo, issueTableKeyEndVersion, value.endVersion);
            serialize_field(jo, issueTableKeyTableViewUrl, value.tableViewUrl);
            serialize_field(jo, issueTableKeyColumns, value.columns);
            serialize_field(jo, issueTableKeyRows, value.rows);
            serialize_field(jo, issueTableKeyTotalRowCount, value.totalRowCount);
            serialize_field(jo, issueTableKeyTotalAddedCount, value.totalAddedCount);
            serialize_field(jo, issueTableKeyTotalRemovedCount, value.totalRemovedCount);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    IssueTableDto IssueTableDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<IssueTableDto>(json);
    }

    IssueTableDto::IssueTableDto(
        std::optional<AnalysisVersionDto> startVersion,
        AnalysisVersionDto endVersion,
        std::optional<QString> tableViewUrl,
        std::optional<std::vector<ColumnInfoDto>> columns,
        std::vector<std::map<QString, Any>> rows,
        std::optional<qint32> totalRowCount,
        std::optional<qint32> totalAddedCount,
        std::optional<qint32> totalRemovedCount
    ) :
        startVersion(std::move(startVersion)),
        endVersion(std::move(endVersion)),
        tableViewUrl(std::move(tableViewUrl)),
        columns(std::move(columns)),
        rows(std::move(rows)),
        totalRowCount(std::move(totalRowCount)),
        totalAddedCount(std::move(totalAddedCount)),
        totalRemovedCount(std::move(totalRemovedCount))
    { }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray IssueTableDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // MetricListDto

    static const QLatin1String metricListKeyVersion{"version"};
    static const QLatin1String metricListKeyMetrics{"metrics"};

    template<>
    class de_serializer<MetricListDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static MetricListDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<MetricListDto>(json);
            return {
                deserialize_field<std::optional<AnalysisVersionDto>>(jo, metricListKeyVersion),
                deserialize_field<std::vector<MetricDto>>(jo, metricListKeyMetrics)
            };
        }

        static QJsonValue serialize(const MetricListDto &value) {
            QJsonObject jo;
            serialize_field(jo, metricListKeyVersion, value.version);
            serialize_field(jo, metricListKeyMetrics, value.metrics);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    MetricListDto MetricListDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<MetricListDto>(json);
    }

    MetricListDto::MetricListDto(
        std::optional<AnalysisVersionDto> version,
        std::vector<MetricDto> metrics
    ) :
        version(std::move(version)),
        metrics(std::move(metrics))
    { }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray MetricListDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // MetricValueRangeDto

    static const QLatin1String metricValueRangeKeyStartVersion{"startVersion"};
    static const QLatin1String metricValueRangeKeyEndVersion{"endVersion"};
    static const QLatin1String metricValueRangeKeyEntity{"entity"};
    static const QLatin1String metricValueRangeKeyMetric{"metric"};
    static const QLatin1String metricValueRangeKeyValues{"values"};

    template<>
    class de_serializer<MetricValueRangeDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static MetricValueRangeDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<MetricValueRangeDto>(json);
            return {
                deserialize_field<AnalysisVersionDto>(jo, metricValueRangeKeyStartVersion),
                deserialize_field<AnalysisVersionDto>(jo, metricValueRangeKeyEndVersion),
                deserialize_field<QString>(jo, metricValueRangeKeyEntity),
                deserialize_field<QString>(jo, metricValueRangeKeyMetric),
                deserialize_field<std::vector<std::optional<double>>>(jo, metricValueRangeKeyValues)
            };
        }

        static QJsonValue serialize(const MetricValueRangeDto &value) {
            QJsonObject jo;
            serialize_field(jo, metricValueRangeKeyStartVersion, value.startVersion);
            serialize_field(jo, metricValueRangeKeyEndVersion, value.endVersion);
            serialize_field(jo, metricValueRangeKeyEntity, value.entity);
            serialize_field(jo, metricValueRangeKeyMetric, value.metric);
            serialize_field(jo, metricValueRangeKeyValues, value.values);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    MetricValueRangeDto MetricValueRangeDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<MetricValueRangeDto>(json);
    }

    MetricValueRangeDto::MetricValueRangeDto(
        AnalysisVersionDto startVersion,
        AnalysisVersionDto endVersion,
        QString entity,
        QString metric,
        std::vector<std::optional<double>> values
    ) :
        startVersion(std::move(startVersion)),
        endVersion(std::move(endVersion)),
        entity(std::move(entity)),
        metric(std::move(metric)),
        values(std::move(values))
    { }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray MetricValueRangeDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // MetricValueTableDto

    static const QLatin1String metricValueTableKeyColumns{"columns"};
    static const QLatin1String metricValueTableKeyRows{"rows"};

    template<>
    class de_serializer<MetricValueTableDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static MetricValueTableDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<MetricValueTableDto>(json);
            return {
                deserialize_field<std::vector<ColumnInfoDto>>(jo, metricValueTableKeyColumns),
                deserialize_field<std::vector<MetricValueTableRowDto>>(jo, metricValueTableKeyRows)
            };
        }

        static QJsonValue serialize(const MetricValueTableDto &value) {
            QJsonObject jo;
            serialize_field(jo, metricValueTableKeyColumns, value.columns);
            serialize_field(jo, metricValueTableKeyRows, value.rows);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    MetricValueTableDto MetricValueTableDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<MetricValueTableDto>(json);
    }

    MetricValueTableDto::MetricValueTableDto(
        std::vector<ColumnInfoDto> columns,
        std::vector<MetricValueTableRowDto> rows
    ) :
        columns(std::move(columns)),
        rows(std::move(rows))
    { }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray MetricValueTableDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // NamedFilterCreateDto

    static const QLatin1String namedFilterCreateKeyDisplayName{"displayName"};
    static const QLatin1String namedFilterCreateKeyKind{"kind"};
    static const QLatin1String namedFilterCreateKeyFilters{"filters"};
    static const QLatin1String namedFilterCreateKeySorters{"sorters"};
    static const QLatin1String namedFilterCreateKeyVisibility{"visibility"};

    template<>
    class de_serializer<NamedFilterCreateDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static NamedFilterCreateDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<NamedFilterCreateDto>(json);
            return {
                deserialize_field<QString>(jo, namedFilterCreateKeyDisplayName),
                deserialize_field<QString>(jo, namedFilterCreateKeyKind),
                deserialize_field<std::map<QString, QString>>(jo, namedFilterCreateKeyFilters),
                deserialize_field<std::vector<SortInfoDto>>(jo, namedFilterCreateKeySorters),
                deserialize_field<std::optional<NamedFilterVisibilityDto>>(jo, namedFilterCreateKeyVisibility)
            };
        }

        static QJsonValue serialize(const NamedFilterCreateDto &value) {
            QJsonObject jo;
            serialize_field(jo, namedFilterCreateKeyDisplayName, value.displayName);
            serialize_field(jo, namedFilterCreateKeyKind, value.kind);
            serialize_field(jo, namedFilterCreateKeyFilters, value.filters);
            serialize_field(jo, namedFilterCreateKeySorters, value.sorters);
            serialize_field(jo, namedFilterCreateKeyVisibility, value.visibility);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    NamedFilterCreateDto NamedFilterCreateDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<NamedFilterCreateDto>(json);
    }

    NamedFilterCreateDto::NamedFilterCreateDto(
        QString displayName,
        QString kind,
        std::map<QString, QString> filters,
        std::vector<SortInfoDto> sorters,
        std::optional<NamedFilterVisibilityDto> visibility
    ) :
        displayName(std::move(displayName)),
        kind(std::move(kind)),
        filters(std::move(filters)),
        sorters(std::move(sorters)),
        visibility(std::move(visibility))
    { }

    NamedFilterCreateDto::NamedFilterCreateDto(
        QString displayName,
        IssueKindForNamedFilterCreation kind,
        std::map<QString, QString> filters,
        std::vector<SortInfoDto> sorters,
        std::optional<NamedFilterVisibilityDto> visibility
    ) : NamedFilterCreateDto(
        std::move(displayName),
        IssueKindForNamedFilterCreationMeta::enumToStr(kind),
        std::move(filters),
        std::move(sorters),
        std::move(visibility))
    { }

    // throws std::range_error
    IssueKindForNamedFilterCreation NamedFilterCreateDto::getKindEnum() const
    {
        return IssueKindForNamedFilterCreationMeta::strToEnum(kind);
    }

    void NamedFilterCreateDto::setKindEnum(IssueKindForNamedFilterCreation newValue)
    {
        kind = IssueKindForNamedFilterCreationMeta::enumToStr(newValue);
    }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray NamedFilterCreateDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // NamedFilterInfoDto

    static const QLatin1String namedFilterInfoKeyKey{"key"};
    static const QLatin1String namedFilterInfoKeyDisplayName{"displayName"};
    static const QLatin1String namedFilterInfoKeyUrl{"url"};
    static const QLatin1String namedFilterInfoKeyIsPredefined{"isPredefined"};
    static const QLatin1String namedFilterInfoKeyType{"type"};
    static const QLatin1String namedFilterInfoKeyCanWrite{"canWrite"};
    static const QLatin1String namedFilterInfoKeyFilters{"filters"};
    static const QLatin1String namedFilterInfoKeySorters{"sorters"};
    static const QLatin1String namedFilterInfoKeySupportsAllIssueKinds{"supportsAllIssueKinds"};
    static const QLatin1String namedFilterInfoKeyIssueKindRestrictions{"issueKindRestrictions"};
    static const QLatin1String namedFilterInfoKeyVisibility{"visibility"};

    template<>
    class de_serializer<NamedFilterInfoDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static NamedFilterInfoDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<NamedFilterInfoDto>(json);
            return {
                deserialize_field<QString>(jo, namedFilterInfoKeyKey),
                deserialize_field<QString>(jo, namedFilterInfoKeyDisplayName),
                deserialize_field<std::optional<QString>>(jo, namedFilterInfoKeyUrl),
                deserialize_field<bool>(jo, namedFilterInfoKeyIsPredefined),
                deserialize_field<std::optional<QString>>(jo, namedFilterInfoKeyType),
                deserialize_field<bool>(jo, namedFilterInfoKeyCanWrite),
                deserialize_field<std::map<QString, QString>>(jo, namedFilterInfoKeyFilters),
                deserialize_field<std::optional<std::vector<SortInfoDto>>>(jo, namedFilterInfoKeySorters),
                deserialize_field<bool>(jo, namedFilterInfoKeySupportsAllIssueKinds),
                deserialize_field<std::optional<std::unordered_set<QString>>>(jo, namedFilterInfoKeyIssueKindRestrictions),
                deserialize_field<std::optional<NamedFilterVisibilityDto>>(jo, namedFilterInfoKeyVisibility)
            };
        }

        static QJsonValue serialize(const NamedFilterInfoDto &value) {
            QJsonObject jo;
            serialize_field(jo, namedFilterInfoKeyKey, value.key);
            serialize_field(jo, namedFilterInfoKeyDisplayName, value.displayName);
            serialize_field(jo, namedFilterInfoKeyUrl, value.url);
            serialize_field(jo, namedFilterInfoKeyIsPredefined, value.isPredefined);
            serialize_field(jo, namedFilterInfoKeyType, value.type);
            serialize_field(jo, namedFilterInfoKeyCanWrite, value.canWrite);
            serialize_field(jo, namedFilterInfoKeyFilters, value.filters);
            serialize_field(jo, namedFilterInfoKeySorters, value.sorters);
            serialize_field(jo, namedFilterInfoKeySupportsAllIssueKinds, value.supportsAllIssueKinds);
            serialize_field(jo, namedFilterInfoKeyIssueKindRestrictions, value.issueKindRestrictions);
            serialize_field(jo, namedFilterInfoKeyVisibility, value.visibility);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    NamedFilterInfoDto NamedFilterInfoDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<NamedFilterInfoDto>(json);
    }

    NamedFilterInfoDto::NamedFilterInfoDto(
        QString key,
        QString displayName,
        std::optional<QString> url,
        bool isPredefined,
        std::optional<QString> type,
        bool canWrite,
        std::map<QString, QString> filters,
        std::optional<std::vector<SortInfoDto>> sorters,
        bool supportsAllIssueKinds,
        std::optional<std::unordered_set<QString>> issueKindRestrictions,
        std::optional<NamedFilterVisibilityDto> visibility
    ) :
        key(std::move(key)),
        displayName(std::move(displayName)),
        url(std::move(url)),
        isPredefined(std::move(isPredefined)),
        type(std::move(type)),
        canWrite(std::move(canWrite)),
        filters(std::move(filters)),
        sorters(std::move(sorters)),
        supportsAllIssueKinds(std::move(supportsAllIssueKinds)),
        issueKindRestrictions(std::move(issueKindRestrictions)),
        visibility(std::move(visibility))
    { }

    NamedFilterInfoDto::NamedFilterInfoDto(
        QString key,
        QString displayName,
        std::optional<QString> url,
        bool isPredefined,
        std::optional<NamedFilterType> type,
        bool canWrite,
        std::map<QString, QString> filters,
        std::optional<std::vector<SortInfoDto>> sorters,
        bool supportsAllIssueKinds,
        std::optional<std::unordered_set<QString>> issueKindRestrictions,
        std::optional<NamedFilterVisibilityDto> visibility
    ) : NamedFilterInfoDto(
        std::move(key),
        std::move(displayName),
        std::move(url),
        std::move(isPredefined),
        optionalTransform<QString, NamedFilterType>(type, NamedFilterTypeMeta::enumToStr),
        std::move(canWrite),
        std::move(filters),
        std::move(sorters),
        std::move(supportsAllIssueKinds),
        std::move(issueKindRestrictions),
        std::move(visibility))
    { }

    // throws std::range_error
    std::optional<NamedFilterType> NamedFilterInfoDto::getTypeEnum() const
    {
        return optionalTransform<NamedFilterType, QString>(type, NamedFilterTypeMeta::strToEnum);
    }

    void NamedFilterInfoDto::setTypeEnum(std::optional<NamedFilterType> newValue)
    {
        type = optionalTransform<QString, NamedFilterType>(newValue, NamedFilterTypeMeta::enumToStr);
    }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray NamedFilterInfoDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // NamedFilterUpdateDto

    static const QLatin1String namedFilterUpdateKeyName{"name"};
    static const QLatin1String namedFilterUpdateKeyFilters{"filters"};
    static const QLatin1String namedFilterUpdateKeySorters{"sorters"};
    static const QLatin1String namedFilterUpdateKeyVisibility{"visibility"};

    template<>
    class de_serializer<NamedFilterUpdateDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static NamedFilterUpdateDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<NamedFilterUpdateDto>(json);
            return {
                deserialize_field<std::optional<QString>>(jo, namedFilterUpdateKeyName),
                deserialize_field<std::optional<std::map<QString, QString>>>(jo, namedFilterUpdateKeyFilters),
                deserialize_field<std::optional<std::vector<SortInfoDto>>>(jo, namedFilterUpdateKeySorters),
                deserialize_field<std::optional<NamedFilterVisibilityDto>>(jo, namedFilterUpdateKeyVisibility)
            };
        }

        static QJsonValue serialize(const NamedFilterUpdateDto &value) {
            QJsonObject jo;
            serialize_field(jo, namedFilterUpdateKeyName, value.name);
            serialize_field(jo, namedFilterUpdateKeyFilters, value.filters);
            serialize_field(jo, namedFilterUpdateKeySorters, value.sorters);
            serialize_field(jo, namedFilterUpdateKeyVisibility, value.visibility);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    NamedFilterUpdateDto NamedFilterUpdateDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<NamedFilterUpdateDto>(json);
    }

    NamedFilterUpdateDto::NamedFilterUpdateDto(
        std::optional<QString> name,
        std::optional<std::map<QString, QString>> filters,
        std::optional<std::vector<SortInfoDto>> sorters,
        std::optional<NamedFilterVisibilityDto> visibility
    ) :
        name(std::move(name)),
        filters(std::move(filters)),
        sorters(std::move(sorters)),
        visibility(std::move(visibility))
    { }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray NamedFilterUpdateDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // ProjectInfoDto

    static const QLatin1String projectInfoKeyName{"name"};
    static const QLatin1String projectInfoKeyIssueFilterHelp{"issueFilterHelp"};
    static const QLatin1String projectInfoKeyTableMetaUri{"tableMetaUri"};
    static const QLatin1String projectInfoKeyUsers{"users"};
    static const QLatin1String projectInfoKeyVersions{"versions"};
    static const QLatin1String projectInfoKeyIssueKinds{"issueKinds"};
    static const QLatin1String projectInfoKeyHasHiddenIssues{"hasHiddenIssues"};

    template<>
    class de_serializer<ProjectInfoDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static ProjectInfoDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<ProjectInfoDto>(json);
            return {
                deserialize_field<QString>(jo, projectInfoKeyName),
                deserialize_field<std::optional<QString>>(jo, projectInfoKeyIssueFilterHelp),
                deserialize_field<std::optional<QString>>(jo, projectInfoKeyTableMetaUri),
                deserialize_field<std::vector<UserRefDto>>(jo, projectInfoKeyUsers),
                deserialize_field<std::vector<AnalysisVersionDto>>(jo, projectInfoKeyVersions),
                deserialize_field<std::vector<IssueKindInfoDto>>(jo, projectInfoKeyIssueKinds),
                deserialize_field<bool>(jo, projectInfoKeyHasHiddenIssues)
            };
        }

        static QJsonValue serialize(const ProjectInfoDto &value) {
            QJsonObject jo;
            serialize_field(jo, projectInfoKeyName, value.name);
            serialize_field(jo, projectInfoKeyIssueFilterHelp, value.issueFilterHelp);
            serialize_field(jo, projectInfoKeyTableMetaUri, value.tableMetaUri);
            serialize_field(jo, projectInfoKeyUsers, value.users);
            serialize_field(jo, projectInfoKeyVersions, value.versions);
            serialize_field(jo, projectInfoKeyIssueKinds, value.issueKinds);
            serialize_field(jo, projectInfoKeyHasHiddenIssues, value.hasHiddenIssues);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    ProjectInfoDto ProjectInfoDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<ProjectInfoDto>(json);
    }

    ProjectInfoDto::ProjectInfoDto(
        QString name,
        std::optional<QString> issueFilterHelp,
        std::optional<QString> tableMetaUri,
        std::vector<UserRefDto> users,
        std::vector<AnalysisVersionDto> versions,
        std::vector<IssueKindInfoDto> issueKinds,
        bool hasHiddenIssues
    ) :
        name(std::move(name)),
        issueFilterHelp(std::move(issueFilterHelp)),
        tableMetaUri(std::move(tableMetaUri)),
        users(std::move(users)),
        versions(std::move(versions)),
        issueKinds(std::move(issueKinds)),
        hasHiddenIssues(std::move(hasHiddenIssues))
    { }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray ProjectInfoDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // RepositoryUpdateResponseDto

    static const QLatin1String repositoryUpdateResponseKeyMessages{"messages"};
    static const QLatin1String repositoryUpdateResponseKeyHasErrors{"hasErrors"};
    static const QLatin1String repositoryUpdateResponseKeyHasWarnings{"hasWarnings"};

    template<>
    class de_serializer<RepositoryUpdateResponseDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static RepositoryUpdateResponseDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<RepositoryUpdateResponseDto>(json);
            return {
                deserialize_field<std::vector<RepositoryUpdateMessageDto>>(jo, repositoryUpdateResponseKeyMessages),
                deserialize_field<bool>(jo, repositoryUpdateResponseKeyHasErrors),
                deserialize_field<bool>(jo, repositoryUpdateResponseKeyHasWarnings)
            };
        }

        static QJsonValue serialize(const RepositoryUpdateResponseDto &value) {
            QJsonObject jo;
            serialize_field(jo, repositoryUpdateResponseKeyMessages, value.messages);
            serialize_field(jo, repositoryUpdateResponseKeyHasErrors, value.hasErrors);
            serialize_field(jo, repositoryUpdateResponseKeyHasWarnings, value.hasWarnings);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    RepositoryUpdateResponseDto RepositoryUpdateResponseDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<RepositoryUpdateResponseDto>(json);
    }

    RepositoryUpdateResponseDto::RepositoryUpdateResponseDto(
        std::vector<RepositoryUpdateMessageDto> messages,
        bool hasErrors,
        bool hasWarnings
    ) :
        messages(std::move(messages)),
        hasErrors(std::move(hasErrors)),
        hasWarnings(std::move(hasWarnings))
    { }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray RepositoryUpdateResponseDto::serialize() const
    {
        return serialize_bytes(*this);
    }

    // TableInfoDto

    static const QLatin1String tableInfoKeyTableDataUri{"tableDataUri"};
    static const QLatin1String tableInfoKeyIssueBaseViewUri{"issueBaseViewUri"};
    static const QLatin1String tableInfoKeyColumns{"columns"};
    static const QLatin1String tableInfoKeyFilters{"filters"};
    static const QLatin1String tableInfoKeyUserDefaultFilter{"userDefaultFilter"};
    static const QLatin1String tableInfoKeyAxivionDefaultFilter{"axivionDefaultFilter"};

    template<>
    class de_serializer<TableInfoDto> final
    {
    public:
        // throws Axivion::Internal::Dto::invalid_dto_exception
        static TableInfoDto deserialize(const QJsonValue &json) {
            const QJsonObject jo = toJsonObject<TableInfoDto>(json);
            return {
                deserialize_field<QString>(jo, tableInfoKeyTableDataUri),
                deserialize_field<std::optional<QString>>(jo, tableInfoKeyIssueBaseViewUri),
                deserialize_field<std::vector<ColumnInfoDto>>(jo, tableInfoKeyColumns),
                deserialize_field<std::vector<NamedFilterInfoDto>>(jo, tableInfoKeyFilters),
                deserialize_field<std::optional<QString>>(jo, tableInfoKeyUserDefaultFilter),
                deserialize_field<QString>(jo, tableInfoKeyAxivionDefaultFilter)
            };
        }

        static QJsonValue serialize(const TableInfoDto &value) {
            QJsonObject jo;
            serialize_field(jo, tableInfoKeyTableDataUri, value.tableDataUri);
            serialize_field(jo, tableInfoKeyIssueBaseViewUri, value.issueBaseViewUri);
            serialize_field(jo, tableInfoKeyColumns, value.columns);
            serialize_field(jo, tableInfoKeyFilters, value.filters);
            serialize_field(jo, tableInfoKeyUserDefaultFilter, value.userDefaultFilter);
            serialize_field(jo, tableInfoKeyAxivionDefaultFilter, value.axivionDefaultFilter);
            return { jo };
        }

        de_serializer() = delete;
        ~de_serializer() = delete;
    };

    // throws Axivion::Internal::Dto::invalid_dto_exception
    TableInfoDto TableInfoDto::deserialize(const QByteArray &json)
    {
        return deserialize_bytes<TableInfoDto>(json);
    }

    TableInfoDto::TableInfoDto(
        QString tableDataUri,
        std::optional<QString> issueBaseViewUri,
        std::vector<ColumnInfoDto> columns,
        std::vector<NamedFilterInfoDto> filters,
        std::optional<QString> userDefaultFilter,
        QString axivionDefaultFilter
    ) :
        tableDataUri(std::move(tableDataUri)),
        issueBaseViewUri(std::move(issueBaseViewUri)),
        columns(std::move(columns)),
        filters(std::move(filters)),
        userDefaultFilter(std::move(userDefaultFilter)),
        axivionDefaultFilter(std::move(axivionDefaultFilter))
    { }

    // throws Axivion::Internal::Dto::invalid_dto_exception
    QByteArray TableInfoDto::serialize() const
    {
        return serialize_bytes(*this);
    }
}
