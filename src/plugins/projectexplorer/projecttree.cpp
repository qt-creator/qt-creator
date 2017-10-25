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

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
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

ProjectTree *ProjectTree::s_instance = nullptr;

ProjectTree::ProjectTree(QObject *parent) : QObject(parent)
{
    s_instance = this;

    connect(Core::EditorManager::instance(), &Core::EditorManager::currentEditorChanged,
            this, &ProjectTree::update);

    connect(qApp, &QApplication::focusChanged,
            this, &ProjectTree::update);

    connect(SessionManager::instance(), &SessionManager::projectAdded,
            this, &ProjectTree::sessionAndTreeChanged);
    connect(SessionManager::instance(), &SessionManager::projectRemoved,
            this, &ProjectTree::sessionAndTreeChanged);
    connect(SessionManager::instance(), &SessionManager::startupProjectChanged,
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

Node *ProjectTree::findCurrentNode()
{
    s_instance->update();
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

void ProjectTree::update()
{
    ProjectTreeWidget *focus = m_focusForContextMenu;
    static QPointer<ProjectTreeWidget> lastFocusedProjectTreeWidget;
    if (!focus) {
        focus = Utils::findOrDefault(m_projectTreeWidgets, &ProjectTree::hasFocus);
        lastFocusedProjectTreeWidget = focus;
    }
    if (!focus)
        focus = lastFocusedProjectTreeWidget;

    if (focus)
        updateFromProjectTreeWidget(focus);
    else
        updateFromDocumentManager();
}

void ProjectTree::updateFromProjectTreeWidget(ProjectTreeWidget *widget)
{
    Node *currentNode = widget->currentNode();
    Project *project = SessionManager::projectForNode(currentNode);

    setCurrent(currentNode, project);
}

void ProjectTree::updateFromDocumentManager()
{
    if (Core::IDocument *document = Core::EditorManager::currentDocument()) {
        const FileName fileName = document->filePath();
        updateFromNode(ProjectTreeWidget::nodeForFile(fileName));
    } else {
        updateFromNode(nullptr);
    }
}

void ProjectTree::updateFromNode(Node *node)
{
    Project *project;
    if (node)
        project = SessionManager::projectForNode(node);
    else
        project = SessionManager::startupProject();

    setCurrent(node, project);
    foreach (ProjectTreeWidget *widget, m_projectTreeWidgets)
        widget->sync(node);
}

void ProjectTree::setCurrent(Node *node, Project *project)
{
    const bool changedProject = project != m_currentProject;
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

    if (Core::IDocument *document = Core::EditorManager::currentDocument()) {
        if (node) {
            disconnect(document, &Core::IDocument::changed,
                       this, &ProjectTree::updateExternalFileWarning);
            document->infoBar()->removeInfo(EXTERNAL_FILE_WARNING);
        } else {
            connect(document, &Core::IDocument::changed,
                    this, &ProjectTree::updateExternalFileWarning,
                    Qt::UniqueConnection);
        }
    }

    if (node != m_currentNode) {
        m_currentNode = node;
        emit currentNodeChanged();
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

void ProjectTree::collapseAll()
{
    if (m_focusForContextMenu)
        m_focusForContextMenu->collapseAll();
}

void ProjectTree::updateExternalFileWarning()
{
    auto document = qobject_cast<Core::IDocument *>(sender());
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
    for (Project *project : projects) {
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
    QMenu *contextMenu = nullptr;
    Project *project = SessionManager::projectForNode(node);
    emit s_instance->aboutToShowContextMenu(project, node);

    if (!node) {
        contextMenu = Core::ActionManager::actionContainer(Constants::M_SESSIONCONTEXT)->menu();
    } else {
        switch (node->nodeType()) {
        case NodeType::Project: {
            if ((node->parentFolderNode() && node->parentFolderNode()->asContainerNode())
                    || node->asContainerNode())
                contextMenu = Core::ActionManager::actionContainer(Constants::M_PROJECTCONTEXT)->menu();
            else
                contextMenu = Core::ActionManager::actionContainer(Constants::M_SUBPROJECTCONTEXT)->menu();
            break;
        }
        case NodeType::VirtualFolder:
        case NodeType::Folder:
            contextMenu = Core::ActionManager::actionContainer(Constants::M_FOLDERCONTEXT)->menu();
            break;
        case NodeType::File:
            contextMenu = Core::ActionManager::actionContainer(Constants::M_FILECONTEXT)->menu();
            break;
        default:
            qWarning("ProjectExplorerPlugin::showContextMenu - Missing handler for node type");
        }
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

    // Shows and focusses a project tree
    QWidget *widget = Core::NavigationWidget::activateSubWidget(ProjectExplorer::Constants::PROJECTTREE_ID, Core::Side::Left);

    if (auto *projectTreeWidget = qobject_cast<ProjectTreeWidget *>(widget))
        projectTreeWidget->showMessage(project->rootProjectNode(), message);
}

void ProjectTree::registerTreeManager(const TreeManagerFunction &treeChange)
{
    if (treeChange)
        s_instance->m_treeManagers.append(treeChange);
}

void ProjectTree::applyTreeManager(FolderNode *folder)
{
    if (!folder)
        return;

    for (TreeManagerFunction &f : s_instance->m_treeManagers)
        f(folder);
}

bool ProjectTree::hasNode(const Node *node)
{
    return Utils::contains(SessionManager::projects(), [node](const Project *p) {
        return p && p->rootProjectNode() && (
                    p->containerNode() == node
                    || p->rootProjectNode()->findNode([node](const Node *n) { return n == node; }));
    });
}

void ProjectTree::hideContextMenu()
{
    m_focusForContextMenu = nullptr;
}

} // namespace ProjectExplorer
