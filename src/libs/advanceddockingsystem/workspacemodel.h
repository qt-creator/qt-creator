// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

#pragma once

#include <QAbstractTableModel>

#include <functional>

namespace ADS {

class DockManager;
class WorkspaceNameInputDialog;

class WorkspaceModel final : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum { WorkspaceFileName = Qt::UserRole, WorkspacePreset, WorkspaceStartup, WorkspaceActive };

    explicit WorkspaceModel(DockManager *manager, QObject *parent = nullptr);

    int indexOfWorkspace(const QString &fileName);
    QString workspaceAt(int row) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Qt::DropActions supportedDropActions() const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void resetWorkspaces();

private:
    DockManager *m_manager;
};

} // namespace ADS
