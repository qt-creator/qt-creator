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

#include "rmdeviceoperation.h"

#include "adddeviceoperation.h"
#include "getoperation.h"

#include "settings.h"

#include <iostream>

QString RmDeviceOperation::name() const
{
    return QLatin1String("rmDev");
}

QString RmDeviceOperation::helpText() const
{
    return QLatin1String("remove a Device from Qt Creator");
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
        std::cerr << "No id given." << std::endl << std::endl;

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
bool RmDeviceOperation::test() const
{
    return true;
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

    if (!found)
        std::cerr << "Device " << qPrintable(id) << " not found." << std::endl;
    return result;
}

