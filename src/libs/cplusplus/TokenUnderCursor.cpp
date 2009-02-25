/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "TokenUnderCursor.h"
#include <Token.h>

#include <QTextCursor>
#include <QTextBlock>
#include <climits>

using namespace CPlusPlus;

TokenUnderCursor::TokenUnderCursor()
{ }

TokenUnderCursor::~TokenUnderCursor()
{ }

SimpleToken TokenUnderCursor::operator()(const QTextCursor &cursor) const
{
    SimpleLexer tokenize;
    QTextBlock block = cursor.block();
    int column = cursor.columnNumber();

    QList<SimpleToken> tokens = tokenize(block.text(), previousBlockState(block));
    for (int index = tokens.size() - 1; index != -1; --index) {
        const SimpleToken &tk = tokens.at(index);
        if (tk.position() < column)
            return tk;
    }

    return SimpleToken();
}

int TokenUnderCursor::previousBlockState(const QTextBlock &block) const
{
    const QTextBlock prevBlock = block.previous();
    if (prevBlock.isValid()) {
        int state = prevBlock.userState();

        if (state != -1)
            return state;
    }
    return 0;
}
