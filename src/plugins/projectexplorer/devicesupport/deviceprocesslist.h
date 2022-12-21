// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../projectexplorer_export.h"
#include "idevicefwd.h"

#include <QAbstractItemModel>
#include <QList>

#include <memory>

namespace Utils { class ProcessInfo; }

namespace ProjectExplorer {

namespace Internal { class DeviceProcessListPrivate; }

class PROJECTEXPLORER_EXPORT DeviceProcessList : public QObject
{
    Q_OBJECT

public:
    DeviceProcessList(const IDeviceConstPtr &device, QObject *parent = nullptr);
    ~DeviceProcessList() override;

    void update();
    void killProcess(int row);
    void setOwnPid(qint64 pid);

    Utils::ProcessInfo at(int row) const;
    QAbstractItemModel *model() const;

signals:
    void processListUpdated();
    void error(const QString &errorMsg);
    void processKilled();

protected:
    void reportError(const QString &message);
    void reportProcessKilled();
    void reportProcessListUpdated(const QList<Utils::ProcessInfo> &processes);

    IDeviceConstPtr device() const;

private:
    virtual void doUpdate() = 0;
    virtual void doKillProcess(const Utils::ProcessInfo &process) = 0;

    void setFinished();

    const std::unique_ptr<Internal::DeviceProcessListPrivate> d;
};

} // namespace ProjectExplorer
