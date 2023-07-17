// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "terminal_global.h"

#include <string>

#include <QPoint>

namespace TerminalSolution {

class TerminalSurface;

class TERMINAL_EXPORT CellIterator
{
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = std::u32string::value_type;
    using pointer = std::u32string::value_type *;
    // We need to return copies for std::reverse_iterator to work
    using reference = std::u32string::value_type;

    enum class State { BEGIN, INSIDE, END } m_state{};

public:
    CellIterator(const TerminalSurface *surface, QPoint pos);
    CellIterator(const TerminalSurface *surface, int pos);
    CellIterator(const TerminalSurface *surface);

public:
    QPoint gridPos() const;

public:
    CellIterator &operator-=(int n);
    CellIterator &operator+=(int n);

    reference operator*() const { return m_char; }
    pointer operator->() { return &m_char; }

    CellIterator &operator++() { return *this += 1; }
    CellIterator operator++(int)
    {
        CellIterator tmp = *this;
        ++(*this);
        return tmp;
    }

    CellIterator &operator--() { return *this -= 1; }
    CellIterator operator--(int)
    {
        CellIterator tmp = *this;
        --(*this);
        return tmp;
    }

    bool operator!=(const CellIterator &other) const
    {
        if (other.m_state != m_state)
            return true;

        if (other.m_pos != m_pos)
            return true;

        return false;
    }

    bool operator==(const CellIterator &other) const { return !operator!=(other); }

    CellIterator operator-(int n) const
    {
        CellIterator result = *this;
        result -= n;
        return result;
    }

    CellIterator operator+(int n) const
    {
        CellIterator result = *this;
        result += n;
        return result;
    }

    int position() const { return m_pos; }
    void setSkipZeros(bool skipZeros) { m_skipZeros = skipZeros; }

private:
    bool updateChar();

    const TerminalSurface *m_surface{nullptr};
    int m_pos{-1};
    int m_maxpos{-1};
    bool m_skipZeros{false};
    mutable std::u32string::value_type m_char;
};

} // namespace TerminalSolution
