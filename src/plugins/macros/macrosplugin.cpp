/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nicolas Arnaud-Cormos.
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "macrosplugin.h"

#include "macrosconstants.h"
#include "macromanager.h"
#include "macrooptionspage.h"
#include "macrolocatorfilter.h"

#include <texteditor/texteditorconstants.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/icore.h>
#include <coreplugin/id.h>
#include <coreplugin/icontext.h>

#include <QtPlugin>
#include <QSettings>
#include <QAction>
#include <QKeySequence>
#include <QMenu>

using namespace Macros;
using namespace Macros::Internal;

MacrosPlugin::MacrosPlugin()
{
}

MacrosPlugin::~MacrosPlugin()
{
}

bool MacrosPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorMessage);

    addAutoReleasedObject(new MacroOptionsPage);
    addAutoReleasedObject(new MacroLocatorFilter);

    Core::ActionManager *am = Core::ICore::actionManager();

    Core::Context globalcontext(Core::Constants::C_GLOBAL);
    Core::Context textContext(TextEditor::Constants::C_TEXTEDITOR);
    m_macroManager = new MacroManager(this);

    // Menus
    Core::ActionContainer *mtools = am->actionContainer(Core::Constants::M_TOOLS);
    Core::ActionContainer *mmacrotools = am->createMenu(Constants::M_TOOLS_MACRO);
    QMenu *menu = mmacrotools->menu();
    menu->setTitle(tr("&Macros"));
    menu->setEnabled(true);
    mtools->addMenu(mmacrotools);

    QAction *startMacro = new QAction(tr("Record Macro"),  this);
    Core::Command *command = am->registerAction(startMacro, Constants::START_MACRO, textContext);
    command->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Ctrl+(") : tr("Alt+(")));
    mmacrotools->addAction(command);
    connect(startMacro, SIGNAL(triggered()), m_macroManager, SLOT(startMacro()));

    QAction *endMacro = new QAction(tr("Stop Recording Macro"),  this);
    endMacro->setEnabled(false);
    command = am->registerAction(endMacro, Constants::END_MACRO, globalcontext);
    command->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Ctrl+)") : tr("Alt+)")));
    mmacrotools->addAction(command);
    connect(endMacro, SIGNAL(triggered()), m_macroManager, SLOT(endMacro()));

    QAction *executeLastMacro = new QAction(tr("Play Last Macro"),  this);
    command = am->registerAction(executeLastMacro, Constants::EXECUTE_LAST_MACRO, textContext);
    command->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+R") : tr("Alt+R")));
    mmacrotools->addAction(command);
    connect(executeLastMacro, SIGNAL(triggered()), m_macroManager, SLOT(executeLastMacro()));

    QAction *saveLastMacro = new QAction(tr("Save Last Macro"),  this);
    saveLastMacro->setEnabled(false);
    command = am->registerAction(saveLastMacro, Constants::SAVE_LAST_MACRO, textContext);
    mmacrotools->addAction(command);
    connect(saveLastMacro, SIGNAL(triggered()), m_macroManager, SLOT(saveLastMacro()));

    return true;
}

void MacrosPlugin::extensionsInitialized()
{
}

Q_EXPORT_PLUGIN(MacrosPlugin)
