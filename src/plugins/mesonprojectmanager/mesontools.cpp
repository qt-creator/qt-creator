// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mesontools.h"

#include <utils/algorithm.h>
#include <utils/environment.h>

using namespace Utils;

namespace MesonProjectManager::Internal {

std::vector<MesonTools::Tool_t> s_tools;

static MesonTools::Tool_t findTool(const Id &id, ToolType toolType)
{
    const auto tool = std::find_if(std::cbegin(s_tools),
                                   std::cend(s_tools),
                                   [&id](const MesonTools::Tool_t &tool) {
                                       return tool->id() == id;
                                   });
    if (tool != std::cend(s_tools) && (*tool)->toolType() == toolType)
        return *tool;
    return nullptr;
}

MesonTools::Tool_t autoDetectedTool(ToolType toolType)
{
    for (const auto &tool : s_tools) {
        if (tool->autoDetected() && tool->toolType() == toolType)
            return tool;
    }
    return nullptr;
}

static void fixAutoDetected(ToolType toolType)
{
    MesonTools::Tool_t autoDetected = autoDetectedTool(toolType);
    if (!autoDetected) {
        QStringList exeNames;
        QString toolName;
        if (toolType == ToolType::Meson) {
            if (std::optional<FilePath> path = findTool(toolType)) {
                s_tools.emplace_back(
                    std::make_shared<ToolWrapper>(toolType,
                        QString("System %1 at %2").arg("Meson").arg(path->toString()), *path, true));
            }
        } else if (toolType == ToolType::Ninja) {
            if (std::optional<FilePath> path = findTool(toolType)) {
                s_tools.emplace_back(
                    std::make_shared<ToolWrapper>(toolType,
                        QString("System %1 at %2").arg("Ninja").arg(path->toString()), *path, true));
            }
        }
    }
}

bool MesonTools::isMesonWrapper(const MesonTools::Tool_t &tool)
{
    return tool->toolType() == ToolType::Meson;
}

bool MesonTools::isNinjaWrapper(const MesonTools::Tool_t &tool)
{
    return tool->toolType() == ToolType::Ninja;
}

void MesonTools::addTool(const Id &itemId, const QString &name, const FilePath &exe)
{
    // TODO improve this
    if (exe.fileName().contains("ninja"))
        addTool(std::make_shared<ToolWrapper>(ToolType::Ninja, name, exe, itemId));
    else
        addTool(std::make_shared<ToolWrapper>(ToolType::Meson, name, exe, itemId));
}

void MesonTools::addTool(Tool_t meson)
{
    s_tools.emplace_back(std::move(meson));
    emit instance()->toolAdded(s_tools.back());
}

void MesonTools::setTools(std::vector<MesonTools::Tool_t> &&tools)
{
    std::swap(s_tools, tools);
    fixAutoDetected(ToolType::Meson);
    fixAutoDetected(ToolType::Ninja);
}

const std::vector<MesonTools::Tool_t> &MesonTools::tools()
{
    return s_tools;
}

void MesonTools::updateTool(const Id &itemId, const QString &name, const FilePath &exe)
{
    auto item = std::find_if(std::begin(s_tools),
                             std::end(s_tools),
                             [&itemId](const Tool_t &tool) { return tool->id() == itemId; });
    if (item != std::end(s_tools)) {
        (*item)->setExe(exe);
        (*item)->setName(name);
    } else {
        addTool(itemId, name, exe);
    }
}

void MesonTools::removeTool(const Id &id)
{
    auto item = Utils::take(s_tools, [&id](const auto &item) { return item->id() == id; });
    QTC_ASSERT(item, return );
    emit instance()->toolRemoved(*item);
}

std::shared_ptr<ToolWrapper> MesonTools::toolById(const Id &id, ToolType toolType)
{
    return findTool(id, toolType);
}

MesonTools *MesonTools::instance()
{
    static MesonTools inst;
    return &inst;
}

} // MesonProjectManager::Internal
