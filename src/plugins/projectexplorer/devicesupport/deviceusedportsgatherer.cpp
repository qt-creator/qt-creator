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

#include "deviceprocess.h"
#include "deviceusedportsgatherer.h"

#include <projectexplorer/runnables.h>

#include <utils/port.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>

using namespace QSsh;
using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

class DeviceUsedPortsGathererPrivate
{
 public:
    QPointer<DeviceProcess> process;
    QList<Port> usedPorts;
    QByteArray remoteStdout;
    QByteArray remoteStderr;
    IDevice::ConstPtr device;
    PortsGatheringMethod::Ptr portsGatheringMethod;
};

} // namespace Internal

DeviceUsedPortsGatherer::DeviceUsedPortsGatherer(QObject *parent) :
    QObject(parent), d(new Internal::DeviceUsedPortsGathererPrivate)
{
}

DeviceUsedPortsGatherer::~DeviceUsedPortsGatherer()
{
    stop();
    delete d;
}

void DeviceUsedPortsGatherer::start(const IDevice::ConstPtr &device)
{
    d->device = device;
    QTC_ASSERT(d->device, emit error("No device given"); return);

    d->portsGatheringMethod = d->device->portsGatheringMethod();
    QTC_ASSERT(d->portsGatheringMethod, emit error("Not implemented"); return);

    const QAbstractSocket::NetworkLayerProtocol protocol = QAbstractSocket::AnyIPProtocol;
    d->process = d->device->createProcess(this);

    connect(d->process.data(), &DeviceProcess::finished,
            this, &DeviceUsedPortsGatherer::handleProcessFinished);
    connect(d->process.data(), &DeviceProcess::error,
            this, &DeviceUsedPortsGatherer::handleProcessError);
    connect(d->process.data(), &DeviceProcess::readyReadStandardOutput,
            this, &DeviceUsedPortsGatherer::handleRemoteStdOut);
    connect(d->process.data(), &DeviceProcess::readyReadStandardError,
            this, &DeviceUsedPortsGatherer::handleRemoteStdErr);

    const Runnable runnable = d->portsGatheringMethod->runnable(protocol);
    d->process->start(runnable);
}

void DeviceUsedPortsGatherer::stop()
{
    d->usedPorts.clear();
    d->remoteStdout.clear();
    d->remoteStderr.clear();
    if (d->process)
        disconnect(d->process.data(), 0, this, 0);
    d->process.clear();
}

Port DeviceUsedPortsGatherer::getNextFreePort(PortList *freePorts) const
{
    while (freePorts->hasMore()) {
        const Port port = freePorts->getNext();
        if (!d->usedPorts.contains(port))
            return port;
    }
    return Port();
}

QList<Port> DeviceUsedPortsGatherer::usedPorts() const
{
    return d->usedPorts;
}

void DeviceUsedPortsGatherer::setupUsedPorts()
{
    d->usedPorts.clear();
    const QList<Port> usedPorts = d->portsGatheringMethod->usedPorts(d->remoteStdout);
    foreach (const Port port, usedPorts) {
        if (d->device->freePorts().contains(port))
            d->usedPorts << port;
    }
    emit portListReady();
}

void DeviceUsedPortsGatherer::handleProcessError()
{
    emit error(tr("Connection error: %1").arg(d->process->errorString()));
    stop();
}

void DeviceUsedPortsGatherer::handleProcessFinished()
{
    if (!d->process)
        return;
    QString errMsg;
    QProcess::ExitStatus exitStatus = d->process->exitStatus();
    switch (exitStatus) {
    case QProcess::CrashExit:
        errMsg = tr("Remote process crashed: %1").arg(d->process->errorString());
        break;
    case QProcess::NormalExit:
        if (d->process->exitCode() == 0)
            setupUsedPorts();
        else
            errMsg = tr("Remote process failed; exit code was %1.").arg(d->process->exitCode());
        break;
    default:
        Q_ASSERT_X(false, Q_FUNC_INFO, "Invalid exit status");
    }

    if (!errMsg.isEmpty()) {
        if (!d->remoteStderr.isEmpty()) {
            errMsg += QLatin1Char('\n');
            errMsg += tr("Remote error output was: %1")
                .arg(QString::fromUtf8(d->remoteStderr));
        }
        emit error(errMsg);
    }
    stop();
}

void DeviceUsedPortsGatherer::handleRemoteStdOut()
{
    if (d->process)
        d->remoteStdout += d->process->readAllStandardOutput();
}

void DeviceUsedPortsGatherer::handleRemoteStdErr()
{
    if (d->process)
        d->remoteStderr += d->process->readAllStandardError();
}

// PortGatherer

PortsGatherer::PortsGatherer(RunControl *runControl)
   : RunWorker(runControl)
{
    setDisplayName("PortGatherer");

    connect(&m_portsGatherer, &DeviceUsedPortsGatherer::error, this, &PortsGatherer::reportFailure);
    connect(&m_portsGatherer, &DeviceUsedPortsGatherer::portListReady, this, [this] {
        m_portList = device()->freePorts();
        appendMessage(tr("Found %n free ports.", nullptr, m_portList.count()), NormalMessageFormat);
        reportStarted();
    });
}

PortsGatherer::~PortsGatherer()
{
}

void PortsGatherer::start()
{
    appendMessage(tr("Checking available ports..."), NormalMessageFormat);
    m_portsGatherer.start(device());
}

Port PortsGatherer::findPort()
{
    return m_portsGatherer.getNextFreePort(&m_portList);
}

void PortsGatherer::stop()
{
    m_portsGatherer.stop();
    reportStopped();
}

} // namespace ProjectExplorer
