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

#include "publickeydeploymentdialog.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <ssh/sshkeycreationdialog.h>
#include <utils/utilsicons.h>
#include <utils/pathchooser.h>

#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStringList>
#include <QVBoxLayout>

namespace RemoteLinux {
namespace Internal {

class GenericLinuxDeviceConfigurationWizardSetupPagePrivate
{
public:
    Ui::GenericLinuxDeviceConfigurationWizardSetupPage ui;
    LinuxDevice::Ptr device;
};

class GenericLinuxDeviceConfigurationWizardFinalPagePrivate
{
public:
    QLabel infoLabel;
};

} // namespace Internal

using namespace QSsh;
using namespace Utils;

GenericLinuxDeviceConfigurationWizardSetupPage::GenericLinuxDeviceConfigurationWizardSetupPage(
        QWidget *parent) :
    QWizardPage(parent), d(new Internal::GenericLinuxDeviceConfigurationWizardSetupPagePrivate)
{
    d->ui.setupUi(this);
    setTitle(tr("Connection"));
    setSubTitle(QLatin1String(" ")); // For Qt bug (background color)
    connect(d->ui.nameLineEdit, &QLineEdit::textChanged, this, &QWizardPage::completeChanged);
    connect(d->ui.hostNameLineEdit, &QLineEdit::textChanged, this, &QWizardPage::completeChanged);
    connect(d->ui.userNameLineEdit, &QLineEdit::textChanged, this, &QWizardPage::completeChanged);
}

GenericLinuxDeviceConfigurationWizardSetupPage::~GenericLinuxDeviceConfigurationWizardSetupPage()
{
    delete d;
}

void GenericLinuxDeviceConfigurationWizardSetupPage::initializePage()
{
    d->ui.nameLineEdit->setText(d->device->displayName());
    d->ui.hostNameLineEdit->setText(d->device->sshParameters().host());
    d->ui.userNameLineEdit->setText(d->device->sshParameters().userName());
}

bool GenericLinuxDeviceConfigurationWizardSetupPage::isComplete() const
{
    return !configurationName().isEmpty()
            && !d->ui.hostNameLineEdit->text().trimmed().isEmpty()
            && !d->ui.userNameLineEdit->text().trimmed().isEmpty();
}

bool GenericLinuxDeviceConfigurationWizardSetupPage::validatePage()
{
    d->device->setDisplayName(configurationName());
    SshConnectionParameters sshParams = d->device->sshParameters();
    sshParams.url = url();
    d->device->setSshParameters(sshParams);
    return true;
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
    url.setPort(22);
    return url;
}

void GenericLinuxDeviceConfigurationWizardSetupPage::setDevice(const LinuxDevice::Ptr &device)
{
    d->device = device;
}

GenericLinuxDeviceConfigurationWizardFinalPage::GenericLinuxDeviceConfigurationWizardFinalPage(
        QWidget *parent)
    : QWizardPage(parent), d(new Internal::GenericLinuxDeviceConfigurationWizardFinalPagePrivate)
{
    setTitle(tr("Summary"));
    setSubTitle(QLatin1String(" ")); // For Qt bug (background color)
    d->infoLabel.setWordWrap(true);
    auto const layout = new QVBoxLayout(this);
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

struct GenericLinuxDeviceConfigurationWizardKeyDeploymentPage::Private
{
    QStringList defaultKeys() const
    {
        const QString baseDir = QDir::homePath() + "/.ssh";
        return QStringList{baseDir + "/id_rsa", baseDir + "/id_ecdsa", baseDir + "/id_ed25519"};
    }

    PathChooser keyFileChooser;
    QLabel iconLabel;
    LinuxDevice::Ptr device;
};

GenericLinuxDeviceConfigurationWizardKeyDeploymentPage::GenericLinuxDeviceConfigurationWizardKeyDeploymentPage(QWidget *parent)
    : QWizardPage(parent), d(new Private)
{
    setTitle(tr("Key Deployment"));
    setSubTitle(" ");
    const QString info = tr("We recommend that you log into your device using public key "
                            "authentication.\n"
                            "If your device is already set up for this, you do not have to do "
                            "anything here.\n"
                            "Otherwise, please deploy the public key for the private key "
                            "with which to connect in the future.\n"
                            "If you do not have a private key yet, you can also "
                            "create one here.");
    d->keyFileChooser.setExpectedKind(PathChooser::File);
    d->keyFileChooser.setHistoryCompleter("Ssh.KeyFile.History");
    d->keyFileChooser.setPromptDialogTitle(tr("Choose a Private Key File"));
    auto const deployButton = new QPushButton(tr("Deploy Public Key"), this);
    connect(deployButton, &QPushButton::clicked,
            this, &GenericLinuxDeviceConfigurationWizardKeyDeploymentPage::deployKey);
    auto const createButton = new QPushButton(tr("Create New Key Pair"), this);
    connect(createButton, &QPushButton::clicked,
            this, &GenericLinuxDeviceConfigurationWizardKeyDeploymentPage::createKey);
    auto const mainLayout = new QVBoxLayout(this);
    auto const keyLayout = new QHBoxLayout;
    auto const deployLayout = new QHBoxLayout;
    mainLayout->addWidget(new QLabel(info));
    keyLayout->addWidget(new QLabel(tr("Private key file:")));
    keyLayout->addWidget(&d->keyFileChooser);
    keyLayout->addWidget(createButton);
    keyLayout->addStretch();
    mainLayout->addLayout(keyLayout);
    deployLayout->addWidget(deployButton);
    deployLayout->addWidget(&d->iconLabel);
    deployLayout->addStretch();
    mainLayout->addLayout(deployLayout);
    connect(&d->keyFileChooser, &PathChooser::pathChanged, this, [this, deployButton] {
        deployButton->setEnabled(d->keyFileChooser.filePath().exists());
        d->iconLabel.clear();
        emit completeChanged();
    });
    for (const QString &defaultKey : d->defaultKeys()) {
        if (QFileInfo::exists(defaultKey)) {
            d->keyFileChooser.setPath(defaultKey);
            break;
        }
    }
}

GenericLinuxDeviceConfigurationWizardKeyDeploymentPage::~GenericLinuxDeviceConfigurationWizardKeyDeploymentPage()
{
    delete d;
}

void GenericLinuxDeviceConfigurationWizardKeyDeploymentPage::setDevice(const LinuxDevice::Ptr &device)
{
    d->device = device;
}

void GenericLinuxDeviceConfigurationWizardKeyDeploymentPage::initializePage()
{
    if (!d->device->sshParameters().privateKeyFile.isEmpty())
        d->keyFileChooser.setPath(privateKeyFilePath());
    d->iconLabel.clear();
}

bool GenericLinuxDeviceConfigurationWizardKeyDeploymentPage::isComplete() const
{
    return d->keyFileChooser.filePath().toString().isEmpty() || d->keyFileChooser.filePath().exists();
}

bool GenericLinuxDeviceConfigurationWizardKeyDeploymentPage::validatePage()
{
    if (!d->defaultKeys().contains(d->keyFileChooser.filePath().toString())) {
        SshConnectionParameters sshParams = d->device->sshParameters();
        sshParams.authenticationType = SshConnectionParameters::AuthenticationTypeSpecificKey;
        sshParams.privateKeyFile = d->keyFileChooser.filePath().toString();
        d->device->setSshParameters(sshParams);
    }
    return true;
}

QString GenericLinuxDeviceConfigurationWizardKeyDeploymentPage::privateKeyFilePath() const
{
    return d->keyFileChooser.filePath().toString();
}

void GenericLinuxDeviceConfigurationWizardKeyDeploymentPage::createKey()
{
    SshKeyCreationDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted)
        d->keyFileChooser.setPath(dlg.privateKeyFilePath());
}

void GenericLinuxDeviceConfigurationWizardKeyDeploymentPage::deployKey()
{
    PublicKeyDeploymentDialog dlg(d->device, privateKeyFilePath() + ".pub", this);
    d->iconLabel.setPixmap((dlg.exec() == QDialog::Accepted ? Icons::OK : Icons::BROKEN).pixmap());
}

} // namespace RemoteLinux
