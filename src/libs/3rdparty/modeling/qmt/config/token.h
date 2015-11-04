/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
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

#ifndef QMT_TOKEN_H
#define QMT_TOKEN_H

#include "sourcepos.h"

#include <QString>

namespace qmt {

class Token
{
public:

    enum Type {
        TOKEN_UNDEFINED,
        TOKEN_ENDOFINPUT,
        TOKEN_ENDOFLINE,
        TOKEN_STRING,
        TOKEN_INTEGER,
        TOKEN_FLOAT,
        TOKEN_IDENTIFIER,
        TOKEN_KEYWORD,
        TOKEN_OPERATOR,
        TOKEN_COLOR
    };

public:
    Token();
    Token(Type type, const QString &text, const SourcePos &sourcePos);
    Token(Type type, int subtype, const QString &text, const SourcePos &sourcePos);
    ~Token();

public:

    Type type() const { return m_type; }

    void setType(Type type);

    int subtype() const { return m_subtype; }

    void setSubtype(int subtype);

    QString text() const { return m_text; }

    void setText(const QString &text);

    SourcePos sourcePos() const { return m_sourcePos; }

    void setSourcePos(const SourcePos &sourcePos);

private:
    Type m_type;
    int m_subtype;
    QString m_text;
    SourcePos m_sourcePos;
};

} // namespace qmt

#endif // QMT_TOKEN_H
