// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace MesonProjectManager::Internal {

class MesonSettings final : public Utils::AspectContainer
{
public:
    MesonSettings();

    Utils::BoolAspect autorunMeson{this};
    Utils::BoolAspect verboseNinja{this};
};

MesonSettings &settings();

} // MesonProjectManager::Internal
