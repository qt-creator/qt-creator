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

class SmallStringView
{
public:
    using const_iterator = Internal::SmallStringIterator<std::random_access_iterator_tag, const char>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using size_type = std::size_t;

    constexpr SmallStringView() = default;

    constexpr17 SmallStringView(const char *characterPointer) noexcept
        : m_pointer(characterPointer)
        , m_size(std::char_traits<char>::length(characterPointer))
    {}

    constexpr SmallStringView(const char *const string, const size_type size) noexcept
        : m_pointer(string)
        , m_size(size)
    {
    }

    constexpr SmallStringView(const const_iterator begin, const const_iterator end) noexcept
        : m_pointer(begin.data())
        , m_size(std::size_t(end - begin))
    {
    }

    template<typename String, typename Utils::enable_if_has_char_data_pointer<String> = 0>
    constexpr SmallStringView(const String &string) noexcept
        : m_pointer(string.data())
        , m_size(string.size())
    {}

    static constexpr17 SmallStringView fromUtf8(const char *const characterPointer)
    {
        return SmallStringView(characterPointer);
    }

    constexpr
    const char *data() const noexcept
    {
        return m_pointer;
    }

    constexpr
    size_type size() const noexcept
    {
        return m_size;
    }

    constexpr
    size_type isEmpty() const noexcept
    {
        return m_size == 0;
    }

    constexpr
    size_type empty() const noexcept
    {
        return m_size == 0;
    }

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

    constexpr
    const_iterator begin() const noexcept
    {
        return data();
    }

    constexpr
    const_iterator end() const noexcept
    {
        return data() + size();
    }

    constexpr17 const_reverse_iterator rbegin() const noexcept
    {
        return const_reverse_iterator(end());
    }

    constexpr17 const_reverse_iterator rend() const noexcept
    {
        return const_reverse_iterator(begin());
    }

    constexpr20 operator std::string() const { return std::string(data(), size()); }

    explicit operator QString() const
    {
        return QString::fromUtf8(data(), int(size()));
    }

    constexpr17 bool startsWith(SmallStringView subStringToSearch) const noexcept
    {
        if (size() >= subStringToSearch.size())
            return !std::char_traits<char>::compare(m_pointer,
                                                    subStringToSearch.data(),
                                                    subStringToSearch.size());

        return false;
    }

    constexpr bool startsWith(char characterToSearch) const noexcept
    {
        return m_pointer[0] == characterToSearch;
    }

    constexpr char back() const { return m_pointer[m_size - 1]; }

    constexpr char operator[](std::size_t index) { return m_pointer[index]; }

private:
    const char *m_pointer = "";
    size_type m_size = 0;
};

constexpr17 bool operator==(SmallStringView first, SmallStringView second) noexcept
{
    return first.size() == second.size()
           && std::char_traits<char>::compare(first.data(), second.data(), first.size()) == 0;
}

constexpr17 bool operator!=(SmallStringView first, SmallStringView second) noexcept
{
    return !(first == second);
}

constexpr17 int compare(SmallStringView first, SmallStringView second) noexcept
{
    int sizeDifference = int(first.size() - second.size());

    if (sizeDifference == 0)
        return std::char_traits<char>::compare(first.data(), second.data(), first.size());

    return sizeDifference;
}

constexpr17 bool operator<(SmallStringView first, SmallStringView second) noexcept
{
    return compare(first, second) < 0;
}

constexpr17 bool operator>(SmallStringView first, SmallStringView second) noexcept
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
