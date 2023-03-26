// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "addkeysoperation.h"

#include <iostream>

#include <QLoggingCategory>
#include <QRegularExpression>

#ifdef WITH_TESTS
#include <QTest>
#endif

Q_LOGGING_CATEGORY(addkeyslog, "qtc.sdktool.operations.addkeys", QtWarningMsg)

QString AddKeysOperation::name() const
{
    return QLatin1String("addKeys");
}

QString AddKeysOperation::helpText() const
{
    return QLatin1String("add arbitrary settings to configuration");
}

QString AddKeysOperation::argumentsHelpText() const
{
    return QLatin1String(
        "A file (relative to top-level settings directory and without .xml extension)\n"
        "followed by one or more Tuples <KEY> <TYPE>:<VALUE> are required.\n");
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

        if (next.isNull()) {
            qCCritical(addkeyslog) << "Missing value for key '" << qPrintable(current) << "'.";
            return false;
        }

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

    QVariantMap result = addKeys(map);
    if (result.isEmpty() || map == result)
        return 4;

    // Write data again:
    return save(result, m_file) ? 0 : 5;
}

#ifdef WITH_TESTS
void AddKeysOperation::unittest()
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
    data.append(
        KeyValuePair(QLatin1String("qbytearray"), QString::fromLatin1("QByteArray:test array.")));

    data.append(KeyValuePair(QLatin1String("subkeys/qbytearray"),
                             QString::fromLatin1("QByteArray:test array.")));
    data.append(KeyValuePair(QLatin1String("subkeys/newsubkeys/qbytearray"),
                             QString::fromLatin1("QByteArray:test array.")));
    data.append(KeyValuePair(QLatin1String("newsub/1/2/3/qbytearray"),
                             QString::fromLatin1("QByteArray:test array.")));
    data.append(KeyValuePair(QLatin1String("newsub/1/2.1/3/qbytearray"),
                             QString::fromLatin1("QByteArray:test array.")));

    QVariantMap result = AddKeysData{data}.addKeys(testMap);
    QCOMPARE(result.count(), 9);

    // subkeys:
    QVariantMap cur = result.value(QLatin1String("subkeys")).toMap();
    QCOMPARE(cur.count(), 4);
    QVERIFY(cur.contains(QLatin1String("qbytearray")));
    QVERIFY(cur.contains(QLatin1String("testbool")));
    QVERIFY(cur.contains(QLatin1String("subsubkeys")));
    QVERIFY(cur.contains(QLatin1String("newsubkeys")));

    // subkeys/newsubkeys:
    QVariantMap tmp = cur;
    cur = cur.value(QLatin1String("newsubkeys")).toMap();
    QCOMPARE(cur.count(), 1);
    QVERIFY(cur.contains(QLatin1String("qbytearray")));

    // subkeys/subsubkeys:
    cur = tmp.value(QLatin1String("subsubkeys")).toMap();
    QVERIFY(cur.isEmpty());

    // newsub:
    cur = result.value(QLatin1String("newsub")).toMap();
    QCOMPARE(cur.count(), 1);
    QVERIFY(cur.contains(QLatin1String("1")));

    // newsub/1:
    cur = cur.value(QLatin1String("1")).toMap();
    QCOMPARE(cur.count(), 2);
    QVERIFY(cur.contains(QLatin1String("2")));
    QVERIFY(cur.contains(QLatin1String("2.1")));

    // newsub/1/2:
    tmp = cur;
    cur = cur.value(QLatin1String("2")).toMap();
    QCOMPARE(cur.count(), 1);
    QVERIFY(cur.contains(QLatin1String("3")));

    // newsub/1/2/3:
    cur = cur.value(QLatin1String("3")).toMap();
    QCOMPARE(cur.count(), 1);
    QVERIFY(cur.contains(QLatin1String("qbytearray")));

    // newsub/1/2.1:
    cur = tmp.value(QLatin1String("2.1")).toMap();
    QCOMPARE(cur.count(), 1);
    QVERIFY(cur.contains(QLatin1String("3")));

    // newsub/1/2.1/3:
    cur = cur.value(QLatin1String("3")).toMap();
    QCOMPARE(cur.count(), 1);
    QVERIFY(cur.contains(QLatin1String("qbytearray")));

    // subkeys2:
    cur = result.value(QLatin1String("subkeys2")).toMap();
    QVERIFY(cur.isEmpty());

    // values:
    QVERIFY(result.contains(QLatin1String("bool-true")));
    QVERIFY(result.value(QLatin1String("bool-true")).toBool());

    QVERIFY(result.contains(QLatin1String("bool-false")));
    QVERIFY(!result.value(QLatin1String("bool-false")).toBool());

    QVERIFY(result.contains(QLatin1String("int")));
    QCOMPARE(result.value(QLatin1String("int")).toInt(), 42);
    QVERIFY(result.contains(QLatin1String("qstring")));
    QCOMPARE(result.value(QLatin1String("qstring")).toString(), QLatin1String("test string."));
    QVERIFY(result.contains(QLatin1String("qbytearray")));
    QCOMPARE(result.value(QLatin1String("qbytearray")).toByteArray(), "test array.");

    // Make sure we do not overwrite data:
    // preexisting:
    QTest::ignoreMessage(QtCriticalMsg,
                         QRegularExpression("Cannot add key .* which already exists."));

    data.clear();
    data.append(KeyValuePair(QLatin1String("testint"), QString::fromLatin1("int:4")));
    result = AddKeysData{data}.addKeys(testMap);
    QVERIFY(result.isEmpty());

    QTest::ignoreMessage(QtCriticalMsg,
                         QRegularExpression("Cannot add key .* which already exists."));

    data.clear();
    data.append(KeyValuePair(QLatin1String("subkeys/testbool"), QString::fromLatin1("int:24")));
    result = AddKeysData{data}.addKeys(testMap);
    QVERIFY(result.isEmpty());

    QTest::ignoreMessage(QtCriticalMsg,
                         QRegularExpression("Cannot add key .* which already exists."));

    // data inserted before:
    data.clear();
    data.append(KeyValuePair(QLatin1String("bool-true"), QString::fromLatin1("bool:trUe")));
    data.append(KeyValuePair(QLatin1String("bool-true"), QString::fromLatin1("bool:trUe")));
    result = AddKeysData{data}.addKeys(testMap);
    QVERIFY(result.isEmpty());
}
#endif

QVariantMap AddKeysData::addKeys(const QVariantMap &map) const
{
    // Insert data:
    QVariantMap result = map;

    for (const KeyValuePair &p : m_data) {
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
            qCCritical(addkeyslog) << "Cannot add key" << qPrintable(p.key.join(QLatin1Char('/')))
                                   << "which already exists.";
            return QVariantMap();
        }
        stack.last().insert(p.key.last(), p.value);

        // Generate new resultset by folding maps back in:
        QVariantMap foldBack = stack.takeLast();
        for (int i = p.key.count() - 2; i >= 0;
             --i) { // skip last key, that is already taken care of
            const QString k = p.key.at(i);
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
