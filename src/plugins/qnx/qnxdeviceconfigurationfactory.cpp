/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: KDAB (info@kdab.com)
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
    return { Constants::QNX_QNX_OS_TYPE };
}

bool QnxDeviceConfigurationFactory::canCreate() const
{
    return true;
}

ProjectExplorer::IDevice::Ptr QnxDeviceConfigurationFactory::create(Core::Id id) const
{
    Q_UNUSED(id);
    QnxDeviceConfigurationWizard wizard;
    if (wizard.exec() != QDialog::Accepted)
        return ProjectExplorer::IDevice::Ptr();
    return wizard.device();
}

bool QnxDeviceConfigurationFactory::canRestore(const QVariantMap &map) const
{
    return ProjectExplorer::IDevice::typeFromMap(map) == Constants::QNX_QNX_OS_TYPE;
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
