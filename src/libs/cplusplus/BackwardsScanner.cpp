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

#include "BackwardsScanner.h"

#include <cplusplus/Token.h>

#include <utils/algorithm.h>

#include <QTextCursor>
#include <QTextDocument>

using namespace CPlusPlus;

BackwardsScanner::BackwardsScanner(const QTextCursor &cursor,
                                   const LanguageFeatures &languageFeatures,
                                   int maxBlockCount,
                                   const QString &suffix,
                                   bool skipComments)
    : _offset(0)
    , _blocksTokenized(0)
    , _block(cursor.block())
    , _maxBlockCount(maxBlockCount)
{
    _tokenize.setLanguageFeatures(languageFeatures);
    _tokenize.setSkipComments(skipComments);
    _text = _block.text().left(cursor.position() - cursor.block().position());

    if (! suffix.isEmpty())
        _text += suffix;

    _tokens += _tokenize(_text, previousBlockState(_block));

    _startToken = _tokens.size();
}

Token BackwardsScanner::LA(int index) const
{ return const_cast<BackwardsScanner *>(this)->fetchToken(_startToken - index); }

Token BackwardsScanner::operator[](int index) const
{ return const_cast<BackwardsScanner *>(this)->fetchToken(index); }

const Token &BackwardsScanner::fetchToken(int tokenIndex)
{
    while (_offset + tokenIndex < 0) {
        _block = _block.previous();
        if (_blocksTokenized == _maxBlockCount || !_block.isValid()) {
            ++_offset;
            _tokens.prepend(Token()); // sentinel
            break;
        } else {
            ++_blocksTokenized;

            QString blockText = _block.text();
            _text.prepend(QLatin1Char('\n'));
            _text.prepend(blockText);

            const int offset = blockText.length() + 1;
            const Tokens adaptedTokens = Utils::transform(_tokens, [offset](Token token) {
                token.utf16charOffset += unsigned(offset);
                return token;
            });

            _tokens = _tokenize(blockText, previousBlockState(_block));
            _offset += _tokens.size();
            _tokens += adaptedTokens;
        }
    }

    return _tokens.at(_offset + tokenIndex);
}

int BackwardsScanner::startToken() const
{ return _startToken; }

int BackwardsScanner::startPosition() const
{ return _block.position(); }

QString BackwardsScanner::text() const
{ return _text; }

QString BackwardsScanner::mid(int index) const
{
    const Token &firstToken = _tokens.at(index + _offset);
    return _text.mid(firstToken.utf16charsBegin());
}

QString BackwardsScanner::text(int index) const
{
    const Token &firstToken = _tokens.at(index + _offset);
    return _text.mid(firstToken.utf16charsBegin(), firstToken.utf16chars());
}

QStringRef BackwardsScanner::textRef(int index) const
{
    const Token &firstToken = _tokens.at(index + _offset);
    return _text.midRef(firstToken.utf16charsBegin(), firstToken.utf16chars());
}

int BackwardsScanner::size() const
{
    return _tokens.size();
}

int BackwardsScanner::startOfMatchingBrace(int index) const
{
    const BackwardsScanner &tk = *this;

    if (tk[index - 1].is(T_RPAREN)) {
        int i = index - 1;
        int count = 0;
        do {
            if (tk[i].is(T_LPAREN)) {
                if (! ++count)
                    return i;
            } else if (tk[i].is(T_RPAREN))
                --count;
            --i;
        } while (count != 0 && tk[i].isNot(T_EOF_SYMBOL));
    } else if (tk[index - 1].is(T_RBRACKET)) {
        int i = index - 1;
        int count = 0;
        do {
            if (tk[i].is(T_LBRACKET)) {
                if (! ++count)
                    return i;
            } else if (tk[i].is(T_RBRACKET))
                --count;
            --i;
        } while (count != 0 && tk[i].isNot(T_EOF_SYMBOL));
    } else if (tk[index - 1].is(T_RBRACE)) {
        int i = index - 1;
        int count = 0;
        do {
            if (tk[i].is(T_LBRACE)) {
                if (! ++count)
                    return i;
            } else if (tk[i].is(T_RBRACE))
                --count;
            --i;
        } while (count != 0 && tk[i].isNot(T_EOF_SYMBOL));
    } else if (tk[index - 1].is(T_GREATER)) {
        int i = index - 1;
        int count = 0;
        do {
            if (tk[i].is(T_LESS)) {
                if (! ++count)
                    return i;
            } else if (tk[i].is(T_GREATER))
                --count;
            --i;
        } while (count != 0 && tk[i].isNot(T_EOF_SYMBOL));
    } else {
        Q_ASSERT(0);
    }

    return index;
}

int BackwardsScanner::startOfLine(int index) const
{
    const BackwardsScanner tk(*this);

    forever {
        const Token &tok = tk[index - 1];

        if (tok.is(T_EOF_SYMBOL))
            break;
        else if (tok.newline())
            return index - 1;

        --index;
    }

    return index;
}

int BackwardsScanner::startOfBlock(int index) const
{
    const BackwardsScanner tk(*this);

    const int start = index;

    forever {
        Token token = tk[index - 1];

        if (token.is(T_EOF_SYMBOL)) {
            break;

        } else if (token.is(T_GREATER)) {
            const int matchingBrace = startOfMatchingBrace(index);

            if (matchingBrace != index && tk[matchingBrace - 1].is(T_TEMPLATE))
                index = matchingBrace;

        } else if (token.is(T_RPAREN) || token.is(T_RBRACKET) || token.is(T_RBRACE)) {
            const int matchingBrace = startOfMatchingBrace(index);

            if (matchingBrace != index)
                index = matchingBrace;

        } else if (token.is(T_LPAREN) || token.is(T_LBRACKET)) {
            break; // unmatched brace

        } else if (token.is(T_LBRACE)) {
            return index - 1;

        }

        --index;
    }

    return start;
}

QString BackwardsScanner::indentationString(int index) const
{
    const Token tokenAfterNewline = operator[](startOfLine(index + 1));
    const int newlinePos = qMax(0, _text.lastIndexOf(QLatin1Char('\n'),
                                                     tokenAfterNewline.utf16charsBegin()));
    return _text.mid(newlinePos, tokenAfterNewline.utf16charsBegin() - newlinePos);
}


int BackwardsScanner::previousBlockState(const QTextBlock &block)
{
    const QTextBlock prevBlock = block.previous();

    if (prevBlock.isValid()) {
        int state = prevBlock.userState();

        if (state != -1)
            return state;
    }

    return 0;
}
