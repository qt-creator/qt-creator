// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sourcepos.h"

#include <QString>

namespace qmt {

class Token
{
public:
    enum Type {
        TokenUndefined,
        TokenEndOfInput,
        TokenEndOfLine,
        TokenString,
        TokenInteger,
        TokenFloat,
        TokenIdentifier,
        TokenKeyword,
        TokenOperator,
        TokenColor
    };

    Token();
    Token(Type type, const QString &text, const SourcePos &sourcePos);
    Token(Type type, int subtype, const QString &text, const SourcePos &sourcePos);

    Type type() const { return m_type; }
    void setType(Type type);
    int subtype() const { return m_subtype; }
    void setSubtype(int subtype);
    QString text() const { return m_text; }
    void setText(const QString &text);
    SourcePos sourcePos() const { return m_sourcePos; }
    void setSourcePos(const SourcePos &sourcePos);

private:
    Type m_type = TokenUndefined;
    int m_subtype = 0;
    QString m_text;
    SourcePos m_sourcePos;
};

} // namespace qmt
