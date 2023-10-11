// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "adddeviceoperation.h"

#include "addkeysoperation.h"
#include "findkeyoperation.h"
#include "findvalueoperation.h"
#include "getoperation.h"
#include "rmkeysoperation.h"

#include "settings.h"

#include <QLoggingCategory>

#ifdef WITH_TESTS
#include <QTest>
#endif

Q_LOGGING_CATEGORY(addDeviceLog, "qtc.sdktool.operations.adddevice", QtWarningMsg)

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
    return QLatin1String("    --id <ID>                                  id of the new device (required).\n"
                         "    --name <NAME>                              display name of the new device (required).\n"
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
                         "    --dockerRepo <STRING>                      Docker image repo.\n"
                         "    --dockerTag <STRING>                       Docker image tag.\n"
                         "    --dockerMappedPaths <STRING>               Docker mapped paths (semi-colon separated).\n"
                         "    --dockerClangdExecutable <STRING>          Path to clangd inside the docker.\n"
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

        if (current == QLatin1String("--dockerMappedPaths")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_dockerMappedPaths = next.split(';');
            continue;
        }

        if (current == QLatin1String("--dockerClangdExecutable")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_clangdExecutable = next;
            continue;
        }

        if (current == QLatin1String("--dockerRepo")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_dockerRepo = next;
            continue;
        }

        if (current == QLatin1String("--dockerTag")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_dockerTag = next;
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
        qCCritical(addDeviceLog) << "No id given for device.";
    if (m_displayName.isEmpty())
        qCCritical(addDeviceLog) << "No name given for device.";

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
void AddDeviceOperation::unittest()
{
    QVariantMap map = initializeDevices();

    AddDeviceData devData;
    devData.m_id = "test id";
    devData.m_displayName = "test name";
    devData.m_type = 1;
    devData.m_authentication = 2;
    devData.m_b2q_platformHardware = "HW";
    devData.m_b2q_platformSoftware = "SW";
    devData.m_debugServer = "debugServer";
    devData.m_freePortsSpec = "ports";
    devData.m_host = "host";
    devData.m_keyFile = "keyfile";
    devData.m_origin = 3;
    devData.m_osType = "ostype";
    devData.m_password = "passwd";
    devData.m_sshPort = 4;
    devData.m_timeout = 5;
    devData.m_uname = "uname";
    devData.m_version = 6;
    devData.m_dockerMappedPaths = QStringList{"/opt", "/data"};
    devData.m_dockerRepo = "repo";
    devData.m_dockerTag = "tag";
    devData.m_clangdExecutable = "clangdexe";

    QVariantMap result = devData.addDevice(map);
    QVariantMap data = result.value(QLatin1String(DEVICEMANAGER_ID)).toMap();
    QVariantList devList = data.value(QLatin1String(DEVICE_LIST_ID)).toList();
    QCOMPARE(devList.count(), 1);
    QVariantMap dev = devList.at(0).toMap();
    QCOMPARE(dev.count(), 21);
    QCOMPARE(dev.value(QLatin1String("Authentication")).toInt(), 2);
    QCOMPARE(dev.value(QLatin1String("DebugServerKey")).toString(), QLatin1String("debugServer"));
    QCOMPARE(dev.value(QLatin1String("FreePortsSpec")).toString(), QLatin1String("ports"));
    QCOMPARE(dev.value(QLatin1String("Host")).toString(), QLatin1String("host"));
    QCOMPARE(dev.value(QLatin1String("InternalId")).toString(), QLatin1String("test id"));
    QCOMPARE(dev.value(QLatin1String("KeyFile")).toString(), QLatin1String("keyfile"));
    QCOMPARE(dev.value(QLatin1String("Name")).toString(), QLatin1String("test name"));
    QCOMPARE(dev.value(QLatin1String("Origin")).toInt(), 3);
    QCOMPARE(dev.value(QLatin1String("OsType")).toString(), QLatin1String("ostype"));
    QCOMPARE(dev.value(QLatin1String("Password")).toString(), QLatin1String("passwd"));
    QCOMPARE(dev.value(QLatin1String("SshPort")).toInt(), 4);
    QCOMPARE(dev.value(QLatin1String("Timeout")).toInt(), 5);
    QCOMPARE(dev.value(QLatin1String("Type")).toInt(), 1);
    QCOMPARE(dev.value(QLatin1String("Uname")).toString(), QLatin1String("uname"));
    QCOMPARE(dev.value(QLatin1String("Version")).toInt(), 6);
    QCOMPARE(dev.value(QLatin1String("DockerDeviceDataRepo")).toString(), "repo");
    QCOMPARE(dev.value(QLatin1String("DockerDeviceDataTag")).toString(), "tag");
    QCOMPARE(dev.value(QLatin1String("DockerDeviceClangDExecutable")).toString(), "clangdexe");

    const QStringList paths = dev.value(QLatin1String("DockerDeviceMappedPaths")).toStringList();
    QCOMPARE(paths, QStringList({"/opt", "/data"}));
}
#endif

QVariantMap AddDeviceData::addDevice(const QVariantMap &map) const
{
    QVariantMap result = map;
    if (exists(map, m_id)) {
        qCCritical(addDeviceLog) << "Device " << qPrintable(m_id) << " already exists!";
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
    dev.append(KeyValuePair(QLatin1String("DockerDeviceMappedPaths"), QVariant(m_dockerMappedPaths)));
    dev.append(KeyValuePair(QLatin1String("DockerDeviceClangDExecutable"), QVariant(m_clangdExecutable)));
    dev.append(KeyValuePair(QLatin1String("DockerDeviceDataRepo"), QVariant(m_dockerRepo)));
    dev.append(KeyValuePair(QLatin1String("DockerDeviceDataTag"), QVariant(m_dockerTag)));
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
    const QVariantList devList = dmMap.value(QLatin1String(DEVICE_LIST_ID)).toList();
    for (const QVariant &dev : devList) {
        QVariantMap devData = dev.toMap();
        QString current = devData.value(QLatin1String(DEVICE_ID_ID)).toString();
        if (current == id)
            return true;
    }
    return false;
}
