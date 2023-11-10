// Copyright (C) 2016 Nicolas Arnaud-Cormos
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "macrolocatorfilter.h"
#include "macromanager.h"
#include "macrooptionspage.h"
#include "macrosconstants.h"
#include "macrostr.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>

#include <extensionsystem/iplugin.h>

#include <texteditor/texteditorconstants.h>

#include <QMenu>

using namespace Core;

namespace Macros::Internal {

class MacrosPluginPrivate final
{
public:
    MacroManager macroManager;
    MacroOptionsPage optionsPage;
    MacroLocatorFilter locatorFilter;
};

class MacrosPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Macros.json")

public:
    ~MacrosPlugin() final
    {
        delete d;
    }

    void initialize() final
    {
        d = new MacrosPluginPrivate;

        Context textContext(TextEditor::Constants::C_TEXTEDITOR);

        // Menus
        ActionContainer *mtools = ActionManager::actionContainer(Core::Constants::M_TOOLS);
        ActionContainer *mmacrotools = ActionManager::createMenu(Constants::M_TOOLS_MACRO);
        QMenu *menu = mmacrotools->menu();
        menu->setTitle(Tr::tr("Text Editing &Macros"));
        menu->setEnabled(true);
        mtools->addMenu(mmacrotools);

        ActionBuilder startMacro(this, Constants::START_MACRO);
        startMacro.setText(Tr::tr("Record Macro"));
        startMacro.setContext(textContext);
        startMacro.setDefaultKeySequence(Tr::tr("Ctrl+["), Tr::tr("Alt+["));
        startMacro.setContainer(Constants::M_TOOLS_MACRO);
        startMacro.setOnTriggered(this, [this] { d->macroManager.startMacro(); });

        ActionBuilder endMacro(this, Constants::END_MACRO);
        endMacro.setText(Tr::tr("Stop Recording Macro"));
        endMacro.setContext(textContext);
        endMacro.setEnabled(false);
        endMacro.setDefaultKeySequence(Tr::tr("Ctrl+]"), Tr::tr("Alt+]"));
        endMacro.setContainer(Constants::M_TOOLS_MACRO);
        endMacro.setOnTriggered(this, [this] { d->macroManager.endMacro(); });

        ActionBuilder executeLastMacro(this, Constants::EXECUTE_LAST_MACRO);
        executeLastMacro.setText(Tr::tr("Play Last Macro"));
        executeLastMacro.setContext(textContext);
        executeLastMacro.setDefaultKeySequence(Tr::tr("Meta+R"), Tr::tr("Alt+R"));
        executeLastMacro.setContainer(Constants::M_TOOLS_MACRO);
        executeLastMacro.setOnTriggered(this, [this] { d->macroManager.executeLastMacro(); });

        ActionBuilder saveLastMacro(this, Constants::SAVE_LAST_MACRO);
        saveLastMacro.setContext(textContext);
        saveLastMacro.setText(Tr::tr("Save Last Macro"));
        saveLastMacro.setEnabled(false);
        saveLastMacro.setContainer(Constants::M_TOOLS_MACRO);
        saveLastMacro.setOnTriggered(this, [this] { d->macroManager.saveLastMacro(); });
    }

private:
    MacrosPluginPrivate *d = nullptr;
};

} // Macros::Internal

#include "macrosplugin.moc"
