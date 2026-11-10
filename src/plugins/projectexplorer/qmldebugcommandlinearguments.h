// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include <utils/qmldebugcommandlinearguments.h>

#include <utils/id.h>

namespace ProjectExplorer {

using enum Utils::QmlDebugServicesPreset;
using Utils::QmlDebugServicesPreset;
using Utils::qmlDebugCommandLineArguments;
using Utils::qmlDebugDesktopTcpArguments;
using Utils::qmlDebugLocalArguments;
using Utils::qmlDebugNativeArguments;
using Utils::qmlDebugServices;
using Utils::qmlDebugTcpArguments;

PROJECTEXPLORER_EXPORT Utils::Id runnerIdForRunMode(Utils::Id runMode);

PROJECTEXPLORER_EXPORT QmlDebugServicesPreset servicesForRunMode(Utils::Id runMode);

} // namespace ProjectExplorer
