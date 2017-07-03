/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <sourcerangewithtextcontainer.h>

#include <QString>
#include <QTextCharFormat>

namespace ClangRefactoring {

template<typename SyntaxHighlighter>
class ClangQueryExampleHighlightMarker
{
    using SourceRange = ClangBackEnd::V2::SourceRangeContainer;
    using SourceRanges = ClangBackEnd::V2::SourceRangeContainers;
    using SourceRangeWithTexts = ClangBackEnd::SourceRangeWithTextContainers;
public:
    ClangQueryExampleHighlightMarker(SourceRangeWithTexts &&sourceRanges,
                                     SyntaxHighlighter &highlighter,
                                     const std::array<QTextCharFormat, 5> &textFormats)
        : m_sourceRanges(std::move(sourceRanges)),
          m_currentSourceRangeIterator(m_sourceRanges.begin()),
          m_highlighter(highlighter),
          m_textFormats(textFormats)
    {
    }

    ClangQueryExampleHighlightMarker(SyntaxHighlighter &highlighter)
        : m_currentSourceRangeIterator(m_sourceRanges.begin()),
          m_highlighter(highlighter)
    {
    }

    void setTextFormats(std::array<QTextCharFormat, 5> &&textFormats)
    {
        m_textFormats = std::move(textFormats);
    }

    void setSourceRanges(SourceRangeWithTexts &&sourceRanges)
    {
        m_currentlyUsedSourceRanges.clear();
        m_sourceRanges = std::move(sourceRanges);
        m_currentSourceRangeIterator = m_sourceRanges.begin();
    }

    void highlightSourceRanges(uint currentLineNumber, const QString &currentText)
    {
        while (hasSourceRangesForCurrentLine(currentLineNumber)) {
            SourceRange &sourceRange = *m_currentSourceRangeIterator;

            popSourceRangeIfMultiLineEndsHere(currentLineNumber, sourceRange.start().column());

            formatSourceRange(sourceRange,
                              currentLineNumber,
                              currentText.size(),
                              int(m_currentlyUsedSourceRanges.size()));

            pushSourceRangeIfMultiLine(sourceRange);
            ++m_currentSourceRangeIterator;
        }
    }

    void highlightCurrentlyUsedSourceRanges(uint currentLineNumber, const QString &currentText)
    {
        formatCurrentlyUsedSourceRanges(currentLineNumber, currentText.size());
        popSourceRangeIfMultiLineEndsHereAndAllSourceRangesAreConsumed(currentLineNumber);
    }

    void highlightBlock(uint currentLineNumber, const QString &currentText)
    {
        popSourceRangeIfMultiLineEndedBefore(currentLineNumber);
        highlightCurrentlyUsedSourceRanges(currentLineNumber, currentText);
        highlightSourceRanges(currentLineNumber, currentText);
    }

    bool hasSourceRangesForCurrentLine(uint currentLineNumber) const
    {
        return m_currentSourceRangeIterator != m_sourceRanges.end()
            && m_currentSourceRangeIterator->start().line() == currentLineNumber;
    }

    bool hasOnlySCurrentlyUsedSourceRanges() const
    {
        return m_currentSourceRangeIterator == m_sourceRanges.end()
            && !m_currentlyUsedSourceRanges.empty();
    }

    void formatSingleSourceRange(const SourceRange &sourceRange,
                                 int textFormatIndex)
    {
        int size = int(sourceRange.end().column() - sourceRange.start().column());
        m_highlighter.setFormat(int(sourceRange.start().column()) - 1,
                                size,
                                m_textFormats[textFormatIndex]);
    }

    void formatStartMultipleSourceRange(const SourceRange &sourceRange,
                                        int textSize,
                                        int textFormatIndex)
    {
        int size = textSize - int(sourceRange.start().column()) + 1;
        m_highlighter.setFormat(int(sourceRange.start().column()) - 1,
                                size,
                                m_textFormats[textFormatIndex]);
    }

    void formatEndMultipleSourceRange(const SourceRange &sourceRange,
                                      int textFormatIndex)
    {
        int size = int(sourceRange.end().column()) - 1;
        m_highlighter.setFormat(0, size, m_textFormats[textFormatIndex]);
    }

    void formatMiddleMultipleSourceRange(int textSize,
                                         int textFormatIndex)
    {
        m_highlighter.setFormat(0, textSize, m_textFormats[textFormatIndex]);
    }

    void formatSourceRange(const SourceRange &sourceRange,
                           uint currentLineNumber,
                           int textSize,
                           int textFormatIndex)
    {
        if (sourceRange.start().line() == sourceRange.end().line())
            formatSingleSourceRange(sourceRange, textFormatIndex);
        else if (sourceRange.start().line() == currentLineNumber)
            formatStartMultipleSourceRange(sourceRange, textSize, textFormatIndex);
        else if (sourceRange.end().line() == currentLineNumber)
            formatEndMultipleSourceRange(sourceRange, textFormatIndex);
        else
            formatMiddleMultipleSourceRange(textSize, textFormatIndex);
    }

    void formatCurrentlyUsedSourceRanges(uint currentLineNumber, int textSize)
    {
        int textFormatIndex = 0;

        for (const SourceRange &sourceRange : m_currentlyUsedSourceRanges) {
            formatSourceRange(sourceRange, currentLineNumber, textSize, textFormatIndex);
            ++textFormatIndex;
        }
    }

    bool currentlyUsedHasEndLineAndColumnNumber(uint currentLineNumber, uint currentColumnNumber)
    {
        return !m_currentlyUsedSourceRanges.empty()
            && m_currentlyUsedSourceRanges.back().end().line() <= currentLineNumber
            && m_currentlyUsedSourceRanges.back().end().column() <= currentColumnNumber;
    }

    void popSourceRangeIfMultiLineEndsHere(uint currentLineNumber, uint currentColumnNumber)
    {
        while (currentlyUsedHasEndLineAndColumnNumber(currentLineNumber, currentColumnNumber))
            m_currentlyUsedSourceRanges.pop_back();
    }

    bool currentlyUsedHasEndLineNumberAndSourceRangesAreConsumed(uint currentLineNumber)
    {
        return !m_currentlyUsedSourceRanges.empty()
            && m_currentSourceRangeIterator == m_sourceRanges.end()
            && m_currentlyUsedSourceRanges.back().end().line() == currentLineNumber;
    }

    void popSourceRangeIfMultiLineEndsHereAndAllSourceRangesAreConsumed(uint currentLineNumber)
    {
        while (currentlyUsedHasEndLineNumberAndSourceRangesAreConsumed(currentLineNumber))
            m_currentlyUsedSourceRanges.pop_back();
    }

    bool  currentlyUsedHasEndedBeforeLineNumber(uint currentLineNumber)
    {
        return !m_currentlyUsedSourceRanges.empty()
            && m_currentlyUsedSourceRanges.back().end().line() < currentLineNumber;
    }

    void popSourceRangeIfMultiLineEndedBefore(uint currentLineNumber)
    {
        while (currentlyUsedHasEndedBeforeLineNumber(currentLineNumber))
            m_currentlyUsedSourceRanges.pop_back();
    }

    void pushSourceRangeIfMultiLine(SourceRange &sourceRange)
    {
        m_currentlyUsedSourceRanges.push_back(sourceRange);
    }

private:
    SourceRangeWithTexts m_sourceRanges;
    SourceRangeWithTexts::iterator m_currentSourceRangeIterator;
    SourceRanges m_currentlyUsedSourceRanges;
    SyntaxHighlighter &m_highlighter;
    std::array<QTextCharFormat, 5> m_textFormats;
};

} // namespace ClangRefactoring
