// Copyright (C) 2016 Tim Sander <tim@krieglstein.org>
// Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "baremetaldevice.h"

#include "baremetalconstants.h"
#include "baremetaldeviceconfigurationwidget.h"
#include "baremetaldeviceconfigurationwizard.h"
#include "baremetaltr.h"
#include "debugserverprovidermanager.h"
#include "idebugserverprovider.h"

#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace BareMetal::Internal {

const char debugServerProviderIdKeyC[] = "IDebugServerProviderId";

// BareMetalDevice

BareMetalDevice::BareMetalDevice()
{
    setDisplayType(Tr::tr("Bare Metal"));
    setOsType(Utils::OsTypeOther);
}

BareMetalDevice::~BareMetalDevice()
{
    if (IDebugServerProvider *provider = DebugServerProviderManager::findProvider(
                m_debugServerProviderId))
        provider->unregisterDevice(this);
}

QString BareMetalDevice::defaultDisplayName()
{
    return Tr::tr("Bare Metal Device");
}

QString BareMetalDevice::debugServerProviderId() const
{
    return m_debugServerProviderId;
}

void BareMetalDevice::setDebugServerProviderId(const QString &id)
{
    if (id == m_debugServerProviderId)
        return;
    if (IDebugServerProvider *currentProvider =
            DebugServerProviderManager::findProvider(m_debugServerProviderId))
        currentProvider->unregisterDevice(this);
    m_debugServerProviderId = id;
    if (IDebugServerProvider *provider = DebugServerProviderManager::findProvider(id))
        provider->registerDevice(this);
}

void BareMetalDevice::unregisterDebugServerProvider(IDebugServerProvider *provider)
{
    if (provider->id() == m_debugServerProviderId)
        m_debugServerProviderId.clear();
}

void BareMetalDevice::fromMap(const Store &map)
{
    IDevice::fromMap(map);
    QString providerId = map.value(debugServerProviderIdKeyC).toString();
    if (providerId.isEmpty()) {
        const QString name = displayName();
        if (IDebugServerProvider *provider =
                DebugServerProviderManager::findByDisplayName(name)) {
            providerId = provider->id();
            setDebugServerProviderId(providerId);
        }
    } else {
        setDebugServerProviderId(providerId);
    }
}

Store BareMetalDevice::toMap() const
{
    Store map = IDevice::toMap();
    map.insert(debugServerProviderIdKeyC, debugServerProviderId());
    return map;
}

IDeviceWidget *BareMetalDevice::createWidget()
{
    return new BareMetalDeviceConfigurationWidget(sharedFromThis());
}

// Factory

BareMetalDeviceFactory::BareMetalDeviceFactory()
    : IDeviceFactory(Constants::BareMetalOsType)
{
    setDisplayName(Tr::tr("Bare Metal Device"));
    setCombinedIcon(":/baremetal/images/baremetaldevicesmall.png",
                    ":/baremetal/images/baremetaldevice.png");
    setConstructionFunction(&BareMetalDevice::create);
    setCreator([] {
        BareMetalDeviceConfigurationWizard wizard;
        if (wizard.exec() != QDialog::Accepted)
            return IDevice::Ptr();
        return wizard.device();
    });
}

} // BareMetal::Internal
