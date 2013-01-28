/**************************************************************************
**
** Copyright (c) 2013 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androidplugin.h"

#include "androidconstants.h"
#include "androidconfigurations.h"
#include "androiddeploystepfactory.h"
#include "androiddevice.h"
#include "androiddevicefactory.h"
#include "androidconfigurations.h"
#include "androidmanager.h"
#include "androidpackagecreationfactory.h"
#include "androidpackageinstallationfactory.h"
#include "androidrunfactories.h"
#include "androidsettingspage.h"
#include "androidtoolchain.h"
#include "androidqtversionfactory.h"
#include "androiddeployconfiguration.h"

#include <QtPlugin>

#include <projectexplorer/devicesupport/devicemanager.h>

namespace Android {

AndroidPlugin::AndroidPlugin()
{ }

bool AndroidPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorMessage);

    Internal::AndroidConfigurations::instance(this);

    addAutoReleasedObject(new Internal::AndroidRunControlFactory);
    addAutoReleasedObject(new Internal::AndroidRunConfigurationFactory);
    addAutoReleasedObject(new Internal::AndroidPackageInstallationFactory);
    addAutoReleasedObject(new Internal::AndroidPackageCreationFactory);
    addAutoReleasedObject(new Internal::AndroidDeployStepFactory);
    addAutoReleasedObject(new Internal::AndroidSettingsPage);
    addAutoReleasedObject(new Internal::AndroidQtVersionFactory);
    addAutoReleasedObject(new Internal::AndroidToolChainFactory);
    addAutoReleasedObject(new Internal::AndroidDeployConfigurationFactory);
    addAutoReleasedObject(new Internal::AndroidDeviceFactory);
    return true;
}

void AndroidPlugin::extensionsInitialized()
{
    ProjectExplorer::DeviceManager *dm = ProjectExplorer::DeviceManager::instance();
    if (dm->find(Core::Id(Constants::ANDROID_DEVICE_ID)).isNull())
        dm->addDevice(ProjectExplorer::IDevice::Ptr(new Internal::AndroidDevice));
}

} // namespace Android

Q_EXPORT_PLUGIN(Android::AndroidPlugin)
