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

#include "addkitoperation.h"

#include "addcmakeoperation.h"
#include "addkeysoperation.h"
#include "addtoolchainoperation.h"
#include "addqtoperation.h"
#include "adddeviceoperation.h"
#include "findkeyoperation.h"
#include "findvalueoperation.h"
#include "getoperation.h"
#include "rmkeysoperation.h"

#include "settings.h"

#include <QDir>
#include <QRegExp>

#include <iostream>

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

// Standard KitInformation:
const char DEBUGGER[] = "Debugger.Information";
const char DEBUGGER_ENGINE[] = "EngineType";
const char DEBUGGER_BINARY[] = "Binary";
const char DEVICE_TYPE[] = "PE.Profile.DeviceType";
const char DEVICE_ID[] = "PE.Profile.Device";
const char SYSROOT[] = "PE.Profile.SysRoot";
const char TOOLCHAIN[] = "PE.Profile.ToolChains";
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
           "                                               (not compatible with --debugger and --debuggerengine)\n"
           "    --debuggerengine <ENGINE>                  debuggerengine of the new kit.\n"
           "    --debugger <PATH>                          debugger of the new kit.\n"
           "    --devicetype <TYPE>                        device type of the new kit (required).\n"
           "    --device <ID>                              device id to use (optional).\n"
           "    --sysroot <PATH>                           sysroot of the new kit.\n"
           "    --toolchain <ID>                           tool chain of the new kit (obsolete!).\n"
           "    --<LANG>toolchain <ID>                     tool chain for a language.\n"
           "    --qt <ID>                                  Qt of the new kit.\n"
           "    --mkspec <PATH>                            mkspec of the new kit.\n"
           "    --env <VALUE>                              add a custom environment setting. [may be repeated]\n"
           "    --cmake <ID>                               set a cmake tool.\n"
           "    --cmake-generator <GEN>:<EXTRA>:<TOOLSET>:<PLATFORM>\n"
           "                                               set a cmake generator.\n"
           "    --cmake-config <KEY:TYPE=VALUE>            set a cmake configuration value [may be repeated]\n"
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
                std::cerr << "Debugger type is not an integer!" << std::endl;
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

        if (current == "--sysroot") {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_sysRoot = QDir::fromNativeSeparators(next);
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
                std::cerr << "Empty langid for toolchain given." << std::endl << std::endl;
                return false;
            }
            if (m_tcs.contains(lang)) {
                std::cerr << "No langid for toolchain given twice." << std::endl << std::endl;
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
        std::cerr << "No id given for kit." << std::endl << std::endl;
    if (m_displayName.isEmpty())
        std::cerr << "No name given for kit." << std::endl << std::endl;
    if (m_deviceType.isEmpty())
        std::cerr << "No devicetype given for kit." << std::endl << std::endl;
    if (!m_debuggerId.isEmpty() && (!m_debugger.isEmpty() || m_debuggerEngine != 0)) {
        std::cerr << "Can not set both debugger id and debugger/debuggerengine." << std::endl << std::endl;
        return false;
    }

    return !m_id.isEmpty() && !m_displayName.isEmpty() && !m_deviceType.isEmpty();
}

int AddKitOperation::execute() const
{
    QVariantMap map = load("Profiles");
    if (map.isEmpty())
        map = initializeKits();

    QVariantMap result = addKit(map, m_id, m_displayName, m_icon, m_debuggerId, m_debuggerEngine,
                                m_debugger, m_deviceType, m_device, m_sysRoot, m_tcs, m_qt,
                                m_mkspec, m_cmakeId, m_cmakeGenerator, m_cmakeExtraGenerator,
                                m_cmakeGeneratorToolset, m_cmakeGeneratorPlatform, m_cmakeConfiguration,
                                m_env, m_extra);

    if (result.isEmpty() || map == result)
        return 2;

    return save(result, "Profiles") ? 0 : 3;
}

