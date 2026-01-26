// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../projectexplorer_export.h"
#include "idevicefwd.h"

#include <utils/filepath.h>

#include <QSortFilterProxyModel>

#include <memory>

namespace Utils { class Id; }

namespace ProjectExplorer {
namespace Internal { class DeviceManagerModelPrivate; }

class PROJECTEXPLORER_EXPORT DeviceManagerModel : public QAbstractListModel
{
public:
    DeviceManagerModel();
    ~DeviceManagerModel() override;

    void setFilter(const QList<Utils::Id> &filter);
    void setTypeFilter(Utils::Id type);
    void showAllEntry();

    IDevicePtr device(int pos) const;
    Utils::Id deviceId(int pos) const;
    int indexOf(IDeviceConstPtr dev) const;
    int indexForId(Utils::Id id) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    void updateDevice(Utils::Id id);

private:
    void handleDeviceAdded(Utils::Id id);
    void handleDeviceRemoved(Utils::Id id);
    void handleDeviceUpdated(Utils::Id id);
    void handleDeviceListChanged();

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool matchesTypeFilter(const IDeviceConstPtr &dev) const;

    const std::unique_ptr<Internal::DeviceManagerModelPrivate> d;
};

// Use with source models that implement the Utils::FilePathRole to filter
// the respective paths by the given device.
class PROJECTEXPLORER_EXPORT DeviceFilterModel : public QSortFilterProxyModel
{
public:
    DeviceFilterModel() = default;

    void setDevice(const IDeviceConstPtr &device);

private:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;

    Utils::FilePath m_deviceRoot;
};

} // namespace ProjectExplorer
