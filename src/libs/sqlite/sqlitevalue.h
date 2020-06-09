/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "sqliteexception.h"

#include <utils/smallstring.h>
#include <utils/variant.h>

#include <QVariant>

#include <cstddef>


#pragma once

namespace Sqlite {

enum class ValueType : unsigned char { Null, Integer, Float, String };

class NullValue
{
    friend bool operator==(NullValue, NullValue) { return false; }
};

template<typename StringType>
class ValueBase
{
public:
    using VariantType = Utils::variant<NullValue, long long, double, StringType>;

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

    bool isNull() const { return value.index() == 0; }

    long long toInteger() const { return Utils::get<int(ValueType::Integer)>(value); }

    double toFloat() const { return Utils::get<int(ValueType::Float)>(value); }

    Utils::SmallStringView toStringView() const
    {
        return Utils::get<int(ValueType::String)>(value);
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
        case ValueType::Null:
            break;
        }

        return {};
    }

    friend bool operator==(const ValueBase &first, std::nullptr_t) { return first.isNull(); }

    friend bool operator==(const ValueBase &first, long long second)
    {
        auto maybeInteger = Utils::get_if<int(ValueType::Integer)>(&first.value);

        return maybeInteger && *maybeInteger == second;
    }

    friend bool operator==(long long first, const ValueBase &second) { return second == first; }

    friend bool operator==(const ValueBase &first, double second)
    {
        auto maybeInteger = Utils::get_if<int(ValueType::Float)>(&first.value);

        return maybeInteger && *maybeInteger == second;
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
        auto maybeInteger = Utils::get_if<int(ValueType::String)>(&first.value);

        return maybeInteger && *maybeInteger == second;
    }

    friend bool operator==(Utils::SmallStringView first, const ValueBase &second)
    {
        return second == first;
    }

    friend bool operator==(const ValueBase &first, const QString &second)
    {
        auto maybeInteger = Utils::get_if<int(ValueType::String)>(&first.value);

        return maybeInteger
               && second == QLatin1String{maybeInteger->data(), int(maybeInteger->size())};
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
        }

        return {};
    }

public:
    VariantType value;
};

class ValueView : public ValueBase<Utils::SmallStringView>
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

class Value : public ValueBase<Utils::SmallString>
{
    using Base = ValueBase<Utils::SmallString>;

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

        switch (value.type()) {
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
        default:
            throw CannotConvert("Cannot convert this QVariant to a ValueBase");
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
        default:
            throw CannotConvert("Cannot convert this QVariant to a ValueBase");
        }
    }
};

using Values = std::vector<Value>;
} // namespace Sqlite
