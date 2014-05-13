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
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef ADDNEWMODEL_H
#define ADDNEWMODEL_H

#include "projectnodes.h"

#include <QAbstractItemModel>

namespace ProjectExplorer {
class FolderNode;

namespace Internal {

class AddNewTree
{
public:
    AddNewTree(const QString &displayName);
    AddNewTree(FolderNode *node, QList<AddNewTree *> children, const QString &displayName);
    AddNewTree(FolderNode *node, QList<AddNewTree *> children, const FolderNode::AddNewInformation &info);
    ~AddNewTree();

    AddNewTree *parent() const;
    QList<AddNewTree *> children() const;

    bool canAdd() const;
    QString displayName() const;
    QString toolTip() const;
    FolderNode *node() const;
    int priority() const;
private:
    AddNewTree *m_parent;
    QList<AddNewTree *> m_children;
    QString m_displayName;
    QString m_toolTip;
    FolderNode *m_node;
    bool m_canAdd;
    int m_priority;
};

class AddNewModel : public QAbstractItemModel
{
public:
    AddNewModel(AddNewTree *root);
    ~AddNewModel();
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;
    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    FolderNode *nodeForIndex(const QModelIndex &index) const;
    QModelIndex indexForTree(AddNewTree *tree) const;
private:
    AddNewTree *m_root;
};
}
}

#endif // ADDNEWMODEL_H
