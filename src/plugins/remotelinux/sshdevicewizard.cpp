// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sshdevicewizard.h"

#include "publickeydeploymentdialog.h"
#include "remotelinuxtr.h"
#include "sshkeycreationdialog.h"

#include <coreplugin/icore.h>

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/sshparameters.h>

#include <utils/fancylineedit.h>
#include <utils/fileutils.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/utilsicons.h>

#include <QLabel>
#include <QPushButton>
#include <QSpinBox>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {

class SetupPage : public QWizardPage
{
public:
    explicit SetupPage(const ProjectExplorer::IDevicePtr &device)
        : m_device(device)
    {
        setTitle(Tr::tr("Connection"));
        setWindowTitle(Tr::tr("WizardPage"));

        m_nameLineEdit = new FancyLineEdit(this);
        m_nameLineEdit->setHistoryCompleter("DeviceName");

        m_hostNameLineEdit = new FancyLineEdit(this);
        m_hostNameLineEdit->setHistoryCompleter("HostName");

        m_sshPortSpinBox = new QSpinBox(this);

        m_userNameLineEdit = new FancyLineEdit(this);
        m_userNameLineEdit->setHistoryCompleter("UserName");

        using namespace Layouting;
        Form {
            Tr::tr("The name to identify this configuration:"), m_nameLineEdit, br,
            Tr::tr("The device's host name or IP address:"), m_hostNameLineEdit, st, br,
            Tr::tr("The device's SSH port number:"), m_sshPortSpinBox, st, br,
            Tr::tr("The username to log into the device:"), m_userNameLineEdit, st, br
        }.attachTo(this);

        setSubTitle(QLatin1String(" ")); // For Qt bug (background color)
        connect(m_nameLineEdit, &QLineEdit::textChanged, this, &QWizardPage::completeChanged);
        connect(m_hostNameLineEdit, &QLineEdit::textChanged, this, &QWizardPage::completeChanged);
        connect(m_sshPortSpinBox, &QSpinBox::valueChanged, this, &QWizardPage::completeChanged);
        connect(m_userNameLineEdit, &QLineEdit::textChanged, this, &QWizardPage::completeChanged);
    }

private:
    void initializePage() final {
        m_nameLineEdit->setText(m_device->displayName());
        m_hostNameLineEdit->setText(m_device->sshParameters().host());
        m_sshPortSpinBox->setValue(22);
        m_sshPortSpinBox->setRange(1, 65535);
        m_userNameLineEdit->setText(m_device->sshParameters().userName());
    }
    bool isComplete() const final {
        return !m_nameLineEdit->text().trimmed().isEmpty()
               && !m_hostNameLineEdit->text().trimmed().isEmpty()
               && !m_userNameLineEdit->text().trimmed().isEmpty();
    }
    bool validatePage() final {
        m_device->settings()->displayName.setValue(m_nameLineEdit->text().trimmed());
        SshParameters sshParams = m_device->sshParameters();
        sshParams.setHost(m_hostNameLineEdit->text().trimmed());
        sshParams.setUserName(m_userNameLineEdit->text().trimmed());
        sshParams.setPort(m_sshPortSpinBox->value());
        m_device->setSshParameters(sshParams);
        return true;
    }

    FancyLineEdit *m_nameLineEdit;
    FancyLineEdit *m_hostNameLineEdit;
    QSpinBox *m_sshPortSpinBox;
    FancyLineEdit *m_userNameLineEdit;
    IDevicePtr m_device;
};

