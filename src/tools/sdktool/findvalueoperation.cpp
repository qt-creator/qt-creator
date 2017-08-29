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

#include "findvalueoperation.h"

#include <iostream>

QString FindValueOperation::name() const
{
    return QLatin1String("find");
}

QString FindValueOperation::helpText() const
{
    return QLatin1String("find a value in the settings");
}

QString FindValueOperation::argumentsHelpText() const
{
    return QLatin1String("A file (relative to top-level settings directory and without .xml extension)\n"
                         "followed by one or more type:value tupels to search for.\n");
}

bool FindValueOperation::setArguments(const QStringList &args)
{
    for (int i = 0; i < args.count(); ++i) {
        const QString current = args.at(i);

        if (m_file.isNull()) {
            m_file = current;
            continue;
        }

        QVariant v = Operation::valueFromString(current);
        if (!v.isValid()) {
            std::cerr << "Value for key '" << qPrintable(current) << "' is not valid." << std::endl << std::endl;
            return false;
        }
        m_values << v;
    }

    if (m_file.isEmpty())
        std::cerr << "No file given." << std::endl << std::endl;
    if (m_values.isEmpty())
        std::cerr << "No values given." << std::endl << std::endl;

    return (!m_file.isEmpty() && !m_values.isEmpty());
}

int FindValueOperation::execute() const
{
    Q_ASSERT(!m_values.isEmpty());
    QVariantMap map = load(m_file);

    foreach (const QVariant &v, m_values) {
        const QStringList result = findValue(map, v);
        foreach (const QString &r, result)
            std::cout << qPrintable(r) << std::endl;
    }

    return 0;
}

#ifdef WITH_TESTS
bool FindValueOperation::test() const
{
    QVariantMap testMap;
    QVariantMap subKeys;
    QVariantMap cur;
    cur.insert(QLatin1String("testint2"), 53);
    subKeys.insert(QLatin1String("subsubkeys"), cur);
    subKeys.insert(QLatin1String("testbool"), true);
    subKeys.insert(QLatin1String("testbool2"), false);
    subKeys.insert(QLatin1String("otherint"), 53);
    testMap.insert(QLatin1String("subkeys"), subKeys);
    subKeys.clear();
    testMap.insert(QLatin1String("subkeys2"), subKeys);
    testMap.insert(QLatin1String("testint"), 23);

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
    result = findValue(testMap, QVariant(23));
    if (result.count() != 1
            || !result.contains(QLatin1String("testint")))
        return false;

    result = findValue(testMap, QVariant(53));
    if (result.count() != 2
            || !result.contains(QLatin1String("subkeys/subsubkeys/testint2"))
            || !result.contains(QLatin1String("subkeys/otherint")))
        return false;

    result = findValue(testMap, QVariant(23456));
    if (!result.isEmpty())
        return false;

    result = findValue(testMap, QVariant(QString::fromLatin1("FindInList")));
    if (result.count() != 1
            || !result.contains(QLatin1String("aList[2][1]/findMe")))
        return false;

    return true;
}
#endif

QStringList FindValueOperation::findValue(const QVariant &in, const QVariant &value,
                                           const QString &prefix)
{
    QStringList result;
    if (in.type() == value.type() && in == value) {
        result << prefix;
    } else if (in.type() == QVariant::Map) {
        const QVariantMap map = in.toMap();
        for (QVariantMap::const_iterator i = map.begin(); i != map.end(); ++i) {
            QString pfx = prefix;
            if (!pfx.isEmpty())
                pfx.append(QLatin1Char('/'));
            pfx.append(i.key());
            result.append(findValue(i.value(), value, pfx));
        }
    } else if (in.type() == QVariant::List) {
        QVariantList list = in.toList();
        for (int pos = 0; pos < list.count(); ++pos) {
            QString pfx = prefix + QLatin1Char('[') + QString::number(pos) + QLatin1Char(']');
            result.append(findValue(list.at(pos), value, pfx));
        }
    }
    return result;
}
