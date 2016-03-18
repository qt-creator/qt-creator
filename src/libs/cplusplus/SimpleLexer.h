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

    static int tokenAt(const Tokens &tokens, unsigned utf16charsOffset);
    static Token tokenAt(const QString &text,
                         unsigned utf16charsOffset,
                         int state,
                         const LanguageFeatures &languageFeatures);

    static int tokenBefore(const Tokens &tokens, unsigned utf16charsOffset);

private:
    int _lastState;
    LanguageFeatures _languageFeatures;
    bool _skipComments: 1;
    bool _endedJoined: 1;
    bool _ppMode: 1;
};

} // namespace CPlusPlus
