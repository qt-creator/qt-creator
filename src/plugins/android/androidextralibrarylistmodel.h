// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QAbstractItemModel>
#include <QStringList>

namespace ProjectExplorer { class BuildSystem; }

namespace Android {

class AndroidExtraLibraryListModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    AndroidExtraLibraryListModel(ProjectExplorer::BuildSystem *buildSystem, QObject *parent);

    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    void removeEntries(QModelIndexList list);
    void addEntries(const QStringList &list);

signals:
    void enabledChanged(bool);

private:
    void updateModel();

    ProjectExplorer::BuildSystem *m_buildSystem;
    QStringList m_entries;
};

} // namespace Android
