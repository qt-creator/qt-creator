/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

TextScannerError::TextScannerError(const QString &errorMsg, const SourcePos &sourcePos)
    : Exception(errorMsg),
      m_sourcePos(sourcePos)
{
}

TextScannerError::~TextScannerError()
{
}

class TextScanner::TextScannerPrivate
{
public:
    QHash<QString, int> m_keywordToSubtypeMap;
    QHash<QString, int> m_operatorToSubtypeMap;
    int m_maxOperatorLength = 0;
    QSet<QChar> m_operatorFirstCharsSet;
    QSet<QChar> m_operatorCharsSet;
    ITextSource *m_source = 0;
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
    foreach (const DefTuple &tuple, keywords)
        d->m_keywordToSubtypeMap.insert(tuple.first.toLower(), tuple.second);
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
        if (op.length() > d->m_maxOperatorLength)
            d->m_maxOperatorLength = op.length();
        d->m_operatorFirstCharsSet.insert(op.at(0));
        foreach (const QChar ch, op)
            d->m_operatorCharsSet.insert(ch);
    }
}

void TextScanner::setSource(ITextSource *textSource)
{
    d->m_source = textSource;
}

SourcePos TextScanner::sourcePos() const
{
    return d->m_lastSourcePos;
}

Token TextScanner::read()
{
    if (!d->m_unreadTokens.isEmpty())
        return d->m_unreadTokens.pop();
    skipWhitespaces();
    SourceChar sourceChar = readChar();
    if (sourceChar.ch == QLatin1Char('\'') || sourceChar.ch == QLatin1Char('\"'))
        return scanString(sourceChar);
    else if (sourceChar.ch.isDigit())
        return scanNumber(sourceChar);
    else if (sourceChar.ch.isLetter() || sourceChar.ch == QLatin1Char('_'))
        return scanIdentifier(sourceChar);
    else if (sourceChar.ch == QLatin1Char('#'))
        return scanColorIdentifier(sourceChar);
    else if (sourceChar.ch == QChar::LineFeed || sourceChar.ch == QChar::CarriageReturn)
        return Token(Token::TokenEndOfLine, QString(), sourceChar.pos);
    else if (sourceChar.ch.isNull())
        return Token(Token::TokenEndOfInput, QString(), sourceChar.pos);
    else if (d->m_operatorFirstCharsSet.contains(sourceChar.ch))
        return scanOperator(sourceChar);
    else
        throw TextScannerError(QStringLiteral("Unexpected character."), sourceChar.pos);
}

void TextScanner::unread(const Token &token)
{
    d->m_unreadTokens.push(token);
}

SourceChar TextScanner::readChar()
{
    SourceChar ch;
    if (!d->m_unreadSourceChars.isEmpty())
        ch = d->m_unreadSourceChars.pop();
    else
        ch = d->m_source->readNextChar();
    d->m_lastSourcePos = ch.pos;
    return ch;
}

void TextScanner::unreadChar(const SourceChar &sourceChar)
{
    d->m_unreadSourceChars.push(sourceChar);
}

void TextScanner::skipWhitespaces()
{
    for (;;) {
        SourceChar sourceChar = readChar();
        if (sourceChar.ch == QLatin1Char('/')) {
            SourceChar secondSourceChar = readChar();
            if (secondSourceChar.ch == QLatin1Char('/')) {
                for (;;) {
                    SourceChar commentChar = readChar();
                    if (commentChar.ch.isNull()
                            || commentChar.ch == QChar::LineFeed
                            || commentChar.ch == QChar::CarriageReturn) {
                        break;
                    }
                }
            } else {
                unreadChar(secondSourceChar);
                unreadChar(sourceChar);
            }
        } else if (sourceChar.ch == QChar::LineFeed
                   || sourceChar.ch == QChar::CarriageReturn
                   || !sourceChar.ch.isSpace()) {
            unreadChar(sourceChar);
            return;
        }
    }
}

