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

#include "qnxabstractrunsupport.h"
#include "qnxrunconfiguration.h"

#include <projectexplorer/devicesupport/deviceapplicationrunner.h>
#include <projectexplorer/devicesupport/deviceusedportsgatherer.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace RemoteLinux;

using namespace Qnx;
using namespace Qnx::Internal;

QnxAbstractRunSupport::QnxAbstractRunSupport(QnxRunConfiguration *runConfig, QObject *parent)
    : QObject(parent)
    , m_remoteExecutable(runConfig->remoteExecutableFilePath())
    , m_device(DeviceKitInformation::device(runConfig->target()->kit()))
    , m_state(Inactive)
    , m_environment(runConfig->environment())
    , m_workingDir(runConfig->workingDirectory())
{
    m_runner = new DeviceApplicationRunner(this);
    m_portsGatherer = new DeviceUsedPortsGatherer(this);

    connect(m_portsGatherer, SIGNAL(error(QString)), SLOT(handleError(QString)));
    connect(m_portsGatherer, SIGNAL(portListReady()), SLOT(handlePortListReady()));
}

void QnxAbstractRunSupport::handleAdapterSetupRequested()
{
    QTC_ASSERT(m_state == Inactive, return);

    m_state = GatheringPorts;
    m_portsGatherer->start(m_device);
}

void QnxAbstractRunSupport::handlePortListReady()
{
    QTC_ASSERT(m_state == GatheringPorts, return);
    m_portList = m_device->freePorts();
    startExecution();
}

void QnxAbstractRunSupport::handleRemoteProcessStarted()
{
    m_state = Running;
}

void QnxAbstractRunSupport::handleRemoteProcessFinished(bool)
{
}

void QnxAbstractRunSupport::setFinished()
{
    if (m_state != GatheringPorts && m_state != Inactive)
        m_runner->stop();

    m_state = Inactive;
}

QnxAbstractRunSupport::State QnxAbstractRunSupport::state() const
{
    return m_state;
}

void QnxAbstractRunSupport::setState(QnxAbstractRunSupport::State state)
{
    m_state = state;
}

DeviceApplicationRunner *QnxAbstractRunSupport::appRunner() const
{
    return m_runner;
}

const IDevice::ConstPtr QnxAbstractRunSupport::device() const
{
    return m_device;
}

void QnxAbstractRunSupport::handleProgressReport(const QString &)
{
}

void QnxAbstractRunSupport::handleRemoteOutput(const QByteArray &)
{
}

void QnxAbstractRunSupport::handleError(const QString &)
{
}

bool QnxAbstractRunSupport::setPort(int &port)
{
    port = m_portsGatherer->getNextFreePort(&m_portList);
    if (port == -1) {
        handleError(tr("Not enough free ports on device for debugging."));
        return false;
    }
    return true;
}

QString QnxAbstractRunSupport::executable() const
{
    return m_remoteExecutable;
}
