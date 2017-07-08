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
