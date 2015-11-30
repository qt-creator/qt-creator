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

