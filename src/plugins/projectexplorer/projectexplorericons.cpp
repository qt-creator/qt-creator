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

#include "projectexplorericons.h"

using namespace Utils;

namespace ProjectExplorer {
namespace Icons {

const Icon BUILD(":/projectexplorer/images/build.png");
const Icon BUILD_FLAT({
        {":/projectexplorer/images/build_hammerhandle_mask.png", Theme::IconsBuildHammerHandleColor},
        {":/projectexplorer/images/build_hammerhead_mask.png", Theme::IconsBuildHammerHeadColor}});
const Icon BUILD_SMALL({
        {":/projectexplorer/images/buildhammerhandle.png", Theme::IconsBuildHammerHandleColor},
        {":/projectexplorer/images/buildhammerhead.png", Theme::IconsBuildHammerHeadColor}}, Icon::Tint);
const Icon REBUILD({
        {":/projectexplorer/images/rebuildhammerhandles.png", Theme::IconsBuildHammerHandleColor},
        {":/projectexplorer/images/buildhammerhandle.png", Theme::IconsBuildHammerHandleColor},
        {":/projectexplorer/images/rebuildhammerheads.png", Theme::IconsBuildHammerHeadColor},
        {":/projectexplorer/images/buildhammerhead.png", Theme::IconsBuildHammerHeadColor}}, Icon::Tint);
const Icon RUN(":/projectexplorer/images/run.png");
const Icon RUN_FLAT({
        {":/projectexplorer/images/run_mask.png", Theme::IconsRunToolBarColor}});
const Icon WINDOW(":/projectexplorer/images/window.png");
const Icon DEBUG_START(":/projectexplorer/images/debugger_start.png");
const Icon DEVICE_READY_INDICATOR({
        {":/utils/images/filledcircle.png", Theme::IconsRunColor}}, Icon::Tint);
const Icon DEVICE_READY_INDICATOR_OVERLAY({
        {":/projectexplorer/images/devicestatusindicator.png", Theme::IconsRunToolBarColor}});
const Icon DEVICE_CONNECTED_INDICATOR({
        {":/utils/images/filledcircle.png", Theme::IconsWarningColor}}, Icon::Tint);
const Icon DEVICE_CONNECTED_INDICATOR_OVERLAY({
        {":/projectexplorer/images/devicestatusindicator.png", Theme::IconsWarningToolBarColor}});
const Icon DEVICE_DISCONNECTED_INDICATOR({
        {":/utils/images/filledcircle.png", Theme::IconsStopColor}}, Icon::Tint);
const Icon DEVICE_DISCONNECTED_INDICATOR_OVERLAY({
        {":/projectexplorer/images/devicestatusindicator.png", Theme::IconsStopToolBarColor}});

const Icon DEBUG_START_FLAT({
        {":/projectexplorer/images/run_mask.png", Theme::IconsRunToolBarColor},
        {":/projectexplorer/images/debugger_beetle_mask.png", Theme::IconsDebugColor}});
const Icon DEBUG_START_SMALL({
        {":/utils/images/run_small.png", Theme::IconsRunColor},
        {":/projectexplorer/images/debugger_overlay_small.png", Theme::PanelTextColorMid}}, Icon::MenuTintedStyle);
const Icon DEBUG_START_SMALL_TOOLBAR({
        {":/utils/images/run_small.png", Theme::IconsRunToolBarColor},
        {":/projectexplorer/images/debugger_overlay_small.png", Theme::IconsDebugColor}});
const Icon ANALYZER_START_SMALL({
        {":/utils/images/run_small.png", Theme::IconsRunColor},
        {":/projectexplorer/images/analyzer_overlay_small.png", Theme::PanelTextColorMid}}, Icon::MenuTintedStyle);
const Icon ANALYZER_START_SMALL_TOOLBAR({
        {":/utils/images/run_small.png", Theme::IconsRunToolBarColor},
        {":/projectexplorer/images/analyzer_overlay_small.png", Theme::IconsBaseColor}});

const Icon BUILDSTEP_MOVEUP({
        {":/projectexplorer/images/buildstepmoveup.png", Theme::PanelTextColorDark}}, Icon::Tint);
const Icon BUILDSTEP_MOVEDOWN({
        {":/projectexplorer/images/buildstepmovedown.png", Theme::PanelTextColorDark}}, Icon::Tint);
const Icon BUILDSTEP_DISABLE({
        {":/projectexplorer/images/buildstepdisable.png", Theme::PanelTextColorDark}}, Icon::Tint);
const Icon BUILDSTEP_REMOVE({
        {":/projectexplorer/images/buildstepremove.png", Theme::PanelTextColorDark}}, Icon::Tint);

const Icon DESKTOP_DEVICE({
        {":/projectexplorer/images/desktopdevice.png", Theme::IconsBaseColor}});
const Icon DESKTOP_DEVICE_SMALL({
        {":/utils/images/desktopdevicesmall.png", Theme::PanelTextColorDark}}, Icon::Tint);

const Icon MODE_PROJECT_CLASSIC(":/projectexplorer/images/mode_project.png");
const Icon MODE_PROJECT_FLAT({
        {":/projectexplorer/images/mode_project_mask.png", Theme::IconsBaseColor}});
const Icon MODE_PROJECT_FLAT_ACTIVE({
        {":/projectexplorer/images/mode_project_mask.png", Theme::IconsModeProjectActiveColor}});

} // namespace Icons
} // namespace ProjectExplorer
