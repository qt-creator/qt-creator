// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/projectsettingswidget.h>

#include <QAbstractListModel>
#include <QTreeView>

QT_BEGIN_NAMESPACE
class QCheckBox;
QT_END_NAMESPACE

namespace Utils { class DetailsWidget; }

namespace ProjectExplorer {

class Project;

namespace Internal {

//
// DependenciesModel
//

class DependenciesModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit DependenciesModel(Project *project, QObject *parent = nullptr);

    int rowCount(const QModelIndex &index) const override;
    int columnCount(const QModelIndex &index) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

private:
    void resetModel();

    Project *m_project;
    QList<Project *> m_projects;
};

class DependenciesView : public QTreeView
{
    Q_OBJECT

public:
    explicit DependenciesView(QWidget *parent);

    QSize sizeHint() const override;
    void setModel(QAbstractItemModel *model) override;

private:
    void updateSizeHint();

    QSize m_sizeHint;
};

class DependenciesWidget : public ProjectSettingsWidget
{
    Q_OBJECT

public:
    explicit DependenciesWidget(Project *project, QWidget *parent = nullptr);

private:
    Project *m_project;
    DependenciesModel *m_model;
    Utils::DetailsWidget *m_detailsContainer;
    QCheckBox *m_cascadeSetActiveCheckBox;
};

} // namespace Internal
} // namespace ProjectExplorer
