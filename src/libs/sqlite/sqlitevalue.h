// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sqliteblob.h"
#include "sqliteexception.h"

#include <utils/smallstring.h>

#include <QVariant>

#include <cstddef>
#include <variant>

namespace Sqlite {

enum class ValueType : unsigned char { Null, Integer, Float, String, Blob };

class NullValue
{
    friend bool operator==(NullValue, NullValue) { return false; }
};

template<typename StringType, typename BlobType>
class ValueBase
{
public:
    using VariantType = std::variant<NullValue, long long, double, StringType, BlobType>;

    ValueBase() = default;

    explicit ValueBase(NullValue) {}

    explicit ValueBase(VariantType &&value)
        : value(value)
    {}

    explicit ValueBase(const char *value)
        : value(Utils::SmallStringView{value})
    {}

    explicit ValueBase(long long value)
        : value(value)
    {}
    explicit ValueBase(int value)
        : value(static_cast<long long>(value))
    {}

    explicit ValueBase(uint value)
        : value(static_cast<long long>(value))
    {}

    explicit ValueBase(double value)
        : value(value)
    {}

    explicit ValueBase(Utils::SmallStringView value)
        : value(value)

    {}

    explicit ValueBase(StringType &&value)
        : value(std::move(value))

    {}

    explicit ValueBase(BlobView value)
        : value(value)

    {}

    explicit ValueBase(Blob &&value)
        : value(std::move(value))

    {}

    explicit ValueBase(const Blob &value)
        : value(value)

    {}

    bool isNull() const { return value.index() == 0; }

    long long toInteger() const { return std::get<int(ValueType::Integer)>(value); }

    double toFloat() const { return std::get<int(ValueType::Float)>(value); }

    Utils::SmallStringView toStringView() const
    {
        return std::get<int(ValueType::String)>(value);
    }

    BlobView toBlobView() const
    {
        const BlobType &blob = std::get<int(ValueType::Blob)>(value);
        if constexpr (std::is_same_v<BlobType, Blob>) {
            return {blob.bytes};
        } else {
            return blob;
        }
    }
    explicit operator QVariant() const
    {
        switch (type()) {
        case ValueType::Integer:
            return QVariant(toInteger());
        case ValueType::Float:
            return QVariant(toFloat());
        case ValueType::String:
            return QVariant(QString(toStringView()));
        case ValueType::Blob: {
            auto blobView = toBlobView();
            return QVariant(QByteArray(blobView.cdata(), blobView.sisize()));
        }
        case ValueType::Null:
            break;
        }

        return {};
    }

    friend bool operator==(const ValueBase &first, std::nullptr_t) { return first.isNull(); }

    friend bool operator==(const ValueBase &first, long long second)
    {
        auto maybeInteger = std::get_if<int(ValueType::Integer)>(&first.value);

        return maybeInteger && *maybeInteger == second;
    }

    friend bool operator==(long long first, const ValueBase &second) { return second == first; }

    friend bool operator==(const ValueBase &first, double second)
    {
        auto maybeFloat = std::get_if<int(ValueType::Float)>(&first.value);

        return maybeFloat && *maybeFloat == second;
    }

    friend bool operator==(const ValueBase &first, int second)
    {
        return first == static_cast<long long>(second);
    }

    friend bool operator==(int first, const ValueBase &second) { return second == first; }

    friend bool operator==(const ValueBase &first, uint second)
    {
        return first == static_cast<long long>(second);
    }

    friend bool operator==(uint first, const ValueBase &second) { return second == first; }

    friend bool operator==(double first, const ValueBase &second) { return second == first; }

    friend bool operator==(const ValueBase &first, Utils::SmallStringView second)
    {
        auto maybeString = std::get_if<int(ValueType::String)>(&first.value);

        return maybeString && *maybeString == second;
    }

    friend bool operator==(Utils::SmallStringView first, const ValueBase &second)
    {
        return second == first;
    }

    friend bool operator==(const ValueBase &first, const QString &second)
    {
        auto maybeString = std::get_if<int(ValueType::String)>(&first.value);

        return maybeString && second == QLatin1String{maybeString->data(), int(maybeString->size())};
    }

    friend bool operator==(const QString &first, const ValueBase &second)
    {
        return second == first;
    }

    friend bool operator==(const ValueBase &first, const char *second)
    {
        return first == Utils::SmallStringView{second};
    }

    friend bool operator==(const char *first, const ValueBase &second) { return second == first; }

