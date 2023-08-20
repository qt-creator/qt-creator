// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

    constexpr BasicSmallStringLiteral() noexcept = default;

    template<size_type ArraySize>
    constexpr BasicSmallStringLiteral(const char (&string)[ArraySize]) noexcept
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
        return Q_LIKELY(isShortString()) ? m_data.shortString : m_data.reference.constPointer;
    }

    size_type size() const noexcept
    {
        return Q_LIKELY(isShortString()) ? m_data.control.shortStringSize() : m_data.reference.size;
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

    constexpr bool isShortString() const noexcept { return m_data.control.isShortString(); }

    constexpr
    bool isReadOnlyReference() const noexcept
    {
        return m_data.control.isReadOnlyReference();
    }

    constexpr operator SmallStringView() const noexcept { return SmallStringView(data(), size()); }

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
