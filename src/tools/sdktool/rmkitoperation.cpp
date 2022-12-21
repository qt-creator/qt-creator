// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "rmkitoperation.h"

#include "adddeviceoperation.h"
#include "addkeysoperation.h"
#include "addkitoperation.h"
#include "addqtoperation.h"
#include "addtoolchainoperation.h"
#include "findkeyoperation.h"
#include "findvalueoperation.h"
#include "getoperation.h"
#include "rmkeysoperation.h"

#include "settings.h"

#include <iostream>

#ifdef WITH_TESTS
#include <QTest>
#endif

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(rmkitlog, "qtc.sdktool.operations.rmkit", QtWarningMsg)

// Qt version file stuff:
const char PREFIX[] = "Profile.";
const char COUNT[] = "Profile.Count";
const char DEFAULT[] = "Profile.Default";
#ifdef WITH_TESTS
const char VERSION[] = "Version";
#endif

// Kit:
const char ID[] = "PE.Profile.Id";

QString RmKitOperation::name() const
{
    return QString("rmKit");
}

QString RmKitOperation::helpText() const
{
    return QString("remove a Kit");
}

QString RmKitOperation::argumentsHelpText() const
{
    return QString("    --id <ID>                                  id of the kit to remove.\n");
}

bool RmKitOperation::setArguments(const QStringList &args)
{
    if (args.count() != 2)
        return false;
    if (args.at(0) != "--id")
        return false;

    m_id = args.at(1);

    if (m_id.isEmpty())
        qCCritical(rmkitlog) << "No id given.";

    return !m_id.isEmpty();
}

int RmKitOperation::execute() const
{
    QVariantMap map = load(QLatin1String("Profiles"));
    if (map.isEmpty())
        map = AddKitOperation::initializeKits();

    QVariantMap result = rmKit(map, m_id);

    if (result == map)
        return 2;

    return save(result, QLatin1String("Profiles")) ? 0 : 3;
}

