// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/span.h>
#include <utils/utility.h>

#include <nanotrace/nanotracehr.h>
#include <type_traits>
#include <vector>

namespace Sqlite {

template<auto Type, typename InternalIntegerType = long long>
class BasicId
{
    template<auto, auto>
    friend class CompoundBasicId;

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

    template<typename Enumeration>
    static constexpr BasicId createSpecialState(Enumeration specialState)
    {
        BasicId id;
        id.id = ::Utils::to_underlying(specialState);
        return id;
    }

    friend constexpr bool compareInvalidAreTrue(BasicId first, BasicId second)
    {
        return first.id == second.id;
    }

    friend constexpr bool operator==(BasicId first, BasicId second)
    {
        return first.id == second.id && first.isValid();
    }

    friend constexpr auto operator<=>(BasicId first, BasicId second) = default;

    constexpr friend InternalIntegerType operator-(BasicId first, BasicId second)
    {
        return first.id - second.id;
    }

    constexpr bool isValid() const { return id > 0; }

    constexpr bool isNull() const { return id == 0; }

    template<typename Enumeration>
    constexpr bool hasSpecialState(Enumeration specialState) const
    {
        return id == ::Utils::to_underlying(specialState);
    }

    explicit operator bool() const { return isValid(); }

    explicit operator std::size_t() const { return static_cast<std::size_t>(id); }

    InternalIntegerType internalId() const { return id; }

    [[noreturn, deprecated]] InternalIntegerType operator&() const { throw std::exception{}; }

    template<typename String>
    friend void convertToString(String &string, BasicId id)
    {
        NanotraceHR::convertToString(string, id.id);
    }

    friend bool compareId(BasicId first, BasicId second) { return first.id == second.id; }

protected:
    InternalIntegerType id = 0;
};

template<auto Type, auto ContextType>
class CompoundBasicId
{
public:
    using IsBasicId = std::true_type;
    using DatabaseType = long long;

    using MainId = BasicId<Type, int>;
    using ContextId = BasicId<ContextType, int>;

    constexpr explicit CompoundBasicId() = default;

    static constexpr CompoundBasicId create(MainId id, ContextId contextId)
    {
        CompoundBasicId compoundId;
        compoundId.id = (static_cast<long long>(contextId.id) << 32) | static_cast<long long>(id.id);

        return compoundId;
    }

    static constexpr CompoundBasicId create(long long idNumber)
    {
        CompoundBasicId id;
        id.id = idNumber;

        return id;
    }

    constexpr MainId mainId() const { return MainId::create(id); }

    constexpr ContextId contextId() const { return ContextId::create(id >> 32); }

    friend constexpr bool compareInvalidAreTrue(CompoundBasicId first, CompoundBasicId second)
    {
        return first.id, second.id;
    }

    friend constexpr bool operator==(CompoundBasicId first, CompoundBasicId second)
    {
        return first.id == second.id && first.isValid();
    }

    friend constexpr auto operator<=>(CompoundBasicId first, CompoundBasicId second) = default;

    constexpr bool isValid() const { return id != 0; }

    constexpr bool isNull() const { return id == 0; }

    explicit operator bool() const { return isValid(); }

    long long internalId() const { return id; }

    explicit operator std::size_t() const { return static_cast<std::size_t>(id | 0xFFFFFFFFULL); }

    template<typename String>
    friend void convertToString(String &string, CompoundBasicId id)
    {
        int mainId = id;
        int contextId = id >> 32;
        convertToString(string, mainId);
        convertToString(string, contextId);
    }

    friend bool compareId(CompoundBasicId first, CompoundBasicId second)
    {
        return first.id == second.id;
    }

protected:
    long long id = 0;
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
