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

#include "projecttree.h"
#include "projecttreewidget.h"
#include "session.h"
#include "project.h"
#include "projectnodes.h"
#include "projectexplorerconstants.h"
#include "nodesvisitor.h"

#include <utils/algorithm.h>
#include <coreplugin/icore.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/idocument.h>
#include <coreplugin/infobar.h>
#include <coreplugin/vcsmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/navigationwidget.h>
#include <coreplugin/modemanager.h>

#include <QApplication>
#include <QMenu>
#include <QTimer>

namespace {
const char EXTERNAL_FILE_WARNING[] = "ExternalFile";
}

using namespace Utils;

namespace ProjectExplorer {

using namespace Internal;

ProjectTree *ProjectTree::s_instance = 0;

ProjectTree::ProjectTree(QObject *parent)
    : QObject(parent),
      m_currentNode(0),
      m_currentProject(0),
      m_resetCurrentNodeFolder(false),
      m_resetCurrentNodeFile(false),
      m_resetCurrentNodeProject(false),
      m_focusForContextMenu(0)
{
    s_instance = this;

    connect(Core::EditorManager::instance(), &Core::EditorManager::currentEditorChanged,
            this, &ProjectTree::documentManagerCurrentFileChanged);

    connect(qApp, &QApplication::focusChanged,
            this, &ProjectTree::focusChanged);

    connect(SessionManager::instance(), &SessionManager::projectAdded,
            this, &ProjectTree::sessionChanged);
    connect(SessionManager::instance(), &SessionManager::projectRemoved,
            this, &ProjectTree::sessionChanged);
    connect(SessionManager::instance(), &SessionManager::startupProjectChanged,
            this, &ProjectTree::sessionChanged);
}

void ProjectTree::aboutToShutDown()
{
    disconnect(qApp, &QApplication::focusChanged,
               s_instance, &ProjectTree::focusChanged);
    s_instance->update(0, 0);
}

ProjectTree *ProjectTree::instance()
{
    return s_instance;
}

Project *ProjectTree::currentProject()
{
    return s_instance->m_currentProject;
}

Node *ProjectTree::currentNode()
{
    return s_instance->m_currentNode;
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

void ProjectTree::focusChanged()
{
    s_instance->updateFromFocus();
}

void ProjectTree::updateFromFocus(bool invalidCurrentNode)
{
    ProjectTreeWidget *focus = m_focusForContextMenu;
    if (!focus)
        focus = Utils::findOrDefault(m_projectTreeWidgets, &ProjectTree::hasFocus);

    if (focus)
        updateFromProjectTreeWidget(focus);
    else
        updateFromDocumentManager(invalidCurrentNode);
}

void ProjectTree::updateFromProjectTreeWidget(ProjectTreeWidget *widget)
{
    Node *currentNode = widget->currentNode();
    Project *project = projectForNode(currentNode);

    update(currentNode, project);
}

void ProjectTree::documentManagerCurrentFileChanged()
{
    updateFromFocus();
}

Project *ProjectTree::projectForNode(Node *node)
{
    if (!node)
        return 0;

    FolderNode *rootProjectNode = node->asFolderNode();
    if (!rootProjectNode)
        rootProjectNode = node->parentFolderNode();

    while (rootProjectNode && rootProjectNode->parentFolderNode() != SessionManager::sessionNode())
        rootProjectNode = rootProjectNode->parentFolderNode();

    Q_ASSERT(rootProjectNode);

    return Utils::findOrDefault(SessionManager::projects(), Utils::equal(&Project::rootProjectNode, rootProjectNode));
}

void ProjectTree::updateFromDocumentManager(bool invalidCurrentNode)
{
    Core::IDocument *document = Core::EditorManager::currentDocument();
    const FileName fileName = document ? document->filePath() : FileName();

    Node *currentNode = 0;
    if (!invalidCurrentNode && m_currentNode && m_currentNode->filePath() == fileName)
        currentNode = m_currentNode;
    else
        currentNode = ProjectTreeWidget::nodeForFile(fileName);

    updateFromNode(currentNode);
}

void ProjectTree::updateFromNode(Node *node)
{
    Project *project;
    if (node)
        project = projectForNode(node);
    else
        project = SessionManager::startupProject();

    update(node, project);
    foreach (ProjectTreeWidget *widget, m_projectTreeWidgets)
        widget->sync(node);
}

void ProjectTree::update(Node *node, Project *project)
{
    bool changedProject = project != m_currentProject;
    bool changedNode = node != m_currentNode;
    if (changedProject) {
        if (m_currentProject) {
            disconnect(m_currentProject, &Project::projectContextUpdated,
                       this, &ProjectTree::updateContext);
            disconnect(m_currentProject, &Project::projectLanguagesUpdated,
                       this, &ProjectTree::updateContext);
        }

        m_currentProject = project;

        if (m_currentProject) {
            connect(m_currentProject, &Project::projectContextUpdated,
                    this, &ProjectTree::updateContext);
            connect(m_currentProject, &Project::projectLanguagesUpdated,
                    this, &ProjectTree::updateContext);
        }
    }

    if (!node && Core::EditorManager::currentDocument()) {
        connect(Core::EditorManager::currentDocument(), &Core::IDocument::changed,
                this, &ProjectTree::updateExternalFileWarning,
                Qt::UniqueConnection);
    }

    if (changedNode) {
        m_currentNode = node;
        emit currentNodeChanged(m_currentNode, project);
    }


    if (changedProject) {
        emit currentProjectChanged(m_currentProject);
        sessionChanged();
        updateContext();
    }
}

void ProjectTree::sessionChanged()
{
    if (m_currentProject)
        Core::DocumentManager::setDefaultLocationForNewFiles(m_currentProject->projectDirectory().toString());
    else if (SessionManager::startupProject())
        Core::DocumentManager::setDefaultLocationForNewFiles(SessionManager::startupProject()->projectDirectory().toString());
    else
        Core::DocumentManager::setDefaultLocationForNewFiles(QString());
    updateFromFocus();
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

void ProjectTree::emitNodeUpdated(Node *node)
{
    if (!isInNodeHierarchy(node))
        return;
    emit nodeUpdated(node);
}

void ProjectTree::emitAboutToChangeShowInSimpleTree(FolderNode *node)
{
    if (!isInNodeHierarchy(node))
        return;
    emit aboutToChangeShowInSimpleTree(node);
}

void ProjectTree::emitShowInSimpleTreeChanged(FolderNode *node)
{
    if (!isInNodeHierarchy(node))
        return;
    emit showInSimpleTreeChanged(node);
}

void ProjectTree::emitFoldersAboutToBeAdded(FolderNode *parentFolder, const QList<FolderNode *> &newFolders)
{
    if (!isInNodeHierarchy(parentFolder))
        return;

    m_foldersAdded = newFolders;

    emit foldersAboutToBeAdded(parentFolder, newFolders);
}

void ProjectTree::emitFoldersAdded(FolderNode *folder)
{
    if (!isInNodeHierarchy(folder))
        return;

    emit foldersAdded();

    if (Utils::anyOf(m_projectTreeWidgets, &ProjectTreeWidget::hasFocus))
        return;

    if (!m_currentNode) {
        Core::IDocument *document = Core::EditorManager::currentDocument();
        const FileName fileName = document ? document->filePath() : FileName();


        FindNodesForFileVisitor findNodes(fileName);
        foreach (FolderNode *fn, m_foldersAdded)
            fn->accept(&findNodes);

        Node *bestNode = ProjectTreeWidget::mostExpandedNode(findNodes.nodes());
        if (!bestNode)
            return;

        updateFromNode(bestNode);
    }
    m_foldersAdded.clear();
}

void ProjectTree::emitFoldersAboutToBeRemoved(FolderNode *parentFolder, const QList<FolderNode *> &staleFolders)
{
    if (!isInNodeHierarchy(parentFolder))
        return;

    Node *n = ProjectTree::currentNode();
    while (n) {
        if (FolderNode *fn = n->asFolderNode()) {
            if (staleFolders.contains(fn)) {
                ProjectNode *pn = n->projectNode();
                // Make sure the node we are switching too isn't going to be removed also
                while (staleFolders.contains(pn))
                    pn = pn->parentFolderNode()->projectNode();
                m_resetCurrentNodeFolder = true;
                break;
            }
        }
        n = n->parentFolderNode();
    }
    emit foldersAboutToBeRemoved(parentFolder, staleFolders);
}

void ProjectTree::emitFoldersRemoved(FolderNode *folder)
{
    if (!isInNodeHierarchy(folder))
        return;

    emit foldersRemoved();

    if (m_resetCurrentNodeFolder) {
        updateFromFocus(true);
        m_resetCurrentNodeFolder = false;
    }
}

void ProjectTree::emitFilesAboutToBeAdded(FolderNode *folder, const QList<FileNode *> &newFiles)
{
    if (!isInNodeHierarchy(folder))
        return;
    m_filesAdded = newFiles;
    emit filesAboutToBeAdded(folder, newFiles);
}

void ProjectTree::emitFilesAdded(FolderNode *folder)
{
    if (!isInNodeHierarchy(folder))
        return;

    emit filesAdded();

    if (Utils::anyOf(m_projectTreeWidgets, &ProjectTreeWidget::hasFocus))
        return;

    if (!m_currentNode) {
        Core::IDocument *document = Core::EditorManager::currentDocument();
        const FileName fileName = document ? document->filePath() : FileName();

        int index = Utils::indexOf(m_filesAdded, Utils::equal(&FileNode::filePath, fileName));

        if (index == -1)
            return;

        updateFromNode(m_filesAdded.at(index));
    }
    m_filesAdded.clear();
}

void ProjectTree::emitFilesAboutToBeRemoved(FolderNode *folder, const QList<FileNode *> &staleFiles)
{
    if (!isInNodeHierarchy(folder))
        return;

    if (m_currentNode)
        if (FileNode *fileNode = m_currentNode->asFileNode())
            if (staleFiles.contains(fileNode))
                m_resetCurrentNodeFile = true;

    emit filesAboutToBeRemoved(folder, staleFiles);
}

void ProjectTree::emitFilesRemoved(FolderNode *folder)
{
    if (!isInNodeHierarchy(folder))
        return;
    emit filesRemoved();

    if (m_resetCurrentNodeFile) {
        updateFromFocus(true);
        m_resetCurrentNodeFile = false;
    }
}

void ProjectTree::emitNodeSortKeyAboutToChange(Node *node)
{
    if (!isInNodeHierarchy(node))
        return;
    emit nodeSortKeyAboutToChange(node);
}

void ProjectTree::emitNodeSortKeyChanged(Node *node)
{
    if (!isInNodeHierarchy(node))
        return;
    emit nodeSortKeyChanged();
}

void ProjectTree::collapseAll()
{
    if (m_focusForContextMenu)
        m_focusForContextMenu->collapseAll();
}

void ProjectTree::updateExternalFileWarning()
{
    Core::IDocument *document = qobject_cast<Core::IDocument *>(sender());
    if (!document || document->filePath().isEmpty())
        return;
    Core::InfoBar *infoBar = document->infoBar();
    Core::Id externalFileId(EXTERNAL_FILE_WARNING);
    if (!document->isModified()) {
        infoBar->removeInfo(externalFileId);
        return;
    }
    if (!infoBar->canInfoBeAdded(externalFileId))
        return;
    const FileName fileName = document->filePath();
    const QList<Project *> projects = SessionManager::projects();
    if (projects.isEmpty())
        return;
    foreach (Project *project, projects) {
        FileName projectDir = project->projectDirectory();
        if (projectDir.isEmpty())
            continue;
        if (fileName.isChildOf(projectDir))
            return;
        // External file. Test if it under the same VCS
        QString topLevel;
        if (Core::VcsManager::findVersionControlForDirectory(projectDir.toString(), &topLevel)
                && fileName.isChildOf(FileName::fromString(topLevel))) {
            return;
        }
    }
    infoBar->addInfo(Core::InfoBarEntry(externalFileId,
                                        tr("<b>Warning:</b> This file is outside the project directory."),
                                        Core::InfoBarEntry::GlobalSuppressionEnabled));
}

bool ProjectTree::hasFocus(ProjectTreeWidget *widget)
{
    return widget
            && ((widget->focusWidget() && widget->focusWidget()->hasFocus())
                || s_instance->m_focusForContextMenu == widget);
}

void ProjectTree::showContextMenu(ProjectTreeWidget *focus, const QPoint &globalPos, Node *node)
{
    QMenu *contextMenu = 0;

    if (!node)
        node = SessionManager::sessionNode();
    if (node->nodeType() != SessionNodeType) {
        Project *project = SessionManager::projectForNode(node);

        emit s_instance->aboutToShowContextMenu(project, node);
        switch (node->nodeType()) {
        case ProjectNodeType:
            if (node->parentFolderNode() == SessionManager::sessionNode())
                contextMenu = Core::ActionManager::actionContainer(Constants::M_PROJECTCONTEXT)->menu();
            else
                contextMenu = Core::ActionManager::actionContainer(Constants::M_SUBPROJECTCONTEXT)->menu();
            break;
        case VirtualFolderNodeType:
        case FolderNodeType:
            contextMenu = Core::ActionManager::actionContainer(Constants::M_FOLDERCONTEXT)->menu();
            break;
        case FileNodeType:
            contextMenu = Core::ActionManager::actionContainer(Constants::M_FILECONTEXT)->menu();
            break;
        default:
            qWarning("ProjectExplorerPlugin::showContextMenu - Missing handler for node type");
        }
    } else { // session item
        emit s_instance->aboutToShowContextMenu(0, node);

        contextMenu = Core::ActionManager::actionContainer(Constants::M_SESSIONCONTEXT)->menu();
    }

    if (contextMenu && contextMenu->actions().count() > 0) {
        contextMenu->popup(globalPos);
        s_instance->m_focusForContextMenu = focus;
        connect(contextMenu, &QMenu::aboutToHide,
                s_instance, &ProjectTree::hideContextMenu,
                Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));
    }
}

void ProjectTree::highlightProject(Project *project, const QString &message)
{

    Core::ModeManager::activateMode(Core::Constants::MODE_EDIT);
    Core::NavigationWidget *navigation = Core::NavigationWidget::instance();

    // Shows and focusses a project tree
    QWidget *widget = navigation->activateSubWidget(ProjectExplorer::Constants::PROJECTTREE_ID);

    if (auto *projectTreeWidget = qobject_cast<ProjectTreeWidget *>(widget))
        projectTreeWidget->showMessage(project->rootProjectNode(), message);
}

void ProjectTree::hideContextMenu()
{
    m_focusForContextMenu = 0;
}

bool ProjectTree::isInNodeHierarchy(Node *n)
{
    Node *sessionNode = SessionManager::sessionNode();
    do {
        if (n == sessionNode)
            return true;
        n = n->parentFolderNode();
    } while (n);
    return false;
}

} // namespace ProjectExplorer
