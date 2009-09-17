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
#include "BackwardsScanner.h"
#include <Token.h>
#include <QtGui/QTextCursor>

using namespace CPlusPlus;

BackwardsScanner::BackwardsScanner(const QTextCursor &cursor, int maxBlockCount)
    : _offset(0)
    , _blocksTokenized(0)
    , _block(cursor.block())
    , _maxBlockCount(maxBlockCount)
{
    _tokenize.setSkipComments(true);
    _text = _block.text().left(cursor.position() - cursor.block().position());
    _tokens.append(_tokenize(_text, previousBlockState(_block)));
}

int BackwardsScanner::state() const
{ return _tokenize.state(); }

const QList<SimpleToken> &BackwardsScanner::tokens() const
{ return _tokens; }

const SimpleToken &BackwardsScanner::operator[](int i) const
{ return const_cast<BackwardsScanner *>(this)->fetchToken(i); }

const SimpleToken &BackwardsScanner::fetchToken(int i)
{
    while (_offset + i < 0) {
        _block = _block.previous();
        if (_blocksTokenized == _maxBlockCount || !_block.isValid()) {
            ++_offset;
            _tokens.prepend(SimpleToken()); // sentinel
            break;
        } else {
            ++_blocksTokenized;

            QString blockText = _block.text();
            _text.prepend(blockText);
            QList<SimpleToken> adaptedTokens;
            for (int i = 0; i < _tokens.size(); ++i) {
                const SimpleToken &t = _tokens.at(i);
                const int position = t.position() + blockText.length();
                adaptedTokens.append(SimpleToken(t.kind(),
                                                 position,
                                                 t.length(),
                                                 _text.midRef(position, t.length())));
            }

            _tokens = _tokenize(blockText, previousBlockState(_block));
            _offset += _tokens.size();
            _tokens += adaptedTokens;
        }
    }

    return _tokens.at(_offset + i);
}

int BackwardsScanner::startToken() const
{ return _tokens.size(); }

int BackwardsScanner::startPosition() const
{ return _block.position(); }

QString BackwardsScanner::text() const
{ return _text; }

QString BackwardsScanner::text(int begin, int end) const
{
    const SimpleToken &firstToken = _tokens.at(begin + _offset);
    const SimpleToken &lastToken = _tokens.at(end + _offset - 1);
    return _text.mid(firstToken.begin(), lastToken.end() - firstToken.begin());
}

QStringRef BackwardsScanner::textRef(int begin, int end) const
{
    const SimpleToken &firstToken = _tokens.at(begin + _offset);
    const SimpleToken &lastToken = _tokens.at(end + _offset - 1);
    return _text.midRef(firstToken.begin(), lastToken.end() - firstToken.begin());
}

int BackwardsScanner::previousBlockState(const QTextBlock &block) const
{
    const QTextBlock prevBlock = block.previous();

    if (prevBlock.isValid()) {
        int state = prevBlock.userState();

        if (state != -1)
            return state;
    }

    return 0;
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
    }

    return index;
}
