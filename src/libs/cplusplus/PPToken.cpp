// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "PPToken.h"

#include <cstring>

using namespace CPlusPlus;
using namespace CPlusPlus::Internal;

bool ByteArrayRef::startsWith(const char *s) const
{
    const int l = int(std::strlen(s));
    if (l > m_length)
        return false;
    return !qstrncmp(start(), s, l);
}

int ByteArrayRef::count(char ch) const
{
    int num = 0;
    const char *b = start();
    const char *i = b + m_length;
    while (i != b)
        if (*--i == ch)
            ++num;
    return num;
}

void PPToken::squeezeSource()
{
    if (hasSource()) {
        m_src = m_src.mid(byteOffset, f.bytes);
        m_src.squeeze();
        m_originalOffset = byteOffset;
        byteOffset = 0;
        utf16charOffset = 0;
    }
}
