/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#pragma once

#include <nodeinstanceglobal.h>

#include <utils/smallstring.h>

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
using AuxiliaryDatasForType = std::vector<std::pair<Utils::SmallString, QVariant>>;
} // namespace QmlDesigner