#ifdef WITH_TESTS
bool AddKitOperation::test() const
{
    QVariantMap map = initializeKits();

    QVariantMap tcMap = AddToolChainOperation::initializeToolChains();
    tcMap = AddToolChainOperation::addToolChain(tcMap, "{tc-id}", "langId", "TC", "/usr/bin/gcc",
                                                "x86-linux-generic-elf-32bit",
                                                "x86-linux-generic-elf-32bit",
                                                KeyValuePairList());

    QVariantMap qtMap = AddQtOperation::initializeQtVersions();
    qtMap = AddQtOperation::addQt(qtMap, "{qt-id}", "Qt", "desktop-qt", "/usr/bin/qmake",
                                  KeyValuePairList());

    QVariantMap devMap = AddDeviceOperation::initializeDevices();
    devMap = AddDeviceOperation::addDevice(devMap, "{dev-id}", "Dev", 0, 0,
                                           "HWplatform", "SWplatform",
                                           "localhost", "10000-11000",
                                           "localhost", "", 42,
                                           "desktop", "", 22, 10000,
                                           "uname", 1,
                                           KeyValuePairList());

    const QStringList env = {"TEST=1", "PATH"};

    if (map.count() != 3
            || !map.contains(VERSION) || map.value(VERSION).toInt() != 1
            || !map.contains(COUNT) || map.value(COUNT).toInt() != 0
            || !map.contains(DEFAULT) || !map.value(DEFAULT).toString().isEmpty())
        return false;

    QHash<QString, QString> tcs;
    tcs.insert("Cxx", "{tcXX-id}");

    // Fail if TC is not there:
    QVariantMap empty = addKit(map, tcMap, qtMap, devMap, QVariantMap(),
                               "testId", "Test Kit", "/tmp/icon.png", QString(), 1,
                               "/usr/bin/gdb-test", "Desktop", "{dev-id}", QString(),
                               tcs, "{qt-id}", "unsupported/mkspec",
                               QString(), QString(), QString(), QString(), QString(), QStringList(),
                               QStringList(),
                               KeyValuePairList({KeyValuePair("PE.Profile.Data/extraData", QVariant("extraValue"))}));
    if (!empty.isEmpty())
        return false;
    // Do not fail if TC is an ABI:
    tcs.clear();
    tcs.insert("C", "x86-linux-generic-elf-64bit");
    empty = addKit(map, tcMap, qtMap, devMap,  QVariantMap(),
                   "testId", "Test Kit", "/tmp/icon.png", QString(), 1,
                   "/usr/bin/gdb-test", "Desktop", "{dev-id}", QString(),
                   tcs, "{qt-id}", "unsupported/mkspec",
                   QString(), QString(), QString(), QString(), QString(), QStringList(), env,
                   KeyValuePairList({KeyValuePair("PE.Profile.Data/extraData", QVariant("extraValue"))}));
    if (empty.isEmpty())
        return false;
    // QTCREATORBUG-11983, mach_o was not covered by the first attempt to fix this.
    tcs.insert("D", "x86-macos-generic-mach_o-64bit");
    empty = addKit(map, tcMap, qtMap, devMap,  QVariantMap(),
                   "testId", "Test Kit", "/tmp/icon.png", QString(), 1,
                   "/usr/bin/gdb-test", "Desktop", "{dev-id}", QString(),
                   tcs, "{qt-id}", "unsupported/mkspec",
                   QString(), QString(), QString(), QString(), QString(), QStringList(), env,
                   KeyValuePairList({KeyValuePair("PE.Profile.Data/extraData", QVariant("extraValue"))}));
    if (empty.isEmpty())
        return false;

    tcs.clear();
    tcs.insert("Cxx", "{tc-id}");

    // Fail if Qt is not there:
    empty = addKit(map, tcMap, qtMap, devMap, QVariantMap(),
                   "testId", "Test Kit", "/tmp/icon.png", QString(), 1,
                   "/usr/bin/gdb-test", "Desktop", "{dev-id}", QString(), tcs, "{qtXX-id}",
                   "unsupported/mkspec",
                   QString(), QString(), QString(), QString(), QString(), QStringList(), env,
                   KeyValuePairList({KeyValuePair("PE.Profile.Data/extraData", QVariant("extraValue"))}));
    if (!empty.isEmpty())
        return false;
    // Fail if dev is not there:
    empty = addKit(map, tcMap, qtMap, devMap, QVariantMap(),
                   "testId", "Test Kit", "/tmp/icon.png", QString(), 1,
                   "/usr/bin/gdb-test", "Desktop", "{devXX-id}", QString(), tcs, "{qt-id}",
                   "unsupported/mkspec",
                   QString(), QString(), QString(), QString(), QString(), QStringList(), env,
                   KeyValuePairList({KeyValuePair("PE.Profile.Data/extraData", QVariant("extraValue"))}));
    if (!empty.isEmpty())
        return false;

    // Profile 0:
    map = addKit(map, tcMap, qtMap, devMap, QVariantMap(),
                 "testId", "Test Kit", "/tmp/icon.png", QString(), 1,
                 "/usr/bin/gdb-test", "Desktop", QString(), QString(), tcs, "{qt-id}",
                 "unsupported/mkspec",
                 QString(), QString(), QString(), QString(), QString(), QStringList(), env,
                 KeyValuePairList({KeyValuePair("PE.Profile.Data/extraData", QVariant("extraValue"))}));

    if (map.count() != 4
            || !map.contains(VERSION) || map.value(VERSION).toInt() != 1
            || !map.contains(COUNT) || map.value(COUNT).toInt() != 1
            || !map.contains(DEFAULT) || map.value(DEFAULT).toString() != "testId"
            || !map.contains("Profile.0"))
        return false;

    QVariantMap profile0 = map.value("Profile.0").toMap();
    if (profile0.count() != 6
            || !profile0.contains(ID) || profile0.value(ID).toString() != "testId"
            || !profile0.contains(DISPLAYNAME) || profile0.value(DISPLAYNAME).toString() != "Test Kit"
            || !profile0.contains(ICON) || profile0.value(ICON).toString() != "/tmp/icon.png"
            || !profile0.contains(DATA) || profile0.value(DATA).type() != QVariant::Map
            || !profile0.contains(AUTODETECTED) || profile0.value(AUTODETECTED).toBool() != true
            || !profile0.contains(SDK) || profile0.value(SDK).toBool() != true)
        return false;

    QVariantMap data = profile0.value(DATA).toMap();
    if (data.count() != 7
            || !data.contains(DEBUGGER) || data.value(DEBUGGER).type() != QVariant::Map
            || !data.contains(DEVICE_TYPE) || data.value(DEVICE_TYPE).toString() != "Desktop"
            || !data.contains(TOOLCHAIN)
            || !data.contains(QT) || data.value(QT).toString() != "SDK.{qt-id}"
            || !data.contains(MKSPEC) || data.value(MKSPEC).toString() != "unsupported/mkspec"
            || !data.contains("extraData") || data.value("extraData").toString() != "extraValue")
        return false;
    QVariantMap tcOutput = data.value(TOOLCHAIN).toMap();
    if (tcOutput.count() != 1
            || !tcOutput.contains("Cxx") || tcOutput.value("Cxx") != "{tc-id}")
        return false;

    // Ignore existing ids:
    QVariantMap result = addKit(map, tcMap, qtMap, devMap, QVariantMap(),
                                "testId", "Test Qt Version X",
                                "/tmp/icon3.png", QString(), 1, "/usr/bin/gdb-test3", "Desktop",
                                QString(), QString(), tcs, "{qt-id}", "unsupported/mkspec",
                                QString(), QString(), QString(), QString(), QString(), QStringList(), env,
                                KeyValuePairList({KeyValuePair("PE.Profile.Data/extraData", QVariant("extraValue"))}));
    if (!result.isEmpty())
        return false;

    // Profile 1: Make sure name is unique:
    map = addKit(map, tcMap, qtMap, devMap, QVariantMap(),
                 "testId2", "Test Kit2", "/tmp/icon2.png", QString(), 1,
                 "/usr/bin/gdb-test2", "Desktop", "{dev-id}", "/sys/root\\\\", tcs,
                 "{qt-id}", "unsupported/mkspec",
                 QString(), QString(), QString(), QString(), QString(), QStringList(), env,
                 KeyValuePairList({KeyValuePair("PE.Profile.Data/extraData", QVariant("extraValue"))}));
    if (map.count() != 5
            || !map.contains(VERSION) || map.value(VERSION).toInt() != 1
            || !map.contains(COUNT) || map.value(COUNT).toInt() != 2
            || !map.contains(DEFAULT) || map.value(DEFAULT).toInt() != 0
            || !map.contains("Profile.0")
            || !map.contains("Profile.1"))

    if (map.value("Profile.0") != profile0)
        return false;

    QVariantMap profile1 = map.value("Profile.1").toMap();
    if (profile1.count() != 6
            || !profile1.contains(ID) || profile1.value(ID).toString() != "testId2"
            || !profile1.contains(DISPLAYNAME) || profile1.value(DISPLAYNAME).toString() != "Test Kit2"
            || !profile1.contains(ICON) || profile1.value(ICON).toString() != "/tmp/icon2.png"
            || !profile1.contains(DATA) || profile1.value(DATA).type() != QVariant::Map
            || !profile1.contains(AUTODETECTED) || profile1.value(AUTODETECTED).toBool() != true
            || !profile1.contains(SDK) || profile1.value(SDK).toBool() != true)
        return false;

    data = profile1.value(DATA).toMap();
    if (data.count() != 9
            || !data.contains(DEBUGGER) || data.value(DEBUGGER).type() != QVariant::Map
            || !data.contains(DEVICE_TYPE) || data.value(DEVICE_TYPE).toString() != "Desktop"
            || !data.contains(DEVICE_ID) || data.value(DEVICE_ID).toString() != "{dev-id}"
            || !data.contains(SYSROOT) || data.value(SYSROOT).toString() != "/sys/root\\\\"
            || !data.contains(TOOLCHAIN)
            || !data.contains(QT) || data.value(QT).toString() != "SDK.{qt-id}"
            || !data.contains(MKSPEC) || data.value(MKSPEC).toString() != "unsupported/mkspec"
            || !data.contains(ENV) || data.value(ENV).toStringList() != env
            || !data.contains("extraData") || data.value("extraData").toString() != "extraValue")
        return false;
    tcOutput = data.value(TOOLCHAIN).toMap();
    if (tcOutput.count() != 1
            || !tcOutput.contains("Cxx") || tcOutput.value("Cxx") != "{tc-id}")
        return false;

    // Profile 2: Test debugger id:
    map = addKit(map, tcMap, qtMap, devMap, QVariantMap(),
                 "test with debugger Id", "Test debugger Id",
                 "/tmp/icon2.png", "debugger Id", 0, QString(), "Desktop", QString(), QString(),
                 tcs, "{qt-id}", "unsupported/mkspec",
                 QString(), QString(), QString(), QString(), QString(), QStringList(), env,
                 KeyValuePairList({KeyValuePair("PE.Profile.Data/extraData", QVariant("extraValue"))}));
    if (map.count() != 6
            || !map.contains(VERSION) || map.value(VERSION).toInt() != 1
            || !map.contains(COUNT) || map.value(COUNT).toInt() != 3
            || !map.contains(DEFAULT) || map.value(DEFAULT).toInt() != 0
            || !map.contains("Profile.0")
            || !map.contains("Profile.1")
            || !map.contains("Profile.2"))

    if (map.value("Profile.0") != profile0)
        return false;
    if (map.value("Profile.1") != profile1)
        return false;

    QVariantMap profile2 = map.value("Profile.2").toMap();
    if (profile2.count() != 6
            || !profile2.contains(ID) || profile2.value(ID).toString() != "test with debugger Id"
            || !profile2.contains(DISPLAYNAME) || profile2.value(DISPLAYNAME).toString() != "Test debugger Id"
            || !profile2.contains(ICON) || profile2.value(ICON).toString() != "/tmp/icon2.png"
            || !profile2.contains(DATA) || profile2.value(DATA).type() != QVariant::Map
            || !profile2.contains(AUTODETECTED) || profile2.value(AUTODETECTED).toBool() != true
            || !profile2.contains(SDK) || profile2.value(SDK).toBool() != true)
        return false;

    data = profile2.value(DATA).toMap();
    if (data.count() != 7
            || !data.contains(DEBUGGER) || data.value(DEBUGGER).toString() != "debugger Id")
        return false;

    return true;
}
#endif

