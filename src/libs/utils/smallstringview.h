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

#include <QtGlobal>

#include <cstring>

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

    static
    SmallStringView fromUtf8(const char *const characterPointer)
    {
        return SmallStringView(characterPointer, std::strlen(characterPointer));
    }

    constexpr
    const char *data() const
    {
        return m_pointer;
    }

    constexpr
    size_type size() const
    {
        return m_size;
    }

    const_iterator begin() const noexcept
    {
        return data();
    }

    const_iterator end() const noexcept
    {
        return data() + size();
    }

    const_reverse_iterator rbegin() const noexcept
    {
        return const_reverse_iterator(end() - static_cast<std::size_t>(1));
    }

    const_reverse_iterator rend() const noexcept
    {
        return const_reverse_iterator(begin() - static_cast<std::size_t>(1));
    }

private:
    const char *m_pointer;
    size_type m_size;
};

inline
bool operator==(const SmallStringView& first, const SmallStringView& second) noexcept
{
    if (Q_LIKELY(first.size() != second.size()))
        return false;

    return !std::memcmp(first.data(), second.data(), first.size());
}

inline
bool operator!=(const SmallStringView& first, const SmallStringView& second) noexcept
{
    return !(first == second);
}

} // namespace Utils
