/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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

#include "blackberrydeviceconfigurationwizardpages.h"
#include "ui_blackberrydeviceconfigurationwizardsetuppage.h"
#include "ui_blackberrydeviceconfigurationwizardsshkeypage.h"

#include <coreplugin/icore.h>
#include <ssh/sshkeygenerator.h>

#include <QFormLayout>
#include <QMessageBox>

using namespace ProjectExplorer;
using namespace Qnx;
using namespace Qnx::Internal;

namespace {
const char DEVICENAME_FIELD_ID[] = "DeviceName";

QString defaultDeviceHostIp(IDevice::MachineType type)
{
    return type == IDevice::Hardware ? QLatin1String("169.254.0.1") : QString();
}
}

BlackBerryDeviceConfigurationWizardSetupPage::BlackBerryDeviceConfigurationWizardSetupPage(QWidget *parent)
    : QWizardPage(parent)
    , m_ui(new Ui::BlackBerryDeviceConfigurationWizardSetupPage)
{
    m_ui->setupUi(this);
    setTitle(tr("Connection Details"));

    m_ui->debugToken->setExpectedKind(Utils::PathChooser::File);
    m_ui->debugToken->setPromptDialogFilter(QLatin1String("*.bar"));

    connect(m_ui->deviceName, SIGNAL(textChanged(QString)), this, SIGNAL(completeChanged()));
    connect(m_ui->deviceHostIp, SIGNAL(textChanged(QString)), this, SIGNAL(completeChanged()));
    connect(m_ui->physicalDevice, SIGNAL(toggled(bool)), this, SLOT(handleMachineTypeChanged()));
    connect(m_ui->debugToken, SIGNAL(changed(QString)), this, SIGNAL(completeChanged()));

    registerField(QLatin1String(DEVICENAME_FIELD_ID), m_ui->deviceName);
}

BlackBerryDeviceConfigurationWizardSetupPage::~BlackBerryDeviceConfigurationWizardSetupPage()
{
    delete m_ui;
    m_ui = 0;
}

void BlackBerryDeviceConfigurationWizardSetupPage::initializePage()
{
    m_ui->deviceName->setText(tr("BlackBerry Device"));
    m_ui->password->setText(QString());
    m_ui->physicalDevice->setChecked(true);
    m_ui->deviceHostIp->setText(defaultDeviceHostIp(machineType()));
}

bool BlackBerryDeviceConfigurationWizardSetupPage::isComplete() const
{
    bool debugTokenComplete = m_ui->simulator->isChecked()
            || (m_ui->physicalDevice->isChecked() && !m_ui->debugToken->fileName().isEmpty()
                && QFileInfo(m_ui->debugToken->fileName().toString()).exists());

    return !m_ui->deviceHostIp->text().isEmpty() && !m_ui->deviceHostIp->text().isEmpty()
            && debugTokenComplete;
}

QString BlackBerryDeviceConfigurationWizardSetupPage::deviceName() const
{
    return m_ui->deviceName->text();
}

QString BlackBerryDeviceConfigurationWizardSetupPage::hostName() const
{
    return m_ui->deviceHostIp->text();
}

QString BlackBerryDeviceConfigurationWizardSetupPage::password() const
{
    return m_ui->password->text();
}

QString BlackBerryDeviceConfigurationWizardSetupPage::debugToken() const
{
    return m_ui->debugToken->fileName().toString();
}

IDevice::MachineType BlackBerryDeviceConfigurationWizardSetupPage::machineType() const
{
    return m_ui->physicalDevice->isChecked() ? IDevice::Hardware : IDevice::Emulator;
}

void BlackBerryDeviceConfigurationWizardSetupPage::handleMachineTypeChanged()
{
    if (m_ui->deviceHostIp->text().isEmpty())
        m_ui->deviceHostIp->setText(defaultDeviceHostIp(machineType()));
}


// ----------------------------------------------------------------------------


