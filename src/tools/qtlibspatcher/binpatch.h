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

#ifndef BINPATCH_H
#define BINPATCH_H

#include <string.h>

typedef unsigned long ulong;
typedef unsigned int uint;

class BinPatch
{
public:
    BinPatch(const char *file)
        : useLength(false), insertReplace(false)
    {
        strcpy(endTokens, "");
        strcpy(fileName, file);
    }

    void enableUseLength(bool enabled)
    { useLength = enabled; }
    void enableInsertReplace(bool enabled)
    { insertReplace = enabled; }
    void setEndTokens(const char *tokens)
    { strcpy(endTokens, tokens); }

    bool patch(const char *oldstr, const char *newstr);

private:
    long getBufferStringLength(char *data, char *end);
    bool endsWithTokens(const char *data);

    bool patchHelper(char *inbuffer, const char *oldstr, 
        const char *newstr, size_t len, long *rw);

    bool useLength;
    bool insertReplace;
    char endTokens[1024];
    char fileName[1024];
};

#endif
