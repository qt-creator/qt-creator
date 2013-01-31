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

#include "findkeyoperation.h"

#include <iostream>

QString FindKeyOperation::name() const
{
    return QLatin1String("findKey");
}

QString FindKeyOperation::helpText() const
{
    return QLatin1String("find a key in the settings of Qt Creator");
}

QString FindKeyOperation::argumentsHelpText() const
{
    return QLatin1String("A file (profiles, qtversions or toolchains) followed by one or more keys to search for.\n");
}

bool FindKeyOperation::setArguments(const QStringList &args)
{
    for (int i = 0; i < args.count(); ++i) {
        const QString current = args.at(i);

        if (m_file.isNull()) {
            m_file = current;
            continue;
        }

        m_keys.append(current);
    }

    return (!m_file.isEmpty() && !m_keys.isEmpty());
}

int FindKeyOperation::execute() const
{
    Q_ASSERT(!m_keys.isEmpty());
    QVariantMap map = load(m_file);

    foreach (const QString &k, m_keys) {
        const QStringList result = findKey(map, k);
        foreach (const QString r, result)
            std::cout << qPrintable(r) << std::endl;
    }

    return 0;
}

#ifdef WITH_TESTS
bool FindKeyOperation::test() const
{
    QVariantMap testMap;
    QVariantMap subKeys;
    QVariantMap cur;
    cur.insert(QLatin1String("testint"), 53);
    subKeys.insert(QLatin1String("subsubkeys"), cur);
    subKeys.insert(QLatin1String("testbool"), true);
    testMap.insert(QLatin1String("subkeys"), subKeys);
    subKeys.clear();
    testMap.insert(QLatin1String("subkeys2"), subKeys);
    testMap.insert(QLatin1String("testint"), 23);
    testMap.insert(QLatin1String("testbool"), true);

    QStringList result;
    result = findKey(testMap, QLatin1String("missing"));
    if (!result.isEmpty())
        return false;

    result = findKey(testMap, QLatin1String("testint"));
    if (result.count() != 2
            || !result.contains(QLatin1String("testint"))
            || !result.contains(QLatin1String("subkeys/subsubkeys/testint")))
        return false;

    result = findKey(testMap, QLatin1String("testbool"));
    if (result.count() != 2
            || !result.contains(QLatin1String("testbool")))
        return false;

    return true;
}
#endif

QStringList FindKeyOperation::findKey(const QVariantMap &map, const QString &key)
{
    QStringList result;
    for (QVariantMap::const_iterator i = map.begin(); i != map.end(); ++i) {
        if (i.key() == key) {
            result << key;
            continue;
        }
        if (i.value().type() == QVariant::Map) {
            QStringList subKeyList = findKey(i.value().toMap(), key);
            foreach (const QString &subKey, subKeyList)
                result << i.key() + QLatin1Char('/') + subKey;
        }
    }
    return result;
}
