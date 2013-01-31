/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "SimpleLexer.h"

#include "ObjectiveCTypeQualifiers.h"

#include <Lexer.h>
#include <Token.h>
#include <QDebug>

using namespace CPlusPlus;

SimpleLexer::SimpleLexer()
    : _lastState(0),
      _skipComments(false),
      _qtMocRunEnabled(true),
      _objCEnabled(false),
      _endedJoined(false),
      _cxx0xEnabled(false)
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

bool SimpleLexer::cxx0xEnabled() const
{
    return _cxx0xEnabled;
}

void SimpleLexer::setCxx0xEnabled(bool enabled)
{
    _cxx0xEnabled = enabled;
}

bool SimpleLexer::skipComments() const
{
    return _skipComments;
}

void SimpleLexer::setSkipComments(bool skipComments)
{
    _skipComments = skipComments;
}

bool SimpleLexer::endedJoined() const
{
    return _endedJoined;
}

QList<Token> SimpleLexer::operator()(const QString &text, int state)
{
    QList<Token> tokens;

    const QByteArray bytes = text.toLatin1();
    const char *firstChar = bytes.constData();
    const char *lastChar = firstChar + bytes.size();

    Lexer lex(firstChar, lastChar);
    lex.setQtMocRunEnabled(_qtMocRunEnabled);
    lex.setObjCEnabled(_objCEnabled);
    lex.setStartWithNewline(true);
    lex.setObjCEnabled(_objCEnabled);
    lex.setCxxOxEnabled(_cxx0xEnabled);

    if (! _skipComments)
        lex.setScanCommentTokens(true);

    if (state != -1)
        lex.setState(state & 0xff);

    bool inPreproc = false;

    for (;;) {
        Token tk;
        lex(&tk);
        if (tk.is(T_EOF_SYMBOL)) {
            _endedJoined = tk.joined();
            break;
        }

        QStringRef spell = text.midRef(lex.tokenOffset(), lex.tokenLength());
        lex.setScanAngleStringLiteralTokens(false);

        if (tk.f.newline && tk.is(T_POUND))
            inPreproc = true;
        else if (inPreproc && tokens.size() == 1 && tk.is(T_IDENTIFIER) &&
                 spell == QLatin1String("include"))
            lex.setScanAngleStringLiteralTokens(true);
        else if (inPreproc && tokens.size() == 1 && tk.is(T_IDENTIFIER) &&
                 spell == QLatin1String("include_next"))
            lex.setScanAngleStringLiteralTokens(true);
        else if (_objCEnabled
                 && inPreproc && tokens.size() == 1 && tk.is(T_IDENTIFIER) &&
                 spell == QLatin1String("import"))
            lex.setScanAngleStringLiteralTokens(true);

        tokens.append(tk);
    }

    _lastState = lex.state();
    return tokens;
}

int SimpleLexer::tokenAt(const QList<Token> &tokens, unsigned offset)
{
    for (int index = tokens.size() - 1; index >= 0; --index) {
        const Token &tk = tokens.at(index);
        if (tk.begin() <= offset && tk.end() >= offset)
            return index;
    }

    return -1;
}

Token SimpleLexer::tokenAt(const QString &text,
                           unsigned offset,
                           int state,
                           bool qtMocRunEnabled)
{
    SimpleLexer tokenize;
    tokenize.setQtMocRunEnabled(qtMocRunEnabled);
    const QList<Token> tokens = tokenize(text, state);
    const int tokenIdx = tokenAt(tokens, offset);
    return (tokenIdx == -1) ? Token() : tokens.at(tokenIdx);
}

int SimpleLexer::tokenBefore(const QList<Token> &tokens, unsigned offset)
{
    for (int index = tokens.size() - 1; index >= 0; --index) {
        const Token &tk = tokens.at(index);
        if (tk.begin() <= offset)
            return index;
    }

    return -1;
}
