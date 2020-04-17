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

#include "cmakebuildconfiguration.h"
#include "cmakebuildstep.h"
#include "cmakebuildsystem.h"
#include "cmakeeditor.h"
#include "cmakekitinformation.h"
#include "cmakelocatorfilter.h"
#include "cmakeproject.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectmanager.h"
#include "cmakeprojectnodes.h"
#include "cmakesettingspage.h"
#include "cmakespecificsettings.h"
#include "cmakespecificsettingspage.h"
#include "cmaketoolmanager.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projecttree.h>
#include <texteditor/snippets/snippetprovider.h>

#include <utils/parameteraction.h>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager {
namespace Internal {

class CMakeProjectPluginPrivate
{
public:
    CMakeToolManager cmakeToolManager; // have that before the first CMakeKitAspect

    ParameterAction buildTargetContextAction{
        CMakeProjectPlugin::tr("Build"),
        CMakeProjectPlugin:: tr("Build \"%1\""),
        ParameterAction::AlwaysEnabled/*handled manually*/
    };

    CMakeSettingsPage settingsPage;
    CMakeSpecificSettingsPage specificSettings{CMakeProjectPlugin::projectTypeSpecificSettings()};

    CMakeManager manager;
    CMakeBuildStepFactory buildStepFactory;
    CMakeBuildConfigurationFactory buildConfigFactory;
    CMakeEditorFactory editorFactor;
    BuildCMakeTargetLocatorFilter buildCMakeTargetLocatorFilter;
    OpenCMakeTargetLocatorFilter openCMakeTargetLocationFilter;

    CMakeKitAspect cmakeKitAspect;
    CMakeGeneratorKitAspect cmakeGeneratorKitAspect;
    CMakeConfigurationKitAspect cmakeConfigurationKitAspect;
};

CMakeSpecificSettings *CMakeProjectPlugin::projectTypeSpecificSettings()
{
    static CMakeSpecificSettings theSettings;
    return &theSettings;
}

CMakeProjectPlugin::~CMakeProjectPlugin()
{
    delete d;
}

bool CMakeProjectPlugin::initialize(const QStringList & /*arguments*/, QString *errorMessage)
{
    Q_UNUSED(errorMessage)

    d = new CMakeProjectPluginPrivate;
    projectTypeSpecificSettings()->fromSettings(ICore::settings());

    const Context projectContext{CMakeProjectManager::Constants::CMAKE_PROJECT_ID};

    FileIconProvider::registerIconOverlayForSuffix(Constants::FILE_OVERLAY_CMAKE, "cmake");
    FileIconProvider::registerIconOverlayForFilename(Constants::FILE_OVERLAY_CMAKE,
                                                     "CMakeLists.txt");

    TextEditor::SnippetProvider::registerGroup(Constants::CMAKE_SNIPPETS_GROUP_ID,
                                               tr("CMake", "SnippetProvider"));
    ProjectManager::registerProjectType<CMakeProject>(Constants::CMAKE_PROJECT_MIMETYPE);

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

    connect(&d->buildTargetContextAction, &ParameterAction::triggered, this, [] {
        if (auto bs = qobject_cast<CMakeBuildSystem *>(ProjectTree::currentBuildSystem())) {
            auto targetNode = dynamic_cast<const CMakeTargetNode *>(ProjectTree::currentNode());
            bs->buildCMakeTarget(targetNode ? targetNode->displayName() : QString());
        }
    });

    return true;
}

void CMakeProjectPlugin::extensionsInitialized()
{
    //restore the cmake tools before loading the kits
    CMakeToolManager::restoreCMakeTools();
}

void CMakeProjectPlugin::updateContextActions()
{
    auto targetNode = dynamic_cast<const CMakeTargetNode *>(ProjectTree::currentNode());
    const QString targetDisplayName = targetNode ? targetNode->displayName() : QString();

    // Build Target:
    d->buildTargetContextAction.setParameter(targetDisplayName);
    d->buildTargetContextAction.setEnabled(targetNode);
    d->buildTargetContextAction.setVisible(targetNode);
}

} // Internal
} // CMakeProjectManager

