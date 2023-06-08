// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/icon.h>

namespace Terminal {

static Utils::Icon NEW_TERMINAL_ICON(
    {{":/terminal/images/terminal.png", Utils::Theme::IconsBaseColor},
     {":/utils/images/iconoverlay_add_small.png", Utils::Theme::IconsRunToolBarColor}});

static Utils::Icon CLOSE_TERMINAL_ICON(
    {{":/terminal/images/terminal.png", Utils::Theme::IconsBaseColor},
     {":/utils/images/iconoverlay_close_small.png", Utils::Theme::IconsStopToolBarColor}});

static Utils::Icon LOCK_KEYBOARD_ICON(
    {{":/terminal/images/keyboardlock.png", Utils::Theme::IconsBaseColor},
     {":/codemodel/images/private.png", Utils::Theme::IconsBaseColor}});

static Utils::Icon UNLOCK_KEYBOARD_ICON(
    {{":/terminal/images/keyboardlock.png", Utils::Theme::IconsBaseColor}});

} // namespace Terminal
