// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "rmkeysoperation.h"

#include <iostream>

#ifdef WITH_TESTS
#include <QTest>
#endif

#include <QLoggingCategory>
#include <QRegularExpression>

Q_LOGGING_CATEGORY(rmkeyslog, "qtc.sdktool.operations.rmkeys", QtWarningMsg)

QString RmKeysOperation::name() const
{
    return QLatin1String("rmKeys");
}

QString RmKeysOperation::helpText() const
{
    return QLatin1String("remove settings from configuration");
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
        qCCritical(rmkeyslog) << "No file given.";
    if (m_keys.isEmpty())
        qCCritical(rmkeyslog) << "No keys given.";

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
void RmKeysOperation::unittest()
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

    QVERIFY(result == testMap);

    data.append(QLatin1String("testint"));
    result = rmKeys(testMap, data);

    QCOMPARE(result.count(), 2);
    QVERIFY(result.contains(QLatin1String("subkeys")));
    QVERIFY(result.contains(QLatin1String("subkeys2")));

    cur = result.value(QLatin1String("subkeys")).toMap();
    QCOMPARE(cur.count(), 2);
    QVERIFY(cur.contains(QLatin1String("subsubkeys")));
    QVERIFY(cur.contains(QLatin1String("testbool")));

    cur = cur.value(QLatin1String("subsubkeys")).toMap();
    QCOMPARE(cur.count(), 1);
    QVERIFY(cur.contains(QLatin1String("testint2")));

    cur = result.value(QLatin1String("subkeys2")).toMap();
    QVERIFY(cur.isEmpty());

    data.clear();
    data.append(QLatin1String("subkeys/subsubkeys"));
    result = rmKeys(testMap, data);

    QCOMPARE(result.count(), 3);
    QVERIFY(result.contains(QLatin1String("subkeys")));
    QVERIFY(result.contains(QLatin1String("subkeys2")));
    QVERIFY(result.contains(QLatin1String("testint")));

    cur = result.value(QLatin1String("subkeys")).toMap();
    QCOMPARE(cur.count(), 1);
    QVERIFY(cur.contains(QLatin1String("testbool")));

    cur = result.value(QLatin1String("subkeys2")).toMap();
    QVERIFY(cur.isEmpty());

    data.clear();
    data.append(QLatin1String("subkeys/testbool"));
    result = rmKeys(testMap, data);

    QCOMPARE(result.count(), 3);
    QVERIFY(result.contains(QLatin1String("subkeys")));
    QVERIFY(result.contains(QLatin1String("subkeys2")));
    QVERIFY(result.contains(QLatin1String("testint")));

    cur = result.value(QLatin1String("subkeys")).toMap();
    QCOMPARE(cur.count(), 1);
    QVERIFY(cur.contains(QLatin1String("subsubkeys")));

    cur = cur.value(QLatin1String("subsubkeys")).toMap();
    QCOMPARE(cur.count(), 1);
    QVERIFY(cur.contains(QLatin1String("testint2")));

    cur = result.value(QLatin1String("subkeys2")).toMap();
    QVERIFY(cur.isEmpty());

    cur = result.value(QLatin1String("subkeys2")).toMap();
    QVERIFY(cur.isEmpty());

    // Test removing of non-existent key ...
    testMap = rmKeys(testMap, data);

    QTest::ignoreMessage(QtWarningMsg,
                         QRegularExpression("Key .* not found."));
    testMap = rmKeys(testMap, data);
}
#endif

QVariantMap RmKeysOperation::rmKeys(const QVariantMap &map, const QStringList &removals)
{
    QVariantMap result = map;

    for (const QString &r : removals) {
        QList<QVariantMap> stack;

        const QStringList keys = r.split(QLatin1Char('/'));

        // Set up a stack of QVariantMaps along the path we take:
        stack.append(result);
        for (int i = 0; i < keys.count() - 1; ++i) {
            QVariantMap subMap;
            if (stack.last().contains(keys.at(i))) {
                subMap = stack.last().value(keys.at(i)).toMap();
            } else {
                qCWarning(rmkeyslog) << "Key" << qPrintable(r) << "not found.";
                continue;
            }
            stack.append(subMap);
        }

        // remove
        Q_ASSERT(stack.count() == keys.count());
        if (!stack.last().contains(keys.last())) {
            qCWarning(rmkeyslog) << "Key" << qPrintable(r) << "not found.";
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

        Q_ASSERT(stack.isEmpty());
        Q_ASSERT(foldBack != map);

        result = foldBack;
    }

    return result;
}
