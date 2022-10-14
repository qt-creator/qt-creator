// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "idevicefwd.h"

#include <projectexplorer/runcontrol.h>

#include <utils/portlist.h>

namespace ProjectExplorer {

namespace Internal {
class DeviceUsedPortsGathererPrivate;
class SubChannelProvider;
} // Internal

class PROJECTEXPLORER_EXPORT DeviceUsedPortsGatherer : public QObject
{
    Q_OBJECT

public:
    DeviceUsedPortsGatherer(QObject *parent = nullptr);
    ~DeviceUsedPortsGatherer() override;

    void start(const IDeviceConstPtr &device);
    void stop();
    QList<Utils::Port> usedPorts() const;

signals:
    void error(const QString &errMsg);
    void portListReady();

private:
    void handleProcessDone();
    void setupUsedPorts();

    Internal::DeviceUsedPortsGathererPrivate * const d;
};

class PROJECTEXPLORER_EXPORT PortsGatherer : public RunWorker
{
    Q_OBJECT

public:
    explicit PortsGatherer(RunControl *runControl);
    ~PortsGatherer() override;

    QUrl findEndPoint();

protected:
    void start() override;
    void stop() override;

private:
    DeviceUsedPortsGatherer m_portsGatherer;
    Utils::PortList m_portList;
};

class PROJECTEXPLORER_EXPORT ChannelForwarder : public RunWorker
{
    Q_OBJECT

public:
    explicit ChannelForwarder(RunControl *runControl);

    using UrlGetter = std::function<QUrl()>;
    void setFromUrlGetter(const UrlGetter &urlGetter);

    QUrl fromUrl() const { return m_fromUrl; }
    QUrl toUrl() const { return m_toUrl; }

private:
    UrlGetter m_fromUrlGetter;
    QUrl m_fromUrl;
    QUrl m_toUrl;
};

class PROJECTEXPLORER_EXPORT ChannelProvider : public RunWorker
{
    Q_OBJECT

public:
    ChannelProvider(RunControl *runControl, int requiredChannels = 1);
    ~ChannelProvider() override;

    QUrl channel(int i = 0) const;

private:
    QVector<Internal::SubChannelProvider *> m_channelProviders;
};

} // namespace ProjectExplorer
