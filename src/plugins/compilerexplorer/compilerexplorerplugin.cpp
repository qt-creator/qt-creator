// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "compilerexplorerplugin.h"

#include "compilerexplorerconstants.h"
#include "compilerexplorereditor.h"
#include "compilerexplorersettings.h"
#include "compilerexplorertr.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>

#include <cppeditor/cppeditorconstants.h>

#include <QMenu>

namespace CompilerExplorer {
namespace Internal {

CompilerExplorerPlugin::CompilerExplorerPlugin() {}

CompilerExplorerPlugin::~CompilerExplorerPlugin() {}

void CompilerExplorerPlugin::initialize()
{
    static CompilerExplorer::EditorFactory ceEditorFactory;

    auto action = new QAction(Tr::tr("Open Compiler Explorer"), this);
    connect(action, &QAction::triggered, this, [] {
        CompilerExplorer::Settings settings;

        const QString src = settings.source();
        QString name("Compiler Explorer");
        Core::EditorManager::openEditorWithContents(Constants::CE_EDITOR_ID, &name, src.toUtf8());
    });

    Core::ActionContainer *mtools = Core::ActionManager::actionContainer(Core::Constants::M_TOOLS);
    Core::ActionContainer *mCompilerExplorer = Core::ActionManager::createMenu(
        "Tools.CompilerExplorer");
    QMenu *menu = mCompilerExplorer->menu();
    menu->setTitle(Tr::tr("Compiler Explorer"));
    mtools->addMenu(mCompilerExplorer);

    Core::Command *cmd
        = Core::ActionManager::registerAction(action, "CompilerExplorer.CompilerExplorerAction");
    mCompilerExplorer->addAction(cmd);
}

void CompilerExplorerPlugin::extensionsInitialized() {}

} // namespace Internal
} // namespace CompilerExplorer
