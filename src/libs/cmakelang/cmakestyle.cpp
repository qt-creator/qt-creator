// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakestyle.h"

using namespace CMakeLang;

QString Style::indentationFor(int level) const
{
    if (level <= 0)
        return {};
    return indentation ? indentation(level) : QString(level * 4, u' ');
}

bool Style::namesKeyword(const QString &command, QStringView argument) const
{
    if (!isKeyword || command.isEmpty() || argument.isEmpty())
        return false;

    for (const QChar c : argument) {
        if (!c.isUpper() && !c.isDigit() && c != u'_')
            return false;
    }
    return isKeyword(command, argument.toString());
}
