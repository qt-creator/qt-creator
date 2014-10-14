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

#ifndef UTILS_TREEMODEL_H
#define UTILS_TREEMODEL_H

#include "utils_global.h"

#include <QAbstractItemModel>

namespace Utils {

class QTCREATOR_UTILS_EXPORT TreeItem
{
public:
    TreeItem();
    virtual ~TreeItem();

    virtual TreeItem *parent() const { return m_parent; }
    virtual TreeItem *child(int pos) const;
    virtual bool isLazy() const;
    virtual int columnCount() const;
    virtual int rowCount() const;
    virtual void populate();

    virtual QVariant data(int column, int role) const;
    virtual Qt::ItemFlags flags(int column) const;

    void prependChild(TreeItem *item);
    void appendChild(TreeItem *item);

    void setLazy(bool on);
    void setPopulated(bool on);
    void setFlags(Qt::ItemFlags flags);

private:
    TreeItem(const TreeItem &) Q_DECL_EQ_DELETE;
    void operator=(const TreeItem &) Q_DECL_EQ_DELETE;

    void clear();
    void ensurePopulated() const;

    TreeItem *m_parent; // Not owned.
    QVector<TreeItem *> m_children; // Owned.
    bool m_lazy;
    mutable bool m_populated;
    Qt::ItemFlags m_flags;
};

class QTCREATOR_UTILS_EXPORT TreeModel : public QAbstractItemModel
{
public:
    explicit TreeModel(QObject *parent = 0);
    virtual ~TreeModel();

    int rowCount(const QModelIndex &idx = QModelIndex()) const;
    int columnCount(const QModelIndex &idx) const;

    QVariant data(const QModelIndex &idx, int role) const;
    QModelIndex index(int, int, const QModelIndex &idx) const;
    QModelIndex parent(const QModelIndex &idx) const;
    Qt::ItemFlags flags(const QModelIndex &idx) const;

    TreeItem *rootItem() const;
    TreeItem *itemFromIndex(const QModelIndex &) const;
    QModelIndex indexFromItem(const TreeItem *needle) const;

private:
    QModelIndex indexFromItemHelper(const TreeItem *needle,
        TreeItem *parentItem, const QModelIndex &parentIndex) const;

    void checkIndex(const QModelIndex &index) const;

    TreeItem *m_root; // Owned.
};

} // namespace Utils

#endif // UTILS_TREEMODEL_H
