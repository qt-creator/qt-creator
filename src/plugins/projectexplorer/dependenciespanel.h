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

class DependenciesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DependenciesWidget(Project *project, QWidget *parent = 0);

private:
    Project *m_project;
    DependenciesModel *m_model;
    Utils::DetailsWidget *m_detailsContainer;
    QCheckBox *m_cascadeSetActiveCheckBox;
};

} // namespace Internal
} // namespace ProjectExplorer
