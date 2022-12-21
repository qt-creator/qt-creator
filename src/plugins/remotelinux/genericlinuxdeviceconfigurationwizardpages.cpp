// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "genericlinuxdeviceconfigurationwizardpages.h"

#include "publickeydeploymentdialog.h"
#include "remotelinuxtr.h"
#include "sshkeycreationdialog.h"

#include <projectexplorer/devicesupport/sshparameters.h>

#include <utils/fileutils.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/utilsicons.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {
namespace Internal {

class GenericLinuxDeviceConfigurationWizardSetupPagePrivate
{
public:
    QLineEdit *nameLineEdit;
    QLineEdit *hostNameLineEdit;
    QSpinBox *sshPortSpinBox;
    QLineEdit *userNameLineEdit;

    LinuxDevice::Ptr device;
};

class GenericLinuxDeviceConfigurationWizardFinalPagePrivate
{
public:
    QLabel infoLabel;
};

} // namespace Internal

GenericLinuxDeviceConfigurationWizardSetupPage::GenericLinuxDeviceConfigurationWizardSetupPage(
        QWidget *parent) :
    QWizardPage(parent), d(new Internal::GenericLinuxDeviceConfigurationWizardSetupPagePrivate)
{
    setTitle(Tr::tr("Connection"));
    setWindowTitle(Tr::tr("WizardPage"));

    d->nameLineEdit = new QLineEdit(this);
    d->hostNameLineEdit = new QLineEdit(this);
    d->sshPortSpinBox = new QSpinBox(this);
    d->userNameLineEdit = new QLineEdit(this);

    using namespace Layouting;
    Form {
        Tr::tr("The name to identify this configuration:"), d->nameLineEdit, br,
        Tr::tr("The device's host name or IP address:"), d->hostNameLineEdit, st, br,
        Tr::tr("The device's SSH port number:"), d->sshPortSpinBox, st, br,
        Tr::tr("The username to log into the device:"), d->userNameLineEdit, st, br
    }.attachTo(this);

    setSubTitle(QLatin1String(" ")); // For Qt bug (background color)
    connect(d->nameLineEdit, &QLineEdit::textChanged, this, &QWizardPage::completeChanged);
    connect(d->hostNameLineEdit, &QLineEdit::textChanged, this, &QWizardPage::completeChanged);
    connect(d->sshPortSpinBox, &QSpinBox::valueChanged, this, &QWizardPage::completeChanged);
    connect(d->userNameLineEdit, &QLineEdit::textChanged, this, &QWizardPage::completeChanged);
}

GenericLinuxDeviceConfigurationWizardSetupPage::~GenericLinuxDeviceConfigurationWizardSetupPage()
{
    delete d;
}

void GenericLinuxDeviceConfigurationWizardSetupPage::initializePage()
{
    d->nameLineEdit->setText(d->device->displayName());
    d->hostNameLineEdit->setText(d->device->sshParameters().host());
    d->sshPortSpinBox->setValue(22);
    d->sshPortSpinBox->setRange(1, 65535);
    d->userNameLineEdit->setText(d->device->sshParameters().userName());
}

bool GenericLinuxDeviceConfigurationWizardSetupPage::isComplete() const
{
    return !configurationName().isEmpty()
            && !d->hostNameLineEdit->text().trimmed().isEmpty()
            && !d->userNameLineEdit->text().trimmed().isEmpty();
}

bool GenericLinuxDeviceConfigurationWizardSetupPage::validatePage()
{
    d->device->setDisplayName(configurationName());
    SshParameters sshParams = d->device->sshParameters();
    sshParams.url = url();
    d->device->setSshParameters(sshParams);
    return true;
}

QString GenericLinuxDeviceConfigurationWizardSetupPage::configurationName() const
{
    return d->nameLineEdit->text().trimmed();
}

QUrl GenericLinuxDeviceConfigurationWizardSetupPage::url() const
{
    QUrl url;
    url.setHost(d->hostNameLineEdit->text().trimmed());
    url.setUserName(d->userNameLineEdit->text().trimmed());
    url.setPort(d->sshPortSpinBox->value());
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
    setTitle(Tr::tr("Summary"));
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
    return Tr::tr("The new device configuration will now be created.\n"
                  "In addition, device connectivity will be tested.");
}

