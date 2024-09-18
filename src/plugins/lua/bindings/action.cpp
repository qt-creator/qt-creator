// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../luaengine.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/modemanager.h>

using namespace Utils;
using namespace Core;

namespace Lua::Internal {

void setupActionModule()
{
    registerProvider("Action", [](sol::state_view lua) -> sol::object {
        sol::table result = lua.create_table();

        result.new_enum("CommandAttribute",
                        "CA_Hide",
                        Core::Command::CA_Hide,
                        "CA_UpdateText",
                        Core::Command::CA_UpdateText,
                        "CA_UpdateIcon",
                        Core::Command::CA_UpdateIcon,
                        "CA_NonConfigurable",
                        Core::Command::CA_NonConfigurable);

        struct ScriptCommand
        {
            Command *m_cmd;
            QAction *m_contextAction;
        };

        result.new_usertype<ScriptCommand>(
            "Command",
            sol::no_constructor,
            "enabled",
            sol::property(
                [](ScriptCommand *cmd) { return cmd->m_contextAction->isEnabled(); },
                [](ScriptCommand *cmd, bool enabled) { cmd->m_contextAction->setEnabled(enabled); }),
            "tooltip",
            sol::property(
                [](ScriptCommand *cmd) { return cmd->m_contextAction->toolTip(); },
                [](ScriptCommand *cmd, const QString &tooltip) {
                    cmd->m_contextAction->setToolTip(tooltip);
                }),
            "text",
            sol::property(
                [](ScriptCommand *cmd) { return cmd->m_contextAction->text(); },
                [](ScriptCommand *cmd, const QString &text) {
                    cmd->m_contextAction->setText(text);
                }));

        result["create"] = [parent = std::make_unique<QObject>()](
                               const std::string &actionId, const sol::table &options) mutable {
            Core::ActionBuilder b(parent.get(), Id::fromString(QString::fromStdString(actionId)));

            for (const auto &[k, v] : options) {
                QString key = k.as<QString>();

                if (key == "context")
                    b.setContext(Id::fromString(v.as<QString>()));
                else if (key == "onTrigger")
                    b.addOnTriggered([f = v.as<sol::function>()]() {
                        auto res = void_safe_call(f);
                        QTC_CHECK_EXPECTED(res);
                    });
                else if (key == "text")
                    b.setText(v.as<QString>());
                else if (key == "iconText")
                    b.setIconText(v.as<QString>());
                else if (key == "toolTip")
                    b.setToolTip(v.as<QString>());
                else if (key == "commandAttributes")
                    b.setCommandAttributes(Core::Command::CommandAttributes::fromInt(v.as<int>()));
                else if (key == "commandDescription")
                    b.setCommandDescription(v.as<QString>());
                else if (key == "defaultKeySequence")
                    b.setDefaultKeySequence(QKeySequence(v.as<QString>()));
                else if (key == "defaultKeySequences") {
                    sol::table t = v.as<sol::table>();
                    QList<QKeySequence> sequences;
                    sequences.reserve(t.size());
                    for (const auto &[_, v] : t)
                        sequences.push_back(QKeySequence(v.as<QString>()));
                    b.setDefaultKeySequences(sequences);
                } else if (key == "asModeAction") {
                    if (v.is<int>()) {
                        Core::ModeManager::addAction(b.commandAction(), v.as<int>());
                    } else {
                        throw std::runtime_error(
                            "asMode needs an integer argument for the priority");
                    }
                } else
                    throw std::runtime_error("Unknown key: " + key.toStdString());
            }

            return ScriptCommand{b.command(), b.contextAction()};
        };

        return result;
    });
}

} // namespace Lua::Internal
