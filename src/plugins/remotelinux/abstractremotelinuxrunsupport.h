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

#include "remotelinux_export.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <utils/port.h>

#include <QObject>

namespace ProjectExplorer {
class DeviceApplicationRunner;
class RunConfiguration;
class StandardRunnable;
}

namespace RemoteLinux {

namespace Internal { class AbstractRemoteLinuxRunSupportPrivate; }

class REMOTELINUX_EXPORT AbstractRemoteLinuxRunSupport : public QObject
{
    Q_OBJECT
protected:
    enum State
    {
        Inactive,
        GatheringResources,
        StartingRunner,
        Running
    };
public:
    AbstractRemoteLinuxRunSupport(ProjectExplorer::RunConfiguration *runConfig,
                          QObject *parent = 0);
    ~AbstractRemoteLinuxRunSupport();

protected:
    void setState(State state);
    State state() const;
    ProjectExplorer::DeviceApplicationRunner *appRunner() const;

    virtual void startExecution() = 0;

    virtual void handleAdapterSetupFailed(const QString &error);
    virtual void handleAdapterSetupDone();

    void setFinished();

    void startPortsGathering();
    Utils::Port findPort() const;

    void createRemoteFifo();
    QString fifo() const;

    const ProjectExplorer::IDevice::ConstPtr device() const;
    const ProjectExplorer::StandardRunnable &runnable() const;

    void reset();

protected:
    virtual void handleRemoteSetupRequested() = 0;
    virtual void handleAppRunnerError(const QString &error) = 0;
    virtual void handleRemoteOutput(const QByteArray &output) = 0;
    virtual void handleRemoteErrorOutput(const QByteArray &output) = 0;
    virtual void handleAppRunnerFinished(bool success) = 0;
    virtual void handleProgressReport(const QString &progressOutput) = 0;

private:
    void handleResourcesError(const QString &message);
    void handleResourcesAvailable();

    friend class Internal::AbstractRemoteLinuxRunSupportPrivate;
    Internal::AbstractRemoteLinuxRunSupportPrivate * const d;
};

} // namespace RemoteLinux