struct GenericLinuxDeviceConfigurationWizardKeyDeploymentPage::Private
{
    FilePaths defaultKeys() const
    {
        const FilePath baseDir = FileUtils::homePath() / ".ssh";
        return {baseDir / "id_rsa", baseDir / "id_ecdsa", baseDir / "id_ed25519"};
    }

    PathChooser keyFileChooser;
    QLabel iconLabel;
    LinuxDevice::Ptr device;
};

GenericLinuxDeviceConfigurationWizardKeyDeploymentPage::GenericLinuxDeviceConfigurationWizardKeyDeploymentPage(QWidget *parent)
    : QWizardPage(parent), d(new Private)
{
    setTitle(Tr::tr("Key Deployment"));
    setSubTitle(" ");
    const QString info = Tr::tr("We recommend that you log into your device using public key "
                                "authentication.\n"
                                "If your device is already set up for this, you do not have to do "
                                "anything here.\n"
                                "Otherwise, please deploy the public key for the private key "
                                "with which to connect in the future.\n"
                                "If you do not have a private key yet, you can also "
                                "create one here.");
    d->keyFileChooser.setExpectedKind(PathChooser::File);
    d->keyFileChooser.setHistoryCompleter("Ssh.KeyFile.History");
    d->keyFileChooser.setPromptDialogTitle(Tr::tr("Choose a Private Key File"));
    auto const deployButton = new QPushButton(Tr::tr("Deploy Public Key"), this);
    connect(deployButton, &QPushButton::clicked,
            this, &GenericLinuxDeviceConfigurationWizardKeyDeploymentPage::deployKey);
    auto const createButton = new QPushButton(Tr::tr("Create New Key Pair"), this);
    connect(createButton, &QPushButton::clicked,
            this, &GenericLinuxDeviceConfigurationWizardKeyDeploymentPage::createKey);
    auto const mainLayout = new QVBoxLayout(this);
    auto const keyLayout = new QHBoxLayout;
    auto const deployLayout = new QHBoxLayout;
    mainLayout->addWidget(new QLabel(info));
    keyLayout->addWidget(new QLabel(Tr::tr("Private key file:")));
    keyLayout->addWidget(&d->keyFileChooser);
    keyLayout->addWidget(createButton);
    keyLayout->addStretch();
    mainLayout->addLayout(keyLayout);
    deployLayout->addWidget(deployButton);
    deployLayout->addWidget(&d->iconLabel);
    deployLayout->addStretch();
    mainLayout->addLayout(deployLayout);
    connect(&d->keyFileChooser, &PathChooser::textChanged, this, [this, deployButton] {
        deployButton->setEnabled(d->keyFileChooser.filePath().exists());
        d->iconLabel.clear();
        emit completeChanged();
    });
    for (const FilePath &defaultKey : d->defaultKeys()) {
        if (defaultKey.exists()) {
            d->keyFileChooser.setFilePath(defaultKey);
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
        d->keyFileChooser.setFilePath(privateKeyFilePath());
    d->iconLabel.clear();
}

bool GenericLinuxDeviceConfigurationWizardKeyDeploymentPage::isComplete() const
{
    return d->keyFileChooser.filePath().toString().isEmpty() || d->keyFileChooser.filePath().exists();
}

bool GenericLinuxDeviceConfigurationWizardKeyDeploymentPage::validatePage()
{
    if (!d->defaultKeys().contains(d->keyFileChooser.filePath())) {
        SshParameters sshParams = d->device->sshParameters();
        sshParams.authenticationType = SshParameters::AuthenticationTypeSpecificKey;
        sshParams.privateKeyFile = d->keyFileChooser.filePath();
        d->device->setSshParameters(sshParams);
    }
    return true;
}

FilePath GenericLinuxDeviceConfigurationWizardKeyDeploymentPage::privateKeyFilePath() const
{
    return d->keyFileChooser.filePath();
}

void GenericLinuxDeviceConfigurationWizardKeyDeploymentPage::createKey()
{
    SshKeyCreationDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted)
        d->keyFileChooser.setFilePath(dlg.privateKeyFilePath());
}

void GenericLinuxDeviceConfigurationWizardKeyDeploymentPage::deployKey()
{
    PublicKeyDeploymentDialog dlg(d->device, privateKeyFilePath().stringAppended(".pub"), this);
    d->iconLabel.setPixmap((dlg.exec() == QDialog::Accepted ? Icons::OK : Icons::BROKEN).pixmap());
}

} // namespace RemoteLinux
