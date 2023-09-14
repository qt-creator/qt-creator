// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "addkitoperation.h"

#include "addcmakeoperation.h"
#include "adddeviceoperation.h"
#include "addkeysoperation.h"
#include "addqtoperation.h"
#include "addtoolchainoperation.h"
#include "findkeyoperation.h"
#include "findvalueoperation.h"
#include "getoperation.h"
#include "rmkeysoperation.h"

#include "settings.h"

#include <QDir>
#include <QLoggingCategory>
#include <QRegularExpression>

#ifdef WITH_TESTS
#include <QTest>
#endif

Q_LOGGING_CATEGORY(addkitlog, "qtc.sdktool.operations.addkit", QtWarningMsg)

// Qt version file stuff:
const char PREFIX[] = "Profile.";
const char VERSION[] = "Version";
const char COUNT[] = "Profile.Count";
const char DEFAULT[] = "Profile.Default";

// Kit:
const char ID[] = "PE.Profile.Id";
const char DISPLAYNAME[] = "PE.Profile.Name";
const char ICON[] = "PE.Profile.Icon";
const char AUTODETECTED[] = "PE.Profile.AutoDetected";
const char SDK[] = "PE.Profile.SDK";
const char ENV[] = "PE.Profile.Environment";
const char DATA[] = "PE.Profile.Data";

// Standard KitAspects:
const char DEBUGGER[] = "Debugger.Information";
const char DEBUGGER_ENGINE[] = "EngineType";
const char DEBUGGER_BINARY[] = "Binary";
const char DEVICE_TYPE[] = "PE.Profile.DeviceType";
const char DEVICE_ID[] = "PE.Profile.Device";
const char BUILDDEVICE_ID[] = "PE.Profile.BuildDevice";
const char SYSROOT[] = "PE.Profile.SysRoot";
const char TOOLCHAIN[] = "PE.Profile.ToolChainsV3";
const char MKSPEC[] = "QtPM4.mkSpecInformation";
const char QT[] = "QtSupport.QtInformation";
const char CMAKE_ID[] = "CMakeProjectManager.CMakeKitInformation";
const char CMAKE_GENERATOR[] = "CMake.GeneratorKitInformation";
const char CMAKE_CONFIGURATION[] = "CMake.ConfigurationKitInformation";

QString AddKitOperation::name() const
{
    return QString("addKit");
}

QString AddKitOperation::helpText() const
{
    return QString("add a Kit");
}

QString AddKitOperation::argumentsHelpText() const
{
    return QString(
        "    --id <ID>                                  id of the new kit (required).\n"
        "    --name <NAME>                              display name of the new kit (required).\n"
        "    --icon <PATH>                              icon of the new kit.\n"
        "    --debuggerid <ID>                          the id of the debugger to use.\n"
        "                                               (not compatible with --debugger and "
        "--debuggerengine)\n"
        "    --debuggerengine <ENGINE>                  debuggerengine of the new kit.\n"
        "    --debugger <PATH>                          debugger of the new kit.\n"
        "    --devicetype <TYPE>                        (run-)device type of the new kit (required).\n"
        "    --device <ID>                              (run-)device id to use (optional).\n"
        "    --builddevice <ID>                         build device id to use (optional).\n"
        "    --sysroot <PATH>                           sysroot of the new kit.\n"
        "    --toolchain <ID>                           tool chain of the new kit (obsolete!).\n"
        "    --<LANG>toolchain <ID>                     tool chain for a language.\n"
        "    --qt <ID>                                  Qt of the new kit.\n"
        "    --mkspec <PATH>                            mkspec of the new kit.\n"
        "    --env <VALUE>                              add a custom environment setting. [may be repeated]\n"
        "    --cmake <ID>                               set a cmake tool.\n"
        "    --cmake-generator <GEN>:<EXTRA>:<TOOLSET>:<PLATFORM>\n"
        "                                               set a cmake generator.\n"
        "    --cmake-config <KEY:TYPE=VALUE>            set a cmake configuration value [may be "
        "repeated]\n"
        "    <KEY> <TYPE:VALUE>                         extra key value pairs\n");
}

