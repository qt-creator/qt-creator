/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
class KitFactory;
class KitManager;

namespace Internal {
class KitManagerConfigWidget;

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
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    Kit *kit(const QModelIndex &);
    QModelIndex indexOf(Kit *k) const;

    void setDefaultKit(const QModelIndex &index);
    bool isDefaultKit(const QModelIndex &index);

    ProjectExplorer::Internal::KitManagerConfigWidget *widget(const QModelIndex &);

    bool isDirty() const;
    bool isDirty(Kit *k) const;

    void apply();

    void markForRemoval(Kit *k);
    Kit *markForAddition(Kit *baseKit);

    QString findNameFor(Kit *k, const QString baseName);

signals:
    void kitStateChanged();

private slots:
    void addKit(ProjectExplorer::Kit *k);
    void removeKit(ProjectExplorer::Kit *k);
    void updateKit(ProjectExplorer::Kit*);
    void changeDefaultKit();
    void setDirty();

private:
    QModelIndex index(KitNode *, int column = 0) const;
    KitNode *findWorkingCopy(Kit *k) const;
    KitNode *createNode(KitNode *parent, Kit *k);
    void setDefaultNode(KitNode *node);
    QList<Kit *> kitList(KitNode *node) const;

    KitNode *m_root;
    KitNode *m_autoRoot;
    KitNode *m_manualRoot;

    QList<KitNode *> m_toRemoveList;

    QBoxLayout *m_parentLayout;
    KitNode *m_defaultNode;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // KITMODEL_H
