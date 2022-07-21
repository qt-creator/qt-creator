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

#include <utils/span.h>

#include <type_traits>
#include <vector>

namespace Sqlite {

template<auto Type, typename InternalIntegerType = long long>
class BasicId
{
public:
    using IsBasicId = std::true_type;
    using DatabaseType = InternalIntegerType;

    constexpr explicit BasicId() = default;

    constexpr BasicId(const char *) = delete;

    static constexpr BasicId create(InternalIntegerType idNumber)
    {
        BasicId id;
        id.id = idNumber;
        return id;
    }

    constexpr friend bool compareInvalidAreTrue(BasicId first, BasicId second)
    {
        return first.id == second.id;
    }

    constexpr friend bool operator==(BasicId first, BasicId second)
    {
        return first.id == second.id && first.isValid() && second.isValid();
    }

    constexpr friend bool operator!=(BasicId first, BasicId second) { return !(first == second); }

    constexpr friend bool operator<(BasicId first, BasicId second) { return first.id < second.id; }
    constexpr friend bool operator>(BasicId first, BasicId second) { return first.id > second.id; }
    constexpr friend bool operator<=(BasicId first, BasicId second)
    {
        return first.id <= second.id;
    }
    constexpr friend bool operator>=(BasicId first, BasicId second)
    {
        return first.id >= second.id;
    }

    constexpr friend InternalIntegerType operator-(BasicId first, BasicId second)
    {
        return first.id - second.id;
    }

    constexpr bool isValid() const { return id >= 0; }

    explicit operator bool() const { return isValid(); }

    explicit operator std::size_t() const { return static_cast<std::size_t>(id); }

    InternalIntegerType internalId() const { return id; }

    [[noreturn, deprecated]] InternalIntegerType operator&() const { throw std::exception{}; }

private:
    InternalIntegerType id = -1;
};

template<typename Container>
auto toIntegers(const Container &container)
{
    using DataType = typename Container::value_type::DatabaseType;
    const DataType *data = reinterpret_cast<const DataType *>(container.data());

    return Utils::span{data, container.size()};
}

} // namespace Sqlite
