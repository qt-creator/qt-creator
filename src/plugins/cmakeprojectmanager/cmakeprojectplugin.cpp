// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakebuildconfiguration.h"
#include "cmakebuildstep.h"
#include "cmakebuildsystem.h"
#include "cmakeeditor.h"
#include "cmakeformatter.h"
#include "cmakeinstallstep.h"
#include "cmakelocatorfilter.h"
#include "cmakekitaspect.h"
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

#include <utils/action.h>
#include <utils/mimeconstants.h>
#include <utils/fsengine/fileiconprovider.h>

#include <QTimer>
#include <QMenu>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager::Internal {

class CMakeProjectPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "CMakeProjectManager.json")

    void initialize() final
    {
        setupCMakeToolManager(this);

        setupCMakeSettingsPage();
        setupCMakeKitAspects();

        setupCMakeBuildConfiguration();
        setupCMakeBuildStep();
        setupCMakeInstallStep();

        setupCMakeEditor();

        setupCMakeLocatorFilters();
        setupCMakeFormatter();

        setupCMakeManager();

#ifdef WITH_TESTS
        addTestCreator(createCMakeConfigTest);
        addTestCreator(createCMakeParserTest);
        addTestCreator(createCMakeProjectImporterTest);
#endif

        FileIconProvider::registerIconOverlayForSuffix(Constants::Icons::FILE_OVERLAY, "cmake");
        FileIconProvider::registerIconOverlayForFilename(Constants::Icons::FILE_OVERLAY,
                                                         Constants::CMAKE_LISTS_TXT);

        TextEditor::SnippetProvider::registerGroup(Constants::CMAKE_SNIPPETS_GROUP_ID,
                                                   Tr::tr("CMake", "SnippetProvider"));
        ProjectManager::registerProjectType<CMakeProject>(Utils::Constants::CMAKE_PROJECT_MIMETYPE);

        ActionBuilder(this, Constants::BUILD_TARGET_CONTEXT_MENU)
            .setParameterText(Tr::tr("Build \"%1\""), Tr::tr("Build"), ActionBuilder::AlwaysEnabled)
            .setContext(CMakeProjectManager::Constants::CMAKE_PROJECT_ID)
            .bindContextAction(&m_buildTargetContextAction)
            .setCommandAttribute(Command::CA_Hide)
            .setCommandAttribute(Command::CA_UpdateText)
            .setCommandDescription(m_buildTargetContextAction->text())
            .addToContainer(ProjectExplorer::Constants::M_SUBPROJECTCONTEXT,
                            ProjectExplorer::Constants::G_PROJECT_BUILD)
            .addOnTriggered(this, [] {
                if (auto bs = qobject_cast<CMakeBuildSystem *>(ProjectTree::currentBuildSystem())) {
                    auto targetNode = dynamic_cast<const CMakeTargetNode *>(ProjectTree::currentNode());
                    bs->buildCMakeTarget(targetNode ? targetNode->displayName() : QString());
                }
            });

        connect(ProjectTree::instance(), &ProjectTree::currentNodeChanged,
                this, &CMakeProjectPlugin::updateContextActions);
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
        m_buildTargetContextAction->setParameter(targetDisplayName);
        m_buildTargetContextAction->setEnabled(targetNode);
        m_buildTargetContextAction->setVisible(targetNode);
    }

    Action *m_buildTargetContextAction = nullptr;
};

} // CMakeProjectManager::Internal

#include "cmakeprojectplugin.moc"
