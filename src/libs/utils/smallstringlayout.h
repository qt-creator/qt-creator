// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "smallstringmemory.h"

#include <QtGlobal>

#include <algorithm>
#include <charconv>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
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

    StringDataLayout(const char *string, size_type size, size_type capacity) noexcept
    {
        if (Q_LIKELY(capacity <= shortStringCapacity())) {
            control = Internal::ControlBlock<MaximumShortStringDataAreaSize>(size, false, false);
            std::char_traits<char>::copy(shortString, string, size);
        } else {
            auto pointer = Memory::allocate(capacity);
            std::char_traits<char>::copy(pointer, string, size);
            initializeLongString(pointer, size, capacity);
        }
    }

    constexpr void initializeLongString(char *pointer, size_type size, size_type capacity) noexcept
    {
        control = Internal::ControlBlock<MaximumShortStringDataAreaSize>(0, false, true);
        reference.pointer = pointer;
        reference.size = size;
        reference.capacity = capacity;
    }

    static constexpr size_type shortStringCapacity() noexcept
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

    void setSize(size_type size) noexcept
    {
        if (isShortString())
            control.setShortStringSize(size);
        else
            reference.size = size;
    }

    size_type size() const noexcept
    {
        if (!control.isShortString())
            return reference.size;

        return control.shortStringSize();
    }

    bool isShortString() const noexcept { return control.isShortString(); }

    size_type capacity() const noexcept
    {
        if (!isShortString())
            return reference.capacity;

        return shortStringCapacity();
    }

    constexpr bool isReadOnlyReference() const noexcept { return control.isReadOnlyReference(); }

    char *data() noexcept { return Q_LIKELY(isShortString()) ? shortString : reference.pointer; }

    const char *data() const noexcept
    {
        return Q_LIKELY(isShortString()) ? shortString : reference.pointer;
    }

    void setPointer(char *p) noexcept { reference.pointer = p; }

    void setAllocatedCapacity(size_type capacity) noexcept { reference.capacity = capacity; }

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
    static_assert(((MaximumShortStringDataAreaSize + 16) % 16) == 0,
                  "Size + 16 must be dividable by 16!");

    constexpr StringDataLayout() noexcept { reset(); }

    constexpr StringDataLayout(const char *string, size_type size) noexcept
        : constPointer{string}
        , size_{static_cast<int>(size)}
        , capacity_{0}
    {}

    template<size_type Size>
    constexpr StringDataLayout(const char (&string)[Size]) noexcept
    {
        constexpr size_type size = Size - 1;
        size_ = size;
        if constexpr (size <= MaximumShortStringDataAreaSize) {
            pointer = buffer;
            capacity_ = MaximumShortStringDataAreaSize;
            for (size_type i = 0; i < size; ++i)
                buffer[i] = string[i];
#if defined(__cpp_lib_is_constant_evaluated) && __cpp_lib_is_constant_evaluated >= 201811L
            if (std::is_constant_evaluated()) {
                for (size_type i = size; i < MaximumShortStringDataAreaSize; ++i)
                    buffer[i] = 0;
            }
#endif
        } else {
            constPointer = string;
            capacity_ = 0;
        }
    }

    void copyHere(const StringDataLayout &other) noexcept
    {
        auto isShortString = other.isShortString();
        auto shortStringSize = isShortString ? other.size() : 0;
        auto areaSize = 8 + shortStringSize;
        pointer = isShortString ? buffer : other.pointer;
        std::memcpy(&size_, &other.size_, areaSize);
    }

    StringDataLayout(const StringDataLayout &other) noexcept { copyHere(other); }

    StringDataLayout &operator=(const StringDataLayout &other) noexcept
    {
        copyHere(other);

        return *this;
    }

    StringDataLayout(const char *string, size_type size, size_type capacity) noexcept
        : size_{static_cast<int>(size)}
        , capacity_{std::max<int>(capacity, MaximumShortStringDataAreaSize)}
    {
        if (Q_LIKELY(capacity <= shortStringCapacity())) {
            std::char_traits<char>::copy(buffer, string, size);
            pointer = buffer;
        } else {
            pointer = Memory::allocate(capacity);
            std::char_traits<char>::copy(pointer, string, size);
        }
    }
    constexpr static size_type shortStringCapacity() noexcept
    {
        return MaximumShortStringDataAreaSize;
    }

    constexpr void reset() noexcept
    {
        pointer = buffer;
        size_ = 0;
        capacity_ = MaximumShortStringDataAreaSize;
    }

    void setSize(size_type size) noexcept { size_ = size; }

    size_type size() const noexcept { return size_; }

    bool isShortString() const noexcept { return pointer == buffer; }

    size_type capacity() const noexcept { return capacity_; }

    constexpr bool isReadOnlyReference() const noexcept { return capacity_ == 0; }

    char *data() noexcept { return pointer; }

    const char *data() const noexcept { return constPointer; }

    void setPointer(char *p) noexcept { pointer = p; }

    void setAllocatedCapacity(size_type capacity) noexcept { capacity_ = capacity; }

    constexpr size_type shortStringSize() const noexcept { return size_; }

    union {
        char *pointer;
        const char *constPointer;
    };

    int size_;
    int capacity_;
    char buffer[MaximumShortStringDataAreaSize];
};

} // namespace Internal
}  // namespace Utils

QT_WARNING_POP