Token TextScanner::scanString(const SourceChar &delimiterChar)
{
    QString text;
    for (;;) {
        SourceChar sourceChar = readChar();
        if (sourceChar.ch == delimiterChar.ch) {
            return Token(Token::TokenString, text, delimiterChar.pos);
        } else if (sourceChar.ch == QLatin1Char('\\')) {
            sourceChar = readChar();
            if (sourceChar.ch == QLatin1Char('n'))
                text += QLatin1Char('\n');
            else if (sourceChar.ch == QLatin1Char('\\'))
                text += QLatin1Char('\\');
            else if (sourceChar.ch == QLatin1Char('t'))
                text += QLatin1Char('\t');
            else if (sourceChar.ch == QLatin1Char('\"'))
                text += QLatin1Char('\"');
            else if (sourceChar.ch == QLatin1Char('\''))
                text += QLatin1Char('\'');
            else
                throw TextScannerError(QStringLiteral("Unexpected character after '\\' in string constant."), sourceChar.pos);
        } else if (sourceChar.ch == QChar::LineFeed || sourceChar.ch == QChar::CarriageReturn) {
            throw TextScannerError(QStringLiteral("Unexpected end of line in string constant."), sourceChar.pos);
        } else {
            text += sourceChar.ch;
        }
    }
}

Token TextScanner::scanNumber(const SourceChar &firstDigit)
{
    QString text = firstDigit.ch;
    SourceChar sourceChar;
    for (;;) {
        sourceChar = readChar();
        if (!sourceChar.ch.isDigit())
            break;
        text += sourceChar.ch;
    }
    if (sourceChar.ch == QLatin1Char('.')) {
        text += sourceChar.ch;
        for (;;) {
            sourceChar = readChar();
            if (!sourceChar.ch.isDigit())
                break;
            text += sourceChar.ch;
        }
        unreadChar(sourceChar);
        return Token(Token::TokenFloat, text, firstDigit.pos);
    } else {
        unreadChar(sourceChar);
        return Token(Token::TokenInteger, text, firstDigit.pos);
    }
}

Token TextScanner::scanIdentifier(const SourceChar &firstChar)
{
    QString text = firstChar.ch;
    SourceChar sourceChar;
    for (;;) {
        sourceChar = readChar();
        if (!sourceChar.ch.isLetterOrNumber() && sourceChar.ch != QLatin1Char('_')) {
            unreadChar(sourceChar);
            QString keyword = text.toLower();
            if (d->m_keywordToSubtypeMap.contains(keyword))
                return Token(Token::TokenKeyword, d->m_keywordToSubtypeMap.value(keyword), text, firstChar.pos);
            return Token(Token::TokenIdentifier, text, firstChar.pos);
        }
        text += sourceChar.ch;
    }
}

Token TextScanner::scanColorIdentifier(const SourceChar &firstChar)
{
    QString text = firstChar.ch;
    SourceChar sourceChar;
    for (;;) {
        sourceChar = readChar();
        QChar ch = sourceChar.ch.toLower();
        if (!(ch.isDigit() || (ch >= QLatin1Char('a') && ch <= QLatin1Char('f')))) {
            unreadChar(sourceChar);
            return Token(Token::TokenColor, text, firstChar.pos);
        }
        text += sourceChar.ch;
    }
}

Token TextScanner::scanOperator(const SourceChar &firstChar)
{
    QString text = firstChar.ch;
    SourceChar sourceChar;
    QStack<SourceChar> extraChars;
    bool haveOperator = false;
    int operatorLength = 0;
    int subtype = 0;
    QString op;
    extraChars.push(firstChar);

    for (;;) {
        if (d->m_operatorToSubtypeMap.contains(text)) {
            haveOperator = true;
            operatorLength = text.length();
            subtype = d->m_operatorToSubtypeMap.value(text);
            op = text;
        }
        sourceChar = readChar();
        if (text.length() >= d->m_maxOperatorLength || !d->m_operatorCharsSet.contains(sourceChar.ch)) {
            unreadChar(sourceChar);
            int i = text.length();
            while (i > operatorLength) {
                --i;
                unreadChar(extraChars.pop());
            }
            QMT_CHECK(haveOperator);
            Q_UNUSED(haveOperator); // avoid warning in release mode
            return Token(Token::TokenOperator, subtype, op, firstChar.pos);
        }
        text += sourceChar.ch;
        extraChars.push(sourceChar);
    }
}

} // namespace qmt
