/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nicolas Arnaud-Cormos.
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "actionmacrohandler.h"
#include "macroevent.h"
#include "macro.h"

#include <texteditor/texteditorconstants.h>

#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/icontext.h>

#include <QObject>
#include <QEvent>
#include <QAction>
#include <QSignalMapper>
#include <QShortcut>
#include <QtAlgorithms>
#include <QStringList>

using namespace Macros;
using namespace Macros::Internal;


static const char EVENTNAME[] = "Action";
static quint8 ACTIONNAME = 0;

ActionMacroHandler::ActionMacroHandler():
    m_mapper(new QSignalMapper(this))
{
    connect(m_mapper, SIGNAL(mapped(const QString &)),
            this, SLOT(addActionEvent(const QString &)));

    const Core::ICore *core = Core::ICore::instance();
    const Core::ActionManager *am = core->actionManager();
    connect(am, SIGNAL(commandAdded(QString)),
            this, SLOT(addCommand(QString)));

    // Register all existing scriptable actions
    QList<Core::Command *> commands = am->commands();
    foreach (Core::Command *command, commands) {
        if (command->isScriptable()) {
            QString id = Core::UniqueIDManager::instance()->stringForUniqueIdentifier(command->id());
            registerCommand(id);
        }
    }
}

bool ActionMacroHandler::canExecuteEvent(const MacroEvent &macroEvent)
{
    return (macroEvent.id() == EVENTNAME);
}

bool ActionMacroHandler::executeEvent(const MacroEvent &macroEvent)
{
    const Core::ICore *core = Core::ICore::instance();
    const Core::ActionManager *am = core->actionManager();

    QAction *action = am->command(macroEvent.value(ACTIONNAME).toString())->action();
    if (!action)
        return false;

    action->trigger();
    return true;
}

void ActionMacroHandler::addActionEvent(const QString &id)
{
    if (!isRecording())
        return;

    const Core::ICore *core = Core::ICore::instance();
    const Core::ActionManager *am = core->actionManager();
    const Core::Command *cmd = am->command(id);
    if (cmd->isScriptable(cmd->context())) {
        MacroEvent e;
        e.setId(EVENTNAME);
        e.setValue(ACTIONNAME, id);
        addMacroEvent(e);
    }
}

void ActionMacroHandler::registerCommand(const QString &id)
{
    if (!m_commandIds.contains(id)) {
        m_commandIds.insert(id);
        const Core::ICore *core = Core::ICore::instance();
        const Core::ActionManager *am = core->actionManager();
        QAction* action = am->command(id)->action();
        if (action) {
            connect(action, SIGNAL(triggered()), m_mapper, SLOT(map()));
            m_mapper->setMapping(action, id);
            return;
        }
        QShortcut* shortcut = am->command(id)->shortcut();
        if (shortcut) {
            connect(shortcut, SIGNAL(activated()), m_mapper, SLOT(map()));
            m_mapper->setMapping(shortcut, id);
        }
    }
}

void ActionMacroHandler::addCommand(const QString &id)
{
    const Core::ICore *core = Core::ICore::instance();
    const Core::ActionManager *am = core->actionManager();
    if (am->command(id)->isScriptable())
        registerCommand(id);
}
