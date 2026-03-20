// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "toolsmodel.h"

#include "mesonprojectmanagertr.h"

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/utilsicons.h>

using namespace Utils;

namespace MesonProjectManager::Internal {

// ToolItem

ToolItem::ToolItem(const QString &name)
    : name{name}
    , id{Id::generate()}
    , autoDetected{false}
{
    selfCheck();
    updateTooltip();
}

ToolItem::ToolItem(const MesonTools::Tool_t &tool)
    : name{tool->name()}
    , tooltip{Tr::tr("Version: %1").arg(tool->version().toString())}
    , executable{tool->exe()}
    , id{tool->id()}
    , autoDetected{tool->autoDetected()}
{
    selfCheck();
}

ToolItem ToolItem::cloned() const
{
    ToolItem result;
    result.name = Tr::tr("Clone of %1").arg(name);
    result.executable = executable;
    result.id = Id::generate();
    result.autoDetected = false;
    result.selfCheck();
    result.updateTooltip();
    return result;
}

QVariant ToolItem::data(int column, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        switch (column) {
        case 0:
            return name;
        case 1:
            return executable.toUserOutput();
        }
        return {};
    case Qt::ToolTipRole:
        if (!pathExists)
            return Tr::tr("Meson executable path does not exist.");
        if (!pathIsFile)
            return Tr::tr("Meson executable path is not a file.");
        if (!pathIsExecutable)
            return Tr::tr("Meson executable path is not executable.");
        return tooltip;
    case Qt::DecorationRole:
        if (column == 0 && (!pathExists || !pathIsFile || !pathIsExecutable))
            return Icons::CRITICAL.icon();
        return {};
    }
    return {};
}

void ToolItem::update(const QString &newName, const FilePath &newExe)
{
    name = newName;
    if (newExe != executable) {
        executable = newExe;
        selfCheck();
        updateTooltip();
    }
}

void ToolItem::selfCheck()
{
    pathExists = executable.exists();
    pathIsFile = executable.toFileInfo().isFile();
    pathIsExecutable = executable.toFileInfo().isExecutable();
}

void ToolItem::updateTooltip(const QVersionNumber &version)
{
    if (version.isNull())
        tooltip = Tr::tr("Cannot get tool version.");
    else
        tooltip = Tr::tr("Version: %1").arg(version.toString());
}

void ToolItem::updateTooltip()
{
    updateTooltip(MesonToolWrapper::read_version(executable));
}

// ToolsModel

ToolsModel::ToolsModel()
{
    setHeader({Tr::tr("Name"), Tr::tr("Location")});
    setFilters(ProjectExplorer::Constants::msgAutoDetected(),
               {{ProjectExplorer::Constants::msgManual(), [this](int row) {
                    return !item(row).autoDetected;
                }}});
    for (const MesonTools::Tool_t &tool : MesonTools::tools())
        appendItem(ToolItem{tool});
}

QModelIndex ToolsModel::addMesonTool()
{
    return appendVolatileItem(ToolItem{uniqueName(Tr::tr("New Meson"))});
}

QModelIndex ToolsModel::cloneMesonTool(int row)
{
    return appendVolatileItem(item(row).cloned());
}

void ToolsModel::updateItem(const Id &itemId, const QString &name, const FilePath &exe)
{
    const int row = rowForId(itemId);
    QTC_ASSERT(row >= 0, return);
    ToolItem it = item(row);
    it.update(name, exe);
    setVolatileItem(row, it);
    notifyRowChanged(row);
}

int ToolsModel::rowForId(const Id &id) const
{
    for (int row = 0; row < itemCount(); ++row) {
        if (item(row).id == id)
            return row;
    }
    return -1;
}

void ToolsModel::apply()
{
    for (int row = 0; row < itemCount(); ++row) {
        if (isRemoved(row)) {
            MesonTools::removeTool(item(row).id);
            continue;
        }
        if (isDirty(row)) {
            const ToolItem it = item(row);
            MesonTools::updateTool(it.id, it.name, it.executable);
        }
    }

    GroupedModel::apply();
}

QVariant ToolsModel::variantData(const QVariant &v, int column, int role) const
{
    return fromVariant(v).data(column, role);
}

QString ToolsModel::uniqueName(const QString &baseName) const
{
    QStringList names;
    for (int row = 0; row < itemCount(); ++row)
        names << item(row).name;
    return Utils::makeUniquelyNumbered(baseName, names);
}

} // namespace MesonProjectManager::Internal
