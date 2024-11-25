// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "toolsmodel.h"

#include "mesonprojectmanagertr.h"
#include "mesontools.h"

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/utilsicons.h>

using namespace Utils;

namespace MesonProjectManager::Internal {

// ToolTreeItem

ToolTreeItem::ToolTreeItem(const QString &name)
    : m_name{name}
    , m_id(Id::generate())
    , m_autoDetected{false}
    , m_unsavedChanges{true}
{
    self_check();
    update_tooltip();
}

ToolTreeItem::ToolTreeItem(const MesonTools::Tool_t &tool)
    : m_name{tool->name()}
    , m_executable{tool->exe()}
    , m_id{tool->id()}
    , m_autoDetected{tool->autoDetected()}
{
    m_tooltip = Tr::tr("Version: %1").arg(tool->version().toString());
    self_check();
}

ToolTreeItem::ToolTreeItem(const ToolTreeItem &other)
    : m_name{Tr::tr("Clone of %1").arg(other.m_name)}
    , m_executable{other.m_executable}
    , m_id{Id::generate()}
    , m_autoDetected{false}
    , m_unsavedChanges{true}
{
    self_check();
    update_tooltip();
}

QVariant ToolTreeItem::data(int column, int role) const
{
    switch (role) {
    case Qt::DisplayRole: {
        switch (column) {
        case 0: {
            return m_name;
        }
        case 1: {
            return m_executable.toUserOutput();
        }
        } // switch (column)
        return QVariant();
    }
    case Qt::FontRole: {
        QFont font;
        font.setBold(m_unsavedChanges);
        return font;
    }
    case Qt::ToolTipRole: {
        if (!m_pathExists)
            return Tr::tr("Meson executable path does not exist.");
        if (!m_pathIsFile)
            return Tr::tr("Meson executable path is not a file.");
        if (!m_pathIsExecutable)
            return Tr::tr("Meson executable path is not executable.");
        return m_tooltip;
    }
    case Qt::DecorationRole: {
        if (column == 0 && (!m_pathExists || !m_pathIsFile || !m_pathIsExecutable))
            return Icons::CRITICAL.icon();
        return {};
    }
    }
    return {};
}

void ToolTreeItem::update(const QString &name, const FilePath &exe)
{
    m_unsavedChanges = true;
    m_name = name;
    if (exe != m_executable) {
        m_executable = exe;
        self_check();
        update_tooltip();
    }
}

void ToolTreeItem::self_check()
{
    m_pathExists = m_executable.exists();
    m_pathIsFile = m_executable.toFileInfo().isFile();
    m_pathIsExecutable = m_executable.toFileInfo().isExecutable();
}

void ToolTreeItem::update_tooltip(const QVersionNumber &version)
{
    if (version.isNull())
        m_tooltip = Tr::tr("Cannot get tool version.");
    else
        m_tooltip = Tr::tr("Version: %1").arg(version.toString());
}

void ToolTreeItem::update_tooltip()
{
    update_tooltip(ToolWrapper::read_version(m_executable));
}

// ToolsModel

ToolsModel::ToolsModel()
{
    setHeader({Tr::tr("Name"), Tr::tr("Location")});
    rootItem()->appendChild(
        new StaticTreeItem({ProjectExplorer::Constants::msgAutoDetected()},
                           {ProjectExplorer::Constants::msgAutoDetectedToolTip()}));
    rootItem()->appendChild(new StaticTreeItem(ProjectExplorer::Constants::msgManual()));
    for (const auto &tool : MesonTools::tools())
        addMesonToolHelper(tool);
}

ToolTreeItem *ToolsModel::mesoneToolTreeItem(const QModelIndex &index) const
{
    return itemForIndexAtLevel<2>(index);
}

void ToolsModel::updateItem(const Id &itemId, const QString &name, const FilePath &exe)
{
    auto treeItem = findItemAtLevel<2>([itemId](ToolTreeItem *n) { return n->id() == itemId; });
    QTC_ASSERT(treeItem, return );
    treeItem->update(name, exe);
}

void ToolsModel::addMesonTool()
{
    manualGroup()->appendChild(new ToolTreeItem{uniqueName(Tr::tr("New Meson or Ninja tool"))});
}

void ToolsModel::removeMesonTool(ToolTreeItem *item)
{
    QTC_ASSERT(item, return );
    const Id id = item->id();
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
            emit this->dataChanged(item->index(), item->index());
        }
    });
    while (!m_itemsToRemove.isEmpty()) {
        MesonTools::removeTool(m_itemsToRemove.dequeue());
    }
}

void ToolsModel::addMesonToolHelper(const MesonTools::Tool_t &tool)
{
    if (tool->autoDetected())
        autoDetectedGroup()->appendChild(new ToolTreeItem(tool));
    else
        manualGroup()->appendChild(new ToolTreeItem(tool));
}

QString ToolsModel::uniqueName(const QString &baseName)
{
    QStringList names;
    forItemsAtLevel<2>([&names](ToolTreeItem *item) { names << item->name(); });
    return Utils::makeUniquelyNumbered(baseName, names);
}

TreeItem *ToolsModel::autoDetectedGroup() const
{
    return rootItem()->childAt(0);
}

TreeItem *ToolsModel::manualGroup() const
{
    return rootItem()->childAt(1);
}

} // MesonProjectManager::Internal
