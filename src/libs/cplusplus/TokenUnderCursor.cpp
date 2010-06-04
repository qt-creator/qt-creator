/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "TokenUnderCursor.h"
#include "BackwardsScanner.h"
#include <Token.h>

#include <QTextCursor>
#include <QTextBlock>
#include <climits>

using namespace CPlusPlus;

TokenUnderCursor::TokenUnderCursor()
{ }

TokenUnderCursor::~TokenUnderCursor()
{ }

SimpleToken TokenUnderCursor::operator()(const QTextCursor &cursor, QTextBlock *b)
{
    SimpleLexer tokenize;
    tokenize.setObjCEnabled(true);
    tokenize.setSkipComments(false);

    QTextBlock block = cursor.block();
    int column = cursor.position() - cursor.block().position();

    _text = block.text();
    _tokens = tokenize(_text, BackwardsScanner::previousBlockState(block));
    for (int index = _tokens.size() - 1; index != -1; --index) {
        const SimpleToken &tk = _tokens.at(index);
        if (tk.position() < column) {
            if (b)
                *b = block;
            return tk;
        }
    }

    return SimpleToken();
}
