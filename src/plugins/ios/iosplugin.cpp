/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
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

#include "iosplugin.h"

#include "iosbuildstep.h"
#include "iosconfigurations.h"
#include "iosconstants.h"
#include "iosdeployconfiguration.h"
#include "iosdeploystepfactory.h"
#include "iosdevicefactory.h"
#include "iosmanager.h"
#include "iosdsymbuildstep.h"
#include "iosqtversionfactory.h"
#include "iosrunfactories.h"
#include "iossettingspage.h"
#include "iossimulator.h"
#include "iossimulatorfactory.h"
#include "iostoolhandler.h"

#include <projectexplorer/kitmanager.h>
#include <qtsupport/qtversionmanager.h>

#include <QtPlugin>

#include <projectexplorer/devicesupport/devicemanager.h>

namespace Ios {
namespace Internal {
Q_LOGGING_CATEGORY(iosLog, "qtc.ios.common")
}

IosPlugin::IosPlugin()
{
    qRegisterMetaType<Ios::IosToolHandler::Dict>("Ios::IosToolHandler::Dict");
}

bool IosPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorMessage);

    Internal::IosConfigurations::initialize();

    addAutoReleasedObject(new Internal::IosRunControlFactory);
    addAutoReleasedObject(new Internal::IosRunConfigurationFactory);
    addAutoReleasedObject(new Internal::IosSettingsPage);
    addAutoReleasedObject(new Internal::IosQtVersionFactory);
    addAutoReleasedObject(new Internal::IosDeviceFactory);
    addAutoReleasedObject(new Internal::IosSimulatorFactory);
    addAutoReleasedObject(new Internal::IosBuildStepFactory);
    addAutoReleasedObject(new Internal::IosDeployStepFactory);
    addAutoReleasedObject(new Internal::IosDsymBuildStepFactory);
    addAutoReleasedObject(new Internal::IosDeployConfigurationFactory);

    return true;
}

void IosPlugin::extensionsInitialized()
{
    connect(ProjectExplorer::KitManager::instance(), SIGNAL(kitsLoaded()),
            this, SLOT(kitsRestored()));
}

void IosPlugin::kitsRestored()
{
    disconnect(ProjectExplorer::KitManager::instance(), SIGNAL(kitsLoaded()),
               this, SLOT(kitsRestored()));
    Internal::IosConfigurations::updateAutomaticKitList();
    connect(QtSupport::QtVersionManager::instance(),
            SIGNAL(qtVersionsChanged(QList<int>,QList<int>,QList<int>)),
            Internal::IosConfigurations::instance(),
            SLOT(updateAutomaticKitList()));
}

} // namespace Ios
