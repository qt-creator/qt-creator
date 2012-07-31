/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "desktopdevice.h"
#include "projectexplorerconstants.h"
#include "deviceprocesslist.h"

#include <QCoreApplication>

namespace ProjectExplorer {

DesktopDevice::DesktopDevice() : IDevice(Core::Id(Constants::DESKTOP_DEVICE_TYPE),
                                         IDevice::AutoDetected,
                                         IDevice::Hardware,
                                         Core::Id(Constants::DESKTOP_DEVICE_ID))
{
    setDisplayName(QCoreApplication::translate("ProjectExplorer::DesktopDevice", "Run locally"));
}

DesktopDevice::DesktopDevice(const DesktopDevice &other) :
    IDevice(other)
{ }

IDevice::DeviceInfo DesktopDevice::deviceInformation() const
{
    return DeviceInfo();
}

QString DesktopDevice::displayType() const
{
    return QCoreApplication::translate("ProjectExplorer::DesktopDevice", "Desktop");
}

IDeviceWidget *DesktopDevice::createWidget()
{
    return 0;
}

QList<Core::Id> DesktopDevice::actionIds() const
{
    return QList<Core::Id>();
}

QString DesktopDevice::displayNameForActionId(Core::Id actionId) const
{
    Q_UNUSED(actionId);
    return QString();
}

void DesktopDevice::executeAction(Core::Id actionId, QWidget *parent) const
{
    Q_UNUSED(actionId);
    Q_UNUSED(parent);
}

IDevice::Ptr DesktopDevice::clone() const
{
    return Ptr(new DesktopDevice(*this));
}

QString DesktopDevice::listProcessesCommandLine() const
{
    return QString();
}

QString DesktopDevice::killProcessCommandLine(const DeviceProcess &process) const
{
    Q_UNUSED(process);
    return QString();
}

QList<DeviceProcess> DesktopDevice::buildProcessList(const QString &listProcessesReply) const
{
    Q_UNUSED(listProcessesReply);
    return QList<DeviceProcess>();
}

} // namespace ProjectExplorer
