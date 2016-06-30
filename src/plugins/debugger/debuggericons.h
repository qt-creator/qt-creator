/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "debugger_global.h"

#include <utils/icon.h>

namespace Debugger {
namespace Icons {

// Used in QmlProfiler.
DEBUGGER_EXPORT extern const Utils::Icon RECORD_ON;
DEBUGGER_EXPORT extern const Utils::Icon RECORD_OFF;

extern const Utils::Icon BREAKPOINT;
extern const Utils::Icon BREAKPOINT_DISABLED;
extern const Utils::Icon BREAKPOINT_PENDING;
extern const Utils::Icon BREAKPOINTS;
extern const Utils::Icon WATCHPOINT;
extern const Utils::Icon TRACEPOINT;
extern const Utils::Icon CONTINUE;
extern const Utils::Icon CONTINUE_FLAT;
extern const Utils::Icon DEBUG_CONTINUE_SMALL;
extern const Utils::Icon DEBUG_CONTINUE_SMALL_TOOLBAR;
extern const Utils::Icon INTERRUPT;
extern const Utils::Icon INTERRUPT_FLAT;
extern const Utils::Icon DEBUG_INTERRUPT_SMALL;
extern const Utils::Icon DEBUG_INTERRUPT_SMALL_TOOLBAR;
extern const Utils::Icon DEBUG_EXIT_SMALL;
extern const Utils::Icon DEBUG_EXIT_SMALL_TOOLBAR;
extern const Utils::Icon LOCATION;
extern const Utils::Icon REVERSE_MODE;
extern const Utils::Icon APP_ON_TOP;
extern const Utils::Icon APP_ON_TOP_TOOLBAR;
extern const Utils::Icon SELECT;
extern const Utils::Icon SELECT_TOOLBAR;
extern const Utils::Icon EMPTY;

extern const Utils::Icon STEP_OVER;
extern const Utils::Icon STEP_OVER_TOOLBAR;
extern const Utils::Icon STEP_INTO;
extern const Utils::Icon STEP_INTO_TOOLBAR;
extern const Utils::Icon STEP_OUT;
extern const Utils::Icon STEP_OUT_TOOLBAR;
extern const Utils::Icon RESTART;
extern const Utils::Icon RESTART_TOOLBAR;
extern const Utils::Icon SINGLE_INSTRUCTION_MODE;

extern const Utils::Icon MODE_DEBUGGER_CLASSIC;
extern const Utils::Icon MODE_DEBUGGER_FLAT;
extern const Utils::Icon MODE_DEBUGGER_FLAT_ACTIVE;

} // namespace Icons
} // namespace Debugger
