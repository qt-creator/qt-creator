// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "debuggericons.h"

using namespace Utils;

namespace Debugger::Icons {

const Icon BREAKPOINT({
        {":/utils/images/filledcircle.png", Theme::IconsErrorColor}}, Icon::Tint);
const Icon BREAKPOINT_DISABLED({
        {":/debugger/images/breakpoint_disabled.png", Theme::IconsErrorColor}}, Icon::Tint);
const Icon BREAKPOINT_PENDING({
        {":/utils/images/filledcircle.png", Theme::IconsErrorColor},
        {":/debugger/images/breakpoint_pending_overlay.png", Theme::PanelTextColorDark}}, Icon::IconStyleOptions(Icon::Tint | Icon::PunchEdges));
const Icon BREAKPOINT_WITH_LOCATION({
        {":/utils/images/filledcircle.png", Theme::IconsErrorColor},
        {":/debugger/images/location.png", Theme::IconsWarningToolBarColor}}, Icon::Tint);
const Icon BREAKPOINTS(
        ":/debugger/images/debugger_breakpoints.png");
const Icon WATCHPOINT({
        {":/utils/images/eye_open.png", Theme::TextColorNormal}}, Icon::Tint);
const Icon TRACEPOINT({
        {":/utils/images/eye_open.png", Theme::TextColorNormal},
        {":/debugger/images/tracepointoverlay.png", Theme::TextColorNormal}}, Icon::Tint | Icon::PunchEdges);
const Icon TRACEPOINT_TOOLBAR({
        {":/utils/images/eye_open.png", Theme::IconsBaseColor},
        {":/debugger/images/tracepointoverlay.png", Theme::IconsBaseColor}});
const Icon CONTINUE(
        ":/debugger/images/debugger_continue.png");
const Icon CONTINUE_FLAT({
        {":/debugger/images/debugger_continue_1_mask.png", Theme::IconsInterruptToolBarColor},
        {":/debugger/images/debugger_continue_2_mask.png", Theme::IconsRunToolBarColor},
        {":/projectexplorer/images/debugger_beetle_mask.png", Theme::IconsDebugColor}});
const Icon DEBUG_CONTINUE_SMALL({
        {":/utils/images/continue_1_small.png", Theme::IconsInterruptColor},
        {":/utils/images/continue_2_small.png", Theme::IconsRunColor},
        {":/utils/images/debugger_overlay_small.png", Theme::PanelTextColorMid}}, Icon::MenuTintedStyle);
const Icon DEBUG_CONTINUE_SMALL_TOOLBAR({
        {":/utils/images/continue_1_small.png", Theme::IconsInterruptToolBarColor},
        {":/utils/images/continue_2_small.png", Theme::IconsRunToolBarColor},
        {":/utils/images/debugger_overlay_small.png", Theme::IconsDebugColor}});
const Icon INTERRUPT(
        ":/debugger/images/debugger_interrupt.png");
const Icon INTERRUPT_FLAT({
        {":/debugger/images/debugger_interrupt_mask.png", Theme::IconsInterruptToolBarColor},
        {":/projectexplorer/images/debugger_beetle_mask.png", Theme::IconsDebugColor}});
const Icon STOP(
        ":/debugger/images/debugger_stop.png");
const Icon STOP_FLAT({
        {":/debugger/images/debugger_stop_mask.png", Theme::IconsStopToolBarColor},
        {":/projectexplorer/images/debugger_beetle_mask.png", Theme::IconsDebugColor}});
const Icon DEBUG_INTERRUPT_SMALL({
        {":/utils/images/interrupt_small.png", Theme::IconsInterruptColor},
        {":/utils/images/debugger_overlay_small.png", Theme::PanelTextColorMid}}, Icon::MenuTintedStyle);
const Icon DEBUG_INTERRUPT_SMALL_TOOLBAR({
        {":/utils/images/interrupt_small.png", Theme::IconsInterruptToolBarColor},
        {":/utils/images/debugger_overlay_small.png", Theme::IconsDebugColor}});
const Icon DEBUG_EXIT_SMALL({
        {":/utils/images/stop_small.png", Theme::IconsStopColor},
        {":/utils/images/debugger_overlay_small.png", Theme::PanelTextColorMid}}, Icon::MenuTintedStyle);
const Icon DEBUG_EXIT_SMALL_TOOLBAR({
        {":/utils/images/stop_small.png", Theme::IconsStopToolBarColor},
        {":/utils/images/debugger_overlay_small.png", Theme::IconsDebugColor}});
const Icon LOCATION({
        {":/debugger/images/location_background.png", Theme::IconsCodeModelOverlayForegroundColor},
        {":/debugger/images/location.png", Theme::IconsWarningToolBarColor}}, Icon::Tint);
const Icon REVERSE_LOCATION({
        {":/debugger/images/debugger_reversemode_background.png", Theme::IconsCodeModelOverlayForegroundColor},
        {":/debugger/images/debugger_reversemode.png", Theme::IconsWarningToolBarColor}}, Icon::Tint);
const Icon REVERSE_MODE({
        {":/debugger/images/debugger_reversemode_background.png", Theme::IconsCodeModelOverlayForegroundColor},
        {":/debugger/images/debugger_reversemode.png", Theme::IconsInfoColor}}, Icon::Tint);
const Icon DIRECTION_BACKWARD({
        {":/debugger/images/debugger_reversemode_background.png", Theme::IconsCodeModelOverlayForegroundColor},
        {":/debugger/images/debugger_reversemode.png", Theme::IconsInfoColor}}, Icon::Tint);
const Icon DIRECTION_FORWARD({
        {":/debugger/images/location_background.png", Theme::IconsCodeModelOverlayForegroundColor},
        {":/debugger/images/location.png", Theme::IconsInfoColor}}, Icon::Tint);
const Icon APP_ON_TOP({
        {":/utils/images/app-on-top.png", Theme::PanelTextColorMid}}, Icon::MenuTintedStyle);
const Icon APP_ON_TOP_TOOLBAR({
        {":/utils/images/app-on-top.png", Theme::IconsBaseColor}});
const Icon SELECT({
        {":/utils/images/select.png", Theme::PanelTextColorMid}}, Icon::MenuTintedStyle);
const Icon SELECT_TOOLBAR({
        {":/utils/images/select.png", Theme::IconsBaseColor}});
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

const Icon MACOS_TOUCHBAR_DEBUG(
        ":/debugger/images/macos_touchbar_debug.png");
const Icon MACOS_TOUCHBAR_DEBUG_EXIT(
        ":/debugger/images/macos_touchbar_debug_exit.png");
const Icon MACOS_TOUCHBAR_DEBUG_INTERRUPT(
        ":/debugger/images/macos_touchbar_debug_interrupt.png");
const Icon MACOS_TOUCHBAR_DEBUG_CONTINUE(
        ":/debugger/images/macos_touchbar_debug_continue.png");
const Icon MACOS_TOUCHBAR_DEBUG_STEP_OVER(
        ":/debugger/images/macos_touchbar_debug_step_over.png");
const Icon MACOS_TOUCHBAR_DEBUG_STEP_INTO(
        ":/debugger/images/macos_touchbar_debug_step_into.png");
const Icon MACOS_TOUCHBAR_DEBUG_STEP_OUT(
        ":/debugger/images/macos_touchbar_debug_step_out.png");

} // Debugger::Icons
