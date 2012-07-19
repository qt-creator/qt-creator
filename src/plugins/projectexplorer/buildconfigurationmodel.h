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

#ifndef BUILDCONFIGURATIONMODEL_H
#define BUILDCONFIGURATIONMODEL_H

#include <QAbstractItemModel>

namespace ProjectExplorer {
class Target;
class BuildConfiguration;

// Documentation inside.
class BuildConfigurationModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit BuildConfigurationModel(Target *target, QObject *parent = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    BuildConfiguration *buildConfigurationAt(int i);
    BuildConfiguration *buildConfigurationFor(const QModelIndex &idx);
    QModelIndex indexFor(BuildConfiguration *rc);
private slots:
    void addedBuildConfiguration(ProjectExplorer::BuildConfiguration*);
    void removedBuildConfiguration(ProjectExplorer::BuildConfiguration*);
    void displayNameChanged();
private:
    Target *m_target;
    QList<BuildConfiguration *> m_buildConfigurations;
};

}

#endif // RUNCONFIGURATIONMODEL_H
