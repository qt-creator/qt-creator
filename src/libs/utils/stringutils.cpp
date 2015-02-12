/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "stringutils.h"

#include "hostosinfo.h"

#include <utils/algorithm.h>

#include <QDir>

#include <limits.h>

namespace Utils {

QTCREATOR_UTILS_EXPORT QString settingsKey(const QString &category)
{
    QString rc(category);
    const QChar underscore = QLatin1Char('_');
    // Remove the sort category "X.Category" -> "Category"
    if (rc.size() > 2 && rc.at(0).isLetter() && rc.at(1) == QLatin1Char('.'))
        rc.remove(0, 2);
    // Replace special characters
    const int size = rc.size();
    for (int i = 0; i < size; i++) {
        const QChar c = rc.at(i);
        if (!c.isLetterOrNumber() && c != underscore)
            rc[i] = underscore;
    }
    return rc;
}

// Figure out length of common start of string ("C:\a", "c:\b"  -> "c:\"
static inline int commonPartSize(const QString &s1, const QString &s2)
{
    const int size = qMin(s1.size(), s2.size());
    for (int i = 0; i < size; i++)
        if (s1.at(i) != s2.at(i))
            return i;
    return size;
}

QTCREATOR_UTILS_EXPORT QString commonPrefix(const QStringList &strings)
{
    switch (strings.size()) {
    case 0:
        return QString();
    case 1:
        return strings.front();
    default:
        break;
    }
    // Figure out common string part: "C:\foo\bar1" "C:\foo\bar2"  -> "C:\foo\bar"
    int commonLength = INT_MAX;
    const int last = strings.size() - 1;
    for (int i = 0; i < last; i++)
        commonLength = qMin(commonLength, commonPartSize(strings.at(i), strings.at(i + 1)));
    if (!commonLength)
        return QString();
    return strings.at(0).left(commonLength);
}

QTCREATOR_UTILS_EXPORT QString commonPath(const QStringList &files)
{
    QStringList appendedSlashes = Utils::transform(files, [](const QString &file) -> QString {
        if (!file.endsWith(QLatin1Char('/')))
            return QString(file + QLatin1Char('/'));
        return file;
    });
    QString common = commonPrefix(appendedSlashes);
    // Find common directory part: "C:\foo\bar" -> "C:\foo"
    int lastSeparatorPos = common.lastIndexOf(QLatin1Char('/'));
    if (lastSeparatorPos == -1)
        lastSeparatorPos = common.lastIndexOf(QLatin1Char('\\'));
    if (lastSeparatorPos == -1)
        return QString();
    if (HostOsInfo::isAnyUnixHost() && lastSeparatorPos == 0) // Unix: "/a", "/b" -> '/'
        lastSeparatorPos = 1;
    common.truncate(lastSeparatorPos);
    return common;
}

QTCREATOR_UTILS_EXPORT QString withTildeHomePath(const QString &path)
{
    if (HostOsInfo::isWindowsHost())
        return path;

    static const QString homePath = QDir::homePath();

    QFileInfo fi(QDir::cleanPath(path));
    QString outPath = fi.absoluteFilePath();
    if (outPath.startsWith(homePath))
        outPath = QLatin1Char('~') + outPath.mid(homePath.size());
    else
        outPath = path;
    return outPath;
}

bool AbstractMacroExpander::expandNestedMacros(const QString &str, int *pos, QString *ret)
{
    QString varName;
    QChar prev;
    QChar c;

    int i = *pos;
    int strLen = str.length();
    varName.reserve(strLen - i);
    for (; i < strLen; prev = c) {
        c = str.at(i++);
        if (c == QLatin1Char('}')) {
            if (varName.isEmpty()) { // replace "%{}" with "%"
                *ret = QString(QLatin1Char('%'));
                *pos = i;
                return true;
            }
            if (resolveMacro(varName, ret)) {
                *pos = i;
                return true;
            }
            return false;
        } else if (c == QLatin1Char('{') && prev == QLatin1Char('%')) {
            if (!expandNestedMacros(str, &i, ret))
                return false;
            varName.chop(1);
            varName += ret;
        } else {
            varName += c;
        }
    }
    return false;
}

int AbstractMacroExpander::findMacro(const QString &str, int *pos, QString *ret)
{
    forever {
        int openPos = str.indexOf(QLatin1String("%{"), *pos);
        if (openPos < 0)
            return 0;
        int varPos = openPos + 2;
        if (expandNestedMacros(str, &varPos, ret)) {
            *pos = openPos;
            return varPos - openPos;
        }
        // An actual expansion may be nested into a "false" one,
        // so we continue right after the last %{.
        *pos = openPos + 2;
    }
}

QTCREATOR_UTILS_EXPORT void expandMacros(QString *str, AbstractMacroExpander *mx)
{
    QString rsts;

    for (int pos = 0; int len = mx->findMacro(*str, &pos, &rsts); ) {
        str->replace(pos, len, rsts);
        pos += rsts.length();
    }
}

QTCREATOR_UTILS_EXPORT QString expandMacros(const QString &str, AbstractMacroExpander *mx)
{
    QString ret = str;
    expandMacros(&ret, mx);
    return ret;
}

} // namespace Utils
