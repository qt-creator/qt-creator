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

#pragma push_macro("constexpr")
#ifndef __cpp_constexpr
#define constexpr
#endif

#pragma push_macro("noexcept")
#ifndef __cpp_noexcept
#define noexcept
#endif

#ifdef __cpp_alignas
#define ALIGNAS_16 alignas(16)
#else
#define ALIGNAS_16
#endif

namespace Utils {

namespace Internal {

using size_type = std::size_t;

static const int smallStringLayoutByteSize = 32;
static const int maximumShortStringDataAreaSize = smallStringLayoutByteSize - 1;

struct AllocatedLayout {
    struct Data {
        char *pointer;
        size_type size;
        size_type capacity;
    } data;
    char dummy[maximumShortStringDataAreaSize - sizeof(Data)];
    std::uint8_t shortStringSize: 6;
    std::uint8_t isReadOnlyReference : 1;
    std::uint8_t isReference : 1;
};

struct ReferenceLayout {
    struct Data {
        const char *pointer;
        size_type size;
        size_type capacity;
    } data;
    char dummy[maximumShortStringDataAreaSize - sizeof(Data)];
    std::uint8_t shortStringSize: 6;
    std::uint8_t isReadOnlyReference : 1;
    std::uint8_t isReference : 1;
};

struct ShortStringLayout {
    char string[maximumShortStringDataAreaSize];
    std::uint8_t shortStringSize: 6;
    std::uint8_t isReadOnlyReference : 1;
    std::uint8_t isReference : 1;
};

struct ALIGNAS_16 StringDataLayout {
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
        if (Size <= maximumShortStringDataAreaSize) {
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



    union {
        AllocatedLayout allocated;
        ReferenceLayout reference;
        ShortStringLayout shortString = {};
    };
};

} // namespace Internal
}  // namespace Utils

#pragma pop_macro("noexcept")
#pragma pop_macro("constexpr")
