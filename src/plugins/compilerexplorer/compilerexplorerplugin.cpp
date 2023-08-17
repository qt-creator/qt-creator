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

#include <QMenu>

#include <extensionsystem/iplugin.h>

using namespace Core;

namespace CompilerExplorer::Internal {

class CompilerExplorerPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "CompilerExplorer.json")

public:
    CompilerExplorerPlugin();
    ~CompilerExplorerPlugin() override;

    void initialize() override
    {
        static CompilerExplorer::EditorFactory ceEditorFactory;

        auto action = new QAction(Tr::tr("Open Compiler Explorer"), this);
        connect(action, &QAction::triggered, this, [] {
            CompilerExplorer::Settings settings;

            const QString src = settings.source();
            QString name("Compiler Explorer");
            EditorManager::openEditorWithContents(Constants::CE_EDITOR_ID, &name, src.toUtf8());
        });

        ActionContainer *mtools = ActionManager::actionContainer(Core::Constants::M_TOOLS);
        ActionContainer *mCompilerExplorer = ActionManager::createMenu("Tools.CompilerExplorer");
        QMenu *menu = mCompilerExplorer->menu();
        menu->setTitle(Tr::tr("Compiler Explorer"));
        mtools->addMenu(mCompilerExplorer);

        Command *cmd = ActionManager::registerAction(action, "CompilerExplorer.CompilerExplorerAction");
        mCompilerExplorer->addAction(cmd);
    }
};

} // CompilerExplorer::Internl

#include "compilerexplorerplugin.moc"
