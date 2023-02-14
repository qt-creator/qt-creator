// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projecttree.h"

#include "project.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"
#include "projectmanager.h"
#include "projectnodes.h"
#include "projecttreewidget.h"
#include "target.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/navigationwidget.h>
#include <coreplugin/vcsmanager.h>

#include <utils/algorithm.h>
#include <utils/infobar.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QFileInfo>
#include <QMenu>
#include <QTimer>

namespace {
const char EXTERNAL_OR_GENERATED_FILE_WARNING[] = "ExternalOrGeneratedFile";
}

using namespace Utils;

namespace ProjectExplorer {

using namespace Internal;

ProjectTree *ProjectTree::s_instance = nullptr;

ProjectTree::ProjectTree(QObject *parent) : QObject(parent)
{
    s_instance = this;

    connect(Core::EditorManager::instance(), &Core::EditorManager::currentEditorChanged,
            this, &ProjectTree::update);

    connect(qApp, &QApplication::focusChanged,
            this, &ProjectTree::update);

    connect(ProjectManager::instance(), &ProjectManager::projectAdded,
            this, &ProjectTree::sessionAndTreeChanged);
    connect(ProjectManager::instance(), &ProjectManager::projectRemoved,
            this, &ProjectTree::sessionAndTreeChanged);
    connect(ProjectManager::instance(), &ProjectManager::startupProjectChanged,
            this, &ProjectTree::sessionChanged);
    connect(this, &ProjectTree::subtreeChanged, this, &ProjectTree::treeChanged);
}

ProjectTree::~ProjectTree()
{
    QTC_ASSERT(s_instance == this, return);
    s_instance = nullptr;
}

void ProjectTree::aboutToShutDown()
{
    disconnect(qApp, &QApplication::focusChanged,
               s_instance, &ProjectTree::update);
    s_instance->setCurrent(nullptr, nullptr);
    qDeleteAll(s_instance->m_projectTreeWidgets);
    QTC_CHECK(s_instance->m_projectTreeWidgets.isEmpty());
}

ProjectTree *ProjectTree::instance()
{
    return s_instance;
}

Project *ProjectTree::currentProject()
{
    return s_instance->m_currentProject;
}

Target *ProjectTree::currentTarget()
{
    Project *p = currentProject();
    return p ? p->activeTarget() : nullptr;
}

BuildSystem *ProjectTree::currentBuildSystem()
{
    Target *t = currentTarget();
    return t ? t->buildSystem() : nullptr;
}

Node *ProjectTree::currentNode()
{
    s_instance->update();
    return s_instance->m_currentNode;
}

FilePath ProjectTree::currentFilePath()
{
    Node *node = currentNode();
    return node ? node->filePath() : FilePath();
}

void ProjectTree::registerWidget(ProjectTreeWidget *widget)
{
    s_instance->m_projectTreeWidgets.append(widget);
    if (hasFocus(widget))
        s_instance->updateFromProjectTreeWidget(widget);
}

void ProjectTree::unregisterWidget(ProjectTreeWidget *widget)
{
    s_instance->m_projectTreeWidgets.removeOne(widget);
    if (hasFocus(widget))
        s_instance->updateFromDocumentManager();
}

void ProjectTree::nodeChanged(ProjectTreeWidget *widget)
{
    if (hasFocus(widget))
        s_instance->updateFromProjectTreeWidget(widget);
}

void ProjectTree::update()
{
    ProjectTreeWidget *focus = m_focusForContextMenu;
    if (!focus)
        focus = currentWidget();

    if (focus)
        updateFromProjectTreeWidget(focus);
    else
        updateFromDocumentManager();
}

void ProjectTree::updateFromProjectTreeWidget(ProjectTreeWidget *widget)
{
    Node *currentNode = widget->currentNode();
    Project *project = projectForNode(currentNode);

    if (!project)
        updateFromNode(nullptr); // Project was removed!
    else
        setCurrent(currentNode, project);
}

void ProjectTree::updateFromDocumentManager()
{
    if (Core::IDocument *document = Core::EditorManager::currentDocument()) {
        const FilePath fileName = document->filePath();
        updateFromNode(ProjectTreeWidget::nodeForFile(fileName));
    } else {
        updateFromNode(nullptr);
    }
}

void ProjectTree::updateFromNode(Node *node)
{
    Project *project;
    if (node)
        project = projectForNode(node);
    else
        project = ProjectManager::startupProject();

    setCurrent(node, project);
    for (ProjectTreeWidget *widget : std::as_const(m_projectTreeWidgets))
        widget->sync(node);
}

void ProjectTree::setCurrent(Node *node, Project *project)
{
    const bool changedProject = project != m_currentProject;
    if (changedProject) {
        if (m_currentProject) {
            disconnect(m_currentProject, &Project::projectLanguagesUpdated,
                       this, &ProjectTree::updateContext);
        }

        m_currentProject = project;

        if (m_currentProject) {
            connect(m_currentProject, &Project::projectLanguagesUpdated,
                    this, &ProjectTree::updateContext);
        }
    }

    if (Core::IDocument *document = Core::EditorManager::currentDocument()) {
        disconnect(document, &Core::IDocument::changed, this, nullptr);
        if (!node || node->isGenerated()) {
            const QString message = node
                    ? Tr::tr("<b>Warning:</b> This file is generated.")
                    : Tr::tr("<b>Warning:</b> This file is outside the project directory.");
            connect(document, &Core::IDocument::changed, this, [this, document, message] {
                updateFileWarning(document, message);
            });
        } else {
            document->infoBar()->removeInfo(EXTERNAL_OR_GENERATED_FILE_WARNING);
        }
    }

    if (node != m_currentNode) {
        m_currentNode = node;
        emit currentNodeChanged(node);
    }

    if (changedProject) {
        emit currentProjectChanged(m_currentProject);
        sessionChanged();
        updateContext();
    }
}

void ProjectTree::sessionChanged()
{
    if (m_currentProject) {
        Core::DocumentManager::setDefaultLocationForNewFiles(m_currentProject->projectDirectory());
    } else if (Project *project = ProjectManager::startupProject()) {
        Core::DocumentManager::setDefaultLocationForNewFiles(project->projectDirectory());
        updateFromNode(nullptr); // Make startup project current if there is no other current
    } else {
        Core::DocumentManager::setDefaultLocationForNewFiles({});
    }
    update();
}

void ProjectTree::updateContext()
{
    Core::Context oldContext;
    oldContext.add(m_lastProjectContext);

    Core::Context newContext;
    if (m_currentProject) {
        newContext.add(m_currentProject->projectContext());
        newContext.add(m_currentProject->projectLanguages());

        m_lastProjectContext = newContext;
    } else {
        m_lastProjectContext = Core::Context();
    }

    Core::ICore::updateAdditionalContexts(oldContext, newContext);
}

void ProjectTree::emitSubtreeChanged(FolderNode *node)
{
    if (hasNode(node))
        emit s_instance->subtreeChanged(node);
}

void ProjectTree::sessionAndTreeChanged()
{
    sessionChanged();
    emit treeChanged();
}

void ProjectTree::expandCurrentNodeRecursively()
{
    if (const auto w = currentWidget())
        w->expandCurrentNodeRecursively();
}

void ProjectTree::collapseAll()
{
    if (const auto w = currentWidget())
        w->collapseAll();
}

void ProjectTree::expandAll()
{
    if (const auto w = currentWidget())
        w->expandAll();
}

void ProjectTree::changeProjectRootDirectory()
{
    if (m_currentProject)
        m_currentProject->changeRootProjectDirectory();
}

void ProjectTree::updateFileWarning(Core::IDocument *document, const QString &text)
{
    if (document->filePath().isEmpty())
        return;
    Utils::InfoBar *infoBar = document->infoBar();
    Utils::Id infoId(EXTERNAL_OR_GENERATED_FILE_WARNING);
    if (!document->isModified()) {
        infoBar->removeInfo(infoId);
        return;
    }
    if (!infoBar->canInfoBeAdded(infoId))
        return;
    const FilePath filePath = document->filePath();
    const QList<Project *> projects = ProjectManager::projects();
    if (projects.isEmpty())
        return;
    for (Project *project : projects) {
        FilePath projectDir = project->projectDirectory();
        if (projectDir.isEmpty())
            continue;
        if (filePath.isChildOf(projectDir))
            return;
        if (filePath.canonicalPath().isChildOf(projectDir.canonicalPath()))
            return;
        // External file. Test if it under the same VCS
        FilePath topLevel;
        if (Core::VcsManager::findVersionControlForDirectory(projectDir, &topLevel)
                && filePath.isChildOf(topLevel)) {
            return;
        }
    }
    infoBar->addInfo(
        Utils::InfoBarEntry(infoId, text, Utils::InfoBarEntry::GlobalSuppression::Enabled));
}

bool ProjectTree::hasFocus(ProjectTreeWidget *widget)
{
    return widget && ((widget->focusWidget() && widget->focusWidget()->hasFocus())
                      || s_instance->m_focusForContextMenu == widget);
}

ProjectTreeWidget *ProjectTree::currentWidget() const
{
    return findOrDefault(m_projectTreeWidgets, &ProjectTree::hasFocus);
}

void ProjectTree::showContextMenu(ProjectTreeWidget *focus, const QPoint &globalPos, Node *node)
{
    QMenu *contextMenu = nullptr;
    emit s_instance->aboutToShowContextMenu(node);

    if (!node) {
        contextMenu = Core::ActionManager::actionContainer(Constants::M_SESSIONCONTEXT)->menu();
    } else  if (node->isProjectNodeType()) {
        if ((node->parentFolderNode() && node->parentFolderNode()->asContainerNode())
                || node->asContainerNode())
            contextMenu = Core::ActionManager::actionContainer(Constants::M_PROJECTCONTEXT)->menu();
        else
            contextMenu = Core::ActionManager::actionContainer(Constants::M_SUBPROJECTCONTEXT)->menu();
    } else if (node->isVirtualFolderType() || node->isFolderNodeType()) {
        contextMenu = Core::ActionManager::actionContainer(Constants::M_FOLDERCONTEXT)->menu();
    } else if (node->asFileNode()) {
        contextMenu = Core::ActionManager::actionContainer(Constants::M_FILECONTEXT)->menu();
    }

    if (contextMenu && !contextMenu->actions().isEmpty()) {
        s_instance->m_focusForContextMenu = focus;
        contextMenu->popup(globalPos);
        connect(contextMenu, &QMenu::aboutToHide,
                s_instance, &ProjectTree::hideContextMenu,
                Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));
    }
}

