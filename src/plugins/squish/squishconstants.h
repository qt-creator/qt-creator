// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

namespace Squish::Constants {

const char SQUISH_ID[]                    = "SquishPlugin.Squish";
const char SQUISH_CONTEXT[]               = "Squish";
const char SQUISH_SETTINGS_CATEGORY[]     = "ZYY.Squish";

// MIME type defined by Squish plugin
const char SQUISH_OBJECTSMAP_MIMETYPE[] = "text/squish-objectsmap";
const char OBJECTSMAP_EDITOR_ID[]       = "Squish.ObjectsMapEditor";

} // namespace Squish::Constants

namespace Squish::Internal {

// SquishProcess related enums
enum SquishProcessState { Idle, Starting, Started, StartFailed, Stopped, StopFailed };

enum class RunnerState {
    None,
    Starting,
    Running,
    RunRequested,
    Interrupted,
    InterruptRequested,
    CancelRequested,
    CancelRequestedWhileInterrupted,
    Canceled,
    Finished
};

enum class StepMode { Continue, StepIn, StepOver, StepOut };

} // namespace Squish::Internal
