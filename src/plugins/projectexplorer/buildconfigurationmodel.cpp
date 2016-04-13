/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "buildconfigurationmodel.h"
#include "target.h"
#include "buildconfiguration.h"

#include <utils/algorithm.h>

using namespace ProjectExplorer;

/*!
    \class ProjectExplorer::BuildConfigurationModel
    \brief The BuildConfigurationModel class is a model to represent the build
    configurations of a target.

    To be used in the dropdown lists of comboboxes.
    Automatically adjusts itself to added and removed BuildConfigurations.
    Very similar to the Run Configuration Model.

    TODO might it possible to share code without making the code a complete mess.
*/

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
    Utils::sort(m_buildConfigurations, BuildConfigurationComparer());

    connect(target, &Target::addedBuildConfiguration,
            this, &BuildConfigurationModel::addedBuildConfiguration);
    connect(target, &Target::removedBuildConfiguration,
            this, &BuildConfigurationModel::removedBuildConfiguration);

    foreach (BuildConfiguration *bc, m_buildConfigurations)
        connect(bc, &ProjectConfiguration::displayNameChanged,
                this, &BuildConfigurationModel::displayNameChanged);
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
    auto rc = qobject_cast<BuildConfiguration *>(sender());
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
        if (row < m_buildConfigurations.size())
            return m_buildConfigurations.at(row)->displayName();
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

void BuildConfigurationModel::addedBuildConfiguration(BuildConfiguration *bc)
{
    // Find the right place to insert
    BuildConfigurationComparer compare;
    int i = 0;
    for (; i < m_buildConfigurations.size(); ++i) {
        if (compare(bc, m_buildConfigurations.at(i)))
            break;
    }

    beginInsertRows(QModelIndex(), i, i);
    m_buildConfigurations.insert(i, bc);
    endInsertRows();


    connect(bc, &ProjectConfiguration::displayNameChanged,
            this, &BuildConfigurationModel::displayNameChanged);
}

void BuildConfigurationModel::removedBuildConfiguration(BuildConfiguration *bc)
{
    int i = m_buildConfigurations.indexOf(bc);
    beginRemoveRows(QModelIndex(), i, i);
    m_buildConfigurations.removeAt(i);
    endRemoveRows();
}
