// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/id.h>

namespace Android::Constants {

const char ANDROID_SETTINGS_ID[] = "BB.Android Configurations";
const char ANDROID_TOOLCHAIN_TYPEID[] = "Qt4ProjectManager.ToolChain.Android";
const char ANDROID_QT_TYPE[] = "Qt4ProjectManager.QtVersion.Android";

const char ANDROID_AM_START_ARGS[] = "Android.AmStartArgs";
// Note: Can be set on RunConfiguration using an aspect and/or
// the AndroidRunnerWorker using recordData()
const char ANDROID_PRESTARTSHELLCMDLIST[] = "Android.PreStartShellCmdList";
const char ANDROID_POSTFINISHSHELLCMDLIST[] = "Android.PostFinishShellCmdList";

const char ANDROID_DEVICE_TYPE[] = "Android.Device.Type";
const char ANDROID_DEVICE_ID[] = "Android Device";
const char ANDROID_MANIFEST_MIME_TYPE[] = "application/vnd.google.android.android_manifest";
const char ANDROID_MANIFEST_EDITOR_ID[] = "Android.AndroidManifestEditor.Id";
const char ANDROID_MANIFEST_EDITOR_CONTEXT[] = "Android.AndroidManifestEditor.Id";

const char ANDROID_KIT_NDK[] = "Android.NDK";
const char ANDROID_KIT_SDK[] = "Android.SDK";

const char ANDROID_BUILD_DIRECTORY[] = "android-build";
const char ANDROID_APP_PROCESS_DIRECTORY[] = "android-app-process";
const char JAVA_EDITOR_ID[] = "java.editor";
const char JLS_SETTINGS_ID[] = "Java::JLSSettingsID";
const char JAVA_MIMETYPE[] = "text/x-java";
const char ANDROID_ARCHITECTURE[] = "Android.Architecture";
const char ANDROID_PACKAGE_SOURCE_DIR[] = "ANDROID_PACKAGE_SOURCE_DIR";
const char ANDROID_EXTRA_LIBS[] = "ANDROID_EXTRA_LIBS";
const char ANDROID_ABI[] = "ANDROID_ABI";
const char ANDROID_TARGET_ARCH[] = "ANDROID_TARGET_ARCH";
const char ANDROID_ABIS[] = "ANDROID_ABIS";
const char ANDROID_APPLICATION_ARGUMENTS[] = "ANDROID_APPLICATION_ARGUMENTS";
const char ANDROID_DEPLOYMENT_SETTINGS_FILE[] = "ANDROID_DEPLOYMENT_SETTINGS_FILE";
const char ANDROID_SO_LIBS_PATHS[] = "ANDROID_SO_LIBS_PATHS";
const char JAVA_HOME_ENV_VAR[] = "JAVA_HOME";

const char ANDROID_RUNCONFIG_ID[] = "Qt4ProjectManager.AndroidRunConfiguration:";
const char ANDROID_PACKAGE_INSTALL_STEP_ID[] = "Qt4ProjectManager.AndroidPackageInstallationStep";
const char ANDROID_BUILD_APK_ID[] = "QmakeProjectManager.AndroidBuildApkStep";
const char ANDROID_DEPLOY_QT_ID[] = "Qt4ProjectManager.AndroidDeployQtStep";

const char AndroidPackageSourceDir[] = "AndroidPackageSourceDir"; // QString
const char AndroidDeploySettingsFile[] = "AndroidDeploySettingsFile"; // QString
const char AndroidExtraLibs[] = "AndroidExtraLibs";  // QStringList
const char AndroidAbi[] = "AndroidAbi"; // QString
const char AndroidAbis[] = "AndroidAbis"; // QStringList
const char AndroidMkSpecAbis[] = "AndroidMkSpecAbis"; // QStringList
const char AndroidSoLibPath[] = "AndroidSoLibPath"; // QStringList
const char AndroidTargets[] = "AndroidTargets"; // QStringList
const char AndroidApplicationArgs[] = "AndroidApplicationArgs"; // QString

// For qbs support
const char AndroidApk[] = "Android.APK"; // QStringList
const char AndroidManifest[] = "Android.Manifest"; // QStringList

const char AndroidNdkPlatform[] = "AndroidNdkPlatform"; //QString
const char NdkLocation[] = "NdkLocation"; // FileName
const char SdkLocation[] = "SdkLocation"; // FileName

// Android Device
const Utils::Id AndroidSerialNumber = "AndroidSerialNumber";
const Utils::Id AndroidAvdName = "AndroidAvdName";
const Utils::Id AndroidCpuAbi = "AndroidCpuAbi";
const Utils::Id AndroidSdk = "AndroidSdk";
const Utils::Id AndroidAvdPath = "AndroidAvdPath";

// SDK Tools
const char cmdlineToolsName[] = "cmdline-tools";
const char ndkPackageName[] = "ndk";

// For AndroidQtVersion
const char ArmToolsDisplayName[] = "arm";
const char ArmV7ToolsDisplayName[] = "armv7";
const char X86ToolsDisplayName[] = "i686";
const char AArch64ToolsDisplayName[] = "aarch64";
const char X86_64ToolsDisplayName[] = "x86_64";

} // Android::Constants
