/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

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
