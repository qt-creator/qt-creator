/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <qmljs/qmljs_global.h>

#include <QRegExp>
#include <QTextBlock>

namespace QmlJS {
class Token;

class QMLJS_EXPORT LineInfo
{
    Q_DISABLE_COPY(LineInfo)

public:
    LineInfo();
    virtual ~LineInfo();

    bool isUnfinishedLine();
    bool isContinuationLine();

    void initialize(QTextBlock begin, QTextBlock end);

protected:
    static const int SmallRoof;
    static const int BigRoof;

    QString trimmedCodeLine(const QString &t);
    QChar firstNonWhiteSpace(const QString &t) const;

    bool hasUnclosedParenOrBracket() const;

    /*
        The "linizer" is a group of functions and variables to iterate
        through the source code of the program to indent. The program is
        given as a list of strings, with the bottom line being the line
        to indent. The actual program might contain extra lines, but
        those are uninteresting and not passed over to us.
    */

    bool readLine();
    void startLinizer();
    bool bottomLineStartsInMultilineComment();
    bool matchBracelessControlStatement();

    Token lastToken() const;
    QStringRef tokenText(const Token &token) const;

protected:
    struct LinizerState
    {
        LinizerState()
            : braceDepth(0),
              leftBraceFollows(false),
              pendingRightBrace(false),
              insertedSemicolon(false)
        { }

        int braceDepth;
        bool leftBraceFollows;
        bool pendingRightBrace;
        bool insertedSemicolon;
        QString line;
        QList<Token> tokens;
        QTextBlock iter;
    };

    class Program
    {
    public:
        Program() {}
        Program(const QTextBlock &begin, const QTextBlock &end)
            : begin(begin), end(end) {}

        QTextBlock firstBlock() const { return begin; }
        QTextBlock lastBlock() const { return end; }

    private:
        QTextBlock begin, end;
    };

    Program yyProgram;
    LinizerState yyLinizerState;

    // shorthands
    const QString *yyLine;
    const int *yyBraceDepth;
    const bool *yyLeftBraceFollows;

    QRegExp braceX;
};

} // namespace QmlJS
