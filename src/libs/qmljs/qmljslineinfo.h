/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLJSLINEINFO_H
#define QMLJSLINEINFO_H

#include <qmljs/qmljs_global.h>
#include <qmljs/qmljsscanner.h>

#include <QRegExp>
#include <QTextBlock>

namespace QmlJS {

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
        Program(QTextBlock begin, QTextBlock end)
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

#endif // QMLJSLINEINFO_H

