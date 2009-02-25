/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef ENVIRONMENTEDITMODEL_H
#define ENVIRONMENTEDITMODEL_H

#include "environment.h"

#include <QtCore/QString>
#include <QtCore/QAbstractItemModel>
#include <QtCore/QDebug>
#include <QtGui/QFont>

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT EnvironmentModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    EnvironmentModel();
    ~EnvironmentModel();
    void setBaseEnvironment(const ProjectExplorer::Environment &env);
    void setMergedEnvironments(bool b);
    bool mergedEnvironments();
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    Qt::ItemFlags flags(const QModelIndex &index) const;
    bool hasChildren(const QModelIndex &index) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    QModelIndex addVariable();
    QModelIndex addVariable(const EnvironmentItem& item);
    void removeVariable(const QString &name);
    void unset(const QString &name);
    bool isUnset(const QString &name);
    bool isInBaseEnvironment(const QString &name);
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    QString indexToVariable(const QModelIndex &index) const;
    bool changes(const QString &key) const;

    QList<EnvironmentItem> userChanges() const;
    void setUserChanges(QList<EnvironmentItem> list);
signals:
    void userChangesUpdated();
private:
    void updateResultEnvironment();
    int findInChanges(const QString &name) const;
    int findInResult(const QString &name) const;
    int findInChangesInsertPosition(const QString &name) const;
    int findInResultInsertPosition(const QString &name) const;

    ProjectExplorer::Environment m_baseEnvironment;
    ProjectExplorer::Environment m_resultEnvironment;
    QList<EnvironmentItem> m_items;
    bool m_mergedEnvironments;
};

} // namespace ProjectExplorer

#endif // ENVIRONMENTEDITMODEL_H
