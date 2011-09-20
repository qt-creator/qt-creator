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

#include "remotelinuxplugin.h"

#include "deployablefile.h"
#include "genericlinuxdeviceconfigurationfactory.h"
#include "genericremotelinuxdeploystepfactory.h"
#include "linuxdeviceconfigurations.h"
#include "qt4projectmanager/qt4projectmanagerconstants.h"
#include "remotelinuxdeployconfigurationfactory.h"
#include "remotelinuxrunconfigurationfactory.h"
#include "remotelinuxruncontrolfactory.h"
#include "remotelinuxsettingspages.h"

#include <QtCore/QtPlugin>

namespace RemoteLinux {
namespace Internal {

RemoteLinuxPlugin::RemoteLinuxPlugin()
{
}

RemoteLinuxPlugin::~RemoteLinuxPlugin()
{
}

bool RemoteLinuxPlugin::initialize(const QStringList &arguments,
    QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    LinuxDeviceConfigurations::instance(this);

    addAutoReleasedObject(new LinuxDeviceConfigurationsSettingsPage);
    addAutoReleasedObject(new GenericLinuxDeviceConfigurationFactory);
    addAutoReleasedObject(new RemoteLinuxRunConfigurationFactory);
    addAutoReleasedObject(new RemoteLinuxRunControlFactory);
    addAutoReleasedObject(new RemoteLinuxDeployConfigurationFactory);
    addAutoReleasedObject(new GenericRemoteLinuxDeployStepFactory);

    qRegisterMetaType<RemoteLinux::DeployableFile>("RemoteLinux::DeployableFile");

    return true;
}

void RemoteLinuxPlugin::extensionsInitialized()
{
}

} // namespace Internal
} // namespace RemoteLinux

Q_EXPORT_PLUGIN(RemoteLinux::Internal::RemoteLinuxPlugin)
