// Copyright (C) 2019 Luxoft Sweden AB
// Copyright (C) 2018 Pelagicore AG
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "appmanagerplugin.h"

#include "appmanagercreatepackagestep.h"
#include "appmanagerdeployconfigurationautoswitcher.h"
#include "appmanagerdeployconfigurationfactory.h"
#include "appmanagerdeploypackagestep.h"
#include "appmanagerinstallpackagestep.h"
#include "appmanagerremoteinstallpackagestep.h"
#include "appmanagermakeinstallstep.h"
#include "appmanagercmakepackagestep.h"
#include "appmanagerrunconfiguration.h"
#include "appmanagerruncontrol.h"

namespace AppManager::Internal {

class AppManagerPluginPrivate
{
public:
    AppManagerRunConfigurationFactory runConfigFactory;
    AppManagerRunWorkerFactory runWorkerFactory;
    AppManagerDebugWorkerFactory debugWorkerFactory;
    AppManagerQmlToolingWorkerFactory toolingWorkerFactory;
};

AppManagerPlugin::~AppManagerPlugin()
{
    delete d;
}

void AppManagerPlugin::initialize()
{
    setupAppManagerCMakePackageStep();
    setupAppManagerMakeInstallStep();
    setupAppManagerCreatePackageStep();
    setupAppManagerDeployPackageStep();
    setupAppManagerInstallPackageStep();
    setupAppManagerRemoteInstallPackageStep();

    setupAppManagerDeployConfiguration();
    setupAppManagerDeployConfigurationAutoSwitcher();

    d = new AppManagerPluginPrivate;
}

} // AppManager::Internal
