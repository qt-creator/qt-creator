// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "celliterator.h"

#include "terminalsurface.h"

#include <stdexcept>

namespace Terminal::Internal {

CellIterator::CellIterator(const TerminalSurface *surface, QPoint pos)
    : m_state(State::INSIDE)
    , m_surface(surface)
{
    m_pos = (pos.x()) + (pos.y() * surface->liveSize().width());
    m_maxpos = surface->fullSize().width() * (surface->fullSize().height()) - 1;
    updateChar();
}

CellIterator::CellIterator(const TerminalSurface *surface, int pos)
    : m_state(State::INSIDE)
    , m_surface(surface)
{
    m_maxpos = surface->fullSize().width() * (surface->fullSize().height()) - 1;
    m_pos = qMin(m_maxpos + 1, pos);
    if (m_pos == 0) {
        m_state = State::BEGIN;
    } else if (m_pos == m_maxpos + 1) {
        m_state = State::END;
    }
    updateChar();
}

CellIterator::CellIterator(const TerminalSurface *surface, State state)
    : m_state(state)
    , m_surface(surface)
    , m_pos()
{
    m_maxpos = surface->fullSize().width() * (surface->fullSize().height()) - 1;
    if (state == State::END) {
        m_pos = m_maxpos + 1;
    }
}

QPoint CellIterator::gridPos() const
{
    return m_surface->posToGrid(m_pos);
}

void CellIterator::updateChar()
{
    QPoint cell = m_surface->posToGrid(m_pos);
    m_char = m_surface->fetchCharAt(cell.x(), cell.y());
}

CellIterator &CellIterator::operator-=(int n)
{
    if (n == 0)
        return *this;

    if (m_pos - n < 0)
        throw new std::runtime_error("-= n too big!");

    m_pos -= n;
    updateChar();

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

    if (m_pos + n < m_maxpos) {
        m_state = State::INSIDE;
        m_pos += n;
        updateChar();
    } else {
        *this = m_surface->end();
    }
    return *this;
}

} // namespace Terminal::Internal