bool AddKitOperation::setArguments(const QStringList &args)
{
    for (int i = 0; i < args.count(); ++i) {
        const QString current = args.at(i);
        const QString next = ((i + 1) < args.count()) ? args.at(i + 1) : QString();

        if (current == "--id") {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_id = next;
            continue;
        }

        if (current == "--name") {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_displayName = next;
            continue;
        }

        if (current == "--icon") {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_icon = next;
            continue;
        }

        if (current == "--debuggerengine") {
            if (next.isNull())
                return false;
            ++i; // skip next;
            bool ok;
            m_debuggerEngine = next.toInt(&ok);
            if (!ok) {
                qCCritical(addkitlog) << "Debugger type is not an integer!";
                return false;
            }
            continue;
        }

        if (current == "--debuggerid") {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_debuggerId = next;
            continue;
        }

        if (current == "--debugger") {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_debugger = next;
            continue;
        }

        if (current == "--devicetype") {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_deviceType = next;
            continue;
        }

        if (current == "--device") {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_device = next;
            continue;
        }

        if (current == "--builddevice") {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_buildDevice = next;
            continue;
        }

        if (current == "--sysroot") {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_sysRoot = next;
            continue;
        }

        if (current.startsWith("--") && current.endsWith("toolchain")) {
            if (next.isNull())
                return false;
            ++i; // skip next;

            const QString tmp = current.mid(2);
            const QString tmp2 = tmp.mid(0, tmp.count() - 9 /* toolchain */);
            const QString lang = tmp2.isEmpty() ? QString("Cxx") : tmp2;

            if (next.isEmpty()) {
                qCCritical(addkitlog) << "Empty langid for toolchain given.";
                return false;
            }
            if (m_tcs.contains(lang)) {
                qCCritical(addkitlog) << "No langid for toolchain given twice.";
                return false;
            }
            m_tcs.insert(lang, next);
            continue;
        }

        if (current == "--qt") {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_qt = next;
            continue;
        }

        if (current == "--mkspec") {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_mkspec = next;
            continue;
        }

        if (current == "--env") {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_env.append(next);
            continue;
        }

        if (current == "--cmake") {
            if (next.isNull())
                return false;
            ++i;
            m_cmakeId = next;
            continue;
        }

        if (current == "--cmake-generator") {
            if (next.isNull())
                return false;
            ++i;
            QStringList parts = next.split(':');
            m_cmakeGenerator = parts.count() >= 1 ? parts.at(0) : QString();
            m_cmakeExtraGenerator = parts.count() >= 2 ? parts.at(1) : QString();
            m_cmakeGeneratorToolset = parts.count() >= 3 ? parts.at(2) : QString();
            m_cmakeGeneratorPlatform = parts.count() >= 4 ? parts.at(3) : QString();
            if (parts.count() > 4)
                return false;
            continue;
        }

        if (current == "--cmake-config") {
            if (next.isNull())
                return false;
            ++i;
            if (!next.isEmpty())
                m_cmakeConfiguration.append(next);
            continue;
        }

        if (next.isNull())
            return false;
        ++i; // skip next;
        KeyValuePair pair(current, next);
        if (!pair.value.isValid())
            return false;
        m_extra << pair;
    }

    if (m_id.isEmpty())
        qCCritical(addkitlog) << "No id given for kit.";
    if (m_displayName.isEmpty())
        qCCritical(addkitlog) << "No name given for kit.";
    if (m_deviceType.isEmpty())
        qCCritical(addkitlog) << "No devicetype given for kit.";
    if (!m_debuggerId.isEmpty() && (!m_debugger.isEmpty() || m_debuggerEngine != 0)) {
        qCCritical(addkitlog) << "Cannot set both debugger id and debugger/debuggerengine.";
        return false;
    }

    return !m_id.isEmpty() && !m_displayName.isEmpty() && !m_deviceType.isEmpty();
}

int AddKitOperation::execute() const
{
    QVariantMap map = load("Profiles");
    if (map.isEmpty())
        map = initializeKits();

    const QVariantMap result = addKit(map);
    if (result.isEmpty() || map == result)
        return 2;

    return save(result, "Profiles") ? 0 : 3;
}

