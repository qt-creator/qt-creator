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

#include "qmakeprojectmanagerplugin.h"

#include "profileeditor.h"
#include "qmakeprojectmanager.h"
#include "qmakenodes.h"
#include "qmakestep.h"
#include "qmakemakestep.h"
#include "qmakebuildconfiguration.h"
#include "desktopqmakerunconfiguration.h"
#include "wizards/guiappwizard.h"
#include "wizards/librarywizard.h"
#include "wizards/simpleprojectwizard.h"
#include "wizards/subdirsprojectwizard.h"
#include "customwidgetwizard/customwidgetwizard.h"
#include "qmakeprojectmanagerconstants.h"
#include "qmakeproject.h"
#include "externaleditors.h"
#include "qmakekitinformation.h"
#include "profilehighlighter.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorconstants.h>

#include <utils/hostosinfo.h>
#include <utils/parameteraction.h>

#ifdef WITH_TESTS
#    include <QTest>
#endif

using namespace Core;
using namespace ProjectExplorer;

namespace QmakeProjectManager {
namespace Internal {

class QmakeProjectManagerPluginPrivate : public QObject
{
public:
    ~QmakeProjectManagerPluginPrivate() override;

    void projectChanged();
    void activeTargetChanged();
    void updateActions();
    void updateRunQMakeAction();
    void updateContextActions();
    void buildStateChanged(Project *pro);
    void updateBuildFileAction();
    void disableBuildFileMenus();
    void enableBuildFileMenus(const Utils::FileName &file);

    QmakeManager qmakeProjectManager;
    Core::Context projectContext;

    CustomWizardMetaFactory<CustomQmakeProjectWizard>
        qmakeProjectWizard{"qmakeproject", IWizardFactory::ProjectWizard};

    QMakeStepFactory qmakeStepFactory;
    QmakeMakeStepFactory makeStepFactory;

    QmakeBuildConfigurationFactory buildConfigFactory;
    DesktopQmakeRunConfigurationFactory runConfigFactory;

    ProFileEditorFactory profileEditorFactory;

    ExternalQtEditor *m_designerEditor{ExternalQtEditor::createDesignerEditor()};
    ExternalQtEditor *m_linguistEditor{ExternalQtEditor::createLinguistEditor()};

    QmakeProject *m_previousStartupProject = nullptr;
    ProjectExplorer::Target *m_previousTarget = nullptr;

    QAction *m_runQMakeAction = nullptr;
    QAction *m_runQMakeActionContextMenu = nullptr;
    Utils::ParameterAction *m_buildSubProjectContextMenu = nullptr;
    QAction *m_subProjectRebuildSeparator = nullptr;
    QAction *m_rebuildSubProjectContextMenu = nullptr;
    QAction *m_cleanSubProjectContextMenu = nullptr;
    QAction *m_buildFileContextMenu = nullptr;
    Utils::ParameterAction *m_buildSubProjectAction = nullptr;
    Utils::ParameterAction *m_rebuildSubProjectAction = nullptr;
    Utils::ParameterAction *m_cleanSubProjectAction = nullptr;
    Utils::ParameterAction *m_buildFileAction = nullptr;
    QAction *m_addLibraryAction = nullptr;
    QAction *m_addLibraryActionContextMenu = nullptr;
};

QmakeProjectManagerPlugin::~QmakeProjectManagerPlugin()
{
    delete d;
}

bool QmakeProjectManagerPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)
    const Context projectContext(QmakeProjectManager::Constants::QMAKEPROJECT_ID);
    Context projecTreeContext(ProjectExplorer::Constants::C_PROJECT_TREE);

    d = new QmakeProjectManagerPluginPrivate;

    //create and register objects
    ProjectManager::registerProjectType<QmakeProject>(QmakeProjectManager::Constants::PROFILE_MIMETYPE);

    ProjectExplorer::KitManager::registerKitInformation<QmakeKitInformation>();

    IWizardFactory::registerFactoryCreator([] {
        return QList<IWizardFactory *> {
            new SubdirsProjectWizard,
            new GuiAppWizard,
            new LibraryWizard,
            new CustomWidgetWizard,
            new SimpleProjectWizard
        };
    });

