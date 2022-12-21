// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmljs/qmljs_global.h>

#include <QRegularExpression>
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
    QStringView tokenText(const Token &token) const;

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

    QRegularExpression braceX;
};

} // namespace QmlJS
