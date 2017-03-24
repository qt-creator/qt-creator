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

#include "projectmodels.h"

#include "project.h"
#include "projectnodes.h"
#include "projectexplorer.h"
#include "projecttree.h"
#include "session.h"

#include <coreplugin/fileiconprovider.h>
#include <utils/algorithm.h>
#include <utils/dropsupport.h>

#include <QDebug>
#include <QFileInfo>
#include <QFont>
#include <QMimeData>
#include <QLoggingCategory>

using namespace Utils;

namespace ProjectExplorer {

using namespace Internal;

static bool sortNodes(const Node *n1, const Node *n2)
{
    if (n1->priority() > n2->priority())
        return true;
    if (n1->priority() < n2->priority())
        return false;

    const int displayNameResult = caseFriendlyCompare(n1->displayName(), n2->displayName());
    if (displayNameResult != 0)
        return displayNameResult < 0;

    const int filePathResult = caseFriendlyCompare(n1->filePath().toString(),
                                 n2->filePath().toString());
    if (filePathResult != 0)
        return filePathResult < 0;
    return n1 < n2; // sort by pointer value
}

static bool sortWrapperNodes(const WrapperNode *w1, const WrapperNode *w2)
{
    return sortNodes(w1->m_node, w2->m_node);
}

FlatModel::FlatModel(QObject *parent)
    : TreeModel<WrapperNode, WrapperNode>(new WrapperNode(nullptr), parent)
{
    ProjectTree *tree = ProjectTree::instance();
    connect(tree, &ProjectTree::subtreeChanged, this, &FlatModel::update);

    SessionManager *sm = SessionManager::instance();
    connect(sm, &SessionManager::projectRemoved, this, &FlatModel::update);
    connect(sm, &SessionManager::sessionLoaded, this, &FlatModel::loadExpandData);
    connect(sm, &SessionManager::aboutToSaveSession, this, &FlatModel::saveExpandData);
    connect(sm, &SessionManager::projectAdded, this, &FlatModel::handleProjectAdded);
    connect(sm, &SessionManager::startupProjectChanged, this, [this] { layoutChanged(); });
    update();
}

QVariant FlatModel::data(const QModelIndex &index, int role) const
{
    QVariant result;

    if (Node *node = nodeForIndex(index)) {
        FolderNode *folderNode = node->asFolderNode();
        switch (role) {
        case Qt::DisplayRole: {
            result = node->displayName();
            break;
        }
        case Qt::EditRole: {
            result = node->filePath().fileName();
            break;
        }
        case Qt::ToolTipRole: {
            result = node->tooltip();
            break;
        }
        case Qt::DecorationRole: {
            if (folderNode)
                result = folderNode->icon();
            else
                result = Core::FileIconProvider::icon(node->filePath().toString());
            break;
        }
        case Qt::FontRole: {
            QFont font;
            if (Project *project = SessionManager::startupProject()) {
                if (node == project->containerNode())
                    font.setBold(true);
            }
            result = font;
            break;
        }
        case Project::FilePathRole: {
            result = node->filePath().toString();
            break;
        }
        case Project::EnabledRole: {
            result = node->isEnabled();
            break;
        }
        }
    }

    return result;
}

Qt::ItemFlags FlatModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;
    // We claim that everything is editable
    // That's slightly wrong
    // We control the only view, and that one does the checks
    Qt::ItemFlags f = Qt::ItemIsSelectable|Qt::ItemIsEnabled|Qt::ItemIsDragEnabled;
    if (Node *node = nodeForIndex(index)) {
        if (!node->asProjectNode()) {
            // either folder or file node
            if (node->supportedActions(node).contains(Rename))
                f = f | Qt::ItemIsEditable;
        }
    }
    return f;
}

bool FlatModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;
    if (role != Qt::EditRole)
        return false;

    Node *node = nodeForIndex(index);
    QTC_ASSERT(node, return false);

    Utils::FileName orgFilePath = node->filePath();
    Utils::FileName newFilePath = orgFilePath.parentDir().appendPath(value.toString());

    ProjectExplorerPlugin::renameFile(node, newFilePath.toString());
    emit renamed(orgFilePath, newFilePath);
    return true;
}

void FlatModel::update()
{
    rebuildModel();
}

void FlatModel::rebuildModel()
{
    QList<Project *> projects = SessionManager::projects();

    Utils::sort(projects, [](Project *p1, Project *p2) {
        const int displayNameResult = caseFriendlyCompare(p1->displayName(), p2->displayName());
        if (displayNameResult != 0)
            return displayNameResult < 0;
        return p1 < p2; // sort by pointer value
    });

    QSet<Node *> seen;

    rootItem()->removeChildren();
    for (Project *project : projects) {
        WrapperNode *container = new WrapperNode(project->containerNode());

        ProjectNode *projectNode = project->rootProjectNode();
        if (projectNode) {
            addFolderNode(container, projectNode, &seen);
        } else {
            FileNode *projectFileNode = new FileNode(project->projectFilePath(), FileType::Project, false);
            seen.insert(projectFileNode);
            container->appendChild(new WrapperNode(projectFileNode));
        }

        container->sortChildren(&sortWrapperNodes);
        rootItem()->appendChild(container);
    }

    forAllItems([this](WrapperNode *node) {
        if (node->m_node) {
            const QString path = node->m_node->filePath().toString();
            const QString displayName = node->m_node->displayName();
            ExpandData ed(path, displayName);
            if (m_toExpand.contains(ed))
                emit requestExpansion(node->index());
        } else {
            emit requestExpansion(node->index());
        }
    });
}

