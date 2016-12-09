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
#include "cmakesnippetprovider.h"
#include "cmakeprojectconstants.h"
#include "cmakelocatorfilter.h"
#include "cmakesettingspage.h"
#include "cmaketoolmanager.h"
#include "cmakekitinformation.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/fileiconprovider.h>

#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projecttree.h>

#include <utils/mimetypes/mimedatabase.h>
#include <utils/parameteraction.h>

using namespace CMakeProjectManager::Internal;
using namespace Core;
using namespace ProjectExplorer;

bool CMakeProjectPlugin::initialize(const QStringList & /*arguments*/, QString *errorMessage)
{
    Q_UNUSED(errorMessage)
    const Context projectContext(Constants::PROJECTCONTEXT);

    Utils::MimeDatabase::addMimeTypes(QLatin1String(":cmakeproject/CMakeProjectManager.mimetypes.xml"));

    Core::FileIconProvider::registerIconOverlayForSuffix(Constants::FILEOVERLAY_CMAKE, "cmake");
    Core::FileIconProvider::registerIconOverlayForFilename(Constants::FILEOVERLAY_CMAKE, "CMakeLists.txt");

    addAutoReleasedObject(new Internal::CMakeSnippetProvider);
    addAutoReleasedObject(new CMakeSettingsPage);
    addAutoReleasedObject(new CMakeManager);
    addAutoReleasedObject(new CMakeBuildStepFactory);
    addAutoReleasedObject(new CMakeRunConfigurationFactory);
    addAutoReleasedObject(new CMakeBuildConfigurationFactory);
    addAutoReleasedObject(new CMakeEditorFactory);
    addAutoReleasedObject(new CMakeLocatorFilter);

    new CMakeToolManager(this);

    KitManager::registerKitInformation(new CMakeKitInformation);
    KitManager::registerKitInformation(new CMakeGeneratorKitInformation);
    KitManager::registerKitInformation(new CMakeConfigurationKitInformation);

    //menus
    ActionContainer *msubproject =
            ActionManager::actionContainer(ProjectExplorer::Constants::M_SUBPROJECTCONTEXT);

    //register actions
    Command *command = nullptr;

    m_buildTargetContextAction = new Utils::ParameterAction(tr("Build"), tr("Build \"%1\""),
                                                            Utils::ParameterAction::AlwaysEnabled/*handled manually*/,
                                                            this);
    command = ActionManager::registerAction(m_buildTargetContextAction, Constants::BUILD_TARGET_CONTEXTMENU, projectContext);
    command->setAttribute(Command::CA_Hide);
    command->setAttribute(Command::CA_UpdateText);
    command->setDescription(m_buildTargetContextAction->text());
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

void CMakeProjectPlugin::updateContextActions(ProjectExplorer::Node *node,
                                              ProjectExplorer::Project *project)
{
    CMakeTargetNode *targetNode = dynamic_cast<CMakeTargetNode *>(node);
    // as targetNode can be deleted while the menu is open, we keep only the
    const QString targetDisplayName = targetNode ? targetNode->displayName() : QString();
    CMakeProject *cmProject = dynamic_cast<CMakeProject *>(project);

    // Build Target:
    disconnect(m_actionConnect);
    m_buildTargetContextAction->setParameter(targetDisplayName);
    m_buildTargetContextAction->setEnabled(targetNode);
    m_buildTargetContextAction->setVisible(targetNode);
    if (cmProject && targetNode) {
        m_actionConnect = connect(m_buildTargetContextAction, &Utils::ParameterAction::triggered,
            cmProject, [cmProject, targetDisplayName]() { cmProject->buildCMakeTarget(targetDisplayName); });
    }
}
