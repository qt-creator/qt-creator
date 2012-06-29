/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (C) 2011 - 2012 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "qnxdeviceconfiguration.h"

using namespace Qnx;
using namespace Qnx::Internal;

QnxDeviceConfiguration::QnxDeviceConfiguration()
    : RemoteLinux::LinuxDeviceConfiguration()
{
}

QnxDeviceConfiguration::QnxDeviceConfiguration(const QString &name, Core::Id type, MachineType machineType, Origin origin, Core::Id id)
    : RemoteLinux::LinuxDeviceConfiguration(name, type, machineType, origin, id)
{
}

QnxDeviceConfiguration::QnxDeviceConfiguration(const QnxDeviceConfiguration &other)
    : RemoteLinux::LinuxDeviceConfiguration(other)
{
}


QnxDeviceConfiguration::Ptr QnxDeviceConfiguration::create()
{
    return Ptr(new QnxDeviceConfiguration);
}

QnxDeviceConfiguration::Ptr QnxDeviceConfiguration::create(const QString &name, Core::Id type, RemoteLinux::LinuxDeviceConfiguration::MachineType machineType, ProjectExplorer::IDevice::Origin origin, Core::Id id)
{
    return Ptr(new QnxDeviceConfiguration(name, type, machineType, origin, id));
}

QString QnxDeviceConfiguration::displayType() const
{
    return tr("QNX");
}

ProjectExplorer::IDevice::Ptr QnxDeviceConfiguration::clone() const
{
    return Ptr(new QnxDeviceConfiguration(*this));
}

