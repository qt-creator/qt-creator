/****************************************************************************
**
** Copyright (C) 2016 Tim Sander <tim@krieglstein.org>
** Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
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

#include "baremetaldeviceconfigurationwizardpages.h"
#include "baremetaldevice.h"

#include "gdbserverproviderchooser.h"

#include <coreplugin/variablechooser.h>
#include <projectexplorer/devicesupport/idevice.h>

#include <QFormLayout>
#include <QLineEdit>

namespace BareMetal {
namespace Internal {

BareMetalDeviceConfigurationWizardSetupPage::BareMetalDeviceConfigurationWizardSetupPage(
        QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Set up GDB Server or Hardware Debugger"));

    auto formLayout = new QFormLayout(this);
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    m_nameLineEdit = new QLineEdit(this);
    formLayout->addRow(tr("Name:"), m_nameLineEdit);
    m_gdbServerProviderChooser = new GdbServerProviderChooser(false, this);
    m_gdbServerProviderChooser->populate();
    formLayout->addRow(tr("GDB server provider:"), m_gdbServerProviderChooser);

    connect(m_nameLineEdit, &QLineEdit::textChanged,
            this, &BareMetalDeviceConfigurationWizardSetupPage::completeChanged);
    connect(m_gdbServerProviderChooser, &GdbServerProviderChooser::providerChanged,
            this, &QWizardPage::completeChanged);
}

void BareMetalDeviceConfigurationWizardSetupPage::initializePage()
{
    m_nameLineEdit->setText(defaultConfigurationName());
}

bool BareMetalDeviceConfigurationWizardSetupPage::isComplete() const
{
    return !configurationName().isEmpty();
}

QString BareMetalDeviceConfigurationWizardSetupPage::configurationName() const
{
    return m_nameLineEdit->text().trimmed();
}

QString BareMetalDeviceConfigurationWizardSetupPage::gdbServerProviderId() const
{
   return m_gdbServerProviderChooser->currentProviderId();
}

QString BareMetalDeviceConfigurationWizardSetupPage::defaultConfigurationName() const
{
    return tr("Bare Metal Device");
}

} // namespace Internal
} // namespace BareMetal