#ifdef WITH_TESTS
void AddKitOperation::unittest()
{
    AddKitData baseData;
    baseData.m_id = "testId";
    baseData.m_displayName = "Test Kit";
    baseData.m_icon = "/tmp/icon.png";
    baseData.m_debuggerEngine = 1;
    baseData.m_debugger = "/usr/bin/gdb-test";
    baseData.m_deviceType = "Desktop";
    baseData.m_device = "{dev-id}";
    baseData.m_qt = "{qt-id}";
    baseData.m_mkspec = "unsupported/mkspec";
    baseData.m_extra = {{"PE.Profile.Data/extraData", QVariant("extraValue")}};

    QVariantMap map = initializeKits();

    QVariantMap tcMap = AddToolChainData::initializeToolChains();
    AddToolChainData atcd;
    atcd.m_id = "{tc-id}";
    atcd.m_languageId = "langId";
    atcd.m_displayName = "TC";
    atcd.m_path = "/usr/bin/gcc";
    atcd.m_targetAbi = "x86-linux-generic-elf-32bit";
    atcd.m_supportedAbis = "x86-linux-generic-elf-32bit";
    atcd.m_extra = {};

    tcMap = atcd.addToolChain(tcMap);

    QVariantMap qtMap = AddQtData::initializeQtVersions();
    AddQtData aqtd;

    aqtd.m_id = "{qt-id}";
    aqtd.m_displayName = "Qt";
    aqtd.m_type = "desktop-qt";
    aqtd.m_qmake = "/usr/bin/qmake";
    aqtd.m_abis = QStringList{};
    aqtd.m_extra = {};

    qtMap = aqtd.addQt(qtMap);

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

    const QStringList env = {"TEST=1", "PATH"};

    QCOMPARE(map.count(), 3);
    QVERIFY(map.contains(VERSION));
    QCOMPARE(map.value(VERSION).toInt(), 1);
    QVERIFY(map.contains(COUNT));
    QCOMPARE(map.value(COUNT).toInt(), 0);
    QVERIFY(map.contains(DEFAULT));
    QVERIFY(map.value(DEFAULT).toString().isEmpty());

    QHash<QString, QString> tcs;
    tcs.insert("Cxx", "{tcXX-id}");

    // Fail if TC is not there:
    QTest::ignoreMessage(QtCriticalMsg,
                         QRegularExpression(
                             "Error: Toolchain .* for language Cxx does not exist."));

    AddKitData kitData = baseData;
    kitData.m_tcs = tcs;
    QVariantMap empty = kitData.addKit(map, tcMap, qtMap, devMap, {});

    QVERIFY(empty.isEmpty());
    // Do not fail if TC is an ABI:
    tcs.clear();
    tcs.insert("C", "x86-linux-generic-elf-64bit");
    kitData = baseData;
    kitData.m_tcs = tcs;
    empty = kitData.addKit(map, tcMap, qtMap, devMap, {});
    QVERIFY(!empty.isEmpty());

    // QTCREATORBUG-11983, mach_o was not covered by the first attempt to fix this.
    tcs.insert("D", "x86-macos-generic-mach_o-64bit");
    kitData = baseData;
    kitData.m_tcs = tcs;
    empty = kitData.addKit(map, tcMap, qtMap, devMap, {});
    QVERIFY(!empty.isEmpty());

    tcs.clear();
    tcs.insert("Cxx", "{tc-id}");

    // Fail if Qt is not there:
    QTest::ignoreMessage(QtCriticalMsg, QRegularExpression("Error: Qt .* does not exist."));

    kitData = baseData;
    kitData.m_qt = "{qtXX-id}";
    empty = kitData.addKit(map, tcMap, qtMap, devMap, {});
    QVERIFY(empty.isEmpty());

    // Fail if dev is not there:
    QTest::ignoreMessage(QtCriticalMsg, QRegularExpression("Error: Device .* does not exist."));

    kitData = baseData;
    kitData.m_device = "{devXX-id}";
    empty = kitData.addKit(map, tcMap, qtMap, devMap, {});
    QVERIFY(empty.isEmpty());

    // Profile 0:
    kitData = baseData;
    kitData.m_tcs = tcs;
    map = kitData.addKit(map, tcMap, qtMap, devMap, {});

    QCOMPARE(map.count(), 4);
    QVERIFY(map.contains(VERSION));
    QCOMPARE(map.value(VERSION).toInt(), 1);
    QVERIFY(map.contains(COUNT));
    QCOMPARE(map.value(COUNT).toInt(), 1);
    QVERIFY(map.contains(DEFAULT));
    QCOMPARE(map.value(DEFAULT).toString(), "testId");
    QVERIFY(map.contains("Profile.0"));

    QVariantMap profile0 = map.value("Profile.0").toMap();
    QCOMPARE(profile0.count(), 6);
    QVERIFY(profile0.contains(ID));
    QCOMPARE(profile0.value(ID).toString(), "testId");
    QVERIFY(profile0.contains(DISPLAYNAME));
    QCOMPARE(profile0.value(DISPLAYNAME).toString(), "Test Kit");
    QVERIFY(profile0.contains(ICON));
    QCOMPARE(profile0.value(ICON).toString(), "/tmp/icon.png");
    QVERIFY(profile0.contains(DATA));
    QCOMPARE(profile0.value(DATA).type(), QVariant::Map);
    QVERIFY(profile0.contains(AUTODETECTED));
    QCOMPARE(profile0.value(AUTODETECTED).toBool(), true);
    QVERIFY(profile0.contains(SDK));
    QCOMPARE(profile0.value(SDK).toBool(), true);

    QVariantMap data = profile0.value(DATA).toMap();
    QCOMPARE(data.count(), 7);
    QVERIFY(data.contains(DEBUGGER));
    QCOMPARE(data.value(DEBUGGER).type(), QVariant::Map);
    QVERIFY(data.contains(DEVICE_TYPE));
    QCOMPARE(data.value(DEVICE_TYPE).toString(), "Desktop");
    QVERIFY(data.contains(TOOLCHAIN));
    QVERIFY(data.contains(QT));
    QCOMPARE(data.value(QT).toString(), "SDK.{qt-id}");
    QVERIFY(data.contains(MKSPEC));
    QCOMPARE(data.value(MKSPEC).toString(), "unsupported/mkspec");
    QVERIFY(data.contains("extraData"));
    QCOMPARE(data.value("extraData").toString(), "extraValue");

    QVariantMap tcOutput = data.value(TOOLCHAIN).toMap();
    QCOMPARE(tcOutput.count(), 1);
    QVERIFY(tcOutput.contains("Cxx"));
    QCOMPARE(tcOutput.value("Cxx"), "{tc-id}");

    // Ignore exist ids:
    QTest::ignoreMessage(QtCriticalMsg, QRegularExpression("Error: Id .* already defined as kit."));

    kitData = baseData;
    kitData.m_displayName = "Test Qt Version X";
    kitData.m_icon = "/tmp/icon3.png";
    kitData.m_debugger = "/usr/bin/gdb-test3";
    kitData.m_tcs = tcs;
    QVariantMap result = kitData.addKit(map, tcMap, qtMap, devMap, {});
    QVERIFY(result.isEmpty());

    // Profile 1: Make sure name is unique:
    kitData = baseData;
    kitData.m_id = "testId2";
    kitData.m_displayName = "Test Kit2";
    kitData.m_icon = "/tmp/icon2.png";
    kitData.m_debugger = "/usr/bin/gdb-test2";
    kitData.m_sysRoot = "/sys/root//";
    kitData.m_env = env;
    kitData.m_tcs = tcs;
    map = kitData.addKit(map, tcMap, qtMap, devMap, {});

    QCOMPARE(map.count(), 5);
    QVERIFY(map.contains(VERSION));
    QCOMPARE(map.value(VERSION).toInt(), 1);
    QVERIFY(map.contains(COUNT));
    QCOMPARE(map.value(COUNT).toInt(), 2);
    QVERIFY(map.contains(DEFAULT));
    QCOMPARE(map.value(DEFAULT).toInt(), 0);
    QVERIFY(map.contains("Profile.0"));
    QVERIFY(map.contains("Profile.1"));
    QCOMPARE(map.value("Profile.0"), profile0);

    QVariantMap profile1 = map.value("Profile.1").toMap();
    QCOMPARE(profile1.count(), 6);
    QVERIFY(profile1.contains(ID));
    QCOMPARE(profile1.value(ID).toString(), "testId2");
    QVERIFY(profile1.contains(DISPLAYNAME));
    QCOMPARE(profile1.value(DISPLAYNAME).toString(), "Test Kit2");
    QVERIFY(profile1.contains(ICON));
    QCOMPARE(profile1.value(ICON).toString(), "/tmp/icon2.png");
    QVERIFY(profile1.contains(DATA));
    QCOMPARE(profile1.value(DATA).type(), QVariant::Map);
    QVERIFY(profile1.contains(AUTODETECTED));
    QCOMPARE(profile1.value(AUTODETECTED).toBool(), true);
    QVERIFY(profile1.contains(SDK));
    QCOMPARE(profile1.value(SDK).toBool(), true);

    data = profile1.value(DATA).toMap();
    QCOMPARE(data.count(), 9);
    QVERIFY(data.contains(DEBUGGER));
    QCOMPARE(data.value(DEBUGGER).type(), QVariant::Map);
    QVERIFY(data.contains(DEVICE_TYPE));
    QCOMPARE(data.value(DEVICE_TYPE).toString(), "Desktop");
    QVERIFY(data.contains(DEVICE_ID));
    QCOMPARE(data.value(DEVICE_ID).toString(), "{dev-id}");
    QVERIFY(data.contains(SYSROOT));
    QCOMPARE(data.value(SYSROOT).toString(), "/sys/root");
    QVERIFY(data.contains(TOOLCHAIN));
    QVERIFY(data.contains(QT));
    QCOMPARE(data.value(QT).toString(), "SDK.{qt-id}");
    QVERIFY(data.contains(MKSPEC));
    QCOMPARE(data.value(MKSPEC).toString(), "unsupported/mkspec");
    QVERIFY(data.contains(ENV));
    QCOMPARE(data.value(ENV).toStringList(), env);
    QVERIFY(data.contains("extraData"));
    QCOMPARE(data.value("extraData").toString(), "extraValue");

    tcOutput = data.value(TOOLCHAIN).toMap();
    QCOMPARE(tcOutput.count(), 1);
    QVERIFY(tcOutput.contains("Cxx"));
    QCOMPARE(tcOutput.value("Cxx"), "{tc-id}");

    // Profile 2: Test debugger id:
    kitData = baseData;
    kitData.m_id = "test with debugger Id";
    kitData.m_displayName = "Test debugger Id";
    kitData.m_icon = "/tmp/icon2.png";
    kitData.m_debuggerId = "debugger Id";
    kitData.m_env = env;

    map = kitData.addKit(map, tcMap, qtMap, devMap, {});
    QCOMPARE(map.count(), 6);
    QVERIFY(map.contains(VERSION));
    QCOMPARE(map.value(VERSION).toInt(), 1);
    QVERIFY(map.contains(COUNT));
    QCOMPARE(map.value(COUNT).toInt(), 3);
    QVERIFY(map.contains(DEFAULT));
    QCOMPARE(map.value(DEFAULT).toInt(), 0);
    QVERIFY(map.contains("Profile.0"));
    QVERIFY(map.contains("Profile.1"));
    QVERIFY(map.contains("Profile.2"));
    QCOMPARE(map.value("Profile.0"), profile0);
    QCOMPARE(map.value("Profile.1"), profile1);

    QVariantMap profile2 = map.value("Profile.2").toMap();
    QCOMPARE(profile2.count(), 6);
    QVERIFY(profile2.contains(ID));
    QCOMPARE(profile2.value(ID).toString(), "test with debugger Id");
    QVERIFY(profile2.contains(DISPLAYNAME));
    QCOMPARE(profile2.value(DISPLAYNAME).toString(), "Test debugger Id");
    QVERIFY(profile2.contains(ICON));
    QCOMPARE(profile2.value(ICON).toString(), "/tmp/icon2.png");
    QVERIFY(profile2.contains(DATA));
    QCOMPARE(profile2.value(DATA).type(), QVariant::Map);
    QVERIFY(profile2.contains(AUTODETECTED));
    QCOMPARE(profile2.value(AUTODETECTED).toBool(), true);
    QVERIFY(profile2.contains(SDK));
    QCOMPARE(profile2.value(SDK).toBool(), true);

    data = profile2.value(DATA).toMap();
    QCOMPARE(data.count(), 7);
    QVERIFY(data.contains(DEBUGGER));
    QCOMPARE(data.value(DEBUGGER).toString(), "debugger Id");
}
#endif

