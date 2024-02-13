// Copyright (C) 2019 Luxoft Sweden AB
// Copyright (C) 2018 Pelagicore AG
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace AppManager {
namespace Constants {

const char APPMAN[] = "appman";
const char APPMAN_CONTROLLER[] = "appman-controller";
const char APPMAN_PACKAGER[] = "appman-packager";
const char APPMAN_LAUNCHER_QML[] = "appman-launcher-qml";

const char DEBUG_LAUNCHER_ID[] = "ApplicationManagerPlugin.Debug.Launcher";
const char DEPLOYCONFIGURATION_ID[] = "ApplicationManagerPlugin.Deploy.Configuration";
const char MAKE_INSTALL_STEP_ID[] = "ApplicationManagerPlugin.Deploy.MakeInstallStep";
const char CMAKE_PACKAGE_STEP_ID[] = "ApplicationManagerPlugin.Deploy.CMakePackageStep";
const char CREATE_PACKAGE_STEP_ID[] = "ApplicationManagerPlugin.Deploy.CreatePackageStep";
const char DEPLOY_PACKAGE_STEP_ID[] = "ApplicationManagerPlugin.Deploy.DeployPackageStep";
const char INSTALL_PACKAGE_STEP_ID[] = "ApplicationManagerPlugin.Deploy.InstallPackageStep";
const char RUNCONFIGURATION_ID[] = "ApplicationManagerPlugin.Run.Configuration";
const char RUNANDDEBUGCONFIGURATION_ID[] = "ApplicationManagerPlugin.RunAndDebug.Configuration";

const char EXTRADATA_TARGET_ID[] = "ApplicationManagerPlugin.ExtraData.Target";

const char APPMAN_PACKAGE_TARGETS[] = "ApplicationmanagerPackageTargets";

const char QMAKE_AM_MANIFEST_VARIABLE[] = "AM_MANIFEST";
const char QMAKE_AM_PACKAGE_DIR_VARIABLE[] = "AM_PACKAGE_DIR";

const char REMOTE_DEFAULT_BIN_PATH[] = "/usr/bin";
const char REMOTE_DEFAULT_TMP_PATH[] = "/tmp";

} // namespace Constants
} // namespace AppManager
