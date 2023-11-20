// Copyright (c) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "haskellbuildconfiguration.h"
#include "haskellconstants.h"
#include "haskelleditorfactory.h"
#include "haskellmanager.h"
#include "haskellproject.h"
#include "haskellrunconfiguration.h"
#include "haskelltr.h"
#include "stackbuildstep.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>

#include <extensionsystem/iplugin.h>

#include <projectexplorer/jsonwizard/jsonwizardfactory.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/runcontrol.h>

#include <texteditor/snippets/snippetprovider.h>

#include <QAction>

namespace Haskell::Internal {

class HaskellPluginPrivate
{
public:
    HaskellEditorFactory editorFactory;
    HaskellRunConfigurationFactory runConfigFactory;
    ProjectExplorer::SimpleTargetRunnerFactory runWorkerFactory{{Constants::C_HASKELL_RUNCONFIG_ID}};
};

static void registerGhciAction(QObject *guard)
{
    QAction *action = new QAction(Tr::tr("Run GHCi"), guard);
    Core::ActionManager::registerAction(action, Constants::A_RUN_GHCI);
    QObject::connect(action, &QAction::triggered, guard, [] {
        if (Core::IDocument *doc = Core::EditorManager::currentDocument())
            HaskellManager::openGhci(doc->filePath());
    });
}

class HaskellPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Haskell.json")

public:
    ~HaskellPlugin() final { delete d; }

private:
    void initialize() final
    {
        d = new HaskellPluginPrivate;

        setupHaskellStackBuildStep();
        setupHaskellBuildConfiguration();

        ProjectExplorer::ProjectManager::registerProjectType<HaskellProject>(
            Constants::C_HASKELL_PROJECT_MIMETYPE);
        TextEditor::SnippetProvider::registerGroup(Constants::C_HASKELLSNIPPETSGROUP_ID,
                                                   Tr::tr("Haskell", "SnippetProvider"));

        registerGhciAction(this);

        ProjectExplorer::JsonWizardFactory::addWizardPath(":/haskell/share/wizards/");
    }

    HaskellPluginPrivate *d = nullptr;
};

} // Haskell::Internal

#include "haskellplugin.moc"
