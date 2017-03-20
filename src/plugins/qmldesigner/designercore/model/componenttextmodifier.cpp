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
    connect(m_originalModifier, &TextModifier::textChanged, this, &TextModifier::textChanged);

    connect(m_originalModifier, &TextModifier::replaced, this, &TextModifier::replaced);
    connect(m_originalModifier, &TextModifier::moved, this, &TextModifier::moved);
}

ComponentTextModifier::~ComponentTextModifier()
{
}

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

int ComponentTextModifier::indentDepth() const
{
    return m_originalModifier->indentDepth();
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
