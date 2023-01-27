/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include <QChar>
#include <QString>
#include <QVector>

#include <memory>

namespace Haskell {
namespace Internal {

enum class TokenType {
    Variable,
    Constructor,
    Operator,
    OperatorConstructor,
    Whitespace,
    String,
    StringError,
    Char,
    CharError,
    EscapeSequence,
    Integer,
    Float,
    Keyword,
    Special,
    SingleLineComment,
    MultiLineComment,
    Unknown
};

class Token {
public:
    bool isValid() const;

    TokenType type = TokenType::Unknown;
    int startCol = -1;
    int length = -1;
    QStringView text;

    std::shared_ptr<QString> source; // keep the string ref alive
};

class Tokens : public QVector<Token>
{
public:
    enum class State {
        None = -1,
        StringGap = 0, // gap == two backslashes enclosing only whitespace
        MultiLineCommentGuard // nothing may follow that
    };

    Tokens(std::shared_ptr<QString> source);

    Token tokenAtColumn(int col) const;

    std::shared_ptr<QString> source;
    int state = int(State::None);
};

class HaskellTokenizer
{
public:
    static Tokens tokenize(const QString &line, int startState);
};

} // Internal
} // Haskell
