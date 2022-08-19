// Copyright (C) 2016 Tim Sander <tim@krieglstein.org>
// Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "baremetaldevice.h"
#include "baremetaldeviceconfigurationwizardpages.h"

#include "debugserverproviderchooser.h"

#include <utils/variablechooser.h>
#include <utils/fileutils.h>
#include <projectexplorer/devicesupport/idevice.h>

#include <QFormLayout>
#include <QLineEdit>

namespace BareMetal {
namespace Internal {

// BareMetalDeviceConfigurationWizardSetupPage

BareMetalDeviceConfigurationWizardSetupPage::BareMetalDeviceConfigurationWizardSetupPage(
        QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Set up Debug Server or Hardware Debugger"));

    const auto formLayout = new QFormLayout(this);
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    m_nameLineEdit = new QLineEdit(this);
    formLayout->addRow(tr("Name:"), m_nameLineEdit);
    m_debugServerProviderChooser = new DebugServerProviderChooser(false, this);
    m_debugServerProviderChooser->populate();
    formLayout->addRow(tr("Debug server provider:"), m_debugServerProviderChooser);

    connect(m_nameLineEdit, &QLineEdit::textChanged,
            this, &BareMetalDeviceConfigurationWizardSetupPage::completeChanged);
    connect(m_debugServerProviderChooser, &DebugServerProviderChooser::providerChanged,
            this, &QWizardPage::completeChanged);
}

void BareMetalDeviceConfigurationWizardSetupPage::initializePage()
{
    m_nameLineEdit->setText(BareMetalDevice::defaultDisplayName());
}

bool BareMetalDeviceConfigurationWizardSetupPage::isComplete() const
{
    return !configurationName().isEmpty();
}

QString BareMetalDeviceConfigurationWizardSetupPage::configurationName() const
{
    return m_nameLineEdit->text().trimmed();
}

QString BareMetalDeviceConfigurationWizardSetupPage::debugServerProviderId() const
{
   return m_debugServerProviderChooser->currentProviderId();
}

} // namespace Internal
} // namespace BareMetal
