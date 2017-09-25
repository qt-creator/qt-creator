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

#include "clangfilepath.h"

#include <utf8string.h>

#include <QByteArray>

#if defined(Q_OS_WIN)
// Parameterized QDir::toNativeSeparators/-fromNativeSeparators for QByteArray
static inline QByteArray replace(const QByteArray &pathName, char toReplace, char replacement)
{
    int i = pathName.indexOf(toReplace);
    if (i != -1) {
        QByteArray n(pathName);

        char * const data = n.data();
        data[i++] = replacement;

        for (; i < n.length(); ++i) {
            if (data[i] == toReplace)
                data[i] = replacement;
        }

        return n;
    }

    return pathName;
}
#endif

static QByteArray fromNativeSeparatorsForQByteArray(const QByteArray &pathName)
{
#if defined(Q_OS_WIN)
    return replace(pathName, '\\', '/');
#else
    return pathName;
#endif
}

static QByteArray toNativeSeparatorsForQByteArray(const QByteArray &pathName)
{
#if defined(Q_OS_WIN)
    return replace(pathName, '/', '\\');
#else
    return pathName;
#endif
}

namespace ClangBackEnd {

Utf8String FilePath::fromNativeSeparators(const Utf8String &pathName)
{
    const QByteArray pathNameAsByteArray = pathName.toByteArray();

    return Utf8String::fromUtf8(fromNativeSeparatorsForQByteArray(pathNameAsByteArray));
}

Utf8String FilePath::toNativeSeparators(const Utf8String &pathName)
{
    const QByteArray pathNameAsByteArray = pathName.toByteArray();

    return Utf8String::fromUtf8(toNativeSeparatorsForQByteArray(pathNameAsByteArray));
}

} // namespace ClangBackEnd
