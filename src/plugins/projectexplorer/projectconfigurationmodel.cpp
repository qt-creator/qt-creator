// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectconfigurationmodel.h"

#include "buildconfiguration.h"
#include "deployconfiguration.h"
#include "projectconfiguration.h"
#include "projectexplorertr.h"
#include "runconfiguration.h"
#include "target.h"

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
    connect(target, &Target::runConfigurationsUpdated, this, [this] {
        emit dataChanged(index(0, 0), index(rowCount(), 0));
    });
}

int ProjectConfigurationModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_projectConfigurations.size();
}

int ProjectConfigurationModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 1;
}

void ProjectConfigurationModel::displayNameChanged(ProjectConfiguration *pc)
{
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
    if (index.row() >= m_projectConfigurations.size())
        return {};

    if (role == Qt::DisplayRole) {
        ProjectConfiguration * const config = m_projectConfigurations.at(index.row());
        QString displayName = config->expandedDisplayName();
        if (const auto rc = qobject_cast<RunConfiguration *>(config); rc && !rc->hasCreator())
            displayName += QString(" [%1]").arg(Tr::tr("unavailable"));
        return displayName;
    }
    return {};
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
            this, [this, pc] { displayNameChanged(pc); });
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
