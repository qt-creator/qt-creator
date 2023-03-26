// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "rmdeviceoperation.h"

#include "adddeviceoperation.h"
#include "getoperation.h"

#include "settings.h"

#include <iostream>

#ifdef WITH_TESTS
#include <QTest>
#endif

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(rmdevicelog, "qtc.sdktool.operations.rmdevice", QtWarningMsg)


QString RmDeviceOperation::name() const
{
    return QLatin1String("rmDev");
}

QString RmDeviceOperation::helpText() const
{
    return QLatin1String("remove a Device");
}

QString RmDeviceOperation::argumentsHelpText() const
{
    return QLatin1String("    --id <ID>                                  id of the device to remove.\n");
}

bool RmDeviceOperation::setArguments(const QStringList &args)
{
    if (args.count() != 2)
        return false;
    if (args.at(0) != QLatin1String("--id"))
        return false;

    m_id = args.at(1);

    if (m_id.isEmpty())
        qCCritical(rmdevicelog) << "No id given.";

    return !m_id.isEmpty();
}

int RmDeviceOperation::execute() const
{
    QVariantMap map = load(QLatin1String("Devices"));
    if (map.isEmpty())
        map = AddDeviceOperation::initializeDevices();

    QVariantMap result = rmDevice(map, m_id);

    if (result == map)
        return 2;

    return save(result, QLatin1String("Devices")) ? 0 : 3;
}

#ifdef WITH_TESTS
void RmDeviceOperation::unittest()
{

}
#endif

QVariantMap RmDeviceOperation::rmDevice(const QVariantMap &map, const QString &id)
{
    QVariantMap result = map;


    QVariantMap dmMap = map.value(QLatin1String(DEVICEMANAGER_ID)).toMap();

    bool found = false;
    QVariantList devList = GetOperation::get(dmMap, QLatin1String(DEVICE_LIST_ID)).toList();
    for (int i = 0; i < devList.count(); ++i) {
        QVariant value = devList.at(i);
        if (value.type() != QVariant::Map)
            continue;
        QVariantMap deviceData = value.toMap();
        QString devId = deviceData.value(QLatin1String(DEVICE_ID_ID)).toString();
        if (devId == id) {
            found = true;
            devList.removeAt(i);
            break;
        }
    }
    dmMap.insert(QLatin1String(DEVICE_LIST_ID), devList);
    result.insert(QLatin1String(DEVICEMANAGER_ID), dmMap);

    if (!found){
        qCCritical(rmdevicelog) << "Device " << qPrintable(id) << " not found.";

    }
    return result;
}