    //menus
    ActionContainer *mbuild =
            ActionManager::actionContainer(ProjectExplorer::Constants::M_BUILDPROJECT);
    ActionContainer *mproject =
            ActionManager::actionContainer(ProjectExplorer::Constants::M_PROJECTCONTEXT);
    ActionContainer *msubproject =
            ActionManager::actionContainer(ProjectExplorer::Constants::M_SUBPROJECTCONTEXT);
    ActionContainer *mfile =
            ActionManager::actionContainer(ProjectExplorer::Constants::M_FILECONTEXT);

    //register actions
    Command *command = nullptr;

    d->m_buildSubProjectContextMenu = new Utils::ParameterAction(tr("Build"), tr("Build \"%1\""),
                                                              Utils::ParameterAction::AlwaysEnabled/*handled manually*/,
                                                              this);
    command = ActionManager::registerAction(d->m_buildSubProjectContextMenu, Constants::BUILDSUBDIRCONTEXTMENU, projectContext);
    command->setAttribute(Command::CA_Hide);
    command->setAttribute(Command::CA_UpdateText);
    command->setDescription(d->m_buildSubProjectContextMenu->text());
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(d->m_buildSubProjectContextMenu, &QAction::triggered,
            &d->qmakeProjectManager, &QmakeManager::buildSubDirContextMenu);

    d->m_runQMakeActionContextMenu = new QAction(tr("Run qmake"), this);
    command = ActionManager::registerAction(d->m_runQMakeActionContextMenu, Constants::RUNQMAKECONTEXTMENU, projectContext);
    command->setAttribute(Command::CA_Hide);
    mproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(d->m_runQMakeActionContextMenu, &QAction::triggered,
            &d->qmakeProjectManager, &QmakeManager::runQMakeContextMenu);

    command = msubproject->addSeparator(projectContext, ProjectExplorer::Constants::G_PROJECT_BUILD,
                                        &d->m_subProjectRebuildSeparator);
    command->setAttribute(Command::CA_Hide);

    d->m_rebuildSubProjectContextMenu = new QAction(tr("Rebuild"), this);
    command = ActionManager::registerAction(
                d->m_rebuildSubProjectContextMenu, Constants::REBUILDSUBDIRCONTEXTMENU, projectContext);
    command->setAttribute(Command::CA_Hide);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(d->m_rebuildSubProjectContextMenu, &QAction::triggered,
            &d->qmakeProjectManager, &QmakeManager::rebuildSubDirContextMenu);

    d->m_cleanSubProjectContextMenu = new QAction(tr("Clean"), this);
    command = ActionManager::registerAction(
                d->m_cleanSubProjectContextMenu, Constants::CLEANSUBDIRCONTEXTMENU, projectContext);
    command->setAttribute(Command::CA_Hide);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(d->m_cleanSubProjectContextMenu, &QAction::triggered,
            &d->qmakeProjectManager, &QmakeManager::cleanSubDirContextMenu);

    d->m_buildFileContextMenu = new QAction(tr("Build"), this);
    command = ActionManager::registerAction(d->m_buildFileContextMenu, Constants::BUILDFILECONTEXTMENU, projectContext);
    command->setAttribute(Command::CA_Hide);
    mfile->addAction(command, ProjectExplorer::Constants::G_FILE_OTHER);
    connect(d->m_buildFileContextMenu, &QAction::triggered,
            &d->qmakeProjectManager, &QmakeManager::buildFileContextMenu);

