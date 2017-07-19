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

#include <dynamicastmatcherdiagnosticcontextcontainer.h>
#include <dynamicastmatcherdiagnosticmessagecontainer.h>
#include <sourcerangecontainerv2.h>

#include <QString>
#include <QTextCharFormat>

namespace ClangRefactoring {

template<typename SyntaxHighlighter>
class ClangQueryHighlightMarker
{
    using SourceRange = ClangBackEnd::V2::SourceRangeContainer;
    using Message = ClangBackEnd::DynamicASTMatcherDiagnosticMessageContainer;
    using Context = ClangBackEnd::DynamicASTMatcherDiagnosticContextContainer;
    using Messages = ClangBackEnd::DynamicASTMatcherDiagnosticMessageContainers;
    using Contexts = ClangBackEnd::DynamicASTMatcherDiagnosticContextContainers;

public:
    ClangQueryHighlightMarker(SyntaxHighlighter &highlighter)
        : m_highlighter(highlighter)
    {
    }

    void setTextFormats(const QTextCharFormat &messageTextFormat,
                        const QTextCharFormat &contextTextFormat)
    {
        m_contextTextFormat = contextTextFormat;
        m_messageTextFormat = messageTextFormat;
    }

    void setMessagesAndContexts(Messages &&messages, Contexts &&contexts)
    {
        m_currentlyUsedContexts.clear();
        m_currentlyUsedMessages.clear();
        m_contexts = std::move(contexts);
        m_messages = std::move(messages);
        m_currentContextsIterator = m_contexts.begin();
        m_currentMessagesIterator = m_messages.begin();
    }

    bool hasMessage(uint currentLineNumber) const
    {
        return m_currentMessagesIterator != m_messages.end()
            && m_currentMessagesIterator->sourceRange().start().line() == currentLineNumber;
    }

    bool hasContext(uint currentLineNumber) const
    {
        return m_currentContextsIterator != m_contexts.end()
            && m_currentContextsIterator->sourceRange().start().line() == currentLineNumber;
    }

    bool isMessageNext() const
    {
        return m_currentMessagesIterator->sourceRange().start().column()
             < m_currentContextsIterator->sourceRange().start().column();
    }

    void formatSameLineSourceRange(const SourceRange &sourceRange, const QTextCharFormat &textFormat)
    {
        uint startColumn = sourceRange.start().column();
        uint endColumn = sourceRange.end().column();
        m_highlighter.setFormat(startColumn - 1, endColumn - startColumn, textFormat);
    }

    void formatStartLineSourceRange(const SourceRange &sourceRange,
                                    int textSize,
                                    const QTextCharFormat &textFormat)
    {
        uint startColumn = sourceRange.start().column();
        m_highlighter.setFormat(startColumn - 1, textSize - startColumn, textFormat);
    }

    void formatEndLineSourceRange(const SourceRange &sourceRange, const QTextCharFormat &textFormat)
    {
        uint endColumn = sourceRange.end().column();
        m_highlighter.setFormat(0, endColumn - 1, textFormat);
    }

    void formatMiddleLineSourceRange(int textSize, const QTextCharFormat &textFormat)
    {
        m_highlighter.setFormat(0, textSize, textFormat);
    }

    static
    bool isSameLine(const SourceRange &sourceRange)
    {
        uint startLine = sourceRange.start().line();
        uint endLine = sourceRange.end().line();

        return startLine == endLine;
    }

    static
    bool isStartLine(const SourceRange &sourceRange, uint currentLineNumber)
    {
        uint startLine = sourceRange.start().line();

        return startLine == currentLineNumber;
    }

    static
    bool isEndLine(const SourceRange &sourceRange, uint currentLineNumber)
    {
        uint endLine = sourceRange.end().line();

        return endLine == currentLineNumber;
    }

    void formatLine(const SourceRange &sourceRange,
                    uint currentLineNumber,
                    int textSize,
                    const QTextCharFormat &textFormat)
    {
        if (isSameLine(sourceRange))
            formatSameLineSourceRange(sourceRange, textFormat);
        else if (isStartLine(sourceRange, currentLineNumber))
            formatStartLineSourceRange(sourceRange, textSize, textFormat);
        else if (isEndLine(sourceRange, currentLineNumber))
            formatEndLineSourceRange(sourceRange, textFormat);
        else
            formatMiddleLineSourceRange(textSize, textFormat);
    }

    template<typename Container>
    void format(Container &container,
                typename Container::iterator &iterator,
                uint currentLineNumber,
                int textSize,
                const QTextCharFormat &textFormat)
    {
        const SourceRange &sourceRange = iterator->sourceRange();

        formatLine(sourceRange, currentLineNumber, textSize, textFormat);

        if (isStartLine(sourceRange, currentLineNumber))
            container.push_back(*iterator);

        if (isSameLine(sourceRange) || isStartLine(sourceRange, currentLineNumber))
            ++iterator;
    }

    template<typename Container>
    static
    void removeEndedContainers(uint currentLineNumber, Container &container)
    {
        auto newEnd = std::remove_if(container.begin(),
                                     container.end(),
                                     [&] (const auto &entry) {
            return ClangQueryHighlightMarker::isEndLine(entry.sourceRange(), currentLineNumber);
        });

        container.erase(newEnd, container.end());
    }