    friend bool operator==(const ValueBase &first, const ValueBase &second)
    {
        return first.value == second.value;
    }

    friend bool operator!=(const ValueBase &first, const ValueBase &second)
    {
        return !(first.value == second.value);
    }

    friend bool operator==(const ValueBase &first, BlobView second)
    {
        auto maybeBlob = std::get_if<int(ValueType::Blob)>(&first.value);

        return maybeBlob && *maybeBlob == second;
    }

    friend bool operator==(BlobView first, const ValueBase &second) { return second == first; }

    ValueType type() const
    {
        switch (value.index()) {
        case 0:
            return ValueType::Null;
        case 1:
            return ValueType::Integer;
        case 2:
            return ValueType::Float;
        case 3:
            return ValueType::String;
        case 4:
            return ValueType::Blob;
        }

        return {};
    }

public:
    VariantType value;
};

class ValueView : public ValueBase<Utils::SmallStringView, BlobView>
{
public:
    explicit ValueView(ValueBase &&base)
        : ValueBase(std::move(base))
    {}

    template<typename Type>
    static ValueView create(Type &&value)
    {
        return ValueView{ValueBase{value}};
    }
};

class Value : public ValueBase<Utils::SmallString, Blob>
{
    using Base = ValueBase<Utils::SmallString, Blob>;

public:
    using Base::Base;

    Value() = default;

    explicit Value(NullValue) {}

    explicit Value(ValueView view)
        : ValueBase(convert(view))
    {}

    explicit Value(const QVariant &value)
        : ValueBase(convert(value))
    {}

    explicit Value(Utils::SmallString &&value)
        : ValueBase(VariantType{std::move(value)})
    {}

    explicit Value(const Utils::SmallString &value)
        : ValueBase(Utils::SmallStringView(value))
    {}

    explicit Value(const QString &value)
        : ValueBase(VariantType{Utils::SmallString(value)})
    {}

    explicit Value(const std::string &value)
        : ValueBase(VariantType{Utils::SmallString(value)})
    {}

    explicit Value(const Blob &value)
        : ValueBase(value)
    {}

    explicit Value(Blob &&value)
        : ValueBase(std::move(value))
    {}

    explicit Value(BlobView value)
        : ValueBase(value)
    {}

    friend bool operator!=(const Value &first, const Value &second)
    {
        return !(first.value == second.value);
    }

    template<typename Type>
    friend bool operator!=(const Value &first, const Type &second)
    {
        return !(first == second);
    }

    template<typename Type>
    friend bool operator!=(const Type &first, const Value &second)
    {
        return !(first == second);
    }

    friend bool operator==(const Value &first, const ValueView &second)
    {
        if (first.type() != second.type())
            return false;

        switch (first.type()) {
        case ValueType::Integer:
            return first.toInteger() == second.toInteger();
        case ValueType::Float:
            return first.toFloat() == second.toFloat();
        case ValueType::String:
            return first.toStringView() == second.toStringView();
        case ValueType::Blob:
            return first.toBlobView() == second.toBlobView();
        case ValueType::Null:
            return false;
        }

        return false;
    }

    friend bool operator==(const ValueView &first, const Value &second) { return second == first; }

private:
    static Base::VariantType convert(const QVariant &value)
    {
        if (value.isNull())
            return VariantType{NullValue{}};

        switch (value.userType()) {
        case QVariant::Int:
            return VariantType{static_cast<long long>(value.toInt())};
        case QVariant::LongLong:
            return VariantType{value.toLongLong()};
        case QVariant::UInt:
            return VariantType{static_cast<long long>(value.toUInt())};
        case QVariant::Double:
            return VariantType{value.toFloat()};
        case QVariant::String:
            return VariantType{value.toString()};
        case QVariant::ByteArray:
            return VariantType{Blob{value.toByteArray()}};
        default:
            throw CannotConvert();
        }
    }

    static Base::VariantType convert(ValueView view)
    {
        switch (view.type()) {
        case ValueType::Null:
            return VariantType(NullValue{});
        case ValueType::Integer:
            return VariantType{view.toInteger()};
        case ValueType::Float:
            return VariantType{view.toFloat()};
        case ValueType::String:
            return VariantType{view.toStringView()};
        case ValueType::Blob:
            return VariantType{view.toBlobView()};
        default:
            throw CannotConvert();
        }
    }
};

using Values = std::vector<Value>;
} // namespace Sqlite
