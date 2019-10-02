/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

namespace QmlDesigner {
namespace TimelineIcons {

// Icons on the timeline ruler
const Utils::Icon WORK_AREA_HANDLE_LEFT(
        ":/timelineplugin/images/work_area_handle_left.png");
const Utils::Icon WORK_AREA_HANDLE_RIGHT(
        ":/timelineplugin/images/work_area_handle_right.png");
const Utils::Icon PLAYHEAD(
        ":/timelineplugin/images/playhead.png");

// Icons on the timeline tracks
const Utils::Icon KEYFRAME_LINEAR_INACTIVE(
        ":/timelineplugin/images/keyframe_linear_inactive.png");
const Utils::Icon KEYFRAME_LINEAR_ACTIVE(
        ":/timelineplugin/images/keyframe_linear_active.png");
const Utils::Icon KEYFRAME_LINEAR_SELECTED(
        ":/timelineplugin/images/keyframe_linear_selected.png");
const Utils::Icon KEYFRAME_MANUALBEZIER_INACTIVE(
        ":/timelineplugin/images/keyframe_manualbezier_inactive.png");
const Utils::Icon KEYFRAME_MANUALBEZIER_ACTIVE(
        ":/timelineplugin/images/keyframe_manualbezier_active.png");
const Utils::Icon KEYFRAME_MANUALBEZIER_SELECTED(
        ":/timelineplugin/images/keyframe_manualbezier_selected.png");
const Utils::Icon KEYFRAME_AUTOBEZIER_INACTIVE(
        ":/timelineplugin/images/keyframe_autobezier_inactive.png");
const Utils::Icon KEYFRAME_AUTOBEZIER_ACTIVE(
        ":/timelineplugin/images/keyframe_autobezier_active.png");
const Utils::Icon KEYFRAME_AUTOBEZIER_SELECTED(
        ":/timelineplugin/images/keyframe_autobezier_selected.png");
const Utils::Icon KEYFRAME_LINEARTOBEZIER_INACTIVE(
        ":/timelineplugin/images/keyframe_lineartobezier_inactive.png");
const Utils::Icon KEYFRAME_LINEARTOBEZIER_ACTIVE(
        ":/timelineplugin/images/keyframe_lineartobezier_active.png");
const Utils::Icon KEYFRAME_LINEARTOBEZIER_SELECTED(
        ":/timelineplugin/images/keyframe_lineartobezier_selected.png");

// Icons on the "section"
const Utils::Icon KEYFRAME(
        ":/timelineplugin/images/keyframe.png");
const Utils::Icon IS_KEYFRAME(
        ":/timelineplugin/images/is_keyframe.png");
const Utils::Icon NEXT_KEYFRAME({
        {":/timelineplugin/images/next_keyframe.png", Utils::Theme::IconsBaseColor}});
const Utils::Icon PREVIOUS_KEYFRAME({
        {":/timelineplugin/images/previous_keyframe.png", Utils::Theme::IconsBaseColor}});
const Utils::Icon LOCAL_RECORD_KEYFRAMES({
        {":/timelineplugin/images/local_record_keyframes.png", Utils::Theme::IconsStopToolBarColor}});
const Utils::Icon ADD_TIMELINE({
        {":/timelineplugin/images/add_timeline.png",  Utils::Theme::PanelTextColorDark}},
        Utils::Icon::Tint);
const Utils::Icon ADD_TIMELINE_TOOLBAR({
        {":/timelineplugin/images/add_timeline.png", Utils::Theme::IconsBaseColor}});
const Utils::Icon REMOVE_TIMELINE({
        {":/timelineplugin/images/remove_timeline.png", Utils::Theme::PanelTextColorDark}},
        Utils::Icon::Tint);

// Icons on the toolbars
const Utils::Icon ANIMATION({
        {":/timelineplugin/images/animation.png", Utils::Theme::IconsBaseColor}});
const Utils::Icon CURVE_EDITORDIALOG({
        {":/timelineplugin/images/curveGraphIcon.png", Utils::Theme::IconsBaseColor}});
const Utils::Icon TO_FIRST_FRAME({
        {":/timelineplugin/images/to_first_frame.png", Utils::Theme::IconsBaseColor}});
const Utils::Icon BACK_ONE_FRAME({
        {":/timelineplugin/images/back_one_frame.png", Utils::Theme::IconsBaseColor}});
const Utils::Icon START_PLAYBACK({
        {":/timelineplugin/images/start_playback.png", Utils::Theme::IconsRunToolBarColor}});
const Utils::Icon PAUSE_PLAYBACK({
        {":/timelineplugin/images/pause_playback.png", Utils::Theme::IconsInterruptToolBarColor}});
const Utils::Icon FORWARD_ONE_FRAME({
        {":/timelineplugin/images/forward_one_frame.png", Utils::Theme::IconsBaseColor}});
const Utils::Icon TO_LAST_FRAME({
        {":/timelineplugin/images/to_last_frame.png", Utils::Theme::IconsBaseColor}});
const Utils::Icon LOOP_PLAYBACK({
        {":/timelineplugin/images/loop_playback.png", Utils::Theme::IconsBaseColor}});
const Utils::Icon CURVE_PICKER({
        {":/timelineplugin/images/curve_picker.png", Utils::Theme::IconsBaseColor}});
const Utils::Icon CURVE_EDITOR({
        {":/timelineplugin/images/curve_editor.png", Utils::Theme::IconsBaseColor}});
const Utils::Icon GLOBAL_RECORD_KEYFRAMES({
        {":/timelineplugin/images/global_record_keyframes.png", Utils::Theme::IconsStopToolBarColor}});
const Utils::Icon GLOBAL_RECORD_KEYFRAMES_OFF({
        {":/timelineplugin/images/global_record_keyframes.png", Utils::Theme::IconsBaseColor}});
const Utils::Icon ZOOM_SMALL({
        {":/timelineplugin/images/zoom_small.png", Utils::Theme::IconsBaseColor}});
const Utils::Icon ZOOM_BIG({
        {":/timelineplugin/images/zoom_big.png", Utils::Theme::IconsBaseColor}});

} // namespace TimelineIcons
} // namespace QmlDesigner