QVariantMap AddKitOperation::addKit(const QVariantMap &map,
                                    const QString &id, const QString &displayName,
                                    const QString &icon, const QString &debuggerId,
                                    const quint32 &debuggerType, const QString &debugger,
                                    const QString &deviceType, const QString &device,
                                    const QString &sysRoot, const QHash<QString, QString> &tcs, const QString &qt,
                                    const QString &mkspec, const QString &cmakeId,
                                    const QString &cmakeGenerator, const QString &cmakeExtraGenerator,
                                    const QString &cmakeGeneratorToolset,
                                    const QString &cmakeGeneratorPlatform,
                                    const QStringList &cmakeConfiguration, const QStringList &env,
                                    const KeyValuePairList &extra)
{
    QVariantMap tcMap = load("ToolChains");
    QVariantMap qtMap = load("QtVersions");
    QVariantMap devMap = load("Devices");
    QVariantMap cmakeMap = load("cmaketools");

    return addKit(map, tcMap, qtMap, devMap, cmakeMap, id, displayName, icon, debuggerId, debuggerType,
                  debugger, deviceType, device, sysRoot, tcs, qt, mkspec,
                  cmakeId, cmakeGenerator, cmakeExtraGenerator, cmakeGeneratorToolset,
                  cmakeGeneratorPlatform, cmakeConfiguration,
                  env, extra);
}

