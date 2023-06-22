// Copyright (C) 2016 Nicolas Arnaud-Cormos
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "actionmacrohandler.h"
#include "macroevent.h"
#include "macro.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>

#include <texteditor/texteditorconstants.h>

#include <utils/algorithm.h>

#include <QAction>
#include <QEvent>

using namespace Core;
using namespace Utils;

namespace Macros {
namespace Internal {

static const char EVENTNAME[] = "Action";
static quint8 ACTIONNAME = 0;

ActionMacroHandler::ActionMacroHandler()
{
    connect(ActionManager::instance(), &ActionManager::commandAdded,
            this, &ActionMacroHandler::addCommand);

    // Register all existing scriptable actions
    const QList<Command *> commands = ActionManager::commands();
    for (Command *command : commands) {
        if (command->isScriptable())
            registerCommand(command->id());
    }
}

bool ActionMacroHandler::canExecuteEvent(const MacroEvent &macroEvent)
{
    return macroEvent.id() == EVENTNAME;
}

bool ActionMacroHandler::executeEvent(const MacroEvent &macroEvent)
{
    QAction *action = ActionManager::command(Id::fromSetting(macroEvent.value(ACTIONNAME)))->action();
    if (!action)
        return false;

    action->trigger();
    return true;
}

void ActionMacroHandler::registerCommand(Id id)
{
    if (Utils::insert(m_commandIds, id)) {
        const Command *command = ActionManager::command(id);
        if (QAction *action = command->action()) {
            connect(action, &QAction::triggered, this, [this, id, command]() {
                if (!isRecording())
                    return;

                if (command->isScriptable(command->context())) {
                    MacroEvent e;
                    e.setId(EVENTNAME);
                    e.setValue(ACTIONNAME, id.toSetting());
                    addMacroEvent(e);
                }
            });
        }
    }
}

void ActionMacroHandler::addCommand(Id id)
{
    const Command *command = ActionManager::command(id);
    if (command->isScriptable())
        registerCommand(id);
}

} // namespace Internal
} // namespace Macros
