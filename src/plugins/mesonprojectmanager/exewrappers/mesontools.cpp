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

#include "mesontools.h"
namespace MesonProjectManager {
namespace Internal {

template<typename T>
inline bool is(const MesonTools::Tool_t &tool)
{
    return bool(std::dynamic_pointer_cast<T>(tool));
}

template<typename T>
std::shared_ptr<T> tool(const Utils::Id &id, const std::vector<MesonTools::Tool_t> &tools)
{
    static_assert(std::is_base_of<ToolWrapper, T>::value, "Type must derive from ToolWrapper");
    const auto tool = std::find_if(std::cbegin(tools),
                                   std::cend(tools),
                                   [&id](const MesonTools::Tool_t &tool) {
                                       return tool->id() == id;
                                   });
    if (tool != std::cend(tools) && is<T>(*tool))
        return std::dynamic_pointer_cast<T>(*tool);
    return nullptr;
}

template<typename T>
std::shared_ptr<T> autoDetected(const std::vector<MesonTools::Tool_t> &tools)
{
    static_assert(std::is_base_of<ToolWrapper, T>::value, "Type must derive from ToolWrapper");
    for (const auto &tool : tools) {
        if (tool->autoDetected() && is<T>(tool)) {
            return std::dynamic_pointer_cast<T>(tool);
        }
    }
    return nullptr;
}

template<typename T>
void fixAutoDetected(std::vector<MesonTools::Tool_t> &tools)
{
    auto autoDetectedTool = autoDetected<T>(tools);
    if (!autoDetectedTool) {
        auto path = T::find();
        if (path)
            tools.emplace_back(std::make_shared<T>(
                QString("System %1 at %2").arg(T::toolName()).arg(path->toString()), *path, true));
    }
}

bool MesonTools::isMesonWrapper(const MesonTools::Tool_t &tool)
{
    return is<MesonWrapper>(tool);
}

bool MesonTools::isNinjaWrapper(const MesonTools::Tool_t &tool)
{
    return is<NinjaWrapper>(tool);
}

void MesonTools::setTools(std::vector<MesonTools::Tool_t> &&tools)
{
    auto self = instance();
    std::swap(self->m_tools, tools);
    fixAutoDetected<MesonWrapper>(self->m_tools);
    fixAutoDetected<NinjaWrapper>(self->m_tools);
}

std::shared_ptr<NinjaWrapper> MesonTools::ninjaWrapper(const Utils::Id &id)
{
    return tool<NinjaWrapper>(id, MesonTools::instance()->m_tools);
}
std::shared_ptr<MesonWrapper> MesonTools::mesonWrapper(const Utils::Id &id)
{
    return tool<MesonWrapper>(id, MesonTools::instance()->m_tools);
}

std::shared_ptr<NinjaWrapper> MesonTools::ninjaWrapper()
{
    return autoDetected<NinjaWrapper>(MesonTools::instance()->m_tools);
}

std::shared_ptr<MesonWrapper> MesonTools::mesonWrapper()
{
    return autoDetected<MesonWrapper>(MesonTools::instance()->m_tools);
}

} // namespace Internal
} // namespace MesonProjectManager
