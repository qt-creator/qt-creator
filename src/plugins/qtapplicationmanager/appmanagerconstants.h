// Copyright (C) 2019 Luxoft Sweden AB
// Copyright (C) 2018 Pelagicore AG
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace AppManager::Constants {

inline constexpr char APPMAN[] = "appman";
inline constexpr char APPMAN_CONTROLLER[] = "appman-controller";
inline constexpr char APPMAN_PACKAGER[] = "appman-packager";
inline constexpr char APPMAN_LAUNCHER_QML[] = "appman-launcher-qml";

inline constexpr char DEBUG_LAUNCHER_ID[] = "ApplicationManagerPlugin.Debug.Launcher";
inline constexpr char DEPLOYCONFIGURATION_ID[] = "ApplicationManagerPlugin.Deploy.Configuration";
inline constexpr char MAKE_INSTALL_STEP_ID[] = "ApplicationManagerPlugin.Deploy.MakeInstallStep";
inline constexpr char CMAKE_PACKAGE_STEP_ID[] = "ApplicationManagerPlugin.Deploy.CMakePackageStep";
inline constexpr char CREATE_PACKAGE_STEP_ID[] = "ApplicationManagerPlugin.Deploy.CreatePackageStep";
inline constexpr char DEPLOY_PACKAGE_STEP_ID[] = "ApplicationManagerPlugin.Deploy.DeployPackageStep";
inline constexpr char INSTALL_PACKAGE_STEP_ID[] = "ApplicationManagerPlugin.Deploy.InstallPackageStep";
inline constexpr char RUNCONFIGURATION_ID[] = "ApplicationManagerPlugin.Run.Configuration";
inline constexpr char RUNANDDEBUGCONFIGURATION_ID[] = "ApplicationManagerPlugin.RunAndDebug.Configuration";

inline constexpr char EXTRADATA_TARGET_ID[] = "ApplicationManagerPlugin.ExtraData.Target";

inline constexpr char APPMAN_PACKAGE_TARGETS[] = "ApplicationmanagerPackageTargets";

inline constexpr char QMAKE_AM_MANIFEST_VARIABLE[] = "AM_MANIFEST";
inline constexpr char QMAKE_AM_PACKAGE_DIR_VARIABLE[] = "AM_PACKAGE_DIR";

inline constexpr char REMOTE_DEFAULT_BIN_PATH[] = "/usr/bin";
inline constexpr char REMOTE_DEFAULT_TMP_PATH[] = "/tmp";

} // namespace AppManager::Constants
