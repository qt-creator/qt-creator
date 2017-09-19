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
#include "smallstringlayout.h"
#include "smallstringview.h"

namespace Utils {

template <int Size>
class BasicSmallStringLiteral
{
    template<uint>
    friend class BasicSmallString;

public:
    using const_iterator = Internal::SmallStringIterator<std::random_access_iterator_tag, const char>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using size_type = std::size_t;

    template<size_type ArraySize>
    constexpr
    BasicSmallStringLiteral(const char(&string)[ArraySize]) noexcept
        : m_data(string)
    {
        static_assert(ArraySize >= 1, "Invalid string literal! Length is zero!");
    }

    constexpr
    BasicSmallStringLiteral(const char *string, const size_type size) noexcept
        : m_data(string, size)
    {
    }

    const char *data() const noexcept
    {
        return Q_LIKELY(isShortString()) ? m_data.shortString.string : m_data.allocated.data.pointer;
    }

    size_type size() const noexcept
    {
        return Q_LIKELY(isShortString()) ? m_data.shortString.shortStringSize : m_data.allocated.data.size;
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

    constexpr static
    size_type shortStringCapacity() noexcept
    {
        return Internal::StringDataLayout<Size>::shortStringCapacity();
    }

    bool isShortString() const noexcept
    {
        return !m_data.shortString.isReference;
    }

    bool isReadOnlyReference() const noexcept
    {
        return m_data.shortString.isReadOnlyReference;
    }

    constexpr
    operator SmallStringView() const
    {
        return SmallStringView(data(), size());
    }

private:
    BasicSmallStringLiteral(const Internal::StringDataLayout<Size> &data) noexcept
        : m_data(data)
    {
    }
private:
    Internal::StringDataLayout<Size> m_data;
};

using SmallStringLiteral = BasicSmallStringLiteral<31>;

}  // namespace Utils
