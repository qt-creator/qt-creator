// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

bool compareNodes(const Node *n1, const Node *n2);

class WrapperNode : public Utils::TypedTreeItem<WrapperNode>
{
public:
    explicit WrapperNode(Node *node) : m_node(node) {}
    Node *m_node = nullptr;

    void appendClone(const WrapperNode &node);
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
    bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column,
                         const QModelIndex &parent) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column,
                      const QModelIndex &parent) override;

    Node *nodeForIndex(const QModelIndex &index) const;
    WrapperNode *wrapperForNode(const Node *node) const;
    QModelIndex indexForNode(const Node *node) const;

    bool projectFilterEnabled();
    bool generatedFilesFilterEnabled();
    bool disabledFilesFilterEnabled() const { return m_filterDisabledFiles; }
    bool trimEmptyDirectoriesEnabled();
    bool hideSourceGroups() { return m_hideSourceGroups; }
    void setProjectFilterEnabled(bool filter);
    void setGeneratedFilesFilterEnabled(bool filter);
    void setDisabledFilesFilterEnabled(bool filter);
    void setTrimEmptyDirectories(bool filter);
    void setHideSourceGroups(bool filter);

    void onExpanded(const QModelIndex &idx);
    void onCollapsed(const QModelIndex &idx);

signals:
    void renamed(const Utils::FilePath &oldName, const Utils::FilePath &newName);

private:
    bool m_filterProjects = false;
    bool m_filterGeneratedFiles = true;
    bool m_filterDisabledFiles = false;
    bool m_trimEmptyDirectories = true;
    bool m_hideSourceGroups = true;

    static const QLoggingCategory &logger();

    void updateSubtree(FolderNode *node);
    void rebuildModel();
    void addFolderNode(WrapperNode *parent, FolderNode *folderNode, QSet<Node *> *seen);
    bool trimEmptyDirectories(WrapperNode *parent);

    ExpandData expandDataForNode(const Node *node) const;
    void loadExpandData();
    void saveExpandData();
    void handleProjectAdded(Project *project);
    void handleProjectRemoved(Project *project);
    WrapperNode *nodeForProject(const Project *project) const;
    void addOrRebuildProjectModel(Project *project);

    void parsingStateChanged(Project *project);

    QTimer m_timer;
    QSet<ExpandData> m_toExpand;
};

} // namespace Internal
} // namespace ProjectExplorer
