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
        byteOffset = 0;
        utf16charOffset = 0;
    }
}
