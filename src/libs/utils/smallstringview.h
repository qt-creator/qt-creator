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

#include "smallstringiterator.h"

#include <QString>

#include <cstring>
#include <string>

namespace Utils {

class SmallStringView
{
public:
    using const_iterator = Internal::SmallStringIterator<std::random_access_iterator_tag, const char>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using size_type = std::size_t;

    template<size_type Size>
    constexpr
    SmallStringView(const char(&string)[Size]) noexcept
        : m_pointer(string),
          m_size(Size - 1)
    {
        static_assert(Size >= 1, "Invalid string literal! Length is zero!");
    }

    template<typename Type,
             typename = std::enable_if_t<std::is_pointer<Type>::value>
             >
    SmallStringView(Type characterPointer) noexcept
        : m_pointer(characterPointer),
          m_size(std::strlen(characterPointer))
    {
        static_assert(!std::is_array<Type>::value, "Input type is array and not char pointer!");
    }

    constexpr
    SmallStringView(const char *const string, const size_type size) noexcept
        : m_pointer(string),
          m_size(size)
    {
    }

    SmallStringView(const std::string &string) noexcept
        : m_pointer(string.data()),
          m_size(string.size())
    {
    }

    static
    SmallStringView fromUtf8(const char *const characterPointer)
    {
        return SmallStringView(characterPointer, std::strlen(characterPointer));
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
    const_iterator begin() const noexcept
    {
        return data();
    }

    constexpr
    const_iterator end() const noexcept
    {
        return data() + size();
    }

    const_reverse_iterator rbegin() const noexcept
    {
        return const_reverse_iterator(end());
    }

    const_reverse_iterator rend() const noexcept
    {
        return const_reverse_iterator(begin());
    }

    operator std::string() const
    {
        return std::string(data(), size());
    }

    explicit operator QString() const
    {
        return QString::fromUtf8(data(), int(size()));
    }

    bool startsWith(SmallStringView subStringToSearch) const noexcept
    {
        if (size() >= subStringToSearch.size())
            return !std::memcmp(m_pointer, subStringToSearch.data(), subStringToSearch.size());

        return false;
    }

    bool startsWith(char characterToSearch) const noexcept
    {
        return m_pointer[0] == characterToSearch;
    }

private:
    const char *m_pointer;
    size_type m_size;
};

inline
bool operator==(SmallStringView first, SmallStringView second) noexcept
{
    return first.size() == second.size() && std::memcmp(first.data(), second.data(), first.size()) == 0;
}

inline
bool operator!=(SmallStringView first, SmallStringView second) noexcept
{
    return !(first == second);
}

inline
int compare(SmallStringView first, SmallStringView second) noexcept
{
    int sizeDifference = int(first.size() - second.size());

    if (sizeDifference == 0)
        return std::memcmp(first.data(), second.data(), first.size());

    return sizeDifference;
}

inline
bool operator<(SmallStringView first, SmallStringView second) noexcept
{
    return compare(first, second) < 0;
}

inline
bool operator>(SmallStringView first, SmallStringView second) noexcept
{
    return second < first;
}

namespace Internal {
inline
int reverse_memcmp(const char *first, const char *second, size_t n)
{

    const char *currentFirst = first + n - 1;
    const char *currentSecond = second + n - 1;

    while (n > 0)
    {
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
}

inline
int reverseCompare(SmallStringView first, SmallStringView second) noexcept
{
    int sizeDifference = int(first.size() - second.size());

    if (sizeDifference == 0)
        return Internal::reverse_memcmp(first.data(), second.data(), first.size());

    return sizeDifference;
}

} // namespace Utils

#ifdef __cpp_user_defined_literals
inline
constexpr Utils::SmallStringView operator""_sv(const char *const string, size_t size)
{
    return Utils::SmallStringView(string, size);
}
#endif
