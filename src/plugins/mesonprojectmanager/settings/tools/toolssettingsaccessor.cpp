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

#include "toolssettingsaccessor.h"
#include <app/app_version.h>
#include <coreplugin/icore.h>
#include <iterator>
#include <mesonpluginconstants.h>
#include <utils/fileutils.h>
#include <vector>
#include <QCoreApplication>
#include <QVariantMap>

namespace MesonProjectManager {
namespace Internal {
namespace {
inline QString entryName(int index)
{
    using namespace Constants;
    return QString("%1%2").arg(ToolsSettings::ENTRY_KEY).arg(index);
}
} // namespace

ToolsSettingsAccessor::ToolsSettingsAccessor()
    : UpgradingSettingsAccessor("QtCreatorMesonTools",
                                QCoreApplication::translate("MesonProjectManager::MesonToolManager",
                                                            "Meson"),
                                Core::Constants::IDE_DISPLAY_NAME)
{
    setBaseFilePath(Utils::FilePath::fromString(Core::ICore::userResourcePath())
                        .pathAppended(Constants::ToolsSettings::FILENAME));
}

void ToolsSettingsAccessor::saveMesonTools(const std::vector<MesonTools::Tool_t> &tools,
                                           QWidget *parent)
{
    using namespace Constants;
    QVariantMap data;
    int entry_count = 0;
    std::for_each(std::cbegin(tools),
                  std::cend(tools),
                  [&data, &entry_count](const MesonTools::Tool_t &tool) {
                      auto asMeson = std::dynamic_pointer_cast<MesonWrapper>(tool);
                      if (asMeson)
                          data.insert(entryName(entry_count), toVariantMap<MesonWrapper>(*asMeson));
                      else {
                          auto asNinja = std::dynamic_pointer_cast<NinjaWrapper>(tool);
                          if (asNinja)
                              data.insert(entryName(entry_count),
                                          toVariantMap<NinjaWrapper>(*asNinja));
                      }
                      entry_count++;
                  });
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
        auto name = entryName(toolIndex);
        if (data.contains(name)) {
            const auto map = data[name].toMap();
            auto type = map.value(ToolsSettings::TOOL_TYPE_KEY, ToolsSettings::TOOL_TYPE_MESON);
            if (type == ToolsSettings::TOOL_TYPE_NINJA)
                result.emplace_back(fromVariantMap<NinjaWrapper *>(data[name].toMap()));
            else
                result.emplace_back(fromVariantMap<MesonWrapper *>(data[name].toMap()));
        }
    }
    return result;
}
} // namespace Internal
} // namespace MesonProjectManager
