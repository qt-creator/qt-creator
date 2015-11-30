/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#ifndef CPLUSPLUS_BACKWARDSSCANNER_H
#define CPLUSPLUS_BACKWARDSSCANNER_H

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

#endif // CPLUSPLUS_BACKWARDSSCANNER_H
