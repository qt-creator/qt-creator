// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "addabiflavor.h"

#include "addkeysoperation.h"
#include "findkeyoperation.h"
#include "findvalueoperation.h"
#include "getoperation.h"
#include "rmkeysoperation.h"

#include "settings.h"

#include <QLoggingCategory>

#ifdef WITH_TESTS
#include <QtTest>
#endif

Q_LOGGING_CATEGORY(addAbiFlavorLog, "qtc.sdktool.operations.addabiflavor", QtWarningMsg)

#include <iostream>

const char VERSION[] = "Version";
const char FLAVORS[] = "Flavors";
const char ABI_FILE_ID[] = "abi";

QString AddAbiFlavor::name() const
{
    return QLatin1String("addAbiFlavor");
}

QString AddAbiFlavor::helpText() const
{
    return QLatin1String("add an ABI flavor");
}

QString AddAbiFlavor::argumentsHelpText() const
{
    return QLatin1String("    --flavor <NAME>                            Name of new ABI flavor (required)\n"
                         "    --oses <OS>(,<OS>)*                        OSes the flavor applies to (required)\n");
}

bool AddAbiFlavor::setArguments(const QStringList &args)
{
    for (int i = 0; i < args.count(); ++i) {
        const QString current = args.at(i);
        const QString next = ((i + 1) < args.count()) ? args.at(i + 1) : QString();

        if (next.isNull() && current.startsWith("--")) {
            qCCritical(addAbiFlavorLog) << "No parameter for option '" << qPrintable(current) << "' given.";
            return false;
        }

        if (current == QLatin1String("--flavor")) {
            ++i; // skip next;
            m_flavor = next;
            continue;
        }

        if (current == QLatin1String("--oses")) {
            ++i; // skip next;
            m_oses = next.split(',');
            continue;
        }

        qCCritical(addAbiFlavorLog) << "Unknown parameter: " << qPrintable(current);
        return false;
    }

    if (m_flavor.isEmpty())
        qCCritical(addAbiFlavorLog) << "Error no flavor was passed.";

    if (m_oses.isEmpty())
        qCCritical(addAbiFlavorLog) << "Error no OSes name was passed.";

    return !m_flavor.isEmpty() && !m_oses.isEmpty();
}

int AddAbiFlavor::execute() const
{
    QVariantMap map = load(QLatin1String(ABI_FILE_ID));
    if (map.isEmpty())
        map = initializeAbiFlavors();

    QVariantMap result = addAbiFlavor(map);

    if (result.isEmpty() || result == map)
        return 2;

    return save(result, QLatin1String(ABI_FILE_ID)) ? 0 : 3;
}

#ifdef WITH_TESTS
void AddAbiFlavor::unittest()
{
    QVariantMap map = initializeAbiFlavors();
    QCOMPARE(map.count(), 1);
    QVERIFY(map.contains(QLatin1String(VERSION)));

    AddAbiFlavorData d;
    d.m_oses = QStringList{"linux", "windows"};
    d.m_flavor = "foo";
    map = d.addAbiFlavor(map);

    QCOMPARE(map.count(), 2);
    QVERIFY(map.contains(QLatin1String(VERSION)));
    QVERIFY(map.contains(QLatin1String(FLAVORS)));

    const QVariantMap flavorMap = map.value(QLatin1String(FLAVORS)).toMap();
    QCOMPARE(flavorMap.count(), 1);
    QCOMPARE(flavorMap.value("foo").toStringList(), QStringList({"linux", "windows"}));

    // Ignore known flavors:
    QTest::ignoreMessage(QtCriticalMsg,
                         QRegularExpression("Error: flavor .* already defined as extra ABI flavor."));
    const QVariantMap result = AddAbiFlavorData({{"linux"}, "foo"}).addAbiFlavor(map);

    QCOMPARE(map, result);
}
#endif

QVariantMap AddAbiFlavorData::addAbiFlavor(const QVariantMap &map) const
{
    // Sanity check: Is flavor already set in abi file?
    if (exists(map, m_flavor)) {
        qCCritical(addAbiFlavorLog) << "Error: flavor" << qPrintable(m_flavor) << "already defined as extra ABI flavor.";
        return map;
    }

    QVariantMap result = map;
    QVariantMap flavorMap = map.value(QLatin1String(FLAVORS)).toMap();
    flavorMap.insert(m_flavor, m_oses);
    result.insert(QLatin1String(FLAVORS), flavorMap);
    return result;
}

QVariantMap AddAbiFlavorData::initializeAbiFlavors()
{
    QVariantMap map;
    map.insert(QLatin1String(VERSION), 1);
    return map;
}

bool AddAbiFlavorData::exists(const QString &flavor)
{
    QVariantMap map = Operation::load(QLatin1String(ABI_FILE_ID));
    return exists(map, flavor);
}

bool AddAbiFlavorData::exists(const QVariantMap &map, const QString &flavor)
{
    const QVariantMap flavorMap = map.value(QLatin1String(FLAVORS)).toMap();
    return flavorMap.contains(flavor);
}
