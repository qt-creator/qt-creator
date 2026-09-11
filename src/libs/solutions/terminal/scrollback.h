// Copyright (c) 2020, Justin Bronder
// Copied and modified from: https://github.com/jsbronder/sff
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <vterm.h>

#include <deque>
#include <functional>
#include <vector>

namespace TerminalSolution {

class Scrollback
{
public:
    class Line
    {
    public:
        Line() = default;

        void appendChars(const VTermScreenCell *chars, int count);
        void close();

        const std::vector<VTermScreenCell> &chars() const { return m_chars; }
        std::vector<VTermScreenCell> &cells() { return m_chars; }
        bool isEmpty() const { return m_chars.empty(); }

        int rowCount(int cols) const;

        struct Span
        {
            int first = 0;
            int count = 0;
        };
        Span rowSpan(int cols, int row) const;

        Line takeRows(int cols, int rows);

    private:
        std::vector<VTermScreenCell> m_chars;
        int m_columns = 0;
        bool m_hasWide = false;
    };

    explicit Scrollback(size_t capacity);
    Scrollback() = delete;

    int capacity() const { return static_cast<int>(m_capacity); }

    int width() const { return m_width; }
    void setWidth(int cols);

    int size() const;
    int lineCount() const { return static_cast<int>(m_lines.size()); }

    void appendRow(int cols, const VTermScreenCell *cells, bool continuation);
    bool popRow(int cols, VTermScreenCell *cells);

    void appendLine(Line line);
    Line takeLastLine();
    void closeLastLine();

    // Calls `visit` for every cell that is kept, to rewrite what they refer to
    void visitCells(const std::function<void(VTermScreenCell &)> &visit);

    void setBlank(const VTermScreenCell &blank);
    const VTermScreenCell *row(int index) const;
    bool rowIsContinuation(int index) const;

    void clear();

private:
    int lineIndexForRow(int index) const;
    long long rowOfLine(int lineIndex) const;
    void rebuildIndex();
    void trim();

    size_t m_capacity;
    int m_width = 0;
    std::deque<Line> m_lines;
    std::deque<long long> m_cum;

    VTermScreenCell m_blank{};
    mutable int m_cachedRow = -1;
    mutable std::vector<VTermScreenCell> m_cachedCells;
};

} // namespace TerminalSolution
