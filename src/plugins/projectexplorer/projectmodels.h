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

#pragma once

#include "expanddata.h"
#include "projectnodes.h"

#include <utils/fileutils.h>
#include <utils/treemodel.h>

#include <QPointer>
#include <QSet>
#include <QTimer>
#include <QTreeView>

namespace ProjectExplorer {

class Node;
class FolderNode;
class Project;
class ProjectNode;

namespace Internal {

class WrapperNode : public Utils::TypedTreeItem<WrapperNode>
{
public:
    explicit WrapperNode(Node *node) : m_node(node) {}
    QPointer<Node> m_node;
};

class FlatModel : public Utils::TreeModel<WrapperNode, WrapperNode>
{
    Q_OBJECT

public:
    FlatModel(QObject *parent);

    // QAbstractItemModel
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    Qt::DropActions supportedDragActions() const override;
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;

    Node *nodeForIndex(const QModelIndex &index) const;
    WrapperNode *wrapperForNode(const Node *node) const;
    QModelIndex indexForNode(const Node *node) const;

    bool projectFilterEnabled();
    bool generatedFilesFilterEnabled();
    void setProjectFilterEnabled(bool filter);
    void setGeneratedFilesFilterEnabled(bool filter);

    void onExpanded(const QModelIndex &idx);
    void onCollapsed(const QModelIndex &idx);

signals:
    void renamed(const Utils::FileName &oldName, const Utils::FileName &newName);
    void requestExpansion(const QModelIndex &index);

private:
    bool m_filterProjects = false;
    bool m_filterGeneratedFiles = true;

    static const QLoggingCategory &logger();

    void updateSubtree(FolderNode *node);
    void rebuildModel();
    void addFolderNode(WrapperNode *parent, FolderNode *folderNode, QSet<Node *> *seen);

    ExpandData expandDataForNode(const Node *node) const;
    void loadExpandData();
    void saveExpandData();
    void handleProjectAdded(Project *project);
    void handleProjectRemoved(Project *project);
    WrapperNode *nodeForProject(Project *project);
    void addOrRebuildProjectModel(Project *project);

    QTimer m_timer;
    QSet<ExpandData> m_toExpand;
};

int caseFriendlyCompare(const QString &a, const QString &b);

} // namespace Internal
} // namespace ProjectExplorer
