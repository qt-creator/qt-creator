// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace Terminal {

class TerminalSettings : public Utils::AspectContainer
{
public:
    TerminalSettings();

    Utils::BoolAspect enableTerminal{this};

    Utils::StringAspect font{this};
    Utils::IntegerAspect fontSize{this};
    Utils::FilePathAspect shell{this};
    Utils::StringAspect shellArguments{this};

    Utils::ColorAspect foregroundColor;
    Utils::ColorAspect backgroundColor;
    Utils::ColorAspect selectionColor;
    Utils::ColorAspect findMatchColor;

    Utils::ColorAspect colors[16];

    Utils::BoolAspect allowBlinkingCursor{this};

    Utils::BoolAspect sendEscapeToTerminal{this};
    Utils::BoolAspect audibleBell{this};
    Utils::BoolAspect lockKeyboard{this};

    Utils::BoolAspect enableMouseTracking{this};
};

TerminalSettings &settings();

} // Terminal
