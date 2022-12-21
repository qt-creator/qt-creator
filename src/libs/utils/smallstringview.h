// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "smallstringfwd.h"
#include "smallstringiterator.h"

#include <QString>

#include <cstring>
#include <string>
#include <string_view>

namespace Utils {

template <typename String>
using enable_if_has_char_data_pointer = typename std::enable_if_t<
                                            std::is_same<
                                                std::remove_const_t<
                                                    std::remove_pointer_t<
                                                        decltype(std::declval<const String>().data())
                                                        >
                                                    >, char>::value
                                            , int>;

class SmallStringView : public std::string_view
{
public:
    using std::string_view::string_view;

    constexpr SmallStringView(const_iterator begin, const_iterator end) noexcept
        : std::string_view{std::addressof(*begin), static_cast<std::size_t>(std::distance(begin, end))}
    {}

#ifdef Q_CC_MSVC
    constexpr SmallStringView(const char *const begin, const char *const end) noexcept
        : std::string_view{begin, static_cast<std::size_t>(std::distance(begin, end))}
    {}
#endif

    template<typename String, typename Utils::enable_if_has_char_data_pointer<String> = 0>
    constexpr SmallStringView(const String &string) noexcept
        : std::string_view{string.data(), static_cast<std::size_t>(string.size())}
    {}

    static constexpr SmallStringView fromUtf8(const char *const characterPointer)
    {
        return SmallStringView(characterPointer);
    }

    constexpr size_type isEmpty() const noexcept { return empty(); }

    constexpr
    SmallStringView mid(size_type position) const noexcept
    {
        return SmallStringView(data() + position, size() - position);
    }

    constexpr
    SmallStringView mid(size_type position, size_type length) const noexcept
    {
        return SmallStringView(data() + position, length);
    }

    constexpr20 operator std::string() const { return std::string(data(), size()); }

    explicit operator QString() const
    {
        return QString::fromUtf8(data(), int(size()));
    }

    explicit operator QByteArray() const
    {
        return QByteArray(data(), int(size()));
    }

    constexpr bool startsWith(SmallStringView subStringToSearch) const noexcept
    {
        if (size() >= subStringToSearch.size())
            return !std::char_traits<char>::compare(data(),
                                                    subStringToSearch.data(),
                                                    subStringToSearch.size());

        return false;
    }

    constexpr bool startsWith(char characterToSearch) const noexcept
    {
        return *begin() == characterToSearch;
    }

    constexpr bool endsWith(SmallStringView ending) const noexcept
    {
        return size() >= ending.size() && std::equal(ending.rbegin(), ending.rend(), rbegin());
    }
};

constexpr bool operator!=(SmallStringView first, SmallStringView second) noexcept
{
    return std::string_view{first} != std::string_view{second};
}

constexpr bool operator==(SmallStringView first, SmallStringView second) noexcept
{
    return std::string_view{first} == std::string_view{second};
}

constexpr bool operator<(SmallStringView first, SmallStringView second) noexcept
{
    return std::string_view{first} < std::string_view{second};
}

constexpr bool operator>(SmallStringView first, SmallStringView second) noexcept
{
    return std::string_view{first} > std::string_view{second};
}

constexpr bool operator<=(SmallStringView first, SmallStringView second) noexcept
{
    return std::string_view{first} <= std::string_view{second};
}

constexpr bool operator>=(SmallStringView first, SmallStringView second) noexcept
{
    return std::string_view{first} >= std::string_view{second};
}

constexpr int compare(SmallStringView first, SmallStringView second) noexcept
{
    return first.compare(second);
}

namespace Internal {
constexpr int reverse_memcmp(const char *first, const char *second, size_t n)
{
    const char *currentFirst = first + n - 1;
    const char *currentSecond = second + n - 1;

    while (n > 0) {
        // If the current characters differ, return an appropriately signed
        // value; otherwise, keep searching backwards
        int difference = *currentFirst - *currentSecond;
        if (difference != 0)
            return difference;

        --currentFirst;
        --currentSecond;
        --n;
    }

    return 0;
}
} // namespace Internal

constexpr int reverseCompare(SmallStringView first, SmallStringView second) noexcept
{
    int difference = Internal::reverse_memcmp(first.data(), second.data(), first.size());

    if (difference == 0)
        return int(first.size()) - int(second.size());

    return difference;
}

} // namespace Utils

constexpr Utils::SmallStringView operator""_sv(const char *const string, size_t size)
{
    return Utils::SmallStringView(string, size);
}
