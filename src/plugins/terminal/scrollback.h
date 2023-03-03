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

namespace Terminal::Internal {

class Scrollback
{
public:
    class Line
    {
    public:
        Line(int cols, const VTermScreenCell *cells, VTermState *vts);
        Line(Line &&other) = default;
        Line() = delete;

        int cols() const { return m_cols; };
        const VTermScreenCell *cell(int i) const;
        const VTermScreenCell *cells() const { return &m_cells[0]; };

        const QTextLayout &layout(int version, const QFont &font, qreal lineSpacing) const;
        const std::u32string &text() const;

    private:
        int m_cols;
        std::unique_ptr<VTermScreenCell[]> m_cells;
        mutable std::unique_ptr<QTextLayout> m_layout;
        mutable int m_layoutVersion{-1};
        mutable std::optional<std::u32string> m_text;
        mutable std::future<std::u32string> m_textFuture;
    };

public:
    Scrollback(size_t capacity);
    Scrollback() = delete;

    size_t capacity() const { return m_capacity; };
    size_t size() const { return m_deque.size(); };
    size_t offset() const { return m_offset; };

    const Line &line(size_t index) const { return m_deque.at(index); };
    const std::deque<Line> &lines() const { return m_deque; };

    void emplace(int cols,
                 const VTermScreenCell *cells,
                 VTermState *vts);
    void popto(int cols, VTermScreenCell *cells);
    size_t scroll(int delta);
    void unscroll() { m_offset = 0; };

    void clear();

    std::u32string currentText();

private:
    size_t m_capacity;
    size_t m_offset{0};
    std::deque<Line> m_deque;
};

} // namespace Terminal::Internal
