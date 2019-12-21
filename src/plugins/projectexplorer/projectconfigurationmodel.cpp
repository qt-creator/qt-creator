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

#include "projectconfigurationmodel.h"

#include "buildconfiguration.h"
#include "deployconfiguration.h"
#include "runconfiguration.h"
#include "target.h"
#include "projectconfiguration.h"

#include <utils/algorithm.h>
#include <utils/stringutils.h>

/*!
    \class ProjectExplorer::ProjectConfigurationModel
    \brief The ProjectConfigurationModel class is a model to represent the build,
    deploy and run configurations of a target.

    To be used in the dropdown lists of comboboxes.
*/

namespace ProjectExplorer {

static bool isOrderedBefore(const ProjectConfiguration *a, const ProjectConfiguration *b)
{
    return Utils::caseFriendlyCompare(a->displayName(), b->displayName()) < 0;
}

ProjectConfigurationModel::ProjectConfigurationModel(Target *target) :
    m_target(target)
{
}

int ProjectConfigurationModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_projectConfigurations.size();
}

int ProjectConfigurationModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 1;
}

void ProjectConfigurationModel::displayNameChanged()
{
    auto pc = qobject_cast<ProjectConfiguration *>(sender());
    if (!pc)
        return;

    // Find the old position
    int oldPos = m_projectConfigurations.indexOf(pc);
    if (oldPos < 0)
        return;

    QModelIndex itemIndex;
    if (oldPos >= 1 && isOrderedBefore(m_projectConfigurations.at(oldPos), m_projectConfigurations.at(oldPos - 1))) {
        // We need to move up
        int newPos = oldPos - 1;
        while (newPos >= 0 && isOrderedBefore(m_projectConfigurations.at(oldPos), m_projectConfigurations.at(newPos))) {
            --newPos;
        }
        ++newPos;

        beginMoveRows(QModelIndex(), oldPos, oldPos, QModelIndex(), newPos);
        m_projectConfigurations.insert(newPos, pc);
        m_projectConfigurations.removeAt(oldPos + 1);
        endMoveRows();
        // Not only did we move, we also changed...
        itemIndex = index(newPos, 0);
    } else if (oldPos < m_projectConfigurations.size() - 1
               && isOrderedBefore(m_projectConfigurations.at(oldPos + 1), m_projectConfigurations.at(oldPos))) {
        // We need to move down
        int newPos = oldPos + 1;
        while (newPos < m_projectConfigurations.size()
            && isOrderedBefore(m_projectConfigurations.at(newPos), m_projectConfigurations.at(oldPos))) {
            ++newPos;
        }
        beginMoveRows(QModelIndex(), oldPos, oldPos, QModelIndex(), newPos);
        m_projectConfigurations.insert(newPos, pc);
        m_projectConfigurations.removeAt(oldPos);
        endMoveRows();

        // We need to subtract one since removing at the old place moves the newIndex down
        itemIndex = index(newPos - 1, 0);
    } else {
        itemIndex = index(oldPos, 0);
    }
    emit dataChanged(itemIndex, itemIndex);
}

QVariant ProjectConfigurationModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole) {
        const int row = index.row();
        if (row < m_projectConfigurations.size())
            return m_projectConfigurations.at(row)->displayName();
    }

    return QVariant();
}

ProjectConfiguration *ProjectConfigurationModel::projectConfigurationAt(int i) const
{
    if (i > m_projectConfigurations.size() || i < 0)
        return nullptr;
    return m_projectConfigurations.at(i);
}

int ProjectConfigurationModel::indexFor(ProjectConfiguration *pc) const
{
    return m_projectConfigurations.indexOf(pc);
}

void ProjectConfigurationModel::addProjectConfiguration(ProjectConfiguration *pc)
{
    // Find the right place to insert
    int i = 0;
    for (; i < m_projectConfigurations.size(); ++i) {
        if (isOrderedBefore(pc, m_projectConfigurations.at(i)))
            break;
    }

    beginInsertRows(QModelIndex(), i, i);
    m_projectConfigurations.insert(i, pc);
    endInsertRows();

    connect(pc, &ProjectConfiguration::displayNameChanged,
            this, &ProjectConfigurationModel::displayNameChanged);
}

void ProjectConfigurationModel::removeProjectConfiguration(ProjectConfiguration *pc)
{
    int i = m_projectConfigurations.indexOf(pc);
    if (i < 0)
        return;
    beginRemoveRows(QModelIndex(), i, i);
    m_projectConfigurations.removeAt(i);
    endRemoveRows();
}

} // ProjectExplorer
