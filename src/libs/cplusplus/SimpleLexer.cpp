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

#include "SimpleLexer.h"

#include "ObjectiveCTypeQualifiers.h"

#include <Lexer.h>
#include <Token.h>
#include <QtDebug>

using namespace CPlusPlus;

SimpleToken::SimpleToken(const Token &token, const QStringRef &text)
    : _kind(token.f.kind)
    , _flags(0)
    , _position(token.begin())
    , _length(token.f.length)
    , _text(text)
{
    f._whitespace = token.f.whitespace;
    f._newline = token.f.newline;
}

bool SimpleToken::isLiteral() const
{
    return _kind >= T_FIRST_LITERAL && _kind <= T_LAST_LITERAL;
}

bool SimpleToken::isOperator() const
{
    return _kind >= T_FIRST_OPERATOR && _kind <= T_LAST_OPERATOR;
}

bool SimpleToken::isKeyword() const
{
    return _kind >= T_FIRST_KEYWORD && _kind < T_FIRST_QT_KEYWORD;
}

bool SimpleToken::isComment() const
{
    return _kind == T_COMMENT || _kind == T_DOXY_COMMENT ||
            _kind == T_CPP_COMMENT || _kind == T_CPP_DOXY_COMMENT;
}

bool SimpleToken::isObjCAtKeyword() const
{
    return _kind >= T_FIRST_OBJC_KEYWORD && _kind <= T_LAST_OBJC_KEYWORD;
}

const char *SimpleToken::name() const
{
    return Token::name(_kind);
}

SimpleLexer::SimpleLexer()
    : _lastState(0),
      _skipComments(false),
      _qtMocRunEnabled(true),
      _objCEnabled(false)
{
}

SimpleLexer::~SimpleLexer()
{ }

bool SimpleLexer::qtMocRunEnabled() const
{
    return _qtMocRunEnabled;
}

void SimpleLexer::setQtMocRunEnabled(bool enabled)
{
    _qtMocRunEnabled = enabled;
}

bool SimpleLexer::objCEnabled() const
{
    return _objCEnabled;
}

void SimpleLexer::setObjCEnabled(bool onoff)
{
    _objCEnabled = onoff;
}

bool SimpleLexer::skipComments() const
{
    return _skipComments;
}

void SimpleLexer::setSkipComments(bool skipComments)
{
    _skipComments = skipComments;
}

QList<SimpleToken> SimpleLexer::operator()(const QString &text, int state)
{
    QList<SimpleToken> tokens;

    const QByteArray bytes = text.toLatin1();
    const char *firstChar = bytes.constData();
    const char *lastChar = firstChar + bytes.size();

    Lexer lex(firstChar, lastChar);
    lex.setQtMocRunEnabled(_qtMocRunEnabled);
    lex.setObjCEnabled(_objCEnabled);
    lex.setStartWithNewline(true);

    if (! _skipComments)
        lex.setScanCommentTokens(true);

    if (state != -1)
        lex.setState(state & 0xff);

    bool inPreproc = false;

    for (;;) {
        Token tk;
        lex(&tk);
        if (tk.is(T_EOF_SYMBOL))
            break;

        QStringRef spell = text.midRef(lex.tokenOffset(), lex.tokenLength());
        SimpleToken simpleTk(tk, spell);
        lex.setScanAngleStringLiteralTokens(false);

        if (tk.f.newline && tk.is(T_POUND))
            inPreproc = true;
        else if (inPreproc && tokens.size() == 1 && simpleTk.is(T_IDENTIFIER) &&
                 simpleTk.text() == QLatin1String("include"))
            lex.setScanAngleStringLiteralTokens(true);

        if (_objCEnabled && tk.is(T_IDENTIFIER))
            simpleTk.f._objcTypeQualifier = (classifyObjectiveCContextKeyword(firstChar + tk.offset, tk.f.length) != Token_identifier);

        tokens.append(simpleTk);
    }

    _lastState = lex.state();
    return tokens;
}


