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

#include "cmakeprojectmanager.h"

#include "cmakebuildsystem.h"
#include "cmakekitinformation.h"
#include "cmakeproject.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectnodes.h"
#include "fileapiparser.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <utils/parameteraction.h>

#include <QAction>
#include <QFileDialog>
#include <QMessageBox>

using namespace ProjectExplorer;
using namespace CMakeProjectManager::Internal;

CMakeManager::CMakeManager()
    : m_runCMakeAction(new QAction(QIcon(), tr("Run CMake"), this))
    , m_clearCMakeCacheAction(new QAction(QIcon(), tr("Clear CMake Configuration"), this))
    , m_runCMakeActionContextMenu(new QAction(QIcon(), tr("Run CMake"), this))
    , m_rescanProjectAction(new QAction(QIcon(), tr("Rescan Project"), this))
    , m_parseAndValidateCMakeReplyFileAction(
          new QAction(QIcon(), tr("Parse and verify a CMake reply file."), this))
{
    Core::ActionContainer *mbuild =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_BUILDPROJECT);
    Core::ActionContainer *mproject =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_PROJECTCONTEXT);
    Core::ActionContainer *msubproject =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_SUBPROJECTCONTEXT);
    Core::ActionContainer *mfile =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_FILECONTEXT);

    const Core::Context projectContext(CMakeProjectManager::Constants::CMAKE_PROJECT_ID);
    const Core::Context globalContext(Core::Constants::C_GLOBAL);

    Core::Command *command = Core::ActionManager::registerAction(m_runCMakeAction,
                                                                 Constants::RUN_CMAKE,
                                                                 globalContext);
    command->setAttribute(Core::Command::CA_Hide);
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_DEPLOY);
    connect(m_runCMakeAction, &QAction::triggered, [this]() {
        runCMake(SessionManager::startupBuildSystem());
    });

    command = Core::ActionManager::registerAction(m_clearCMakeCacheAction,
                                                  Constants::CLEAR_CMAKE_CACHE,
                                                  globalContext);
    command->setAttribute(Core::Command::CA_Hide);
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_DEPLOY);
    connect(m_clearCMakeCacheAction, &QAction::triggered, [this]() {
        clearCMakeCache(SessionManager::startupBuildSystem());
    });

    command = Core::ActionManager::registerAction(m_runCMakeActionContextMenu,
                                                  Constants::RUN_CMAKE_CONTEXT_MENU,
                                                  projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    mproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_runCMakeActionContextMenu, &QAction::triggered, [this]() {
        runCMake(ProjectTree::currentBuildSystem());
    });

    m_buildFileContextMenu = new QAction(tr("Build"), this);
    command = Core::ActionManager::registerAction(m_buildFileContextMenu,
                                                  Constants::BUILD_FILE_CONTEXT_MENU,
                                                  projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    mfile->addAction(command, ProjectExplorer::Constants::G_FILE_OTHER);
    connect(m_buildFileContextMenu, &QAction::triggered,
            this, &CMakeManager::buildFileContextMenu);

    command = Core::ActionManager::registerAction(m_rescanProjectAction,
                                                  Constants::RESCAN_PROJECT,
                                                  globalContext);
    command->setAttribute(Core::Command::CA_Hide);
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_DEPLOY);
    connect(m_rescanProjectAction, &QAction::triggered, [this]() {
        rescanProject(ProjectTree::currentBuildSystem());
    });

    m_buildFileAction = new Utils::ParameterAction(tr("Build File"),
                                                   tr("Build File \"%1\""),
                                                   Utils::ParameterAction::AlwaysEnabled,
                                                   this);
    command = Core::ActionManager::registerAction(m_buildFileAction, Constants::BUILD_FILE);
    command->setAttribute(Core::Command::CA_Hide);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDescription(m_buildFileAction->text());
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl+Alt+B")));
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_BUILD);
    connect(m_buildFileAction, &QAction::triggered, this, [this] { buildFile(); });

    command = Core::ActionManager::registerAction(m_parseAndValidateCMakeReplyFileAction,
                                                  "CMakeProject.Debug.ParseAndVerifyReplyFile");
    connect(m_parseAndValidateCMakeReplyFileAction,
            &QAction::triggered,
            this,
            &CMakeManager::parseAndValidateCMakeReplyFile);

    connect(SessionManager::instance(), &SessionManager::startupProjectChanged,
            this, &CMakeManager::updateCmakeActions);
    connect(BuildManager::instance(), &BuildManager::buildStateChanged,
            this, &CMakeManager::updateCmakeActions);
    connect(Core::EditorManager::instance(), &Core::EditorManager::currentEditorChanged,
            this, &CMakeManager::updateBuildFileAction);
    connect(ProjectTree::instance(), &ProjectTree::currentNodeChanged,
            this, &CMakeManager::updateCmakeActions);

    updateCmakeActions();
}

void CMakeManager::updateCmakeActions()
{
    auto project = qobject_cast<CMakeProject *>(SessionManager::startupProject());
    const bool visible = project && !BuildManager::isBuilding(project);
    m_runCMakeAction->setVisible(visible);
    m_clearCMakeCacheAction->setVisible(visible);
    m_rescanProjectAction->setVisible(visible);
    enableBuildFileMenus(ProjectTree::currentNode());
}

