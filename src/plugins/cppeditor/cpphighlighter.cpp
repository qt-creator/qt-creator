/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "cpphighlighter.h"

#include <Token.h>
#include <cplusplus/SimpleLexer.h>
#include <texteditor/basetexteditor.h>

#include <QtGui/QTextDocument>
#include <QtCore/QDebug>

using namespace CppEditor::Internal;
using namespace TextEditor;
using namespace CPlusPlus;

CppHighlighter::CppHighlighter(QTextDocument *document) :
    QSyntaxHighlighter(document)
{
    visualSpaceFormat.setForeground(Qt::lightGray);
}

void CppHighlighter::highlightBlock(const QString &text)
{
    QTextCharFormat emptyFormat;

    const int previousState = previousBlockState();
    int state = 0, braceDepth = 0;
    if (previousState != -1) {
        state = previousState & 0xff;
        braceDepth = previousState >> 8;
    }

    SimpleLexer tokenize;
    tokenize.setQtMocRunEnabled(false);

    int initialState = state;
    const QList<SimpleToken> tokens = tokenize(text, initialState);
    state = tokenize.state(); // refresh the state

    if (tokens.isEmpty()) {
        setCurrentBlockState(previousState);
        if (TextBlockUserData *userData = TextEditDocumentLayout::testUserData(currentBlock())) {
            userData->setClosingCollapseMode(TextBlockUserData::NoClosingCollapse);
            userData->setCollapseMode(TextBlockUserData::NoCollapse);
        }
        TextEditDocumentLayout::clearParentheses(currentBlock());
        return;
    }

    const int firstNonSpace = tokens.first().position();

    Parentheses parentheses;
    parentheses.reserve(20); // assume wizard level ;-)

    bool highlightAsPreprocessor = false;

    for (int i = 0; i < tokens.size(); ++i) {
        const SimpleToken &tk = tokens.at(i);

        int previousTokenEnd = 0;
        if (i != 0) {
            // mark the whitespaces
            previousTokenEnd = tokens.at(i - 1).position() +
                               tokens.at(i - 1).length();
        }

        if (previousTokenEnd != tk.position()) {
            setFormat(previousTokenEnd, tk.position() - previousTokenEnd,
                      visualSpaceFormat);
        }

        if (tk.is(T_LPAREN) || tk.is(T_LBRACE) || tk.is(T_LBRACKET)) {
            const QChar c(tk.text().at(0));
            parentheses.append(Parenthesis(Parenthesis::Opened, c, tk.position()));
            if (tk.is(T_LBRACE))
                ++braceDepth;
        } else if (tk.is(T_RPAREN) || tk.is(T_RBRACE) || tk.is(T_RBRACKET)) {
            const QChar c(tk.text().at(0));
            parentheses.append(Parenthesis(Parenthesis::Closed, c, tk.position()));
            if (tk.is(T_RBRACE)) {
                if (--braceDepth < 0)
                    braceDepth = 0;
            }
        }

        bool highlightCurrentWordAsPreprocessor = highlightAsPreprocessor;
        if (highlightAsPreprocessor)
            highlightAsPreprocessor = false;

        if (i == 0 && tk.is(T_POUND)) {
            setFormat(tk.position(), tk.length(), m_formats[CppPreprocessorFormat]);
            highlightAsPreprocessor = true;
        } else if (highlightCurrentWordAsPreprocessor &&
                   (tk.isKeyword() || tk.is(T_IDENTIFIER)) && isPPKeyword(tk.text()))
            setFormat(tk.position(), tk.length(), m_formats[CppPreprocessorFormat]);
        else if (tk.is(T_INT_LITERAL) || tk.is(T_FLOAT_LITERAL))
            setFormat(tk.position(), tk.length(), m_formats[CppNumberFormat]);
        else if (tk.is(T_STRING_LITERAL) || tk.is(T_CHAR_LITERAL) || tk.is(T_ANGLE_STRING_LITERAL))
            setFormat(tk.position(), tk.length(), m_formats[CppStringFormat]);
        else if (tk.is(T_WIDE_STRING_LITERAL) || tk.is(T_WIDE_CHAR_LITERAL))
            setFormat(tk.position(), tk.length(), m_formats[CppStringFormat]);
        else if (tk.is(T_COMMENT)) {
            setFormat(tk.position(), tk.length(), m_formats[CppCommentFormat]);
            // we need to insert a close comment parenthesis, if
            //  - the line starts in a C Comment (initalState != 0)
            //  - the first token of the line is a T_COMMENT (i == 0 && tk.is(T_COMMENT))
            //  - is not a continuation line (tokens.size() > 1 || ! state)
            if (initialState && i == 0 && (tokens.size() > 1 || ! state)) {
                --braceDepth;

                const int tokenEnd = tk.position() + tk.length() - 1;
                parentheses.append(Parenthesis(Parenthesis::Closed, QLatin1Char('-'), tokenEnd));

                // clear the initial state.
                initialState = 0;
            }
        } else if (tk.isKeyword() || isQtKeyword(tk.text()))
            setFormat(tk.position(), tk.length(), m_formats[CppKeywordFormat]);
        else if (tk.isOperator())
            setFormat(tk.position(), tk.length(), m_formats[CppOperatorFormat]);
        else if (i == 0 && tokens.size() > 1 && tk.is(T_IDENTIFIER) && tokens.at(1).is(T_COLON))
            setFormat(tk.position(), tk.length(), m_formats[CppLabelFormat]);
        else if (tk.is(T_IDENTIFIER))
            highlightWord(tk.text(), tk.position(), tk.length());
    }

    // mark the trailing white spaces
    if (! tokens.isEmpty()) {
        const SimpleToken tk = tokens.last();
        const int lastTokenEnd = tk.position() + tk.length();
        if (text.length() > lastTokenEnd)
            setFormat(lastTokenEnd, text.length() - lastTokenEnd, visualSpaceFormat);
    }

    if (TextBlockUserData *userData = TextEditDocumentLayout::testUserData(currentBlock())) {
        userData->setClosingCollapseMode(TextBlockUserData::NoClosingCollapse);
        userData->setCollapseMode(TextBlockUserData::NoCollapse);
    }

    if (! initialState && state && ! tokens.isEmpty()) {
        parentheses.append(Parenthesis(Parenthesis::Opened, QLatin1Char('+'),
                                       tokens.last().position()));
        ++braceDepth;
    }

    QChar c;
    int collapse = Parenthesis::collapseAtPos(parentheses, &c);
    if (collapse >= 0) {
        TextBlockUserData::CollapseMode collapseMode = TextBlockUserData::CollapseAfter;
        if (collapse == firstNonSpace && c != QLatin1Char('+'))
            collapseMode = TextBlockUserData::CollapseThis;
        TextEditDocumentLayout::userData(currentBlock())->setCollapseMode(collapseMode);
    }


    int cc = Parenthesis::closeCollapseAtPos(parentheses);
    if (cc >= 0) {
        TextBlockUserData *userData = TextEditDocumentLayout::userData(currentBlock());
        userData->setClosingCollapseMode(TextBlockUserData::ClosingCollapse);
        QString trailingText = text.mid(cc+1).simplified();
        if (trailingText.isEmpty() || trailingText == QLatin1String(";")) {
            userData->setClosingCollapseMode(TextBlockUserData::ClosingCollapseAtEnd);
        }
    }

    TextEditDocumentLayout::setParentheses(currentBlock(), parentheses);

    setCurrentBlockState((braceDepth << 8) | tokenize.state());
}


