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

#pragma once

#include <QAbstractItemModel>

#include <functional>

namespace ProjectExplorer {
class Target;
class ProjectConfiguration;

// Documentation inside.
class ProjectConfigurationModel : public QAbstractListModel
{
    Q_OBJECT
public:
    using FilterFunction = std::function<bool(const ProjectConfiguration *)>;

    explicit ProjectConfigurationModel(Target *target, FilterFunction filter,
                                       QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    ProjectConfiguration *projectConfigurationAt(int i);
    ProjectConfiguration *projectConfigurationFor(const QModelIndex &idx);
    QModelIndex indexFor(ProjectConfiguration *pc);

private:
    void addedProjectConfiguration(ProjectConfiguration *pc);
    void removedProjectConfiguration(ProjectConfiguration *pc);
    void displayNameChanged();

    Target *m_target;
    FilterFunction m_filter;
    QList<ProjectConfiguration *> m_projectConfigurations;
};

class BuildConfigurationModel : public ProjectConfigurationModel
{
public:
    explicit BuildConfigurationModel(Target *t, QObject *parent = nullptr);
};

class DeployConfigurationModel : public ProjectConfigurationModel
{
public:
    explicit DeployConfigurationModel(Target *t, QObject *parent = nullptr);
};

class RunConfigurationModel : public ProjectConfigurationModel
{
public:
    explicit RunConfigurationModel(Target *t, QObject *parent = nullptr);
};

} // namespace ProjectExplorer
