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

#include "idevice.h"

#include <QAbstractItemModel>

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

    void setDevice(const IDevice::ConstPtr &device);

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

} // namespace QSsh;
