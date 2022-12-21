// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "toolitemsettings.h"
#include "toolsmodel.h"

#include <coreplugin/dialogs/ioptionspage.h>

namespace Utils { class DetailsWidget; }

namespace MesonProjectManager::Internal {

class ToolTreeItem;

class ToolsSettingsWidget final : public Core::IOptionsPageWidget
{
public:
    explicit ToolsSettingsWidget();
    ~ToolsSettingsWidget();

private:
    void apply() final;

    void cloneMesonTool();
    void removeMesonTool();
    void currentMesonToolChanged(const QModelIndex &newCurrent);

    ToolsModel m_model;
    ToolItemSettings *m_itemSettings;
    ToolTreeItem *m_currentItem = nullptr;

    QTreeView *m_mesonList;
    Utils::DetailsWidget *m_mesonDetails;
    QPushButton *m_cloneButton;
    QPushButton *m_removeButton;
};

} // MesonProjectManager::Internal
