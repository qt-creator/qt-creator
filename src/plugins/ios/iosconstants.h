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
const char IOS_SETTINGS_ID[] = "CC.Ios Configurations";
const char IOSQT[] = "Qt4ProjectManager.QtVersion.Ios"; // this literal is replicated to avoid dependencies

const char IOS_DEVICE_TYPE[] = "Ios.Device.Type";
const char IOS_SIMULATOR_TYPE[] = "Ios.Simulator.Type";
const char IOS_DEVICE_ID[] = "iOS Device ";
const char IOS_SIMULATOR_DEVICE_ID[] = "iOS Simulator Device ";
const char IOS_PRESET_BUILD_STEP_ID[] = "Ios.IosPresetBuildStep";
const char IOS_DSYM_BUILD_STEP_ID[] = "Ios.IosDsymBuildStep";
const char IOS_DEPLOY_STEP_ID[] = "Qt4ProjectManager.IosDeployStep";
const char IOS_RUNCONFIG_ID[] = "Qt4ProjectManager.IosRunConfiguration:";

const char IosTarget[] = "IosTarget"; // QString
const char IosBuildDir[] = "IosBuildDir"; // QString
const char IosCmakeGenerator[] = "IosCmakeGenerator";

const quint16 IOS_DEVICE_PORT_START = 30000;
const quint16 IOS_DEVICE_PORT_END = 31000;
const quint16 IOS_SIMULATOR_PORT_START = 30000;
const quint16 IOS_SIMULATOR_PORT_END = 31000;

const char EXTRA_INFO_KEY[] = "extraInfo";

} // namespace Constants;
} // namespace Ios
