// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "deviceusedportsgatherer.h"

#include "idevice.h"
#include "sshparameters.h"
#include "../projectexplorertr.h"

#include <remotelinux/remotelinux_constants.h>

#include <utils/port.h>
#include <utils/portlist.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/url.h>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

class DeviceUsedPortsGathererPrivate
{
public:
    std::unique_ptr<Process> process;
    QList<Port> usedPorts;
    IDevice::ConstPtr device;
    PortsGatheringMethod portsGatheringMethod;
    QString m_errorString;
};

} // namespace Internal

DeviceUsedPortsGatherer::DeviceUsedPortsGatherer(QObject *parent)
    : QObject(parent)
    , d(new Internal::DeviceUsedPortsGathererPrivate)
{}

DeviceUsedPortsGatherer::~DeviceUsedPortsGatherer()
{
    stop();
    delete d;
}

void DeviceUsedPortsGatherer::start()
{
    const auto emitError = [this](const QString &errorString) {
        d->m_errorString = errorString;
        emit done(false);
    };

    d->usedPorts.clear();
    d->m_errorString.clear();
    QTC_ASSERT(d->device, emitError("No device given"); return);

    d->portsGatheringMethod = d->device->portsGatheringMethod();
    QTC_ASSERT(d->portsGatheringMethod.commandLine, emitError("Not implemented"); return);
    QTC_ASSERT(d->portsGatheringMethod.parsePorts, emitError("Not implemented"); return);

    const QAbstractSocket::NetworkLayerProtocol protocol = QAbstractSocket::AnyIPProtocol;

    d->process.reset(new Process);
    d->process->setCommand(d->portsGatheringMethod.commandLine(protocol));

    connect(d->process.get(), &Process::done, this, [this, emitError] {
        if (d->process->result() == ProcessResult::FinishedWithSuccess) {
            d->usedPorts.clear();
            const QList<Port> usedPorts = d->portsGatheringMethod.parsePorts(
                d->process->rawStdOut());
            for (const Port port : usedPorts) {
                if (d->device->freePorts().contains(port))
                    d->usedPorts << port;
            }
            emit done(true);
        } else {
            const QString errorString = d->process->errorString();
            const QString stdErr = d->process->readAllStandardError();
            const QString outputString
                = stdErr.isEmpty() ? stdErr : Tr::tr("Remote error output was: %1").arg(stdErr);
            emitError(Utils::joinStrings({errorString, outputString}, '\n'));
        }
        stop();
    });
    d->process->start();
}

void DeviceUsedPortsGatherer::stop()
{
    if (d->process) {
        d->process->disconnect();
        d->process.release()->deleteLater();
    }
}

void DeviceUsedPortsGatherer::setDevice(const IDeviceConstPtr &device)
{
    d->device = device;
}

QList<Port> DeviceUsedPortsGatherer::usedPorts() const
{
    return d->usedPorts;
}

QString DeviceUsedPortsGatherer::errorString() const
{
    return d->m_errorString;
}

} // namespace ProjectExplorer
