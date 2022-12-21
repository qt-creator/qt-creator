// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cstddef>
#include <iterator>

namespace Utils {

namespace Internal {

template<class Category,
         class Type,
         typename DistanceType = ptrdiff_t,
         typename Pointer = Type *,
         typename Reference = Type &>
struct SmallStringIterator
{
    using iterator_category = Category;
    using value_type = Type;
    using difference_type = DistanceType;
    using pointer = Pointer;
    using reference = Reference;

    constexpr SmallStringIterator() noexcept = default;

    constexpr SmallStringIterator(Pointer ptr) noexcept
        : pointer_(ptr)
    {}

    constexpr SmallStringIterator operator++() noexcept { return ++pointer_; }

    constexpr SmallStringIterator operator++(int) noexcept { return pointer_++; }

    constexpr SmallStringIterator operator--() noexcept { return --pointer_; }

    constexpr SmallStringIterator operator--(int) noexcept { return pointer_--; }

    constexpr SmallStringIterator operator+(DistanceType difference) const noexcept
    {
        return pointer_ + difference;
    }

    constexpr SmallStringIterator operator-(DistanceType difference) const noexcept
    {
        return pointer_ - difference;
    }

    constexpr SmallStringIterator operator+(std::size_t difference) const noexcept
    {
        return pointer_ + difference;
    }

    constexpr SmallStringIterator operator-(std::size_t difference) const noexcept
    {
        return pointer_ - difference;
    }

    constexpr DistanceType operator-(SmallStringIterator other) const noexcept
    {
        return pointer_ - other.data();
    }

    constexpr SmallStringIterator operator+=(DistanceType difference) noexcept
    {
        return pointer_ += difference;
    }

    constexpr SmallStringIterator operator-=(DistanceType difference) noexcept
    {
        return pointer_ -= difference;
    }

    constexpr Reference operator*() noexcept { return *pointer_; }

    const Reference operator*() const noexcept { return *pointer_; }

    constexpr Pointer operator->() noexcept { return pointer_; }

    constexpr const Pointer operator->() const noexcept { return pointer_; }

    constexpr bool operator==(SmallStringIterator other) const noexcept
    {
        return pointer_ == other.pointer_;
    }

    constexpr bool operator!=(SmallStringIterator other) const noexcept
    {
        return pointer_ != other.pointer_;
    }

    constexpr bool operator<(SmallStringIterator other) const noexcept
    {
        return pointer_ < other.pointer_;
    }

    constexpr Pointer data() noexcept { return pointer_; }

    constexpr const Pointer data() const noexcept { return pointer_; }

private:
    Pointer pointer_ = nullptr;
};

} // namespace Internal

} // namespace Utils
