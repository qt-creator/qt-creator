// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmakeprojectmanagerplugin.h"

#include "addlibrarywizard.h"
#include "customwidgetwizard/customwidgetwizard.h"
#include "profileeditor.h"
#include "qmakebuildconfiguration.h"
#include "qmakemakestep.h"
#include "qmakenodes.h"
#include "qmakeproject.h"
#include "qmakeprojectmanagerconstants.h"
#include "qmakeprojectmanagertr.h"
#include "qmakestep.h"
#include "wizards/subdirsprojectwizard.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorericons.h>

#include <texteditor/texteditor.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorconstants.h>

#include <utils/hostosinfo.h>
#include <utils/parameteraction.h>
#include <utils/utilsicons.h>

#ifdef WITH_TESTS
#    include <QTest>
#endif

using namespace Core;
using namespace ProjectExplorer;
using namespace TextEditor;
using namespace Utils;

namespace QmakeProjectManager {
namespace Internal {

class QmakeProjectManagerPluginPrivate : public QObject
{
public:
    void projectChanged();
    void activeTargetChanged();
    void updateActions();
    void updateRunQMakeAction();
    void updateContextActions(Node *node);
    void buildStateChanged(Project *pro);
    void updateBuildFileAction();
    void disableBuildFileMenus();
    void enableBuildFileMenus(const FilePath &file);

    Core::Context projectContext;

    CustomWizardMetaFactory<CustomQmakeProjectWizard>
        qmakeProjectWizard{"qmakeproject", IWizardFactory::ProjectWizard};

    QMakeStepFactory qmakeStepFactory;
    QmakeMakeStepFactory makeStepFactory;

    QmakeBuildConfigurationFactory buildConfigFactory;

    ProFileEditorFactory profileEditorFactory;

    QmakeProject *m_previousStartupProject = nullptr;
    Target *m_previousTarget = nullptr;

    QAction *m_runQMakeAction = nullptr;
    QAction *m_runQMakeActionContextMenu = nullptr;
    ParameterAction *m_buildSubProjectContextMenu = nullptr;
    QAction *m_subProjectRebuildSeparator = nullptr;
    QAction *m_rebuildSubProjectContextMenu = nullptr;
    QAction *m_cleanSubProjectContextMenu = nullptr;
    QAction *m_buildFileContextMenu = nullptr;
    ParameterAction *m_buildSubProjectAction = nullptr;
    QAction *m_rebuildSubProjectAction = nullptr;
    QAction *m_cleanSubProjectAction = nullptr;
    ParameterAction *m_buildFileAction = nullptr;
    QAction *m_addLibraryAction = nullptr;
    QAction *m_addLibraryActionContextMenu = nullptr;

    void addLibrary();
    void addLibraryContextMenu();
    void runQMake();
    void runQMakeContextMenu();

    void buildSubDirContextMenu() { handleSubDirContextMenu(QmakeBuildSystem::BUILD, false); }
    void rebuildSubDirContextMenu() { handleSubDirContextMenu(QmakeBuildSystem::REBUILD, false); }
    void cleanSubDirContextMenu() { handleSubDirContextMenu(QmakeBuildSystem::CLEAN, false); }
    void buildFileContextMenu() { handleSubDirContextMenu(QmakeBuildSystem::BUILD, true); }
    void buildFile();

