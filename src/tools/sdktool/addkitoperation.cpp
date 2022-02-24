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

#include <QRegularExpression>

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

// Standard KitAspects:
const char DEBUGGER[] = "Debugger.Information";
const char DEBUGGER_ENGINE[] = "EngineType";
const char DEBUGGER_BINARY[] = "Binary";
const char DEVICE_TYPE[] = "PE.Profile.DeviceType";
const char DEVICE_ID[] = "PE.Profile.Device";
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
        std::cerr << "No id given for kit." << std::endl << std::endl;
    if (m_displayName.isEmpty())
        std::cerr << "No name given for kit." << std::endl << std::endl;
    if (m_deviceType.isEmpty())
        std::cerr << "No devicetype given for kit." << std::endl << std::endl;
    if (!m_debuggerId.isEmpty() && (!m_debugger.isEmpty() || m_debuggerEngine != 0)) {
        std::cerr << "Cannot set both debugger id and debugger/debuggerengine." << std::endl << std::endl;
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
bool AddKitOperation::test() const
{
    AddKitData baseData;
    baseData.m_id ="testId";
    baseData.m_displayName = "Test Kit";
    baseData.m_icon = "/tmp/icon.png";
    baseData.m_debuggerEngine = 1;
    baseData.m_debugger = "/usr/bin/gdb-test";
    baseData.m_deviceType = "Desktop";
    baseData.m_device =  "{dev-id}";
    baseData.m_qt = "{qt-id}";
    baseData.m_mkspec ="unsupported/mkspec";
    baseData.m_extra = {{"PE.Profile.Data/extraData", QVariant("extraValue")}};

    QVariantMap map = initializeKits();

    QVariantMap tcMap = AddToolChainData::initializeToolChains();
    tcMap = AddToolChainData{"{tc-id}", "langId", "TC", "/usr/bin/gcc",
                         "x86-linux-generic-elf-32bit", "x86-linux-generic-elf-32bit", {}}
            .addToolChain(tcMap);

    QVariantMap qtMap = AddQtData::initializeQtVersions();
    qtMap = AddQtData{"{qt-id}", "Qt", "desktop-qt", "/usr/bin/qmake", {}, {}}.addQt(qtMap);

    QVariantMap devMap = AddDeviceOperation::initializeDevices();
    devMap = AddDeviceData{"{dev-id}", "Dev", 0, 0,
                           "HWplatform", "SWplatform",
                           "localhost", "10000-11000",
                           "localhost", "", 42,
                           "desktop", "", 22, 10000,
                           "uname", 1,
                           KeyValuePairList()}
            .addDevice(devMap);

    const QStringList env = {"TEST=1", "PATH"};

    if (map.count() != 3)
        return false;
    if (!map.contains(VERSION))
        return false;
    if (map.value(VERSION).toInt() != 1)
        return false;
    if (!map.contains(COUNT))
        return false;
    if (map.value(COUNT).toInt() != 0)
        return false;
    if (!map.contains(DEFAULT))
        return false;
    if (!map.value(DEFAULT).toString().isEmpty())
        return false;

    QHash<QString, QString> tcs;
    tcs.insert("Cxx", "{tcXX-id}");

    // Fail if TC is not there:
    AddKitData kitData = baseData;
    kitData.m_tcs = tcs;
    QVariantMap empty = kitData.addKit(map, tcMap, qtMap, devMap, {});

    if (!empty.isEmpty())
        return false;
    // Do not fail if TC is an ABI:
    tcs.clear();
    tcs.insert("C", "x86-linux-generic-elf-64bit");
    kitData = baseData;
    kitData.m_tcs = tcs;
    empty = kitData.addKit(map, tcMap, qtMap, devMap, {});
    if (empty.isEmpty())
        return false;

    // QTCREATORBUG-11983, mach_o was not covered by the first attempt to fix this.
    tcs.insert("D", "x86-macos-generic-mach_o-64bit");
    kitData = baseData;
    kitData.m_tcs = tcs;
    empty = kitData.addKit(map, tcMap, qtMap, devMap, {});
    if (empty.isEmpty())
        return false;

    tcs.clear();
    tcs.insert("Cxx", "{tc-id}");

    // Fail if Qt is not there:
    kitData = baseData;
    kitData.m_qt = "{qtXX-id}";
    empty = kitData.addKit(map, tcMap, qtMap, devMap, {});
    if (!empty.isEmpty())
        return false;

    // Fail if dev is not there:
    kitData = baseData;
    kitData.m_device = "{devXX-id}";
    empty = kitData.addKit(map, tcMap, qtMap, devMap, {});
    if (!empty.isEmpty())
        return false;

    // Profile 0:
    kitData = baseData;
    kitData.m_tcs = tcs;
    map = kitData.addKit(map, tcMap, qtMap, devMap, {});

    if (map.count() != 4)
        return false;
    if (!map.contains(VERSION))
        return false;
    if (map.value(VERSION).toInt() != 1)
        return false;
    if (!map.contains(COUNT))
        return false;
    if (map.value(COUNT).toInt() != 1)
        return false;
    if (!map.contains(DEFAULT))
        return false;
    if (map.value(DEFAULT).toString() != "testId")
        return false;
    if (!map.contains("Profile.0"))
        return false;

    QVariantMap profile0 = map.value("Profile.0").toMap();
    if (profile0.count() != 6)
        return false;

    if (!profile0.contains(ID))
        return false;
    if (profile0.value(ID).toString() != "testId")
        return false;
    if (!profile0.contains(DISPLAYNAME))
        return false;
    if (profile0.value(DISPLAYNAME).toString() != "Test Kit")
        return false;
    if (!profile0.contains(ICON))
        return false;
    if (profile0.value(ICON).toString() != "/tmp/icon.png")
        return false;
    if (!profile0.contains(DATA))
        return false;
    if (profile0.value(DATA).type() != QVariant::Map)
        return false;
    if (!profile0.contains(AUTODETECTED))
        return false;
    if (profile0.value(AUTODETECTED).toBool() != true)
        return false;
    if (!profile0.contains(SDK))
        return false;
    if (profile0.value(SDK).toBool() != true)
        return false;

    QVariantMap data = profile0.value(DATA).toMap();
    if (data.count() != 7)
        return false;
    if (!data.contains(DEBUGGER))
        return false;
    if (data.value(DEBUGGER).type() != QVariant::Map)
        return false;
    if (!data.contains(DEVICE_TYPE))
        return false;
    if (data.value(DEVICE_TYPE).toString() != "Desktop")
        return false;
    if (!data.contains(TOOLCHAIN))
        return false;
    if (!data.contains(QT))
        return false;
    if (data.value(QT).toString() != "SDK.{qt-id}")
        return false;
    if (!data.contains(MKSPEC))
        return false;
    if (data.value(MKSPEC).toString() != "unsupported/mkspec")
        return false;
    if (!data.contains("extraData"))
        return false;
    if (data.value("extraData").toString() != "extraValue")
        return false;

    QVariantMap tcOutput = data.value(TOOLCHAIN).toMap();
    if (tcOutput.count() != 1)
        return false;
    if (!tcOutput.contains("Cxx"))
        return false;
    if (tcOutput.value("Cxx") != "{tc-id}")
        return false;

    // Ignore exist ids:
    kitData = baseData;
    kitData.m_displayName = "Test Qt Version X";
    kitData.m_icon = "/tmp/icon3.png";
    kitData.m_debugger = "/usr/bin/gdb-test3";
    kitData.m_tcs = tcs;
    QVariantMap result = kitData.addKit(map, tcMap, qtMap, devMap, {});
    if (!result.isEmpty())
        return false;

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

    if (map.count() != 5)
        return false;
    if (!map.contains(VERSION) )
        return false;
    if (map.value(VERSION).toInt() != 1)
        return false;
    if (!map.contains(COUNT) )
        return false;
    if (map.value(COUNT).toInt() != 2)
        return false;
    if (!map.contains(DEFAULT) )
        return false;
    if (map.value(DEFAULT).toInt() != 0)
        return false;
    if (!map.contains("Profile.0"))
        return false;
    if (!map.contains("Profile.1"))
        return false;
    if (map.value("Profile.0") != profile0)
        return false;

    QVariantMap profile1 = map.value("Profile.1").toMap();
    if (profile1.count() != 6)
        return false;
    if (!profile1.contains(ID) )
        return false;
    if (profile1.value(ID).toString() != "testId2")
        return false;
    if (!profile1.contains(DISPLAYNAME) )
        return false;
    if (profile1.value(DISPLAYNAME).toString() != "Test Kit2")
        return false;
    if (!profile1.contains(ICON) )
        return false;
    if (profile1.value(ICON).toString() != "/tmp/icon2.png")
        return false;
    if (!profile1.contains(DATA) )
        return false;
    if (profile1.value(DATA).type() != QVariant::Map)
        return false;
    if (!profile1.contains(AUTODETECTED) )
        return false;
    if (profile1.value(AUTODETECTED).toBool() != true)
        return false;
    if (!profile1.contains(SDK) )
        return false;
    if (profile1.value(SDK).toBool() != true)
        return false;

    data = profile1.value(DATA).toMap();
    if (data.count() != 9)
        return false;
    if (!data.contains(DEBUGGER) )
        return false;
    if (data.value(DEBUGGER).type() != QVariant::Map)
        return false;
    if (!data.contains(DEVICE_TYPE) )
        return false;
    if (data.value(DEVICE_TYPE).toString() != "Desktop")
        return false;
    if (!data.contains(DEVICE_ID) )
        return false;
    if (data.value(DEVICE_ID).toString() != "{dev-id}")
        return false;
    if (!data.contains(SYSROOT) )
        return false;
    if (data.value(SYSROOT).toString() != "/sys/root//")
        return false;
    if (!data.contains(TOOLCHAIN))
        return false;
    if (!data.contains(QT) )
        return false;
    if (data.value(QT).toString() != "SDK.{qt-id}")
        return false;
    if (!data.contains(MKSPEC) )
        return false;
    if (data.value(MKSPEC).toString() != "unsupported/mkspec")
        return false;
    if (!data.contains(ENV) )
        return false;
    if (data.value(ENV).toStringList() != env)
        return false;
    if (!data.contains("extraData") )
        return false;
    if (data.value("extraData").toString() != "extraValue")
        return false;

    tcOutput = data.value(TOOLCHAIN).toMap();
    if (tcOutput.count() != 1)
        return false;
    if (!tcOutput.contains("Cxx") )
        return false;
    if (tcOutput.value("Cxx") != "{tc-id}")
        return false;

    // Profile 2: Test debugger id:
    kitData = baseData;
    kitData.m_id = "test with debugger Id";
    kitData.m_displayName = "Test debugger Id";
    kitData.m_icon = "/tmp/icon2.png";
    kitData.m_debuggerId = "debugger Id";
    kitData.m_env = env;

    map = kitData.addKit(map, tcMap, qtMap, devMap, {});
    if (map.count() != 6)
        return false;
    if (!map.contains(VERSION) )
        return false;
    if (map.value(VERSION).toInt() != 1)
        return false;
    if (!map.contains(COUNT) )
        return false;
    if (map.value(COUNT).toInt() != 3)
        return false;
    if (!map.contains(DEFAULT) )
        return false;
    if (map.value(DEFAULT).toInt() != 0)
        return false;
    if (!map.contains("Profile.0"))
        return false;
    if (!map.contains("Profile.1"))
        return false;
    if (!map.contains("Profile.2"))
        return false;
    if (map.value("Profile.0") != profile0)
        return false;
    if (map.value("Profile.1") != profile1)
        return false;

    QVariantMap profile2 = map.value("Profile.2").toMap();
    if (profile2.count() != 6)
        return false;
    if (!profile2.contains(ID) )
        return false;
    if (profile2.value(ID).toString() != "test with debugger Id")
        return false;
    if (!profile2.contains(DISPLAYNAME) )
        return false;
    if (profile2.value(DISPLAYNAME).toString() != "Test debugger Id")
        return false;
    if (!profile2.contains(ICON) )
        return false;
    if (profile2.value(ICON).toString() != "/tmp/icon2.png")
        return false;
    if (!profile2.contains(DATA) )
        return false;
    if (profile2.value(DATA).type() != QVariant::Map)
        return false;
    if (!profile2.contains(AUTODETECTED) )
        return false;
    if (profile2.value(AUTODETECTED).toBool() != true)
        return false;
    if (!profile2.contains(SDK) )
        return false;
    if (profile2.value(SDK).toBool() != true)
        return false;

    data = profile2.value(DATA).toMap();
    if (data.count() != 7)
        return false;
    if (!data.contains(DEBUGGER))
        return false;
    if (data.value(DEBUGGER).toString() != "debugger Id")
        return false;

    return true;
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

QVariantMap AddKitData::addKit(const QVariantMap &map, const QVariantMap &tcMap,
                                    const QVariantMap &qtMap, const QVariantMap &devMap,
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
        std::cerr << "Error: Id " << qPrintable(m_id) << " already defined as kit." << std::endl;
        return QVariantMap();
    }

    for (auto i = m_tcs.constBegin(); i != m_tcs.constEnd(); ++i) {
        if (!i.value().isEmpty() && !AddToolChainOperation::exists(tcMap, i.value())) {
            const QRegularExpression abiRegExp("^[a-z0-9_]+-[a-z0-9_]+-[a-z0-9_]+-[a-z0-9_]+-(8|16|32|64|128)bit$");
            if (!abiRegExp.match(i.value()).hasMatch()) {
                std::cerr << "Error: Toolchain " << qPrintable(i.value())
                          << " for language " << qPrintable(i.key()) << " does not exist." << std::endl;
                return QVariantMap();
            }
        }
    }

    QString qtId = m_qt;
    if (!qtId.isEmpty() && !qtId.startsWith("SDK."))
        qtId = QString::fromLatin1("SDK.") + m_qt;
    if (!qtId.isEmpty() && !AddQtData::exists(qtMap, qtId)) {
        std::cerr << "Error: Qt " << qPrintable(qtId) << " does not exist." << std::endl;
        return QVariantMap();
    }
    if (!m_device.isEmpty() && !AddDeviceOperation::exists(devMap, m_device)) {
        std::cerr << "Error: Device " << qPrintable(m_device) << " does not exist." << std::endl;
        return QVariantMap();
    }

    // Treat a qt that was explicitly set to '' as "no Qt"
    if (!qtId.isNull() && qtId.isEmpty())
        qtId = "-1";

    if (!m_cmakeId.isEmpty() && !AddCMakeData::exists(cmakeMap, m_cmakeId)) {
        std::cerr << "Error: CMake tool " << qPrintable(m_cmakeId) << " does not exist." << std::endl;
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
            data << KeyValuePair({kit, DATA, DEBUGGER }, QVariant(m_debuggerId));
        }
    }

    if (!m_deviceType.isNull())
        data << KeyValuePair({kit, DATA, DEVICE_TYPE}, QVariant(m_deviceType));
    if (!m_device.isNull())
        data << KeyValuePair({kit, DATA, DEVICE_ID}, QVariant(m_device));
    if (!m_sysRoot.isNull())
        data << KeyValuePair({kit, DATA, SYSROOT}, Utils::FilePath::fromUserInput(m_sysRoot).toVariant());
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
    foreach (const KeyValuePair &pair, m_extra)
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
