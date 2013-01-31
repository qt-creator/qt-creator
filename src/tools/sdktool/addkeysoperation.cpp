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

#include "addkeysoperation.h"

#include <iostream>

QString AddKeysOperation::name() const
{
    return QLatin1String("addKeys");
}

QString AddKeysOperation::helpText() const
{
    return QLatin1String("add settings to Qt Creator configuration");
}

QString AddKeysOperation::argumentsHelpText() const
{
    return QLatin1String("A file (profiles, qtversions or toolchains) followed by one or more Tuples\n"
                         "<KEY> <TYPE>:<VALUE> are required. KEY needs to start with\n"
                         "the name of the file to insert data into (minus the .xml extension).");
}

bool AddKeysOperation::setArguments(const QStringList &args)
{
    for (int i = 0; i < args.count(); ++i) {
        const QString current = args.at(i);
        const QString next = ((i + 1) < args.count()) ? args.at(i + 1) : QString();

        if (m_file.isNull()) {
            m_file = current;
            continue;
        }

        if (next.isNull())
            return false;

        ++i;
        KeyValuePair pair(current, next);
        if (pair.key.isEmpty() || !pair.value.isValid())
            return false;
        m_data << pair;
    }

    return !m_file.isEmpty();
}

int AddKeysOperation::execute() const
{
    if (m_data.isEmpty())
        return 0;

    QVariantMap map = load(m_file);

    map = addKeys(map, m_data);
    if (map.isEmpty())
        return 1;

    // Write data again:
    return save(map, m_file) ? 0 : 2;
}

