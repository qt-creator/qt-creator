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

#include "SimpleLexer.h"

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
    QStringRef textRef(int index) const;
    // 1-based
    Token LA(int index) const;

    // n-la token is [startToken - n]
    Token operator[](int index) const; // ### deprecate

    QString indentationString(int index) const;

    int startOfLine(int index) const;
    int startOfMatchingBrace(int index) const;
    int startOfBlock(int index) const;

    int size() const;

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
