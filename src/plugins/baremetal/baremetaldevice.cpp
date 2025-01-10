// Copyright (C) 2016 Tim Sander <tim@krieglstein.org>
// Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "baremetaldevice.h"

#include "baremetalconstants.h"
#include "baremetalconstants.h"
#include "baremetaldevice.h"
#include "baremetaldeviceconfigurationwidget.h"
#include "baremetaltr.h"
#include "debugserverproviderchooser.h"
#include "debugserverprovidermanager.h"
#include "idebugserverprovider.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/idevicefactory.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/variablechooser.h>
#include <utils/wizard.h>

#include <QFormLayout>
#include <QLineEdit>
#include <QWizardPage>

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

//  BareMetalDeviceConfigurationWizardSetupPage

class BareMetalDeviceConfigurationWizardSetupPage final : public QWizardPage
{
public:
    explicit BareMetalDeviceConfigurationWizardSetupPage(QWidget *parent)
        : QWizardPage(parent)
    {
        setTitle(Tr::tr("Set up Debug Server or Hardware Debugger"));

        m_nameLineEdit = new QLineEdit(this);

        m_providerChooser = new DebugServerProviderChooser(false, this);
        m_providerChooser->populate();

        const auto formLayout = new QFormLayout(this);
        formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        formLayout->addRow(Tr::tr("Name:"), m_nameLineEdit);
        formLayout->addRow(Tr::tr("Debug server provider:"), m_providerChooser);

        connect(m_nameLineEdit, &QLineEdit::textChanged,
                this, &BareMetalDeviceConfigurationWizardSetupPage::completeChanged);
        connect(m_providerChooser, &DebugServerProviderChooser::providerChanged,
                this, &QWizardPage::completeChanged);
    }

    void initializePage() final
    {
        m_nameLineEdit->setText(BareMetalDevice::defaultDisplayName());
    }

    bool isComplete() const final
    {
        return !configurationName().isEmpty();
    }

    QString configurationName() const { return m_nameLineEdit->text().trimmed(); }
    QString debugServerProviderId() const { return m_providerChooser->currentProviderId(); }

private:
    QLineEdit *m_nameLineEdit = nullptr;
    DebugServerProviderChooser *m_providerChooser = nullptr;
};

//  BareMetalDeviceConfigurationWizardSetupPage

class BareMetalDeviceConfigurationWizard final : public Wizard
{
public:
    BareMetalDeviceConfigurationWizard()
        : m_setupPage(new BareMetalDeviceConfigurationWizardSetupPage(this))
    {
        enum PageId { SetupPageId };

        setWindowTitle(Tr::tr("New Bare Metal Device Configuration Setup"));
        setPage(SetupPageId, m_setupPage);
        m_setupPage->setCommitPage(true);
    }

    IDevicePtr device() const
    {
        const auto dev = BareMetalDevice::create();
        dev->setupId(IDevice::ManuallyAdded, Utils::Id());
        dev->setDefaultDisplayName(m_setupPage->configurationName());
        dev->setType(Constants::BareMetalOsType);
        dev->setMachineType(IDevice::Hardware);
        dev->setDebugServerProviderId(m_setupPage->debugServerProviderId());
        return dev;
    }

private:
    BareMetalDeviceConfigurationWizardSetupPage *m_setupPage = nullptr;
};


// Factory

class BareMetalDeviceFactory final : public IDeviceFactory
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
