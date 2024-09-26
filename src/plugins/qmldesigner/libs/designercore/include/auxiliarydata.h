// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <nodeinstanceglobal.h>

#include <utils/smallstring.h>
#include <utils/span.h>

#include <QColor>
#include <QStringView>
#include <QVariant>

#include <vector>

namespace QmlDesigner {

template<typename NameType>
class BasicAuxiliaryDataKey
{
public:
    constexpr BasicAuxiliaryDataKey() = default;
    constexpr BasicAuxiliaryDataKey(AuxiliaryDataType type, NameType name)
        : type{type}
        , name{std::move(name)}
    {}

    template<typename OtherNameType, typename = std::enable_if_t<!std::is_same_v<NameType, OtherNameType>>>
    constexpr explicit BasicAuxiliaryDataKey(const BasicAuxiliaryDataKey<OtherNameType> &other)
        : type{other.type}
        , name{NameType{other.name}}
    {}

    operator BasicAuxiliaryDataKey<Utils::SmallStringView>() const { return {type, name}; }

public:
    AuxiliaryDataType type = AuxiliaryDataType::None;
    NameType name;
};

template<typename First, typename Second>
bool operator<(const BasicAuxiliaryDataKey<First> &first, const BasicAuxiliaryDataKey<Second> &second)
{
    return std::tie(first.type, first.name) < std::tie(second.type, second.name);
}

template<typename First, typename Second>
bool operator==(const BasicAuxiliaryDataKey<First> &first, const BasicAuxiliaryDataKey<Second> &second)
{
    return first.type == second.type && first.name == second.name;
}

template<typename First, typename Second>
bool operator!=(const BasicAuxiliaryDataKey<First> &first, const BasicAuxiliaryDataKey<Second> &second)
{
    return !(first == second);
}

using AuxiliaryDataKey = BasicAuxiliaryDataKey<Utils::SmallString>;
using AuxiliaryDataKeyView = BasicAuxiliaryDataKey<Utils::SmallStringView>;
using AuxiliaryData = std::pair<AuxiliaryDataKey, QVariant>;
using AuxiliaryDatas = std::vector<AuxiliaryData>;
using AuxiliaryDatasView = Utils::span<const AuxiliaryData>;
using AuxiliaryDatasForType = std::vector<std::pair<Utils::SmallString, QVariant>>;

using PropertyValue = std::variant<int, long long, double, bool, QColor, QStringView, Qt::Corner>;

inline QVariant toQVariant(const PropertyValue &variant)
{
    return std::visit([](const auto &value) { return QVariant::fromValue(value); }, variant);
}

class AuxiliaryDataKeyDefaultValue : public AuxiliaryDataKeyView
{
public:
    constexpr AuxiliaryDataKeyDefaultValue() = default;
    constexpr AuxiliaryDataKeyDefaultValue(AuxiliaryDataType type,
                                           Utils::SmallStringView name,
                                           PropertyValue defaultValue)
        : AuxiliaryDataKeyView{type, name}
        , defaultValue{std::move(defaultValue)}
    {}

public:
    PropertyValue defaultValue;
};

template<typename Type>
QVariant getDefaultValueAsQVariant(const Type &key)
{
    if constexpr (std::is_same_v<AuxiliaryDataKey, AuxiliaryDataKeyDefaultValue>)
        return toQVariant(key.defaultvalue);

    return {};
}

} // namespace QmlDesigner

namespace std {

template<class T, class U, template<class> class TQual, template<class> class UQual>
struct basic_common_reference<QmlDesigner::BasicAuxiliaryDataKey<T>, QmlDesigner::BasicAuxiliaryDataKey<U>, TQual, UQual>
{
    using type = QmlDesigner::AuxiliaryDataKeyView;
};
} // namespace std
