/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef PROJECTEXPLORERICONS_H
#define PROJECTEXPLORERICONS_H

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
const Utils::Icon CLEAN(
        QLatin1String(":/projectexplorer/images/clean.png"));
const Utils::Icon CLEAN_SMALL(
        QLatin1String(":/projectexplorer/images/clean_small.png"));
const Utils::Icon REBUILD(
        QLatin1String(":/projectexplorer/images/rebuild.png"));
const Utils::Icon REBUILD_SMALL(
        QLatin1String(":/projectexplorer/images/rebuild_small.png"));
const Utils::Icon RUN(
        QLatin1String(":/projectexplorer/images/run.png"));
const Utils::Icon RUN_FLAT({
        {QLatin1String(":/projectexplorer/images/run_mask.png"), Utils::Theme::IconsRunColor}});
const Utils::Icon WINDOW(
        QLatin1String(":/projectexplorer/images/window.png"));
const Utils::Icon DEBUG_START(
        QLatin1String(":/projectexplorer/images/debugger_start.png"));

const Utils::Icon DEBUG_START_FLAT({
        {QLatin1String(":/projectexplorer/images/debugger_beetle_mask.png"), Utils::Theme::IconsDebugColor},
        {QLatin1String(":/projectexplorer/images/debugger_run_mask.png"), Utils::Theme::IconsRunColor}});
const Utils::Icon RUN_SMALL({
        {QLatin1String(":/projectexplorer/images/run_small.png"), Utils::Theme::IconsRunColor}});
const Utils::Icon STOP_SMALL({
        {QLatin1String(":/projectexplorer/images/stop_small.png"), Utils::Theme::IconsStopColor}});
const Utils::Icon MODE_PROJECT_CLASSIC(
        QLatin1String(":/projectexplorer/images/mode_project.png"));
const Utils::Icon MODE_PROJECT_FLAT({
        {QLatin1String(":/projectexplorer/images/mode_project_mask.png"), Utils::Theme::IconsBaseColor}});
const Utils::Icon MODE_PROJECT_FLAT_ACTIVE({
        {QLatin1String(":/projectexplorer/images/mode_project_mask.png"), Utils::Theme::IconsModeProjetcsActiveColor}});
const Utils::Icon INTERRUPT_SMALL({
        {QLatin1String(":/projectexplorer/images/interrupt_small.png"), Utils::Theme::IconsInterruptColor}});

} // namespace Icons
} // namespace ProjectExplorer

#endif // PROJECTEXPLORERICONS_H
