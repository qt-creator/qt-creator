// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "stringinputstream.h"

namespace Debugger::Internal {

StringInputStream::StringInputStream(QString &str)
    : m_target(str)
{
}

void StringInputStream::appendSeparator(char c)
{
    if (!m_target.isEmpty() && !m_target.endsWith(c))
        m_target.append(c);
}

void hexPrefixOn(StringInputStream &bs)
{
    bs.setHexPrefix(true);
}

void hexPrefixOff(StringInputStream &bs)
{
    bs.setHexPrefix(false);
}

void hex(StringInputStream &bs)
{
    bs.setIntegerBase(16);
}

void dec(StringInputStream &bs)
{
    bs.setIntegerBase(10);
}

void blankSeparator(StringInputStream &bs)
{
    bs.appendSeparator();
}

} // namespace Debugger::Internal
