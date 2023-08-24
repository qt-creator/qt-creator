// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "toolssettingsaccessor.h"

#include "mesonpluginconstants.h"
#include "mesonprojectmanagertr.h"

#include <coreplugin/icore.h>

#include <utils/filepath.h>
#include <utils/store.h>

#include <QGuiApplication>

#include <iterator>
#include <vector>

using namespace Utils;

namespace MesonProjectManager {
namespace Internal {

static Key entryName(int index)
{
    return Constants::ToolsSettings::ENTRY_KEY + Key::number(index);
}

ToolsSettingsAccessor::ToolsSettingsAccessor()
{
    setDocType("QtCreatorMesonTools");
    setApplicationDisplayName(QGuiApplication::applicationDisplayName());
    setBaseFilePath(Core::ICore::userResourcePath(Constants::ToolsSettings::FILENAME));
}

void ToolsSettingsAccessor::saveMesonTools(const std::vector<MesonTools::Tool_t> &tools,
                                           QWidget *parent)
{
    using namespace Constants;
    Store data;
    int entry_count = 0;
    for (const MesonTools::Tool_t &tool : tools) {
        auto asMeson = std::dynamic_pointer_cast<MesonWrapper>(tool);
        if (asMeson)
            data.insert(entryName(entry_count), QVariant::fromValue(toVariantMap<MesonWrapper>(*asMeson)));
        else {
            auto asNinja = std::dynamic_pointer_cast<NinjaWrapper>(tool);
            if (asNinja)
                data.insert(entryName(entry_count), QVariant::fromValue(toVariantMap<NinjaWrapper>(*asNinja)));
        }
        entry_count++;
    }
    data.insert(ToolsSettings::ENTRY_COUNT, entry_count);
    saveSettings(data, parent);
}

std::vector<MesonTools::Tool_t> ToolsSettingsAccessor::loadMesonTools(QWidget *parent)
{
    using namespace Constants;
    auto data = restoreSettings(parent);
    auto entry_count = data.value(ToolsSettings::ENTRY_COUNT, 0).toInt();
    std::vector<MesonTools::Tool_t> result;
    for (auto toolIndex = 0; toolIndex < entry_count; toolIndex++) {
        Key name = entryName(toolIndex);
        if (data.contains(name)) {
            const auto map = data[name].toMap();
            auto type = map.value(ToolsSettings::TOOL_TYPE_KEY, ToolsSettings::TOOL_TYPE_MESON);
            if (type == ToolsSettings::TOOL_TYPE_NINJA)
                result.emplace_back(fromVariantMap<NinjaWrapper *>(data[name].value<Store>()));
            else
                result.emplace_back(fromVariantMap<MesonWrapper *>(data[name].value<Store>()));
        }
    }
    return result;
}

} // namespace Internal
} // namespace MesonProjectManager
