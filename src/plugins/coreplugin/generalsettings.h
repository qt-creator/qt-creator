// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace Core::Internal {

class GeneralSettings : public Utils::AspectContainer
{
public:
    GeneralSettings();

    Utils::BoolAspect showShortcutsInContextMenus{this};

    static void applyToolbarStyleFromSettings();
};

GeneralSettings &generalSettings();

} // Core::Internal
