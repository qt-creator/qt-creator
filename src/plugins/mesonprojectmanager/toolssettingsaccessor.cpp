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

    void saveMesonTools();
    void loadMesonTools();
};

ToolsSettingsAccessor::ToolsSettingsAccessor()
{
    setDocType("QtCreatorMesonTools");
    setApplicationDisplayName(QGuiApplication::applicationDisplayName());
    setBaseFilePath(ICore::userResourcePath(Constants::ToolsSettings::FILENAME));

    loadMesonTools();

    QObject::connect(ICore::instance(), &ICore::saveSettingsRequested, [this] {
        saveMesonTools();
    });
}

void ToolsSettingsAccessor::saveMesonTools()
{
    using namespace Constants;
    Store data;
    int entry_count = 0;
    for (const MesonTools::Tool_t &tool : MesonTools::tools()) {
        data.insert(entryName(entry_count), variantFromStore(tool->toVariantMap()));
        ++entry_count;
    }
    data.insert(ToolsSettings::ENTRY_COUNT, entry_count);
    saveSettings(data, ICore::dialogParent());
}

void ToolsSettingsAccessor::loadMesonTools()
{
    using namespace Constants;
    auto data = restoreSettings(ICore::dialogParent());
    auto entry_count = data.value(ToolsSettings::ENTRY_COUNT, 0).toInt();
    std::vector<MesonTools::Tool_t> result;
    for (auto toolIndex = 0; toolIndex < entry_count; toolIndex++) {
        Key name = entryName(toolIndex);
        Store store = storeFromVariant(data[name]);
        result.emplace_back(new ToolWrapper(store));
    }

    MesonTools::setTools(std::move(result));
}

void setupToolsSettingsAccessor()
{
    static ToolsSettingsAccessor theToolSettingsAccessor;
}

} // MesonProjectManager::Internal
