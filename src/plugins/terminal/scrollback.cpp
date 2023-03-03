// Copyright (c) 2020, Justin Bronder
// Copied and modified from: https://github.com/jsbronder/sff
// SPDX-License-Identifier: BSD-3-Clause

#include "scrollback.h"
#include "celllayout.h"

#include <cassert>
#include <cstring>
#include <future>

namespace Terminal::Internal {

Scrollback::Line::Line(int cols, const VTermScreenCell *cells, VTermState *vts)
    : m_cols(cols)
    , m_cells(std::make_unique<VTermScreenCell[]>(cols))
{
    m_textFuture = std::async(std::launch::async, [this, cols] {
        std::u32string text;
        text.reserve(cols);
        for (int i = 0; i < cols; ++i) {
            text += cellToString(m_cells[i]);
        }
        return text;
    });

    memcpy(m_cells.get(), cells, cols * sizeof(cells[0]));
    for (int i = 0; i < cols; ++i) {
        vterm_state_convert_color_to_rgb(vts, &m_cells[i].fg);
        vterm_state_convert_color_to_rgb(vts, &m_cells[i].bg);
    }
}

const VTermScreenCell *Scrollback::Line::cell(int i) const
{
    assert(i >= 0 && i < m_cols);
    return &m_cells[i];
}

const QTextLayout &Scrollback::Line::layout(int version, const QFont &font, qreal lineSpacing) const
{
    if (!m_layout)
        m_layout = std::make_unique<QTextLayout>();

    if (m_layoutVersion != version) {
        VTermColor defaultBg;
        defaultBg.type = VTERM_COLOR_DEFAULT_BG;
        m_layout->clearLayout();
        m_layout->setFont(font);
        createTextLayout(*m_layout,
                         nullptr,
                         defaultBg,
                         QRect(0, 0, m_cols, 1),
                         lineSpacing,
                         [this](int x, int) { return &m_cells[x]; });
        m_layoutVersion = version;
    }
    return *m_layout;
}

const std::u32string &Scrollback::Line::text() const
{
    if (!m_text)
        m_text = m_textFuture.get();
    return *m_text;
}

Scrollback::Scrollback(size_t capacity)
    : m_capacity(capacity)
{}

void Scrollback::emplace(int cols, const VTermScreenCell *cells, VTermState *vts)
{
    m_deque.emplace_front(cols, cells, vts);
    while (m_deque.size() > m_capacity) {
        m_deque.pop_back();
    }
}

void Scrollback::popto(int cols, VTermScreenCell *cells)
{
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
}

size_t Scrollback::scroll(int delta)
{
    m_offset = std::min(std::max(0, static_cast<int>(m_offset) + delta),
                        static_cast<int>(m_deque.size()));
    return m_offset;
}

void Scrollback::clear()
{
    m_offset = 0;
    m_deque.clear();
}

std::u32string Scrollback::currentText()
{
    std::u32string currentText;
    for (auto it = m_deque.rbegin(); it != m_deque.rend(); ++it) {
        currentText += it->text();
    }
    return currentText;
}

} // namespace Terminal::Internal
