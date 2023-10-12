// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

#include <extensionsystem/iplugin.h>

#include <projectexplorer/jsonwizard/jsonwizardfactory.h>

#include <utils/fsengine/fileiconprovider.h>

#include <QMenu>

using namespace Core;

namespace CompilerExplorer::Internal {

class CompilerExplorerPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "CompilerExplorer.json")

public:
    void initialize() override
    {
        static CompilerExplorer::EditorFactory ceEditorFactory;

        auto action = new QAction(Tr::tr("Open Compiler Explorer"), this);
        connect(action, &QAction::triggered, this, [] {
            QString name("Compiler Explorer $");
            Core::EditorManager::openEditorWithContents(Constants::CE_EDITOR_ID,
                                                        &name,
                                                        settings().defaultDocument().toUtf8());
        });

        Utils::FileIconProvider::registerIconForMimeType(QIcon(":/compilerexplorer/logos/ce.ico"),
                                                         "application/compiler-explorer");

        ProjectExplorer::JsonWizardFactory::addWizardPath(":/compilerexplorer/wizard/");

        ActionContainer *mtools = ActionManager::actionContainer(Core::Constants::M_TOOLS);
        ActionContainer *mCompilerExplorer = ActionManager::createMenu("Tools.CompilerExplorer");
        QMenu *menu = mCompilerExplorer->menu();
        menu->setTitle(Tr::tr("Compiler Explorer"));
        mtools->addMenu(mCompilerExplorer);

        Command *cmd = ActionManager::registerAction(action,
                                                     "CompilerExplorer.CompilerExplorerAction");
        mCompilerExplorer->addAction(cmd);
    }
};

} // namespace CompilerExplorer::Internal

#include "compilerexplorerplugin.moc"
