// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/id.h>

namespace HarmonyOs::Constants {

inline constexpr char HARMONYOS_SETTINGS_ID[] = "ZZ.HarmonyOS Configurations";
inline constexpr char HARMONYOS_TOOLCHAIN_TYPEID[] = "Qt4ProjectManager.ToolChain.HarmonyOS";
inline constexpr char HARMONYOS_QT_TYPE[] = "Qt4ProjectManager.QtVersion.HarmonyOS";

inline constexpr char HARMONYOS_DEVICE_TYPE[] = "HarmonyOS.Device.Type";

inline const Utils::Id HARMONYOS_SERIAL_NUMBER = "HarmonyOS.SerialNumber";

inline constexpr char HARMONYOS_DEPLOY_CONFIG_ID[] = "HarmonyOS.DeployConfiguration";
inline constexpr char HARMONYOS_MAKE_HAP_STEP_ID[] = "HarmonyOS.MakeHapStep";
inline constexpr char HARMONYOS_INSTALL_HAP_STEP_ID[] = "HarmonyOS.InstallHapStep";

// Kit
inline constexpr char HARMONYOS_KIT_SDK[] = "HarmonyOS.SDK";
inline constexpr char harmonyOsConfigurationId[] = "HarmonyOSConfiguration";

// Environment variables consumed by the ohos-clang mkspec and the deployment tools
inline constexpr char NATIVE_OHOS_SDK_ENV_VAR[] = "NATIVE_OHOS_SDK";
inline constexpr char DEVECO_SDK_HOME_ENV_VAR[] = "DEVECO_SDK_HOME";

} // namespace HarmonyOs::Constants