QVariantMap AddKitData::addKit(const QVariantMap &map) const
{
    QVariantMap tcMap = Operation::load("ToolChains");
    QVariantMap qtMap = Operation::load("QtVersions");
    QVariantMap devMap = Operation::load("Devices");
    QVariantMap cmakeMap = Operation::load("cmaketools");

    return AddKitData::addKit(map, tcMap, qtMap, devMap, cmakeMap);
}

QVariantMap AddKitData::addKit(const QVariantMap &map,
                               const QVariantMap &tcMap,
                               const QVariantMap &qtMap,
                               const QVariantMap &devMap,
                               const QVariantMap &cmakeMap) const
{
    // Sanity check: Make sure autodetection source is not in use already:
    const QStringList valueKeys = FindValueOperation::findValue(map, QVariant(m_id));
    bool hasId = false;
    for (const QString &k : valueKeys) {
        if (k.endsWith(QString('/') + ID)) {
            hasId = true;
            break;
        }
    }
    if (hasId) {
        qCCritical(addkitlog) << "Error: Id" << qPrintable(m_id) << "already defined as kit.";
        return QVariantMap();
    }

    for (auto i = m_tcs.constBegin(); i != m_tcs.constEnd(); ++i) {
        if (!i.value().isEmpty() && !AddToolChainOperation::exists(tcMap, i.value())) {
            const QRegularExpression abiRegExp(
                "^[a-z0-9_]+-[a-z0-9_]+-[a-z0-9_]+-[a-z0-9_]+-(8|16|32|64|128)bit$");
            if (!abiRegExp.match(i.value()).hasMatch()) {
                qCCritical(addkitlog) << "Error: Toolchain" << qPrintable(i.value())
                                      << "for language" << qPrintable(i.key()) << "does not exist.";
                return QVariantMap();
            }
        }
    }

    QString qtId = m_qt;
    if (!qtId.isEmpty() && !qtId.startsWith("SDK."))
        qtId = QString::fromLatin1("SDK.") + m_qt;
    if (!qtId.isEmpty() && !AddQtData::exists(qtMap, qtId)) {
        qCCritical(addkitlog) << "Error: Qt" << qPrintable(qtId) << "does not exist.";
        return QVariantMap();
    }
    if (!m_device.isEmpty() && !AddDeviceOperation::exists(devMap, m_device)) {
        qCCritical(addkitlog) << "Error: Device" << qPrintable(m_device) << "does not exist.";
        return QVariantMap();
    }
    if (!m_buildDevice.isEmpty() && !AddDeviceOperation::exists(devMap, m_buildDevice)) {
        qCCritical(addkitlog) << "Error: Device" << qPrintable(m_buildDevice) << "does not exist.";
        return QVariantMap();
    }

    // Treat a qt that was explicitly set to '' as "no Qt"
    if (!qtId.isNull() && qtId.isEmpty())
        qtId = "-1";

    if (!m_cmakeId.isEmpty() && !AddCMakeData::exists(cmakeMap, m_cmakeId)) {
        qCCritical(addkitlog) << "Error: CMake tool" << qPrintable(m_cmakeId) << "does not exist.";
        return QVariantMap();
    }

    // Find position to insert:
    bool ok;
    int count = GetOperation::get(map, COUNT).toInt(&ok);
    if (!ok || count < 0) {
        qCCritical(addkitlog) << "Error: Count found in kits file seems wrong.";
        return QVariantMap();
    }
    const QString kit = QString::fromLatin1(PREFIX) + QString::number(count);

    QString defaultKit = GetOperation::get(map, DEFAULT).toString();
    if (defaultKit.isEmpty())
        defaultKit = m_id;

    // remove data:
    QVariantMap cleaned = RmKeysOperation::rmKeys(map, {COUNT, DEFAULT});

    // insert data:
    KeyValuePairList data = {KeyValuePair({kit, ID}, QVariant(m_id)),
                             KeyValuePair({kit, DISPLAYNAME}, QVariant(m_displayName)),
                             KeyValuePair({kit, ICON}, QVariant(m_icon)),
                             KeyValuePair({kit, AUTODETECTED}, QVariant(true)),
                             KeyValuePair({kit, SDK}, QVariant(true))};

    if (!m_debuggerId.isEmpty() || !m_debugger.isEmpty()) {
        if (m_debuggerId.isEmpty()) {
            data << KeyValuePair({kit, DATA, DEBUGGER, DEBUGGER_ENGINE}, QVariant(m_debuggerEngine));
            data << KeyValuePair({kit, DATA, DEBUGGER, DEBUGGER_BINARY}, QVariant(m_debugger));
        } else {
            data << KeyValuePair({kit, DATA, DEBUGGER}, QVariant(m_debuggerId));
        }
    }

    if (!m_deviceType.isNull())
        data << KeyValuePair({kit, DATA, DEVICE_TYPE}, QVariant(m_deviceType));
    if (!m_device.isNull())
        data << KeyValuePair({kit, DATA, DEVICE_ID}, QVariant(m_device));
    if (!m_buildDevice.isNull())
        data << KeyValuePair({kit, DATA, BUILDDEVICE_ID}, QVariant(m_buildDevice));
    if (!m_sysRoot.isNull())
        data << KeyValuePair({kit, DATA, SYSROOT}, QVariant(cleanPath(m_sysRoot)));
    for (auto i = m_tcs.constBegin(); i != m_tcs.constEnd(); ++i)
        data << KeyValuePair({kit, DATA, TOOLCHAIN, i.key()}, QVariant(i.value()));
    if (!qtId.isNull())
        data << KeyValuePair({kit, DATA, QT}, QVariant(qtId));
    if (!m_mkspec.isNull())
        data << KeyValuePair({kit, DATA, MKSPEC}, QVariant(m_mkspec));
    if (!m_cmakeId.isNull())
        data << KeyValuePair({kit, DATA, CMAKE_ID}, QVariant(m_cmakeId));
    if (!m_cmakeGenerator.isNull()) {
        QVariantMap generatorMap;
        generatorMap.insert("Generator", m_cmakeGenerator);
        if (!m_cmakeExtraGenerator.isNull())
            generatorMap.insert("ExtraGenerator", m_cmakeExtraGenerator);
        if (!m_cmakeGeneratorToolset.isNull())
            generatorMap.insert("Toolset", m_cmakeGeneratorToolset);
        if (!m_cmakeGeneratorPlatform.isNull())
            generatorMap.insert("Platform", m_cmakeGeneratorPlatform);
        data << KeyValuePair({kit, DATA, CMAKE_GENERATOR}, generatorMap);
    }
    if (!m_cmakeConfiguration.isEmpty())
        data << KeyValuePair({kit, DATA, CMAKE_CONFIGURATION}, QVariant(m_cmakeConfiguration));
    if (!m_env.isEmpty())
        data << KeyValuePair({kit, DATA, ENV}, QVariant(m_env));

    data << KeyValuePair(DEFAULT, QVariant(defaultKit));
    data << KeyValuePair(COUNT, QVariant(count + 1));

    KeyValuePairList qtExtraList;
    for (const KeyValuePair &pair : std::as_const(m_extra))
        qtExtraList << KeyValuePair(QStringList() << kit << pair.key, pair.value);
    data.append(qtExtraList);

    return AddKeysData{data}.addKeys(cleaned);
}

QVariantMap AddKitData::initializeKits()
{
    QVariantMap map;
    map.insert(VERSION, 1);
    map.insert(DEFAULT, QString());
    map.insert(COUNT, 0);
    return map;
}
