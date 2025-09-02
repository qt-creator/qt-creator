// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QAbstractItemModel>

namespace ProjectExplorer {
class ProjectConfiguration;

// Documentation inside.
class ProjectConfigurationModel : public QAbstractListModel
{
    Q_OBJECT

public:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    ProjectConfiguration *projectConfigurationAt(int i) const;
    int indexFor(ProjectConfiguration *pc) const;

    void addProjectConfiguration(ProjectConfiguration *pc);
    void removeProjectConfiguration(ProjectConfiguration *pc);

    void triggerUpdate();

private:
    void displayNameChanged(ProjectConfiguration *pc);

    QList<ProjectConfiguration *> m_projectConfigurations;
};

} // namespace ProjectExplorer
