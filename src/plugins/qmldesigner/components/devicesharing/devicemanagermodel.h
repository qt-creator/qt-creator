// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QAbstractTableModel>

namespace QmlDesigner::DeviceShare {

class DeviceManager;
class DeviceManagerModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit DeviceManagerModel(DeviceManager &deviceManager, QObject *parent = nullptr);

    enum DeviceStatus { Offline, Online };
    Q_ENUM(DeviceStatus)

    enum DeviceColumns {
        Status,
        Active,
        Alias,
        IPv4Addr,
        OS,
        OSVersion,
        Architecture,
        ScreenSize,
        AppVersion,
        DeviceId,
        COLUMN_COUNT
    };
    Q_ENUM(DeviceColumns)

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    Q_INVOKABLE bool addDevice(const QString &ip);
    Q_INVOKABLE void removeDevice(const QString &deviceId);

private:
    DeviceManager &m_deviceManager;
};

} // namespace QmlDesigner::DeviceShare
