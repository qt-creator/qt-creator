/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef QMLJSINDENTER_H
#define QMLJSINDENTER_H

#include <qmljs/qmljs_global.h>
#include <qmljs/qmljsscanner.h>

#include <QtCore/QRegExp>
#include <QtCore/QStringList>
#include <QtGui/QTextBlock>

namespace QmlJS {

class QMLJS_EXPORT QmlJSIndenter
{
    Q_DISABLE_COPY(QmlJSIndenter)

public:
    QmlJSIndenter();
    ~QmlJSIndenter();

    void setTabSize(int size);
    void setIndentSize(int size);

    int indentForBottomLine(QTextBlock firstBlock, QTextBlock lastBlock, QChar typedIn);
    QChar firstNonWhiteSpace(const QString &t) const;

private:
    static const int SmallRoof;
    static const int BigRoof;

    bool isOnlyWhiteSpace(const QString &t) const;
    int columnForIndex(const QString &t, int index) const;
    int indentOfLine(const QString &t) const;
    QString trimmedCodeLine(const QString &t);

    void eraseChar(QString &t, int k, QChar ch) const;
    QChar lastParen(const QString &t) const;
    bool okay(QChar typedIn, QChar okayCh) const;

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
    int indentWhenBottomLineStartsInMultiLineComment();
    bool matchBracelessControlStatement();
    bool isUnfinishedLine();
    bool isContinuationLine();
    int indentForContinuationLine();
    int indentForStandaloneLine();

    Token lastToken() const;
    QStringRef tokenText(const Token &token) const;

private:
    int ppHardwareTabSize;
    int ppIndentSize;
    int ppContinuationIndentSize;
    int ppCommentOffset;

private:
    struct LinizerState
    {
        LinizerState()
            : braceDepth(0),
              leftBraceFollows(false),
              pendingRightBrace(false)
        { }

        int braceDepth;
        bool leftBraceFollows;
        bool pendingRightBrace;
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

    QRegExp label;
    QRegExp braceX;
    QRegExp iflikeKeyword;
};

} // namespace QmlJS

#endif // QMLJSINDENTER_H

