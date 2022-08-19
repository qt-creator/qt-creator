// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <nodeinstanceglobal.h>

#include <utils/smallstring.h>
#include <utils/span.h>

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
using AuxiliaryDatas = std::vector<std::pair<AuxiliaryDataKey, QVariant>>;
using AuxiliaryDatasView = Utils::span<const std::pair<AuxiliaryDataKey, QVariant>>;
using AuxiliaryDatasForType = std::vector<std::pair<Utils::SmallString, QVariant>>;
} // namespace QmlDesigner