    void handleSubDirContextMenu(QmakeBuildSystem::Action action, bool isFileBuild);
    void addLibraryImpl(const FilePath &filePath, TextEditor::BaseTextEditor *editor);
    void runQMakeImpl(Project *p, ProjectExplorer::Node *node);
};

QmakeProjectManagerPlugin::~QmakeProjectManagerPlugin()
{
    delete d;
}

void QmakeProjectManagerPlugin::initialize()
{
    const Context projectContext(QmakeProjectManager::Constants::QMAKEPROJECT_ID);
    Context projectTreeContext(ProjectExplorer::Constants::C_PROJECT_TREE);

    d = new QmakeProjectManagerPluginPrivate;

    //create and register objects
    ProjectManager::registerProjectType<QmakeProject>(QmakeProjectManager::Constants::PROFILE_MIMETYPE);

    IWizardFactory::registerFactoryCreator([] { return new SubdirsProjectWizard; });
    IWizardFactory::registerFactoryCreator([] { return new CustomWidgetWizard; });

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

    d->m_buildSubProjectContextMenu = new ParameterAction(Tr::tr("Build"), Tr::tr("Build \"%1\""),
                                                              ParameterAction::AlwaysEnabled/*handled manually*/,
                                                              this);
    command = ActionManager::registerAction(d->m_buildSubProjectContextMenu, Constants::BUILDSUBDIRCONTEXTMENU, projectContext);
    command->setAttribute(Command::CA_Hide);
    command->setAttribute(Command::CA_UpdateText);
    command->setDescription(d->m_buildSubProjectContextMenu->text());
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(d->m_buildSubProjectContextMenu, &QAction::triggered,
            d, &QmakeProjectManagerPluginPrivate::buildSubDirContextMenu);

    d->m_runQMakeActionContextMenu = new QAction(Tr::tr("Run qmake"), this);
    command = ActionManager::registerAction(d->m_runQMakeActionContextMenu, Constants::RUNQMAKECONTEXTMENU, projectContext);
    command->setAttribute(Command::CA_Hide);
    mproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(d->m_runQMakeActionContextMenu, &QAction::triggered,
            d, &QmakeProjectManagerPluginPrivate::runQMakeContextMenu);

    command = msubproject->addSeparator(projectContext, ProjectExplorer::Constants::G_PROJECT_BUILD,
                                        &d->m_subProjectRebuildSeparator);
    command->setAttribute(Command::CA_Hide);

    d->m_rebuildSubProjectContextMenu = new QAction(Tr::tr("Rebuild"), this);
    command = ActionManager::registerAction(
                d->m_rebuildSubProjectContextMenu, Constants::REBUILDSUBDIRCONTEXTMENU, projectContext);
    command->setAttribute(Command::CA_Hide);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(d->m_rebuildSubProjectContextMenu, &QAction::triggered,
            d, &QmakeProjectManagerPluginPrivate::rebuildSubDirContextMenu);

    d->m_cleanSubProjectContextMenu = new QAction(Tr::tr("Clean"), this);
    command = ActionManager::registerAction(
                d->m_cleanSubProjectContextMenu, Constants::CLEANSUBDIRCONTEXTMENU, projectContext);
    command->setAttribute(Command::CA_Hide);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(d->m_cleanSubProjectContextMenu, &QAction::triggered,
            d, &QmakeProjectManagerPluginPrivate::cleanSubDirContextMenu);

    d->m_buildFileContextMenu = new QAction(Tr::tr("Build"), this);
    command = ActionManager::registerAction(d->m_buildFileContextMenu, Constants::BUILDFILECONTEXTMENU, projectContext);
    command->setAttribute(Command::CA_Hide);
    mfile->addAction(command, ProjectExplorer::Constants::G_FILE_OTHER);
    connect(d->m_buildFileContextMenu, &QAction::triggered,
            d, &QmakeProjectManagerPluginPrivate::buildFileContextMenu);

    d->m_buildSubProjectAction = new ParameterAction(Tr::tr("Build &Subproject"), Tr::tr("Build &Subproject \"%1\""),
                                                         ParameterAction::AlwaysEnabled, this);
    command = ActionManager::registerAction(d->m_buildSubProjectAction, Constants::BUILDSUBDIR, projectContext);
    command->setAttribute(Command::CA_Hide);
    command->setAttribute(Command::CA_UpdateText);
    command->setDescription(d->m_buildSubProjectAction->text());
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_SUBPROJECT);
    connect(d->m_buildSubProjectAction, &QAction::triggered,
            d, &QmakeProjectManagerPluginPrivate::buildSubDirContextMenu);

