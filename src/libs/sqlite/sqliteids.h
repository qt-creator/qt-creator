// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

    constexpr bool isValid() const { return id > 0; }

    explicit operator bool() const { return isValid(); }

    explicit operator std::size_t() const { return static_cast<std::size_t>(id); }

    InternalIntegerType internalId() const { return id; }

    [[noreturn, deprecated]] InternalIntegerType operator&() const { throw std::exception{}; }

private:
    InternalIntegerType id = 0;
};

template<typename Container>
auto toIntegers(const Container &container)
{
    using DataType = typename Container::value_type::DatabaseType;
    const DataType *data = reinterpret_cast<const DataType *>(container.data());

    return Utils::span{data, container.size()};
}

} // namespace Sqlite

namespace std {
template<auto Type, typename InternalIntegerType>
struct hash<Sqlite::BasicId<Type, InternalIntegerType>>
{
    auto operator()(const Sqlite::BasicId<Type, InternalIntegerType> &id) const
    {
        return std::hash<InternalIntegerType>{}(id.internalId());
    }
};
} // namespace std