    template<typename Container>
    void formatCurrentlyUsed(Container container,
                                       uint currentLineNumber,
                                       int textSize,
                                       const QTextCharFormat &textFormat)
    {
        for (const auto &entry : container) {
            formatLine(entry.sourceRange(), currentLineNumber, textSize, textFormat);;
        }
    }

    void formatMessage(uint currentLineNumber, int textSize)
    {
        format(m_currentlyUsedMessages,
               m_currentMessagesIterator,
               currentLineNumber,
               textSize,
               m_messageTextFormat);
    }

    void formatContext(uint currentLineNumber, int textSize)
    {
        format(m_currentlyUsedContexts,
               m_currentContextsIterator,
               currentLineNumber,
               textSize,
               m_contextTextFormat);
    }

    void formatCurrentlyUsedMessagesAndContexts(uint currentLineNumber, int textSize)
    {
        formatCurrentlyUsed(m_currentlyUsedContexts, currentLineNumber, textSize, m_contextTextFormat);
        formatCurrentlyUsed(m_currentlyUsedMessages, currentLineNumber, textSize, m_messageTextFormat);

        removeEndedContainers(currentLineNumber, m_currentlyUsedContexts);
        removeEndedContainers(currentLineNumber, m_currentlyUsedMessages);
    }

    void formatCurrentMessageOrContext(uint currentLineNumber, int textSize)
    {
        bool hasContext = this->hasContext(currentLineNumber);
        bool hasMessage = this->hasMessage(currentLineNumber);

        while (hasContext || hasMessage) {
            if (!hasContext)
                formatMessage(currentLineNumber, textSize);
            else if (!hasMessage)
                formatContext(currentLineNumber, textSize);
            else if (isMessageNext())
                formatMessage(currentLineNumber, textSize);
            else
                formatContext(currentLineNumber, textSize);

            hasContext = this->hasContext(currentLineNumber);
            hasMessage = this->hasMessage(currentLineNumber);
        }
    }

    void highlightBlock(uint currentLineNumber, const QString &currentText)
    {
        formatCurrentlyUsedMessagesAndContexts(currentLineNumber, currentText.size());
        formatCurrentMessageOrContext(currentLineNumber, currentText.size());
    }

    bool hasMessagesOrContexts() const
    {
        return !m_messages.empty() || !m_contexts.empty();
    }

    static
    bool isAfterStartColumn(const SourceRange &sourceRange, uint line, uint column)
    {
        return sourceRange.start().line() == line && sourceRange.start().column() <= column;
    }

    static
    bool isBeforeEndColumn(const SourceRange &sourceRange, uint line, uint column)
    {
        return sourceRange.end().line() == line && sourceRange.end().column() >= column;
    }

    static
    bool isInBetweenLine(const SourceRange &sourceRange, uint line)
    {
        return sourceRange.start().line() < line && sourceRange.end().line() > line;
    }

    static
    bool isSingleLine(const SourceRange &sourceRange)
    {
        return sourceRange.start().line() == sourceRange.end().line();
    }

    static
    bool isInsideMultiLine(const SourceRange &sourceRange, uint line, uint column)
    {
        return !isSingleLine(sourceRange)
            && (isAfterStartColumn(sourceRange, line, column)
             || isInBetweenLine(sourceRange, line)
             || isBeforeEndColumn(sourceRange, line, column));
    }

    static
    bool isInsideSingleLine(const SourceRange &sourceRange, uint line, uint column)
    {
        return isSingleLine(sourceRange)
            && isAfterStartColumn(sourceRange, line, column)
            && isBeforeEndColumn(sourceRange, line, column);
    }

    static
    bool isInsideRange(const SourceRange &sourceRange, uint line, uint column)
    {
        return isInsideSingleLine(sourceRange, line, column)
            || isInsideMultiLine(sourceRange, line, column);
    }

    Messages messagesForLineAndColumn(uint line, uint column) const
    {
        Messages messages;

        auto underPosition = [=] (const Message &message) {
            return ClangQueryHighlightMarker::isInsideRange(message.sourceRange(), line, column);
        };

        std::copy_if(m_messages.begin(),
                     m_messages.end(),
                     std::back_inserter(messages),
                     underPosition);

        return messages;
    }

    Contexts contextsForLineAndColumn(uint line, uint column) const
    {
        Contexts contexts;

        auto underPosition = [=] (const Context &context) {
            return ClangQueryHighlightMarker::isInsideRange(context.sourceRange(), line, column);
        };

        std::copy_if(m_contexts.begin(),
                     m_contexts.end(),
                     std::back_inserter(contexts),
                     underPosition);

        return contexts;
    }

private:
    Contexts m_contexts;
    Messages m_messages;
    Contexts m_currentlyUsedContexts;
    Messages m_currentlyUsedMessages;
    Contexts::iterator m_currentContextsIterator{m_contexts.begin()};
    Messages::iterator m_currentMessagesIterator{m_messages.begin()};
    QTextCharFormat m_messageTextFormat;
    QTextCharFormat m_contextTextFormat;
    SyntaxHighlighter &m_highlighter;
};

} // namespace ClangRefactoring