    d->m_runQMakeAction = new QAction(Tr::tr("Run qmake"), this);
    const Context globalcontext(Core::Constants::C_GLOBAL);
    command = ActionManager::registerAction(d->m_runQMakeAction, Constants::RUNQMAKE, globalcontext);
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_BUILD);
    connect(d->m_runQMakeAction, &QAction::triggered,
            d, &QmakeProjectManagerPluginPrivate::runQMake);

    d->m_rebuildSubProjectAction = new QAction(ProjectExplorer::Icons::REBUILD.icon(), Tr::tr("Rebuild"), this);
    d->m_rebuildSubProjectAction->setWhatsThis(Tr::tr("Rebuild Subproject"));
    command = ActionManager::registerAction(d->m_rebuildSubProjectAction, Constants::REBUILDSUBDIR, projectContext);
    command->setAttribute(Command::CA_Hide);
    command->setAttribute(Command::CA_UpdateText);
    command->setDescription(d->m_rebuildSubProjectAction->whatsThis());
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_SUBPROJECT);
    connect(d->m_rebuildSubProjectAction, &QAction::triggered,
            d, &QmakeProjectManagerPluginPrivate::rebuildSubDirContextMenu);

    d->m_cleanSubProjectAction = new QAction(Utils::Icons::CLEAN.icon(),Tr::tr("Clean"), this);
    d->m_cleanSubProjectAction->setWhatsThis(Tr::tr("Clean Subproject"));
    command = ActionManager::registerAction(d->m_cleanSubProjectAction, Constants::CLEANSUBDIR, projectContext);
    command->setAttribute(Command::CA_Hide);
    command->setAttribute(Command::CA_UpdateText);
    command->setDescription(d->m_cleanSubProjectAction->whatsThis());
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_SUBPROJECT);
    connect(d->m_cleanSubProjectAction, &QAction::triggered,
            d, &QmakeProjectManagerPluginPrivate::cleanSubDirContextMenu);

    d->m_buildFileAction = new ParameterAction(Tr::tr("Build File"), Tr::tr("Build File \"%1\""),
                                                   ParameterAction::AlwaysEnabled, this);
    command = ActionManager::registerAction(d->m_buildFileAction, Constants::BUILDFILE, projectContext);
    command->setAttribute(Command::CA_Hide);
    command->setAttribute(Command::CA_UpdateText);
    command->setDescription(d->m_buildFileAction->text());
    command->setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+Alt+B")));
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_FILE);
    connect(d->m_buildFileAction, &QAction::triggered,
            d, &QmakeProjectManagerPluginPrivate::buildFile);

    connect(BuildManager::instance(), &BuildManager::buildStateChanged,
            d, &QmakeProjectManagerPluginPrivate::buildStateChanged);
    connect(ProjectManager::instance(), &ProjectManager::startupProjectChanged,
            d, &QmakeProjectManagerPluginPrivate::projectChanged);
    connect(ProjectTree::instance(), &ProjectTree::currentProjectChanged,
            d, &QmakeProjectManagerPluginPrivate::projectChanged);

    connect(ProjectTree::instance(), &ProjectTree::currentNodeChanged,
            d, &QmakeProjectManagerPluginPrivate::updateContextActions);

    ActionContainer *contextMenu = ActionManager::createMenu(QmakeProjectManager::Constants::M_CONTEXT);

    Context proFileEditorContext = Context(QmakeProjectManager::Constants::PROFILE_EDITOR_ID);

    command = ActionManager::command(TextEditor::Constants::JUMP_TO_FILE_UNDER_CURSOR);
    contextMenu->addAction(command);

    d->m_addLibraryAction = new QAction(Tr::tr("Add Library..."), this);
    command = ActionManager::registerAction(d->m_addLibraryAction,
        Constants::ADDLIBRARY, proFileEditorContext);
    connect(d->m_addLibraryAction, &QAction::triggered,
            d, &QmakeProjectManagerPluginPrivate::addLibrary);
    contextMenu->addAction(command);

    d->m_addLibraryActionContextMenu = new QAction(Tr::tr("Add Library..."), this);
    command = ActionManager::registerAction(d->m_addLibraryActionContextMenu,
        Constants::ADDLIBRARY, projectTreeContext);
    connect(d->m_addLibraryActionContextMenu, &QAction::triggered,
            d, &QmakeProjectManagerPluginPrivate::addLibraryContextMenu);
    mproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_FILES);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_FILES);

    contextMenu->addSeparator(proFileEditorContext);

    command = ActionManager::command(TextEditor::Constants::UN_COMMENT_SELECTION);
    contextMenu->addAction(command);

    connect(EditorManager::instance(), &EditorManager::currentEditorChanged,
            d, &QmakeProjectManagerPluginPrivate::updateBuildFileAction);

    d->updateActions();
}

