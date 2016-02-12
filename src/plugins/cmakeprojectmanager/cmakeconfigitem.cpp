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

#include "cmakeconfigitem.h"

#include <utils/qtcassert.h>

namespace CMakeProjectManager {

// --------------------------------------------------------------------
// CMakeConfigItem:
// --------------------------------------------------------------------

CMakeConfigItem::CMakeConfigItem() = default;

CMakeConfigItem::CMakeConfigItem(const CMakeConfigItem &other) :
    key(other.key), type(other.type), isAdvanced(other.isAdvanced),
    value(other.value), documentation(other.documentation)
{ }

CMakeConfigItem::CMakeConfigItem(const QByteArray &k, CMakeConfigItem::Type &t,
                                 const QByteArray &d, const QByteArray &v) :
    key(k), type(t), value(v), documentation(d)
{ }

CMakeConfigItem::CMakeConfigItem(const QByteArray &k, const QByteArray &v) :
    key(k), value(v)
{ }

std::function<bool (const CMakeConfigItem &a, const CMakeConfigItem &b)> CMakeConfigItem::sortOperator()
{
    return [](const CMakeConfigItem &a, const CMakeConfigItem &b) { return a.key < b.key; };
}

CMakeConfigItem CMakeConfigItem::fromString(const QString &s)
{
    // Strip comments:
    int commentStart = s.count();
    int pos = s.indexOf(QLatin1Char('#'));
    if (pos >= 0)
        commentStart = pos;
    pos = s.indexOf(QLatin1String("//"));
    if (pos >= 0 && pos < commentStart)
        commentStart = pos;

    const QString line = s.mid(0, commentStart);

    // Split up line:
    int firstPos = -1;
    int colonPos = -1;
    int equalPos = -1;
    for (int i = 0; i < line.count(); ++i) {
        const QChar c = s.at(i);
        if (firstPos < 0 && !c.isSpace()) {
            firstPos = i;
        }
        if (c == QLatin1Char(':')) {
            if (colonPos > 0)
                break;
            colonPos = i;
            continue;
        }
        if (c == QLatin1Char('=')) {
            equalPos = i;
            break;
        }
    }

    QString key;
    QString type;
    QString value;
    if (equalPos >= 0) {
        key = line.mid(firstPos, ((colonPos >= 0) ? colonPos : equalPos) - firstPos);
        type = (colonPos < 0) ? QString() : line.mid(colonPos + 1, equalPos - colonPos - 1);
        value = line.mid(equalPos + 1);
    }

    // Fill in item:
    CMakeConfigItem item;
    if (!key.isEmpty()) {
        CMakeConfigItem::Type t = CMakeConfigItem::STRING;
        if (type == QLatin1String("FILEPATH"))
            t = CMakeConfigItem::FILEPATH;
        else if (type == QLatin1String("PATH"))
            t = CMakeConfigItem::PATH;
        else if (type == QLatin1String("BOOL"))
            t = CMakeConfigItem::BOOL;
        else if (type == QLatin1String("INTERNAL"))
            t = CMakeConfigItem::INTERNAL;

        item.key = key.toUtf8();
        item.type = t;
        item.value = value.toUtf8();
    }
    return item;
}

QString CMakeConfigItem::toString() const
{
    if (key.isEmpty())
        return QString();

    QString typeStr;
    switch (type)
    {
    case CMakeProjectManager::CMakeConfigItem::FILEPATH:
        typeStr = QLatin1String("FILEPATH");
        break;
    case CMakeProjectManager::CMakeConfigItem::PATH:
        typeStr = QLatin1String("PATH");
        break;
    case CMakeProjectManager::CMakeConfigItem::BOOL:
        typeStr = QLatin1String("BOOL");
        break;
    case CMakeProjectManager::CMakeConfigItem::INTERNAL:
        typeStr = QLatin1String("INTERNAL");
        break;
    case CMakeProjectManager::CMakeConfigItem::STRING:
    default:
        typeStr = QLatin1String("STRING");
        break;
    }

    return QString::fromUtf8(key) + QLatin1Char(':') + typeStr + QLatin1Char('=') + QString::fromUtf8(value);
}

} // namespace CMakeProjectManager
