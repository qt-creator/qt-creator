// Copyright (C) 2026 Sebastian Mosiej (sebastian.mosiej@wp.pl)
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace ZenMode {

class ZenModeSettings : public Utils::AspectContainer
{
public:
    ZenModeSettings();

    Utils::IntegerAspect contentWidth{this};
    Utils::SelectionAspect modes{this};
};

ZenModeSettings &settings();

} // namespace ZenMode
