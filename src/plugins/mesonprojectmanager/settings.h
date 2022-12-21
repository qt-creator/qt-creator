// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/aspects.h>

namespace MesonProjectManager {
namespace Internal {

class Settings : public Utils::AspectContainer
{
public:
    Settings();

    static Settings *instance();

    Utils::BoolAspect autorunMeson;
    Utils::BoolAspect verboseNinja;
};

class GeneralSettingsPage final : public Core::IOptionsPage
{
public:
    GeneralSettingsPage();
};

} // namespace Internal
} // namespace MesonProjectManager
