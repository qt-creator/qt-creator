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
