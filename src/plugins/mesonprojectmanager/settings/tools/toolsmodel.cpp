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

#include "toolsmodel.h"
#include "tooltreeitem.h"
#include "utils/qtcassert.h"
#include "utils/stringutils.h"
#include <exewrappers/mesontools.h>

namespace MesonProjectManager {
namespace Internal {

ToolsModel::ToolsModel()
{
    setHeader({tr("Name"), tr("Location")});
    rootItem()->appendChild(new Utils::StaticTreeItem(tr("Auto-detected")));
    rootItem()->appendChild(new Utils::StaticTreeItem(tr("Manual")));
    for (const auto &tool : MesonTools::tools()) {
        addMesonTool(tool);
    }
}

ToolTreeItem *ToolsModel::mesoneToolTreeItem(const QModelIndex &index) const
{
    return itemForIndexAtLevel<2>(index);
}

void ToolsModel::updateItem(const Utils::Id &itemId, const QString &name, const Utils::FilePath &exe)
{
    auto treeItem = findItemAtLevel<2>([itemId](ToolTreeItem *n) { return n->id() == itemId; });
    QTC_ASSERT(treeItem, return );
    treeItem->update(name, exe);
}

void ToolsModel::addMesonTool()
{
    manualGroup()->appendChild(new ToolTreeItem{uniqueName(tr("New Meson or Ninja tool"))});
}

void ToolsModel::removeMesonTool(ToolTreeItem *item)
{
    QTC_ASSERT(item, return );
    const Utils::Id id = item->id();
    destroyItem(item);
    m_itemsToRemove.enqueue(id);
}

ToolTreeItem *ToolsModel::cloneMesonTool(ToolTreeItem *item)
{
    QTC_ASSERT(item, return nullptr);
    auto newItem = new ToolTreeItem(*item);
    manualGroup()->appendChild(newItem);
    return item;
}

void ToolsModel::apply()
{
    forItemsAtLevel<2>([this](ToolTreeItem *item) {
        if (item->hasUnsavedChanges()) {
            MesonTools::updateTool(item->id(), item->name(), item->executable());
            item->setSaved();
            emit this->dataChanged(item->index(),item->index());
        }
    });
    while (!m_itemsToRemove.isEmpty()) {
        MesonTools::removeTool(m_itemsToRemove.dequeue());
    }

}

void ToolsModel::addMesonTool(const MesonTools::Tool_t &tool)
{
    if (tool->autoDetected())
        autoDetectedGroup()->appendChild(new ToolTreeItem(tool));
    else
        manualGroup()->appendChild(new ToolTreeItem(tool));
}

QString ToolsModel::uniqueName(const QString &baseName)
{
    QStringList names;
    forItemsAtLevel<2>([&names](auto *item) { names << item->name(); });
    return Utils::makeUniquelyNumbered(baseName, names);
}

Utils::TreeItem *ToolsModel::autoDetectedGroup() const
{
    return rootItem()->childAt(0);
}

Utils::TreeItem *ToolsModel::manualGroup() const
{
    return rootItem()->childAt(1);
}
} // namespace Internal
} // namespace MesonProjectManager
