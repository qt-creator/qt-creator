// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "idevicefwd.h"

#include <projectexplorer/runcontrol.h>

#include <solutions/tasking/tasktree.h>

#include <utils/portlist.h>

using namespace Tasking;

namespace ProjectExplorer {

namespace Internal { class DeviceUsedPortsGathererPrivate; }

class PROJECTEXPLORER_EXPORT DeviceUsedPortsGatherer : public QObject
{
    Q_OBJECT

public:
    DeviceUsedPortsGatherer(QObject *parent = nullptr);
    ~DeviceUsedPortsGatherer() override;

    void start();
    void stop();
    void setDevice(const IDeviceConstPtr &device);
    QList<Utils::Port> usedPorts() const;
    QString errorString() const;

signals:
    void done(bool success);

private:
    Internal::DeviceUsedPortsGathererPrivate * const d;
};

class PROJECTEXPLORER_EXPORT DeviceUsedPortsGathererTaskAdapter
    : public TaskAdapter<DeviceUsedPortsGatherer>
{
public:
    DeviceUsedPortsGathererTaskAdapter() {
        connect(task(), &DeviceUsedPortsGatherer::done, this, [this](bool success) {
            emit done(toDoneResult(success));
        });
    }
    void start() final { task()->start(); }
};

class PROJECTEXPLORER_EXPORT PortsGatherer : public RunWorker
{
    Q_OBJECT

public:
    explicit PortsGatherer(RunControl *runControl);

    QUrl findEndPoint();

protected:
    void start() override;
    void stop() override;

private:
    DeviceUsedPortsGatherer m_portsGatherer;
    Utils::PortList m_portList;
};

using DeviceUsedPortsGathererTask = CustomTask<DeviceUsedPortsGathererTaskAdapter>;

} // namespace ProjectExplorer
