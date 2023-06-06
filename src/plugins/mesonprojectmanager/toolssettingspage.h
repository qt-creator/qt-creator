// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

namespace MesonProjectManager::Internal {

class ToolsSettingsPage final : public Core::IOptionsPage
{
public:
    ToolsSettingsPage();
};

} // MesonProjectManager::Internal
