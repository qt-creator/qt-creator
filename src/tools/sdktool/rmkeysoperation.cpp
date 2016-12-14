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

#include "rmkeysoperation.h"

#include <iostream>

QString RmKeysOperation::name() const
{
    return QLatin1String("rmKeys");
}

QString RmKeysOperation::helpText() const
{
    return QLatin1String("remove settings from Qt Creator configuration");
}

QString RmKeysOperation::argumentsHelpText() const
{
    return QLatin1String("A file (relative to top-level settings directory and without .xml extension)\n"
                         "followed by one or more keys to remove.\n");
}

bool RmKeysOperation::setArguments(const QStringList &args)
{
    if (args.count() < 2)
        return false;

    m_keys = args;
    m_file = m_keys.takeFirst();

    if (m_file.isEmpty())
        std::cerr << "No file given." << std::endl << std::endl;
    if (m_keys.isEmpty())
        std::cerr << "No keys given." << std::endl << std::endl;

    return !m_file.isEmpty() && !m_keys.isEmpty();
}

int RmKeysOperation::execute() const
{
    Q_ASSERT(!m_keys.isEmpty());
    QVariantMap map = load(m_file);

    QVariantMap result = rmKeys(map, m_keys);
    if (map == result)
        return 1;

    // Write data again:
    return save(result, m_file) ? 0 : 2;
}

#ifdef WITH_TESTS
bool RmKeysOperation::test() const
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

    QStringList data;

    QVariantMap result = rmKeys(testMap, data);

    if (result != testMap)
        return false;

    data.append(QLatin1String("testint"));
    result = rmKeys(testMap, data);

    if (result.count() != 2
            || !result.contains(QLatin1String("subkeys"))
            || !result.contains(QLatin1String("subkeys2")))
        return false;
    cur = result.value(QLatin1String("subkeys")).toMap();
    if (cur.count() != 2
            || !cur.contains(QLatin1String("subsubkeys"))
            || !cur.contains(QLatin1String("testbool")))
        return false;

    cur = cur.value(QLatin1String("subsubkeys")).toMap();
    if (cur.count() != 1
            || !cur.contains(QLatin1String("testint2")))
        return false;

    cur = result.value(QLatin1String("subkeys2")).toMap();
    if (cur.count() != 0)
        return false;

    data.clear();
    data.append(QLatin1String("subkeys/subsubkeys"));
    result = rmKeys(testMap, data);

    if (result.count() != 3
            || !result.contains(QLatin1String("subkeys"))
            || !result.contains(QLatin1String("subkeys2"))
            || !result.contains(QLatin1String("testint")))
        return false;
    cur = result.value(QLatin1String("subkeys")).toMap();
    if (cur.count() != 1
            || !cur.contains(QLatin1String("testbool")))
        return false;

    cur = result.value(QLatin1String("subkeys2")).toMap();
    if (cur.count() != 0)
        return false;

    data.clear();
    data.append(QLatin1String("subkeys/testbool"));
    result = rmKeys(testMap, data);

    if (result.count() != 3
            || !result.contains(QLatin1String("subkeys"))
            || !result.contains(QLatin1String("subkeys2"))
            || !result.contains(QLatin1String("testint")))
        return false;
    cur = result.value(QLatin1String("subkeys")).toMap();
    if (cur.count() != 1
            || !cur.contains(QLatin1String("subsubkeys")))
        return false;

    cur = cur.value(QLatin1String("subsubkeys")).toMap();
    if (cur.count() != 1
            || !cur.contains(QLatin1String("testint2")))
        return false;

    cur = result.value(QLatin1String("subkeys2")).toMap();
    if (cur.count() != 0)
        return false;

    cur = result.value(QLatin1String("subkeys2")).toMap();
    if (cur.count() != 0)
        return false;

    return true;
}
#endif

QVariantMap RmKeysOperation::rmKeys(const QVariantMap &map, const QStringList &removals)
{
    QVariantMap result = map;

    foreach (const QString &r, removals) {
        QList<QVariantMap> stack;

        const QStringList keys = r.split(QLatin1Char('/'));

        // Set up a stack of QVariantMaps along the path we take:
        stack.append(result);
        for (int i = 0; i < keys.count() - 1; ++i) {
            QVariantMap subMap;
            if (stack.last().contains(keys.at(i))) {
                subMap = stack.last().value(keys.at(i)).toMap();
            } else {
                std::cerr << "Warning: Key " << qPrintable(r) << " not found." << std::endl;
                continue;
            }
            stack.append(subMap);
        }

        // remove
        Q_ASSERT(stack.count() == keys.count());
        if (!stack.last().contains(keys.last())) {
            std::cerr << "Warning: Key " << qPrintable(r) << " not found." << std::endl;
            continue;
        }
        stack.last().remove(keys.last());

        // Generate new resultset by folding maps back in:
        QVariantMap foldBack = stack.takeLast();
        for (int i = keys.count() - 2; i >= 0; --i) { // skip last key, that is already taken care of
            const QString k = keys.at(i);
            QVariantMap current = stack.takeLast();
            current.insert(k, foldBack);
            foldBack = current;
        }

        Q_ASSERT(stack.count() == 0);
        Q_ASSERT(foldBack != map);

        result = foldBack;
    }

    return result;
}
