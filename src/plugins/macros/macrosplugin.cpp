// Copyright (C) 2016 Nicolas Arnaud-Cormos
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "macrosplugin.h"

#include "macrolocatorfilter.h"
#include "macromanager.h"
#include "macrooptionspage.h"
#include "macrosconstants.h"
#include "macrostr.h"

#include <texteditor/texteditorconstants.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>

#include <QAction>
#include <QKeySequence>
#include <QMenu>

namespace Macros {
namespace Internal {

class MacrosPluginPrivate
{
public:
    MacroManager macroManager;
    MacroOptionsPage optionsPage;
    MacroLocatorFilter locatorFilter;
};

MacrosPlugin::~MacrosPlugin()
{
    delete d;
}

void MacrosPlugin::initialize()
{
    d = new MacrosPluginPrivate;

    Core::Context textContext(TextEditor::Constants::C_TEXTEDITOR);

    // Menus
    Core::ActionContainer *mtools = Core::ActionManager::actionContainer(Core::Constants::M_TOOLS);
    Core::ActionContainer *mmacrotools = Core::ActionManager::createMenu(Constants::M_TOOLS_MACRO);
    QMenu *menu = mmacrotools->menu();
    menu->setTitle(Tr::tr("Text Editing &Macros"));
    menu->setEnabled(true);
    mtools->addMenu(mmacrotools);

    QAction *startMacro = new QAction(Tr::tr("Record Macro"),  this);
    Core::Command *command = Core::ActionManager::registerAction(startMacro, Constants::START_MACRO, textContext);
    command->setDefaultKeySequence(QKeySequence(Core::useMacShortcuts ? Tr::tr("Ctrl+[") : Tr::tr("Alt+[")));
    mmacrotools->addAction(command);
    connect(startMacro, &QAction::triggered, &d->macroManager, &MacroManager::startMacro);

    QAction *endMacro = new QAction(Tr::tr("Stop Recording Macro"),  this);
    endMacro->setEnabled(false);
    command = Core::ActionManager::registerAction(endMacro, Constants::END_MACRO);
    command->setDefaultKeySequence(QKeySequence(Core::useMacShortcuts ? Tr::tr("Ctrl+]") : Tr::tr("Alt+]")));
    mmacrotools->addAction(command);
    connect(endMacro, &QAction::triggered, &d->macroManager, &MacroManager::endMacro);

    QAction *executeLastMacro = new QAction(Tr::tr("Play Last Macro"),  this);
    command = Core::ActionManager::registerAction(executeLastMacro, Constants::EXECUTE_LAST_MACRO, textContext);
    command->setDefaultKeySequence(QKeySequence(Core::useMacShortcuts ? Tr::tr("Meta+R") : Tr::tr("Alt+R")));
    mmacrotools->addAction(command);
    connect(executeLastMacro, &QAction::triggered, &d->macroManager, &MacroManager::executeLastMacro);

    QAction *saveLastMacro = new QAction(Tr::tr("Save Last Macro"),  this);
    saveLastMacro->setEnabled(false);
    command = Core::ActionManager::registerAction(saveLastMacro, Constants::SAVE_LAST_MACRO, textContext);
    mmacrotools->addAction(command);
    connect(saveLastMacro, &QAction::triggered, &d->macroManager, &MacroManager::saveLastMacro);
}

} // Internal
} // Macros
