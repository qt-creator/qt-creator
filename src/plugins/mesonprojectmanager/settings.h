// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/aspects.h>

namespace MesonProjectManager::Internal {

class Settings : public Core::PagedSettings
{
public:
    Settings();

    Utils::BoolAspect autorunMeson{this};
    Utils::BoolAspect verboseNinja{this};
};

Settings &settings();

} // MesonProjectManager::Internal
