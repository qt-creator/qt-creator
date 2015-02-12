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

#ifndef PYTHONSCANNER_H
#define PYTHONSCANNER_H

#include "pythonformattoken.h"
#include "sourcecodestream.h"

#include <QString>
#include <QSet>

namespace PythonEditor {
namespace Internal {

/**
 * @brief The Scanner class - scans source code for highlighting only
 */
class Scanner
{
    Scanner(const Scanner &other);
    void operator =(const Scanner &other);

public:
    enum State {
        State_Default,
        State_String,
        State_MultiLineString
    };

    Scanner(const QChar *text, const int length);
    ~Scanner();

    void setState(int state);
    int state() const;
    FormatToken read();
    QString value(const FormatToken& tk) const;

private:
    FormatToken onDefaultState();

    void checkEscapeSequence(QChar quoteChar);
    FormatToken readStringLiteral(QChar quoteChar);
    FormatToken readMultiLineStringLiteral(QChar quoteChar);
    FormatToken readIdentifier();
    FormatToken readNumber();
    FormatToken readFloatNumber();
    FormatToken readComment();
    FormatToken readDoxygenComment();
    FormatToken readWhiteSpace();
    FormatToken readOperator();

    void clearState();
    void saveState(State state, QChar savedData);
    void parseState(State &state, QChar &savedData) const;

    SourceCodeStream m_src;
    int m_state;
    const QSet<QString> m_keywords;
    const QSet<QString> m_magics;
    const QSet<QString> m_builtins;
};

} // namespace Internal
} // namespace PythonEditor

#endif // PYTHONSCANNER_H
