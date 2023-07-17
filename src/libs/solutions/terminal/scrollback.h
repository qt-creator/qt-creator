// Copyright (c) 2020, Justin Bronder
// Copied and modified from: https://github.com/jsbronder/sff
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <vterm.h>

#include <deque>
#include <future>
#include <memory>

#include <QFont>
#include <QTextLayout>

namespace TerminalSolution {

class Scrollback
{
public:
    class Line
    {
    public:
        Line(int cols, const VTermScreenCell *cells);
        Line(Line &&other) = default;
        Line() = delete;

        int cols() const { return m_cols; };
        const VTermScreenCell *cell(int i) const;
        const VTermScreenCell *cells() const { return &m_cells[0]; };

    private:
        int m_cols;
        std::unique_ptr<VTermScreenCell[]> m_cells;
    };

public:
    Scrollback(size_t capacity);
    Scrollback() = delete;

    int capacity() const { return static_cast<int>(m_capacity); };
    int size() const { return static_cast<int>(m_deque.size()); };

    const Line &line(size_t index) const { return m_deque.at(index); };
    const std::deque<Line> &lines() const { return m_deque; };

    void emplace(int cols, const VTermScreenCell *cells);
    void popto(int cols, VTermScreenCell *cells);

    void clear();

private:
    size_t m_capacity;
    std::deque<Line> m_deque;
};

} // namespace TerminalSolution
