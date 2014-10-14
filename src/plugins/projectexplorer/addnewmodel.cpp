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

#include "addnewmodel.h"

#include "projectexplorer.h"

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

AddNewTree::AddNewTree(const QString &displayName)
    : m_parent(0),
      m_children(QList<AddNewTree *>()),
      m_displayName(displayName),
      m_node(0),
      m_canAdd(true),
      m_priority(-1)
{

}

AddNewTree::AddNewTree(FolderNode *node, QList<AddNewTree *> children, const QString &displayName)
    : m_parent(0),
      m_children(children),
      m_displayName(displayName),
      m_node(0),
      m_canAdd(false),
      m_priority(-1)
{
    if (node)
        m_toolTip = ProjectExplorerPlugin::directoryFor(node);
    foreach (AddNewTree *child, m_children)
        child->m_parent = this;
}

AddNewTree::AddNewTree(FolderNode *node, QList<AddNewTree *> children, const FolderNode::AddNewInformation &info)
    : m_parent(0),
      m_children(children),
      m_displayName(info.displayName),
      m_node(node),
      m_canAdd(true),
      m_priority(info.priority)
{
    if (node)
        m_toolTip = ProjectExplorerPlugin::directoryFor(node);
    foreach (AddNewTree *child, m_children)
        child->m_parent = this;
}

AddNewTree::~AddNewTree()
{
    qDeleteAll(m_children);
}

AddNewTree *AddNewTree::parent() const
{
    return m_parent;
}

QList<AddNewTree *> AddNewTree::children() const
{
    return m_children;
}

bool AddNewTree::canAdd() const
{
    return m_canAdd;
}

QString AddNewTree::displayName() const
{
    return m_displayName;
}

QString AddNewTree::toolTip() const
{
    return m_toolTip;
}

FolderNode *AddNewTree::node() const
{
    return m_node;
}

int AddNewTree::priority() const
{
    return m_priority;
}

AddNewModel::AddNewModel(AddNewTree *root)
    : m_root(root)
{

}

AddNewModel::~AddNewModel()
{
    delete m_root;
}

int AddNewModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return m_root->children().size();
    AddNewTree *tree = static_cast<AddNewTree *>(parent.internalPointer());
    return tree->children().size();
}

int AddNewModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 1;
}

QModelIndex AddNewModel::index(int row, int column, const QModelIndex &parent) const
{
    if (column != 0)
        return QModelIndex();
    if (!parent.isValid()) {
        if (row >= 0 && row < m_root->children().size())
            return createIndex(row, column, m_root->children().at(row));
        return QModelIndex();
    }
    AddNewTree *tree = static_cast<AddNewTree *>(parent.internalPointer());
    if (row >= 0 && row < tree->children().size())
        return createIndex(row, column, tree->children().at(row));
    return QModelIndex();
}

QModelIndex AddNewModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return QModelIndex();
    AddNewTree *childTree = static_cast<AddNewTree *>(child.internalPointer());
    if (childTree == m_root)
        return QModelIndex();
    AddNewTree *parent = childTree->parent();
    if (parent == m_root)
        return QModelIndex();
    AddNewTree *grandparent = parent->parent();
    for (int i = 0; i < grandparent->children().size(); ++i) {
        if (grandparent->children().at(i) == parent)
            return createIndex(i, 0, parent);
    }
    return QModelIndex();
}

QVariant AddNewModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    AddNewTree *tree = static_cast<AddNewTree *>(index.internalPointer());
    if (role == Qt::DisplayRole)
        return tree->displayName();
    else if (role == Qt::ToolTipRole)
        return tree->toolTip();
    return QVariant();
}

Qt::ItemFlags AddNewModel::flags(const QModelIndex &index) const
{
    AddNewTree *tree = static_cast<AddNewTree *>(index.internalPointer());
    if (tree && tree->canAdd())
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    return Qt::NoItemFlags;
}

FolderNode *AddNewModel::nodeForIndex(const QModelIndex &index) const
{
    if (!index.isValid())
        return m_root->node();
    AddNewTree *tree = static_cast<AddNewTree *>(index.internalPointer());
    return tree->node();
}

QModelIndex AddNewModel::indexForTree(AddNewTree *tree) const
{
    if (!tree)
        return index(0, 0, QModelIndex());
    AddNewTree *parent = tree->parent();
    if (!parent)
        return QModelIndex();
    for (int i = 0; i < parent->children().size(); ++i)
        if (parent->children().at(i) == tree)
            return createIndex(i, 0, tree);
    return QModelIndex();
}
