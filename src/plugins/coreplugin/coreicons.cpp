// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
