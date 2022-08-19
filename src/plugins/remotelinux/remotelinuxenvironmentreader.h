// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/devicesupport/idevicefwd.h>
#include <utils/environment.h>

#include <QObject>
#include <QSharedPointer>

namespace Utils { class QtcProcess; }

namespace RemoteLinux {
namespace Internal {

class RemoteLinuxEnvironmentReader : public QObject
{
    Q_OBJECT

public:
    RemoteLinuxEnvironmentReader(const ProjectExplorer::IDeviceConstPtr &device,
                                 QObject *parent = nullptr);
    void start();
    void stop();

    Utils::Environment remoteEnvironment() const { return m_env; }
    void handleCurrentDeviceConfigChanged();

signals:
    void finished();
    void error(const QString &error);

private:
    void handleDone();
    void setFinished();

    Utils::Environment m_env;
    ProjectExplorer::IDeviceConstPtr m_device;
    Utils::QtcProcess *m_deviceProcess = nullptr;
};

} // namespace Internal
} // namespace RemoteLinux
