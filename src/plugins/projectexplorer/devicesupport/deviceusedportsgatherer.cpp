// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "deviceusedportsgatherer.h"

#include "idevice.h"
#include "sshparameters.h"
#include "../projectexplorertr.h"

#include <utils/port.h>
#include <utils/portlist.h>
#include <utils/process.h>
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
    d->usedPorts.clear();
    d->m_errorString.clear();
    QTC_ASSERT(d->device, emitError("No device given"); return);

    d->portsGatheringMethod = d->device->portsGatheringMethod();
    QTC_ASSERT(d->portsGatheringMethod.commandLine, emitError("Not implemented"); return);
    QTC_ASSERT(d->portsGatheringMethod.parsePorts, emitError("Not implemented"); return);

    const QAbstractSocket::NetworkLayerProtocol protocol = QAbstractSocket::AnyIPProtocol;

    d->process.reset(new Process);
    d->process->setCommand(d->portsGatheringMethod.commandLine(protocol));

    connect(d->process.get(), &Process::done, this, &DeviceUsedPortsGatherer::handleProcessDone);
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

void DeviceUsedPortsGatherer::setupUsedPorts()
{
    d->usedPorts.clear();
    const QList<Port> usedPorts = d->portsGatheringMethod.parsePorts(
                                  d->process->readAllRawStandardOutput());
    for (const Port port : usedPorts) {
        if (d->device->freePorts().contains(port))
            d->usedPorts << port;
    }
    emit portListReady();
}

void DeviceUsedPortsGatherer::emitError(const QString &errorString)
{
    d->m_errorString = errorString;
    emit error(errorString);
}

void DeviceUsedPortsGatherer::handleProcessDone()
{
    if (d->process->result() == ProcessResult::FinishedWithSuccess) {
        setupUsedPorts();
    } else {
        const QString errorString = d->process->errorString();
        const QString stdErr = d->process->readAllStandardError();
        const QString outputString
            = stdErr.isEmpty() ? stdErr : Tr::tr("Remote error output was: %1").arg(stdErr);
        emitError(Utils::joinStrings({errorString, outputString}, '\n'));
    }
    stop();
}

// PortGatherer

PortsGatherer::PortsGatherer(RunControl *runControl)
   : RunWorker(runControl)
{
    setId("PortGatherer");

    connect(&m_portsGatherer, &DeviceUsedPortsGatherer::error, this, &PortsGatherer::reportFailure);
    connect(&m_portsGatherer, &DeviceUsedPortsGatherer::portListReady, this, [this] {
        m_portList = device()->freePorts();
        appendMessage(Tr::tr("Found %n free ports.", nullptr, m_portList.count()), NormalMessageFormat);
        reportStarted();
    });
}

void PortsGatherer::start()
{
    appendMessage(Tr::tr("Checking available ports..."), NormalMessageFormat);
    m_portsGatherer.setDevice(device());
    m_portsGatherer.start();
}

QUrl PortsGatherer::findEndPoint()
{
    QUrl result;
    result.setScheme(urlTcpScheme());
    result.setHost(device()->sshParameters().host());
    result.setPort(m_portList.getNextFreePort(m_portsGatherer.usedPorts()).number());
    return result;
}

void PortsGatherer::stop()
{
    m_portsGatherer.stop();
    reportStopped();
}

namespace Internal {

// SubChannelProvider

/*!
    \class ProjectExplorer::SubChannelProvider

    \internal

    This is a helper RunWorker implementation to either use or not
    use port forwarding for one SubChannel in the ChannelProvider
    implementation.

    By default it is assumed that no forwarding is needed, i.e.
    end points provided by the shared endpoint resource provider
    are directly accessible.
*/

class SubChannelProvider : public RunWorker
{
public:
    SubChannelProvider(RunControl *runControl, PortsGatherer *portsGatherer)
        : RunWorker(runControl)
        , m_portGatherer(portsGatherer)
    {
        setId("SubChannelProvider");
    }

    void start() final
    {
        m_channel.setScheme(urlTcpScheme());
        m_channel.setHost(device()->toolControlChannel(IDevice::ControlChannelHint()).host());
        if (m_portGatherer)
            m_channel.setPort(m_portGatherer->findEndPoint().port());
        reportStarted();
    }

    QUrl channel() const { return m_channel; }

private:
    QUrl m_channel;
    PortsGatherer *m_portGatherer = nullptr;
};

} // Internal

// ChannelProvider

/*!
    \class ProjectExplorer::ChannelProvider

    \internal

    The class implements a \c RunWorker to provide
    to provide a set of urls indicating usable connection end
    points for 'server-using' tools (typically one, like plain
    gdbserver and the Qml tooling, but two for mixed debugging).

    Urls can describe local or tcp servers that are directly
    accessible to the host tools.

    The tool implementations can assume that any needed port
    forwarding setup is setup and handled transparently by
    a \c ChannelProvider instance.

    If there are multiple subchannels needed that need to share a
    common set of resources on the remote side, a device implementation
    can provide a "SharedEndpointGatherer" RunWorker.

    If none is provided, it is assumed that the shared resource
    is open TCP ports, provided by the device's PortGatherer i
    implementation.

    FIXME: The current implementation supports only the case
    of "any number of TCP channels that do not need actual
    forwarding.
*/

ChannelProvider::ChannelProvider(RunControl *runControl, int requiredChannels)
   : RunWorker(runControl)
{
    setId("ChannelProvider");
    auto portsGatherer = new PortsGatherer(runControl);

    for (int i = 0; i < requiredChannels; ++i) {
        auto channelProvider = new Internal::SubChannelProvider(runControl, portsGatherer);
        m_channelProviders.append(channelProvider);
        addStartDependency(channelProvider);
    }
}

ChannelProvider::~ChannelProvider() = default;

QUrl ChannelProvider::channel(int i) const
{
    if (Internal::SubChannelProvider *provider = m_channelProviders.value(i))
        return provider->channel();
    return {};
}

} // namespace ProjectExplorer
