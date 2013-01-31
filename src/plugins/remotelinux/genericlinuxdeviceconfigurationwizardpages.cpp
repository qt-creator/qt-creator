/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "genericlinuxdeviceconfigurationwizardpages.h"
#include "ui_genericlinuxdeviceconfigurationwizardsetuppage.h"

#include <projectexplorer/devicesupport/idevice.h>

namespace RemoteLinux {
namespace Internal {

class GenericLinuxDeviceConfigurationWizardSetupPagePrivate
{
public:
    Ui::GenericLinuxDeviceConfigurationWizardSetupPage ui;
};

class GenericLinuxDeviceConfigurationWizardFinalPagePrivate
{
public:
    QLabel infoLabel;
};

} // namespace Internal

using namespace QSsh;
using namespace Utils;

GenericLinuxDeviceConfigurationWizardSetupPage::GenericLinuxDeviceConfigurationWizardSetupPage(QWidget *parent) :
    QWizardPage(parent), d(new Internal::GenericLinuxDeviceConfigurationWizardSetupPagePrivate)
{
    d->ui.setupUi(this);
    setTitle(tr("Connection Data"));
    setSubTitle(QLatin1String(" ")); // For Qt bug (background color)
    d->ui.privateKeyPathChooser->setExpectedKind(PathChooser::File);
    connect(d->ui.nameLineEdit, SIGNAL(textChanged(QString)), SIGNAL(completeChanged()));
    connect(d->ui.hostNameLineEdit, SIGNAL(textChanged(QString)), SIGNAL(completeChanged()));
    connect(d->ui.userNameLineEdit, SIGNAL(textChanged(QString)), SIGNAL(completeChanged()));
    connect(d->ui.privateKeyPathChooser, SIGNAL(validChanged()), SIGNAL(completeChanged()));
    connect(d->ui.passwordButton, SIGNAL(toggled(bool)), SLOT(handleAuthTypeChanged()));
}

GenericLinuxDeviceConfigurationWizardSetupPage::~GenericLinuxDeviceConfigurationWizardSetupPage()
{
    delete d;
}

void GenericLinuxDeviceConfigurationWizardSetupPage::initializePage()
{
    d->ui.nameLineEdit->setText(defaultConfigurationName());
    d->ui.hostNameLineEdit->setText(defaultHostName());
    d->ui.userNameLineEdit->setText(defaultUserName());
    d->ui.passwordButton->setChecked(true);
    d->ui.passwordLineEdit->setText(defaultPassWord());
    d->ui.privateKeyPathChooser->setPath(ProjectExplorer::IDevice::defaultPrivateKeyFilePath());
    handleAuthTypeChanged();
}

bool GenericLinuxDeviceConfigurationWizardSetupPage::isComplete() const
{
    return !configurationName().isEmpty() && !hostName().isEmpty() && !userName().isEmpty()
            && (authenticationType() == SshConnectionParameters::AuthenticationByPassword
                || d->ui.privateKeyPathChooser->isValid());
}

QString GenericLinuxDeviceConfigurationWizardSetupPage::configurationName() const
{
    return d->ui.nameLineEdit->text().trimmed();
}

QString GenericLinuxDeviceConfigurationWizardSetupPage::hostName() const
{
    return d->ui.hostNameLineEdit->text().trimmed();
}

QString GenericLinuxDeviceConfigurationWizardSetupPage::userName() const
{
    return d->ui.userNameLineEdit->text().trimmed();
}

SshConnectionParameters::AuthenticationType GenericLinuxDeviceConfigurationWizardSetupPage::authenticationType() const
{
    return d->ui.passwordButton->isChecked()
        ? SshConnectionParameters::AuthenticationByPassword
        : SshConnectionParameters::AuthenticationByKey;
}

QString GenericLinuxDeviceConfigurationWizardSetupPage::password() const
{
    return d->ui.passwordLineEdit->text();
}

QString GenericLinuxDeviceConfigurationWizardSetupPage::privateKeyFilePath() const
{
    return d->ui.privateKeyPathChooser->path();
}

QString GenericLinuxDeviceConfigurationWizardSetupPage::defaultConfigurationName() const
{
    return tr("Generic Linux Device");
}

QString GenericLinuxDeviceConfigurationWizardSetupPage::defaultHostName() const
{
    return QString();
}

QString GenericLinuxDeviceConfigurationWizardSetupPage::defaultUserName() const
{
    return QString();
}

QString GenericLinuxDeviceConfigurationWizardSetupPage::defaultPassWord() const
{
    return QString();
}

void GenericLinuxDeviceConfigurationWizardSetupPage::handleAuthTypeChanged()
{
    d->ui.passwordLineEdit->setEnabled(authenticationType() == SshConnectionParameters::AuthenticationByPassword);
    d->ui.privateKeyPathChooser->setEnabled(authenticationType() == SshConnectionParameters::AuthenticationByKey);
    emit completeChanged();
}


GenericLinuxDeviceConfigurationWizardFinalPage::GenericLinuxDeviceConfigurationWizardFinalPage(QWidget *parent)
    : QWizardPage(parent), d(new Internal::GenericLinuxDeviceConfigurationWizardFinalPagePrivate)
{
    setTitle(tr("Setup Finished"));
    setSubTitle(QLatin1String(" ")); // For Qt bug (background color)
    d->infoLabel.setWordWrap(true);
    QVBoxLayout * const layout = new QVBoxLayout(this);
    layout->addWidget(&d->infoLabel);
}

GenericLinuxDeviceConfigurationWizardFinalPage::~GenericLinuxDeviceConfigurationWizardFinalPage()
{
    delete d;
}

void GenericLinuxDeviceConfigurationWizardFinalPage::initializePage()
{
    d->infoLabel.setText(infoText());
}

QString GenericLinuxDeviceConfigurationWizardFinalPage::infoText() const
{
    return tr("The new device configuration will now be created.\n"
        "In addition, device connectivity will be tested.");
}

} // namespace RemoteLinux
