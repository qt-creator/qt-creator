// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace HarmonyOs::Internal {

class HarmonyOsSettings final : public Utils::AspectContainer
{
public:
    HarmonyOsSettings();

    Utils::FilePathAspect sdkLocation{this};
    Utils::BoolAspect automaticKitCreation{this};
};

HarmonyOsSettings &settings();

void setupHarmonyOsSettingsPage();

} // namespace HarmonyOs::Internal
