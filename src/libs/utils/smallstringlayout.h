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

#include <QtGlobal>

#include <cstdint>
#include <memory>
#include <type_traits>

namespace Utils {

namespace Internal {

using size_type = std::size_t;

template <uint MaximumShortStringDataAreaSize,
          typename ControlType = typename std::conditional_t<(MaximumShortStringDataAreaSize < 64), uint8_t, uint16_t>>
struct ControlBlock
{
    using SizeType =  ControlType;
    constexpr ControlBlock() noexcept = default;
    constexpr ControlBlock(ControlType shortStringSize, bool isReadOnlyReference, bool isReference) noexcept
          : m_shortStringSize(shortStringSize),
            m_isReadOnlyReference(isReadOnlyReference),
            m_isReference(isReference)
    {}

    constexpr void setShortStringSize(size_type size)
    {
       m_shortStringSize = static_cast<SizeType>(size);
    }

    constexpr size_type shortStringSize() const { return m_shortStringSize; }

    constexpr void setIsReadOnlyReference(bool isReadOnlyReference)
    {
        m_isReadOnlyReference = isReadOnlyReference;
    }

    constexpr void setIsReference(bool isReference) { m_isReference = isReference; }

    constexpr void setIsShortString(bool isShortString) { m_isReference = !isShortString; }

    constexpr
    SizeType stringSize() const
    {
        return m_shortStringSize;
    }

    constexpr
    bool isReadOnlyReference() const
    {
        return m_isReadOnlyReference;
    }

    constexpr
    bool isReference() const
    {
        return m_isReference;
    }

    constexpr
    bool isShortString() const
    {
        return !m_isReference;
    }

private:
    ControlType m_shortStringSize : (sizeof(ControlType) * 8) - 2;
    ControlType m_isReadOnlyReference : 1;
    ControlType m_isReference : 1;
};

template <uint MaximumShortStringDataAreaSize>
struct AllocatedLayout
{
    struct Data
    {
        char *pointer;
        size_type size;
        size_type capacity;
    };

    ControlBlock<MaximumShortStringDataAreaSize> control;
    Data data;
};

template <uint MaximumShortStringDataAreaSize>
struct ReferenceLayout
{
    constexpr ReferenceLayout() noexcept = default;
    constexpr ReferenceLayout(const char *stringPointer,
                              size_type size,
                              size_type capacity) noexcept
          : control(0, true, true),
          data{stringPointer, size, capacity}
    {}

    struct Data
    {
        const char *pointer;
        size_type size;
        size_type capacity;
    };

    ControlBlock<MaximumShortStringDataAreaSize> control;
    Data data;
};

template <uint MaximumShortStringDataAreaSize>
struct ShortStringLayout
{
    ControlBlock<MaximumShortStringDataAreaSize> control;
    char string[MaximumShortStringDataAreaSize];
};

template <uint MaximumShortStringDataAreaSize>
struct StringDataLayout {
    static_assert(MaximumShortStringDataAreaSize >= 15, "Size must be greater equal than 15 bytes!");
    static_assert(MaximumShortStringDataAreaSize < 64
                ? ((MaximumShortStringDataAreaSize + 1) % 16) == 0
                : ((MaximumShortStringDataAreaSize + 2) % 16) == 0,
                  "Size + 1 must be dividable by 16 if under 64 and Size + 2 must be dividable by 16 if over 64!");

    StringDataLayout() noexcept
    {
        reset();
    }

    constexpr StringDataLayout(const char *string,
                               size_type size) noexcept
       : reference(string, size, 0)
    {
    }

    template<size_type Size,
             typename std::enable_if_t<Size <= MaximumShortStringDataAreaSize, int> = 0>
    constexpr StringDataLayout(const char (&string)[Size]) noexcept
        : shortString(ShortStringLayout<MaximumShortStringDataAreaSize>{})
    {
       for (size_type i = 0; i < Size; ++i)
           shortString.string[i] = string[i];

       shortString.control.setShortStringSize(Size - 1);
       shortString.control.setIsShortString(true);
       shortString.control.setIsReadOnlyReference(false);
    }

    template<size_type Size,
            typename std::enable_if_t<!(Size <= MaximumShortStringDataAreaSize), int> = 1>
    constexpr StringDataLayout(const char(&string)[Size]) noexcept
        : reference(string, Size - 1, 0)
    {
    }

    constexpr static
    size_type shortStringCapacity() noexcept
    {
        return MaximumShortStringDataAreaSize - 1;
    }

    void reset()
    {
        shortString.control = ControlBlock<MaximumShortStringDataAreaSize>();
        shortString.string[0] = '\0';
    }

    union {
        AllocatedLayout<MaximumShortStringDataAreaSize> allocated;
        ReferenceLayout<MaximumShortStringDataAreaSize> reference;
        ShortStringLayout<MaximumShortStringDataAreaSize> shortString;
    };
};

} // namespace Internal
}  // namespace Utils
