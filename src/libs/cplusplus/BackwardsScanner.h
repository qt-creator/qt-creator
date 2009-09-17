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
#ifndef BACKWARDSSCANNER_H
#define BACKWARDSSCANNER_H

#include "SimpleLexer.h"

#include <QtGui/QTextCursor>
#include <QtGui/QTextBlock>

namespace CPlusPlus {

class CPLUSPLUS_EXPORT BackwardsScanner
{
    enum { MAX_BLOCK_COUNT = 10 };

public:
    BackwardsScanner(const QTextCursor &cursor,
                     const QString &suffix = QString(),
                     int maxBlockCount = MAX_BLOCK_COUNT);

    int state() const;
    int startToken() const;

    int startPosition() const;

    QString text() const;
    QString text(int begin, int end) const;
    QStringRef textRef(int begin, int end) const;

    const SimpleToken &operator[](int i) const;

    int startOfMatchingBrace(int index) const;
    int previousBlockState(const QTextBlock &block) const;

private:
    const SimpleToken &fetchToken(int i);
    const QList<SimpleToken> &tokens() const;

private:
    QList<SimpleToken> _tokens;
    int _offset;
    int _blocksTokenized;
    QTextBlock _block;
    QString _text;
    SimpleLexer _tokenize;
    int _maxBlockCount;
};

} // end of namespace CPlusPlus

#endif // BACKWARDSSCANNER_H
