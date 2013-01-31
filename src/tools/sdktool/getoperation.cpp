/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "getoperation.h"

#include <iostream>

QString GetOperation::name() const
{
    return QLatin1String("get");
}

QString GetOperation::helpText() const
{
    return QLatin1String("get settings from Qt Creator configuration");
}

QString GetOperation::argumentsHelpText() const
{
    return QLatin1String("A file (profiles, qtversions or toolchains) followed by one or more keys to list.\n");
}

bool GetOperation::setArguments(const QStringList &args)
{
    if (args.count() < 2)
        return false;

    m_keys = args;
    m_file = m_keys.takeFirst();
    return true;
}

int GetOperation::execute() const
{
    Q_ASSERT(!m_keys.isEmpty());
    QVariantMap map = load(m_file);

    foreach (const QString &key, m_keys) {
        const QVariant result = get(map, key);
        if (result.isValid())
            return -2;
        std::cout << qPrintable(result.toString()) << std::endl;
    }

    return 0;
}

#ifdef WITH_TESTS
bool GetOperation::test() const
{
    QVariantMap testMap;
    QVariantMap subKeys;
    QVariantMap cur;
    cur.insert(QLatin1String("testint2"), 53);
    subKeys.insert(QLatin1String("subsubkeys"), cur);
    subKeys.insert(QLatin1String("testbool"), true);
    testMap.insert(QLatin1String("subkeys"), subKeys);
    subKeys.clear();
    testMap.insert(QLatin1String("subkeys2"), subKeys);
    testMap.insert(QLatin1String("testint"), 23);

    QVariant result;
    result = get(testMap, QLatin1String("testint"));
    if (result.toString() != QLatin1String("23"))
        return false;

    result = get(testMap, QLatin1String("subkeys/testbool"));
    if (result.toString() != QLatin1String("true"))
        return false;

    result = get(testMap, QLatin1String("subkeys/subsubkeys"));
    if (result.type() != QVariant::Map)
        return false;

    result = get(testMap, QLatin1String("nonexistant"));
    if (result.isValid())
        return false;

    return true;
}
#endif

QVariant GetOperation::get(const QVariantMap &map, const QString &key)
{
    if (key.isEmpty())
        return QString();

    const QStringList keys = key.split(QLatin1Char('/'));

    QVariantMap subMap = map;
    for (int i = 0; i < keys.count() - 1; ++i) {
        const QString k = keys.at(i);
        if (!subMap.contains(k))
            return QString();
        subMap = subMap.value(k).toMap();
    }

    return subMap.value(keys.last());
}
