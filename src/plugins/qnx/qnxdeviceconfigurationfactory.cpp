/**************************************************************************
**
** Copyright (C) 2012 - 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
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
