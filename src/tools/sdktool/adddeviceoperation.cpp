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

#include "adddeviceoperation.h"

#include "addkeysoperation.h"
#include "findkeyoperation.h"
#include "findvalueoperation.h"
#include "getoperation.h"
#include "rmkeysoperation.h"

#include "settings.h"

#include <iostream>

const char DEVICEMANAGER_ID[] = "DeviceManager";
const char DEFAULT_DEVICES_ID[] = "DefaultDevices";
const char DEVICE_LIST_ID[] = "DeviceList";

const char DEVICE_ID_ID[] = "InternalId";

static const char INTERNAL_DSEKTOP_DEVICE_ID[] = "Desktop Device";

QString AddDeviceOperation::name() const
{
    return QLatin1String("addDev");
}

QString AddDeviceOperation::helpText() const
{
    return QLatin1String("add a Device");
}

QString AddDeviceOperation::argumentsHelpText() const
{
    return QLatin1String("    --id <ID>                                  id of the new kit (required).\n"
                         "    --name <NAME>                              display name of the new kit (required).\n"
                         "    --type <INT>                               type (required).\n"
                         "    --authentication <INT>                     authentication.\n"
                         "    --b2qHardware <STRING>                     Boot2Qt Platform Info Hardware.\n"
                         "    --b2qSoftware <STRING>                     Boot2Qt Platform Info Software.\n"
                         "    --freePorts <STRING>                       Free ports specification.\n"
                         "    --host <STRING>                            Host.\n"
                         "    --debugServerKey <STRING>                  Debug server key.\n"
                         "    --keyFile <STRING>                         Key file.\n"
                         "    --origin <INT>                             origin.\n"
                         "    --osType <STRING>                          Os Type.\n"
                         "    --password <STRING>                        Password.\n"
                         "    --sshPort <INT>                            ssh port.\n"
                         "    --timeout <INT>                            timeout.\n"
                         "    --uname <STRING>                           uname.\n"
                         "    <KEY> <TYPE:VALUE>                         extra key value pairs\n");
}

