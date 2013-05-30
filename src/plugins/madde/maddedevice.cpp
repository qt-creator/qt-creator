/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#include "maddedevice.h"

#include "maddedevicetester.h"
#include "maemoconstants.h"

#include <remotelinux/linuxdevicetestdialog.h>
#include <remotelinux/publickeydeploymentdialog.h>
#include <remotelinux/remotelinux_constants.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace RemoteLinux;

namespace Madde {
namespace Internal {

MaddeDevice::Ptr MaddeDevice::create()
{
    return Ptr(new MaddeDevice);
}

MaddeDevice::Ptr MaddeDevice::create(const QString &name, Core::Id type,
        MachineType machineType, Origin origin, Core::Id id)
{
    return Ptr(new MaddeDevice(name, type, machineType, origin, id));
}

MaddeDevice::MaddeDevice()
{
}

MaddeDevice::MaddeDevice(const QString &name, Core::Id type, MachineType machineType,
        Origin origin, Core::Id id)
    : LinuxDevice(name, type, machineType, origin, id)
{
}

MaddeDevice::MaddeDevice(const MaddeDevice &other) : LinuxDevice(other)
{
}

ProjectExplorer::IDevice::Ptr MaddeDevice::clone() const
{
    return Ptr(new MaddeDevice(*this));
}

QString MaddeDevice::displayType() const
{
    return maddeDisplayType(type());
}

QString MaddeDevice::maddeDisplayType(Core::Id type)
{
    if (type == Maemo5OsType)
        return tr("Maemo5/Fremantle");
    if (type == HarmattanOsType)
        return tr("MeeGo 1.2 Harmattan");
    QTC_ASSERT(false, return QString());
    return QString(); // For crappy compilers.
}

bool MaddeDevice::allowsRemoteMounts(Core::Id type)
{
    return type == Maemo5OsType;
}

bool MaddeDevice::allowsPackagingDisabling(Core::Id type)
{
    return type == Maemo5OsType;
}

bool MaddeDevice::allowsQmlDebugging(Core::Id type)
{
    return type == HarmattanOsType;
}

QSize MaddeDevice::packageManagerIconSize(Core::Id type)
{
    if (type == Maemo5OsType)
        return QSize(48, 48);
    if (type == HarmattanOsType)
        return QSize(64, 64);
    return QSize();
}

AbstractLinuxDeviceTester *MaddeDevice::createDeviceTester() const
{
    return new MaddeDeviceTester;
}

} // namespace Internal
} // namespace Madde
