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
#include <type_traits>

#if defined(Q_CC_MSVC) && !defined(_WIN64)
#   define ALIGNAS_16
#else
#   define ALIGNAS_16 alignas(16)
#endif

namespace Utils {

namespace Internal {

using size_type = std::size_t;

template<bool Bool>
struct block_type
{
    using type = uint8_t;
};

template<>
struct block_type<false> {
    using type = uint16_t;
};

template <uint MaximumShortStringDataAreaSize,
          typename BlockType = typename block_type<(MaximumShortStringDataAreaSize < 64)>::type>
struct AllocatedLayout {
    struct Data {
        char *pointer;
        size_type size;
        size_type capacity;
    } data;
    char dummy[MaximumShortStringDataAreaSize - sizeof(Data)];
    BlockType shortStringSize : (sizeof(BlockType) * 8) - 2;
    BlockType isReadOnlyReference : 1;
    BlockType isReference : 1;
};

template <uint MaximumShortStringDataAreaSize,
          typename BlockType = typename block_type<(MaximumShortStringDataAreaSize < 64)>::type>
struct ReferenceLayout {
    struct Data {
        const char *pointer;
        size_type size;
        size_type capacity;
    } data;
    char dummy[MaximumShortStringDataAreaSize - sizeof(Data)];
    BlockType shortStringSize : (sizeof(BlockType) * 8) - 2;
    BlockType isReadOnlyReference : 1;
    BlockType isReference : 1;
};

template <uint MaximumShortStringDataAreaSize,
          typename BlockType = typename block_type<(MaximumShortStringDataAreaSize < 64)>::type>
struct ShortStringLayout {
    char string[MaximumShortStringDataAreaSize];
    BlockType shortStringSize : (sizeof(BlockType) * 8) - 2;
    BlockType isReadOnlyReference : 1;
    BlockType isReference : 1;
};

template <uint MaximumShortStringDataAreaSize>
struct ALIGNAS_16 StringDataLayout {
    static_assert(MaximumShortStringDataAreaSize >= 15, "Size must be greater equal than 15 bytes!");
    static_assert(MaximumShortStringDataAreaSize < 64
                ? ((MaximumShortStringDataAreaSize + 1) % 16) == 0
                : ((MaximumShortStringDataAreaSize + 2) % 16) == 0,
                  "Size + 1 must be dividable by 16 if under 64 and Size + 2 must be dividable by 16 if over 64!");

    StringDataLayout() noexcept = default;

    constexpr StringDataLayout(const char *string,
                               size_type size) noexcept
       : reference({{string, size, 0}, {}, 0, true, true})
    {
    }

    template<size_type Size>
    constexpr StringDataLayout(const char(&string)[Size]) noexcept
#if __cpp_constexpr < 201304
        : reference({{string, Size - 1, 0}, {}, 0, true, true})
#endif
    {
#if __cpp_constexpr >= 201304
        if (Size <= MaximumShortStringDataAreaSize) {
            for (size_type i = 0; i < Size; ++i)
                shortString.string[i] = string[i];

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverflow"
            shortString.shortStringSize = std::uint8_t(Size) - 1;
#pragma GCC diagnostic pop
#endif
            shortString.isReference = false;
            shortString.isReadOnlyReference = false;
        } else {
            reference.data.pointer = string;
            reference.data.size = Size - 1;
            reference.data.capacity = 0;
            reference.shortStringSize = 0;
            reference.isReference = true;
            reference.isReadOnlyReference = true;
        }
#endif
    }

    constexpr static
    size_type shortStringCapacity() noexcept
    {
        return MaximumShortStringDataAreaSize - 1;
    }

    union {
        AllocatedLayout<MaximumShortStringDataAreaSize> allocated;
        ReferenceLayout<MaximumShortStringDataAreaSize> reference;
        ShortStringLayout<MaximumShortStringDataAreaSize> shortString = ShortStringLayout<MaximumShortStringDataAreaSize>();
    };
};

} // namespace Internal
}  // namespace Utils
