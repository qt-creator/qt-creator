// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "findvalueoperation.h"

#include <iostream>

#ifdef WITH_TESTS
#include <QTest>
#endif

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(findvaluelog, "qtc.sdktool.operations.findvalue", QtWarningMsg)

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

        QVariant v = valueFromString(current);
        if (!v.isValid()) {
            qCCritical(findvaluelog) << "Value for key '" << qPrintable(current) << "' is not valid.";
            return false;
        }
        m_values << v;
    }

    if (m_file.isEmpty())
        qCCritical(findvaluelog) << "No file given.";
    if (m_values.isEmpty())
        qCCritical(findvaluelog) << "No values given.";

    return (!m_file.isEmpty() && !m_values.isEmpty());
}

int FindValueOperation::execute() const
{
    Q_ASSERT(!m_values.isEmpty());
    QVariantMap map = load(m_file);

    for (const QVariant &v : std::as_const(m_values)) {
        const QStringList result = findValue(map, v);
        for (const QString &r : result)
            std::cout << qPrintable(r) << std::endl;
    }

    return 0;
}

#ifdef WITH_TESTS
void FindValueOperation::unittest()
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
    QCOMPARE(result.count(), 1);
    QVERIFY(result.contains(QLatin1String("testint")));

    result = findValue(testMap, QVariant(53));
    QCOMPARE(result.count(), 2);

    QVERIFY(result.contains(QLatin1String("subkeys/subsubkeys/testint2")));
    QVERIFY(result.contains(QLatin1String("subkeys/otherint")));

    result = findValue(testMap, QVariant(23456));
    QVERIFY(result.isEmpty());

    result = findValue(testMap, QVariant(QString::fromLatin1("FindInList")));
    QCOMPARE(result.count(), 1);
    QVERIFY(result.contains(QLatin1String("aList[2][1]/findMe")));
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
