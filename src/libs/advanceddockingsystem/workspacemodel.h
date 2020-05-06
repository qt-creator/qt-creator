/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or (at your option) any later version.
** The licenses are as published by the Free Software Foundation
** and appearing in the file LICENSE.LGPLv21 included in the packaging
** of this file. Please review the following information to ensure
** the GNU Lesser General Public License version 2.1 requirements
** will be met: https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <QAbstractTableModel>

#include <functional>

namespace ADS {

class DockManager;
class WorkspaceNameInputDialog;

class WorkspaceModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum { PresetWorkspaceRole = Qt::UserRole + 1, LastWorkspaceRole, ActiveWorkspaceRole };

    explicit WorkspaceModel(DockManager *manager, QObject *parent = nullptr);

    int indexOfWorkspace(const QString &workspace);
    QString workspaceAt(int row) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

signals:
    void workspaceSwitched();
    void workspaceCreated(const QString &workspaceName);

public:
    void resetWorkspaces();
    void newWorkspace(QWidget *parent);
    void cloneWorkspace(QWidget *parent, const QString &workspace);
    void deleteWorkspaces(const QStringList &workspaces);
    void renameWorkspace(QWidget *parent, const QString &workspace);
    void resetWorkspace(const QString &workspace);
    void switchToWorkspace(const QString &workspace);

    void importWorkspace(const QString &workspace);
    void exportWorkspace(const QString &target, const QString &workspace);

private:
    void runWorkspaceNameInputDialog(WorkspaceNameInputDialog *workspaceInputDialog,
                                     std::function<void(const QString &)> createWorkspace);

    QStringList m_sortedWorkspaces;
    DockManager *m_manager;
    int m_currentSortColumn;
    Qt::SortOrder m_currentSortOrder = Qt::AscendingOrder;
};

} // namespace ADS
