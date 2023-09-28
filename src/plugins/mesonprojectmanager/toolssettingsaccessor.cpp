// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "toolssettingsaccessor.h"

#include "mesonpluginconstants.h"
#include "mesonprojectmanagertr.h"

#include <coreplugin/icore.h>
#include <coreplugin/icore.h>

#include <utils/filepath.h>
#include <utils/store.h>

#include <QGuiApplication>

#include <iterator>
#include <vector>

using namespace Core;
using namespace Utils;

namespace MesonProjectManager {
namespace Internal {

static Key entryName(int index)
{
    return numberedKey(Constants::ToolsSettings::ENTRY_KEY, index);
}

ToolsSettingsAccessor::ToolsSettingsAccessor()
{
    setDocType("QtCreatorMesonTools");
    setApplicationDisplayName(QGuiApplication::applicationDisplayName());
    setBaseFilePath(ICore::userResourcePath(Constants::ToolsSettings::FILENAME));

    MesonTools::setTools(loadMesonTools());

    QObject::connect(ICore::instance(), &ICore::saveSettingsRequested, [this] {
        saveMesonTools(MesonTools::tools());
    });
}

void ToolsSettingsAccessor::saveMesonTools(const std::vector<MesonTools::Tool_t> &tools)
{
    using namespace Constants;
    Store data;
    int entry_count = 0;
    for (const MesonTools::Tool_t &tool : tools) {
        auto asMeson = std::dynamic_pointer_cast<MesonWrapper>(tool);
        if (asMeson)
            data.insert(entryName(entry_count), variantFromStore(toVariantMap<MesonWrapper>(*asMeson)));
        else {
            auto asNinja = std::dynamic_pointer_cast<NinjaWrapper>(tool);
            if (asNinja)
                data.insert(entryName(entry_count), variantFromStore(toVariantMap<NinjaWrapper>(*asNinja)));
        }
        entry_count++;
    }
    data.insert(ToolsSettings::ENTRY_COUNT, entry_count);
    saveSettings(data, ICore::dialogParent());
}

std::vector<MesonTools::Tool_t> ToolsSettingsAccessor::loadMesonTools()
{
    using namespace Constants;
    auto data = restoreSettings(ICore::dialogParent());
    auto entry_count = data.value(ToolsSettings::ENTRY_COUNT, 0).toInt();
    std::vector<MesonTools::Tool_t> result;
    for (auto toolIndex = 0; toolIndex < entry_count; toolIndex++) {
        Key name = entryName(toolIndex);
        if (data.contains(name)) {
            const auto map = data[name].toMap();
            auto type = map.value(ToolsSettings::TOOL_TYPE_KEY, ToolsSettings::TOOL_TYPE_MESON);
            if (type == ToolsSettings::TOOL_TYPE_NINJA)
                result.emplace_back(fromVariantMap<NinjaWrapper *>(storeFromVariant(data[name])));
            else
                result.emplace_back(fromVariantMap<MesonWrapper *>(storeFromVariant(data[name])));
        }
    }
    return result;
}

} // namespace Internal
} // namespace MesonProjectManager
