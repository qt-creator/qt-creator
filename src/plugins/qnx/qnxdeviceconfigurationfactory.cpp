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

#include "qnxdeviceconfigurationfactory.h"

#include "qnxconstants.h"
#include "qnxdeviceconfigurationwizard.h"
#include "qnxdeviceconfiguration.h"

#include <utils/qtcassert.h>

using namespace Qnx;
using namespace Qnx::Internal;

QnxDeviceConfigurationFactory::QnxDeviceConfigurationFactory(QObject *parent) :
    ProjectExplorer::IDeviceFactory(parent)
{
}

QString QnxDeviceConfigurationFactory::displayNameForId(Core::Id type) const
{
    Q_UNUSED(type);
    return tr("QNX Device");
}

QList<Core::Id> QnxDeviceConfigurationFactory::availableCreationIds() const
{
    QList<Core::Id> result;
    result << Core::Id(Constants::QNX_QNX_OS_TYPE);
    return result;
}

bool QnxDeviceConfigurationFactory::canCreate() const
{
    return true;
}

ProjectExplorer::IDevice::Ptr QnxDeviceConfigurationFactory::create(Core::Id id) const
{
    Q_UNUSED(id);
    QnxDeviceConfigurationWizard wizard;
    if (wizard.exec() != QDialog::Accepted) {
        return ProjectExplorer::IDevice::Ptr();
    }
    return wizard.device();
}

bool QnxDeviceConfigurationFactory::canRestore(const QVariantMap &map) const
{
    return ProjectExplorer::IDevice::typeFromMap(map) == Core::Id(Constants::QNX_QNX_OS_TYPE);
}

ProjectExplorer::IDevice::Ptr QnxDeviceConfigurationFactory::restore(const QVariantMap &map) const
{
    QTC_ASSERT(canRestore(map), return QnxDeviceConfiguration::Ptr());
    const QnxDeviceConfiguration::Ptr device = QnxDeviceConfiguration::create();
    device->fromMap(map);
    return device;
}

Core::Id QnxDeviceConfigurationFactory::deviceType()
{
    return Core::Id(Constants::QNX_QNX_OS_TYPE);
}
