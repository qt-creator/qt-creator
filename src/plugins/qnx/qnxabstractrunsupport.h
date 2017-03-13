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

#include <projectexplorer/devicesupport/idevice.h>
#include <utils/environment.h>
#include <utils/portlist.h>

#include <QObject>
#include <QString>

namespace ProjectExplorer {
class ApplicationLauncher;
class DeviceUsedPortsGatherer;
}

namespace Qnx {
namespace Internal {

class QnxRunConfiguration;

class QnxAbstractRunSupport : public QObject
{
    Q_OBJECT
protected:
    enum State {
        Inactive,
        GatheringPorts,
        StartingRemoteProcess,
        Running
    };
public:
    QnxAbstractRunSupport(QnxRunConfiguration *runConfig, QObject *parent = 0);

protected:
    bool setPort(Utils::Port &port);
    virtual void startExecution() = 0;

    void setFinished();

    State state() const;
    void setState(State state);

    ProjectExplorer::ApplicationLauncher *appRunner() const;
    const ProjectExplorer::IDevice::ConstPtr device() const;

public slots:
    virtual void handleAdapterSetupRequested();

    virtual void handleRemoteProcessStarted();
    virtual void handleRemoteProcessFinished(bool);
    virtual void handleProgressReport(const QString &progressOutput);
    virtual void handleRemoteOutput(const QByteArray &output);
    virtual void handleError(const QString &);

private slots:
    void handlePortListReady();

private:
    ProjectExplorer::DeviceUsedPortsGatherer * m_portsGatherer;
    Utils::PortList m_portList;
    ProjectExplorer::IDevice::ConstPtr m_device;
    ProjectExplorer::ApplicationLauncher *m_launcher;
    State m_state;
};

} // namespace Internal
} // namespace Qnx
