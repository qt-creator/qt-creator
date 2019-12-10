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

#include <memory>

namespace ProjectExplorer {

namespace Internal { class DeviceProcessListPrivate; }

class PROJECTEXPLORER_EXPORT DeviceProcessItem
{
public:
    bool operator<(const DeviceProcessItem &other) const;

    qint64 pid = 0;
    QString cmdLine;
    QString exe;
};

class PROJECTEXPLORER_EXPORT DeviceProcessList : public QObject
{
    Q_OBJECT

public:
    DeviceProcessList(const IDevice::ConstPtr &device, QObject *parent = nullptr);
    ~DeviceProcessList() override;

    void update();
    void killProcess(int row);
    void setOwnPid(qint64 pid);

    DeviceProcessItem at(int row) const;
    QAbstractItemModel *model() const;

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
    virtual void doUpdate() = 0;
    virtual void doKillProcess(const DeviceProcessItem &process) = 0;

    void setFinished();

    const std::unique_ptr<Internal::DeviceProcessListPrivate> d;
};

} // namespace ProjectExplorer
