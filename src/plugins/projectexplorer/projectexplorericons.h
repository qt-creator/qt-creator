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

#include <utils/icon.h>

namespace ProjectExplorer {
namespace Icons {

const Utils::Icon BUILD(
        QLatin1String(":/projectexplorer/images/build.png"));
const Utils::Icon BUILD_FLAT({
        {QLatin1String(":/projectexplorer/images/build_hammerhandle_mask.png"), Utils::Theme::IconsBuildHammerHandleColor},
        {QLatin1String(":/projectexplorer/images/build_hammerhead_mask.png"), Utils::Theme::IconsBuildHammerHeadColor}});
const Utils::Icon BUILD_SMALL(
        QLatin1String(":/projectexplorer/images/build_small.png"));
const Utils::Icon CLEAN({
        {QLatin1String(":/core/images/clean_pane_small.png"), Utils::Theme::PanelTextColorMid}}, Utils::Icon::Tint);
const Utils::Icon REBUILD({
        {QLatin1String(":/projectexplorer/images/rebuildhammerhandles.png"), Utils::Theme::IconsBuildHammerHandleColor},
        {QLatin1String(":/projectexplorer/images/rebuildhammerheads.png"), Utils::Theme::IconsBuildHammerHeadColor}}, Utils::Icon::Tint);
const Utils::Icon RUN(
        QLatin1String(":/projectexplorer/images/run.png"));
const Utils::Icon RUN_FLAT({
        {QLatin1String(":/projectexplorer/images/run_mask.png"), Utils::Theme::IconsRunToolBarColor}});
const Utils::Icon WINDOW(
        QLatin1String(":/projectexplorer/images/window.png"));
const Utils::Icon DEBUG_START(
        QLatin1String(":/projectexplorer/images/debugger_start.png"));

const Utils::Icon DEBUG_START_FLAT({
        {QLatin1String(":/projectexplorer/images/run_mask.png"), Utils::Theme::IconsRunToolBarColor},
        {QLatin1String(":/projectexplorer/images/debugger_beetle_mask.png"), Utils::Theme::IconsDebugColor}});
const Utils::Icon DEBUG_START_SMALL({
        {QLatin1String(":/core/images/run_small.png"), Utils::Theme::IconsRunColor},
        {QLatin1String(":/projectexplorer/images/debugger_overlay_small.png"), Utils::Theme::PanelTextColorMid}}, Utils::Icon::MenuTintedStyle);
const Utils::Icon DEBUG_START_SMALL_TOOLBAR({
        {QLatin1String(":/core/images/run_small.png"), Utils::Theme::IconsRunToolBarColor},
        {QLatin1String(":/projectexplorer/images/debugger_overlay_small.png"), Utils::Theme::IconsDebugColor}});

const Utils::Icon BUILDSTEP_MOVEUP({
        {QLatin1String(":/projectexplorer/images/buildstepmoveup.png"), Utils::Theme::PanelTextColorDark}}, Utils::Icon::Tint);
const Utils::Icon BUILDSTEP_MOVEDOWN({
        {QLatin1String(":/projectexplorer/images/buildstepmovedown.png"), Utils::Theme::PanelTextColorDark}}, Utils::Icon::Tint);
const Utils::Icon BUILDSTEP_DISABLE({
        {QLatin1String(":/projectexplorer/images/buildstepdisable.png"), Utils::Theme::PanelTextColorDark}}, Utils::Icon::Tint);
const Utils::Icon BUILDSTEP_REMOVE({
        {QLatin1String(":/projectexplorer/images/buildstepremove.png"), Utils::Theme::PanelTextColorDark}}, Utils::Icon::Tint);

const Utils::Icon DESKTOP_DEVICE({
        {QLatin1String(":/projectexplorer/images/desktopdevice.png"), Utils::Theme::IconsBaseColor}});
const Utils::Icon DESKTOP_DEVICE_SMALL({
        {QLatin1String(":/core/images/desktopdevicesmall.png"), Utils::Theme::PanelTextColorDark}}, Utils::Icon::Tint);

const Utils::Icon MODE_PROJECT_CLASSIC(
        QLatin1String(":/projectexplorer/images/mode_project.png"));
const Utils::Icon MODE_PROJECT_FLAT({
        {QLatin1String(":/projectexplorer/images/mode_project_mask.png"), Utils::Theme::IconsBaseColor}});
const Utils::Icon MODE_PROJECT_FLAT_ACTIVE({
        {QLatin1String(":/projectexplorer/images/mode_project_mask.png"), Utils::Theme::IconsModeProjetcsActiveColor}});

} // namespace Icons
} // namespace ProjectExplorer
