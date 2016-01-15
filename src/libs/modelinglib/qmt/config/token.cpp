/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#include "token.h"

namespace qmt {

Token::Token()
{
}

Token::Token(Type type, const QString &text, const SourcePos &sourcePos)
    : m_type(type),
      m_text(text),
      m_sourcePos(sourcePos)
{
}

Token::Token(Token::Type type, int subtype, const QString &text, const SourcePos &sourcePos)
    : m_type(type),
      m_subtype(subtype),
      m_text(text),
      m_sourcePos(sourcePos)
{
}

Token::~Token()
{
}

void Token::setType(Token::Type type)
{
    m_type = type;
}

void Token::setSubtype(int subtype)
{
    m_subtype = subtype;
}

void Token::setText(const QString &text)
{
    m_text = text;
}

void Token::setSourcePos(const SourcePos &sourcePos)
{
    m_sourcePos = sourcePos;
}

} // namespace qmt
