/****************************************************************************
**
** Copyright (C) 2015 Tim Sander <tim@krieglstein.org>
** Copyright (C) 2015 Denis Shienkov <denis.shienkov@gmail.com>
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

    connect(m_nameLineEdit, SIGNAL(textChanged(QString)),
            SIGNAL(completeChanged()));
    connect(m_gdbServerProviderChooser.data(), &GdbServerProviderChooser::providerChanged,
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
