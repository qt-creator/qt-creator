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

#include <cstdio>
#include <cstring>
#include <locale>

#include <qglobal.h>

#include "binpatch.h"

#ifdef Q_OS_WIN
#	define strcasecmp _stricmp
#	define strncasecmp _strnicmp
#endif

// returns positive value if it finds a null termination inside the buffer
long BinPatch::getBufferStringLength(char *data, char *end)
{
    long size = 0;
    while (data < end) {
        if (*data == '\0')
            return size;
        ++data;
        ++size;
    }

    return -1;
}

// returns true if data ends with one of the tokens. (Sep. with ;)
bool BinPatch::endsWithTokens(const char *data)
{
    if(strlen(endTokens) > 0) {
        char endstmp[1024];
        ulong tlen = ulong(strlen(data));

        if(strlen(endTokens) >= sizeof(endstmp))
            return false;

        strcpy(endstmp, endTokens);

        char *token = strtok(endstmp, ";");

        while(token != NULL) {
            // check if it ends with the token
            if ((tlen >= strlen(token)) 
                && (strcasecmp((data+tlen)-strlen(token), token) == 0))
                return true;
            token = strtok(NULL, ";");
        }
    } else {
        return true; //true if no tokens
    }

    return false; //no matching tokens
}

bool BinPatch::patchHelper(char *inbuffer, const char *oldstr, const char *newstr, size_t len, long *rw)
{
    char nc1 = *oldstr;
    char nc2 = 0;
    char *inend = inbuffer + len;
    size_t oldlen = strlen(oldstr);
    size_t newlen = strlen(newstr);
    char *instart = inbuffer;
    *rw = 0;
    bool write = true;

    isupper(nc1) ? nc2 = tolower(nc1) : nc2 = toupper(nc1);

    while(inbuffer < inend) {
        if ((*inbuffer == nc1) || (*inbuffer == nc2)) {
            if (inbuffer > (inend-oldlen) || inbuffer > (inend-newlen)) {
                *rw = (long)(inend-inbuffer); //rewind, not enough to make a compare
                break;
            }

            if (strncasecmp(inbuffer, oldstr, oldlen) == 0) {
                if (useLength && (instart == inbuffer)) {
                    *rw = (long)(len+1); //we don't have access to the length byte, rewind all + 1!
                    write = false;
                    break;
                }

                long oldsize = -1;
                if (useLength) { //VC60
                    oldsize = (unsigned char)(*(inbuffer-1));

                    // vc60 pdb files sometimes uses 0A, then it should be null terminated
                    if (oldsize < (long)oldlen) {
                        if (oldsize != 0x0A) { //strange case... skip
                            inbuffer+=oldlen;
                            continue;
                        }

                        oldsize = getBufferStringLength(inbuffer, inend);

                        if (oldsize < 0) {
                            *rw = (long)(inend-inbuffer); //rewind, entire string not in buffer
                            break;
                        }
                    }

                    if (inbuffer > (inend-oldsize)) {
                        *rw = (long)(inend-inbuffer); //rewind, entire string not in buffer
                        break;
                    }
                } else { //VC7x
                    oldsize = getBufferStringLength(inbuffer, inend);
                    if (oldsize < 0) {
                        *rw = (long)(inend-inbuffer); //rewind, entire string not in buffer
                        break;
                    }
                }

                char oldPath[1024];
                if (oldsize > (long)sizeof(oldPath)) {
                    //at least don't crash
                    inbuffer+=oldsize;
                    continue;
                }
                memset(oldPath, '\0', sizeof(oldPath));
                strncpy(oldPath, newstr, newlen);

                if (insertReplace)
                    strncat(oldPath, inbuffer+oldlen, oldsize-oldlen);

                // just replace if it ends with a token in endTokens
                if (endsWithTokens(oldPath)) {
                    if (oldsize < (long)strlen(oldPath))
                        oldsize = (long)strlen(oldPath);

                    memcpy(inbuffer, oldPath, oldsize);
                }

                inbuffer+=oldsize;
                continue;
            }
        }
        ++inbuffer;
    }

    return write;
}

bool BinPatch::patch(const char *oldstr, const char *newstr)
{
    size_t oldlen = strlen(oldstr);
    size_t newlen = strlen(newstr);

    if ((!fileName || strlen(fileName) < 1)
        || (!oldstr || oldlen < 1)
        || (!newstr || newlen < 1))
        return false;

    FILE *input;

    if (!(input = fopen(fileName, "r+b")))
    {
        fprintf(stderr, "Warning: Could not open file %s\n", fileName);
        return false;
    }

    char data[60000];
    long rw = 0;
    long offset = 0;

    size_t len;
    len = fread(data, sizeof(char), sizeof(data), input);

    do {
        if (patchHelper(data, oldstr, newstr, len, &rw)) {
            fseek(input, offset, SEEK_SET); //overwrite
            fwrite(data, sizeof(char), len, input);
        }

        offset += (long)((-rw) + len);
        if (fseek(input, offset, SEEK_SET) != 0)
            break;
        len = fread(data, sizeof(char), sizeof(data), input);
    } while(!(feof(input) && (len <= oldlen || len <= newlen)));

    fclose(input);

    return true;
}
