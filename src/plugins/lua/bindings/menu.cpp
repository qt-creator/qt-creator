// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../luaengine.h"

#include "utils.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/modemanager.h>

using namespace Utils;
using namespace Core;

namespace Lua::Internal {

void setupMenuModule()
{
    registerProvider("Menu", [](sol::state_view lua) -> sol::object {
        sol::table result = lua.create_table();

        mirrorEnum(result, QMetaEnum::fromType<ActionContainer::OnAllDisabledBehavior>());

        result["create"] = [](const std::string &menuId, const sol::table &options) {
            MenuBuilder builder(Id::fromString(QString::fromStdString(menuId)));

            for (const auto &[k, v] : options) {
                const QString key = QString::fromStdString(k.as<std::string>());
                if (key == "title") {
                    builder.setTitle(v.as<QString>());
                } else if (key == "icon") {
                    builder.setIcon(toIcon(v.as<IconFilePathOrString>())->icon());
                } else if (key == "onAllDisabledBehavior") {
                    int behavior = v.as<int>();
                    builder.setOnAllDisabledBehavior(
                        static_cast<ActionContainer::OnAllDisabledBehavior>(behavior));
                } else if (key == "containers") {
                    v.as<sol::table>().for_each([&builder](sol::object, sol::object value) {
                        if (value.is<sol::table>()) {
                            const sol::table t = value.as<sol::table>();
                            const auto containerId = t.get<std::string>("containerId");
                            const auto groupId = t.get_or<std::string>("groupId", {});
                            builder.addToContainer(
                                Id::fromString(QString::fromStdString(containerId)),
                                Id::fromString(QString::fromStdString(groupId)));
                        } else if (value.is<QString>()) {
                            builder.addToContainer(Id::fromString(value.as<QString>()));
                        }
                    });
                } else {
                    throw std::runtime_error("Unknown key: " + key.toStdString());
                }
            }
        };

        result["addSeparator"] = [](const std::string &menuId) {
            ActionContainer *container = ActionManager::actionContainer(
                Id::fromString(QString::fromStdString(menuId)));
            if (!container)
                throw std::runtime_error("Menu not found: " + menuId);
            container->addSeparator();
        };

        return result;
    });
}

} // namespace Lua::Internal
