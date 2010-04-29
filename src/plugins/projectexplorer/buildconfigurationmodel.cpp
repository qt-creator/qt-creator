/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "buildconfigurationmodel.h"
#include "target.h"
#include "buildconfiguration.h"

using namespace ProjectExplorer;

///
/// RunConfigurationsModel
///

class BuildConfigurationComparer
{
public:
    bool operator()(BuildConfiguration *a, BuildConfiguration *b)
    {
        return a->displayName() < b->displayName();
    }
};

BuildConfigurationModel::BuildConfigurationModel(Target *target, QObject *parent)
    : QAbstractListModel(parent),
      m_target(target)
{
    m_buildConfigurations = m_target->buildConfigurations();
    qSort(m_buildConfigurations.begin(), m_buildConfigurations.end(), BuildConfigurationComparer());

    connect(target, SIGNAL(addedBuildConfiguration(ProjectExplorer::BuildConfiguration*)),
            this, SLOT(addedBuildConfiguration(ProjectExplorer::BuildConfiguration*)));
    connect(target, SIGNAL(removedBuildConfiguration(ProjectExplorer::BuildConfiguration*)),
            this, SLOT(removedBuildConfiguration(ProjectExplorer::BuildConfiguration*)));

    foreach (BuildConfiguration *bc, m_buildConfigurations)
        connect(bc, SIGNAL(displayNameChanged()),
                this, SLOT(displayNameChanged()));
}

int BuildConfigurationModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_buildConfigurations.size();
}

int BuildConfigurationModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 1;
}

void BuildConfigurationModel::displayNameChanged()
{
    BuildConfiguration *rc = qobject_cast<BuildConfiguration *>(sender());
    if (!rc)
        return;

    BuildConfigurationComparer compare;
    // Find the old position
    int oldPos = m_buildConfigurations.indexOf(rc);

    if (oldPos >= 1 && compare(m_buildConfigurations.at(oldPos), m_buildConfigurations.at(oldPos - 1))) {
        // We need to move up
        int newPos = oldPos - 1;
        while (newPos >= 0 && compare(m_buildConfigurations.at(oldPos), m_buildConfigurations.at(newPos))) {
            --newPos;
        }
        ++newPos;

        beginMoveRows(QModelIndex(), oldPos, oldPos, QModelIndex(), newPos);
        m_buildConfigurations.insert(newPos, rc);
        m_buildConfigurations.removeAt(oldPos + 1);
        endMoveRows();
        // Not only did we move, we also changed...
        emit dataChanged(index(newPos, 0), index(newPos,0));
    } else if  (oldPos < m_buildConfigurations.size() - 1
                && compare(m_buildConfigurations.at(oldPos + 1), m_buildConfigurations.at(oldPos))) {
        // We need to move down
        int newPos = oldPos + 1;
        while (newPos < m_buildConfigurations.size()
            && compare(m_buildConfigurations.at(newPos), m_buildConfigurations.at(oldPos))) {
            ++newPos;
        }
        beginMoveRows(QModelIndex(), oldPos, oldPos, QModelIndex(), newPos);
        m_buildConfigurations.insert(newPos, rc);
        m_buildConfigurations.removeAt(oldPos);
        endMoveRows();

        // We need to subtract one since removing at the old place moves the newIndex down
        emit dataChanged(index(newPos - 1, 0), index(newPos - 1, 0));
    } else {
        emit dataChanged(index(oldPos, 0), index(oldPos, 0));
    }
}

QVariant BuildConfigurationModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole) {
        const int row = index.row();
        if (row < m_buildConfigurations.size()) {
            return m_buildConfigurations.at(row)->displayName();
        }
    }

    return QVariant();
}

BuildConfiguration *BuildConfigurationModel::buildConfigurationAt(int i)
{
    if (i > m_buildConfigurations.size() || i < 0)
        return 0;
    return m_buildConfigurations.at(i);
}

BuildConfiguration *BuildConfigurationModel::buildConfigurationFor(const QModelIndex &idx)
{
    if (idx.row() > m_buildConfigurations.size() || idx.row() < 0)
        return 0;
    return m_buildConfigurations.at(idx.row());
}

QModelIndex BuildConfigurationModel::indexFor(BuildConfiguration *rc)
{
    int idx = m_buildConfigurations.indexOf(rc);
    if (idx == -1)
        return QModelIndex();
    return index(idx, 0);
}

void BuildConfigurationModel::addedBuildConfiguration(ProjectExplorer::BuildConfiguration *bc)
{
    // Find the right place to insert
    BuildConfigurationComparer compare;
    int i = 0;
    for (; i < m_buildConfigurations.size(); ++i) {
        if (compare(bc, m_buildConfigurations.at(i))) {
            break;
        }
    }

    beginInsertRows(QModelIndex(), i, i);
    m_buildConfigurations.insert(i, bc);
    endInsertRows();


    connect(bc, SIGNAL(displayNameChanged()),
            this, SLOT(displayNameChanged()));
}

void BuildConfigurationModel::removedBuildConfiguration(ProjectExplorer::BuildConfiguration *bc)
{
    int i = m_buildConfigurations.indexOf(bc);
    beginRemoveRows(QModelIndex(), i, i);
    m_buildConfigurations.removeAt(i);
    endRemoveRows();
}
