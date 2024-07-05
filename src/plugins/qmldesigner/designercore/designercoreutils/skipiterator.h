// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <iterator>

namespace QmlDesigner {

template<typename BaseIterator, typename Increment>
class SkipIterator
{
public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type = qsizetype;
    using value_type = typename BaseIterator::value_type;
    using pointer = typename BaseIterator::pointer;
    using reference = typename BaseIterator::reference;

    SkipIterator() = default;
    SkipIterator(BaseIterator current, Increment increment)
        : m_current{current}
        , m_increment{std::move(increment)}
    {}

    SkipIterator operator++()
    {
        m_current = m_increment(m_current);
        return *this;
    }

    SkipIterator operator++(int)
    {
        auto tmp = *this;
        m_current = m_increment(m_current);
        return tmp;
    }

    reference operator*() const { return *m_current; }
    pointer operator->() const { return m_current.operator->(); }

    friend bool operator==(const SkipIterator &first, const SkipIterator &second)
    {
        return first.m_current == second.m_current;
    }

    friend bool operator!=(const SkipIterator &first, const SkipIterator &second)
    {
        return first.m_current != second.m_current;
    }

private:
    BaseIterator m_current = {};
    Increment m_increment;
};

template<typename Container, typename Increment>
class SkipRange
{
public:
    using container = Container;
    using base = SkipRange<Container, Increment>;
    using iterator = SkipIterator<typename Container::const_iterator, Increment>;

    SkipRange(const Container &container, Increment increment)
        : m_begin{container.begin(), increment}
        , m_end{container.end(), increment}
    {}

    iterator begin() const { return m_begin; }
    iterator end() const { return m_end; }

private:
    iterator m_begin;
    iterator m_end;
};
} // namespace QmlDesigner
