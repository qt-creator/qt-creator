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
inline constexpr char HVIGOR_ENV_VAR[] = "QT_HARMONYOS_HVIGOR";

// harmonydeployqt reads these to sign the .hap. The values come from the DevEco
// signing configuration in the project's harmonyos-build/build-profile.json5.
inline constexpr char SIGNING_CERT_PATH_ENV_VAR[] = "QT_HARMONYOS_SIGNING_CERT_PATH";
inline constexpr char SIGNING_PROFILE_ENV_VAR[] = "QT_HARMONYOS_SIGNING_PROFILE";
inline constexpr char SIGNING_STORE_FILE_ENV_VAR[] = "QT_HARMONYOS_SIGNING_STORE_FILE";
inline constexpr char SIGNING_KEY_ALIAS_ENV_VAR[] = "QT_HARMONYOS_SIGNING_KEY_ALIAS";
inline constexpr char SIGNING_KEY_PASSWORD_ENV_VAR[] = "QT_HARMONYOS_SIGNING_KEY_PASSWORD";
inline constexpr char SIGNING_STORE_PASSWORD_ENV_VAR[] = "QT_HARMONYOS_SIGNING_STORE_PASSWORD";
inline constexpr char SIGNING_ALG_ENV_VAR[] = "QT_HARMONYOS_SIGNING_ALG";

} // namespace HarmonyOs::Constants