#ifdef WITH_TESTS
void RmKitOperation::unittest()
{
    QVariantMap tcMap = AddToolChainOperation::initializeToolChains();
    AddToolChainData d;
    d.m_id = "{tc-id}";
    d.m_languageId = "langId";
    d.m_displayName = "TC";
    d.m_path = "/usr/bin/gcc";
    d.m_targetAbi = "x86-linux-generic-elf-32bit";
    d.m_supportedAbis = "x86-linux-generic-elf-32bit";
    d.m_extra = {};

    tcMap = d.addToolChain(tcMap);

    QVariantMap qtMap = AddQtData::initializeQtVersions();
    AddQtData qtd;
    qtd.m_id = "{qt-id}";
    qtd.m_displayName = "Qt";
    qtd.m_type = "desktop-qt";
    qtd.m_qmake = "/usr/bin/qmake";

    qtMap = qtd.addQt(qtMap);

    QVariantMap devMap = AddDeviceOperation::initializeDevices();
    AddDeviceData devData;
    devData.m_id = "{dev-id}";
    devData.m_displayName = "Dev";
    devData.m_type = 0;
    devData.m_authentication = 0;
    devData.m_b2q_platformHardware = "HWplatform";
    devData.m_b2q_platformSoftware = "SWplatform";
    devData.m_debugServer = "localhost";
    devData.m_freePortsSpec = "10000-11000";
    devData.m_host = "localhost";
    devData.m_keyFile = "";
    devData.m_origin = 42;
    devData.m_osType = "desktop";
    devData.m_password = "";
    devData.m_sshPort = 22;
    devData.m_timeout = 10000;
    devData.m_uname = "uname";
    devData.m_version = 1;
    devMap = devData.addDevice(devMap);

    QHash<QString, QString> tcs;
    tcs.insert("Cxx", "{tc-id}");

    AddKitData kitData;
    kitData.m_id = "testId";
    kitData.m_displayName = "Test Qt Version";
    kitData.m_icon = "/tmp/icon.png";
    kitData.m_debuggerEngine = 1;
    kitData.m_debugger = "/usr/bin/gdb-test";
    kitData.m_deviceType = "Desktop";
    kitData.m_tcs = tcs;
    kitData.m_qt = "{qt-id}";
    kitData.m_mkspec = "unsupported/mkspec";
    kitData.m_extra = {{"PE.Profile.Data/extraData", QVariant("extraValue")}};

    QVariantMap map = kitData.addKit(AddKitData::initializeKits(), tcMap, qtMap, devMap, {});

    kitData.m_id = "testId2";
    kitData.m_icon = "/tmp/icon2.png";
    kitData.m_icon = "/usr/bin/gdb-test2";
    kitData.m_mkspec = "unsupported/mkspec2";
    kitData.m_extra = {{"PE.Profile.Data/extraData", QVariant("extraValue2")}};

    map = kitData.addKit(map, tcMap, qtMap, devMap, {});

    QTest::ignoreMessage(QtCriticalMsg, "Error: Could not find the default kit.");
    QVariantMap result = rmKit(map, "testId");
    QCOMPARE(result.count(), 4);
    QVERIFY(result.contains("Profile.0"));
    QVERIFY(result.contains(COUNT));
    QCOMPARE(result.value(COUNT).toInt(), 1);
    QVERIFY(result.contains(DEFAULT));
    QCOMPARE(result.value(DEFAULT).toInt(), 0);
    QVERIFY(result.contains(VERSION));
    QCOMPARE(result.value(VERSION).toInt(), 1);

    QTest::ignoreMessage(QtCriticalMsg, "Error: Id was not found.");
    result = rmKit(map, "unknown");
    QCOMPARE(result, map);

    QTest::ignoreMessage(QtCriticalMsg, "Error: Could not find the default kit.");
    result = rmKit(map, "testId2");
    QCOMPARE(result.count(), 4);
    QVERIFY(result.contains("Profile.0"));
    QVERIFY(result.contains(COUNT));
    QCOMPARE(result.value(COUNT).toInt(), 1);
    QVERIFY(result.contains(DEFAULT));
    QCOMPARE(result.value(DEFAULT).toInt(), 0);
    QVERIFY(result.contains(VERSION));
    QCOMPARE(result.value(VERSION).toInt(), 1);

    result = rmKit(result, QLatin1String("testId"));
    QCOMPARE(result.count(), 3);
    QVERIFY(result.contains(COUNT));
    QCOMPARE(result.value(COUNT).toInt(), 0);
    QVERIFY(result.contains(DEFAULT));
    QCOMPARE(result.value(DEFAULT).toInt(), -1);
    QVERIFY(result.contains(VERSION));
    QCOMPARE(result.value(VERSION).toInt(), 1);
}
#endif

QVariantMap RmKitOperation::rmKit(const QVariantMap &map, const QString &id)
{
    QVariantMap result = AddKitOperation::initializeKits();

    QVariantList profileList;
    bool ok;
    int count = GetOperation::get(map, COUNT).toInt(&ok);
    if (!ok) {
        qCCritical(rmkitlog) << "Error: The count found in map is not an integer.";
        return map;
    }

    int kitPos = -1;
    for (int i = 0; i < count; ++i) {
        const QString key = QString::fromLatin1(PREFIX) + QString::number(i);
        QVariantMap profile = map.value(key).toMap();
        if (profile.value(ID).toString() == id) {
            kitPos = i;
            continue;
        }
        profileList << profile;
    }
    if (profileList.count() == map.count() - 3) {
        qCCritical(rmkitlog) << "Error: Id was not found.";
        return map;
    }

    int defaultKit = GetOperation::get(map, DEFAULT).toInt(&ok);
    if (!ok) {
        qCCritical(rmkitlog) << "Error: Could not find the default kit.";
        defaultKit = -1;
    }

    if (defaultKit == kitPos || defaultKit < 0)
        defaultKit = (count > 1) ? 0 : -1;

    // remove data:
    result = RmKeysOperation::rmKeys(result, {COUNT, DEFAULT});

    // insert data:
    KeyValuePairList data;
    data << KeyValuePair(DEFAULT, QVariant(defaultKit));
    data << KeyValuePair(COUNT, QVariant(count - 1));

    for (int i = 0; i < profileList.count(); ++i)
        data << KeyValuePair(QString::fromLatin1(PREFIX) + QString::number(i), profileList.at(i));

    return AddKeysData{data}.addKeys(result);
}
