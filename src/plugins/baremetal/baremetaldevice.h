// Copyright (C) 2016 Tim Sander <tim@krieglstein.org>
// Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/idevicefactory.h>

namespace BareMetal::Internal {

class IDebugServerProvider;

// BareMetalDevice

class BareMetalDevice final : public ProjectExplorer::IDevice
{
public:
    using Ptr = QSharedPointer<BareMetalDevice>;
    using ConstPtr = QSharedPointer<const BareMetalDevice>;

    static Ptr create() { return Ptr(new BareMetalDevice); }
    ~BareMetalDevice() final;

    static QString defaultDisplayName();

    ProjectExplorer::IDeviceWidget *createWidget() final;

    QString debugServerProviderId() const;
    void setDebugServerProviderId(const QString &id);
    void unregisterDebugServerProvider(IDebugServerProvider *provider);

protected:
    void fromMap(const Utils::Store &map) final;
    Utils::Store toMap() const final;

private:
    BareMetalDevice();
    QString m_debugServerProviderId;
};

// BareMetalDeviceFactory

class BareMetalDeviceFactory final : public ProjectExplorer::IDeviceFactory
{
public:
    BareMetalDeviceFactory();
};

} // BareMetal::Internal