QVariantMap AddKitOperation::addKit(const QVariantMap &map, const QVariantMap &tcMap,
                                    const QVariantMap &qtMap, const QVariantMap &devMap,
                                    const QVariantMap &cmakeMap,
                                    const QString &id, const QString &displayName,
                                    const QString &icon, const QString &debuggerId,
                                    const quint32 &debuggerType, const QString &debugger,
                                    const QString &deviceType, const QString &device,
                                    const QString &sysRoot, const QHash<QString, QString> &tcs, const QString &qt,
                                    const QString &mkspec, const QString &cmakeId,
                                    const QString &cmakeGenerator, const QString &cmakeExtraGenerator,
                                    const QString &cmakeGeneratorToolset,
                                    const QString &cmakeGeneratorPlatform,
                                    const QStringList &cmakeConfiguration, const QStringList &env,
                                    const KeyValuePairList &extra)
{
    // Sanity check: Make sure autodetection source is not in use already:
    QStringList valueKeys = FindValueOperation::findValue(map, QVariant(id));
    bool hasId = false;
    foreach (const QString &k, valueKeys) {
        if (k.endsWith(QString('/') + ID)) {
            hasId = true;
            break;
        }
    }
    if (hasId) {
        std::cerr << "Error: Id " << qPrintable(id) << " already defined as kit." << std::endl;
        return QVariantMap();
    }

    for (auto i = tcs.constBegin(); i != tcs.constEnd(); ++i) {
        if (!i.value().isEmpty() && !AddToolChainOperation::exists(tcMap, i.value())) {
            QRegExp abiRegExp = QRegExp("[a-z0-9_]+-[a-z0-9_]+-[a-z0-9_]+-[a-z0-9_]+-(8|16|32|64|128)bit");
            if (!abiRegExp.exactMatch(i.value())) {
                std::cerr << "Error: Toolchain " << qPrintable(i.value())
                          << " for language " << qPrintable(i.key()) << " does not exist." << std::endl;
                return QVariantMap();
            }
        }
    }

    QString qtId = qt;
    if (!qtId.isEmpty() && !qtId.startsWith("SDK."))
        qtId = QString::fromLatin1("SDK.") + qt;
    if (!qtId.isEmpty() && !AddQtOperation::exists(qtMap, qtId)) {
        std::cerr << "Error: Qt " << qPrintable(qtId) << " does not exist." << std::endl;
        return QVariantMap();
    }
    if (!device.isEmpty() && !AddDeviceOperation::exists(devMap, device)) {
        std::cerr << "Error: Device " << qPrintable(device) << " does not exist." << std::endl;
        return QVariantMap();
    }
    if (!cmakeId.isEmpty() && !AddCMakeOperation::exists(cmakeMap, cmakeId)) {
        std::cerr << "Error: CMake tool " << qPrintable(cmakeId) << " does not exist." << std::endl;
        return QVariantMap();
    }

    // Find position to insert:
    bool ok;
    int count = GetOperation::get(map, COUNT).toInt(&ok);
    if (!ok || count < 0) {
        std::cerr << "Error: Count found in kits file seems wrong." << std::endl;
        return QVariantMap();
    }
    const QString kit = QString::fromLatin1(PREFIX) + QString::number(count);

    QString defaultKit = GetOperation::get(map, DEFAULT).toString();
    if (defaultKit.isEmpty())
        defaultKit = id;

    // remove data:
    QVariantMap cleaned = RmKeysOperation::rmKeys(map, {COUNT, DEFAULT});

    // Sanity check: Make sure displayName is unique.
    QStringList nameKeys = FindKeyOperation::findKey(map, DISPLAYNAME);
    QStringList nameList;
    foreach (const QString &nameKey, nameKeys)
        nameList << GetOperation::get(map, nameKey).toString();
    const QString uniqueName = makeUnique(displayName, nameList);

    // insert data:
    KeyValuePairList data = { KeyValuePair({kit, ID}, QVariant(id)),
                              KeyValuePair({kit, DISPLAYNAME}, QVariant(uniqueName)),
                              KeyValuePair({kit, ICON}, QVariant(icon)),
                              KeyValuePair({kit, AUTODETECTED}, QVariant(true)),
                              KeyValuePair({kit, SDK}, QVariant(true))};

    if (!debuggerId.isEmpty() || !debugger.isEmpty()) {
        if (debuggerId.isEmpty()) {
            data << KeyValuePair({kit, DATA, DEBUGGER, DEBUGGER_ENGINE}, QVariant(debuggerType));
            data << KeyValuePair({kit, DATA, DEBUGGER, DEBUGGER_BINARY}, QVariant(debugger));
        } else {
            data << KeyValuePair({kit, DATA, DEBUGGER }, QVariant(debuggerId));
        }
    }
    if (!deviceType.isNull())
        data << KeyValuePair({kit, DATA, DEVICE_TYPE}, QVariant(deviceType));
    if (!device.isNull())
        data << KeyValuePair({kit, DATA, DEVICE_ID}, QVariant(device));
    if (!sysRoot.isNull())
        data << KeyValuePair({kit, DATA, SYSROOT}, QVariant(sysRoot));
    for (auto i = tcs.constBegin(); i != tcs.constEnd(); ++i)
        data << KeyValuePair({kit, DATA, TOOLCHAIN, i.key()}, QVariant(i.value()));
    if (!qtId.isNull())
        data << KeyValuePair({kit, DATA, QT}, QVariant(qtId));
    if (!mkspec.isNull())
        data << KeyValuePair({kit, DATA, MKSPEC}, QVariant(mkspec));
    if (!cmakeId.isNull())
        data << KeyValuePair({kit, DATA, CMAKE_ID}, QVariant(cmakeId));
    if (!cmakeGenerator.isNull()) {
        QVariantMap generatorMap;
        generatorMap.insert("Generator", cmakeGenerator);
        if (!cmakeExtraGenerator.isNull())
            generatorMap.insert("ExtraGenerator", cmakeExtraGenerator);
        if (!cmakeGeneratorToolset.isNull())
            generatorMap.insert("Toolset", cmakeGeneratorToolset);
        if (!cmakeGeneratorPlatform.isNull())
            generatorMap.insert("Platform", cmakeGeneratorPlatform);
        data << KeyValuePair({kit, DATA, CMAKE_GENERATOR}, generatorMap);
    }
    if (!cmakeConfiguration.isEmpty())
        data << KeyValuePair({kit, DATA, CMAKE_CONFIGURATION}, QVariant(cmakeConfiguration));
    if (!env.isEmpty())
        data << KeyValuePair({kit, DATA, ENV}, QVariant(env));

    data << KeyValuePair(DEFAULT, QVariant(defaultKit));
    data << KeyValuePair(COUNT, QVariant(count + 1));

    KeyValuePairList qtExtraList;
    foreach (const KeyValuePair &pair, extra)
        qtExtraList << KeyValuePair(QStringList() << kit << pair.key, pair.value);
    data.append(qtExtraList);

    return AddKeysOperation::addKeys(cleaned, data);
}

QVariantMap AddKitOperation::initializeKits()
{
    QVariantMap map;
    map.insert(VERSION, 1);
    map.insert(DEFAULT, QString());
    map.insert(COUNT, 0);
    return map;
}
