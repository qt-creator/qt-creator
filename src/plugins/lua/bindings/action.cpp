// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../luaengine.h"

#include "utils.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/modemanager.h>

using namespace Utils;
using namespace Core;

namespace Lua::Internal {

void setupActionModule()
{
    registerProvider("Action", [](sol::state_view lua) -> sol::object {
        sol::table result = lua.create_table();

        result.new_enum(
            "CommandAttribute",
            "CA_Hide",
            Command::CA_Hide,
            "CA_UpdateText",
            Command::CA_UpdateText,
            "CA_UpdateIcon",
            Command::CA_UpdateIcon,
            "CA_NonConfigurable",
            Command::CA_NonConfigurable);

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
            "toolTip",
            sol::property(
                [](ScriptCommand *cmd) { return cmd->m_contextAction->toolTip(); },
                [](ScriptCommand *cmd, const QString &toolTip) {
                    cmd->m_contextAction->setToolTip(toolTip);
                }),
            "text",
            sol::property(
                [](ScriptCommand *cmd) { return cmd->m_contextAction->text(); },
                [](ScriptCommand *cmd, const QString &text) {
                    cmd->m_contextAction->setText(text);
                }),
            "icon",
            sol::property([](ScriptCommand *cmd, const IconFilePathOrString &&icon) {
                cmd->m_contextAction->setIcon(toIcon(icon)->icon());
            }));

        result["create"] = [parent = std::make_unique<QObject>()](
                               const std::string &actionId, const sol::table &options) mutable {
            ActionBuilder b(parent.get(), Id::fromString(QString::fromStdString(actionId)));

            for (const auto &[k, v] : options) {
                QString key = k.as<QString>();

                if (key == "context")
                    b.setContext(Id::fromString(v.as<QString>()));
                else if (key == "onTrigger")
                    b.addOnTriggered([f = v.as<sol::main_function>()]() {
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
                    b.setCommandAttributes(Command::CommandAttributes::fromInt(v.as<int>()));
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
                        ModeManager::addAction(b.commandAction(), v.as<int>());
                    } else {
                        throw std::runtime_error(
                            "asMode needs an integer argument for the priority");
                    }
                } else if (key == "icon") {
                    b.setIcon(toIcon(v.as<IconFilePathOrString>())->icon());
                } else
                    throw std::runtime_error("Unknown key: " + key.toStdString());
            }

            return ScriptCommand{b.command(), b.contextAction()};
        };

        result["trigger"] = [](const std::string &actionId) mutable {
            Command *command = ActionManager::command(
                Id::fromString(QString::fromStdString(actionId)));
            if (!command)
                throw std::runtime_error("Action not found: " + actionId);
            if (!command->action())
                throw std::runtime_error("Action not assigned: " + actionId);
            if (!command->action()->isEnabled())
                throw std::runtime_error("Action not enabled: " + actionId);
            command->action()->trigger();
        };

        return result;
    });
}

} // namespace Lua::Internal
