// Copyright (C) 2019 The Qt Company Ltd.
// Copyright (C) 2019 Christoph Schlosser, B/S/H/ <christoph.schlosser@bshg.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "addvalueoperation.h"
#include "addkeysoperation.h"
#include "getoperation.h"
#include "rmkeysoperation.h"

#include <iomanip>
#include <iostream>

#ifdef WITH_TESTS
#include <QTest>
#endif

#include <QLoggingCategory>
#include <QRegularExpression>

Q_LOGGING_CATEGORY(addvaluelog, "qtc.sdktool.operations.addvalue", QtWarningMsg)

namespace {
constexpr auto SUCCESS = 0;
constexpr auto FAILURE = !SUCCESS;
} // namespace

QString AddValueOperation::name() const
{
    return QLatin1String("addValue");
}

QString AddValueOperation::helpText() const
{
    return QLatin1String("add a value to an existing list");
}

QString AddValueOperation::argumentsHelpText() const
{
    return QLatin1String(
        "A file (relative to top-level settings directory and without .xml extension)\n"
        "followed by the key of the list followed by one or more type:values to append to the "
        "list.\n");
}

bool AddValueOperation::setArguments(const QStringList &args)
{
    if (args.size() < 3) {
        std::cerr << "Error: No";
        switch (args.size()) {
        case 0:
            std::cerr << " file,";
            Q_FALLTHROUGH();
        case 1:
            std::cerr << " key and";
            Q_FALLTHROUGH();
        case 2:
            std::cerr << " values";
            break;
        }
        std::cerr << " given.\n" << std::endl;
        return false;
    }

    auto tempArgs = args;
    m_file = tempArgs.takeFirst();
    m_key = tempArgs.takeFirst();
    for (const auto &arg : tempArgs) {
        const auto val = valueFromString(arg);
        if (!val.isValid() || val.isNull()) {
            std::cerr << "Error: " << std::quoted(arg.toStdString())
                      << " is not a valid QVariant like string Type:Value.\n"
                      << std::endl;
            return false;
        }
        m_values.append(val);
    }

    return true;
}

int AddValueOperation::execute() const
{
    auto map = load(m_file);

    if (map.empty()) {
        std::cerr << "Error: Could not load " << m_file.toStdString() << std::endl;
        return FAILURE;
    }

    bool status = appendListToMap(map);

    if (status) {
        status = save(map, m_file);
    }

    return status ? SUCCESS : FAILURE;
}

#ifdef WITH_TESTS
void AddValueOperation::unittest()
{
    QVariantList testDataList;
    testDataList.append(QLatin1String("Some String"));
    testDataList.append(int(1860));

    KeyValuePairList testKvpList;
    testKvpList.append(KeyValuePair(QLatin1String("test/foo"), QString::fromLatin1("QString:Foo")));
    testKvpList.append(KeyValuePair(QLatin1String("test/foobar"), QString::fromLatin1("int:42")));
    testKvpList.append(KeyValuePair(QLatin1String("test/bar"), testDataList));

    const QVariantList valueList = {valueFromString("QString:ELIL"), valueFromString("int:-1")};

    QVariantMap testMap;

    // add to empty map
    AddValueData d;
    d.m_key = "some key";
    d.m_values = valueList;

    QTest::ignoreMessage(QtCriticalMsg,
                         QRegularExpression("Error: Could not retrieve value for key .*."));

    QVERIFY(!d.appendListToMap(testMap));

    testMap.insert(QLatin1String("someEmptyThing"), QVariantMap());
    testMap.insert(QLatin1String("aKey"), "withAString");

    // append to a value
    d.m_key = "aKey";

    QTest::ignoreMessage(QtCriticalMsg,
                         QRegularExpression("Error: Data stored in .* is not a QVariantList."));

    QVERIFY(!d.appendListToMap(testMap));

    testMap = AddKeysData{testKvpList}.addKeys(testMap);

    // quick sanity check
    QCOMPARE(testMap.count(), 3);
    QCOMPARE(testDataList.count(), 2);
    QCOMPARE(testKvpList.count(), 3);

    // successful adding of values
    d.m_key = "test/bar";
    QVERIFY(d.appendListToMap(testMap));

    const auto newList = qvariant_cast<QVariantList>(GetOperation::get(testMap, "test/bar"));
    QCOMPARE(newList.count(), (testDataList.count() + valueList.count()));

    QVERIFY(newList.contains(1860));
    QVERIFY(newList.contains(QString("Some String")));
    QVERIFY(newList.contains("ELIL"));
    QVERIFY(newList.contains(-1));
}
#endif

bool AddValueData::appendListToMap(QVariantMap &map) const
{
    const QVariant data = GetOperation::get(map, m_key);

    if (!data.isValid() || data.isNull()) {
        qCCritical(addvaluelog) << "Error: Could not retrieve value for key"
                                << m_key;
        return false;
    }

    if (data.type() != QVariant::List) {
        qCCritical(addvaluelog) << "Error: Data stored in" << m_key
                                << "is not a QVariantList.";
        return false;
    }

    auto newList = qvariant_cast<QVariantList>(data);

    newList.append(m_values);

    map = RmKeysOperation::rmKeys(map, {m_key});
    map = AddKeysData{{{m_key, newList}}}.addKeys(map);

    return true;
}
