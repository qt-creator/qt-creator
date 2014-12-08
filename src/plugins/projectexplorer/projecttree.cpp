/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "projecttree.h"
#include "projecttreewidget.h"
#include "session.h"
#include "project.h"
#include "projectnodes.h"

#include <utils/algorithm.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/infobar.h>
#include <coreplugin/vcsmanager.h>

#include <QApplication>

namespace {
const char EXTERNAL_FILE_WARNING[] = "ExternalFile";
}

using namespace ProjectExplorer;
using namespace Internal;

ProjectTree *ProjectTree::s_instance = 0;

ProjectTree::ProjectTree(QObject *parent)
    : QObject(parent),
      m_currentNode(0),
      m_currentProject(0),
      m_resetCurrentNodeFolder(false),
      m_resetCurrentNodeFile(false),
      m_resetCurrentNodeProject(false)
{
    s_instance = this;

    connect(Core::DocumentManager::instance(), &Core::DocumentManager::currentFileChanged,
            this, &ProjectTree::documentManagerCurrentFileChanged);

    SessionManager *session = SessionManager::instance();
    connect(session, &SessionManager::aboutToRemoveProject,
            this, &ProjectTree::aboutToRemoveProject);
    connect(session, &SessionManager::projectRemoved,
            this, &ProjectTree::projectRemoved);


    NodesWatcher *watcher = new NodesWatcher(this);
    SessionManager::sessionNode()->registerWatcher(watcher);

    connect(watcher, &NodesWatcher::foldersAboutToBeRemoved,
            this, &ProjectTree::foldersAboutToBeRemoved);
    connect(watcher, &NodesWatcher::foldersRemoved,
            this, &ProjectTree::foldersRemoved);

    connect(watcher, &NodesWatcher::filesAboutToBeRemoved,
            this, &ProjectTree::filesAboutToBeRemoved);
    connect(watcher, &NodesWatcher::filesRemoved,
            this, &ProjectTree::filesRemoved);

    connect(watcher, &NodesWatcher::foldersAdded,
            this, &ProjectTree::nodesAdded);
    connect(watcher, &NodesWatcher::filesAdded,
            this, &ProjectTree::nodesAdded);

    connect(qApp, &QApplication::focusChanged,
            this, &ProjectTree::focusChanged);
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

bool ProjectTree::editorHasFocus()
{
    Core::IEditor *editor = Core::EditorManager::currentEditor();
    if (!editor)
        return false;
    QWidget *widget = editor->widget();
    if (!widget)
        return false;
    widget = widget->focusWidget();
    if (!widget)
        return false;
    return widget->hasFocus();
}

void ProjectTree::updateFromFocus(bool invalidCurrentNode)
{
    ProjectTreeWidget *focus = Utils::findOrDefault(s_instance->m_projectTreeWidgets,
                                                    &ProjectTree::hasFocus);

    if (focus)
        updateFromProjectTreeWidget(focus);
    else if (editorHasFocus())
        updateFromDocumentManager(invalidCurrentNode);
}

void ProjectTree::updateFromProjectTreeWidget(ProjectTreeWidget *widget)
{
    ProjectExplorer::Node *currentNode = widget->currentNode();
    ProjectExplorer::Project *project = projectForNode(currentNode);

    update(currentNode, project);
}

void ProjectTree::documentManagerCurrentFileChanged()
{
    if (editorHasFocus())
        updateFromDocumentManager();
}

Project *ProjectTree::projectForNode(Node *node)
{
    if (!node)
        return 0;

    FolderNode *rootProjectNode = qobject_cast<FolderNode*>(node);
    if (!rootProjectNode)
        rootProjectNode = node->parentFolderNode();

    while (rootProjectNode && rootProjectNode->parentFolderNode() != SessionManager::sessionNode())
        rootProjectNode = rootProjectNode->parentFolderNode();

    Q_ASSERT(rootProjectNode);

    return Utils::findOrDefault(SessionManager::projects(), Utils::equal(&Project::rootProjectNode, rootProjectNode));
}

void ProjectTree::updateFromDocumentManager(bool invalidCurrentNode)
{
    const QString &fileName = Core::DocumentManager::currentFile();

    ProjectExplorer::Node *currentNode = 0;
    if (!invalidCurrentNode && m_currentNode && m_currentNode->path() == fileName)
        currentNode = m_currentNode;
    else
        currentNode = ProjectTreeWidget::nodeForFile(fileName);

    ProjectExplorer::Project *project = projectForNode(currentNode);

    update(currentNode, project);
    foreach (ProjectTreeWidget *widget, m_projectTreeWidgets)
        widget->sync(currentNode);
}

void ProjectTree::update(Node *node, Project *project)
{
    if (project != m_currentProject) {
        if (m_currentProject) {
            disconnect(m_currentProject, &Project::projectContextUpdated,
                       this, &ProjectTree::updateContext);
            disconnect(m_currentProject, &Project::projectLanguagesUpdated,
                       this, &ProjectTree::updateContext);
        }

        m_currentProject = project;
        emit currentProjectChanged(m_currentProject);

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

    if (node != m_currentNode) {
        m_currentNode = node;
        emit currentNodeChanged(m_currentNode, project);
    }

    updateContext();
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

void ProjectTree::foldersAboutToBeRemoved(FolderNode *, const QList<FolderNode*> &list)
{
    Node *n = ProjectTree::currentNode();
    while (n) {
        if (FolderNode *fn = qobject_cast<FolderNode *>(n)) {
            if (list.contains(fn)) {
                ProjectNode *pn = n->projectNode();
                // Make sure the node we are switching too isn't going to be removed also
                while (list.contains(pn))
                    pn = pn->parentFolderNode()->projectNode();
                m_resetCurrentNodeFolder = true;
                return;
            }
        }
        n = n->parentFolderNode();
    }
}

void ProjectTree::foldersRemoved()
{
    if (m_resetCurrentNodeFolder) {
        updateFromFocus(true);
        m_resetCurrentNodeFolder = false;
    }
}

void ProjectTree::filesAboutToBeRemoved(FolderNode *, const QList<FileNode*> &list)
{
    if (FileNode *fileNode = qobject_cast<FileNode *>(m_currentNode))
        if (list.contains(fileNode))
            m_resetCurrentNodeFile = false;
}

void ProjectTree::filesRemoved()
{
    if (m_resetCurrentNodeFile) {
        updateFromFocus(true);
        m_resetCurrentNodeFile = false;
    }
}

void ProjectTree::aboutToRemoveProject(Project *project)
{
    if (m_currentProject == project)
        m_resetCurrentNodeProject = true;
}

void ProjectTree::projectRemoved()
{
    updateFromFocus(true);
    m_resetCurrentNodeProject = false;
}

void ProjectTree::nodesAdded()
{
    if (Utils::anyOf(m_projectTreeWidgets, &ProjectTreeWidget::hasFocus))
        return;

    updateFromDocumentManager();
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
    Utils::FileName fileName = Utils::FileName::fromString(document->filePath());
    foreach (Project *project, SessionManager::projects()) {
        Utils::FileName projectDir = project->projectDirectory();
        if (projectDir.isEmpty())
            continue;
        if (fileName.isChildOf(projectDir))
            return;
        // External file. Test if it under the same VCS
        QString topLevel;
        if (Core::VcsManager::findVersionControlForDirectory(projectDir.toString(), &topLevel)
                && fileName.isChildOf(Utils::FileName::fromString(topLevel))) {
            return;
        }
    }
    infoBar->addInfo(Core::InfoBarEntry(externalFileId,
                                        tr("<b>Warning:</b> This file is outside the project directory."),
                                        Core::InfoBarEntry::GlobalSuppressionEnabled));
}

bool ProjectExplorer::ProjectTree::hasFocus(ProjectExplorer::Internal::ProjectTreeWidget *widget)
{
    return widget && widget->focusWidget() && widget->focusWidget()->hasFocus();
}
