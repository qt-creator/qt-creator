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
#include <QList>

namespace ProjectExplorer {

namespace Internal { class DeviceProcessListPrivate; }

class PROJECTEXPLORER_EXPORT DeviceProcessItem
{
public:
    DeviceProcessItem() : pid(0) {}
    bool operator<(const DeviceProcessItem &other) const;

    int pid;
    QString cmdLine;
    QString exe;
};

class PROJECTEXPLORER_EXPORT DeviceProcessList : public QAbstractItemModel
{
    Q_OBJECT

public:
    DeviceProcessList(const IDevice::ConstPtr &device, QObject *parent = 0);
    ~DeviceProcessList() override;

    void update();
    void killProcess(int row);
    DeviceProcessItem at(int row) const;

    static QList<DeviceProcessItem> localProcesses();

signals:
    void processListUpdated();
    void error(const QString &errorMsg);
    void processKilled();

protected:
    void reportError(const QString &message);
    void reportProcessKilled();
    void reportProcessListUpdated(const QList<DeviceProcessItem> &processes);

    IDevice::ConstPtr device() const;

private:
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QModelIndex parent(const QModelIndex &) const override;
    bool hasChildren(const QModelIndex &parent) const override;

    virtual void doUpdate() = 0;
    virtual void doKillProcess(const DeviceProcessItem &process) = 0;

    void setFinished();

    Internal::DeviceProcessListPrivate * const d;
};

} // namespace ProjectExplorer