void FlatModel::onCollapsed(const QModelIndex &idx)
{
    m_toExpand.remove(expandDataForNode(nodeForIndex(idx)));
}

void FlatModel::onExpanded(const QModelIndex &idx)
{
    m_toExpand.insert(expandDataForNode(nodeForIndex(idx)));
}

ExpandData FlatModel::expandDataForNode(const Node *node) const
{
    QTC_ASSERT(node, return ExpandData());
    const QString path = node->filePath().toString();
    const QString displayName = node->displayName();
    return ExpandData(path, displayName);
}

void FlatModel::handleProjectAdded(Project *project)
{
    Node *node = project->rootProjectNode();
    m_toExpand.insert(expandDataForNode(node));
    if (WrapperNode *wrapper = wrapperForNode(node)) {
        wrapper->forFirstLevelChildren([this](WrapperNode *child) {
            m_toExpand.insert(expandDataForNode(child->m_node));
        });
    }
    update();
}

void FlatModel::loadExpandData()
{
    const QList<QVariant> data = SessionManager::value("ProjectTree.ExpandData").value<QList<QVariant>>();
    m_toExpand = Utils::transform<QSet>(data, [](const QVariant &v) { return ExpandData::fromSettings(v); });
    m_toExpand.remove(ExpandData());
}

void FlatModel::saveExpandData()
{
    // TODO if there are multiple ProjectTreeWidgets, the last one saves the data
    QList<QVariant> data = Utils::transform<QList>(m_toExpand, &ExpandData::toSettings);
    SessionManager::setValue(QLatin1String("ProjectTree.ExpandData"), data);
}

void FlatModel::addFolderNode(WrapperNode *parent, FolderNode *folderNode, QSet<Node *> *seen)
{
    for (Node *node : folderNode->nodes()) {
        if (FolderNode *subFolderNode = node->asFolderNode()) {
            if (!filter(subFolderNode) && !seen->contains(subFolderNode)) {
                seen->insert(subFolderNode);
                auto node = new WrapperNode(subFolderNode);
                parent->appendChild(node);
                addFolderNode(node, subFolderNode, seen);
                node->sortChildren(&sortWrapperNodes);
            } else {
                addFolderNode(parent, subFolderNode, seen);
            }
        }
    }
    for (Node *node : folderNode->nodes()) {
        if (FileNode *fileNode = node->asFileNode()) {
            if (!filter(fileNode) && !seen->contains(fileNode)) {
                seen->insert(fileNode);
                parent->appendChild(new WrapperNode(fileNode));
            }
        }
    }
}

Qt::DropActions FlatModel::supportedDragActions() const
{
    return Qt::MoveAction;
}

QStringList FlatModel::mimeTypes() const
{
    return Utils::DropSupport::mimeTypesForFilePaths();
}

QMimeData *FlatModel::mimeData(const QModelIndexList &indexes) const
{
    auto data = new Utils::DropMimeData;
    foreach (const QModelIndex &index, indexes) {
        if (Node *node = nodeForIndex(index)) {
            if (node->asFileNode())
                data->addFile(node->filePath().toString());
            data->addValue(QVariant::fromValue(node));
        }
    }
    return data;
}

WrapperNode *FlatModel::wrapperForNode(const Node *node) const
{
    return findNonRooItem([this, node](WrapperNode *item) {
        return item->m_node == node;
    });
}

QModelIndex FlatModel::indexForNode(const Node *node) const
{
    WrapperNode *wrapper = wrapperForNode(node);
    return wrapper ? indexForItem(wrapper) : QModelIndex();
}

void FlatModel::setProjectFilterEnabled(bool filter)
{
    if (filter == m_filterProjects)
        return;
    m_filterProjects = filter;
    rebuildModel();
}

void FlatModel::setGeneratedFilesFilterEnabled(bool filter)
{
    m_filterGeneratedFiles = filter;
    rebuildModel();
}

bool FlatModel::projectFilterEnabled()
{
    return m_filterProjects;
}

bool FlatModel::generatedFilesFilterEnabled()
{
    return m_filterGeneratedFiles;
}

Node *FlatModel::nodeForIndex(const QModelIndex &index) const
{
    WrapperNode *flatNode = itemForIndex(index);
    return flatNode ? flatNode->m_node : nullptr;
}

bool FlatModel::filter(Node *node) const
{
    bool isHidden = false;
    if (FolderNode *folderNode = node->asFolderNode()) {
        if (m_filterProjects)
            isHidden = !folderNode->showInSimpleTree();
    } else if (FileNode *fileNode = node->asFileNode()) {
        if (m_filterGeneratedFiles)
            isHidden = fileNode->isGenerated();
    }
    return isHidden;
}

const QLoggingCategory &FlatModel::logger()
{
    static QLoggingCategory logger("qtc.projectexplorer.flatmodel");
    return logger;
}

namespace Internal {

int caseFriendlyCompare(const QString &a, const QString &b)
{
    int result = a.compare(b, Qt::CaseInsensitive);
    if (result != 0)
        return result;
    return a.compare(b, Qt::CaseSensitive);
}

}

} // namespace ProjectExplorer

