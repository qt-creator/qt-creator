/**************************************************************************
**
** Copyright (C) 2015 Nicolas Arnaud-Cormos
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "actionmacrohandler.h"
#include "macroevent.h"
#include "macro.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/id.h>

#include <texteditor/texteditorconstants.h>

#include <QAction>
#include <QEvent>
#include <QSignalMapper>

using namespace Core;

namespace Macros {
namespace Internal {

static const char EVENTNAME[] = "Action";
static quint8 ACTIONNAME = 0;

ActionMacroHandler::ActionMacroHandler():
    m_mapper(new QSignalMapper(this))
{
    connect(m_mapper, SIGNAL(mapped(QString)),
            this, SLOT(addActionEvent(QString)));

    connect(ActionManager::instance(), &ActionManager::commandAdded,
            this, &ActionMacroHandler::addCommand);

    // Register all existing scriptable actions
    QList<Command *> commands = ActionManager::commands();
    foreach (Command *command, commands) {
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

void ActionMacroHandler::addActionEvent(const QString &name)
{
    if (!isRecording())
        return;

    const Id id = Id::fromString(name);
    const Command *command = ActionManager::command(id);
    if (command->isScriptable(command->context())) {
        MacroEvent e;
        e.setId(EVENTNAME);
        e.setValue(ACTIONNAME, id.toSetting());
        addMacroEvent(e);
    }
}

void ActionMacroHandler::registerCommand(Id id)
{
    if (!m_commandIds.contains(id)) {
        m_commandIds.insert(id);
        const Command *command = ActionManager::command(id);
        if (QAction *action = command->action()) {
            connect(action, SIGNAL(triggered()), m_mapper, SLOT(map()));
            m_mapper->setMapping(action, id.toString());
            return;
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
