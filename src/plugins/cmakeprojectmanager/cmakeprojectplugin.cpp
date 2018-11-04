/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "cmakeprojectplugin.h"

#include "cmakeeditor.h"
#include "cmakebuildstep.h"
#include "cmakeproject.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectmanager.h"
#include "cmakeprojectnodes.h"
#include "cmakebuildconfiguration.h"
#include "cmakerunconfiguration.h"
#include "cmakeprojectconstants.h"
#include "cmakelocatorfilter.h"
#include "cmakesettingspage.h"
#include "cmaketoolmanager.h"
#include "cmakekitinformation.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/icore.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projecttree.h>
#include <texteditor/snippets/snippetprovider.h>

#include <utils/parameteraction.h>

using namespace Core;
using namespace ProjectExplorer;

namespace CMakeProjectManager {
namespace Internal {

class CMakeProjectPluginPrivate
{
public:
    Utils::ParameterAction *m_buildTargetContextAction = nullptr;
    QMetaObject::Connection m_actionConnect;

    CMakeSettingsPage settingsPage;
    static const std::unique_ptr<CMakeSpecificSettings> projectTypeSpecificSettings;
    CMakeManager manager;
    CMakeBuildStepFactory buildStepFactory;
    CMakeRunConfigurationFactory runConfigFactory;
    CMakeBuildConfigurationFactory buildConfigFactory;
    CMakeEditorFactory editorFactor;
    CMakeLocatorFilter locatorFiler;
};

const std::unique_ptr<CMakeSpecificSettings>
CMakeProjectPluginPrivate::projectTypeSpecificSettings{std::make_unique<CMakeSpecificSettings>()};

CMakeSpecificSettings *CMakeProjectPlugin::projectTypeSpecificSettings()
{
    return CMakeProjectPluginPrivate::projectTypeSpecificSettings.get();
}

CMakeProjectPlugin::~CMakeProjectPlugin()
{
    delete d;
}

bool CMakeProjectPlugin::initialize(const QStringList & /*arguments*/, QString *errorMessage)
{
    Q_UNUSED(errorMessage)

    d = new CMakeProjectPluginPrivate;
    CMakeProjectPluginPrivate::projectTypeSpecificSettings->fromSettings(ICore::settings());
    new CMakeSpecificSettingsPage(CMakeProjectPluginPrivate::projectTypeSpecificSettings.get(),
                                  this); //do not store as this will be cleaned after program close

    const Context projectContext(CMakeProjectManager::Constants::CMAKEPROJECT_ID);

    Core::FileIconProvider::registerIconOverlayForSuffix(Constants::FILEOVERLAY_CMAKE, "cmake");
    Core::FileIconProvider::registerIconOverlayForFilename(Constants::FILEOVERLAY_CMAKE,
                                                           "CMakeLists.txt");

    TextEditor::SnippetProvider::registerGroup(Constants::CMAKE_SNIPPETS_GROUP_ID,
                                               tr("CMake", "SnippetProvider"));
    ProjectManager::registerProjectType<CMakeProject>(Constants::CMAKEPROJECTMIMETYPE);

    new CMakeToolManager(this);

    KitManager::registerKitInformation<CMakeKitInformation>();
    KitManager::registerKitInformation<CMakeGeneratorKitInformation>();
    KitManager::registerKitInformation<CMakeConfigurationKitInformation>();

    //menus
    ActionContainer *msubproject =
            ActionManager::actionContainer(ProjectExplorer::Constants::M_SUBPROJECTCONTEXT);

    //register actions
    Command *command = nullptr;

    d->m_buildTargetContextAction = new Utils::ParameterAction(tr("Build"), tr("Build \"%1\""),
                                                               Utils::ParameterAction::AlwaysEnabled/*handled manually*/,
                                                               this);
    command = ActionManager::registerAction(d->m_buildTargetContextAction,
                                            Constants::BUILD_TARGET_CONTEXTMENU, projectContext);
    command->setAttribute(Command::CA_Hide);
    command->setAttribute(Command::CA_UpdateText);
    command->setDescription(d->m_buildTargetContextAction->text());
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);

    // Wire up context menu updates:
    connect(ProjectTree::instance(), &ProjectTree::currentNodeChanged,
            this, &CMakeProjectPlugin::updateContextActions);

    return true;
}

void CMakeProjectPlugin::extensionsInitialized()
{
    //restore the cmake tools before loading the kits
    CMakeToolManager::restoreCMakeTools();
}

void CMakeProjectPlugin::updateContextActions()
{
    Project *project = ProjectTree::currentProject();
    const Node *node = ProjectTree::findCurrentNode();
    auto targetNode = dynamic_cast<const CMakeTargetNode *>(node);
    // as targetNode can be deleted while the menu is open, we keep only the
    const QString targetDisplayName = targetNode ? targetNode->displayName() : QString();
    auto cmProject = dynamic_cast<CMakeProject *>(project);

    // Build Target:
    disconnect(d->m_actionConnect);
    d->m_buildTargetContextAction->setParameter(targetDisplayName);
    d->m_buildTargetContextAction->setEnabled(targetNode);
    d->m_buildTargetContextAction->setVisible(targetNode);
    if (cmProject && targetNode) {
        d->m_actionConnect = connect(d->m_buildTargetContextAction, &Utils::ParameterAction::triggered,
        cmProject, [cmProject, targetDisplayName]() {
            cmProject->buildCMakeTarget(targetDisplayName);
        });
    }
}

} // Internal
} // CMakeProjectManager

