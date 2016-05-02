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

namespace {

const auto ComparisonOperator =
    [](const ProjectConfiguration *a, const ProjectConfiguration *b) {
        return a->displayName() < b->displayName();
    };

} // namespace

ProjectConfigurationModel::ProjectConfigurationModel(Target *target, FilterFunction filter,
                                                     QObject *parent) :
    QAbstractListModel(parent),
    m_target(target),
    m_filter(filter)
{
    m_projectConfigurations = Utils::filtered(m_target->projectConfigurations(), m_filter);
    Utils::sort(m_projectConfigurations, ComparisonOperator);

    connect(target, &Target::addedProjectConfiguration,
            this, &ProjectConfigurationModel::addedProjectConfiguration);
    connect(target, &Target::removedProjectConfiguration,
            this, &ProjectConfigurationModel::removedProjectConfiguration);

    foreach (ProjectConfiguration *pc, m_projectConfigurations)
        connect(pc, &ProjectConfiguration::displayNameChanged,
                this, &ProjectConfigurationModel::displayNameChanged);
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

    if (oldPos >= 1 && ComparisonOperator(m_projectConfigurations.at(oldPos), m_projectConfigurations.at(oldPos - 1))) {
        // We need to move up
        int newPos = oldPos - 1;
        while (newPos >= 0 && ComparisonOperator(m_projectConfigurations.at(oldPos), m_projectConfigurations.at(newPos))) {
            --newPos;
        }
        ++newPos;

        beginMoveRows(QModelIndex(), oldPos, oldPos, QModelIndex(), newPos);
        m_projectConfigurations.insert(newPos, pc);
        m_projectConfigurations.removeAt(oldPos + 1);
        endMoveRows();
        // Not only did we move, we also changed...
        emit dataChanged(index(newPos, 0), index(newPos,0));
    } else if (oldPos < m_projectConfigurations.size() - 1
               && ComparisonOperator(m_projectConfigurations.at(oldPos + 1), m_projectConfigurations.at(oldPos))) {
        // We need to move down
        int newPos = oldPos + 1;
        while (newPos < m_projectConfigurations.size()
            && ComparisonOperator(m_projectConfigurations.at(newPos), m_projectConfigurations.at(oldPos))) {
            ++newPos;
        }
        beginMoveRows(QModelIndex(), oldPos, oldPos, QModelIndex(), newPos);
        m_projectConfigurations.insert(newPos, pc);
        m_projectConfigurations.removeAt(oldPos);
        endMoveRows();

        // We need to subtract one since removing at the old place moves the newIndex down
        emit dataChanged(index(newPos - 1, 0), index(newPos - 1, 0));
    } else {
        emit dataChanged(index(oldPos, 0), index(oldPos, 0));
    }
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

ProjectConfiguration *ProjectConfigurationModel::projectConfigurationAt(int i)
{
    if (i > m_projectConfigurations.size() || i < 0)
        return nullptr;
    return m_projectConfigurations.at(i);
}

ProjectConfiguration *ProjectConfigurationModel::projectConfigurationFor(const QModelIndex &idx)
{
    if (idx.row() > m_projectConfigurations.size() || idx.row() < 0)
        return nullptr;
    return m_projectConfigurations.at(idx.row());
}

QModelIndex ProjectConfigurationModel::indexFor(ProjectConfiguration *pc)
{
    int idx = m_projectConfigurations.indexOf(pc);
    if (idx == -1)
        return QModelIndex();
    return index(idx, 0);
}

void ProjectConfigurationModel::addedProjectConfiguration(ProjectConfiguration *pc)
{
    if (!m_filter(pc))
        return;

    // Find the right place to insert
    int i = 0;
    for (; i < m_projectConfigurations.size(); ++i) {
        if (ComparisonOperator(pc, m_projectConfigurations.at(i)))
            break;
    }

    beginInsertRows(QModelIndex(), i, i);
    m_projectConfigurations.insert(i, pc);
    endInsertRows();

    connect(pc, &ProjectConfiguration::displayNameChanged,
            this, &ProjectConfigurationModel::displayNameChanged);
}

void ProjectConfigurationModel::removedProjectConfiguration(ProjectConfiguration *pc)
{
    int i = m_projectConfigurations.indexOf(pc);
    if (i < 0)
        return;
    beginRemoveRows(QModelIndex(), i, i);
    m_projectConfigurations.removeAt(i);
    endRemoveRows();
}

BuildConfigurationModel::BuildConfigurationModel(Target *t, QObject *parent) :
    ProjectConfigurationModel(t,
                              [](const ProjectConfiguration *pc) {
                                  return qobject_cast<const BuildConfiguration *>(pc) != nullptr;
                              },
                              parent)
{ }

DeployConfigurationModel::DeployConfigurationModel(Target *t, QObject *parent) :
    ProjectConfigurationModel(t,
                              [](const ProjectConfiguration *pc) {
                                  return qobject_cast<const DeployConfiguration *>(pc) != nullptr;
                              },
                              parent)
{ }

RunConfigurationModel::RunConfigurationModel(Target *t, QObject *parent) :
    ProjectConfigurationModel(t,
                              [](const ProjectConfiguration *pc) {
                                  return qobject_cast<const RunConfiguration *>(pc) != nullptr;
                              },
                              parent)
{ }