void QmakeProjectManagerPluginPrivate::projectChanged()
{
    if (m_previousStartupProject)
        disconnect(m_previousStartupProject, &Project::activeTargetChanged,
                   this, &QmakeProjectManagerPluginPrivate::activeTargetChanged);

    if (ProjectTree::currentProject())
        m_previousStartupProject = qobject_cast<QmakeProject *>(ProjectTree::currentProject());
    else
        m_previousStartupProject = qobject_cast<QmakeProject *>(ProjectManager::startupProject());

    if (m_previousStartupProject) {
        connect(m_previousStartupProject, &Project::activeTargetChanged,
                this, &QmakeProjectManagerPluginPrivate::activeTargetChanged);
    }

    activeTargetChanged();
}

static QmakeProFileNode *buildableFileProFile(Node *node)
{
    if (node) {
        auto subPriFileNode = dynamic_cast<QmakePriFileNode *>(node);
        if (!subPriFileNode)
            subPriFileNode = dynamic_cast<QmakePriFileNode *>(node->parentProjectNode());
        if (subPriFileNode)
            return subPriFileNode->proFileNode();
    }
    return nullptr;
}

void QmakeProjectManagerPluginPrivate::addLibrary()
{
    if (auto editor = qobject_cast<BaseTextEditor *>(Core::EditorManager::currentEditor()))
        addLibraryImpl(editor->document()->filePath(), editor);
}

void QmakeProjectManagerPluginPrivate::addLibraryContextMenu()
{
    FilePath projectPath;

    Node *node = ProjectTree::currentNode();
    if (ContainerNode *cn = node->asContainerNode())
        projectPath = cn->project()->projectFilePath();
    else if (dynamic_cast<QmakeProFileNode *>(node))
        projectPath = node->filePath();

    addLibraryImpl(projectPath, nullptr);
}

void QmakeProjectManagerPluginPrivate::addLibraryImpl(const FilePath &filePath, BaseTextEditor *editor)
{
    if (filePath.isEmpty())
        return;

    Internal::AddLibraryWizard wizard(filePath, Core::ICore::dialogParent());
    if (wizard.exec() != QDialog::Accepted)
        return;

    if (!editor)
        editor = qobject_cast<BaseTextEditor *>(Core::EditorManager::openEditor(filePath,
            Constants::PROFILE_EDITOR_ID, Core::EditorManager::DoNotMakeVisible));
    if (!editor)
        return;

    const int endOfDoc = editor->position(EndOfDocPosition);
    editor->setCursorPosition(endOfDoc);
    QString snippet = wizard.snippet();

    // add extra \n in case the last line is not empty
    int line, column;
    editor->convertPosition(endOfDoc, &line, &column);
    if (!editor->textAt(endOfDoc - column, column).simplified().isEmpty())
        snippet = QLatin1Char('\n') + snippet;

    editor->insert(snippet);
}

void QmakeProjectManagerPluginPrivate::runQMake()
{
    runQMakeImpl(ProjectManager::startupProject(), nullptr);
}

void QmakeProjectManagerPluginPrivate::runQMakeContextMenu()
{
    runQMakeImpl(ProjectTree::currentProject(), ProjectTree::currentNode());
}

