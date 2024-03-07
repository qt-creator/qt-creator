// Copyright (C) 2019 Luxoft Sweden AB
// Copyright (C) 2018 Pelagicore AG
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "appmanagercreatepackagestep.h"
#include "appmanagerdeployconfigurationautoswitcher.h"
#include "appmanagerdeployconfigurationfactory.h"
#include "appmanagerdeploypackagestep.h"
#include "appmanagerinstallpackagestep.h"
#include "appmanagercmakepackagestep.h"
#include "appmanagerrunconfiguration.h"
#include "appmanagerruncontrol.h"

#include <extensionsystem/iplugin.h>

namespace AppManager::Internal {

class AppManagerPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "QtApplicationManagerIntegration.json")

    void initialize() final
    {
        setupAppManagerCMakePackageStep();
        setupAppManagerCreatePackageStep();
        setupAppManagerDeployPackageStep();
        setupAppManagerInstallPackageStep();

        setupAppManagerDeployConfiguration();
        setupAppManagerDeployConfigurationAutoSwitcher();

        setupAppManagerRunConfiguration();

        setupAppManagerRunWorker();
        setupAppManagerDebugWorker();
        setupAppManagerQmlToolingWorker();
        setupAppManagerPerfProfilerWorker();
    }
};

} // AppManager::Internal

#include "appmanagerplugin.moc"