    d->m_buildSubProjectAction = new Utils::ParameterAction(tr("Build Subproject"), tr("Build Subproject \"%1\""),
                                                         Utils::ParameterAction::AlwaysEnabled, this);
    command = ActionManager::registerAction(d->m_buildSubProjectAction, Constants::BUILDSUBDIR, projectContext);
    command->setAttribute(Command::CA_Hide);
    command->setAttribute(Command::CA_UpdateText);
    command->setDescription(d->m_buildSubProjectAction->text());
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_BUILD);
    connect(d->m_buildSubProjectAction, &QAction::triggered,
            &d->qmakeProjectManager, &QmakeManager::buildSubDirContextMenu);

    d->m_runQMakeAction = new QAction(tr("Run qmake"), this);
    const Context globalcontext(Core::Constants::C_GLOBAL);
    command = ActionManager::registerAction(d->m_runQMakeAction, Constants::RUNQMAKE, globalcontext);
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_BUILD);
    connect(d->m_runQMakeAction, &QAction::triggered, &d->qmakeProjectManager, &QmakeManager::runQMake);

    d->m_rebuildSubProjectAction = new Utils::ParameterAction(tr("Rebuild Subproject"), tr("Rebuild Subproject \"%1\""),
                                                           Utils::ParameterAction::AlwaysEnabled, this);
    command = ActionManager::registerAction(d->m_rebuildSubProjectAction, Constants::REBUILDSUBDIR, projectContext);
    command->setAttribute(Command::CA_Hide);
    command->setAttribute(Command::CA_UpdateText);
    command->setDescription(d->m_rebuildSubProjectAction->text());
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_REBUILD);
    connect(d->m_rebuildSubProjectAction, &QAction::triggered,
            &d->qmakeProjectManager, &QmakeManager::rebuildSubDirContextMenu);

    d->m_cleanSubProjectAction = new Utils::ParameterAction(tr("Clean Subproject"), tr("Clean Subproject \"%1\""),
                                                         Utils::ParameterAction::AlwaysEnabled, this);
    command = ActionManager::registerAction(d->m_cleanSubProjectAction, Constants::CLEANSUBDIR, projectContext);
    command->setAttribute(Command::CA_Hide);
    command->setAttribute(Command::CA_UpdateText);
    command->setDescription(d->m_cleanSubProjectAction->text());
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_CLEAN);
    connect(d->m_cleanSubProjectAction, &QAction::triggered,
            &d->qmakeProjectManager, &QmakeManager::cleanSubDirContextMenu);

    d->m_buildFileAction = new Utils::ParameterAction(tr("Build File"), tr("Build File \"%1\""),
                                                   Utils::ParameterAction::AlwaysEnabled, this);
    command = ActionManager::registerAction(d->m_buildFileAction, Constants::BUILDFILE, projectContext);
    command->setAttribute(Command::CA_Hide);
    command->setAttribute(Command::CA_UpdateText);
    command->setDescription(d->m_buildFileAction->text());
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl+Alt+B")));
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_BUILD);
    connect(d->m_buildFileAction, &QAction::triggered, &d->qmakeProjectManager, &QmakeManager::buildFile);

    connect(BuildManager::instance(), &BuildManager::buildStateChanged,
            d, &QmakeProjectManagerPluginPrivate::buildStateChanged);
    connect(SessionManager::instance(), &SessionManager::startupProjectChanged,
            d, &QmakeProjectManagerPluginPrivate::projectChanged);
    connect(ProjectTree::instance(), &ProjectTree::currentProjectChanged,
            d, &QmakeProjectManagerPluginPrivate::projectChanged);

    connect(ProjectTree::instance(), &ProjectTree::currentNodeChanged,
            d, &QmakeProjectManagerPluginPrivate::updateContextActions);

    ActionContainer *contextMenu = ActionManager::createMenu(QmakeProjectManager::Constants::M_CONTEXT);

    Context proFileEditorContext = Context(QmakeProjectManager::Constants::PROFILE_EDITOR_ID);

    command = ActionManager::command(TextEditor::Constants::JUMP_TO_FILE_UNDER_CURSOR);
    contextMenu->addAction(command);

    d->m_addLibraryAction = new QAction(tr("Add Library..."), this);
    command = ActionManager::registerAction(d->m_addLibraryAction,
        Constants::ADDLIBRARY, proFileEditorContext);
    connect(d->m_addLibraryAction, &QAction::triggered, &d->qmakeProjectManager, &QmakeManager::addLibrary);
    contextMenu->addAction(command);

    d->m_addLibraryActionContextMenu = new QAction(tr("Add Library..."), this);
    command = ActionManager::registerAction(d->m_addLibraryActionContextMenu,
        Constants::ADDLIBRARY, projecTreeContext);
    connect(d->m_addLibraryActionContextMenu, &QAction::triggered,
            &d->qmakeProjectManager, &QmakeManager::addLibraryContextMenu);
    mproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_FILES);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_FILES);

    contextMenu->addSeparator(proFileEditorContext);

    command = ActionManager::command(TextEditor::Constants::UN_COMMENT_SELECTION);
    contextMenu->addAction(command);

    connect(EditorManager::instance(), &EditorManager::currentEditorChanged,
            d, &QmakeProjectManagerPluginPrivate::updateBuildFileAction);

    d->updateActions();

    return true;
}

