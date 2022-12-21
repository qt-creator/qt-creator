// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "debugger_global.h"

#include <utils/icon.h>

namespace Debugger::Icons {

// Used in QmlProfiler.
DEBUGGER_EXPORT extern const Utils::Icon RECORD_ON;
DEBUGGER_EXPORT extern const Utils::Icon RECORD_OFF;

DEBUGGER_EXPORT extern const Utils::Icon TRACEPOINT;
DEBUGGER_EXPORT extern const Utils::Icon TRACEPOINT_TOOLBAR;

// Used in Squish.
DEBUGGER_EXPORT extern const Utils::Icon LOCATION;
DEBUGGER_EXPORT extern const Utils::Icon DEBUG_CONTINUE_SMALL_TOOLBAR;
DEBUGGER_EXPORT extern const Utils::Icon STEP_OVER_TOOLBAR;
DEBUGGER_EXPORT extern const Utils::Icon STEP_INTO_TOOLBAR;
DEBUGGER_EXPORT extern const Utils::Icon STEP_OUT_TOOLBAR;

extern const Utils::Icon BREAKPOINT;
extern const Utils::Icon BREAKPOINT_DISABLED;
extern const Utils::Icon BREAKPOINT_PENDING;
extern const Utils::Icon BREAKPOINT_WITH_LOCATION;
extern const Utils::Icon BREAKPOINTS;
extern const Utils::Icon WATCHPOINT;
extern const Utils::Icon CONTINUE;
extern const Utils::Icon CONTINUE_FLAT;
extern const Utils::Icon DEBUG_CONTINUE_SMALL;
extern const Utils::Icon INTERRUPT;
extern const Utils::Icon INTERRUPT_FLAT;
extern const Utils::Icon STOP;
extern const Utils::Icon STOP_FLAT;
extern const Utils::Icon DEBUG_INTERRUPT_SMALL;
extern const Utils::Icon DEBUG_INTERRUPT_SMALL_TOOLBAR;
extern const Utils::Icon DEBUG_EXIT_SMALL;
extern const Utils::Icon DEBUG_EXIT_SMALL_TOOLBAR;
extern const Utils::Icon REVERSE_LOCATION;
extern const Utils::Icon REVERSE_MODE;
extern const Utils::Icon DIRECTION_FORWARD;
extern const Utils::Icon DIRECTION_BACKWARD;
extern const Utils::Icon APP_ON_TOP;
extern const Utils::Icon APP_ON_TOP_TOOLBAR;
extern const Utils::Icon SELECT;
extern const Utils::Icon SELECT_TOOLBAR;
extern const Utils::Icon EMPTY;

extern const Utils::Icon STEP_OVER;
extern const Utils::Icon STEP_INTO;
extern const Utils::Icon STEP_OUT;
extern const Utils::Icon RESTART;
extern const Utils::Icon RESTART_TOOLBAR;
extern const Utils::Icon SINGLE_INSTRUCTION_MODE;

extern const Utils::Icon MODE_DEBUGGER_CLASSIC;
extern const Utils::Icon MODE_DEBUGGER_FLAT;
extern const Utils::Icon MODE_DEBUGGER_FLAT_ACTIVE;

extern const Utils::Icon MACOS_TOUCHBAR_DEBUG;
extern const Utils::Icon MACOS_TOUCHBAR_DEBUG_EXIT;
extern const Utils::Icon MACOS_TOUCHBAR_DEBUG_INTERRUPT;
extern const Utils::Icon MACOS_TOUCHBAR_DEBUG_CONTINUE;
extern const Utils::Icon MACOS_TOUCHBAR_DEBUG_STEP_OVER;
extern const Utils::Icon MACOS_TOUCHBAR_DEBUG_STEP_INTO;
extern const Utils::Icon MACOS_TOUCHBAR_DEBUG_STEP_OUT;

} // namespace Debugger::Icons
