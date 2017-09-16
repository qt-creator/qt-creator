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

namespace Autotest {
namespace Icons {

const Utils::Icon SORT_ALPHABETICALLY({
        {":/images/sort.png", Utils::Theme::IconsBaseColor}});
const Utils::Icon SORT_NATURALLY({
        {":/images/leafsort.png", Utils::Theme::IconsBaseColor}});
const Utils::Icon RUN_SELECTED_OVERLAY({
        {":/images/runselected_boxes.png", Utils::Theme::BackgroundColorDark},
        {":/images/runselected_tickmarks.png", Utils::Theme::IconsBaseColor}});

const Utils::Icon RESULT_PASS({
        {":/utils/images/filledcircle.png", Utils::Theme::OutputPanes_TestPassTextColor}},
        Utils::Icon::Tint);
const Utils::Icon RESULT_FAIL({
        {":/utils/images/filledcircle.png", Utils::Theme::OutputPanes_TestFailTextColor}},
        Utils::Icon::Tint);
const Utils::Icon RESULT_XFAIL({
        {":/utils/images/filledcircle.png", Utils::Theme::OutputPanes_TestXFailTextColor}},
        Utils::Icon::Tint);
const Utils::Icon RESULT_XPASS({
        {":/utils/images/filledcircle.png", Utils::Theme::OutputPanes_TestXPassTextColor}},
        Utils::Icon::Tint);
const Utils::Icon RESULT_SKIP({
        {":/utils/images/filledcircle.png", Utils::Theme::OutputPanes_TestSkipTextColor}},
        Utils::Icon::Tint);
const Utils::Icon RESULT_BLACKLISTEDPASS({
        {":/utils/images/filledcircle.png", Utils::Theme::OutputPanes_TestPassTextColor},
        {":/projectexplorer/images/buildstepdisable.png", Utils::Theme::PanelTextColorDark}},
        Utils::Icon::Tint | Utils::Icon::PunchEdges);
const Utils::Icon RESULT_BLACKLISTEDFAIL({
        {":/utils/images/filledcircle.png", Utils::Theme::OutputPanes_TestFailTextColor},
        {":/projectexplorer/images/buildstepdisable.png", Utils::Theme::PanelTextColorDark}},
        Utils::Icon::Tint | Utils::Icon::PunchEdges);
const Utils::Icon RESULT_BENCHMARK({
        {":/utils/images/filledcircle.png", Utils::Theme::BackgroundColorNormal},
        {":/images/benchmark.png", Utils::Theme::PanelTextColorDark}}, Utils::Icon::Tint);
const Utils::Icon RESULT_MESSAGEDEBUG({
        {":/utils/images/filledcircle.png", Utils::Theme::OutputPanes_TestDebugTextColor}},
        Utils::Icon::Tint);
const Utils::Icon RESULT_MESSAGEWARN({
        {":/utils/images/filledcircle.png", Utils::Theme::OutputPanes_TestWarnTextColor}},
        Utils::Icon::Tint);
const Utils::Icon RESULT_MESSAGEPASSWARN({
        {":/utils/images/filledcircle.png", Utils::Theme::OutputPanes_TestPassTextColor},
        {":/utils/images/iconoverlay_warning.png", Utils::Theme::OutputPanes_TestWarnTextColor}},
        Utils::Icon::Tint | Utils::Icon::PunchEdges);
const Utils::Icon RESULT_MESSAGEFAILWARN({
        {":/utils/images/filledcircle.png", Utils::Theme::OutputPanes_TestFailTextColor},
        {":/utils/images/iconoverlay_warning.png", Utils::Theme::OutputPanes_TestWarnTextColor}},
        Utils::Icon::Tint | Utils::Icon::PunchEdges);
const Utils::Icon RESULT_MESSAGEFATAL({
        {":/utils/images/filledcircle.png", Utils::Theme::OutputPanes_TestFatalTextColor}},
        Utils::Icon::Tint);
const Utils::Icon VISUAL_DISPLAY({{":/images/visual.png", Utils::Theme::IconsBaseColor}});
const Utils::Icon TEXT_DISPLAY({{":/images/text.png", Utils::Theme::IconsBaseColor}});

} // namespace Icons
} // namespace Autotest