bool AddDeviceOperation::setArguments(const QStringList &args)
{
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

        if (current == QLatin1String("--authentication")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            bool ok;
            m_authentication = next.toInt(&ok);
            if (!ok)
                return false;
            continue;
        }

        if (current == QLatin1String("--b2qHardware")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_b2q_platformHardware = next;
            continue;
        }

        if (current == QLatin1String("--b2qSoftware")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_b2q_platformSoftware = next;
            continue;
        }

        if (current == QLatin1String("--freePorts")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_freePortsSpec = next;
            continue;
        }

        if (current == QLatin1String("--host")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_host = next;
            continue;
        }

        if (current == QLatin1String("--debugServerKey")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_debugServer = next;
            continue;
        }

        if (current == QLatin1String("--keyFile")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_keyFile = next;
            continue;
        }

        if (current == QLatin1String("--origin")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            bool ok;
            m_origin = next.toInt(&ok);
            if (!ok)
                return false;
            continue;
        }

        if (current == QLatin1String("--osType")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_osType = next;
            continue;
        }

        if (current == QLatin1String("--password")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_password = next;
            continue;
        }

        if (current == QLatin1String("--sshPort")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            bool ok;
            m_sshPort = next.toInt(&ok);
            if (!ok)
                return false;
            continue;
        }

        if (current == QLatin1String("--timeout")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            bool ok;
            m_timeout = next.toInt(&ok);
            if (!ok)
                return false;
            continue;
        }

        if (current == QLatin1String("--type")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            bool ok;
            m_type = next.toInt(&ok);
            if (!ok)
                return false;
            continue;
        }


        if (current == QLatin1String("--uname")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_uname = next;
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
        std::cerr << "No id given for device." << std::endl << std::endl;
    if (m_displayName.isEmpty())
        std::cerr << "No name given for device." << std::endl << std::endl;

    return !m_id.isEmpty() && !m_displayName.isEmpty() && m_type >= 0;
}

int AddDeviceOperation::execute() const
{
    QVariantMap map = load(QLatin1String("Devices"));
    if (map.isEmpty())
        map = initializeDevices();

    QVariantMap result = addDevice(map);

    if (result.isEmpty() || map == result)
        return 2;

    return save(result, QLatin1String("Devices")) ? 0 : 3;
}

#ifdef WITH_TESTS
bool AddDeviceOperation::test() const
{
    QVariantMap map = initializeDevices();

    AddDeviceData devData = {
        QLatin1String("test id"), QLatin1String("test name"),
        1, 2, QLatin1String("HW"), QLatin1String("SW"),
        QLatin1String("debugServer"), QLatin1String("ports"),
        QLatin1String("host"), QLatin1String("keyfile"), 3,
        QLatin1String("ostype"), QLatin1String("passwd"), 4, 5,
        QLatin1String("uname"), 6, KeyValuePairList()
    };
    QVariantMap result = devData.addDevice(map);
    QVariantMap data = result.value(QLatin1String(DEVICEMANAGER_ID)).toMap();
    QVariantList devList = data.value(QLatin1String(DEVICE_LIST_ID)).toList();
    if (devList.count() != 1)
        return false;
    QVariantMap dev = devList.at(0).toMap();
    if (dev.count() != 17)
        return false;
    if (dev.value(QLatin1String("Authentication")).toInt() != 2)
        return false;
    if (dev.value(QLatin1String("DebugServerKey")).toString() != QLatin1String("debugServer"))
        return false;
    if (dev.value(QLatin1String("FreePortsSpec")).toString() != QLatin1String("ports"))
        return false;
    if (dev.value(QLatin1String("Host")).toString() != QLatin1String("host"))
        return false;
    if (dev.value(QLatin1String("InternalId")).toString() != QLatin1String("test id"))
        return false;
    if (dev.value(QLatin1String("KeyFile")).toString() != QLatin1String("keyfile"))
        return false;
    if (dev.value(QLatin1String("Name")).toString() != QLatin1String("test name"))
        return false;
    if (dev.value(QLatin1String("Origin")).toInt() != 3)
        return false;
    if (dev.value(QLatin1String("OsType")).toString() != QLatin1String("ostype"))
        return false;
    if (dev.value(QLatin1String("Password")).toString() != QLatin1String("passwd"))
        return false;
    if (dev.value(QLatin1String("SshPort")).toInt() != 4)
        return false;
    if (dev.value(QLatin1String("Timeout")).toInt() != 5)
        return false;
    if (dev.value(QLatin1String("Type")).toInt() != 1)
        return false;
    if (dev.value(QLatin1String("Uname")).toString() != QLatin1String("uname"))
        return false;
    if (dev.value(QLatin1String("Version")).toInt() != 6)
        return false;

    return true;
}
#endif

QVariantMap AddDeviceData::addDevice(const QVariantMap &map) const
{
    QVariantMap result = map;
    if (exists(map, m_id)) {
        std::cerr << "Device " << qPrintable(m_id) << " already exists!" << std::endl;
        return result;
    }

    QVariantMap dmMap = map.value(QLatin1String(DEVICEMANAGER_ID)).toMap();
    QVariantList devList = dmMap.value(QLatin1String(DEVICE_LIST_ID)).toList();

    KeyValuePairList dev;
    dev.append(KeyValuePair(QLatin1String(DEVICE_ID_ID), QVariant(m_id)));
    dev.append(KeyValuePair(QLatin1String("Name"), QVariant(m_displayName)));
    dev.append(KeyValuePair(QLatin1String("Type"), QVariant(m_type)));
    dev.append(KeyValuePair(QLatin1String("Authentication"), QVariant(m_authentication)));
    dev.append(KeyValuePair(QLatin1String("Boot2Qt.PlatformInfoHardware"), QVariant(m_b2q_platformHardware)));
    dev.append(KeyValuePair(QLatin1String("Boot2Qt.PlatformInfoSoftware"), QVariant(m_b2q_platformSoftware)));
    dev.append(KeyValuePair(QLatin1String("DebugServerKey"), QVariant(m_debugServer)));
    dev.append(KeyValuePair(QLatin1String("FreePortsSpec"), QVariant(m_freePortsSpec)));
    dev.append(KeyValuePair(QLatin1String("Host"), QVariant(m_host)));
    dev.append(KeyValuePair(QLatin1String("KeyFile"), QVariant(m_keyFile)));
    dev.append(KeyValuePair(QLatin1String("Origin"), QVariant(m_origin)));
    dev.append(KeyValuePair(QLatin1String("OsType"), QVariant(m_osType)));
    dev.append(KeyValuePair(QLatin1String("Password"), QVariant(m_password)));
    dev.append(KeyValuePair(QLatin1String("SshPort"), QVariant(m_sshPort)));
    dev.append(KeyValuePair(QLatin1String("Timeout"), QVariant(m_timeout)));
    dev.append(KeyValuePair(QLatin1String("Uname"), QVariant(m_uname)));
    dev.append(KeyValuePair(QLatin1String("Version"), QVariant(m_version)));
    dev.append(m_extra);

    QVariantMap devMap = AddKeysData{dev}.addKeys(QVariantMap());

    devList.append(devMap);

    dmMap.insert(QLatin1String(DEVICE_LIST_ID), devList);

    result.insert(QLatin1String(DEVICEMANAGER_ID), dmMap);

    return result;
}

QVariantMap AddDeviceData::initializeDevices()
{
    QVariantMap dmData;
    dmData.insert(QLatin1String(DEFAULT_DEVICES_ID), QVariantMap());
    dmData.insert(QLatin1String(DEVICE_LIST_ID), QVariantList());

    QVariantMap data;
    data.insert(QLatin1String(DEVICEMANAGER_ID), dmData);
    return data;
}

bool AddDeviceData::exists(const QString &id)
{
    QVariantMap map = Operation::load(QLatin1String("Devices"));
    return exists(map, id);
}

bool AddDeviceData::exists(const QVariantMap &map, const QString &id)
{
    if (id == QLatin1String(INTERNAL_DSEKTOP_DEVICE_ID))
        return true;
    QVariantMap dmMap = map.value(QLatin1String(DEVICEMANAGER_ID)).toMap();
    QVariantList devList = dmMap.value(QLatin1String(DEVICE_LIST_ID)).toList();
    foreach (const QVariant &dev, devList) {
        QVariantMap devData = dev.toMap();
        QString current = devData.value(QLatin1String(DEVICE_ID_ID)).toString();
        if (current == id)
            return true;
    }
    return false;
}
