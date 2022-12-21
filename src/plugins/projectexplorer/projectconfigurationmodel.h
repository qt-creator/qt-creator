// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    explicit ProjectConfigurationModel(Target *target);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    ProjectConfiguration *projectConfigurationAt(int i) const;
    int indexFor(ProjectConfiguration *pc) const;

    void addProjectConfiguration(ProjectConfiguration *pc);
    void removeProjectConfiguration(ProjectConfiguration *pc);

private:
    void displayNameChanged(ProjectConfiguration *pc);

    Target *m_target;
    QList<ProjectConfiguration *> m_projectConfigurations;
};

} // namespace ProjectExplorer
