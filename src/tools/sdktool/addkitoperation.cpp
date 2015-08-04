/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "addkitoperation.h"

#include "addkeysoperation.h"
#include "addtoolchainoperation.h"
#include "addqtoperation.h"
#include "adddeviceoperation.h"
#include "findkeyoperation.h"
#include "findvalueoperation.h"
#include "getoperation.h"
#include "rmkeysoperation.h"

#include "settings.h"

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
const char TOOLCHAIN[] = "PE.Profile.ToolChain";
const char MKSPEC[] = "QtPM4.mkSpecInformation";
const char QT[] = "QtSupport.QtInformation";

AddKitOperation::AddKitOperation()
    : m_debuggerEngine(0)
{
}

QString AddKitOperation::name() const
{
    return QLatin1String("addKit");
}

QString AddKitOperation::helpText() const
{
    return QLatin1String("add a Kit to Qt Creator");
}

QString AddKitOperation::argumentsHelpText() const
{
    return QLatin1String("    --id <ID>                                  id of the new kit (required).\n"
                         "    --name <NAME>                              display name of the new kit (required).\n"
                         "    --icon <PATH>                              icon of the new kit.\n"
                         "    --debuggerid <ID>                          the id of the debugger to use.\n"
                         "                                               (not compatible with --debugger and --debuggerengine)\n"
                         "    --debuggerengine <ENGINE>                  debuggerengine of the new kit.\n"
                         "    --debugger <PATH>                          debugger of the new kit.\n"
                         "    --devicetype <TYPE>                        device type of the new kit (required).\n"
                         "    --device <ID>                              device id to use (optional).\n"
                         "    --sysroot <PATH>                           sysroot of the new kit.\n"
                         "    --toolchain <ID>                           tool chain of the new kit.\n"
                         "    --qt <ID>                                  Qt of the new kit.\n"
                         "    --mkspec <PATH>                            mkspec of the new kit.\n"
                         "    --env <VALUE>                              add a custom environment setting. [may be repeated]\n"
                         "    <KEY> <TYPE:VALUE>                         extra key value pairs\n");
}

