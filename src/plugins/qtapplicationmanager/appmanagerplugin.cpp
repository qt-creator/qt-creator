// Copyright (C) 2019 Luxoft Sweden AB
// Copyright (C) 2018 Pelagicore AG
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "appmanagerplugin.h"

#include "appmanagerconstants.h"
#include "appmanagercreatepackagestep.h"
#include "appmanagerdeployconfigurationautoswitcher.h"
#include "appmanagerdeployconfigurationfactory.h"
#include "appmanagerdeploypackagestep.h"
#include "appmanagerinstallpackagestep.h"
#include "appmanagermakeinstallstep.h"
#include "appmanagerrunconfiguration.h"
#include "appmanagerruncontrol.h"
#include "appmanagerutilities.h"

#include <coreplugin/helpmanager.h>

#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <qtsupport/qtkitaspect.h>

#include <remotelinux/remotelinux_constants.h>

using namespace Core;
using namespace ProjectExplorer;
using namespace QtSupport;

namespace AppManager::Internal {

const char QdbLinuxOsType[] = "QdbLinuxOsType";
const char QdbLinuxEmulatorOsType[] = "QdbLinuxEmulatorOsType";

void cloneAutodetectedBoot2QtKits()
{
    if (auto kitManager = KitManager::instance()) {
        QHash<QtVersion *, Kit *> boot2QtKits;
        QHash<QtVersion *, Kit *> genericLinuxDeviceKits;
        for (auto kit : kitManager->kits()) {
            if (auto qtVersion = QtKitAspect::qtVersion(kit)) {
                const auto deviceTypeId = DeviceTypeKitAspect::deviceTypeId(kit);
                if (deviceTypeId == RemoteLinux::Constants::GenericLinuxOsType) {
                    genericLinuxDeviceKits[qtVersion] = kit;
                } else if (kit->isAutoDetected()) {
                    if (deviceTypeId == QdbLinuxOsType) {
                        boot2QtKits[qtVersion] = kit;
                    } else if (deviceTypeId == QdbLinuxEmulatorOsType) {
                        boot2QtKits[qtVersion] = kit;
                    }
                }
            }
        }
        for (auto qtVersion : boot2QtKits.keys()) {
            if (!genericLinuxDeviceKits.contains(qtVersion)) {
                const auto boot2QtKit = boot2QtKits.value(qtVersion);
                const auto copyIntoKit = [boot2QtKit](Kit *k) {
                    k->copyFrom(boot2QtKit);
                    k->setAutoDetected(false);
                    k->setUnexpandedDisplayName(QString("%1 for Generic Linux Devices").arg(boot2QtKit->unexpandedDisplayName()));
                    DeviceTypeKitAspect::setDeviceTypeId(k, RemoteLinux::Constants::GenericLinuxOsType);
                };
                if (auto genericLinuxDeviceKit = KitManager::instance()->registerKit(copyIntoKit)) {
                    genericLinuxDeviceKit->setup();
                }
            }
        }
    }
}

class AppManagerPluginPrivate
{
public:
    AppManagerMakeInstallStepFactory makeInstallStepFactory;
    AppManagerCreatePackageStepFactory createPackageStepFactory;
    AppManagerDeployPackageStepFactory deployPackageStepFactory;
    AppManagerInstallPackageStepFactory installPackageStepFactory;

    AppManagerDeployConfigurationAutoSwitcher deployConfigurationAutoSwitcher;
    AppManagerDeployConfigurationFactory deployConfigFactory;

    AppManagerRunConfigurationFactory runConfigFactory;
    AppManagerDebugWorkerFactory debugWorkerFactory;
    AppManagerRunWorkerFactory runWorkerFactory;
};

AppManagerPlugin::~AppManagerPlugin()
{
    delete d;
}

void AppManagerPlugin::initialize()
{
    d = new AppManagerPluginPrivate;
    d->deployConfigurationAutoSwitcher.initialize();

    if (auto kitManager = KitManager::instance()) {
        connect(kitManager, &KitManager::kitsLoaded, &cloneAutodetectedBoot2QtKits);
    }
}

} // AppManager::Internal