void CMakeManager::clearCMakeCache(BuildSystem *buildSystem)
{
    auto cmakeBuildSystem = dynamic_cast<CMakeBuildSystem *>(buildSystem);
    QTC_ASSERT(cmakeBuildSystem, return);

    cmakeBuildSystem->clearCMakeCache();
}

void CMakeManager::runCMake(BuildSystem *buildSystem)
{
    auto cmakeBuildSystem = dynamic_cast<CMakeBuildSystem *>(buildSystem);
    QTC_ASSERT(cmakeBuildSystem, return );

    if (ProjectExplorerPlugin::saveModifiedFiles())
        cmakeBuildSystem->runCMake();
}

void CMakeManager::rescanProject(BuildSystem *buildSystem)
{
    auto cmakeBuildSystem = dynamic_cast<CMakeBuildSystem *>(buildSystem);
    QTC_ASSERT(cmakeBuildSystem, return);

    cmakeBuildSystem->runCMakeAndScanProjectTree();// by my experience: every rescan run requires cmake run too
}

void CMakeManager::updateBuildFileAction()
{
    Node *node = nullptr;
    if (Core::IDocument *currentDocument = Core::EditorManager::currentDocument())
        node = ProjectTree::nodeForFile(currentDocument->filePath());
    enableBuildFileMenus(node);
}

void CMakeManager::enableBuildFileMenus(Node *node)
{
    m_buildFileAction->setVisible(false);
    m_buildFileAction->setEnabled(false);
    m_buildFileAction->setParameter(QString());
    m_buildFileContextMenu->setEnabled(false);

    if (!node)
        return;
    Project *project = ProjectTree::projectForNode(node);
    if (!project)
        return;
    Target *target = project->activeTarget();
    if (!target)
        return;
    const QString generator = CMakeGeneratorKitAspect::generator(target->kit());
    if (generator != "Ninja" && !generator.contains("Makefiles"))
        return;

    if (const FileNode *fileNode = node->asFileNode()) {
        const FileType type = fileNode->fileType();
        const bool visible = qobject_cast<CMakeProject *>(project)
                && dynamic_cast<CMakeTargetNode *>(node->parentProjectNode())
                && (type == FileType::Source || type == FileType::Header);

        const bool enabled = visible && !BuildManager::isBuilding(project);
        m_buildFileAction->setVisible(visible);
        m_buildFileAction->setEnabled(enabled);
        m_buildFileAction->setParameter(node->filePath().fileName());
        m_buildFileContextMenu->setEnabled(enabled);
    }
}

void CMakeManager::parseAndValidateCMakeReplyFile()
{
    QString replyFile = QFileDialog::getOpenFileName(Core::ICore::mainWindow(),
                                                     tr("Select a CMake Reply File"),
                                                     QString(),
                                                     QString("index*.json"));
    if (replyFile.isEmpty())
        return;

    QString errorMessage;
    auto result = FileApiParser::parseData(QFileInfo(replyFile), errorMessage);

    const QString message
        = errorMessage.isEmpty()
              ? tr("The reply file \"%1\" and referenced data parsed OK and passed validation.")
                    .arg(QDir::toNativeSeparators(replyFile))
              : tr("The reply file \"%1\" failed to parse or validate with error "
                   "message:<br><b>\"%2\"</b>")
                    .arg(QDir::toNativeSeparators(replyFile))
                    .arg(errorMessage);
    QMessageBox::information(Core::ICore::mainWindow(), tr("Parsing Result"), message);
}

void CMakeManager::buildFile(Node *node)
{
    if (!node) {
        Core::IDocument *currentDocument= Core::EditorManager::currentDocument();
        if (!currentDocument)
            return;
        const Utils::FilePath file = currentDocument->filePath();
        node = ProjectTree::nodeForFile(file);
    }
    FileNode *fileNode  = node ? node->asFileNode() : nullptr;
    if (!fileNode)
        return;
    Project *project = ProjectTree::projectForNode(fileNode);
    if (!project)
        return;
    CMakeTargetNode *targetNode = dynamic_cast<CMakeTargetNode *>(fileNode->parentProjectNode());
    if (!targetNode)
        return;
    Target *target = project->activeTarget();
    QTC_ASSERT(target, return);
    const QString generator = CMakeGeneratorKitAspect::generator(target->kit());
    const QString relativeSource = fileNode->filePath().relativeChildPath(targetNode->filePath()).toString();
    const QString objExtension = Utils::HostOsInfo::isWindowsHost() ? QString(".obj") : QString(".o");
    Utils::FilePath targetBase;
    BuildConfiguration *bc = target->activeBuildConfiguration();
    QTC_ASSERT(bc, return);
    if (generator == "Ninja") {
        const Utils::FilePath relativeBuildDir = targetNode->buildDirectory().relativeChildPath(
                    bc->buildDirectory());
        targetBase = relativeBuildDir / "CMakeFiles" / (targetNode->displayName() + ".dir");
    } else if (!generator.contains("Makefiles")) {
        Core::MessageManager::write(tr("Build File is not supported for generator \"%1\"")
                                    .arg(generator));
        return;
    }

    static_cast<CMakeBuildSystem *>(bc->buildSystem())
            ->buildCMakeTarget(targetBase.pathAppended(relativeSource).toString() + objExtension);
}

void CMakeManager::buildFileContextMenu()
{
    if (Node *node = ProjectTree::currentNode())
        buildFile(node);
}
