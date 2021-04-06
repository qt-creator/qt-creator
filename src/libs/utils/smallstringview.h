/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
};

constexpr bool operator==(SmallStringView first, SmallStringView second) noexcept
{
    return first.size() == second.size()
           && std::char_traits<char>::compare(first.data(), second.data(), first.size()) == 0;
}

constexpr bool operator!=(SmallStringView first, SmallStringView second) noexcept
{
    return !(first == second);
}

constexpr int compare(SmallStringView first, SmallStringView second) noexcept
{
    int sizeDifference = int(first.size() - second.size());

    if (sizeDifference == 0)
        return std::char_traits<char>::compare(first.data(), second.data(), first.size());

    return sizeDifference;
}

constexpr bool operator<(SmallStringView first, SmallStringView second) noexcept
{
    return compare(first, second) < 0;
}

constexpr bool operator>(SmallStringView first, SmallStringView second) noexcept
{
    return second < first;
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
    int sizeDifference = int(first.size() - second.size());

    if (sizeDifference == 0)
        return Internal::reverse_memcmp(first.data(), second.data(), first.size());

    return sizeDifference;
}

} // namespace Utils

constexpr Utils::SmallStringView operator""_sv(const char *const string, size_t size)
{
    return Utils::SmallStringView(string, size);
}