void ProjectTree::highlightProject(Project *project, const QString &message)
{
    Core::ModeManager::activateMode(Core::Constants::MODE_EDIT);

    // Shows and focusses a project tree
    QWidget *widget = Core::NavigationWidget::activateSubWidget(ProjectExplorer::Constants::PROJECTTREE_ID, Core::Side::Left);

    if (auto *projectTreeWidget = qobject_cast<ProjectTreeWidget *>(widget))
        projectTreeWidget->showMessage(project->rootProjectNode(), message);
}

/*!
    Registers the function \a treeChange to be run on a (sub tree of the)
    project tree when it is created. The function must be thread-safe, and
    applying the function on the same tree a second time must be a no-op.
*/
void ProjectTree::registerTreeManager(const TreeManagerFunction &treeChange)
{
    if (treeChange)
        s_instance->m_treeManagers.append(treeChange);
}

void ProjectTree::applyTreeManager(FolderNode *folder, ConstructionPhase phase)
{
    if (!folder)
        return;

    for (TreeManagerFunction &f : s_instance->m_treeManagers)
        f(folder, phase);
}

bool ProjectTree::hasNode(const Node *node)
{
    return Utils::contains(ProjectManager::projects(), [node](const Project *p) {
        if (!p)
            return false;
        if (p->containerNode() == node)
            return true;
        // When parsing fails we have a living container node but no rootProjectNode.
        ProjectNode *pn = p->rootProjectNode();
        if (!pn)
            return false;
        return pn->findNode([node](const Node *n) { return n == node; }) != nullptr;
    });
}

