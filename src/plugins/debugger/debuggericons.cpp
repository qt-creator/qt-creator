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

#include "debuggericons.h"

using namespace Utils;

namespace Debugger {
namespace Icons {

const Icon BREAKPOINT({
        {":/utils/images/filledcircle.png", Theme::IconsErrorColor}}, Icon::Tint);
const Icon BREAKPOINT_DISABLED({
        {":/debugger/images/breakpoint_disabled.png", Theme::IconsErrorColor}}, Icon::Tint);
const Icon BREAKPOINT_PENDING({
        {":/utils/images/filledcircle.png", Theme::IconsErrorColor},
        {":/debugger/images/breakpoint_pending_overlay.png", Theme::PanelTextColorDark}}, Icon::IconStyleOptions(Icon::Tint | Icon::PunchEdges));
const Icon BREAKPOINTS(
        ":/debugger/images/debugger_breakpoints.png");
const Icon WATCHPOINT({
        {":/utils/images/eye_open.png", Theme::TextColorNormal}}, Icon::Tint);
const Icon TRACEPOINT({
        {":/utils/images/eye_open.png", Theme::TextColorNormal},
        {":/debugger/images/tracepointoverlay.png", Theme::TextColorNormal}}, Icon::Tint | Icon::PunchEdges);
const Icon CONTINUE(
        ":/debugger/images/debugger_continue.png");
const Icon CONTINUE_FLAT({
        {":/debugger/images/debugger_continue_1_mask.png", Theme::IconsInterruptToolBarColor},
        {":/debugger/images/debugger_continue_2_mask.png", Theme::IconsRunToolBarColor},
        {":/projectexplorer/images/debugger_beetle_mask.png", Theme::IconsDebugColor}});
const Icon DEBUG_CONTINUE_SMALL({
        {":/projectexplorer/images/continue_1_small.png", Theme::IconsInterruptColor},
        {":/projectexplorer/images/continue_2_small.png", Theme::IconsRunColor},
        {":/projectexplorer/images/debugger_overlay_small.png", Theme::PanelTextColorMid}}, Icon::MenuTintedStyle);
const Icon DEBUG_CONTINUE_SMALL_TOOLBAR({
        {":/projectexplorer/images/continue_1_small.png", Theme::IconsInterruptToolBarColor},
        {":/projectexplorer/images/continue_2_small.png", Theme::IconsRunToolBarColor},
        {":/projectexplorer/images/debugger_overlay_small.png", Theme::IconsDebugColor}});
const Icon INTERRUPT(
        ":/debugger/images/debugger_interrupt.png");
const Icon INTERRUPT_FLAT({
        {":/debugger/images/debugger_interrupt_mask.png", Theme::IconsInterruptToolBarColor},
        {":/projectexplorer/images/debugger_beetle_mask.png", Theme::IconsDebugColor}});
const Icon DEBUG_INTERRUPT_SMALL({
        {":/utils/images/interrupt_small.png", Theme::IconsInterruptColor},
        {":/projectexplorer/images/debugger_overlay_small.png", Theme::PanelTextColorMid}}, Icon::MenuTintedStyle);
const Icon DEBUG_INTERRUPT_SMALL_TOOLBAR({
        {":/utils/images/interrupt_small.png", Theme::IconsInterruptToolBarColor},
        {":/projectexplorer/images/debugger_overlay_small.png", Theme::IconsDebugColor}});
const Icon DEBUG_EXIT_SMALL({
        {":/utils/images/stop_small.png", Theme::IconsStopColor},
        {":/projectexplorer/images/debugger_overlay_small.png", Theme::PanelTextColorMid}}, Icon::MenuTintedStyle);
const Icon DEBUG_EXIT_SMALL_TOOLBAR({
        {":/utils/images/stop_small.png", Theme::IconsStopToolBarColor},
        {":/projectexplorer/images/debugger_overlay_small.png", Theme::IconsDebugColor}});
const Icon LOCATION({
        {":/debugger/images/location_background.png", Theme::IconsCodeModelOverlayForegroundColor},
        {":/debugger/images/location.png", Theme::IconsWarningToolBarColor}}, Icon::Tint);
const Icon REVERSE_MODE({
        {":/debugger/images/debugger_reversemode_background.png", Theme::IconsCodeModelOverlayForegroundColor},
        {":/debugger/images/debugger_reversemode.png", Theme::IconsInfoColor}}, Icon::Tint);
const Icon APP_ON_TOP({
        {":/debugger/images/qml/app-on-top.png", Theme::PanelTextColorMid}}, Icon::MenuTintedStyle);
const Icon APP_ON_TOP_TOOLBAR({
        {":/debugger/images/qml/app-on-top.png", Theme::IconsBaseColor}});
const Icon SELECT({
        {":/debugger/images/qml/select.png", Theme::PanelTextColorMid}}, Icon::MenuTintedStyle);
const Icon SELECT_TOOLBAR({
        {":/debugger/images/qml/select.png", Theme::IconsBaseColor}});
const Icon EMPTY(
        ":/debugger/images/debugger_empty_14.png");
const Icon RECORD_ON({
        {":/debugger/images/recordfill.png", Theme::IconsStopColor},
        {":/debugger/images/recordoutline.png", Theme::IconsBaseColor}}, Icon::Tint | Icon::DropShadow);
const Icon RECORD_OFF({
        {":/debugger/images/recordfill.png", Theme::IconsDisabledColor},
        {":/debugger/images/recordoutline.png", Theme::IconsBaseColor}}, Icon::Tint | Icon::DropShadow);

const Icon STEP_OVER({
        {":/debugger/images/debugger_stepover_small.png", Theme::PanelTextColorMid}}, Icon::MenuTintedStyle);
const Icon STEP_OVER_TOOLBAR({
        {":/debugger/images/debugger_stepover_small.png", Theme::IconsBaseColor}});
const Icon STEP_INTO({
        {":/debugger/images/debugger_stepinto_small.png", Theme::PanelTextColorMid}}, Icon::MenuTintedStyle);
const Icon STEP_INTO_TOOLBAR({
        {":/debugger/images/debugger_stepinto_small.png", Theme::IconsBaseColor}});
const Icon STEP_OUT({
        {":/debugger/images/debugger_stepout_small.png", Theme::PanelTextColorMid}}, Icon::MenuTintedStyle);
const Icon STEP_OUT_TOOLBAR({
        {":/debugger/images/debugger_stepout_small.png", Theme::IconsBaseColor}});
const Icon RESTART({
        {":/debugger/images/debugger_restart_small.png", Theme::PanelTextColorMid}}, Icon::MenuTintedStyle);
const Icon RESTART_TOOLBAR({
        {":/debugger/images/debugger_restart_small.png", Theme::IconsRunToolBarColor}});
const Icon SINGLE_INSTRUCTION_MODE({
        {":/debugger/images/debugger_singleinstructionmode.png", Theme::IconsBaseColor}});

const Icon MODE_DEBUGGER_CLASSIC(
        ":/debugger/images/mode_debug.png");
const Icon MODE_DEBUGGER_FLAT({
        {":/debugger/images/mode_debug_mask.png", Theme::IconsBaseColor}});
const Icon MODE_DEBUGGER_FLAT_ACTIVE({
        {":/debugger/images/mode_debug_mask.png", Theme::IconsModeDebugActiveColor}});

} // namespace Icons
} // namespace Debugger
