// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/id.h>

namespace Qdb {
namespace Constants {

const char QdbLinuxOsType[] = "QdbLinuxOsType";

const char QdbDeployConfigurationId[] = "Qt4ProjectManager.Qdb.QdbDeployConfiguration";

const char QdbStopApplicationStepId[] = "Qdb.StopApplicationStep";
const char QdbMakeDefaultAppStepId[] = "Qdb.MakeDefaultAppStep";

const Utils::Id QdbHardwareDevicePrefix = "QdbHardwareDevice";
const char AppcontrollerFilepath[] = "/usr/bin/appcontroller";

} // namespace Constants

namespace Internal {
enum VmState { VmReady, VmNotReady, VmShutDown };
}

} // namespace Qdb
