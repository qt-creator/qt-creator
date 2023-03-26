// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "componenttextmodifier.h"

using namespace QmlDesigner;

ComponentTextModifier::ComponentTextModifier(TextModifier *originalModifier, int componentStartOffset, int componentEndOffset, int rootStartOffset) :
        m_originalModifier(originalModifier),
        m_componentStartOffset(componentStartOffset),
        m_componentEndOffset(componentEndOffset),
        m_rootStartOffset(rootStartOffset)
{
    connect(m_originalModifier, &TextModifier::textChanged,
            this, &ComponentTextModifier::handleOriginalTextChanged);

    connect(m_originalModifier, &TextModifier::replaced, this, &TextModifier::replaced);
    connect(m_originalModifier, &TextModifier::moved, this, &TextModifier::moved);

    m_originalText = m_originalModifier->text();
}

ComponentTextModifier::~ComponentTextModifier() = default;

void ComponentTextModifier::replace(int offset, int length, const QString& replacement)
{
    m_originalModifier->replace(offset, length, replacement);
}

void ComponentTextModifier::move(const MoveInfo &moveInfo)
{
    m_originalModifier->move(moveInfo);
}

void ComponentTextModifier::indent(int offset, int length)
{
    int componentStartLine = getLineInDocument(m_originalModifier->textDocument(), m_componentStartOffset);
    int componentEndLine = getLineInDocument(m_originalModifier->textDocument(), m_componentEndOffset);

    /* Do not indent lines that contain code of the component and the surrounding QML.
     * example:
     * delegate: Item { //startLine
     * ...
     * } // endLine
     * Indenting such lines will change the offsets of the component.
     */

    --componentStartLine;
    --componentEndLine;

    int startLine = getLineInDocument(m_originalModifier->textDocument(), offset);
    int endLine = getLineInDocument(m_originalModifier->textDocument(), offset + length);

    if (startLine < componentStartLine)
        startLine = componentStartLine;
    if (endLine > componentEndLine)
        endLine = componentEndLine;

    indentLines(startLine, endLine);
}

void ComponentTextModifier::indentLines(int startLine, int endLine)
{
    m_originalModifier->indentLines(startLine, endLine);
}

TextEditor::TabSettings ComponentTextModifier::tabSettings() const
{
    return m_originalModifier->tabSettings();
}

void ComponentTextModifier::startGroup()
{
    m_originalModifier->startGroup();
    m_startLength = m_originalModifier->text().length();
}

void ComponentTextModifier::flushGroup()
{
    m_originalModifier->flushGroup();

    int textLength = m_originalModifier->text().length();
    m_componentEndOffset += (textLength - m_startLength);
    m_startLength = textLength;

}

void ComponentTextModifier::commitGroup()
{
    m_originalModifier->commitGroup();

    m_originalText = m_originalModifier->text();
    int textLength = m_originalText.length();
    m_componentEndOffset += (textLength - m_startLength);
    m_startLength = textLength;
}

QTextDocument *ComponentTextModifier::textDocument() const
{
    return m_originalModifier->textDocument();
}

QString ComponentTextModifier::text() const
{
    if (m_componentStartOffset == -1)
        return {};

    QString txt(m_originalModifier->text());

    const int leader = m_componentStartOffset - m_rootStartOffset;
    txt.replace(m_rootStartOffset, leader, QString(leader, QLatin1Char(' ')));

    const int textLength = txt.size();
    const int trailer = textLength - m_componentEndOffset;
    txt.replace(m_componentEndOffset, trailer, QString(trailer, QLatin1Char(' ')));

    return txt;
}

QTextCursor ComponentTextModifier::textCursor() const
{
    return m_originalModifier->textCursor();
}

void ComponentTextModifier::deactivateChangeSignals()
{
    m_originalModifier->deactivateChangeSignals();
}

void ComponentTextModifier::reactivateChangeSignals()
{
    m_originalModifier->reactivateChangeSignals();
}

void ComponentTextModifier::handleOriginalTextChanged()
{
    // Update offsets when original text changes, if necessary

    const QString currentText = m_originalModifier->text();

    const int oldLen = m_originalText.size();
    const int newLen = currentText.size();

    if (oldLen != newLen) {
        int newSpace = 0;
        int oldSpace = 0;
        int newIdx = 0;
        int nonWhiteSpaceChangeIdx = -1;
        int newStartOffset = m_componentStartOffset;

        // Adjust for removal/addition of whitespace in the document.
        // Whitespace changes that happen when document is saved can be spread around throughout
        // the entire document in multiple places.
        // Check that non-whitespace portion of the text is the same and count the whitespace diff
        for (int oldIdx = 0; oldIdx < oldLen; ++oldIdx) {
            const QChar oldChar = m_originalText[oldIdx];
            if (oldIdx == m_componentStartOffset)
                newStartOffset += newSpace - oldSpace;
            if (oldIdx == m_componentEndOffset) {
                m_componentEndOffset += newSpace - oldSpace;
                m_componentStartOffset = newStartOffset;
                m_originalText = currentText;
                break;
            }

            while (newIdx < newLen && currentText[newIdx].isSpace()) {
                ++newSpace;
                ++newIdx;
            }

            if (oldChar.isSpace()) {
                ++oldSpace;
                continue;
            }

            if (currentText[newIdx] != oldChar) {
                nonWhiteSpaceChangeIdx = oldIdx;
                // A non-whitespace change is a result of manual text edit or undo/redo operation.
                // Assumption is that separate whitespace changes and a non-whitespace change can't
                // both happen simultaneously, so break out of whitespace check loop.
                break;
            } else {
                ++newIdx;
            }
        }

        if (nonWhiteSpaceChangeIdx >= 0) {
            // For non-whitespace change, we assume the whole change is either before the component
            // or inside the component. If the change spans both, it's likely the change is
            // invalid anyway, and we don't care about trying to keep offsets up to date.
            int diff = newLen - oldLen;
            if (nonWhiteSpaceChangeIdx < m_componentEndOffset)
                m_componentEndOffset += diff;
            if (nonWhiteSpaceChangeIdx < m_componentStartOffset)
                m_componentStartOffset += diff;
            m_originalText = currentText;
        }

    }

    emit textChanged();
}
