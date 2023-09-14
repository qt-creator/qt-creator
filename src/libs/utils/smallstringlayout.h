// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <type_traits>

QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wgnu-anonymous-struct")
QT_WARNING_DISABLE_CLANG("-Wnested-anon-types")

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

    constexpr void setShortStringSize(size_type size) noexcept
    {
       m_shortStringSize = static_cast<SizeType>(size);
    }

    constexpr size_type shortStringSize() const noexcept { return m_shortStringSize; }

    constexpr void setIsReadOnlyReference(bool isReadOnlyReference) noexcept
    {
        m_isReadOnlyReference = isReadOnlyReference;
    }

    constexpr void setIsReference(bool isReference) noexcept { m_isReference = isReference; }

    constexpr void setIsShortString(bool isShortString) noexcept { m_isReference = !isShortString; }

    constexpr SizeType stringSize() const noexcept { return m_shortStringSize; }

    constexpr bool isReadOnlyReference() const noexcept { return m_isReadOnlyReference; }

    constexpr bool isReference() const noexcept { return m_isReference; }

    constexpr bool isShortString() const noexcept { return !m_isReference; }

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

    constexpr StringDataLayout() noexcept { reset(); }

    constexpr StringDataLayout(const char *string, size_type size) noexcept
        : controlReference{0, true, true}
        , reference{{string}, size, 0}
    {}

    template<size_type Size>
    constexpr StringDataLayout(const char (&string)[Size]) noexcept
    {
        constexpr auto size = Size - 1;
        if constexpr (size <= MaximumShortStringDataAreaSize) {
            control = {size, false, false};
            for (size_type i = 0; i < size; ++i)
                shortString[i] = string[i];
#if defined(__cpp_lib_is_constant_evaluated) && __cpp_lib_is_constant_evaluated >= 201811L
            if (std::is_constant_evaluated()) {
                for (size_type i = size; i < MaximumShortStringDataAreaSize; ++i)
                    shortString[i] = 0;
            }
#endif
        } else {
            control = {0, true, true};
            reference = {{string}, size, 0};
        }
    }

    constexpr static size_type shortStringCapacity() noexcept
    {
        return MaximumShortStringDataAreaSize;
    }

    constexpr void reset() noexcept
    {
        control = ControlBlock<MaximumShortStringDataAreaSize>();
#if defined(__cpp_lib_is_constant_evaluated) && __cpp_lib_is_constant_evaluated >= 201811L
        if (std::is_constant_evaluated()) {
            for (size_type i = 0; i < MaximumShortStringDataAreaSize; ++i)
                shortString[i] = 0;
        }
#endif
    }

    union {
        struct
        {
            ControlBlock<MaximumShortStringDataAreaSize> control;
            char shortString[MaximumShortStringDataAreaSize];
        };
        struct
        {
            ControlBlock<MaximumShortStringDataAreaSize> controlReference;
            ReferenceLayout reference;
        };
    };
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

    constexpr StringDataLayout() noexcept { reset(); }

    constexpr StringDataLayout(const char *string, size_type size) noexcept
        : controlReference{0, true, true}
        , reference{{string}, size, 0}
    {}

    template<size_type Size>
    constexpr StringDataLayout(const char (&string)[Size]) noexcept
    {
        constexpr auto size = Size - 1;
        if constexpr (size <= MaximumShortStringDataAreaSize) {
            control = {size, false, false};
            for (size_type i = 0; i < size; ++i)
                shortString[i] = string[i];
#if defined(__cpp_lib_is_constant_evaluated) && __cpp_lib_is_constant_evaluated >= 201811L
            if (std::is_constant_evaluated()) {
                for (size_type i = size; i < MaximumShortStringDataAreaSize; ++i)
                    shortString[i] = 0;
            }
#endif
        } else {
            control = {0, true, true};
            reference = {{string}, size, 0};
        }
    }

    void copyHere(const StringDataLayout &other) noexcept
    {
        constexpr auto controlBlockSize = sizeof(ControlBlock<MaximumShortStringDataAreaSize>);
        auto shortStringLayoutSize = other.control.stringSize() + controlBlockSize;
        constexpr auto referenceLayoutSize = sizeof(ReferenceLayout);
        std::memcpy(this, &other, std::max(shortStringLayoutSize, referenceLayoutSize));
    }

    StringDataLayout(const StringDataLayout &other) noexcept { copyHere(other); }

    StringDataLayout &operator=(const StringDataLayout &other) noexcept
    {
        copyHere(other);

        return *this;
    }

    constexpr static size_type shortStringCapacity() noexcept
    {
        return MaximumShortStringDataAreaSize;
    }

    constexpr void reset() noexcept
    {
        control = ControlBlock<MaximumShortStringDataAreaSize>();
#if defined(__cpp_lib_is_constant_evaluated) && __cpp_lib_is_constant_evaluated >= 201811L
        if (std::is_constant_evaluated()) {
            for (size_type i = 0; i < MaximumShortStringDataAreaSize; ++i)
                shortString[i] = 0;
        }
#endif
    }

    union {
        struct
        {
            ControlBlock<MaximumShortStringDataAreaSize> control;
            char shortString[MaximumShortStringDataAreaSize];
        };
        struct
        {
            ControlBlock<MaximumShortStringDataAreaSize> controlReference;
            ReferenceLayout reference;
        };
    };
};

} // namespace Internal
}  // namespace Utils

QT_WARNING_POP
