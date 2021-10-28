/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

    int textLength = m_originalModifier->text().length();
    m_componentEndOffset += (textLength - m_startLength);
    m_startLength = textLength;
}

QTextDocument *ComponentTextModifier::textDocument() const
{
    return m_originalModifier->textDocument();
}

QString ComponentTextModifier::text() const
{
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

    // Detect and adjust for removal/addition of unrelated text before the subcomponent code,
    // as that can happen even without user editing the text (e.g. whitespace removal at save time)

    const QString currentText = m_originalModifier->text();

    if (m_originalText.left(m_componentStartOffset) != currentText.left(m_componentStartOffset)) {
        // Subcomponent item id is the only reliable indicator for adjustment
        const int idIndex = m_originalText.indexOf("id:", m_componentStartOffset);
        if (idIndex != -1 && idIndex < m_componentEndOffset) {
            int newLineIndex = m_originalText.indexOf('\n', idIndex);
            if (newLineIndex != -1) {
                const QString checkLine = m_originalText.mid(idIndex, newLineIndex - idIndex);
                int lineIndex = currentText.indexOf(checkLine);
                if (lineIndex != -1) {
                    // Paranoia check - This shouldn't happen except when modifying text manually,
                    // but it's possible something was inserted between id and start
                    // of the component, which would throw off the calculation, so check that
                    // the first line is still correct even with new offset.
                    const int diff = idIndex - lineIndex;
                    newLineIndex = m_originalText.indexOf('\n', m_componentStartOffset);
                    if (newLineIndex != -1) {
                        const QString firstLine = m_originalText.mid(m_componentStartOffset,
                                                                     newLineIndex - m_componentStartOffset);
                        const int newStart = m_componentStartOffset - diff;
                        if (firstLine == currentText.mid(newStart, firstLine.size())) {
                            m_componentEndOffset -= diff;
                            m_componentStartOffset = newStart;
                            m_originalText = currentText;
                        }
                    }
                }
            }
        }
    }

    emit textChanged();
}
