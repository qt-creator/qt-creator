// Copyright (c) 2020, Justin Bronder
// Copied and modified from: https://github.com/jsbronder/sff
// SPDX-License-Identifier: BSD-3-Clause

#include "scrollback.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <vector>

namespace TerminalSolution {

Scrollback::Line::Line(int cols, const VTermScreenCell *cells, bool continuation)
    : m_cols(cols)
    , m_continuation(continuation)
    , m_cells(std::make_unique<VTermScreenCell[]>(cols))
{
    memcpy(m_cells.get(), cells, cols * sizeof(cells[0]));
}

Scrollback::Line::Line(int cols, std::unique_ptr<VTermScreenCell[]> cells, bool continuation)
    : m_cols(cols)
    , m_continuation(continuation)
    , m_cells(std::move(cells))
{}

const VTermScreenCell *Scrollback::Line::cell(int i) const
{
    assert(i >= 0 && i < m_cols);
    return &m_cells[i];
}

Scrollback::Scrollback(size_t capacity)
    : m_capacity(capacity)
{}

void Scrollback::emplace(int cols, const VTermScreenCell *cells, bool continuation)
{
    m_deque.emplace_front(cols, cells, continuation);
    while (m_deque.size() > m_capacity) {
        m_deque.pop_back();
    }
}

void Scrollback::emplace(int cols, std::unique_ptr<VTermScreenCell[]> cells, bool continuation)
{
    m_deque.emplace_front(cols, std::move(cells), continuation);
    while (m_deque.size() > m_capacity) {
        m_deque.pop_back();
    }
}

bool Scrollback::popto(int cols, VTermScreenCell *cells)
{
    if (m_deque.empty())
        return false;

    const Line &sbl = m_deque.front();

    int ncells = cols;
    if (ncells > sbl.cols())
        ncells = sbl.cols();

    memcpy(cells, sbl.cells(), sizeof(cells[0]) * ncells);
    for (size_t i = ncells; i < static_cast<size_t>(cols); ++i) {
        cells[i].chars[0] = '\0';
        cells[i].width = 1;
        cells[i].bg = cells[ncells - 1].bg;
    }

    m_deque.pop_front();
    return true;
}

void Scrollback::clear()
{
    m_deque.clear();
}

} // namespace TerminalSolution