QmakeProjectManagerPluginPrivate::~QmakeProjectManagerPluginPrivate()
{
    delete m_designerEditor;
    delete m_linguistEditor;
}

void QmakeProjectManagerPluginPrivate::projectChanged()
{
    if (m_previousStartupProject)
        disconnect(m_previousStartupProject, &Project::activeTargetChanged,
                   this, &QmakeProjectManagerPluginPrivate::activeTargetChanged);

    if (ProjectTree::currentProject())
        m_previousStartupProject = qobject_cast<QmakeProject *>(ProjectTree::currentProject());
    else
        m_previousStartupProject = qobject_cast<QmakeProject *>(SessionManager::startupProject());

    if (m_previousStartupProject) {
        connect(m_previousStartupProject, &Project::activeTargetChanged,
                this, &QmakeProjectManagerPluginPrivate::activeTargetChanged);
        connect(m_previousStartupProject, &Project::parsingFinished,
                this, &QmakeProjectManagerPluginPrivate::updateActions);
    }

    activeTargetChanged();
}

void QmakeProjectManagerPluginPrivate::activeTargetChanged()
{
    if (m_previousTarget)
        disconnect(m_previousTarget, &Target::activeBuildConfigurationChanged,
                   this, &QmakeProjectManagerPluginPrivate::updateRunQMakeAction);

    m_previousTarget = m_previousStartupProject ? m_previousStartupProject->activeTarget() : nullptr;

    if (m_previousTarget)
        connect(m_previousTarget, &Target::activeBuildConfigurationChanged,
                this, &QmakeProjectManagerPluginPrivate::updateRunQMakeAction);

    updateRunQMakeAction();
}

void QmakeProjectManagerPluginPrivate::updateActions()
{
    updateRunQMakeAction();
    updateContextActions();
}

void QmakeProjectManagerPluginPrivate::updateRunQMakeAction()
{
    bool enable = true;
    if (BuildManager::isBuilding(m_previousStartupProject))
        enable = false;
    auto pro = qobject_cast<QmakeProject *>(m_previousStartupProject);
    m_runQMakeAction->setVisible(pro);
    if (!pro
            || !pro->rootProjectNode()
            || !pro->activeTarget()
            || !pro->activeTarget()->activeBuildConfiguration())
        enable = false;

    m_runQMakeAction->setEnabled(enable);
}

