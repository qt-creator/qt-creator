/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef PROFILEMODEL_H
#define PROFILEMODEL_H

#include "projectexplorer_export.h"

#include <QAbstractItemModel>

QT_BEGIN_NAMESPACE
class QTreeWidgetItem;
class QBoxLayout;
QT_END_NAMESPACE

namespace ProjectExplorer {

class Profile;
class ProfileConfigWidget;
class ProfileFactory;
class ProfileManager;

namespace Internal {

class ProfileNode;

// --------------------------------------------------------------------------
// ProfileModel:
// --------------------------------------------------------------------------

class ProfileModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit ProfileModel(QBoxLayout *parentLayout, QObject *parent = 0);
    ~ProfileModel();

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    Profile *profile(const QModelIndex &);
    QModelIndex indexOf(Profile *p) const;

    void setDefaultProfile(const QModelIndex &index);
    bool isDefaultProfile(const QModelIndex &index);

    ProfileConfigWidget *widget(const QModelIndex &);

    bool isDirty() const;
    bool isDirty(Profile *p) const;

    void apply();

    void markForRemoval(Profile *p);
    void markForAddition(Profile *p);

signals:
    void profileStateChanged();

private slots:
    void addProfile(ProjectExplorer::Profile *p);
    void removeProfile(ProjectExplorer::Profile *p);
    void updateProfile(ProjectExplorer::Profile *p);
    void changeDefaultProfile();
    void setDirty();

private:
    QModelIndex index(ProfileNode *, int column = 0) const;
    ProfileNode *find(Profile *) const;
    ProfileNode *createNode(ProfileNode *parent, Profile *p, bool changed);
    void setDefaultNode(ProfileNode *node);

    ProfileNode *m_root;
    ProfileNode *m_autoRoot;
    ProfileNode *m_manualRoot;

    QList<ProfileNode *> m_toAddList;
    QList<ProfileNode *> m_toRemoveList;

    QBoxLayout *m_parentLayout;
    ProfileNode *m_defaultNode;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // PROFILEMODEL_H
