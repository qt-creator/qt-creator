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

#include <algorithm>
#include <array>
#include <cstdint>
#include <ostream>
#include <iterator>

namespace Utils {

template <typename T, std::uint8_t MaxSize>
class SizedArray : public std::array<T, MaxSize>
{
public:
    using size_type = std::uint8_t;
    using reference = typename std::array<T, MaxSize>::reference;
    using const_reference = typename std::array<T, MaxSize>::const_reference;
    using iterator = typename std::array<T, MaxSize>::iterator;
    using const_iterator = typename std::array<T, MaxSize>::const_iterator;
    using reverse_iterator = typename std::array<T, MaxSize>::reverse_iterator;
    using const_reverse_iterator = typename std::array<T, MaxSize>::const_reverse_iterator;
    using value_type = typename std::array<T, MaxSize>::value_type;
    using pointer = typename std::array<T, MaxSize>::pointer;
    using difference_type = typename std::array<T, MaxSize>::difference_type;

    using std::array<T, MaxSize>::operator[];
    using std::array<T, MaxSize>::begin;
    using std::array<T, MaxSize>::cbegin;
    using std::array<T, MaxSize>::rend;
    using std::array<T, MaxSize>::crend;

    constexpr SizedArray() = default;
    SizedArray(std::initializer_list<T> list)
        : m_size(std::uint8_t(list.size()))
    {
        std::copy(list.begin(), list.end(), begin());
    }

    constexpr size_type size() const
    {
        return m_size;
    }

    void push_back(const T &value)
    {
        operator[](m_size) = value;

        ++m_size;
    }

    constexpr const_reference back() const
    {
        return operator[](m_size - 1);
    }

    iterator end()
    {
        return std::next(begin(), m_size);
    }

    const_iterator end() const
    {
        return std::next(begin(), m_size);
    }

    const_iterator cend() const
    {
        return std::next(cbegin(), m_size);
    }

    reverse_iterator rbegin()
    {
        return std::prev(rend(), m_size);
    }

    const_reverse_iterator rbegin() const
    {
        return std::prev(rend(), m_size);
    }

    const_reverse_iterator crbegin() const
    {
        return std::prev(crend(), m_size);
    }

    bool empty() const
    {
        return m_size == 0;
    }

    void initializeElements()
    {
        std::array<T, MaxSize>::fill(T{});
    }

    bool contains(const T &item) const
    {
        return std::any_of(begin(), end(), [&item](const T &current) {
            return item == current;
        });
    }

    friend std::ostream &operator<<(std::ostream &out, SizedArray array)
    {
        out << "[";
        copy(array.cbegin(), array.cend(), std::ostream_iterator<T>(out, ", "));
        out << "]";

        return out;
    }

private:
    std::uint8_t m_size = 0;
};

}  // namespace Utils
