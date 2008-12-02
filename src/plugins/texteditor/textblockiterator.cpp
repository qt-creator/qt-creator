/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
#include "textblockiterator.h"

#include <QtGui/QTextDocument>

using namespace TextEditor;

TextBlockIterator::TextBlockIterator()
    : m_document(0),
      m_initialized(false)
{ }

TextBlockIterator::TextBlockIterator(const QTextBlock &block)
    : m_document(block.document()),
      m_block(block),
      m_initialized(false)
{ }

bool TextBlockIterator::equals(const TextBlockIterator &other) const
{
    if (m_document != other.m_document)
        return false;
    return m_block == other.m_block;
}

QString TextBlockIterator::operator*() const
{
    if (! m_initialized)
        read();
    return m_text;
}

void TextBlockIterator::read() const
{
    m_initialized = true;
    m_text = m_block.text();
}

TextBlockIterator &TextBlockIterator::operator++()
{
    m_initialized = false;
    m_block = m_block.next();
    return *this;
}

TextBlockIterator &TextBlockIterator::operator--()
{
    m_initialized = false;
    m_block = m_block.previous();
    return *this;
}

TextBlockIterator TextBlockIterator::operator++(int)
{
    TextBlockIterator prev;
    ++*this;
    return prev;
}

TextBlockIterator TextBlockIterator::operator--(int)
{
    TextBlockIterator prev;
    --*this;
    return prev;
}
