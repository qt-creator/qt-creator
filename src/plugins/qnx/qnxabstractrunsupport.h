/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QNXABSTRACTRUNSUPPORT_H
#define QNXABSTRACTRUNSUPPORT_H

#include <projectexplorer/devicesupport/idevice.h>
#include <utils/environment.h>
#include <utils/portlist.h>

#include <QObject>
#include <QString>

namespace ProjectExplorer {
class DeviceApplicationRunner;
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
    bool setPort(int &port);
    virtual void startExecution() = 0;

    virtual QString executable() const;
    Utils::Environment environment() const { return m_environment; }
    QString workingDirectory() const { return m_workingDir; }

    void setFinished();

    State state() const;
    void setState(State state);

    ProjectExplorer::DeviceApplicationRunner *appRunner() const;
    const ProjectExplorer::IDevice::ConstPtr device() const;

protected slots:
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
    const QString m_remoteExecutable;
    ProjectExplorer::IDevice::ConstPtr m_device;
    ProjectExplorer::DeviceApplicationRunner *m_runner;
    State m_state;
    Utils::Environment m_environment;
    QString m_workingDir;
};

} // namespace Internal
} // namespace Qnx

#endif // QNXABSTRACTRUNSUPPORT_H
