// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
