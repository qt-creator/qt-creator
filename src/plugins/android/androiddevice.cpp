/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androiddevice.h"
#include "androidconstants.h"

#include <QCoreApplication>

namespace Android {
namespace Internal {

AndroidDevice::AndroidDevice():
    ProjectExplorer::IDevice(Core::Id(Constants::ANDROID_DEVICE_TYPE),
                             IDevice::AutoDetected,
                             IDevice::Hardware,
                             Core::Id(Constants::ANDROID_DEVICE_ID))
{
    setDisplayName(QCoreApplication::translate("ProjectExplorer::AndroidDevice", "Run on Android"));
    setDeviceState(DeviceReadyToUse);
}

AndroidDevice::AndroidDevice(const AndroidDevice &other):
    ProjectExplorer::IDevice(other)
{ }


ProjectExplorer::IDevice::DeviceInfo AndroidDevice::deviceInformation() const
{
    return ProjectExplorer::IDevice::DeviceInfo();
}

QString AndroidDevice::displayType() const
{
    return QCoreApplication::translate("ProjectExplorer::AndroidDevice", "Android");
}

ProjectExplorer::IDeviceWidget *AndroidDevice::createWidget()
{
    return 0;
}

QList<Core::Id> AndroidDevice::actionIds() const
{
    return QList<Core::Id>()<<Core::Id(Constants::ANDROID_DEVICE_ID);
}

QString AndroidDevice::displayNameForActionId(Core::Id actionId) const
{
    Q_UNUSED(actionId)
    return QString();
}

void AndroidDevice::executeAction(Core::Id actionId, QWidget *parent) const
{
    Q_UNUSED(actionId)
    Q_UNUSED(parent)
}

ProjectExplorer::IDevice::Ptr AndroidDevice::clone() const
{
    return ProjectExplorer::IDevice::Ptr(new AndroidDevice(*this));
}

} // namespace Internal
} // namespace Android
