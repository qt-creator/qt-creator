// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace ExtensionManager::Internal {

class ExtensionManagerSettings final : public Utils::AspectContainer
{
public:
    ExtensionManagerSettings();

    Utils::StringAspect externalRepoUrl{this};
    Utils::BoolAspect useExternalRepo{this};
};

ExtensionManagerSettings &settings();

} // ExtensionManager::Internal
