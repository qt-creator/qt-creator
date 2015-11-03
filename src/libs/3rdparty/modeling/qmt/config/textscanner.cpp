/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
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

#include "textscanner.h"

#include "textsource.h"
#include "token.h"

#include "qmt/infrastructure/qmtassert.h"

#include <QHash>
#include <QSet>
#include <QStack>
#include <QPair>

typedef QPair<QString, int> DefTuple;

namespace qmt {

TextScannerError::TextScannerError(const QString &error_msg, const SourcePos &source_pos)
    : Exception(error_msg),
      m_sourcePos(source_pos)
{
}

TextScannerError::~TextScannerError()
{
}

struct TextScanner::TextScannerPrivate {
    TextScannerPrivate()
        : m_maxOperatorLength(0),
          m_source(0)
    {
    }

    QHash<QString, int> m_keywordToSubtypeMap;
    QHash<QString, int> m_operatorToSubtypeMap;
    int m_maxOperatorLength;
    QSet<QChar> m_operatorFirstCharsSet;
    QSet<QChar> m_operatorCharsSet;
    ITextSource *m_source;
    QStack<SourceChar> m_unreadSourceChars;
    SourcePos m_lastSourcePos;
    QStack<Token> m_unreadTokens;
};

TextScanner::TextScanner(QObject *parent) :
    QObject(parent),
    d(new TextScannerPrivate)
{
}

TextScanner::~TextScanner()
{
    delete d;
}

void TextScanner::setKeywords(const QList<QPair<QString, int> > &keywords)
{
    d->m_keywordToSubtypeMap.clear();
    foreach (const DefTuple &tuple, keywords) {
        d->m_keywordToSubtypeMap.insert(tuple.first.toLower(), tuple.second);
    }
}

void TextScanner::setOperators(const QList<QPair<QString, int> > &operators)
{
    d->m_operatorToSubtypeMap.clear();
    d->m_maxOperatorLength = 0;
    d->m_operatorFirstCharsSet.clear();
    d->m_operatorCharsSet.clear();
    foreach (const DefTuple &tuple, operators) {
        QString op = tuple.first;
        d->m_operatorToSubtypeMap.insert(op, tuple.second);
        if (op.length() > d->m_maxOperatorLength) {
            d->m_maxOperatorLength = op.length();
        }
        d->m_operatorFirstCharsSet.insert(op.at(0));
        foreach (const QChar ch, op) {
            d->m_operatorCharsSet.insert(ch);
        }
    }
}

void TextScanner::setSource(ITextSource *text_source)
{
    d->m_source = text_source;
}

SourcePos TextScanner::getSourcePos() const
{
    return d->m_lastSourcePos;
}

Token TextScanner::read()
{
    if (!d->m_unreadTokens.isEmpty()) {
        return d->m_unreadTokens.pop();
    }
    skipWhitespaces();
    SourceChar source_char = readChar();
    if (source_char.ch == QLatin1Char('\'') || source_char.ch == QLatin1Char('\"')) {
        return scanString(source_char);
    } else if (source_char.ch.isDigit()) {
        return scanNumber(source_char);
    } else if (source_char.ch.isLetter() || source_char.ch == QLatin1Char('_')) {
        return scanIdentifier(source_char);
    } else if (source_char.ch == QLatin1Char('#')) {
        return scanColorIdentifier(source_char);
    } else if (source_char.ch == QChar::LineFeed || source_char.ch == QChar::CarriageReturn) {
        return Token(Token::TOKEN_ENDOFLINE, QString(), source_char.pos);
    } else if (source_char.ch.isNull()) {
        return Token(Token::TOKEN_ENDOFINPUT, QString(), source_char.pos);
    } else if (d->m_operatorFirstCharsSet.contains(source_char.ch)) {
        return scanOperator(source_char);
    } else {
        throw TextScannerError(QStringLiteral("Unexpected character."), source_char.pos);
    }
}

void TextScanner::unread(const Token &token)
{
    d->m_unreadTokens.push(token);
}

SourceChar TextScanner::readChar()
{
    SourceChar ch;
    if (!d->m_unreadSourceChars.isEmpty()) {
        ch = d->m_unreadSourceChars.pop();
    } else {
        ch = d->m_source->readNextChar();
    }
    d->m_lastSourcePos = ch.pos;
    return ch;
}

void TextScanner::unreadChar(const SourceChar &source_char)
{
    d->m_unreadSourceChars.push(source_char);
}

void TextScanner::skipWhitespaces()
{
    for (;;) {
        SourceChar source_char = readChar();
        if (source_char.ch == QLatin1Char('/')) {
            SourceChar second_source_char = readChar();
            if (second_source_char.ch == QLatin1Char('/')) {
                for (;;) {
                    SourceChar comment_char = readChar();
                    if (comment_char.ch.isNull() || comment_char.ch == QChar::LineFeed || comment_char.ch == QChar::CarriageReturn) {
                        break;
                    }
                }
            } else {
                unreadChar(second_source_char);
                unreadChar(source_char);
            }
        } else if (source_char.ch == QChar::LineFeed || source_char.ch == QChar::CarriageReturn || !source_char.ch.isSpace()) {
            unreadChar(source_char);
            return;
        }
    }
}

Token TextScanner::scanString(const SourceChar &delimiter_char)
{
    QString text;
    for (;;) {
        SourceChar source_char = readChar();
        if (source_char.ch == delimiter_char.ch) {
            return Token(Token::TOKEN_STRING, text, delimiter_char.pos);
        } else if (source_char.ch == QLatin1Char('\\')) {
            source_char = readChar();
            if (source_char.ch == QLatin1Char('n')) {
                text += QLatin1Char('\n');
            } else if (source_char.ch == QLatin1Char('\\')) {
                text += QLatin1Char('\\');
            } else if (source_char.ch == QLatin1Char('t')) {
                text += QLatin1Char('\t');
            } else if (source_char.ch == QLatin1Char('\"')) {
                text += QLatin1Char('\"');
            } else if (source_char.ch == QLatin1Char('\'')) {
                text += QLatin1Char('\'');
            } else {
                throw TextScannerError(QStringLiteral("Unexpected character after '\\' in string constant."), source_char.pos);
            }
        } else if (source_char.ch == QChar::LineFeed || source_char.ch == QChar::CarriageReturn) {
            throw TextScannerError(QStringLiteral("Unexpected end of line in string constant."), source_char.pos);
        } else {
            text += source_char.ch;
        }
    }
}

Token TextScanner::scanNumber(const SourceChar &first_digit)
{
    QString text = first_digit.ch;
    SourceChar source_char;
    for (;;) {
        source_char = readChar();
        if (!source_char.ch.isDigit()) {
            break;
        }
        text += source_char.ch;
    }
    if (source_char.ch == QLatin1Char('.')) {
        text += source_char.ch;
        for (;;) {
            source_char = readChar();
            if (!source_char.ch.isDigit()) {
                break;
            }
            text += source_char.ch;
        }
        unreadChar(source_char);
        return Token(Token::TOKEN_FLOAT, text, first_digit.pos);
    } else {
        unreadChar(source_char);
        return Token(Token::TOKEN_INTEGER, text, first_digit.pos);
    }
}

Token TextScanner::scanIdentifier(const SourceChar &first_char)
{
    QString text = first_char.ch;
    SourceChar source_char;
    for (;;) {
        source_char = readChar();
        if (!source_char.ch.isLetterOrNumber() && source_char.ch != QLatin1Char('_')) {
            unreadChar(source_char);
            QString keyword = text.toLower();
            if (d->m_keywordToSubtypeMap.contains(keyword)) {
                return Token(Token::TOKEN_KEYWORD, d->m_keywordToSubtypeMap.value(keyword), text, first_char.pos);
            }
            return Token(Token::TOKEN_IDENTIFIER, text, first_char.pos);
        }
        text += source_char.ch;
    }
}

Token TextScanner::scanColorIdentifier(const SourceChar &first_char)
{
    QString text = first_char.ch;
    SourceChar source_char;
    for (;;) {
        source_char = readChar();
        QChar ch = source_char.ch.toLower();
        if (!(ch.isDigit() || (ch >= QLatin1Char('a') && ch <= QLatin1Char('f')))) {
            unreadChar(source_char);
            return Token(Token::TOKEN_COLOR, text, first_char.pos);
        }
        text += source_char.ch;
    }
}

Token TextScanner::scanOperator(const SourceChar &first_char)
{
    QString text = first_char.ch;
    SourceChar source_char;
    QStack<SourceChar> extra_chars;
    bool have_operator = false;
    int operator_length = 0;
    int subtype = 0;
    QString op;
    extra_chars.push(first_char);

    for (;;) {
        if (d->m_operatorToSubtypeMap.contains(text)) {
            have_operator = true;
            operator_length = text.length();
            subtype = d->m_operatorToSubtypeMap.value(text);
            op = text;
        }
        source_char = readChar();
        if (text.length() >= d->m_maxOperatorLength || !d->m_operatorCharsSet.contains(source_char.ch)) {
            unreadChar(source_char);
            int i = text.length();
            while (i > operator_length) {
                --i;
                unreadChar(extra_chars.pop());
            }
            QMT_CHECK(have_operator);
            Q_UNUSED(have_operator); // avoid warning in release mode
            return Token(Token::TOKEN_OPERATOR, subtype, op, first_char.pos);
        }
        text += source_char.ch;
        extra_chars.push(source_char);
    }
}

} // namespace qmt
