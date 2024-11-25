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

#include <projectexplorer/devicesupport/idevicefactory.h>

#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace BareMetal::Internal {

// BareMetalDevice

BareMetalDevice::BareMetalDevice()
{
    setDisplayType(Tr::tr("Bare Metal"));
    setOsType(Utils::OsTypeOther);

    m_debugServerProviderId.setSettingsKey("IDebugServerProviderId");
}

BareMetalDevice::~BareMetalDevice()
{
    if (IDebugServerProvider *provider = DebugServerProviderManager::findProvider(
                debugServerProviderId()))
        provider->unregisterDevice(this);
}

QString BareMetalDevice::defaultDisplayName()
{
    return Tr::tr("Bare Metal Device");
}

QString BareMetalDevice::debugServerProviderId() const
{
    return m_debugServerProviderId();
}

void BareMetalDevice::setDebugServerProviderId(const QString &id)
{
    if (id == debugServerProviderId())
        return;
    if (IDebugServerProvider *currentProvider =
            DebugServerProviderManager::findProvider(debugServerProviderId()))
        currentProvider->unregisterDevice(this);
    m_debugServerProviderId.setValue(id);
    if (IDebugServerProvider *provider = DebugServerProviderManager::findProvider(id))
        provider->registerDevice(this);
}

void BareMetalDevice::unregisterDebugServerProvider(IDebugServerProvider *provider)
{
    if (provider->id() == debugServerProviderId())
        m_debugServerProviderId.setValue(QString());
}

void BareMetalDevice::fromMap(const Store &map)
{
    IDevice::fromMap(map);

    if (debugServerProviderId().isEmpty()) {
        const QString name = displayName();
        if (IDebugServerProvider *provider =
                DebugServerProviderManager::findByDisplayName(name)) {
            setDebugServerProviderId(provider->id());
        }
    }
}

IDeviceWidget *BareMetalDevice::createWidget()
{
    return new BareMetalDeviceConfigurationWidget(shared_from_this());
}

// Factory

class BareMetalDeviceFactory final : public ProjectExplorer::IDeviceFactory
{
public:
    BareMetalDeviceFactory()
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
};

void setupBareMetalDevice()
{
    static BareMetalDeviceFactory theBareMetalDeviceFactory;
}

} // BareMetal::Internal
