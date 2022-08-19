// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <QCoreApplication>

namespace MesonProjectManager {
namespace Internal {

class MesonTools;

class ToolsSettingsPage final : public Core::IOptionsPage
{
    Q_DECLARE_TR_FUNCTIONS(MesonProjectManager::Internal::ToolsSettingsPage)

public:
    ToolsSettingsPage();
};

} // namespace Internal
} // namespace MesonProjectManager
