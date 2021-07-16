/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "utils_global.h"

#include <QtGlobal>

#include <type_traits>

namespace Utils {

/// A class useful for the implementation of the -> operator of proxy iterators.
template <typename Reference>
struct ArrowProxy {
    Reference r;
    Reference *operator->() {
        return &r;
    }
};

/// Random-access const iterator over elements of a container providing an overloaded operator[]
/// (which may return a proxy object, like std::vector<bool>, rather than a reference).
template <typename Container>
class IndexedContainerProxyConstIterator
{
public:
    typedef std::random_access_iterator_tag iterator_category;
    typedef typename std::make_signed<typename Container::size_type>::type difference_type;
    typedef typename Container::value_type value_type;
    typedef value_type reference;
    typedef ArrowProxy<reference> pointer;
    typedef typename Container::size_type size_type;

    IndexedContainerProxyConstIterator()
      : m_container(nullptr), m_index(0)
    {}

    IndexedContainerProxyConstIterator(const Container &container, size_type index)
      : m_container(&container), m_index(index)
    {}

    reference operator*() const
    {
        Q_ASSERT(m_container);
        return (*m_container)[m_index];
    }

    pointer operator->() const
    {
        Q_ASSERT(m_container);
        return pointer{(*m_container)[m_index]};
    }

    reference operator[](difference_type j) const
    {
        Q_ASSERT(m_container);
        return (*m_container)[m_index + j];
    }

    bool operator==(const IndexedContainerProxyConstIterator &other) const
    {
        Q_ASSERT(m_container == other.m_container);
        return m_index == other.m_index;
    }

    bool operator!=(const IndexedContainerProxyConstIterator &other) const
    {
        Q_ASSERT(m_container == other.m_container);
        return m_index != other.m_index;
    }

    bool operator<(const IndexedContainerProxyConstIterator &other) const
    {
        Q_ASSERT(m_container == other.m_container);
        return m_index < other.m_index;
    }

    bool operator<=(const IndexedContainerProxyConstIterator &other) const
    {
        Q_ASSERT(m_container == other.m_container);
        return m_index <= other.m_index;
    }

    bool operator>(const IndexedContainerProxyConstIterator &other) const
    {
        Q_ASSERT(m_container == other.m_container);
        return m_index > other.m_index;
    }

    bool operator>=(const IndexedContainerProxyConstIterator &other) const
    {
        Q_ASSERT(m_container == other.m_container);
        return m_index >= other.m_index;
    }

    IndexedContainerProxyConstIterator &operator++()
    {
        ++m_index;
        return *this;
    }

    IndexedContainerProxyConstIterator operator++(int)
    {
        IndexedContainerProxyConstIterator copy(*this);
        ++m_index;
        return copy;
    }

    IndexedContainerProxyConstIterator &operator--()
    {
        --m_index;
        return *this;
    }

    IndexedContainerProxyConstIterator operator--(int)
    {
        IndexedContainerProxyConstIterator copy(*this);
        --m_index;
        return copy;
    }

    IndexedContainerProxyConstIterator &operator+=(difference_type j)
    {
        m_index += j;
        return *this;
    }

    IndexedContainerProxyConstIterator &operator-=(difference_type j)
    {
        m_index -= j;
        return *this;
    }

    IndexedContainerProxyConstIterator operator+(difference_type j) const
    {
        IndexedContainerProxyConstIterator result(*this);
        result += j;
        return result;
    }

    IndexedContainerProxyConstIterator operator-(difference_type j) const
    {
        IndexedContainerProxyConstIterator result(*this);
        result -= j;
        return result;
    }

    friend IndexedContainerProxyConstIterator operator+(
            difference_type j, const IndexedContainerProxyConstIterator &other)
    {
        return other + j;
    }

    difference_type operator-(const IndexedContainerProxyConstIterator &other) const
    {
        return static_cast<difference_type>(m_index) - static_cast<difference_type>(other.m_index);
    }

private:
    const Container *m_container;
    size_type m_index;
};

} // namespace Utils
