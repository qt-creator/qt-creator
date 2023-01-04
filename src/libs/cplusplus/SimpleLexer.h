// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cplusplus/CPlusPlusForwardDeclarations.h>
#include <cplusplus/Token.h>

#include <QString>
#include <QVector>

namespace CPlusPlus {

class SimpleLexer;
class Token;
typedef QVector<Token> Tokens;

class CPLUSPLUS_EXPORT SimpleLexer
{
public:
    SimpleLexer();
    ~SimpleLexer();

    bool skipComments() const;
    void setSkipComments(bool skipComments);

    void setPreprocessorMode(bool ppMode)
    { _ppMode = ppMode; }

    LanguageFeatures languageFeatures() const { return _languageFeatures; }
    void setLanguageFeatures(LanguageFeatures features) { _languageFeatures = features; }

    bool endedJoined() const;

    Tokens operator()(const QString &text, int state = 0);

    int state() const
    { return _lastState; }

    QByteArray expectedRawStringSuffix() const { return _expectedRawStringSuffix; }
    void setExpectedRawStringSuffix(const QByteArray &suffix)
    { _expectedRawStringSuffix = suffix; }

    static int tokenAt(const Tokens &tokens, int utf16charsOffset);
    static Token tokenAt(const QString &text,
                         int utf16charsOffset,
                         int state,
                         const LanguageFeatures &languageFeatures);

    static int tokenBefore(const Tokens &tokens, int utf16charsOffset);

private:
    QByteArray _expectedRawStringSuffix;
    int _lastState;
    LanguageFeatures _languageFeatures;
    bool _skipComments: 1;
    bool _endedJoined: 1;
    bool _ppMode: 1;
};

} // namespace CPlusPlus
