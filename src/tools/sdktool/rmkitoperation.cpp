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

#include "rmkitoperation.h"

#include "addkeysoperation.h"
#include "addtoolchainoperation.h"
#include "adddeviceoperation.h"
#include "addqtoperation.h"
#include "addkitoperation.h"
#include "findkeyoperation.h"
#include "findvalueoperation.h"
#include "getoperation.h"
#include "rmkeysoperation.h"

#include "settings.h"

#include <iostream>

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
        std::cerr << "No id given." << std::endl << std::endl;

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
bool RmKitOperation::test() const
{
    QVariantMap tcMap = AddToolChainOperation::initializeToolChains();
    tcMap = AddToolChainData{"{tc-id}", "langId", "TC", "/usr/bin/gcc",
                             "x86-linux-generic-elf-32bit", "x86-linux-generic-elf-32bit", {}}
        .addToolChain(tcMap);

    QVariantMap qtMap = AddQtData::initializeQtVersions();
    qtMap = AddQtData{"{qt-id}", "Qt", "desktop-qt", "/usr/bin/qmake", {}, {}}.addQt(qtMap);

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
    kitData.m_debugger =  "/usr/bin/gdb-test";
    kitData.m_deviceType = "Desktop";
    kitData.m_tcs = tcs;
    kitData.m_qt = "{qt-id}";
    kitData.m_mkspec = "unsupported/mkspec";
    kitData.m_extra =  {{"PE.Profile.Data/extraData", QVariant("extraValue")}};

    QVariantMap map = kitData.addKit(AddKitData::initializeKits(), tcMap, qtMap, devMap, {});


    kitData.m_id = "testId2";
    kitData.m_icon = "/tmp/icon2.png";
    kitData.m_icon = "/usr/bin/gdb-test2";
    kitData.m_mkspec = "unsupported/mkspec2";
    kitData.m_extra =  {{"PE.Profile.Data/extraData", QVariant("extraValue2")}};

    map = kitData.addKit(map, tcMap, qtMap, devMap, {});

    QVariantMap result = rmKit(map, "testId");
    if (result.count() != 4
            || !result.contains("Profile.0")
            || !result.contains(COUNT) || result.value(COUNT).toInt() != 1
            || !result.contains(DEFAULT) || result.value(DEFAULT).toInt() != 0
            || !result.contains(VERSION) || result.value(VERSION).toInt() != 1)
        return false;

    result = rmKit(map, "unknown");
    if (result != map)
        return false;

    result = rmKit(map, "testId2");
    if (result.count() != 4
            || !result.contains("Profile.0")
            || !result.contains(COUNT) || result.value(COUNT).toInt() != 1
            || !result.contains(DEFAULT) || result.value(DEFAULT).toInt() != 0
            || !result.contains(VERSION) || result.value(VERSION).toInt() != 1)
        return false;

    result = rmKit(result, QLatin1String("testId"));
    if (result.count() != 3
            || !result.contains(COUNT) || result.value(COUNT).toInt() != 0
            || !result.contains(DEFAULT) || result.value(DEFAULT).toInt() != -1
            || !result.contains(VERSION) || result.value(VERSION).toInt() != 1)
        return false;

    return true;
}
#endif

QVariantMap RmKitOperation::rmKit(const QVariantMap &map, const QString &id)
{
    QVariantMap result = AddKitOperation::initializeKits();

    QVariantList profileList;
    bool ok;
    int count = GetOperation::get(map, COUNT).toInt(&ok);
    if (!ok) {
        std::cerr << "Error: The count found in map is not an integer." << std::endl;
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
        std::cerr << "Error: Id was not found." << std::endl;
        return map;
    }

    int defaultKit = GetOperation::get(map, DEFAULT).toInt(&ok);
    if (!ok) {
        std::cerr << "Error: Could not find the default kit." << std::endl;
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
        data << KeyValuePair(QString::fromLatin1(PREFIX) + QString::number(i),
                             profileList.at(i));

    return AddKeysData{data}.addKeys(result);
}
