/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "componenttextmodifier.h"

using namespace QmlDesigner;
ComponentTextModifier::ComponentTextModifier(TextModifier *originalModifier, int componentStartOffset, int componentEndOffset, int rootStartOffset) :
        m_originalModifier(originalModifier),
        m_componentStartOffset(componentStartOffset),
        m_componentEndOffset(componentEndOffset),
        m_rootStartOffset(rootStartOffset)
{
    connect(m_originalModifier, SIGNAL(replaced(int, int, int)), this, SLOT(contentsChange(int,int,int)));
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

int ComponentTextModifier::indentDepth() const
{
    return m_originalModifier->indentDepth();
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

QmlJS::Snapshot ComponentTextModifier::getSnapshot() const
{ return m_originalModifier->getSnapshot(); }

QStringList ComponentTextModifier::importPaths() const
{ return m_originalModifier->importPaths(); }