bool AddKitOperation::setArguments(const QStringList &args)
{
    m_debuggerEngine = 0;

    for (int i = 0; i < args.count(); ++i) {
        const QString current = args.at(i);
        const QString next = ((i + 1) < args.count()) ? args.at(i + 1) : QString();

        if (current == QLatin1String("--id")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_id = next;
            continue;
        }

        if (current == QLatin1String("--name")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_displayName = next;
            continue;
        }

        if (current == QLatin1String("--icon")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_icon = next;
            continue;
        }

        if (current == QLatin1String("--debuggerengine")) {
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

        if (current == QLatin1String("--debuggerid")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_debuggerId = next;
            continue;
        }

        if (current == QLatin1String("--debugger")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_debugger = next;
            continue;
        }

        if (current == QLatin1String("--devicetype")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_deviceType = next;
            continue;
        }

        if (current == QLatin1String("--device")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_device = next;
            continue;
        }

        if (current == QLatin1String("--sysroot")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_sysRoot = next;
            continue;
        }

        if (current == QLatin1String("--toolchain")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_tc = next;
            continue;
        }

        if (current == QLatin1String("--qt")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_qt = next;
            continue;
        }

        if (current == QLatin1String("--mkspec")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_mkspec = next;
            continue;
        }

        if (current == QLatin1String("--env")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_env.append(next);
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

    if (m_icon.isEmpty())
        m_icon = QLatin1String(":///DESKTOP///");

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
    QVariantMap map = load(QLatin1String("Profiles"));
    if (map.isEmpty())
        map = initializeKits();

    QVariantMap result = addKit(map, m_id, m_displayName, m_icon, m_debuggerId, m_debuggerEngine,
                                m_debugger, m_deviceType, m_device, m_sysRoot, m_tc, m_qt,
                                m_mkspec, m_env, m_extra);

    if (result.isEmpty() || map == result)
        return 2;

    return save(result, QLatin1String("Profiles")) ? 0 : 3;
}

#ifdef WITH_TESTS
bool AddKitOperation::test() const
{
    QVariantMap map = initializeKits();

    QVariantMap tcMap = AddToolChainOperation::initializeToolChains();
    tcMap = AddToolChainOperation::addToolChain(tcMap, QLatin1String("{tc-id}"), QLatin1String("TC"),
                                                QLatin1String("/usr/bin/gcc"),
                                                QLatin1String("x86-linux-generic-elf-32bit"),
                                                QLatin1String("x86-linux-generic-elf-32bit"),
                                                KeyValuePairList());

    QVariantMap qtMap = AddQtOperation::initializeQtVersions();
    qtMap = AddQtOperation::addQt(qtMap, QLatin1String("{qt-id}"), QLatin1String("Qt"),
                                  QLatin1String("desktop-qt"), QLatin1String("/usr/bin/qmake"),
                                  KeyValuePairList());

    QVariantMap devMap = AddDeviceOperation::initializeDevices();
    devMap = AddDeviceOperation::addDevice(devMap, QLatin1String("{dev-id}"), QLatin1String("Dev"), 0, 0,
                                           QLatin1String("HWplatform"), QLatin1String("SWplatform"),
                                           QLatin1String("localhost"), QLatin1String("10000-11000"),
                                           QLatin1String("localhost"), QLatin1String(""), 42,
                                           QLatin1String("desktop"), QLatin1String(""), 22, 10000,
                                           QLatin1String("uname"), 1,
                                           KeyValuePairList());

    QStringList env;
    env << QLatin1String("TEST=1") << QLatin1String("PATH");

    if (map.count() != 3
            || !map.contains(QLatin1String(VERSION))
            || map.value(QLatin1String(VERSION)).toInt() != 1
            || !map.contains(QLatin1String(COUNT))
            || map.value(QLatin1String(COUNT)).toInt() != 0
            || !map.contains(QLatin1String(DEFAULT))
            || !map.value(QLatin1String(DEFAULT)).toString().isEmpty())
        return false;

    // Fail if TC is not there:
    QVariantMap empty = addKit(map, tcMap, qtMap, devMap,
                               QLatin1String("testId"), QLatin1String("Test Kit"), QLatin1String("/tmp/icon.png"),
                               QString(), 1, QLatin1String("/usr/bin/gdb-test"),
                               QLatin1String("Desktop"), QLatin1String("{dev-id}"), QString(),
                               QLatin1String("{tcXX-id}"), QLatin1String("{qt-id}"), QLatin1String("unsupported/mkspec"),
                               QStringList(),
                               KeyValuePairList() << KeyValuePair(QLatin1String("PE.Profile.Data/extraData"), QVariant(QLatin1String("extraValue"))));
    if (!empty.isEmpty())
        return false;
    // Do not fail if TC is an ABI:
    empty = addKit(map, tcMap, qtMap, devMap,
                   QLatin1String("testId"), QLatin1String("Test Kit"), QLatin1String("/tmp/icon.png"),
                   QString(), 1, QLatin1String("/usr/bin/gdb-test"),
                   QLatin1String("Desktop"), QLatin1String("{dev-id}"), QString(),
                   QLatin1String("x86-linux-generic-elf-64bit"), QLatin1String("{qt-id}"), QLatin1String("unsupported/mkspec"),
                   env,
                   KeyValuePairList() << KeyValuePair(QLatin1String("PE.Profile.Data/extraData"), QVariant(QLatin1String("extraValue"))));
    if (empty.isEmpty())
        return false;
    // QTCREATORBUG-11983, mach_o was not covered by the first attempt to fix this.
    empty = addKit(map, tcMap, qtMap, devMap,
                   QLatin1String("testId"), QLatin1String("Test Kit"), QLatin1String("/tmp/icon.png"),
                   QString(), 1, QLatin1String("/usr/bin/gdb-test"),
                   QLatin1String("Desktop"), QLatin1String("{dev-id}"), QString(),
                   QLatin1String("x86-macos-generic-mach_o-64bit"), QLatin1String("{qt-id}"), QLatin1String("unsupported/mkspec"),
                   env,
                   KeyValuePairList() << KeyValuePair(QLatin1String("PE.Profile.Data/extraData"), QVariant(QLatin1String("extraValue"))));
    if (empty.isEmpty())
        return false;

    // Fail if Qt is not there:
    empty = addKit(map, tcMap, qtMap, devMap,
                   QLatin1String("testId"), QLatin1String("Test Kit"), QLatin1String("/tmp/icon.png"),
                   QString(), 1, QLatin1String("/usr/bin/gdb-test"),
                   QLatin1String("Desktop"), QLatin1String("{dev-id}"), QString(),
                   QLatin1String("{tc-id}"), QLatin1String("{qtXX-id}"), QLatin1String("unsupported/mkspec"),
                   env,
                   KeyValuePairList() << KeyValuePair(QLatin1String("PE.Profile.Data/extraData"), QVariant(QLatin1String("extraValue"))));
    if (!empty.isEmpty())
        return false;
    // Fail if dev is not there:
    empty = addKit(map, tcMap, qtMap, devMap,
                   QLatin1String("testId"), QLatin1String("Test Kit"), QLatin1String("/tmp/icon.png"),
                   QString(), 1, QLatin1String("/usr/bin/gdb-test"),
                   QLatin1String("Desktop"), QLatin1String("{devXX-id}"), QString(),
                   QLatin1String("{tc-id}"), QLatin1String("{qt-id}"), QLatin1String("unsupported/mkspec"),
                   env,
                   KeyValuePairList() << KeyValuePair(QLatin1String("PE.Profile.Data/extraData"), QVariant(QLatin1String("extraValue"))));
    if (!empty.isEmpty())
        return false;

    // Profile 0:
    map = addKit(map, tcMap, qtMap, devMap,
                 QLatin1String("testId"), QLatin1String("Test Kit"), QLatin1String("/tmp/icon.png"),
                 QString(), 1, QLatin1String("/usr/bin/gdb-test"),
                 QLatin1String("Desktop"), QString(), QString(),
                 QLatin1String("{tc-id}"), QLatin1String("{qt-id}"), QLatin1String("unsupported/mkspec"),
                 env,
                 KeyValuePairList() << KeyValuePair(QLatin1String("PE.Profile.Data/extraData"), QVariant(QLatin1String("extraValue"))));

    if (map.count() != 4
            || !map.contains(QLatin1String(VERSION))
            || map.value(QLatin1String(VERSION)).toInt() != 1
            || !map.contains(QLatin1String(COUNT))
            || map.value(QLatin1String(COUNT)).toInt() != 1
            || !map.contains(QLatin1String(DEFAULT))
            || map.value(QLatin1String(DEFAULT)).toString() != QLatin1String("testId")
            || !map.contains(QLatin1String("Profile.0")))
        return false;

    QVariantMap profile0 = map.value(QLatin1String("Profile.0")).toMap();
    if (profile0.count() != 6
            || !profile0.contains(QLatin1String(ID))
            || profile0.value(QLatin1String(ID)).toString() != QLatin1String("testId")
            || !profile0.contains(QLatin1String(DISPLAYNAME))
            || profile0.value(QLatin1String(DISPLAYNAME)).toString() != QLatin1String("Test Kit")
            || !profile0.contains(QLatin1String(ICON))
            || profile0.value(QLatin1String(ICON)).toString() != QLatin1String("/tmp/icon.png")
            || !profile0.contains(QLatin1String(DATA))
            || profile0.value(QLatin1String(DATA)).type() != QVariant::Map
            || !profile0.contains(QLatin1String(AUTODETECTED))
            || profile0.value(QLatin1String(AUTODETECTED)).toBool() != true
            || !profile0.contains(QLatin1String(SDK))
            || profile0.value(QLatin1String(SDK)).toBool() != true)
        return false;

    QVariantMap data = profile0.value(QLatin1String(DATA)).toMap();
    if (data.count() != 7
            || !data.contains(QLatin1String(DEBUGGER))
            || data.value(QLatin1String(DEBUGGER)).type() != QVariant::Map
            || !data.contains(QLatin1String(DEVICE_TYPE))
            || data.value(QLatin1String(DEVICE_TYPE)).toString() != QLatin1String("Desktop")
            || !data.contains(QLatin1String(TOOLCHAIN))
            || data.value(QLatin1String(TOOLCHAIN)).toString() != QLatin1String("{tc-id}")
            || !data.contains(QLatin1String(QT))
            || data.value(QLatin1String(QT)).toString() != QLatin1String("SDK.{qt-id}")
            || !data.contains(QLatin1String(MKSPEC))
            || data.value(QLatin1String(MKSPEC)).toString() != QLatin1String("unsupported/mkspec")
            || !data.contains(QLatin1String("extraData"))
            || data.value(QLatin1String("extraData")).toString() != QLatin1String("extraValue"))
        return false;

    // Ignore existing ids:
    QVariantMap result = addKit(map, tcMap, qtMap, devMap,
                                QLatin1String("testId"), QLatin1String("Test Qt Version X"), QLatin1String("/tmp/icon3.png"),
                                QString(), 1, QLatin1String("/usr/bin/gdb-test3"),
                                QLatin1String("Desktop"), QString(), QString(),
                                QLatin1String("{tc-id}"), QLatin1String("{qt-id}"), QLatin1String("unsupported/mkspec"),
                                env,
                                KeyValuePairList() << KeyValuePair(QLatin1String("PE.Profile.Data/extraData"), QVariant(QLatin1String("extraValue"))));
    if (!result.isEmpty())
        return false;

    // Profile 1: Make sure name is unique:
    map = addKit(map, tcMap, qtMap, devMap,
                 QLatin1String("testId2"), QLatin1String("Test Kit2"), QLatin1String("/tmp/icon2.png"),
                 QString(), 1, QLatin1String("/usr/bin/gdb-test2"),
                 QLatin1String("Desktop"), QLatin1String("{dev-id}"), QLatin1String("/sys/root\\\\"),
                 QLatin1String("{tc-id}"), QLatin1String("{qt-id}"), QLatin1String("unsupported/mkspec"),
                 env,
                 KeyValuePairList() << KeyValuePair(QLatin1String("PE.Profile.Data/extraData"), QVariant(QLatin1String("extraValue"))));
    if (map.count() != 5
            || !map.contains(QLatin1String(VERSION))
            || map.value(QLatin1String(VERSION)).toInt() != 1
            || !map.contains(QLatin1String(COUNT))
            || map.value(QLatin1String(COUNT)).toInt() != 2
            || !map.contains(QLatin1String(DEFAULT))
            || map.value(QLatin1String(DEFAULT)).toInt() != 0
            || !map.contains(QLatin1String("Profile.0"))
            || !map.contains(QLatin1String("Profile.1")))

    if (map.value(QLatin1String("Profile.0")) != profile0)
        return false;

    QVariantMap profile1 = map.value(QLatin1String("Profile.1")).toMap();
    if (profile1.count() != 6
            || !profile1.contains(QLatin1String(ID))
            || profile1.value(QLatin1String(ID)).toString() != QLatin1String("testId2")
            || !profile1.contains(QLatin1String(DISPLAYNAME))
            || profile1.value(QLatin1String(DISPLAYNAME)).toString() != QLatin1String("Test Kit2")
            || !profile1.contains(QLatin1String(ICON))
            || profile1.value(QLatin1String(ICON)).toString() != QLatin1String("/tmp/icon2.png")
            || !profile1.contains(QLatin1String(DATA))
            || profile1.value(QLatin1String(DATA)).type() != QVariant::Map
            || !profile1.contains(QLatin1String(AUTODETECTED))
            || profile1.value(QLatin1String(AUTODETECTED)).toBool() != true
            || !profile1.contains(QLatin1String(SDK))
            || profile1.value(QLatin1String(SDK)).toBool() != true)
        return false;

    data = profile1.value(QLatin1String(DATA)).toMap();
    if (data.count() != 9
            || !data.contains(QLatin1String(DEBUGGER))
            || data.value(QLatin1String(DEBUGGER)).type() != QVariant::Map
            || !data.contains(QLatin1String(DEVICE_TYPE))
            || data.value(QLatin1String(DEVICE_TYPE)).toString() != QLatin1String("Desktop")
            || !data.contains(QLatin1String(DEVICE_ID))
            || data.value(QLatin1String(DEVICE_ID)).toString() != QLatin1String("{dev-id}")
            || !data.contains(QLatin1String(SYSROOT))
            || data.value(QLatin1String(SYSROOT)).toString() != QLatin1String("/sys/root\\\\")
            || !data.contains(QLatin1String(TOOLCHAIN))
            || data.value(QLatin1String(TOOLCHAIN)).toString() != QLatin1String("{tc-id}")
            || !data.contains(QLatin1String(QT))
            || data.value(QLatin1String(QT)).toString() != QLatin1String("SDK.{qt-id}")
            || !data.contains(QLatin1String(MKSPEC))
            || data.value(QLatin1String(MKSPEC)).toString() != QLatin1String("unsupported/mkspec")
            || !data.contains(QLatin1String(ENV))
            || data.value(QLatin1String(ENV)).toStringList() != env
            || !data.contains(QLatin1String("extraData"))
            || data.value(QLatin1String("extraData")).toString() != QLatin1String("extraValue"))
        return false;

    // Profile 2: Test debugger id:
    map = addKit(map, tcMap, qtMap, devMap,
                 QLatin1String("test with debugger Id"), QLatin1String("Test debugger Id"), QLatin1String("/tmp/icon2.png"),
                 QString::fromLatin1("debugger Id"), 0, QString(),
                 QLatin1String("Desktop"), QString(), QString(),
                 QLatin1String("{tc-id}"), QLatin1String("{qt-id}"), QLatin1String("unsupported/mkspec"),
                 env,
                 KeyValuePairList() << KeyValuePair(QLatin1String("PE.Profile.Data/extraData"), QVariant(QLatin1String("extraValue"))));
    if (map.count() != 6
            || !map.contains(QLatin1String(VERSION))
            || map.value(QLatin1String(VERSION)).toInt() != 1
            || !map.contains(QLatin1String(COUNT))
            || map.value(QLatin1String(COUNT)).toInt() != 3
            || !map.contains(QLatin1String(DEFAULT))
            || map.value(QLatin1String(DEFAULT)).toInt() != 0
            || !map.contains(QLatin1String("Profile.0"))
            || !map.contains(QLatin1String("Profile.1"))
            || !map.contains(QLatin1String("Profile.2")))

    if (map.value(QLatin1String("Profile.0")) != profile0)
        return false;
    if (map.value(QLatin1String("Profile.1")) != profile1)
        return false;

    QVariantMap profile2 = map.value(QLatin1String("Profile.2")).toMap();
    if (profile2.count() != 6
            || !profile2.contains(QLatin1String(ID))
            || profile2.value(QLatin1String(ID)).toString() != QLatin1String("test with debugger Id")
            || !profile2.contains(QLatin1String(DISPLAYNAME))
            || profile2.value(QLatin1String(DISPLAYNAME)).toString() != QLatin1String("Test debugger Id")
            || !profile2.contains(QLatin1String(ICON))
            || profile2.value(QLatin1String(ICON)).toString() != QLatin1String("/tmp/icon2.png")
            || !profile2.contains(QLatin1String(DATA))
            || profile2.value(QLatin1String(DATA)).type() != QVariant::Map
            || !profile2.contains(QLatin1String(AUTODETECTED))
            || profile2.value(QLatin1String(AUTODETECTED)).toBool() != true
            || !profile2.contains(QLatin1String(SDK))
            || profile2.value(QLatin1String(SDK)).toBool() != true)
        return false;

    data = profile2.value(QLatin1String(DATA)).toMap();
    if (data.count() != 7
            || !data.contains(QLatin1String(DEBUGGER))
            || data.value(QLatin1String(DEBUGGER)).toString() != QLatin1String("debugger Id"))
        return false;

    return true;
}
#endif

QVariantMap AddKitOperation::addKit(const QVariantMap &map,
                                    const QString &id, const QString &displayName,
                                    const QString &icon, const QString &debuggerId,
                                    const quint32 &debuggerType, const QString &debugger,
                                    const QString &deviceType, const QString &device,
                                    const QString &sysRoot, const QString &tc, const QString &qt,
                                    const QString &mkspec, const QStringList &env,
                                    const KeyValuePairList &extra)
{
    QVariantMap tcMap = load(QLatin1String("ToolChains"));
    QVariantMap qtMap = load(QLatin1String("QtVersions"));
    QVariantMap devMap = load(QLatin1String("Devices"));

    return addKit(map, tcMap, qtMap, devMap, id, displayName, icon, debuggerId, debuggerType,
                  debugger, deviceType, device, sysRoot, tc, qt, mkspec, env, extra);
}

QVariantMap AddKitOperation::addKit(const QVariantMap &map, const QVariantMap &tcMap,
                                    const QVariantMap &qtMap, const QVariantMap &devMap,
                                    const QString &id, const QString &displayName,
                                    const QString &icon, const QString &debuggerId,
                                    const quint32 &debuggerType, const QString &debugger,
                                    const QString &deviceType, const QString &device,
                                    const QString &sysRoot, const QString &tc, const QString &qt,
                                    const QString &mkspec, const QStringList &env,
                                    const KeyValuePairList &extra)
{
    // Sanity check: Make sure autodetection source is not in use already:
    QStringList valueKeys = FindValueOperation::findValue(map, QVariant(id));
    bool hasId = false;
    foreach (const QString &k, valueKeys) {
        if (k.endsWith(QString(QLatin1Char('/')) + QLatin1String(ID))) {
            hasId = true;
            break;
        }
    }
    if (hasId) {
        std::cerr << "Error: Id " << qPrintable(id) << " already defined as kit." << std::endl;
        return QVariantMap();
    }

    if (!tc.isEmpty() && !AddToolChainOperation::exists(tcMap, tc)) {
        QRegExp abiRegExp = QRegExp(QLatin1String("[a-z0-9_]+-[a-z0-9_]+-[a-z0-9_]+-[a-z0-9_]+-(8|16|32|64|128)bit"));
        if (!abiRegExp.exactMatch(tc)) {
            std::cerr << "Error: Toolchain " << qPrintable(tc) << " does not exist." << std::endl;
            return QVariantMap();
        }
    }

    QString qtId = qt;
    if (!qtId.isEmpty() && !qtId.startsWith(QLatin1String("SDK.")))
        qtId = QString::fromLatin1("SDK.") + qt;
    if (!qtId.isEmpty() && !AddQtOperation::exists(qtMap, qtId)) {
        std::cerr << "Error: Qt " << qPrintable(qtId) << " does not exist." << std::endl;
        return QVariantMap();
    }
    if (!device.isEmpty() && !AddDeviceOperation::exists(devMap, device)) {
        std::cerr << "Error: Device " << qPrintable(device) << " does not exist." << std::endl;
        return QVariantMap();
    }

    // Find position to insert:
    bool ok;
    int count = GetOperation::get(map, QLatin1String(COUNT)).toInt(&ok);
    if (!ok || count < 0) {
        std::cerr << "Error: Count found in kits file seems wrong." << std::endl;
        return QVariantMap();
    }
    const QString kit = QString::fromLatin1(PREFIX) + QString::number(count);

    QString defaultKit = GetOperation::get(map, QLatin1String(DEFAULT)).toString();
    if (defaultKit.isEmpty())
        defaultKit = id;

    // remove data:
    QStringList toRemove;
    toRemove << QLatin1String(COUNT) << QLatin1String(DEFAULT);
    QVariantMap cleaned = RmKeysOperation::rmKeys(map, toRemove);

    // Sanity check: Make sure displayName is unique.
    QStringList nameKeys = FindKeyOperation::findKey(map, QLatin1String(DISPLAYNAME));
    QStringList nameList;
    foreach (const QString &nameKey, nameKeys)
        nameList << GetOperation::get(map, nameKey).toString();
    const QString uniqueName = makeUnique(displayName, nameList);

    // insert data:
    KeyValuePairList data;
    data << KeyValuePair(QStringList() << kit << QLatin1String(ID), QVariant(id));
    data << KeyValuePair(QStringList() << kit << QLatin1String(DISPLAYNAME), QVariant(uniqueName));
    data << KeyValuePair(QStringList() << kit << QLatin1String(ICON), QVariant(icon));
    data << KeyValuePair(QStringList() << kit << QLatin1String(AUTODETECTED), QVariant(true));
    data << KeyValuePair(QStringList() << kit << QLatin1String(SDK), QVariant(true));

    if (!debuggerId.isEmpty() || !debugger.isEmpty()) {
        if (debuggerId.isEmpty()) {
            data << KeyValuePair(QStringList() << kit << QLatin1String(DATA)
                                 << QLatin1String(DEBUGGER) << QLatin1String(DEBUGGER_ENGINE), QVariant(debuggerType));
            data << KeyValuePair(QStringList() << kit << QLatin1String(DATA)
                                 << QLatin1String(DEBUGGER) << QLatin1String(DEBUGGER_BINARY), QVariant(debugger));
        } else {
            data << KeyValuePair(QStringList() << kit << QLatin1String(DATA) << QLatin1String(DEBUGGER),
                                 QVariant(debuggerId));
        }
    }
    if (!deviceType.isNull())
        data << KeyValuePair(QStringList() << kit << QLatin1String(DATA)
                             << QLatin1String(DEVICE_TYPE), QVariant(deviceType));
    if (!device.isNull())
        data << KeyValuePair(QStringList() << kit << QLatin1String(DATA)
                             << QLatin1String(DEVICE_ID), QVariant(device));
    if (!sysRoot.isNull())
        data << KeyValuePair(QStringList() << kit << QLatin1String(DATA)
                             << QLatin1String(SYSROOT), QVariant(sysRoot));
    if (!tc.isNull())
        data << KeyValuePair(QStringList() << kit << QLatin1String(DATA)
                             << QLatin1String(TOOLCHAIN), QVariant(tc));
    if (!qtId.isNull())
        data << KeyValuePair(QStringList() << kit << QLatin1String(DATA)
                             << QLatin1String(QT), QVariant(qtId));
    if (!mkspec.isNull())
        data << KeyValuePair(QStringList() << kit << QLatin1String(DATA)
                             << QLatin1String(MKSPEC), QVariant(mkspec));
    if (!env.isEmpty())
        data << KeyValuePair(QStringList() << kit << QLatin1String(DATA)
                             << QLatin1String(ENV), QVariant(env));

    data << KeyValuePair(QStringList() << QLatin1String(DEFAULT), QVariant(defaultKit));
    data << KeyValuePair(QStringList() << QLatin1String(COUNT), QVariant(count + 1));

    KeyValuePairList qtExtraList;
    foreach (const KeyValuePair &pair, extra)
        qtExtraList << KeyValuePair(QStringList() << kit << pair.key, pair.value);
    data.append(qtExtraList);

    return AddKeysOperation::addKeys(cleaned, data);
}

QVariantMap AddKitOperation::initializeKits()
{
    QVariantMap map;
    map.insert(QLatin1String(VERSION), 1);
    map.insert(QLatin1String(DEFAULT), QString());
    map.insert(QLatin1String(COUNT), 0);
    return map;
}