void ProjectTree::forEachNode(const std::function<void(Node *)> &task)
{
    const QList<Project *> projects = ProjectManager::projects();
    for (Project *project : projects) {
        if (ProjectNode *projectNode = project->rootProjectNode()) {
            task(projectNode);
            projectNode->forEachGenericNode(task);
        }
    }
}

Project *ProjectTree::projectForNode(const Node *node)
{
    if (!node)
        return nullptr;

    const FolderNode *folder = node->asFolderNode();
    if (!folder)
        folder = node->parentFolderNode();

    while (folder && folder->parentFolderNode())
        folder = folder->parentFolderNode();

    return Utils::findOrDefault(ProjectManager::projects(), [folder](const Project *pro) {
        return pro->containerNode() == folder;
    });
}

Node *ProjectTree::nodeForFile(const FilePath &fileName)
{
    Node *node = nullptr;
    for (const Project *project : ProjectManager::projects()) {
        project->nodeForFilePath(fileName, [&](const Node *n) {
            if (!node || (!node->asFileNode() && n->asFileNode()))
                node = const_cast<Node *>(n);
            return false;
        });
        // early return:
        if (node && node->asFileNode())
            return node;
    }
    return node;
}

const QList<Node *> ProjectTree::siblingsWithSameBaseName(const Node *fileNode)
{
    ProjectNode *productNode = fileNode->parentProjectNode();
    while (productNode && !productNode->isProduct())
        productNode = productNode->parentProjectNode();
    if (!productNode)
        return {};
    const QFileInfo fi = fileNode->filePath().toFileInfo();
    const auto filter = [&fi](const Node *n) {
        return n->asFileNode()
                && n->filePath().toFileInfo().dir() == fi.dir()
                && n->filePath().completeBaseName() == fi.completeBaseName()
                && n->filePath().toString() != fi.filePath();
    };
    return productNode->findNodes(filter);
}

void ProjectTree::hideContextMenu()
{
    if (m_keepCurrentNodeRequests == 0)
        m_focusForContextMenu = nullptr;
}

ProjectTree::CurrentNodeKeeper::CurrentNodeKeeper()
    : m_active(ProjectTree::instance()->m_focusForContextMenu)
{
    if (m_active)
        ++ProjectTree::instance()->m_keepCurrentNodeRequests;
}

ProjectTree::CurrentNodeKeeper::~CurrentNodeKeeper()
{
    if (m_active && --ProjectTree::instance()->m_keepCurrentNodeRequests == 0) {
        ProjectTree::instance()->m_focusForContextMenu = nullptr;
        ProjectTree::instance()->update();
    }
}

} // namespace ProjectExplorer
