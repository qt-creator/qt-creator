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

#include <qmljs/qmljshighlighter.h>

#include <QtCore/QSet>
#include <QtCore/QtAlgorithms>
#include <QtCore/QDebug>

using namespace QmlJS;

QScriptHighlighter::QScriptHighlighter(bool duiEnabled, QTextDocument *parent):
        QSyntaxHighlighter(parent),
        m_duiEnabled(duiEnabled)
{
    QVector<QTextCharFormat> rc;
    rc.resize(NumFormats);
    rc[NumberFormat].setForeground(Qt::blue);
    rc[StringFormat].setForeground(Qt::darkGreen);
    rc[TypeFormat].setForeground(Qt::darkMagenta);
    rc[KeywordFormat].setForeground(Qt::darkYellow);
    rc[LabelFormat].setForeground(Qt::darkRed);
    rc[CommentFormat].setForeground(Qt::red); rc[CommentFormat].setFontItalic(true);
    rc[PreProcessorFormat].setForeground(Qt::darkBlue);
    rc[VisualWhitespace].setForeground(Qt::lightGray); // for debug: rc[VisualWhitespace].setBackground(Qt::red);
    setFormats(rc);
}

bool QScriptHighlighter::isDuiEnabled() const
{
    return m_duiEnabled;
}

QTextCharFormat QScriptHighlighter::labelTextCharFormat() const
{
    return m_formats[LabelFormat];
}

static bool checkStartOfBinding(const Token &token)
{
    switch (token.kind) {
    case Token::Semicolon:
    case Token::LeftBrace:
    case Token::RightBrace:
    case Token::LeftBracket:
    case Token::RightBracket:
        return true;

    default:
        return false;
    } // end of switch
}

void QScriptHighlighter::highlightBlock(const QString &text)
{
    const QList<Token> tokens = m_scanner(text, onBlockStart());

    int index = 0;
    while (index < tokens.size()) {
        const Token &token = tokens.at(index);

        switch (token.kind) {
            case Token::Keyword:
                setFormat(token.offset, token.length, m_formats[KeywordFormat]);
                break;

            case Token::LeftParenthesis:
                onOpeningParenthesis('(', token.offset);
                break;

            case Token::RightParenthesis:
                onClosingParenthesis(')', token.offset);
                break;

            case Token::LeftBrace:
                onOpeningParenthesis('{', token.offset);
                break;

            case Token::RightBrace:
                onClosingParenthesis('}', token.offset);
                break;

            case Token::LeftBracket:
                onOpeningParenthesis('[', token.offset);
                break;

            case Token::RightBracket:
                onClosingParenthesis(']', token.offset);
                break;

            case Token::Identifier: {
                if (m_duiEnabled && maybeQmlKeyword(text.midRef(token.offset, token.length))) {
                    // check the previous token
                    if (index == 0 || tokens.at(index - 1).isNot(Token::Dot)) {
                        if (index + 1 == tokens.size() || tokens.at(index + 1).isNot(Token::Colon)) {
                            setFormat(token.offset, token.length, m_formats[KeywordFormat]);
                            break;
                        }
                    }
                }

                if (index + 1 < tokens.size()) {
                    if (tokens.at(index + 1).is(Token::LeftBrace) && text.at(token.offset).isUpper()) {
                        setFormat(token.offset, token.length, m_formats[TypeFormat]);
                    } else if (index == 0 || checkStartOfBinding(tokens.at(index - 1))) {
                        const int start = index;

                        ++index; // skip the identifier.
                        while (index + 1 < tokens.size() &&
                               tokens.at(index).is(Token::Dot) &&
                               tokens.at(index + 1).is(Token::Identifier)) {
                            index += 2;
                        }

                        if (index < tokens.size() && tokens.at(index).is(Token::Colon)) {
                            // it's a binding.
                            for (int i = start; i < index; ++i) {
                                const Token &tok = tokens.at(i);
                                setFormat(tok.offset, tok.length, m_formats[LabelFormat]);
                            }
                            break;
                        } else {
                            index = start;
                        }
                    }
                }
            }   break;

            case Token::Delimiter:
                break;

            default:
                break;
        } // end swtich

        ++index;
    }

    int previousTokenEnd = 0;
    for (int index = 0; index < tokens.size(); ++index) {
        const Token &token = tokens.at(index);
        setFormat(previousTokenEnd, token.begin() - previousTokenEnd, m_formats[VisualWhitespace]);

        switch (token.kind) {
        case Token::Comment:
        case Token::String: {
            int i = token.begin(), e = token.end();
            while (i < e) {
                const QChar ch = text.at(i);
                if (ch.isSpace()) {
                    const int start = i;
                    do {
                        ++i;
                    } while (i < e && text.at(i).isSpace());
                    setFormat(start, i - start, m_formats[VisualWhitespace]);
                } else {
                    ++i;
                }
            }
        } break;

        default:
            break;
        } // end of switch

        previousTokenEnd = token.end();
    }

    setFormat(previousTokenEnd, text.length() - previousTokenEnd, m_formats[VisualWhitespace]);

    int firstNonSpace = 0;
    if (! tokens.isEmpty())
        firstNonSpace = tokens.first().offset;

    setCurrentBlockState(m_scanner.state());
    onBlockEnd(m_scanner.state(), firstNonSpace);
}

void QScriptHighlighter::setFormats(const QVector<QTextCharFormat> &s)
{
    Q_ASSERT(s.size() == NumFormats);
    qCopy(s.constBegin(), s.constEnd(), m_formats);
}

int QScriptHighlighter::onBlockStart()
{
    return currentBlockState();
}

void QScriptHighlighter::onBlockEnd(int, int)
{
}

void QScriptHighlighter::onOpeningParenthesis(QChar, int)
{
}

void QScriptHighlighter::onClosingParenthesis(QChar, int)
{
}

bool QScriptHighlighter::maybeQmlKeyword(const QStringRef &text) const
{
    if (text.isEmpty())
        return false;

    const QChar ch = text.at(0);
    if (ch == QLatin1Char('p') && text == QLatin1String("property")) {
        return true;
    } else if (ch == QLatin1Char('a') && text == QLatin1String("alias")) {
        return true;
    } else if (ch == QLatin1Char('s') && text == QLatin1String("signal")) {
        return true;
    } else if (ch == QLatin1Char('p') && text == QLatin1String("property")) {
        return true;
    } else if (ch == QLatin1Char('r') && text == QLatin1String("readonly")) {
        return true;
    } else if (ch == QLatin1Char('i') && text == QLatin1String("import")) {
        return true;
    } else {
        return false;
    }
}
