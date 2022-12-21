// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "findkeyoperation.h"

#include <iostream>

#ifdef WITH_TESTS
#include <QTest>
#endif

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(findkeylog, "qtc.sdktool.operations.findkey", QtWarningMsg)


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
        qCCritical(findkeylog) << "No file given.";
    if (m_keys.isEmpty())
        qCCritical(findkeylog) << "No keys given.";

    return (!m_file.isEmpty() && !m_keys.isEmpty());
}

int FindKeyOperation::execute() const
{
    Q_ASSERT(!m_keys.isEmpty());
    QVariantMap map = load(m_file);

    for (const auto &k : m_keys) {
        const QStringList result = findKey(map, k);
        for (const auto &r: result) {
            std::cout << qPrintable(r) << std::endl;
        }
    }

    return 0;
}

#ifdef WITH_TESTS
void FindKeyOperation::unittest()
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
    QVERIFY(result.isEmpty());

    result = findKey(testMap, QLatin1String("testint"));
    QCOMPARE(result.count(), 2);
    QVERIFY(result.contains(QLatin1String("testint")));
    QVERIFY(result.contains(QLatin1String("subkeys/subsubkeys/testint")));

    result = findKey(testMap, QLatin1String("testbool"));
    QCOMPARE(result.count(), 2);
    QVERIFY(result.contains(QLatin1String("testbool")));

    result = findKey(testMap, QLatin1String("findMe"));
    QCOMPARE(result.count(), 1);
    QVERIFY(result.contains(QLatin1String("aList[2][1]/findMe")));
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
