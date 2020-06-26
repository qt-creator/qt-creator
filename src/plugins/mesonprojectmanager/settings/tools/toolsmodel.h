/****************************************************************************
**
** Copyright (C) 2020 Alexis Jeandet.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "toolssettingspage.h"
#include <exewrappers/mesontools.h>

#include <utils/treemodel.h>
#include <QCoreApplication>
#include <QQueue>

namespace MesonProjectManager {
namespace Internal {
class ToolTreeItem;
class ToolsModel final : public Utils::TreeModel<Utils::TreeItem, Utils::TreeItem, ToolTreeItem>
{
    Q_DECLARE_TR_FUNCTIONS(MesonProjectManager::Internal::ToolsSettingsPage)
public:
    ToolsModel();
    ToolTreeItem *mesoneToolTreeItem(const QModelIndex &index) const;
    void updateItem(const Utils::Id &itemId, const QString &name, const Utils::FilePath &exe);
    void addMesonTool();
    void removeMesonTool(ToolTreeItem *item);
    ToolTreeItem *cloneMesonTool(ToolTreeItem *item);
    void apply();

private:
    void addMesonTool(const MesonTools::Tool_t &);
    QString uniqueName(const QString &baseName);
    Utils::TreeItem *autoDetectedGroup() const;
    Utils::TreeItem *manualGroup() const;
    QQueue<Utils::Id> m_itemsToRemove;
};
} // namespace Internal
} // namespace MesonProjectManager
