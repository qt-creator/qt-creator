// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace Terminal {
class TerminalSettings : public Utils::AspectContainer
{
public:
    TerminalSettings();

    static TerminalSettings &instance();

    Utils::BoolAspect enableTerminal;

    Utils::StringAspect font;
    Utils::IntegerAspect fontSize;
    Utils::StringAspect shell;

    Utils::ColorAspect foregroundColor;
    Utils::ColorAspect backgroundColor;
    Utils::ColorAspect selectionColor;
    Utils::ColorAspect findMatchColor;

    Utils::ColorAspect colors[16];

    Utils::BoolAspect allowBlinkingCursor;

    Utils::BoolAspect sendEscapeToTerminal;
    Utils::BoolAspect audibleBell;
};

} // namespace Terminal
