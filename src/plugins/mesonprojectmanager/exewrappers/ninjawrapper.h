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
#include "../mesonpluginconstants.h"
#include "toolwrapper.h"
namespace MesonProjectManager {
namespace Internal {

class NinjaWrapper final : public ToolWrapper
{
public:
    using ToolWrapper::ToolWrapper;

    static inline Utils::optional<Utils::FilePath> find()
    {
        return ToolWrapper::findTool({"ninja", "ninja-build"});
    }
    static inline QString toolName() { return {"Ninja"}; };
};

template<>
inline QVariantMap toVariantMap<NinjaWrapper>(const NinjaWrapper &meson)
{
    QVariantMap data;
    data.insert(Constants::ToolsSettings::NAME_KEY, meson.m_name);
    data.insert(Constants::ToolsSettings::EXE_KEY, meson.m_exe.toVariant());
    data.insert(Constants::ToolsSettings::AUTO_DETECTED_KEY, meson.m_autoDetected);
    data.insert(Constants::ToolsSettings::ID_KEY, meson.m_id.toSetting());
    data.insert(Constants::ToolsSettings::TOOL_TYPE_KEY, Constants::ToolsSettings::TOOL_TYPE_NINJA);
    return data;
}
template<>
inline NinjaWrapper *fromVariantMap<NinjaWrapper *>(const QVariantMap &data)
{
    return new NinjaWrapper(data[Constants::ToolsSettings::NAME_KEY].toString(),
                            Utils::FilePath::fromVariant(data[Constants::ToolsSettings::EXE_KEY]),
                            Utils::Id::fromSetting(data[Constants::ToolsSettings::ID_KEY]),
                            data[Constants::ToolsSettings::AUTO_DETECTED_KEY].toBool());
}

} // namespace Internal
} // namespace MesonProjectManager
