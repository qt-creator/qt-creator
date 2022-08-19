// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "exewrappers/mesonwrapper.h"
#include "toolitemsettings.h"
#include "toolsmodel.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <QCoreApplication>
#include <QTabWidget>

namespace MesonProjectManager {
namespace Internal {

namespace Ui { class ToolsSettingsWidget; }

class ToolTreeItem;
class ToolsSettingsWidget final : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(MesonProjectManager::Internal::ToolsSettingsWidget)
    void apply() final;

public:
    explicit ToolsSettingsWidget();
    ~ToolsSettingsWidget();

private:
    void cloneMesonTool();
    void removeMesonTool();
    void currentMesonToolChanged(const QModelIndex &newCurrent);
    Ui::ToolsSettingsWidget *ui;
    ToolsModel m_model;
    ToolItemSettings *m_itemSettings;
    ToolTreeItem *m_currentItem = nullptr;
};

} // namespace Internal
} // namespace MesonProjectManager
