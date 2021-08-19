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

#include "coreicons.h"

using namespace Utils;

namespace Core {
namespace Icons {

const Icon QTCREATORLOGO_BIG(
        ":/core/images/qtcreatorlogo-big.png");
const Icon QTLOGO(
        ":/core/images/qtlogo.png");
const Icon FIND_CASE_INSENSITIVELY(
        ":/find/images/casesensitively.png");
const Icon FIND_WHOLE_WORD(
        ":/find/images/wholewords.png");
const Icon FIND_REGEXP(
        ":/find/images/regexp.png");
const Icon FIND_PRESERVE_CASE(
        ":/find/images/preservecase.png");

const Icon MODE_EDIT_CLASSIC(
        ":/fancyactionbar/images/mode_Edit.png");
const Icon MODE_EDIT_FLAT({
        {":/fancyactionbar/images/mode_edit_mask.png", Theme::IconsBaseColor}});
const Icon MODE_EDIT_FLAT_ACTIVE({
        {":/fancyactionbar/images/mode_edit_mask.png", Theme::IconsModeEditActiveColor}});
const Icon MODE_DESIGN_CLASSIC(
        ":/fancyactionbar/images/mode_Design.png");
const Icon MODE_DESIGN_FLAT({
        {":/fancyactionbar/images/mode_design_mask.png", Theme::IconsBaseColor}});
const Icon MODE_DESIGN_FLAT_ACTIVE({
        {":/fancyactionbar/images/mode_design_mask.png", Theme::IconsModeDesignActiveColor}});

const Icon DESKTOP_DEVICE_SMALL({{":/utils/images/desktopdevicesmall.png",
                                  Theme::PanelTextColorDark}},
                                Icon::Tint);

} // namespace Icons
} // namespace Core
