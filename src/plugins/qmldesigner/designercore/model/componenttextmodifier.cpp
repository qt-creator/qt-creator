/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
    connect(m_originalModifier, SIGNAL(textChanged()), this, SIGNAL(textChanged()));

    connect(m_originalModifier, SIGNAL(replaced(int,int,int)), this, SIGNAL(replaced(int,int,int)));
    connect(m_originalModifier, SIGNAL(moved(TextModifier::MoveInfo)), this, SIGNAL(moved(TextModifier::MoveInfo)));
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

    uint textLength = m_originalModifier->text().length();
    m_componentEndOffset += (textLength - m_startLength);
    m_startLength = textLength;

}

void ComponentTextModifier::commitGroup()
{
    m_originalModifier->commitGroup();

    uint textLength = m_originalModifier->text().length();
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

void ComponentTextModifier::contentsChange(int /*position*/, int /*charsRemoved*/, int /*charsAdded*/)
{
}
