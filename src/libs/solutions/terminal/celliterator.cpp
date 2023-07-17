// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "celliterator.h"

#include "terminalsurface.h"

#include <stdexcept>

namespace TerminalSolution {

CellIterator::CellIterator(const TerminalSurface *surface, QPoint pos)
    : CellIterator(surface, pos.x() + (pos.y() * surface->liveSize().width()))
{}

CellIterator::CellIterator(const TerminalSurface *surface, int pos)
    : m_state(State::INSIDE)
    , m_surface(surface)
{
    m_maxpos = surface->fullSize().width() * (surface->fullSize().height()) - 1;
    m_pos = qMax(0, qMin(m_maxpos + 1, pos));

    if (m_pos == 0) {
        m_state = State::BEGIN;
    } else if (m_pos == m_maxpos + 1) {
        m_state = State::END;
    }

    if (m_state != State::END)
        updateChar();
}

CellIterator::CellIterator(const TerminalSurface *surface)
    : m_state(State::END)
    , m_surface(surface)
{
    m_maxpos = surface->fullSize().width() * (surface->fullSize().height()) - 1;
    m_pos = m_maxpos + 1;
}

QPoint CellIterator::gridPos() const
{
    return m_surface->posToGrid(m_pos);
}

bool CellIterator::updateChar()
{
    QPoint cell = m_surface->posToGrid(m_pos);
    m_char = m_surface->fetchCharAt(cell.x(), cell.y());
    return m_char != 0;
}

CellIterator &CellIterator::operator-=(int n)
{
    if (n == 0)
        return *this;

    if (m_pos - n < 0)
        throw new std::runtime_error("-= n too big!");

    m_pos -= n;

    while (!updateChar() && m_pos > 0 && m_skipZeros)
        m_pos--;

    m_state = State::INSIDE;

    if (m_pos == 0) {
        m_state = State::BEGIN;
    }

    return *this;
}

CellIterator &CellIterator::operator+=(int n)
{
    if (n == 0)
        return *this;

    if (m_pos + n < m_maxpos + 1) {
        m_state = State::INSIDE;
        m_pos += n;
        while (!updateChar() && m_pos < (m_maxpos + 1) && m_skipZeros)
            m_pos++;

        if (m_pos == m_maxpos + 1)
            m_state = State::END;
    } else {
        *this = m_surface->end();
    }
    return *this;
}

} // namespace TerminalSolution
