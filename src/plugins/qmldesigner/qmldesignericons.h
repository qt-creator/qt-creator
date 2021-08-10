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

namespace QmlDesigner {
namespace Icons {

const Utils::Icon ARROW_UP({
        {":/navigator/icon/arrowup.png", Utils::Theme::IconsBaseColor}});
const Utils::Icon ARROW_RIGHT({
        {":/navigator/icon/arrowright.png", Utils::Theme::IconsBaseColor}});
const Utils::Icon ARROW_DOWN({
        {":/navigator/icon/arrowdown.png", Utils::Theme::IconsBaseColor}});
const Utils::Icon ARROW_LEFT({
        {":/navigator/icon/arrowleft.png", Utils::Theme::IconsBaseColor}});
const Utils::Icon EXPORT_CHECKED(":/navigator/icon/export_checked.png");
const Utils::Icon EXPORT_UNCHECKED(":/navigator/icon/export_unchecked.png");
const Utils::Icon SNAPPING({
        {":/icon/layout/snapping.png", Utils::Theme::IconsBaseColor}});
const Utils::Icon NO_SNAPPING({
        {":/icon/layout/no_snapping.png", Utils::Theme::IconsBaseColor}});
const Utils::Icon NO_SNAPPING_AND_ANCHORING({
        {":/icon/layout/snapping_and_anchoring.png", Utils::Theme::IconsBaseColor}});

const Utils::Icon EDIT3D_LIGHT_ON({
        {":/edit3d/images/edit_light_on.png", Utils::Theme::QmlDesigner_HighlightColor}});
const Utils::Icon EDIT3D_LIGHT_OFF({
        {":/edit3d/images/edit_light_off.png", Utils::Theme::IconsBaseColor}});
const Utils::Icon EDIT3D_GRID_ON({
        {":/edit3d/images/grid_on.png", Utils::Theme::QmlDesigner_HighlightColor}});
const Utils::Icon EDIT3D_GRID_OFF({
        {":/edit3d/images/grid_off.png", Utils::Theme::IconsBaseColor}});
const Utils::Icon EDIT3D_SELECTION_MODE_ON({
        {":/edit3d/images/select_group.png", Utils::Theme::QmlDesigner_HighlightColor}});
const Utils::Icon EDIT3D_SELECTION_MODE_OFF({
        {":/edit3d/images/select_item.png", Utils::Theme::IconsBaseColor}});
const Utils::Icon EDIT3D_MOVE_TOOL_ON({
        {":/edit3d/images/move_on.png", Utils::Theme::QmlDesigner_HighlightColor}});
const Utils::Icon EDIT3D_MOVE_TOOL_OFF({
        {":/edit3d/images/move_off.png", Utils::Theme::IconsBaseColor}});
const Utils::Icon EDIT3D_ROTATE_TOOL_ON({
        {":/edit3d/images/rotate_on.png", Utils::Theme::QmlDesigner_HighlightColor}});
const Utils::Icon EDIT3D_ROTATE_TOOL_OFF({
        {":/edit3d/images/rotate_off.png", Utils::Theme::IconsBaseColor}});
const Utils::Icon EDIT3D_SCALE_TOOL_ON({
        {":/edit3d/images/scale_on.png", Utils::Theme::QmlDesigner_HighlightColor}});
const Utils::Icon EDIT3D_SCALE_TOOL_OFF({
        {":/edit3d/images/scale_off.png", Utils::Theme::IconsBaseColor}});
const Utils::Icon EDIT3D_FIT_SELECTED_OFF({
        {":/edit3d/images/fit_selected.png", Utils::Theme::IconsBaseColor}});
const Utils::Icon EDIT3D_EDIT_CAMERA_ON({
        {":/edit3d/images/perspective_camera.png", Utils::Theme::QmlDesigner_HighlightColor}});
const Utils::Icon EDIT3D_EDIT_CAMERA_OFF({
        {":/edit3d/images/orthographic_camera.png", Utils::Theme::IconsBaseColor}});
const Utils::Icon EDIT3D_ORIENTATION_ON({
        {":/edit3d/images/global.png", Utils::Theme::QmlDesigner_HighlightColor}});
const Utils::Icon EDIT3D_ORIENTATION_OFF({
        {":/edit3d/images/local.png", Utils::Theme::IconsBaseColor}});

} // Icons
} // QmlDesigner
