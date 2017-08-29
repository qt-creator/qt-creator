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

template <class Category,
          class Type,
          typename DistanceType = ptrdiff_t,
          typename Pointer = Type*,
          typename Reference = Type&>
struct SmallStringIterator : public  std::iterator<Category, Type, DistanceType, Pointer, Reference>
{
    constexpr
    SmallStringIterator() noexcept = default;

    constexpr
    SmallStringIterator(Pointer ptr) noexcept : pointer_(ptr)
    {
    }

    SmallStringIterator operator++() noexcept
    {
        return ++pointer_;
    }
    SmallStringIterator operator++(int) noexcept
    {
        return pointer_++;
    }

    SmallStringIterator operator--() noexcept
    {
        return --pointer_;
    }

    SmallStringIterator operator--(int) noexcept
    {
        return pointer_--;
    }

    SmallStringIterator operator+(DistanceType difference) const noexcept
    {
        return pointer_ + difference;
    }

    SmallStringIterator operator-(DistanceType difference) const noexcept
    {
        return pointer_ - difference;
    }

    SmallStringIterator operator+(std::size_t difference) const noexcept
    {
        return pointer_ + difference;
    }

    SmallStringIterator operator-(std::size_t difference) const noexcept
    {
        return pointer_ - difference;
    }

    DistanceType operator-(SmallStringIterator other) const noexcept
    {
        return pointer_ - other.data();
    }

    SmallStringIterator operator+=(DistanceType difference) noexcept
    {
        return pointer_ += difference;
    }

    SmallStringIterator operator-=(DistanceType difference) noexcept
    {
        return pointer_ -= difference;
    }

    Reference operator*() noexcept
    {
        return *pointer_;
    }

    const Reference operator*() const noexcept
    {
        return *pointer_;
    }

    Pointer operator->() noexcept
    {
        return pointer_;
    }

    const Pointer operator->() const noexcept
    {
        return pointer_;
    }

    bool operator==(SmallStringIterator other) const noexcept
    {
        return pointer_ == other.pointer_;
    }

    bool operator!=(SmallStringIterator other) const noexcept
    {
        return pointer_ != other.pointer_;
    }

    bool operator<(SmallStringIterator other) const noexcept
    {
        return pointer_ < other.pointer_;
    }

    Pointer data() noexcept
    {
        return pointer_;
    }

private:
    Pointer pointer_ = nullptr;
};

} // namespace Internal

} // namespace Utils
