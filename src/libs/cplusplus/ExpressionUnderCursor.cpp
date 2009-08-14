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

#include "ExpressionUnderCursor.h"
#include "SimpleLexer.h"
#include <Token.h>

#include <QTextCursor>
#include <QTextBlock>

using namespace CPlusPlus;

namespace CPlusPlus {

class BackwardsScanner
{
    enum { MAX_BLOCK_COUNT = 10 };

public:
    BackwardsScanner(const QTextCursor &cursor)
        : _offset(0)
        , _blocksTokenized(0)
        , _block(cursor.block())
    {
        _tokenize.setSkipComments(true);
        _text = _block.text().left(cursor.position() - cursor.block().position());
        _tokens.append(_tokenize(_text, previousBlockState(_block)));
    }

    QList<SimpleToken> tokens() const { return _tokens; }

    const SimpleToken &operator[](int i)
    {
        while (_offset + i < 0) {
            _block = _block.previous();
            if (_blocksTokenized == MAX_BLOCK_COUNT || !_block.isValid()) {
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

    int startPosition() const
    { return _block.position(); }

    const QString &text() const
    { return _text; }

    QString text(int begin, int end) const
    {
        const SimpleToken &firstToken = _tokens.at(begin + _offset);
        const SimpleToken &lastToken = _tokens.at(end + _offset - 1);
        return _text.mid(firstToken.begin(), lastToken.end() - firstToken.begin());
    }

    int previousBlockState(const QTextBlock &block)
    {
        const QTextBlock prevBlock = block.previous();
        if (prevBlock.isValid()) {
            int state = prevBlock.userState();

            if (state != -1)
                return state;
        }
        return 0;
    }

private:
    QList<SimpleToken> _tokens;
    int _offset;
    int _blocksTokenized;
    QTextBlock _block;
    QString _text;
    SimpleLexer _tokenize;
};

}

ExpressionUnderCursor::ExpressionUnderCursor()
{ }

ExpressionUnderCursor::~ExpressionUnderCursor()
{ }

int ExpressionUnderCursor::startOfMatchingBrace(BackwardsScanner &tk, int index)
{
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

int ExpressionUnderCursor::startOfExpression(BackwardsScanner &tk, int index)
{
    // tk is a reference to a const QList. So, don't worry about [] access.
    // ### TODO implement multiline support. It should be pretty easy.
    if (tk[index - 1].isLiteral()) {
        return index - 1;
    } else if (tk[index - 1].is(T_THIS)) {
        return index - 1;
    } else if (tk[index - 1].is(T_TYPEID)) {
        return index - 1;
    } else if (tk[index - 1].is(T_SIGNAL)) {
        if (tk[index - 2].is(T_COMMA) && !_jumpedComma) {
            _jumpedComma = true;
            return startOfExpression(tk, index - 2);
        }
        return index - 1;
    } else if (tk[index - 1].is(T_SLOT)) {
        if (tk[index - 2].is(T_COMMA) && !_jumpedComma) {
            _jumpedComma = true;
            return startOfExpression(tk, index - 2);
        }
        return index - 1;
    } else if (tk[index - 1].is(T_IDENTIFIER)) {
        if (tk[index - 2].is(T_TILDE)) {
            if (tk[index - 3].is(T_COLON_COLON)) {
                return startOfExpression(tk, index - 3);
            } else if (tk[index - 3].is(T_DOT) || tk[index - 3].is(T_ARROW)) {
                return startOfExpression(tk, index - 3);
            }
            return index - 2;
        } else if (tk[index - 2].is(T_COLON_COLON)) {
            return startOfExpression(tk, index - 1);
        } else if (tk[index - 2].is(T_DOT) || tk[index - 2].is(T_ARROW)) {
            return startOfExpression(tk, index - 2);
        } else if (tk[index - 2].is(T_DOT_STAR) || tk[index - 2].is(T_ARROW_STAR)) {
            return startOfExpression(tk, index - 2);
        }
        return index - 1;
    } else if (tk[index - 1].is(T_RPAREN)) {
        int rparenIndex = startOfMatchingBrace(tk, index);
        if (rparenIndex != index) {
            if (tk[rparenIndex - 1].is(T_GREATER)) {
                int lessIndex = startOfMatchingBrace(tk, rparenIndex);
                if (lessIndex != rparenIndex - 1) {
                    if (tk[lessIndex - 1].is(T_DYNAMIC_CAST)     ||
                        tk[lessIndex - 1].is(T_STATIC_CAST)      ||
                        tk[lessIndex - 1].is(T_CONST_CAST)       ||
                        tk[lessIndex - 1].is(T_REINTERPRET_CAST))
                        return lessIndex - 1;
                    else if (tk[lessIndex - 1].is(T_IDENTIFIER))
                        return startOfExpression(tk, lessIndex);
                    else if (tk[lessIndex - 1].is(T_SIGNAL))
                        return startOfExpression(tk, lessIndex);
                    else if (tk[lessIndex - 1].is(T_SLOT))
                        return startOfExpression(tk, lessIndex);
                }
            }
            return startOfExpression(tk, rparenIndex);
        }
        return index;
    } else if (tk[index - 1].is(T_RBRACKET)) {
        int rbracketIndex = startOfMatchingBrace(tk, index);
        if (rbracketIndex != index)
            return startOfExpression(tk, rbracketIndex);
        return index;
    } else if (tk[index - 1].is(T_COLON_COLON)) {
        if (tk[index - 2].is(T_GREATER)) { // ### not exactly
            int lessIndex = startOfMatchingBrace(tk, index - 1);
            if (lessIndex != index - 1)
                return startOfExpression(tk, lessIndex);
            return index - 1;
        } else if (tk[index - 2].is(T_IDENTIFIER)) {
            return startOfExpression(tk, index - 1);
        }
        return index - 1;
    } else if (tk[index - 1].is(T_DOT) || tk[index - 1].is(T_ARROW)) {
        return startOfExpression(tk, index - 1);
    } else if (tk[index - 1].is(T_DOT_STAR) || tk[index - 1].is(T_ARROW_STAR)) {
        return startOfExpression(tk, index - 1);
    }

    return index;
}

bool ExpressionUnderCursor::isAccessToken(const SimpleToken &tk)
{
    switch (tk.kind()) {
    case T_COLON_COLON:
    case T_DOT:      case T_ARROW:
    case T_DOT_STAR: case T_ARROW_STAR:
        return true;
    default:
        return false;
    } // switch
}

QString ExpressionUnderCursor::operator()(const QTextCursor &cursor)
{
    BackwardsScanner scanner(cursor);

    _jumpedComma = false;

    const int initialSize = scanner.tokens().size();
    const int i = startOfExpression(scanner, initialSize);
    if (i == initialSize)
        return QString();

    return scanner.text(i, initialSize);
}

int ExpressionUnderCursor::startOfFunctionCall(const QTextCursor &cursor)
{
    QString text;

    BackwardsScanner scanner(cursor);

    int index = scanner.tokens().size();

    forever {
        const SimpleToken &tk = scanner[index - 1];

        if (tk.is(T_EOF_SYMBOL))
            break;
        else if (tk.is(T_LPAREN))
            return scanner.startPosition() + tk.position();
        else if (tk.is(T_RPAREN)) {
            int matchingBrace = startOfMatchingBrace(scanner, index);

            if (matchingBrace == index) // If no matching brace found
                return -1;

            index = matchingBrace;
        } else
            --index;
    }

    return -1;
}
