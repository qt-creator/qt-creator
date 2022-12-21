// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../projectexplorer_export.h"
#include "idevicefwd.h"

#include <QAbstractItemModel>

namespace Utils { class FilePath; }

namespace ProjectExplorer {

namespace Internal {
class DeviceFileSystemModelPrivate;
class RemoteDirNode;
}

// Very simple read-only model. Symbolic links are not followed.
class PROJECTEXPLORER_EXPORT DeviceFileSystemModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit DeviceFileSystemModel(QObject *parent = nullptr);
    ~DeviceFileSystemModel();

    void setDevice(const IDeviceConstPtr &device);

    // Use this to get the full path of a file or directory.
    static const int PathRole = Qt::UserRole;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
    bool canFetchMore(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    void fetchMore(const QModelIndex &parent) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    void collectEntries(const Utils::FilePath &filePath, Internal::RemoteDirNode *parentNode);

    Internal::DeviceFileSystemModelPrivate * const d;
};

} // namespace ProjectExplorer;
