// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "mesontools.h"

#include <utils/treemodel.h>

#include <QQueue>

namespace MesonProjectManager::Internal {

class ToolTreeItem final : public Utils::TreeItem
{
public:
    ToolTreeItem(const QString &name);
    ToolTreeItem(const MesonTools::Tool_t &tool);
    ToolTreeItem(const ToolTreeItem &other);

    QVariant data(int column, int role) const override;
    bool isAutoDetected() const noexcept { return m_autoDetected; }
    QString name() const noexcept { return m_name; }
    Utils::FilePath executable() const noexcept { return m_executable; }
    Utils::Id id() const noexcept { return m_id; }
    bool hasUnsavedChanges() const noexcept { return m_unsavedChanges; }
    void setSaved() { m_unsavedChanges = false; }
    void update(const QString &name, const Utils::FilePath &exe);

private:
    void self_check();
    void update_tooltip(const QVersionNumber &version);
    void update_tooltip();
    QString m_name;
    QString m_tooltip;
    Utils::FilePath m_executable;
    Utils::Id m_id;
    bool m_autoDetected;
    bool m_pathExists;
    bool m_pathIsFile;
    bool m_pathIsExecutable;
    bool m_unsavedChanges = false;
};

class ToolsModel final : public Utils::TreeModel<Utils::TreeItem, Utils::TreeItem, ToolTreeItem>
{
public:
    ToolsModel();

    ToolTreeItem *mesoneToolTreeItem(const QModelIndex &index) const;
    void updateItem(const Utils::Id &itemId, const QString &name, const Utils::FilePath &exe);
    void addMesonTool();
    void removeMesonTool(ToolTreeItem *item);
    ToolTreeItem *cloneMesonTool(ToolTreeItem *item);
    void apply();

private:
    void addMesonToolHelper(const MesonTools::Tool_t &);
    QString uniqueName(const QString &baseName);
    Utils::TreeItem *autoDetectedGroup() const;
    Utils::TreeItem *manualGroup() const;
    QQueue<Utils::Id> m_itemsToRemove;
};

} // MesonProjectManager::Internal
