// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakebuildconfiguration.h"
#include "cmakebuildstep.h"
#include "cmakebuildsystem.h"
#include "cmakeeditor.h"
#include "cmakeformatter.h"
#include "cmakeinstallstep.h"
#include "cmakelocatorfilter.h"
#include "cmakeparser.h"
#include "cmakeproject.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectimporter.h"
#include "cmakeprojectmanager.h"
#include "cmakeprojectmanagertr.h"
#include "cmakeprojectnodes.h"
#include "cmakesettingspage.h"
#include "cmaketoolmanager.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>

#include <extensionsystem/iplugin.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projecttree.h>

#include <texteditor/formattexteditor.h>
#include <texteditor/snippets/snippetprovider.h>

#include <utils/mimeconstants.h>
#include <utils/fsengine/fileiconprovider.h>
#include <utils/parameteraction.h>

#include <QTimer>
#include <QMenu>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager::Internal {

class CMakeProjectPluginPrivate : public QObject
{
public:
    Action buildTargetContextAction{
        Tr::tr("Build"),
        Tr::tr("Build \"%1\""),
        Action::AlwaysEnabled/*handled manually*/
    };

    CMakeSettingsPage settingsPage;

    CMakeBuildStepFactory buildStepFactory;
    CMakeBuildConfigurationFactory buildConfigFactory;
    CMakeEditorFactory editorFactor;
    CMakeInstallStepFactory installStepFactory;
    CMakeBuildTargetFilter cMakeBuildTargetFilter;
    CMakeOpenTargetFilter cMakeOpenTargetFilter;

    CMakeFormatter cmakeFormatter;
};

class CMakeProjectPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "CMakeProjectManager.json")

    ~CMakeProjectPlugin()
    {
        delete d;
    }

    void initialize() final
    {
        setupCMakeToolManager(this);

        d = new CMakeProjectPluginPrivate;

        setupCMakeManager();

#ifdef WITH_TESTS
        addTestCreator(createCMakeConfigTest);
        addTestCreator(createCMakeParserTest);
        addTestCreator(createCMakeProjectImporterTest);
#endif

        const Context projectContext{CMakeProjectManager::Constants::CMAKE_PROJECT_ID};

        FileIconProvider::registerIconOverlayForSuffix(Constants::Icons::FILE_OVERLAY, "cmake");
        FileIconProvider::registerIconOverlayForFilename(Constants::Icons::FILE_OVERLAY,
                                                         Constants::CMAKE_LISTS_TXT);

        TextEditor::SnippetProvider::registerGroup(Constants::CMAKE_SNIPPETS_GROUP_ID,
                                                   Tr::tr("CMake", "SnippetProvider"));
        ProjectManager::registerProjectType<CMakeProject>(Utils::Constants::CMAKE_PROJECT_MIMETYPE);

        //register actions
        Command *command = ActionManager::registerAction(&d->buildTargetContextAction,
                                                         Constants::BUILD_TARGET_CONTEXT_MENU,
                                                         projectContext);
        command->setAttribute(Command::CA_Hide);
        command->setAttribute(Command::CA_UpdateText);
        command->setDescription(d->buildTargetContextAction.text());

        ActionManager::actionContainer(ProjectExplorer::Constants::M_SUBPROJECTCONTEXT)
            ->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);

        // Wire up context menu updates:
        connect(ProjectTree::instance(), &ProjectTree::currentNodeChanged,
                this, &CMakeProjectPlugin::updateContextActions);

        connect(&d->buildTargetContextAction, &Action::triggered, this, [] {
            if (auto bs = qobject_cast<CMakeBuildSystem *>(ProjectTree::currentBuildSystem())) {
                auto targetNode = dynamic_cast<const CMakeTargetNode *>(ProjectTree::currentNode());
                bs->buildCMakeTarget(targetNode ? targetNode->displayName() : QString());
            }
        });
    }

    void extensionsInitialized() final
    {
        // Delay the restoration to allow the devices to load first.
        QTimer::singleShot(0, this, [] { CMakeToolManager::restoreCMakeTools(); });
    }

    void updateContextActions(ProjectExplorer::Node *node)
    {
        auto targetNode = dynamic_cast<const CMakeTargetNode *>(node);
        const QString targetDisplayName = targetNode ? targetNode->displayName() : QString();

        // Build Target:
        d->buildTargetContextAction.setParameter(targetDisplayName);
        d->buildTargetContextAction.setEnabled(targetNode);
        d->buildTargetContextAction.setVisible(targetNode);
    }

    class CMakeProjectPluginPrivate *d = nullptr;
};

} // CMakeProjectManager::Internal

#include "cmakeprojectplugin.moc"