#ifdef WITH_TESTS
bool AddKeysOperation::test() const
{
    QVariantMap testMap;
    QVariantMap subKeys;
    subKeys.insert(QLatin1String("subsubkeys"), QVariantMap());
    subKeys.insert(QLatin1String("testbool"), true);
    testMap.insert(QLatin1String("subkeys"), subKeys);
    subKeys.clear();
    testMap.insert(QLatin1String("subkeys2"), subKeys);
    testMap.insert(QLatin1String("testint"), 23);

    KeyValuePairList data;
    data.append(KeyValuePair(QLatin1String("bool-true"), QString::fromLatin1("bool:trUe")));
    data.append(KeyValuePair(QLatin1String("bool-false"), QString::fromLatin1("bool:false")));
    data.append(KeyValuePair(QLatin1String("int"), QString::fromLatin1("int:42")));
    data.append(KeyValuePair(QLatin1String("qstring"), QString::fromLatin1("QString:test string.")));
    data.append(KeyValuePair(QLatin1String("qbytearray"), QString::fromLatin1("QByteArray:test array.")));

    data.append(KeyValuePair(QLatin1String("subkeys/qbytearray"), QString::fromLatin1("QByteArray:test array.")));
    data.append(KeyValuePair(QLatin1String("subkeys/newsubkeys/qbytearray"), QString::fromLatin1("QByteArray:test array.")));
    data.append(KeyValuePair(QLatin1String("newsub/1/2/3/qbytearray"), QString::fromLatin1("QByteArray:test array.")));
    data.append(KeyValuePair(QLatin1String("newsub/1/2.1/3/qbytearray"), QString::fromLatin1("QByteArray:test array.")));

    QVariantMap result = addKeys(testMap, data);
    if (result.count() != 9)
        return false;

    // subkeys:
    QVariantMap cur = result.value(QLatin1String("subkeys")).toMap();
    if (cur.count() != 4
            || !cur.contains(QLatin1String("qbytearray"))
            || !cur.contains(QLatin1String("testbool"))
            || !cur.contains(QLatin1String("subsubkeys"))
            || !cur.contains(QLatin1String("newsubkeys")))
        return false;

    // subkeys/newsubkeys:
    QVariantMap tmp = cur;
    cur = cur.value(QLatin1String("newsubkeys")).toMap();
    if (cur.count() != 1
            || !cur.contains(QLatin1String("qbytearray")))
        return false;

    // subkeys/subsubkeys:
    cur = tmp.value(QLatin1String("subsubkeys")).toMap();
    if (cur.count() != 0)
        return false;

    // newsub:
    cur = result.value(QLatin1String("newsub")).toMap();
    if (cur.count() != 1
            || !cur.contains(QLatin1String("1")))
        return false;

    // newsub/1:
    cur = cur.value(QLatin1String("1")).toMap();
    if (cur.count() != 2
            || !cur.contains(QLatin1String("2"))
            || !cur.contains(QLatin1String("2.1")))
        return false;

    // newsub/1/2:
    tmp = cur;
    cur = cur.value(QLatin1String("2")).toMap();
    if (cur.count() != 1
            || !cur.contains(QLatin1String("3")))
        return false;

    // newsub/1/2/3:
    cur = cur.value(QLatin1String("3")).toMap();
    if (cur.count() != 1
            || !cur.contains(QLatin1String("qbytearray")))
        return false;

    // newsub/1/2.1:
    cur = tmp.value(QLatin1String("2.1")).toMap();
    if (cur.count() != 1
            || !cur.contains(QLatin1String("3")))
        return false;

    // newsub/1/2.1/3:
    cur = cur.value(QLatin1String("3")).toMap();
    if (cur.count() != 1
            || !cur.contains(QLatin1String("qbytearray")))
        return false;

    // subkeys2:
    cur = result.value(QLatin1String("subkeys2")).toMap();
    if (cur.count() != 0)
        return false;

    // values:
    if (!result.contains(QLatin1String("bool-true")) || !result.value(QLatin1String("bool-true")).toBool())
        return false;
    if (!result.contains(QLatin1String("bool-false")) || result.value(QLatin1String("bool-false")).toBool())
        return false;
    if (!result.contains(QLatin1String("int")) || result.value(QLatin1String("int")).toInt() != 42)
        return false;
    if (!result.contains(QLatin1String("qstring"))
            || result.value(QLatin1String("qstring")).toString() != QLatin1String("test string."))
        return false;
    if (!result.contains(QLatin1String("qbytearray"))
            || result.value(QLatin1String("qbytearray")).toByteArray() != "test array.")
        return false;

    // Make sure we do not overwrite data:
    // preexisting:
    data.clear();
    data.append(KeyValuePair(QLatin1String("testint"), QString::fromLatin1("int:4")));
    result = addKeys(testMap, data);
    if (!result.isEmpty())
        return false;

    data.clear();
    data.append(KeyValuePair(QLatin1String("subkeys/testbool"), QString::fromLatin1("int:24")));
    result = addKeys(testMap, data);
    if (!result.isEmpty())
        return false;

    // data inserted before:
    data.clear();
    data.append(KeyValuePair(QLatin1String("bool-true"), QString::fromLatin1("bool:trUe")));
    data.append(KeyValuePair(QLatin1String("bool-true"), QString::fromLatin1("bool:trUe")));
    result = addKeys(testMap, data);
    if (!result.isEmpty())
        return false;

    return true;
}
#endif

QVariantMap AddKeysOperation::addKeys(const QVariantMap &map, const KeyValuePairList &additions)
{
    // Insert data:
    QVariantMap result = map;

    foreach (const KeyValuePair &p, additions) {
        QList<QVariantMap> stack;

        // Set up a stack of QVariantMaps along the path we take:
        stack.append(result);
        for (int i = 0; i < p.key.count() - 1; ++i) {
            QVariantMap subMap;
            if (stack.last().contains(p.key.at(i)))
                subMap = stack.last().value(p.key.at(i)).toMap();
            stack.append(subMap);
        }

        // insert
        Q_ASSERT(stack.count() == p.key.count());
        if (stack.last().contains(p.key.last())) {
            std::cerr << "DEBUG: Adding key " << qPrintable(p.key.join(QLatin1String("/"))) << " which already exists." << std::endl;
            return QVariantMap();
        }
        stack.last().insert(p.key.last(), p.value);

        // Generate new resultset by folding maps back in:
        QVariantMap foldBack = stack.takeLast();
        for (int i = p.key.count() - 2; i >= 0; --i) { // skip last key, that is already taken care of
            const QString k = p.key.at(i);
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
