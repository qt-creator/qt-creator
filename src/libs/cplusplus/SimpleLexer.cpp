// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    lex.setExpectedRawStringSuffix(_expectedRawStringSuffix);
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
        const QStringView spell = tk.utf16charsBegin() + tk.utf16chars() > text.size()
                ? QStringView(text).mid(tk.utf16charsBegin())
                : QStringView(text).mid(tk.utf16charsBegin(), tk.utf16chars());
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
    _expectedRawStringSuffix = lex.expectedRawStringSuffix();
    return tokens;
}

int SimpleLexer::tokenAt(const Tokens &tokens, int utf16charsOffset)
{
    for (int index = tokens.size() - 1; index >= 0; --index) {
        const Token &tk = tokens.at(index);
        if (tk.utf16charsBegin() <= utf16charsOffset && tk.utf16charsEnd() >= utf16charsOffset)
            return index;
    }

    return -1;
}

Token SimpleLexer::tokenAt(const QString &text,
                           int utf16charsOffset,
                           int state,
                           const LanguageFeatures &languageFeatures)
{
    SimpleLexer tokenize;
    tokenize.setLanguageFeatures(languageFeatures);
    const QVector<Token> tokens = tokenize(text, state);
    const int tokenIdx = tokenAt(tokens, utf16charsOffset);
    return (tokenIdx == -1) ? Token() : tokens.at(tokenIdx);
}

int SimpleLexer::tokenBefore(const Tokens &tokens, int utf16charsOffset)
{
    for (int index = tokens.size() - 1; index >= 0; --index) {
        const Token &tk = tokens.at(index);
        if (tk.utf16charsBegin() <= utf16charsOffset)
            return index;
    }

    return -1;
}
