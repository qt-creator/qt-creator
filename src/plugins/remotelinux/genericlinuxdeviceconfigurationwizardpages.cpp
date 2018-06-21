/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
    setTitle(tr("Connection"));
    setSubTitle(QLatin1String(" ")); // For Qt bug (background color)
    d->ui.privateKeyPathChooser->setExpectedKind(PathChooser::File);
    d->ui.privateKeyPathChooser->setHistoryCompleter(QLatin1String("Ssh.KeyFile.History"));
    d->ui.privateKeyPathChooser->setPromptDialogTitle(tr("Choose a Private Key File"));
    connect(d->ui.nameLineEdit, &QLineEdit::textChanged, this, &QWizardPage::completeChanged);
    connect(d->ui.hostNameLineEdit, &QLineEdit::textChanged, this, &QWizardPage::completeChanged);
    connect(d->ui.userNameLineEdit, &QLineEdit::textChanged, this, &QWizardPage::completeChanged);
    connect(d->ui.privateKeyPathChooser, &PathChooser::validChanged,
            this, &QWizardPage::completeChanged);
    connect(d->ui.passwordButton, &QAbstractButton::toggled,
            this, &GenericLinuxDeviceConfigurationWizardSetupPage::handleAuthTypeChanged);
    connect(d->ui.keyButton, &QAbstractButton::toggled,
            this, &GenericLinuxDeviceConfigurationWizardSetupPage::handleAuthTypeChanged);
    connect(d->ui.agentButton, &QAbstractButton::toggled,
            this, &GenericLinuxDeviceConfigurationWizardSetupPage::handleAuthTypeChanged);
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
    return !configurationName().isEmpty()
            && !d->ui.hostNameLineEdit->text().trimmed().isEmpty()
            && !d->ui.userNameLineEdit->text().trimmed().isEmpty()
            && (authenticationType() != SshConnectionParameters::AuthenticationTypePublicKey
                || d->ui.privateKeyPathChooser->isValid());
}

QString GenericLinuxDeviceConfigurationWizardSetupPage::configurationName() const
{
    return d->ui.nameLineEdit->text().trimmed();
}

QUrl GenericLinuxDeviceConfigurationWizardSetupPage::url() const
{
    QUrl url;
    url.setHost(d->ui.hostNameLineEdit->text().trimmed());
    url.setUserName(d->ui.userNameLineEdit->text().trimmed());
    url.setPassword(d->ui.passwordLineEdit->text());
    url.setPort(22);
    return url;
}

SshConnectionParameters::AuthenticationType GenericLinuxDeviceConfigurationWizardSetupPage::authenticationType() const
{
    return d->ui.passwordButton->isChecked()
            ? SshConnectionParameters::AuthenticationTypeTryAllPasswordBasedMethods
            : d->ui.keyButton->isChecked() ? SshConnectionParameters::AuthenticationTypePublicKey
                                           : SshConnectionParameters::AuthenticationTypeAgent;
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
    d->ui.passwordLineEdit->setEnabled(authenticationType()
            == SshConnectionParameters::AuthenticationTypeTryAllPasswordBasedMethods);
    d->ui.privateKeyPathChooser->setEnabled(authenticationType()
            == SshConnectionParameters::AuthenticationTypePublicKey);
    emit completeChanged();
}


GenericLinuxDeviceConfigurationWizardFinalPage::GenericLinuxDeviceConfigurationWizardFinalPage(QWidget *parent)
    : QWizardPage(parent), d(new Internal::GenericLinuxDeviceConfigurationWizardFinalPagePrivate)
{
    setTitle(tr("Summary"));
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
