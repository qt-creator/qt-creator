// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/id.h>

namespace Android::Constants {

inline constexpr char ANDROID_SETTINGS_ID[] = "BB.Android Configurations";
inline constexpr char ANDROID_TOOLCHAIN_TYPEID[] = "Qt4ProjectManager.ToolChain.Android";
inline constexpr char ANDROID_QT_TYPE[] = "Qt4ProjectManager.QtVersion.Android";

inline constexpr char ANDROID_AM_START_ARGS[] = "Android.AmStartArgs";
// Note: Can be set on RunConfiguration using an aspect and/or
// the AndroidRunnerWorker using recordData()
inline constexpr char ANDROID_PRESTARTSHELLCMDLIST[] = "Android.PreStartShellCmdList";
inline constexpr char ANDROID_POSTFINISHSHELLCMDLIST[] = "Android.PostFinishShellCmdList";

inline constexpr char ANDROID_DEVICE_TYPE[] = "Android.Device.Type";
inline constexpr char ANDROID_DEVICE_ID[] = "Android Device";
inline constexpr char ANDROID_MANIFEST_MIME_TYPE[] = "application/vnd.google.android.android_manifest";
inline constexpr char ANDROID_MANIFEST_EDITOR_ID[] = "Android.AndroidManifestEditor.Id";
inline constexpr char ANDROID_MANIFEST_EDITOR_CONTEXT[] = "Android.AndroidManifestEditor.Id";

inline constexpr char ANDROID_KIT_NDK[] = "Android.NDK";
inline constexpr char ANDROID_KIT_SDK[] = "Android.SDK";

inline constexpr char ANDROID_BUILD_DIRECTORY[] = "android-build";
inline constexpr char JAVA_EDITOR_ID[] = "java.editor";
inline constexpr char JLS_SETTINGS_ID[] = "Java::JLSSettingsID";
inline constexpr char ANDROID_ARCHITECTURE[] = "Android.Architecture";
inline constexpr char ANDROID_PACKAGE_SOURCE_DIR[] = "ANDROID_PACKAGE_SOURCE_DIR";
inline constexpr char QT_ANDROID_PACKAGE_SOURCE_DIR[] = "QT_ANDROID_PACKAGE_SOURCE_DIR";
inline constexpr char ANDROID_EXTRA_LIBS[] = "ANDROID_EXTRA_LIBS";
inline constexpr char ANDROID_ABI[] = "ANDROID_ABI";
inline constexpr char ANDROID_TARGET_ARCH[] = "ANDROID_TARGET_ARCH";
inline constexpr char ANDROID_ABIS[] = "ANDROID_ABIS";
inline constexpr char ANDROID_APPLICATION_ARGUMENTS[] = "ANDROID_APPLICATION_ARGUMENTS";
inline constexpr char ANDROID_DEPLOYMENT_SETTINGS_FILE[] = "ANDROID_DEPLOYMENT_SETTINGS_FILE";
inline constexpr char ANDROID_SO_LIBS_PATHS[] = "ANDROID_SO_LIBS_PATHS";
inline constexpr char JAVA_HOME_ENV_VAR[] = "JAVA_HOME";

inline constexpr char ANDROID_RUNCONFIG_ID[] = "Qt4ProjectManager.AndroidRunConfiguration:";
inline constexpr char ANDROID_PACKAGE_INSTALL_STEP_ID[] = "Qt4ProjectManager.AndroidPackageInstallationStep";
inline constexpr char ANDROID_BUILD_APK_ID[] = "QmakeProjectManager.AndroidBuildApkStep";
inline constexpr char ANDROID_DEPLOY_QT_ID[] = "Qt4ProjectManager.AndroidDeployQtStep";

inline constexpr char ANDROID_EXECUTION_TYPE_ID[] = "AndroidExecutionType";

inline constexpr char AndroidPackageSourceDir[] = "AndroidPackageSourceDir"; // QString
inline constexpr char AndroidDeploySettingsFile[] = "AndroidDeploySettingsFile"; // QString
inline constexpr char AndroidExtraLibs[] = "AndroidExtraLibs";  // QStringList
inline constexpr char AndroidAbi[] = "AndroidAbi"; // QString
inline constexpr char AndroidAbis[] = "AndroidAbis"; // QStringList
inline constexpr char AndroidMkSpecAbis[] = "AndroidMkSpecAbis"; // QStringList
inline constexpr char AndroidSoLibPath[] = "AndroidSoLibPath"; // QStringList
inline constexpr char AndroidTargets[] = "AndroidTargets"; // QStringList
inline constexpr char AndroidApplicationArgs[] = "AndroidApplicationArgs"; // QString
inline constexpr char AndroidClassPaths[] = "AndroidClassPath"; // QStringList

inline constexpr char AndroidBuildTargetDirSupport[] = "AndroidBuildTargetDirSupport"; // bool
inline constexpr char UseAndroidBuildTargetDir[] = "UseAndroidBuildTargetDir"; // bool

// For qbs support
inline constexpr char AndroidApk[] = "Android.APK"; // QStringList
inline constexpr char AndroidManifest[] = "Android.Manifest"; // QStringList

inline constexpr char AndroidNdkPlatform[] = "AndroidNdkPlatform"; //QString
inline constexpr char NdkLocation[] = "NdkLocation"; // FileName
inline constexpr char SdkLocation[] = "SdkLocation"; // FileName

// Android Device
const Utils::Id AndroidSerialNumber = "AndroidSerialNumber";
const Utils::Id AndroidAvdName = "AndroidAvdName";
const Utils::Id AndroidCpuAbi = "AndroidCpuAbi";
const Utils::Id AndroidSdk = "AndroidSdk";
const Utils::Id AndroidAvdPath = "AndroidAvdPath";

// SDK Tools
inline constexpr char cmdlineToolsName[] = "cmdline-tools";
inline constexpr char ndkPackageName[] = "ndk";
inline constexpr char platformsPackageName[] = "platforms";
inline constexpr char buildToolsPackageName[] = "build-tools";
inline constexpr char systemImagesPackageName[] = "system-images";

} // Android::Constants
