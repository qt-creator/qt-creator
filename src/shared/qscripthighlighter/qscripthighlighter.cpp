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

#include "qscripthighlighter.h"

#include <QtCore/QSet>
#include <QtCore/QtAlgorithms>
#include <QtCore/QDebug>

using namespace SharedTools;

QSet<QString> QScriptHighlighter::m_keywords;

QScriptHighlighter::QScriptHighlighter(bool duiEnabled, QTextDocument *parent):
        QSyntaxHighlighter(parent),
        m_scanner(m_duiEnabled),
        m_duiEnabled(duiEnabled)
{
    setFormats(defaultFormats());

    m_scanner.setKeywords(keywords());
}

QSet<QString> QScriptHighlighter::keywords()
{
    if (m_keywords.isEmpty()) {
        m_keywords << QLatin1String("Infinity");
        m_keywords << QLatin1String("NaN");
        m_keywords << QLatin1String("abstract");
        m_keywords << QLatin1String("boolean");
        m_keywords << QLatin1String("break");
        m_keywords << QLatin1String("byte");
        m_keywords << QLatin1String("case");
        m_keywords << QLatin1String("catch");
        m_keywords << QLatin1String("char");
        m_keywords << QLatin1String("class");
        m_keywords << QLatin1String("const");
        m_keywords << QLatin1String("constructor");
        m_keywords << QLatin1String("continue");
        m_keywords << QLatin1String("debugger");
        m_keywords << QLatin1String("default");
        m_keywords << QLatin1String("delete");
        m_keywords << QLatin1String("do");
        m_keywords << QLatin1String("double");
        m_keywords << QLatin1String("else");
        m_keywords << QLatin1String("enum");
        m_keywords << QLatin1String("export");
        m_keywords << QLatin1String("extends");
        m_keywords << QLatin1String("false");
        m_keywords << QLatin1String("final");
        m_keywords << QLatin1String("finally");
        m_keywords << QLatin1String("float");
        m_keywords << QLatin1String("for");
        m_keywords << QLatin1String("function");
        m_keywords << QLatin1String("goto");
        m_keywords << QLatin1String("if");
        m_keywords << QLatin1String("implements");
        m_keywords << QLatin1String("import");
        m_keywords << QLatin1String("in");
        m_keywords << QLatin1String("instanceof");
        m_keywords << QLatin1String("int");
        m_keywords << QLatin1String("interface");
        m_keywords << QLatin1String("long");
        m_keywords << QLatin1String("native");
        m_keywords << QLatin1String("new");
        m_keywords << QLatin1String("package");
        m_keywords << QLatin1String("private");
        m_keywords << QLatin1String("protected");
        m_keywords << QLatin1String("public");
        m_keywords << QLatin1String("return");
        m_keywords << QLatin1String("short");
        m_keywords << QLatin1String("static");
        m_keywords << QLatin1String("super");
        m_keywords << QLatin1String("switch");
        m_keywords << QLatin1String("synchronized");
        m_keywords << QLatin1String("this");
        m_keywords << QLatin1String("throw");
        m_keywords << QLatin1String("throws");
        m_keywords << QLatin1String("transient");
        m_keywords << QLatin1String("true");
        m_keywords << QLatin1String("try");
        m_keywords << QLatin1String("typeof");
        m_keywords << QLatin1String("undefined");
        m_keywords << QLatin1String("var");
        m_keywords << QLatin1String("void");
        m_keywords << QLatin1String("volatile");
        m_keywords << QLatin1String("while");
        m_keywords << QLatin1String("with");
    }

    return m_keywords;
}

bool QScriptHighlighter::isDuiEnabled() const
{ return m_duiEnabled; }

void QScriptHighlighter::highlightBlock(const QString &text)
{
    m_scanner(onBlockStart(), text);

    QTextCharFormat emptyFormat;
    foreach (const QScriptIncrementalScanner::Token &token, m_scanner.tokens()) {
        switch (token.kind) {
            case QScriptIncrementalScanner::Token::Keyword:
                setFormat(token.offset, token.length, m_formats[KeywordFormat]);
                break;

            case QScriptIncrementalScanner::Token::Type:
                setFormat(token.offset, token.length, m_formats[TypeFormat]);
                break;

            case QScriptIncrementalScanner::Token::Label:
                setFormat(token.offset, token.length, m_formats[LabelFormat]);
                break;

            case QScriptIncrementalScanner::Token::String:
                setFormat(token.offset, token.length, m_formats[StringFormat]);
                break;

            case QScriptIncrementalScanner::Token::Comment:
                setFormat(token.offset, token.length, m_formats[CommentFormat]);
                break;

            case QScriptIncrementalScanner::Token::Number:
                setFormat(token.offset, token.length, m_formats[NumberFormat]);
                break;

            case QScriptIncrementalScanner::Token::LeftParenthesis:
                onOpeningParenthesis('(', token.offset);
                break;

            case QScriptIncrementalScanner::Token::RightParenthesis:
                onClosingParenthesis(')', token.offset);
                break;

            case QScriptIncrementalScanner::Token::LeftBrace:
                onOpeningParenthesis('{', token.offset);
                break;

            case QScriptIncrementalScanner::Token::RightBrace:
                onClosingParenthesis('}', token.offset);
                break;

            case QScriptIncrementalScanner::Token::LeftBracket:
                onOpeningParenthesis('[', token.offset);
                break;

            case QScriptIncrementalScanner::Token::RightBracket:
                onClosingParenthesis(']', token.offset);
                break;

            case QScriptIncrementalScanner::Token::PreProcessor:
                setFormat(token.offset, token.length, m_formats[PreProcessorFormat]);
                break;

            case QScriptIncrementalScanner::Token::Empty:
            default:
                setFormat(token.offset, token.length, emptyFormat);
                break;

        }
    }

    onBlockEnd(m_scanner.endState(), m_scanner.firstNonSpace());
}

const QVector<QTextCharFormat> &QScriptHighlighter::defaultFormats()
{
    static QVector<QTextCharFormat> rc;
    if (rc.empty()) {
        rc.resize(NumFormats);
        rc[NumberFormat].setForeground(Qt::blue);
        rc[StringFormat].setForeground(Qt::darkGreen);
        rc[TypeFormat].setForeground(Qt::darkMagenta);
        rc[KeywordFormat].setForeground(Qt::darkYellow);
        rc[LabelFormat].setForeground(Qt::darkRed);
        rc[CommentFormat].setForeground(Qt::red);
        rc[CommentFormat].setFontItalic(true);
        rc[PreProcessorFormat].setForeground(Qt::darkBlue);
        rc[VisualWhitespace].setForeground(Qt::lightGray);
    }
    return rc;
}

void QScriptHighlighter::setFormats(const QVector<QTextCharFormat> &s)
{
    qCopy(s.constBegin(), s.constEnd(), m_formats);
}

int QScriptHighlighter::onBlockStart()
{
    int state = 0;
    int previousState = previousBlockState();
    if (previousState != -1)
        state = previousState;
    return state;
}
void QScriptHighlighter::onOpeningParenthesis(QChar, int) {}
void QScriptHighlighter::onClosingParenthesis(QChar, int) {}
void QScriptHighlighter::onBlockEnd(int state, int) { return setCurrentBlockState(state); }
