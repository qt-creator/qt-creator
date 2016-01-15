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

#include "SimpleLexer.h"

#include <cplusplus/ObjectiveCTypeQualifiers.h>
#include <cplusplus/Lexer.h>
#include <cplusplus/Token.h>

#include <QDebug>

using namespace CPlusPlus;

SimpleLexer::SimpleLexer()
    : _lastState(0),
      _skipComments(false),
      _endedJoined(false),
      _ppMode(false)
{}

SimpleLexer::~SimpleLexer()
{ }

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

Tokens SimpleLexer::operator()(const QString &text, int state)
{
    Tokens tokens;

    const QByteArray bytes = text.toUtf8();
    const char *firstChar = bytes.constData();
    const char *lastChar = firstChar + bytes.size();

    Lexer lex(firstChar, lastChar);
    lex.setLanguageFeatures(_languageFeatures);
    lex.setStartWithNewline(true);
    lex.setPreprocessorMode(_ppMode);

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

        QStringRef spell = text.midRef(tk.bytesBegin(), tk.bytes());
        lex.setScanAngleStringLiteralTokens(false);

        if (tk.newline() && tk.is(T_POUND))
            inPreproc = true;
        else if (inPreproc && tokens.size() == 1 && tk.is(T_IDENTIFIER) &&
                 spell == QLatin1String("include"))
            lex.setScanAngleStringLiteralTokens(true);
        else if (inPreproc && tokens.size() == 1 && tk.is(T_IDENTIFIER) &&
                 spell == QLatin1String("include_next"))
            lex.setScanAngleStringLiteralTokens(true);
        else if (_languageFeatures.objCEnabled
                 && inPreproc && tokens.size() == 1 && tk.is(T_IDENTIFIER) &&
                 spell == QLatin1String("import"))
            lex.setScanAngleStringLiteralTokens(true);

        tokens.append(tk);
    }

    _lastState = lex.state();
    return tokens;
}

int SimpleLexer::tokenAt(const Tokens &tokens, unsigned utf16charsOffset)
{
    for (int index = tokens.size() - 1; index >= 0; --index) {
        const Token &tk = tokens.at(index);
        if (tk.utf16charsBegin() <= utf16charsOffset && tk.utf16charsEnd() >= utf16charsOffset)
            return index;
    }

    return -1;
}

Token SimpleLexer::tokenAt(const QString &text,
                           unsigned utf16charsOffset,
                           int state,
                           const LanguageFeatures &languageFeatures)
{
    SimpleLexer tokenize;
    tokenize.setLanguageFeatures(languageFeatures);
    const QVector<Token> tokens = tokenize(text, state);
    const int tokenIdx = tokenAt(tokens, utf16charsOffset);
    return (tokenIdx == -1) ? Token() : tokens.at(tokenIdx);
}

int SimpleLexer::tokenBefore(const Tokens &tokens, unsigned utf16charsOffset)
{
    for (int index = tokens.size() - 1; index >= 0; --index) {
        const Token &tk = tokens.at(index);
        if (tk.utf16charsBegin() <= utf16charsOffset)
            return index;
    }

    return -1;
}