class KeyDeploymentPage : public QWizardPage
{
public:
    explicit KeyDeploymentPage(const ProjectExplorer::IDevicePtr &device)
        : m_device(device)
    {
        setTitle(Tr::tr("Key Deployment"));
        setSubTitle(" ");
        const QString info = Tr::tr(
            "We recommend that you log into your device using public key authentication.\n"
            "If your device is already set up for this, you do not have to do anything here.\n"
            "Otherwise, please deploy the public key for the private key "
            "with which to connect in the future.\n"
            "If you do not have a private key yet, you can also create one here.");
        m_keyFileChooser.setExpectedKind(PathChooser::File);
        m_keyFileChooser.setHistoryCompleter("Ssh.KeyFile.History");
        m_keyFileChooser.setPromptDialogTitle(Tr::tr("Choose a Private Key File"));
        auto const deployButton = new QPushButton(Tr::tr("Deploy Public Key"), this);
        connect(deployButton, &QPushButton::clicked, this, [this] {
            Internal::PublicKeyDeploymentDialog dlg(
                m_device, m_keyFileChooser.filePath().stringAppended(".pub"), this);
            m_iconLabel.setPixmap((dlg.exec() == QDialog::Accepted ? Icons::OK : Icons::BROKEN).pixmap());
        });
        auto const createButton = new QPushButton(Tr::tr("Create New Key Pair"), this);
        connect(createButton, &QPushButton::clicked, this, [this] {
            SshKeyCreationDialog dlg(this);
            if (dlg.exec() == QDialog::Accepted)
                m_keyFileChooser.setFilePath(dlg.privateKeyFilePath());
        });
        auto const mainLayout = new QVBoxLayout(this);
        auto const keyLayout = new QHBoxLayout;
        auto const deployLayout = new QHBoxLayout;
        mainLayout->addWidget(new QLabel(info));
        keyLayout->addWidget(new QLabel(Tr::tr("Private key file:")));
        keyLayout->addWidget(&m_keyFileChooser);
        keyLayout->addWidget(createButton);
        keyLayout->addStretch();
        mainLayout->addLayout(keyLayout);
        deployLayout->addWidget(deployButton);
        deployLayout->addWidget(&m_iconLabel);
        deployLayout->addStretch();
        mainLayout->addLayout(deployLayout);
        connect(&m_keyFileChooser, &PathChooser::textChanged, this, [this, deployButton] {
            deployButton->setEnabled(m_keyFileChooser.filePath().exists());
            m_iconLabel.clear();
            emit completeChanged();
        });
        for (const FilePath &defaultKey : defaultKeys()) {
            if (defaultKey.exists()) {
                m_keyFileChooser.setFilePath(defaultKey);
                break;
            }
        }
    }

private:
    void initializePage() final {
        if (!m_device->sshParameters().privateKeyFile.isEmpty())
            m_keyFileChooser.setFilePath(m_keyFileChooser.filePath());
        m_iconLabel.clear();
    }
    bool isComplete() const final {
        return m_keyFileChooser.filePath().toString().isEmpty()
               || m_keyFileChooser.filePath().exists();
    }
    bool validatePage() final {
        if (!defaultKeys().contains(m_keyFileChooser.filePath())) {
            SshParameters sshParams = m_device->sshParameters();
            sshParams.authenticationType = SshParameters::AuthenticationTypeSpecificKey;
            sshParams.privateKeyFile = m_keyFileChooser.filePath();
            m_device->setSshParameters(sshParams);
        }
        return true;
    }
    FilePaths defaultKeys() const {
        const FilePath baseDir = FileUtils::homePath() / ".ssh";
        return {baseDir / "id_rsa", baseDir / "id_ecdsa", baseDir / "id_ed25519"};
    }

    PathChooser m_keyFileChooser;
    QLabel m_iconLabel;
    IDevicePtr m_device;
};

class FinalPage final : public QWizardPage
{
public:
    FinalPage()
    {
        setTitle(Tr::tr("Summary"));
        setSubTitle(QLatin1String(" ")); // For Qt bug (background color)
        auto infoLabel = new QLabel(Tr::tr("The new device configuration will now be created.\n"
                                           "In addition, device connectivity will be tested."));
        infoLabel->setWordWrap(true);
        auto layout = new QVBoxLayout(this);
        layout->addWidget(infoLabel);
        setCommitPage(true);
    }
};

SshDeviceWizard::SshDeviceWizard(const QString &title, const ProjectExplorer::IDevicePtr &device)
    : Wizard(Core::ICore::dialogParent())
{
    setWindowTitle(title);

    addPage(new SetupPage(device));
    addPage(new KeyDeploymentPage(device));
    addPage(new FinalPage);
}

} // namespace RemoteLinux
