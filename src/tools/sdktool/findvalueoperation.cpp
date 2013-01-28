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

#include "findvalueoperation.h"

#include <iostream>

QString FindValueOperation::name() const
{
    return QLatin1String("find");
}

QString FindValueOperation::helpText() const
{
    return QLatin1String("find a value in the settings of Qt Creator");
}

QString FindValueOperation::argumentsHelpText() const
{
    return QLatin1String("A file (profiles, qtversions or toolchains) followed by one or more type:value tupels to search for.\n");
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
        if (!v.isValid())
            return false;
        m_values << v;
    }

    return (!m_file.isEmpty() && !m_values.isEmpty());
}

int FindValueOperation::execute() const
{
    Q_ASSERT(!m_values.isEmpty());
    QVariantMap map = load(m_file);

    foreach (const QVariant &v, m_values) {
        const QStringList result = findValues(map, v);
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
    subKeys.insert(QLatin1String("otherint"), 53);
    testMap.insert(QLatin1String("subkeys"), subKeys);
    subKeys.clear();
    testMap.insert(QLatin1String("subkeys2"), subKeys);
    testMap.insert(QLatin1String("testint"), 23);

    QStringList result;
    result = findValues(testMap, QVariant(23));
    if (result.count() != 1
            || !result.contains(QLatin1String("testint")))
        return false;

    result = findValues(testMap, QVariant(53));
    if (result.count() != 2
            || !result.contains(QLatin1String("subkeys/subsubkeys/testint2"))
            || !result.contains(QLatin1String("subkeys/otherint")))
        return false;

    result = findValues(testMap, QVariant(23456));
    if (!result.isEmpty())
        return false;

    return true;
}
#endif

QStringList FindValueOperation::findValues(const QVariantMap &map, const QVariant &value)
{
    QStringList result;
    if (!value.isValid())
        return result;

    for (QVariantMap::const_iterator i = map.begin(); i != map.end(); ++i) {
        if (i.value() == value)
            result << i.key();
        if (i.value().type() == QVariant::Map) {
            const QStringList subKeys = findValues(i.value().toMap(), value);
            foreach (const QString &subKey, subKeys)
                result << i.key() + QLatin1Char('/') + subKey;
        }
    }

    return result;
}
