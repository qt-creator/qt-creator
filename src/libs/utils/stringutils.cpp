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

#include "stringutils.h"

#include "hostosinfo.h"

#include <utils/algorithm.h>

#include <QDir>
#include <QRegularExpression>

#include <limits.h>

namespace Utils {

QTCREATOR_UTILS_EXPORT QString settingsKey(const QString &category)
{
    QString rc(category);
    const QChar underscore = '_';
    // Remove the sort category "X.Category" -> "Category"
    if (rc.size() > 2 && rc.at(0).isLetter() && rc.at(1) == '.')
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
        if (!file.endsWith('/'))
            return QString(file + '/');
        return file;
    });
    QString common = commonPrefix(appendedSlashes);
    // Find common directory part: "C:\foo\bar" -> "C:\foo"
    int lastSeparatorPos = common.lastIndexOf('/');
    if (lastSeparatorPos == -1)
        lastSeparatorPos = common.lastIndexOf('\\');
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
        outPath = '~' + outPath.mid(homePath.size());
    else
        outPath = path;
    return outPath;
}

static bool validateVarName(const QString &varName)
{
    return !varName.startsWith("JS:");
}

bool AbstractMacroExpander::expandNestedMacros(const QString &str, int *pos, QString *ret)
{
    QString varName;
    QString pattern, replace;
    QString defaultValue;
    QString *currArg = &varName;
    QChar prev;
    QChar c;
    bool replaceAll = false;

    int i = *pos;
    int strLen = str.length();
    varName.reserve(strLen - i);
    for (; i < strLen; prev = c) {
        c = str.at(i++);
        if (c == '\\' && i < strLen && validateVarName(varName)) {
            c = str.at(i++);
            // For the replacement, do not skip the escape sequence when followed by a digit.
            // This is needed for enabling convenient capture group replacement,
            // like %{var/(.)(.)/\2\1}, without escaping the placeholders.
            if (currArg == &replace && c.isDigit())
                *currArg += '\\';
            *currArg += c;
        } else if (c == '}') {
            if (varName.isEmpty()) { // replace "%{}" with "%"
                *ret = QString('%');
                *pos = i;
                return true;
            }
            if (resolveMacro(varName, ret)) {
                *pos = i;
                if (!pattern.isEmpty() && currArg == &replace) {
                    const QRegularExpression regexp(pattern);
                    if (regexp.isValid()) {
                        if (replaceAll) {
                            ret->replace(regexp, replace);
                        } else {
                            // There isn't an API for replacing once...
                            const QRegularExpressionMatch match = regexp.match(*ret);
                            if (match.hasMatch()) {
                                *ret = ret->left(match.capturedStart(0))
                                        + match.captured(0).replace(regexp, replace)
                                        + ret->mid(match.capturedEnd(0));
                            }
                        }
                    }
                }
                return true;
            }
            if (!defaultValue.isEmpty()) {
                *pos = i;
                *ret = defaultValue;
                return true;
            }
            return false;
        } else if (c == '{' && prev == '%') {
            if (!expandNestedMacros(str, &i, ret))
                return false;
            varName.chop(1);
            varName += *ret;
        } else if (currArg == &varName && c == '-' && prev == ':' && validateVarName(varName)) {
            varName.chop(1);
            currArg = &defaultValue;
        } else if (currArg == &varName && c == '/' && validateVarName(varName)) {
            currArg = &pattern;
            if (i < strLen && str.at(i) == '/') {
                ++i;
                replaceAll = true;
            }
        } else if (currArg == &pattern && c == '/') {
            currArg = &replace;
        } else {
            *currArg += c;
        }
    }
    return false;
}

int AbstractMacroExpander::findMacro(const QString &str, int *pos, QString *ret)
{
    forever {
        int openPos = str.indexOf("%{", *pos);
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
