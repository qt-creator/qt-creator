// Copyright (c) 2020, Justin Bronder
// Copied and modified from: https://github.com/jsbronder/sff
// SPDX-License-Identifier: BSD-3-Clause

#include "scrollback.h"

#include <algorithm>

namespace TerminalSolution {

static bool isSpacer(const VTermScreenCell &cell)
{
    return cell.chars[0] == static_cast<uint32_t>(-1);
}

static int cellWidth(const VTermScreenCell &cell)
{
    return cell.width < 1 ? 1 : static_cast<int>(cell.width);
}

void Scrollback::Line::appendChars(const VTermScreenCell *chars, int count)
{
    if (!m_chars.empty() && m_chars.back().chars[0] == 0) {
        for (int i = 0; i < count; ++i) {
            if (isSpacer(chars[i]))
                continue;
            if (cellWidth(chars[i]) == 2) {
                m_columns -= cellWidth(m_chars.back());
                m_chars.pop_back();
            }
            break;
        }
    }

    for (int i = 0; i < count; ++i) {
        const VTermScreenCell &cell = chars[i];
        if (isSpacer(cell))
            continue;

        m_chars.push_back(cell);
        const int width = cellWidth(cell);
        m_columns += width;
        if (width > 1)
            m_hasWide = true;
    }
}

void Scrollback::Line::close()
{
    while (!m_chars.empty() && m_chars.back().chars[0] == 0) {
        m_columns -= cellWidth(m_chars.back());
        m_chars.pop_back();
    }
}

int Scrollback::Line::rowCount(int cols) const
{
    if (cols <= 0 || m_chars.empty())
        return 1;

    if (!m_hasWide)
        return (m_columns + cols - 1) / cols;

    int rows = 1;
    int used = 0;
    for (const VTermScreenCell &cell : m_chars) {
        const int width = cellWidth(cell);
        if (used + width > cols) {
            ++rows;
            used = 0;
        }
        used += width;
    }
    return rows;
}

Scrollback::Line::Span Scrollback::Line::rowSpan(int cols, int row) const
{
    Span span;
    if (cols <= 0 || m_chars.empty())
        return span;

    int current = 0;
    int used = 0;
    int first = 0;
    for (size_t i = 0; i < m_chars.size(); ++i) {
        const int width = cellWidth(m_chars[i]);
        if (used + width > cols) {
            if (current == row) {
                span.first = first;
                span.count = static_cast<int>(i) - first;
                return span;
            }
            ++current;
            used = 0;
            first = static_cast<int>(i);
        }
        used += width;
    }

    if (current == row) {
        span.first = first;
        span.count = static_cast<int>(m_chars.size()) - first;
    }
    return span;
}

Scrollback::Line Scrollback::Line::takeRows(int cols, int rows)
{
    Line head;
    if (rows <= 0)
        return head;

    if (rows >= rowCount(cols)) {
        head = std::move(*this);
        *this = Line();
        return head;
    }

    const Span span = rowSpan(cols, rows - 1);
    const int cut = span.first + span.count;
    head.appendChars(m_chars.data(), cut);

    m_chars.erase(m_chars.begin(), m_chars.begin() + cut);
    m_columns = 0;
    m_hasWide = false;
    for (const VTermScreenCell &cell : m_chars) {
        const int width = cellWidth(cell);
        m_columns += width;
        if (width > 1)
            m_hasWide = true;
    }
    return head;
}

Scrollback::Scrollback(size_t capacity)
    : m_capacity(capacity)
{
    m_cum.push_back(0);
}

void Scrollback::setWidth(int cols)
{
    if (cols == m_width)
        return;

    m_width = cols;
    rebuildIndex();
}

void Scrollback::rebuildIndex()
{
    const long long base = m_cum.empty() ? 0 : m_cum.front();
    m_cum.clear();
    m_cum.push_back(base);
    for (const Line &line : m_lines)
        m_cum.push_back(m_cum.back() + line.rowCount(m_width));
    m_cachedRow = -1;
}

int Scrollback::size() const
{
    return static_cast<int>(m_cum.back() - m_cum.front());
}

long long Scrollback::rowOfLine(int lineIndex) const
{
    return m_cum[static_cast<size_t>(lineIndex)] - m_cum.front();
}

int Scrollback::lineIndexForRow(int index) const
{
    const long long target = m_cum.front() + index;
    const auto it = std::upper_bound(m_cum.begin(), m_cum.end(), target);
    return static_cast<int>(it - m_cum.begin()) - 1;
}

void Scrollback::appendRow(int cols, const VTermScreenCell *cells, bool continuation)
{
    if (m_width == 0)
        m_width = cols;

    if (!continuation || m_lines.empty()) {
        closeLastLine();
        m_lines.emplace_back();
        m_cum.push_back(m_cum.back());
    }

    Line &line = m_lines.back();
    line.appendChars(cells, cols);
    m_cum.back() = m_cum[m_cum.size() - 2] + line.rowCount(m_width);
    m_cachedRow = -1;
    trim();
}

void Scrollback::closeLastLine()
{
    if (m_lines.empty())
        return;

    m_lines.back().close();
    m_cum.back() = m_cum[m_cum.size() - 2] + m_lines.back().rowCount(m_width);
    m_cachedRow = -1;
}

void Scrollback::appendLine(Line line)
{
    m_lines.push_back(std::move(line));
    m_cum.push_back(m_cum.back() + m_lines.back().rowCount(m_width));
    m_cachedRow = -1;
    trim();
}

Scrollback::Line Scrollback::takeLastLine()
{
    if (m_lines.empty())
        return Line();

    Line line = std::move(m_lines.back());
    m_lines.pop_back();
    m_cum.pop_back();
    m_cachedRow = -1;
    return line;
}

bool Scrollback::popRow(int cols, VTermScreenCell *cells)
{
    const int last = size() - 1;
    if (last < 0)
        return false;

    const int lineIndex = lineIndexForRow(last);
    Line &line = m_lines[static_cast<size_t>(lineIndex)];
    const int subRow = last - static_cast<int>(rowOfLine(lineIndex));
    const Line::Span span = line.rowSpan(m_width, subRow);

    VTermScreenCell blank{};
    blank.width = 1;
    if (span.count > 0)
        blank.bg = line.chars()[static_cast<size_t>(span.first + span.count) - 1].bg;

    for (int i = 0; i < cols; ++i)
        cells[i] = blank;

    int col = 0;
    for (int i = 0; i < span.count && col < cols; ++i) {
        const VTermScreenCell &cell = line.chars()[static_cast<size_t>(span.first) + static_cast<size_t>(i)];
        const int width = cellWidth(cell);
        cells[col] = cell;
        if (width == 2 && col + 1 < cols) {
            cells[col + 1] = cell;
            cells[col + 1].chars[0] = static_cast<uint32_t>(-1);
            cells[col + 1].width = 1;
        }
        col += width;
    }

    if (subRow == 0)
        m_lines.erase(m_lines.begin() + lineIndex);
    else
        line = line.takeRows(m_width, subRow);

    rebuildIndex();
    return true;
}

void Scrollback::visitCells(const std::function<void(VTermScreenCell &)> &visit)
{
    for (Line &line : m_lines) {
        for (VTermScreenCell &cell : line.cells())
            visit(cell);
    }

    m_cachedRow = -1;
}

void Scrollback::setBlank(const VTermScreenCell &blank)
{
    m_blank = blank;
    m_cachedRow = -1;
}

const VTermScreenCell *Scrollback::row(int index) const
{
    if (index < 0 || index >= size() || m_width <= 0)
        return nullptr;

    if (m_cachedRow == index)
        return m_cachedCells.data();

    const int lineIndex = lineIndexForRow(index);
    const Line &line = m_lines[static_cast<size_t>(lineIndex)];
    const int subRow = index - static_cast<int>(rowOfLine(lineIndex));
    const Line::Span span = line.rowSpan(m_width, subRow);

    m_cachedCells.assign(static_cast<size_t>(m_width), m_blank);

    int col = 0;
    for (int i = 0; i < span.count && col < m_width; ++i) {
        const VTermScreenCell &cell = line.chars()[static_cast<size_t>(span.first) + static_cast<size_t>(i)];
        const int width = cellWidth(cell);
        m_cachedCells[static_cast<size_t>(col)] = cell;
        if (width == 2 && col + 1 < m_width) {
            m_cachedCells[static_cast<size_t>(col) + 1] = cell;
            m_cachedCells[static_cast<size_t>(col) + 1].chars[0] = static_cast<uint32_t>(-1);
            m_cachedCells[static_cast<size_t>(col) + 1].width = 1;
        }
        col += width;
    }

    m_cachedRow = index;
    return m_cachedCells.data();
}

bool Scrollback::rowIsContinuation(int index) const
{
    if (index <= 0 || index >= size())
        return false;

    const int lineIndex = lineIndexForRow(index);
    return index != static_cast<int>(rowOfLine(lineIndex));
}

void Scrollback::trim()
{
    while (m_lines.size() > m_capacity) {
        m_lines.pop_front();
        m_cum.pop_front();
    }
}

void Scrollback::clear()
{
    m_lines.clear();
    m_cum.clear();
    m_cum.push_back(0);
    m_cachedRow = -1;
}

} // namespace TerminalSolution