void QmakeProjectManagerPluginPrivate::updateContextActions()
{
    const Node *node = ProjectTree::findCurrentNode();
    Project *project = ProjectTree::currentProject();

    const ContainerNode *containerNode = node ? node->asContainerNode() : nullptr;
    const auto *proFileNode = dynamic_cast<const QmakeProFileNode *>(containerNode ? containerNode->rootProjectNode() : node);

    m_addLibraryActionContextMenu->setEnabled(proFileNode);
    auto *qmakeProject = qobject_cast<QmakeProject *>(QmakeManager::contextProject());
    QmakeProFileNode *subProjectNode = nullptr;
    disableBuildFileMenus();
    if (node) {
        auto subPriFileNode = dynamic_cast<const QmakePriFileNode *>(node);
        if (!subPriFileNode)
            subPriFileNode = dynamic_cast<QmakePriFileNode *>(node->parentProjectNode());
        subProjectNode = subPriFileNode ? subPriFileNode->proFileNode() : nullptr;

        if (const FileNode *fileNode = node->asFileNode())
            enableBuildFileMenus(fileNode->filePath());
    }

    bool subProjectActionsVisible = false;
    if (qmakeProject && subProjectNode) {
        if (ProjectNode *rootNode = qmakeProject->rootProjectNode())
            subProjectActionsVisible = subProjectNode != rootNode;
    }

    QString subProjectName;
    if (subProjectActionsVisible)
        subProjectName = subProjectNode->displayName();

    m_buildSubProjectAction->setParameter(subProjectName);
    m_rebuildSubProjectAction->setParameter(subProjectName);
    m_cleanSubProjectAction->setParameter(subProjectName);
    m_buildSubProjectContextMenu->setParameter(proFileNode ? proFileNode->displayName() : QString());

    auto buildConfiguration = (qmakeProject && qmakeProject->activeTarget()) ?
                static_cast<QmakeBuildConfiguration *>(qmakeProject->activeTarget()->activeBuildConfiguration()) : nullptr;
    bool isProjectNode = qmakeProject && proFileNode && buildConfiguration;
    bool isBuilding = BuildManager::isBuilding(project);
    bool enabled = subProjectActionsVisible && !isBuilding;

    m_buildSubProjectAction->setVisible(subProjectActionsVisible);
    m_rebuildSubProjectAction->setVisible(subProjectActionsVisible);
    m_cleanSubProjectAction->setVisible(subProjectActionsVisible);
    m_buildSubProjectContextMenu->setVisible(subProjectActionsVisible && isProjectNode);
    m_subProjectRebuildSeparator->setVisible(subProjectActionsVisible && isProjectNode);
    m_rebuildSubProjectContextMenu->setVisible(subProjectActionsVisible && isProjectNode);
    m_cleanSubProjectContextMenu->setVisible(subProjectActionsVisible && isProjectNode);

    m_buildSubProjectAction->setEnabled(enabled);
    m_rebuildSubProjectAction->setEnabled(enabled);
    m_cleanSubProjectAction->setEnabled(enabled);
    m_buildSubProjectContextMenu->setEnabled(enabled && isProjectNode);
    m_rebuildSubProjectContextMenu->setEnabled(enabled && isProjectNode);
    m_cleanSubProjectContextMenu->setEnabled(enabled && isProjectNode);
    m_runQMakeActionContextMenu->setEnabled(isProjectNode && !isBuilding
                                            && buildConfiguration->qmakeStep());
}

void QmakeProjectManagerPluginPrivate::buildStateChanged(Project *pro)
{
    if (pro == ProjectTree::currentProject()) {
        updateRunQMakeAction();
        updateContextActions();
        updateBuildFileAction();
    }
}

void QmakeProjectManagerPluginPrivate::updateBuildFileAction()
{
    disableBuildFileMenus();
    if (IDocument *currentDocument = EditorManager::currentDocument())
        enableBuildFileMenus(currentDocument->filePath());
}

void QmakeProjectManagerPluginPrivate::disableBuildFileMenus()
{
    m_buildFileAction->setVisible(false);
    m_buildFileAction->setEnabled(false);
    m_buildFileAction->setParameter(QString());
    m_buildFileContextMenu->setEnabled(false);
}

void QmakeProjectManagerPluginPrivate::enableBuildFileMenus(const Utils::FileName &file)
{
    bool visible = false;
    bool enabled = false;

    if (Node *node = ProjectTree::nodeForFile(file)) {
        if (Project *project = SessionManager::projectForFile(file)) {
            if (const FileNode *fileNode = node->asFileNode()) {
                const FileType type = fileNode->fileType();
                visible = qobject_cast<QmakeProject *>(project)
                        && dynamic_cast<QmakePriFileNode *>(node->parentProjectNode())
                        && (type == FileType::Source || type == FileType::Header);

                enabled = !BuildManager::isBuilding(project);
                m_buildFileAction->setParameter(file.fileName());
            }
        }
    }
    m_buildFileAction->setVisible(visible);
    m_buildFileAction->setEnabled(enabled);
    m_buildFileContextMenu->setEnabled(visible && enabled);
}

} // Internal
} // QmakeProjectManager
