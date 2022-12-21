// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/icon.h>

namespace Help {
namespace Icons {

const Utils::Icon MODE_HELP_CLASSIC(
        ":/help/images/mode_help.png");
const Utils::Icon MODE_HELP_FLAT({
        {":/help/images/mode_help_mask.png", Utils::Theme::IconsBaseColor}});
const Utils::Icon MODE_HELP_FLAT_ACTIVE({
        {":/help/images/mode_help_mask.png", Utils::Theme::IconsModeHelpActiveColor}});
const Utils::Icon MACOS_TOUCHBAR_HELP(
        ":/help/images/macos_touchbar_help.png");

} // namespace Icons
} // namespace Help
