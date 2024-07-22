// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mesontools.h"

#include <utils/environment.h>

using namespace Utils;

namespace MesonProjectManager {
namespace Internal {

MesonTools::Tool_t tool(const Id &id,
                        const std::vector<MesonTools::Tool_t> &tools,
                        ToolType toolType)
{
    const auto tool = std::find_if(std::cbegin(tools),
                                   std::cend(tools),
                                   [&id](const MesonTools::Tool_t &tool) {
                                       return tool->id() == id;
                                   });
    if (tool != std::cend(tools) && (*tool)->toolType() == toolType)
        return *tool;
    return nullptr;
}

static MesonTools::Tool_t autoDetected(const std::vector<MesonTools::Tool_t> &tools, ToolType toolType)
{
    for (const auto &tool : tools) {
        if (tool->autoDetected() && tool->toolType() == toolType)
            return tool;
    }
    return nullptr;
}

static void fixAutoDetected(std::vector<MesonTools::Tool_t> &tools, ToolType toolType)
{
    MesonTools::Tool_t autoDetectedTool = autoDetected(tools, toolType);
    if (!autoDetectedTool) {
        QStringList exeNames;
        QString toolName;
        if (toolType == ToolType::Meson) {
            if (std::optional<FilePath> path = findMesonTool()) {
                tools.emplace_back(
                    std::make_shared<ToolWrapper>(toolType,
                        QString("System %1 at %2").arg("Meson").arg(path->toString()), *path, true));
            }
        } else if (toolType == ToolType::Ninja) {
            if (std::optional<FilePath> path = findNinjaTool()) {
                tools.emplace_back(
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
    auto self = instance();
    self->m_tools.emplace_back(std::move(meson));
    emit self->toolAdded(self->m_tools.back());
}

void MesonTools::setTools(std::vector<MesonTools::Tool_t> &&tools)
{
    auto self = instance();
    std::swap(self->m_tools, tools);
    fixAutoDetected(self->m_tools, ToolType::Meson);
    fixAutoDetected(self->m_tools, ToolType::Ninja);
}

void MesonTools::updateTool(const Id &itemId, const QString &name, const FilePath &exe)
{
    auto self = instance();
    auto item = std::find_if(std::begin(self->m_tools),
                             std::end(self->m_tools),
                             [&itemId](const Tool_t &tool) { return tool->id() == itemId; });
    if (item != std::end(self->m_tools)) {
        (*item)->setExe(exe);
        (*item)->setName(name);
    } else {
        addTool(itemId, name, exe);
    }
}

void MesonTools::removeTool(const Id &id)
{
    auto self = instance();
    auto item = Utils::take(self->m_tools, [&id](const auto &item) { return item->id() == id; });
    QTC_ASSERT(item, return );
    emit self->toolRemoved(*item);
}

std::shared_ptr<ToolWrapper> MesonTools::ninjaWrapper(const Id &id)
{
    return tool(id, MesonTools::instance()->m_tools, ToolType::Ninja);
}

std::shared_ptr<ToolWrapper> MesonTools::mesonWrapper(const Id &id)
{
    return tool(id, MesonTools::instance()->m_tools, ToolType::Meson);
}

std::shared_ptr<ToolWrapper> MesonTools::autoDetectedNinja()
{
    return autoDetected(MesonTools::instance()->m_tools, ToolType::Ninja);
}

std::shared_ptr<ToolWrapper> MesonTools::autoDetectedMeson()
{
    return autoDetected(MesonTools::instance()->m_tools, ToolType::Meson);
}

} // namespace Internal
} // namespace MesonProjectManager
