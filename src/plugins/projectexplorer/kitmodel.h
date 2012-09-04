/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef KITMODEL_H
#define KITMODEL_H

#include "projectexplorer_export.h"

#include <QAbstractItemModel>

QT_BEGIN_NAMESPACE
class QTreeWidgetItem;
class QBoxLayout;
QT_END_NAMESPACE

namespace ProjectExplorer {

class Kit;
class KitConfigWidget;
class KitFactory;
class KitManager;

namespace Internal {

class KitNode;

// --------------------------------------------------------------------------
// KitModel:
// --------------------------------------------------------------------------

class KitModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit KitModel(QBoxLayout *parentLayout, QObject *parent = 0);
    ~KitModel();

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    Kit *kit(const QModelIndex &);
    QModelIndex indexOf(Kit *k) const;

    void setDefaultKit(const QModelIndex &index);
    bool isDefaultKit(const QModelIndex &index);

    KitConfigWidget *widget(const QModelIndex &);

    bool isDirty() const;
    bool isDirty(Kit *k) const;

    void apply();

    void markForRemoval(Kit *k);
    void markForAddition(Kit *k);

signals:
    void kitStateChanged();

private slots:
    void addKit(ProjectExplorer::Kit *k);
    void removeKit(ProjectExplorer::Kit *k);
    void updateKit(ProjectExplorer::Kit *k);
    void changeDefaultKit();
    void setDirty();

private:
    QModelIndex index(KitNode *, int column = 0) const;
    KitNode *find(Kit *k) const;
    KitNode *createNode(KitNode *parent, Kit *k, bool changed);
    void setDefaultNode(KitNode *node);

    KitNode *m_root;
    KitNode *m_autoRoot;
    KitNode *m_manualRoot;

    QList<KitNode *> m_toAddList;
    QList<KitNode *> m_toRemoveList;

    QBoxLayout *m_parentLayout;
    KitNode *m_defaultNode;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // KITMODEL_H
