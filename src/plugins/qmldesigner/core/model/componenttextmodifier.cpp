/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "componenttextmodifier.h"

namespace QmlDesigner {

ComponentTextModifier::ComponentTextModifier(TextModifier *originalModifier, int componentStartOffset, int componentEndOffset, int rootStartOffset) :
        m_originalModifier(originalModifier),
        m_componentStartOffset(componentStartOffset),
        m_componentEndOffset(componentEndOffset),
        m_rootStartOffset(rootStartOffset)
{
    connect(m_originalModifier->textDocument(), SIGNAL(contentsChange(int,int,int)), this, SLOT(contentsChange(int,int,int)));
    connect(m_originalModifier, SIGNAL(textChanged()), this, SIGNAL(textChanged()));

    connect(m_originalModifier, SIGNAL(replaced(int, int, int)), this, SIGNAL(replaced(int, int, int)));
    connect(m_originalModifier, SIGNAL(moved(const TextModifier::MoveInfo &)), this, SIGNAL(moved(const TextModifier::MoveInfo &)));
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
    m_originalModifier->indent(offset, length);
}

void ComponentTextModifier::startGroup()
{
    m_originalModifier->startGroup();
}

void ComponentTextModifier::flushGroup()
{
    m_originalModifier->flushGroup();
}

void ComponentTextModifier::commitGroup()
{
    m_originalModifier->commitGroup();
}

QTextDocument *ComponentTextModifier::textDocument() const
{
    return m_originalModifier->textDocument();
}

QString ComponentTextModifier::text() const
{
    QString txt(m_originalModifier->text());

    const int leader = m_componentStartOffset - m_rootStartOffset;
    txt.replace(m_rootStartOffset, leader, QString(leader, ' '));

    const int textLength = txt.size();
    const int trailer = textLength - m_componentEndOffset;
    txt.replace(m_componentEndOffset, trailer, QString(trailer, ' '));

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

void ComponentTextModifier::contentsChange(int position, int charsRemoved, int charsAdded)
{
    const int diff = charsAdded - charsRemoved;

    if (position < m_rootStartOffset) {
        m_rootStartOffset += diff;
    }

    if (position < m_componentStartOffset) {
        m_componentStartOffset += diff;
    }

    if (position < m_componentEndOffset) {
        m_componentEndOffset += diff;
    }
}

} // namespace QmlDesigner
