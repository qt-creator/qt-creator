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

#include "findkeyoperation.h"

#include <iostream>

QString FindKeyOperation::name() const
{
    return QLatin1String("findKey");
}

QString FindKeyOperation::helpText() const
{
    return QLatin1String("find a key in the settings");
}

QString FindKeyOperation::argumentsHelpText() const
{
    return QLatin1String("A file (relative to top-level settings directory and without .xml extension)\n"
                         "followed by one or more keys to search for.\n");
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

    if (m_file.isEmpty())
        std::cerr << "No file given." << std::endl << std::endl;
    if (m_keys.isEmpty())
        std::cerr << "No keys given." << std::endl << std::endl;

    return (!m_file.isEmpty() && !m_keys.isEmpty());
}

int FindKeyOperation::execute() const
{
    Q_ASSERT(!m_keys.isEmpty());
    QVariantMap map = load(m_file);

    foreach (const QString &k, m_keys) {
        const QStringList result = findKey(map, k);
        foreach (const QString &r, result)
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

    subKeys.clear();
    QVariantList list1;
    list1.append(QLatin1String("ignore this"));
    list1.append(QLatin1String("ignore this2"));
    QVariantList list2;
    list2.append(QLatin1String("somevalue"));
    subKeys.insert(QLatin1String("findMe"), QLatin1String("FindInList"));
    list2.append(subKeys);
    list2.append(QLatin1String("someothervalue"));
    list1.append(QVariant(list2));

    testMap.insert(QLatin1String("aList"), list1);

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

    result = findKey(testMap, QLatin1String("findMe"));
    if (result.count() != 1
            || !result.contains(QLatin1String("aList[2][1]/findMe")))
        return false;

    return true;
}
#endif

QStringList FindKeyOperation::findKey(const QVariant &in, const QString &key, const QString &prefix)
{
    QStringList result;
    if (in.type() == QVariant::Map) {
        const QVariantMap map = in.toMap();
        for (QVariantMap::const_iterator i = map.begin(); i != map.end(); ++i) {
            QString pfx = prefix;
            if (!pfx.isEmpty())
                pfx.append(QLatin1Char('/'));
            if (i.key() == key) {
                result << pfx + key;
            } else {
                pfx.append(i.key());
                result.append(findKey(i.value(), key, pfx));
            }
        }
    } else if (in.type() == QVariant::List) {
        QVariantList list = in.toList();
        for (int pos = 0; pos < list.count(); ++pos) {
            QString pfx = prefix + QLatin1Char('[') + QString::number(pos) + QLatin1Char(']');
            result.append(findKey(list.at(pos), key, pfx));
        }
    }
    return result;
}
