// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/icon.h>

namespace ScxmlEditor::Icons {

static const Utils::Icon INITIAL(
        {{":/scxmleditor/images/outerRing.png", Utils::Theme::TextColorNormal},
         {":/scxmleditor/images/innerFill.png", Utils::Theme::PaletteTextDisabled}},
        Utils::Icon::Tint);
static const Utils::Icon FINAL(
        {{":/scxmleditor/images/outerRing.png", Utils::Theme::TextColorNormal},
         {":/scxmleditor/images/innerFill.png", Utils::Theme::PaletteTextDisabled},
         {":/scxmleditor/images/midRing.png", Utils::Theme::BackgroundColorNormal}},
        Utils::Icon::Tint);
static const Utils::Icon STATE(
        {{":/scxmleditor/images/state.png", Utils::Theme::TextColorNormal}},
        Utils::Icon::Tint);
static const Utils::Icon PARALLEL(
        {{":/scxmleditor/images/state.png", Utils::Theme::TextColorNormal},
         {":/scxmleditor/images/parallel_icon.png", Utils::Theme::TextColorNormal}},
        Utils::Icon::ToolBarStyle);
static const Utils::Icon HISTORY(
        {{":/scxmleditor/images/outerRing.png", Utils::Theme::TextColorNormal},
         {":/scxmleditor/images/innerFill.png", Utils::Theme::BackgroundColorNormal},
         {":/scxmleditor/images/history.png", Utils::Theme::TextColorNormal}},
        Utils::Icon::Tint);
};