void QmakeProjectManagerPluginPrivate::runQMakeImpl(Project *p, Node *node)
{
    if (!ProjectExplorerPlugin::saveModifiedFiles())
        return;
    auto *qmakeProject = qobject_cast<QmakeProject *>(p);
    QTC_ASSERT(qmakeProject, return);

    if (!qmakeProject->activeTarget() || !qmakeProject->activeTarget()->activeBuildConfiguration())
        return;

    auto *bc = static_cast<QmakeBuildConfiguration *>(qmakeProject->activeTarget()->activeBuildConfiguration());
    QMakeStep *qs = bc->qmakeStep();
    if (!qs)
        return;

    //found qmakeStep, now use it
    qs->setForced(true);

    if (node && node != qmakeProject->rootProjectNode())
        if (auto *profile = dynamic_cast<QmakeProFileNode *>(node))
            bc->setSubNodeBuild(profile);

    BuildManager::appendStep(qs, Tr::tr("QMake"));
    bc->setSubNodeBuild(nullptr);
}

void QmakeProjectManagerPluginPrivate::buildFile()
{
    Core::IDocument *currentDocument = Core::EditorManager::currentDocument();
    if (!currentDocument)
        return;

    const FilePath file = currentDocument->filePath();
    Node *n = ProjectTree::nodeForFile(file);
    FileNode *node  = n ? n->asFileNode() : nullptr;
    if (!node)
        return;
    Project *project = ProjectManager::projectForFile(file);
    if (!project)
        return;
    Target *target = project->activeTarget();
    if (!target)
        return;

    if (auto bs = qobject_cast<QmakeBuildSystem *>(target->buildSystem()))
        bs->buildHelper(QmakeBuildSystem::BUILD, true, buildableFileProFile(node), node);
}

void QmakeProjectManagerPluginPrivate::handleSubDirContextMenu(QmakeBuildSystem::Action action, bool isFileBuild)
{
    Node *node = ProjectTree::currentNode();

    QmakeProFileNode *subProjectNode = buildableFileProFile(node);
    FileNode *fileNode = node ? node->asFileNode() : nullptr;
    bool buildFilePossible = subProjectNode && fileNode && fileNode->fileType() == FileType::Source;
    FileNode *buildableFileNode = buildFilePossible ? fileNode : nullptr;

    if (auto bs = qobject_cast<QmakeBuildSystem *>(ProjectTree::currentBuildSystem()))
        bs->buildHelper(action, isFileBuild, subProjectNode, buildableFileNode);
}

void QmakeProjectManagerPluginPrivate::activeTargetChanged()
{
    if (m_previousTarget)
        disconnect(m_previousTarget, &Target::activeBuildConfigurationChanged,
                   this, &QmakeProjectManagerPluginPrivate::updateRunQMakeAction);

    m_previousTarget = m_previousStartupProject ? m_previousStartupProject->activeTarget() : nullptr;

    if (m_previousTarget) {
        connect(m_previousTarget, &Target::activeBuildConfigurationChanged,
                this, &QmakeProjectManagerPluginPrivate::updateRunQMakeAction);
        connect(m_previousTarget, &Target::parsingFinished,
                this, &QmakeProjectManagerPluginPrivate::updateActions);
    }

    updateRunQMakeAction();
}

void QmakeProjectManagerPluginPrivate::updateActions()
{
    updateRunQMakeAction();
    updateContextActions(ProjectTree::currentNode());
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

void QmakeProjectManagerPluginPrivate::updateContextActions(Node *node)
{
    Project *project = ProjectTree::currentProject();

    const ContainerNode *containerNode = node ? node->asContainerNode() : nullptr;
    const auto *proFileNode = dynamic_cast<const QmakeProFileNode *>(containerNode ? containerNode->rootProjectNode() : node);

    m_addLibraryActionContextMenu->setEnabled(proFileNode);
    auto *qmakeProject = qobject_cast<QmakeProject *>(project);
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
        updateContextActions(ProjectTree::currentNode());
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

void QmakeProjectManagerPluginPrivate::enableBuildFileMenus(const FilePath &file)
{
    bool visible = false;
    bool enabled = false;

    if (Node *node = ProjectTree::nodeForFile(file)) {
        if (Project *project = ProjectManager::projectForFile(file)) {
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
