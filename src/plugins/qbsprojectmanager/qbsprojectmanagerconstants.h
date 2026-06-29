// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

namespace QbsProjectManager::Constants {

// Contexts
inline constexpr char PROJECT_ID[] = "Qbs.QbsProject";

// Actions:
inline constexpr char ACTION_REPARSE_QBS[] = "Qbs.Reparse";
inline constexpr char ACTION_REPARSE_QBS_CONTEXT[] = "Qbs.ReparseCtx";
inline constexpr char ACTION_BUILD_PRODUCT[] = "Qbs.BuildProduct";
inline constexpr char ACTION_BUILD_SUBPROJECT[] = "Qbs.BuildSubproduct";
inline constexpr char ACTION_CLEAN_PRODUCT[] = "Qbs.CleanProduct";
inline constexpr char ACTION_CLEAN_SUBPROJECT[] = "Qbs.CleanSubproject";
inline constexpr char ACTION_REBUILD_PRODUCT[] = "Qbs.RebuildProduct";
inline constexpr char ACTION_REBUILD_SUBPROJECT[] = "Qbs.RebuildSubproject";

// Ids:
inline constexpr char QBS_BUILDSTEP_ID[] = "Qbs.BuildStep";
inline constexpr char QBS_CLEANSTEP_ID[] = "Qbs.CleanStep";
inline constexpr char QBS_INSTALLSTEP_ID[] = "Qbs.InstallStep";
inline constexpr char QBS_BC_ID[] = "Qbs.QbsBuildConfiguration";

// QBS strings:
inline constexpr char QBS_VARIANT_DEBUG[] = "debug";
inline constexpr char QBS_VARIANT_RELEASE[] = "release";
inline constexpr char QBS_VARIANT_PROFILING[] = "profiling";

inline constexpr char QBS_CONFIG_VARIANT_KEY[] = "qbs.defaultBuildVariant";
inline constexpr char QBS_CONFIG_PROFILE_KEY[] = "qbs.profile";
inline constexpr char QBS_INSTALL_ROOT_KEY[] = "qbs.installRoot";
inline constexpr char QBS_CONFIG_DECLARATIVE_DEBUG_KEY[] = "modules.Qt.declarative.qmlDebugging";
inline constexpr char QBS_CONFIG_QUICK_DEBUG_KEY[] = "modules.Qt.quick.qmlDebugging";
inline constexpr char QBS_CONFIG_QUICK_COMPILER_KEY[] = "modules.Qt.quick.useCompiler";
inline constexpr char QBS_CONFIG_SEPARATE_DEBUG_INFO_KEY[] = "modules.cpp.separateDebugInformation";
inline constexpr char QBS_FORCE_PROBES_KEY[] = "qbspm.forceProbes";
inline constexpr char QBS_RESTORE_BEHAVIOR_KEY[] = "restore-behavior";

// Toolchain related settings:
inline constexpr char QBS_TARGETPLATFORM[] = "qbs.targetPlatform";
inline constexpr char QBS_SYSROOT[] = "qbs.sysroot";
inline constexpr char QBS_ARCHITECTURES[] = "qbs.architectures";
inline constexpr char QBS_ARCHITECTURE[] = "qbs.architecture";
inline constexpr char CPP_TOOLCHAINPATH[] = "cpp.toolchainInstallPath";
inline constexpr char CPP_TOOLCHAINPREFIX[] = "cpp.toolchainPrefix";
inline constexpr char CPP_COMPILERNAME[] = "cpp.compilerName";
inline constexpr char CPP_CCOMPILERNAME[] = "cpp.cCompilerName";
inline constexpr char CPP_CXXCOMPILERNAME[] = "cpp.cxxCompilerName";
inline constexpr char CPP_PLATFORMCOMMONCOMPILERFLAGS[] = "cpp.platformCommonCompilerFlags";
inline constexpr char CPP_PLATFORMLINKERFLAGS[] = "cpp.platformLinkerFlags";
inline constexpr char CPP_VCVARSALLPATH[] = "cpp.vcvarsallPath";
inline constexpr char XCODE_DEVELOPERPATH[] = "xcode.developerPath";
inline constexpr char XCODE_SDK[] = "xcode.sdk";
inline constexpr char JAVA_ADDITIONAL_CLASSPATHS[] = "java.additionalClassPaths";

// Settings page
inline constexpr char QBS_SETTINGS_CATEGORY[]  = "K.Qbs";
inline constexpr char QBS_SETTINGS_TR_CATEGORY[] = QT_TRANSLATE_NOOP("QtC::QbsProjectManager", "Qbs");
inline constexpr char QBS_SETTINGS_CATEGORY_ICON[]  = ":/projectexplorer/images/build.png";

inline constexpr char QBS_PROFILING_ENV[] = "QTC_QBS_PROFILING";

inline constexpr char QBS_TOOL_ID[] = "QbsExecutable";

} // namespace QbsProjectManager::Constants