BlackBerryDeviceConfigurationWizardSshKeyPage::BlackBerryDeviceConfigurationWizardSshKeyPage(QWidget *parent)
    : QWizardPage(parent)
    , m_ui(new Ui::BlackBerryDeviceConfigurationWizardSshKeyPage)
    , m_keyGen(0)
    , m_isGenerated(false)
{
    m_ui->setupUi(this);

    m_ui->privateKey->setExpectedKind(Utils::PathChooser::File);

    setTitle(tr("SSH Key Setup"));
    setSubTitle(tr("Please select an existing <b>4096</b>-bit key or click <b>Generate</b> to create a new one."));

    connect(m_ui->privateKey, SIGNAL(changed(QString)), this, SLOT(findMatchingPublicKey(QString)));
    connect(m_ui->privateKey, SIGNAL(changed(QString)), this, SIGNAL(completeChanged()));
    connect(m_ui->generate, SIGNAL(clicked()), this, SLOT(generateSshKey()));
}

BlackBerryDeviceConfigurationWizardSshKeyPage::~BlackBerryDeviceConfigurationWizardSshKeyPage()
{
    delete m_ui;
    m_ui = 0;

    delete m_keyGen;
    m_keyGen = 0;
}

void BlackBerryDeviceConfigurationWizardSshKeyPage::initializePage()
{
}

bool BlackBerryDeviceConfigurationWizardSshKeyPage::isComplete() const
{
    QFileInfo privateKeyFi(m_ui->privateKey->fileName().toString());
    QFileInfo publicKeyFi(m_ui->publicKey->text());

    return (privateKeyFi.exists() && publicKeyFi.exists()) || m_isGenerated;
}

QString BlackBerryDeviceConfigurationWizardSshKeyPage::privateKey() const
{
    return m_ui->privateKey->fileName().toString();
}

QString BlackBerryDeviceConfigurationWizardSshKeyPage::publicKey() const
{
    return m_ui->publicKey->text();
}

bool BlackBerryDeviceConfigurationWizardSshKeyPage::isGenerated() const
{
    return m_isGenerated;
}

QSsh::SshKeyGenerator *BlackBerryDeviceConfigurationWizardSshKeyPage::keyGenerator() const
{
    return m_keyGen;
}

void BlackBerryDeviceConfigurationWizardSshKeyPage::findMatchingPublicKey(const QString &privateKeyPath)
{
    const QString candidate = privateKeyPath + QLatin1String(".pub");
    if (QFileInfo(candidate).exists())
        m_ui->publicKey->setText(candidate);
}

void BlackBerryDeviceConfigurationWizardSshKeyPage::generateSshKey()
{
    if (!m_keyGen)
        m_keyGen = new QSsh::SshKeyGenerator;

    const bool success = m_keyGen->generateKeys(QSsh::SshKeyGenerator::Rsa,
                                                QSsh::SshKeyGenerator::Mixed, 4096,
                                                QSsh::SshKeyGenerator::DoNotOfferEncryption);

    if (!success) {
        QMessageBox::critical(this, tr("Key Generation Failed"), m_keyGen->error());
        m_isGenerated = false;
        return;
    }
    m_isGenerated = true;

    const QString storeLocation = Core::ICore::userResourcePath() + QLatin1String("/qnx/")
            + field(QLatin1String(DEVICENAME_FIELD_ID)).toString();
    const QString privKeyPath = storeLocation + QLatin1String("/id_rsa");
    const QString pubKeyPath = storeLocation + QLatin1String("/id_rsa.pub");

    m_ui->privateKey->setFileName(Utils::FileName::fromString(privKeyPath));
    m_ui->publicKey->setText(pubKeyPath);
    m_ui->privateKey->setEnabled(false);
}

// ----------------------------------------------------------------------------


BlackBerryDeviceConfigurationWizardFinalPage::BlackBerryDeviceConfigurationWizardFinalPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Setup Finished"));

    QVBoxLayout *layout = new QVBoxLayout(this);
    QLabel *label = new QLabel(tr("The new device configuration will now be created."), this);
    layout->addWidget(label);
}
