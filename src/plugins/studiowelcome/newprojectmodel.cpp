/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "newprojectmodel.h"

using namespace StudioWelcome;

/****************** BaseNewProjectModel ******************/

BaseNewProjectModel::BaseNewProjectModel(QObject *parent)
    : QAbstractListModel(parent)
{}

QHash<int, QByteArray> BaseNewProjectModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames[Qt::UserRole] = "name";
    return roleNames;
}

void BaseNewProjectModel::setProjects(const ProjectsByCategory &projectsByCategory)
{
    beginResetModel();

    for (auto &[id, category] : projectsByCategory) {
        m_categories.push_back(category.name);
        m_projects.push_back(category.items);
    }

    endResetModel();
}

/****************** NewProjectCategoryModel ******************/

NewProjectCategoryModel::NewProjectCategoryModel(QObject *parent)
    : BaseNewProjectModel(parent)
{}

int NewProjectCategoryModel::rowCount(const QModelIndex &) const
{
    return static_cast<int>(categories().size());
}

QVariant NewProjectCategoryModel::data(const QModelIndex &index, int role) const
{
    Q_UNUSED(role)
    return categories().at(index.row());
}

/****************** NewProjectModel ******************/

NewProjectModel::NewProjectModel(QObject *parent)
    : BaseNewProjectModel(parent)
{}

int NewProjectModel::rowCount(const QModelIndex &) const
{
    if (projects().empty())
        return 0;

    return static_cast<int>(projectsOfCurrentCategory().size());
}

QVariant NewProjectModel::data(const QModelIndex &index, int role) const
{
    Q_UNUSED(role)
    return projectsOfCurrentCategory().at(index.row()).name;
}

void NewProjectModel::setPage(int index)
{
    beginResetModel();

    m_page = static_cast<size_t>(index);

    endResetModel();
}

QString NewProjectModel::fontIconCode(int index) const
{
    Utils::optional<ProjectItem> projectItem = project(index);
    if (!projectItem)
        return "";

    return projectItem->fontIconCode;
}
