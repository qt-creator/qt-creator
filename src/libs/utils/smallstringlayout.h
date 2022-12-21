// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <type_traits>

namespace Utils {

namespace Internal {

using size_type = std::size_t;

template <uint MaximumShortStringDataAreaSize,
          typename ControlType = typename std::conditional_t<(MaximumShortStringDataAreaSize < 64), uint8_t, uint16_t>>
struct ControlBlock
{
    using SizeType = ControlType;
    constexpr ControlBlock() noexcept = default;
    constexpr ControlBlock(size_type shortStringSize,
                           bool isReadOnlyReference,
                           bool isReference) noexcept
        : m_shortStringSize(static_cast<SizeType>(shortStringSize))
        , m_isReadOnlyReference(isReadOnlyReference)
        , m_isReference(isReference)
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

struct ReferenceLayout
{    union {
        const char *constPointer;
        char *pointer;
    };
    size_type size;
    size_type capacity;
};

template<uint MaximumShortStringDataAreaSize, typename = void>
struct alignas(16) StringDataLayout
{
    static_assert(MaximumShortStringDataAreaSize >= 15, "Size must be greater equal than 15 bytes!");
    static_assert(MaximumShortStringDataAreaSize < 32, "Size must be less than 32 bytes!");
    static_assert(((MaximumShortStringDataAreaSize + 1) % 16) == 0,
                  "Size + 1 must be dividable by 16 if under 64 and Size + 2 must be dividable by "
                  "16 if over 64!");

    StringDataLayout() noexcept { reset(); }

    constexpr StringDataLayout(const char *string, size_type size) noexcept
        : control{0, true, true}
        , reference{{string}, size, 0}
    {}

    template<size_type Size>
    constexpr StringDataLayout(const char (&string)[Size]) noexcept
    {
        if constexpr (Size + 1 <= MaximumShortStringDataAreaSize) {
            control = {Size - 1, false, false};
            for (size_type i = 0; i < Size; ++i)
                shortString[i] = string[i];
        } else {
            control = {0, true, true};
            reference = {string, Size - 1, 0};
        }
    }

    constexpr static size_type shortStringCapacity() noexcept
    {
        return MaximumShortStringDataAreaSize;
    }

    constexpr void reset() { control = ControlBlock<MaximumShortStringDataAreaSize>(); }

#pragma pack(push)
#pragma pack(1)
    ControlBlock<MaximumShortStringDataAreaSize> control;
    union {
        char shortString[MaximumShortStringDataAreaSize];
        struct
        {
            char dummy[sizeof(void *) - sizeof(ControlBlock<MaximumShortStringDataAreaSize>)];
            ReferenceLayout reference;
        };
    };
#pragma pack(pop)
};

template<uint MaximumShortStringDataAreaSize>
struct alignas(16) StringDataLayout<MaximumShortStringDataAreaSize,
                                    std::enable_if_t<MaximumShortStringDataAreaSize >= 32>>
{
    static_assert(MaximumShortStringDataAreaSize > 31, "Size must be greater than 31 bytes!");
    static_assert(MaximumShortStringDataAreaSize < 64
                      ? ((MaximumShortStringDataAreaSize + 1) % 16) == 0
                      : ((MaximumShortStringDataAreaSize + 2) % 16) == 0,
                  "Size + 1 must be dividable by 16 if under 64 and Size + 2 must be dividable by "
                  "16 if over 64!");

    StringDataLayout() noexcept { reset(); }

    constexpr StringDataLayout(const char *string, size_type size) noexcept
        : control{0, true, true}
        , reference{{string}, size, 0}
    {}

    template<size_type Size>
    constexpr StringDataLayout(const char (&string)[Size]) noexcept
    {
        if constexpr (Size + 1 <= MaximumShortStringDataAreaSize) {
            control = {Size - 1, false, false};
            for (size_type i = 0; i < Size; ++i)
                shortString[i] = string[i];
        } else {
            control = {0, true, true};
            reference = {string, Size - 1, 0};
        }
    }

    StringDataLayout(const StringDataLayout &other) noexcept { *this = other; }

    StringDataLayout &operator=(const StringDataLayout &other) noexcept
    {
        constexpr auto controlBlockSize = sizeof(ControlBlock<MaximumShortStringDataAreaSize>);
        auto shortStringLayoutSize = other.control.stringSize() + controlBlockSize;
        constexpr auto referenceLayoutSize = sizeof(ReferenceLayout);
        std::memcpy(this, &other, std::max(shortStringLayoutSize, referenceLayoutSize));

        return *this;
    }

    constexpr static size_type shortStringCapacity() noexcept
    {
        return MaximumShortStringDataAreaSize;
    }

    constexpr void reset() { control = ControlBlock<MaximumShortStringDataAreaSize>(); }

#pragma pack(push)
#pragma pack(1)
    ControlBlock<MaximumShortStringDataAreaSize> control;
    union {
        char shortString[MaximumShortStringDataAreaSize];
        struct
        {
            char dummy[sizeof(void *) - sizeof(ControlBlock<MaximumShortStringDataAreaSize>)];
            ReferenceLayout reference;
        };
    };
#pragma pack(pop)
};

} // namespace Internal
}  // namespace Utils
