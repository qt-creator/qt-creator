// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "toolssettingsaccessor.h"

#include "mesonpluginconstants.h"
#include "mesontools.h"

#include <coreplugin/icore.h>

#include <utils/filepath.h>
#include <utils/settingsaccessor.h>
#include <utils/store.h>

#include <QGuiApplication>

#include <iterator>
#include <vector>

using namespace Core;
using namespace Utils;

namespace MesonProjectManager::Internal {

static Key entryName(int index)
{
    return numberedKey(Constants::ToolsSettings::ENTRY_KEY, index);
}

class ToolsSettingsAccessor final : public UpgradingSettingsAccessor
{
public:
    ToolsSettingsAccessor();

    void saveMesonTools(const std::vector<MesonTools::Tool_t> &tools);
    std::vector<MesonTools::Tool_t> loadMesonTools();
};

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
        Store store = storeFromVariant(data[name]);
        QString type = store.value(ToolsSettings::TOOL_TYPE_KEY).toString();
        if (type == ToolsSettings::TOOL_TYPE_NINJA)
            result.emplace_back(fromVariantMap<NinjaWrapper *>(storeFromVariant(data[name])));
        else if (type == ToolsSettings::TOOL_TYPE_MESON)
            result.emplace_back(fromVariantMap<MesonWrapper *>(storeFromVariant(data[name])));
        else
            QTC_CHECK(false);
    }
    return result;
}

void setupToolsSettingsAccessor()
{
    static ToolsSettingsAccessor theToolSettingsAccessor;
}

} // MesonProjectManager::Internal
