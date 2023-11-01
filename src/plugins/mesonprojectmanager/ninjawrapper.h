// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "mesonpluginconstants.h"
#include "toolwrapper.h"

namespace MesonProjectManager {
namespace Internal {

class NinjaWrapper final : public ToolWrapper
{
public:
    using ToolWrapper::ToolWrapper;

    static std::optional<Utils::FilePath> find()
    {
        return ToolWrapper::findTool({"ninja", "ninja-build"});
    }
    static QString toolName() { return {"Ninja"}; }
};

template<>
inline Utils::Store toVariantMap<NinjaWrapper>(const NinjaWrapper &meson)
{
    Utils::Store data;
    data.insert(Constants::ToolsSettings::NAME_KEY, meson.m_name);
    data.insert(Constants::ToolsSettings::EXE_KEY, meson.m_exe.toSettings());
    data.insert(Constants::ToolsSettings::AUTO_DETECTED_KEY, meson.m_autoDetected);
    data.insert(Constants::ToolsSettings::ID_KEY, meson.m_id.toSetting());
    data.insert(Constants::ToolsSettings::TOOL_TYPE_KEY, Constants::ToolsSettings::TOOL_TYPE_NINJA);
    return data;
}
template<>
inline NinjaWrapper *fromVariantMap<NinjaWrapper *>(const Utils::Store &data)
{
    return new NinjaWrapper(data[Constants::ToolsSettings::NAME_KEY].toString(),
                            Utils::FilePath::fromSettings(data[Constants::ToolsSettings::EXE_KEY]),
                            Utils::Id::fromSetting(data[Constants::ToolsSettings::ID_KEY]),
                            data[Constants::ToolsSettings::AUTO_DETECTED_KEY].toBool());
}

} // namespace Internal
} // namespace MesonProjectManager
