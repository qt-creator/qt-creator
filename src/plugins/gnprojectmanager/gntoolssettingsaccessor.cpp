// Copyright (C) 2024 The Qt Company Ltd.
// Copyright (C) 2026 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gntoolssettingsaccessor.h"

#include "gnpluginconstants.h"
#include "gntools.h"

#include <coreplugin/icore.h>

#include <utils/filepath.h>
#include <utils/settingsaccessor.h>
#include <utils/store.h>

#include <QGuiApplication>

#include <vector>

using namespace Core;
using namespace Utils;

namespace GNProjectManager::Internal {

static Key entryName(int index)
{
    return numberedKey(Constants::ToolsSettings::ENTRY_KEY, index);
}

class GNToolsSettingsAccessor final : public UpgradingSettingsAccessor
{
public:
    GNToolsSettingsAccessor();

    void saveGNTools();
    void loadGNTools();
};

GNToolsSettingsAccessor::GNToolsSettingsAccessor()
{
    setDocType("QtCreatorGNTools");
    setApplicationDisplayName(QGuiApplication::applicationDisplayName());
    setBaseFilePath(ICore::userResourcePath(Constants::ToolsSettings::FILENAME));

    loadGNTools();

    QObject::connect(ICore::instance(), &ICore::saveSettingsRequested, [this] { saveGNTools(); });
}

void GNToolsSettingsAccessor::saveGNTools()
{
    using namespace Constants;
    Store data;
    int entryCount = 0;
    for (const GNTools::Tool_t &tool : GNTools::tools()) {
        data.insert(entryName(entryCount), variantFromStore(tool->toVariantMap()));
        ++entryCount;
    }
    data.insert(ToolsSettings::ENTRY_COUNT, entryCount);
    saveSettings(data);
}

void GNToolsSettingsAccessor::loadGNTools()
{
    using namespace Constants;
    Store data = restoreSettings();
    int entryCount = data.value(ToolsSettings::ENTRY_COUNT, 0).toInt();
    std::vector<GNTools::Tool_t> result;
    for (int toolIndex = 0; toolIndex < entryCount; ++toolIndex) {
        Key name = entryName(toolIndex);
        Store store = storeFromVariant(data[name]);
        if (!store.isEmpty())
            result.emplace_back(new GNTool(store));
    }

    GNTools::setTools(std::move(result));
}

void setupGNToolsSettingsAccessor()
{
    static GNToolsSettingsAccessor theGNToolsSettingsAccessor;
}

} // namespace GNProjectManager::Internal
