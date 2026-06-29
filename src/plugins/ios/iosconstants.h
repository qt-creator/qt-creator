// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>
#include <QLoggingCategory>

namespace Ios {
namespace Internal {
Q_DECLARE_LOGGING_CATEGORY(iosLog)
} // namespace Internal

namespace Constants {
inline constexpr char IOS_SETTINGS_ID[] = "CC.Ios Configurations";
inline constexpr char IOSQT[] = "Qt4ProjectManager.QtVersion.Ios"; // this literal is replicated to avoid dependencies

inline constexpr char IOS_DEVICE_TYPE[] = "Ios.Device.Type";
inline constexpr char IOS_SIMULATOR_TYPE[] = "Ios.Simulator.Type";
inline constexpr char IOS_DEVICE_ID[] = "iOS Device ";
inline constexpr char IOS_SIMULATOR_DEVICE_ID[] = "iOS Simulator Device ";
inline constexpr char IOS_PRESET_BUILD_STEP_ID[] = "Ios.IosPresetBuildStep";
inline constexpr char IOS_DSYM_BUILD_STEP_ID[] = "Ios.IosDsymBuildStep";
inline constexpr char IOS_DEPLOY_STEP_ID[] = "Qt4ProjectManager.IosDeployStep";
inline constexpr char IOS_RUNCONFIG_ID[] = "Qt4ProjectManager.IosRunConfiguration:";

inline constexpr char IOS_EXECUTION_TYPE_ID[] = "IosExecutionType";

inline constexpr char IosTarget[] = "IosTarget"; // QString
inline constexpr char IosBuildDir[] = "IosBuildDir"; // QString
inline constexpr char IosCmakeGenerator[] = "IosCmakeGenerator";

const quint16 IOS_DEVICE_PORT_START = 30000;
const quint16 IOS_DEVICE_PORT_END = 31000;

inline constexpr char EXTRA_INFO_KEY[] = "extraInfo";

} // namespace Constants;
} // namespace Ios
