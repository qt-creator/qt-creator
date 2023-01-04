// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "SimpleLexer.h"

#include <QStringView>
#include <QTextBlock>

QT_FORWARD_DECLARE_CLASS(QTextCursor)

namespace CPlusPlus {

class CPLUSPLUS_EXPORT BackwardsScanner
{
    enum { MAX_BLOCK_COUNT = 10 };

public:
    BackwardsScanner(const QTextCursor &cursor,
                     const LanguageFeatures &languageFeatures,
                     int maxBlockCount = MAX_BLOCK_COUNT,
                     const QString &suffix = QString(),
                     bool skipComments = true);

    int startToken() const;

    int startPosition() const;

    QString text() const;
    QString mid(int index) const;

    QString text(int index) const;
    QStringView textRef(int index) const;
    // 1-based
    Token LA(int index) const;

    // n-la token is [startToken - n]
    Token operator[](int index) const; // ### deprecate

    QString indentationString(int index) const;

    int startOfLine(int index) const;
    int startOfMatchingBrace(int index) const;
    int startOfBlock(int index) const;

    int size() const;
    int offset() const;

    static int previousBlockState(const QTextBlock &block);

private:
    const Token &fetchToken(int tokenIndex);

private:
    Tokens _tokens;
    int _offset;
    int _blocksTokenized;
    QTextBlock _block;
    SimpleLexer _tokenize;
    QString _text;
    int _maxBlockCount;
    int _startToken;
};

} // namespace CPlusPlus
