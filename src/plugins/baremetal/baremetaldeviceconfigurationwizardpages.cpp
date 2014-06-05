/****************************************************************************
**
** Copyright (C) 2014 Tim Sander <tim@krieglstein.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "baremetaldeviceconfigurationwizardpages.h"
#include "ui_baremetaldeviceconfigurationwizardsetuppage.h"

#include <coreplugin/variablechooser.h>
#include <projectexplorer/devicesupport/idevice.h>

namespace BareMetal {
namespace Internal {
class BareMetalDeviceConfigurationWizardSetupPagePrivate;
} // namespace Internal

BareMetalDeviceConfigurationWizardSetupPage::BareMetalDeviceConfigurationWizardSetupPage(QWidget *parent) :
    QWizardPage(parent), d(new Internal::BareMetalDeviceConfigurationWizardSetupPagePrivate)
{
    d->ui.setupUi(this);
    setTitle(tr("Set up GDB Server or Hardware Debugger"));
    setSubTitle(QLatin1String(" ")); // For Qt bug (background color)
    connect(d->ui.hostNameLineEdit, SIGNAL(textChanged(QString)), SIGNAL(completeChanged()));
    connect(d->ui.nameLineEdit, SIGNAL(textChanged(QString)), SIGNAL(completeChanged()));
    connect(d->ui.portSpinBox, SIGNAL(valueChanged(int)), SIGNAL(completeChanged()));
    connect(d->ui.gdbInitCommandsPlainTextEdit, SIGNAL(textChanged()), SIGNAL(completeChanged()));
    Core::VariableChooser::addVariableSupport(d->ui.gdbInitCommandsPlainTextEdit);
    new Core::VariableChooser(this);
}

BareMetalDeviceConfigurationWizardSetupPage::~BareMetalDeviceConfigurationWizardSetupPage()
{
    delete d;
}

void BareMetalDeviceConfigurationWizardSetupPage::initializePage()
{
    d->ui.nameLineEdit->setText(defaultConfigurationName());

}

bool BareMetalDeviceConfigurationWizardSetupPage::isComplete() const
{
    return !configurationName().isEmpty();
}

QString BareMetalDeviceConfigurationWizardSetupPage::configurationName() const
{
    return d->ui.nameLineEdit->text().trimmed();
}

QString BareMetalDeviceConfigurationWizardSetupPage::gdbHostname() const
{
    return d->ui.hostNameLineEdit->text().trimmed();
}

quint16 BareMetalDeviceConfigurationWizardSetupPage::gdbPort() const
{
    return quint16(d->ui.portSpinBox->value());
}

QString BareMetalDeviceConfigurationWizardSetupPage::gdbInitCommands() const
{
    return d->ui.gdbInitCommandsPlainTextEdit->toPlainText();
}

QString BareMetalDeviceConfigurationWizardSetupPage::defaultConfigurationName() const
{
    return tr("Bare Metal Device");
}

} // namespace BareMetal
