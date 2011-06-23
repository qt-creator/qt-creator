/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
#include "abstractlinuxdevicedeploystep.h"

#include "linuxdeviceconfiguration.h"
#include "qt4maemodeployconfiguration.h"

using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {


AbstractLinuxDeviceDeployStep::AbstractLinuxDeviceDeployStep(DeployConfiguration *dc)
    : m_deployConfiguration(qobject_cast<Qt4MaemoDeployConfiguration *>(dc))
{
}

AbstractLinuxDeviceDeployStep::~AbstractLinuxDeviceDeployStep()
{
}

LinuxDeviceConfiguration::ConstPtr AbstractLinuxDeviceDeployStep::deviceConfiguration() const
{
    return m_deviceConfiguration;
}

bool AbstractLinuxDeviceDeployStep::isDeploymentPossible(QString &whyNot) const
{
    if (!m_deployConfiguration->deviceConfiguration()) {
        whyNot = tr("No valid device set.");
        return false;
    }
    return isDeploymentPossibleInternal(whyNot);
}

bool AbstractLinuxDeviceDeployStep::initialize(QString &errorMsg)
{
    if (!isDeploymentPossible(errorMsg))
        return false;
    m_deviceConfiguration = m_deployConfiguration->deviceConfiguration();
    return true;
}

} // namespace Internal
} // namespace RemoteLinux
