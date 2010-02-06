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

#ifndef QTCONTEXTKEYWORDS_H
#define QTCONTEXTKEYWORDS_H

#include "CPlusPlusForwardDeclarations.h"

namespace CPlusPlus {

enum {
    Token_not_Qt_context_keyword = 0,
    Token_READ,
    Token_USER,
    Token_FINAL,
    Token_RESET,
    Token_WRITE,
    Token_NOTIFY,
    Token_STORED,
    Token_CONSTANT,
    Token_DESIGNABLE,
    Token_SCRIPTABLE
};

CPLUSPLUS_EXPORT int classifyQtContextKeyword(const char *s, int n);
} // namespace CPlusPlus;

#endif // QTCONTEXTKEYWORDS_H
