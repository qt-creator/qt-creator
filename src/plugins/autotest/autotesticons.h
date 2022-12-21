// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/icon.h>

namespace Autotest {
namespace Icons {

const Utils::Icon SORT_NATURALLY({
        {":/autotest/images/leafsort.png", Utils::Theme::IconsBaseColor}});

const Utils::Icon RUN_FAILED({
        {":/utils/images/run_small.png", Utils::Theme::IconsRunColor},
        {":/utils/images/iconoverlay_reset.png", Utils::Theme::IconsStopColor}},
        Utils::Icon::MenuTintedStyle);
const Utils::Icon RUN_FAILED_TOOLBAR({
        {":/utils/images/run_small.png", Utils::Theme::IconsRunToolBarColor},
        {":/utils/images/iconoverlay_reset.png", Utils::Theme::IconsStopToolBarColor}});
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
const Utils::Icon RESULT_BLACKLISTEDXPASS({
        {":/utils/images/filledcircle.png", Utils::Theme::OutputPanes_TestXPassTextColor},
        {":/projectexplorer/images/buildstepdisable.png", Utils::Theme::PanelTextColorDark}},
        Utils::Icon::Tint | Utils::Icon::PunchEdges);
const Utils::Icon RESULT_BLACKLISTEDXFAIL({
        {":/utils/images/filledcircle.png", Utils::Theme::OutputPanes_TestXFailTextColor},
        {":/projectexplorer/images/buildstepdisable.png", Utils::Theme::PanelTextColorDark}},
        Utils::Icon::Tint | Utils::Icon::PunchEdges);
const Utils::Icon RESULT_BENCHMARK({
        {":/utils/images/filledcircle.png", Utils::Theme::BackgroundColorNormal},
        {":/utils/images/stopwatch.png", Utils::Theme::PanelTextColorDark}}, Utils::Icon::Tint);
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
const Utils::Icon VISUAL_DISPLAY({{":/autotest/images/visual.png", Utils::Theme::IconsBaseColor}});
const Utils::Icon TEXT_DISPLAY({{":/autotest/images/text.png", Utils::Theme::IconsBaseColor}});

} // namespace Icons
} // namespace Autotest
