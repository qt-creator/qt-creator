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

#ifndef QTSCRIPTINDENTER_H
#define QTSCRIPTINDENTER_H

#include <QtCore/QRegExp>
#include <QtCore/QStringList>
#include <texteditor/textblockiterator.h>

namespace QtScriptEditor {
namespace Internal {

class QtScriptIndenter
{
    Q_DISABLE_COPY(QtScriptIndenter)

public:
    QtScriptIndenter();
    ~QtScriptIndenter();

    void setTabSize(int size);
    void setIndentSize(int size);

    int indentForBottomLine(TextEditor::TextBlockIterator begin, TextEditor::TextBlockIterator end, QChar typedIn);
    QChar firstNonWhiteSpace(const QString &t);

private:
    static const int SmallRoof;
    static const int BigRoof;

    bool isOnlyWhiteSpace(const QString &t);
    int columnForIndex(const QString &t, int index);
    int indentOfLine(const QString &t);
    QString trimmedCodeLine(const QString &t);

    inline void eraseChar(QString &t, int k, QChar ch);
    inline QChar lastParen(const QString &t);
    inline bool okay(QChar typedIn, QChar okayCh);

    /*
        The "linizer" is a group of functions and variables to iterate
        through the source code of the program to indent. The program is
        given as a list of strings, with the bottom line being the line
        to indent. The actual program might contain extra lines, but
        those are uninteresting and not passed over to us.
    */

    bool readLine();
    void startLinizer();
    bool bottomLineStartsInCComment();
    int indentWhenBottomLineStartsInCComment();
    bool matchBracelessControlStatement();
    bool isUnfinishedLine();
    bool isContinuationLine();
    int indentForContinuationLine();
    int indentForStandaloneLine();

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
              inCComment(false),
              pendingRightBrace(false)
        { }

        int braceDepth;
        bool leftBraceFollows;
        bool inCComment;
        bool pendingRightBrace;
        QString line;
        TextEditor::TextBlockIterator iter;
    };

    struct Program {
        TextEditor::TextBlockIterator b, e;
        typedef TextEditor::TextBlockIterator iterator;
        typedef TextEditor::TextBlockIterator const_iterator;

        Program() {}
        Program(TextEditor::TextBlockIterator begin, TextEditor::TextBlockIterator end)
            : b(begin), e(end) {}

        iterator begin() const { return b; }
        iterator end() const { return e; }

        const_iterator constBegin() const { return b; }
        const_iterator constEnd() const { return e; }
    };

    Program yyProgram;
    LinizerState yyLinizerState;

    // shorthands
    const QString *yyLine;
    const int *yyBraceDepth;
    const bool *yyLeftBraceFollows;

    QRegExp literal;
    QRegExp label;
    QRegExp inlineCComment;
    QRegExp braceX;
    QRegExp iflikeKeyword;
};

} // namespace Internal
} // namespace QtScriptEditor

#endif // QTSCRIPTINDENTER_H

