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
#include <projectexplorer/runconfiguration.h>

#include <utils/port.h>

namespace RemoteLinux {

namespace Internal { class AbstractRemoteLinuxRunSupportPrivate; }

class REMOTELINUX_EXPORT AbstractRemoteLinuxRunSupport : public ProjectExplorer::TargetRunner
{
    Q_OBJECT

public:
    enum State
    {
        Inactive,
        GatheringResources,
        StartingRunner,
        Running
    };

    explicit AbstractRemoteLinuxRunSupport(ProjectExplorer::RunControl *runControl);
    ~AbstractRemoteLinuxRunSupport();

    ProjectExplorer::ApplicationLauncher *applicationLauncher();

    void setState(State state);
    State state() const;

    void setFinished();

    void startPortsGathering();
    Utils::Port findPort() const;

    void createRemoteFifo();
    QString fifo() const;

    void reset();

signals:
    void adapterSetupFailed(const QString &error);
    void executionStartRequested();

private:
    void handleResourcesError(const QString &message);
    void handleResourcesAvailable();

    Internal::AbstractRemoteLinuxRunSupportPrivate * const d;
};

} // namespace RemoteLinux