bool CppHighlighter::isPPKeyword(const QStringRef &text) const
{
    switch (text.length())
    {
    case 2:
        if (text.at(0) == 'i' && text.at(1) == 'f')
            return true;
        break;

    case 4:
        if (text.at(0) == 'e' && text == QLatin1String("elif"))
            return true;
        else if (text.at(0) == 'e' && text == QLatin1String("else"))
            return true;
        break;

    case 5:
        if (text.at(0) == 'i' && text == QLatin1String("ifdef"))
            return true;
        else if (text.at(0) == 'u' && text == QLatin1String("undef"))
            return true;
        else if (text.at(0) == 'e' && text == QLatin1String("endif"))
            return true;
        else if (text.at(0) == 'e' && text == QLatin1String("error"))
            return true;
        break;

    case 6:
        if (text.at(0) == 'i' && text == QLatin1String("ifndef"))
            return true;
        else if (text.at(0) == 'd' && text == QLatin1String("define"))
            return true;
        else if (text.at(0) == 'p' && text == QLatin1String("pragma"))
            return true;
        break;

    case 7:
        if (text.at(0) == 'i' && text == QLatin1String("include"))
            return true;
        else if (text.at(0) == 'w' && text == QLatin1String("warning"))
            return true;
        break;

    case 12:
        if (text.at(0) == 'i' && text == QLatin1String("include_next"))
            return true;
        break;

    default:
        break;
    }

    return false;
}

bool CppHighlighter::isQtKeyword(const QStringRef &text) const
{
    switch (text.length()) {
    case 4:
        if (text.at(0) == 'e' && text == QLatin1String("emit"))
            return true;
        else if (text.at(0) == 'S' && text == QLatin1String("SLOT"))
            return true;
        break;

    case 5:
        if (text.at(0) == 's' && text == QLatin1String("slots"))
            return true;
        break;

    case 6:
        if (text.at(0) == 'S' && text == QLatin1String("SIGNAL"))
            return true;
        break;

    case 7:
        if (text.at(0) == 's' && text == QLatin1String("signals"))
            return true;
        else if (text.at(0) == 'f' && text == QLatin1String("foreach"))
            return true;
        else if (text.at(0) == 'f' && text == QLatin1String("forever"))
            return true;
        break;

    default:
        break;
    }
    return false;
}

void CppHighlighter::highlightWord(QStringRef word, int position, int length)
{
    // try to highlight Qt 'identifiers' like QObject and Q_PROPERTY
    // but don't highlight words like 'Query'
    if (word.length() > 1
        && word.at(0) == QLatin1Char('Q')
        && (word.at(1).isUpper()
            || word.at(1) == QLatin1Char('_')
            || word.at(1) == QLatin1Char('t'))) {
        setFormat(position, length, m_formats[CppTypeFormat]);
    }
}
