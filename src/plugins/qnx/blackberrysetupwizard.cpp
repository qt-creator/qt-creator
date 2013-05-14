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

#include "blackberrysetupwizard.h"
#include "blackberrysetupwizardpages.h"
#include "blackberrydeviceconfiguration.h"
#include "blackberrycsjregistrar.h"
#include "blackberrycertificate.h"
#include "blackberryconfiguration.h"
#include "blackberrydebugtokenrequester.h"
#include "blackberrydebugtokenuploader.h"
#include "blackberrydeviceinformation.h"
#include "blackberryutils.h"
#include "qnxconstants.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <ssh/sshconnection.h>
#include <ssh/sshkeygenerator.h>
#include <utils/fileutils.h>
#include <coreplugin/icore.h>

#include <QDir>
#include <QMessageBox>
#include <QHostInfo>
#include <QAbstractButton>

using namespace ProjectExplorer;
using namespace Qnx;
using namespace Qnx::Internal;

BlackBerrySetupWizard::BlackBerrySetupWizard(QWidget *parent) :
    QWizard(parent),
    m_ndkPage(0),
    m_keysPage(0),
    m_devicePage(0),
    m_registrar(0),
    m_certificate(0),
    m_deviceInfo(0),
    m_requester(0),
    m_uploader(0),
    m_keyGenerator(0),
    m_currentStep(-1),
    m_stepOffset(0)
{
    setWindowTitle(tr("BlackBerry Development Environment Setup Wizard"));
    setOption(QWizard::IndependentPages, true);

    m_welcomePage = new BlackBerrySetupWizardWelcomePage(this);
    m_ndkPage = new BlackBerrySetupWizardNdkPage(this);
    m_keysPage = new BlackBerrySetupWizardKeysPage(this);
    m_devicePage = new BlackBerrySetupWizardDevicePage(this);
    m_finishPage = new BlackBerrySetupWizardFinishPage(this);

    setPage(WelcomePageId, m_welcomePage);
    setPage(NdkPageId, m_ndkPage);
    setPage(KeysPageId, m_keysPage);
    setPage(DevicePageId, m_devicePage);
    setPage(FinishPageId, m_finishPage);

    m_registrar = new BlackBerryCsjRegistrar(this);
    m_deviceInfo = new BlackBerryDeviceInformation(this);
    m_requester = new BlackBerryDebugTokenRequester(this);
    m_uploader = new BlackBerryDebugTokenUploader(this);
    m_keyGenerator = new QSsh::SshKeyGenerator;

    connect(m_registrar, SIGNAL(finished(int,QString)),
            this, SLOT(registrarFinished(int,QString)));
    connect(m_deviceInfo, SIGNAL(finished(int)),
            this, SLOT(deviceInfoFinished(int)));
    connect(m_requester, SIGNAL(finished(int)),
            this, SLOT(debugTokenArrived(int)));
    connect(m_uploader, SIGNAL(finished(int)),
            this, SLOT(uploaderFinished(int)));
    connect(this, SIGNAL(stepFinished()),
            this, SLOT(processNextStep()));

    registerStep("requestDevicePin", tr("Reading device PIN..."));
    registerStep("createKeys", tr("Registering CSJ keys..."));
    registerStep("generateDeveloperCertificate", tr("Generating developer certificate..."));
    registerStep("generateSshKeys", tr("Generating SSH keys..."));
    registerStep("requestDebugToken", tr("Requesting a debug token for the device..."));
    registerStep("uploadDebugToken", tr("Now uploading the debug token..."));
    registerStep("writeDeviceInformation", tr("Writing device information..."));
}

BlackBerrySetupWizard::~BlackBerrySetupWizard()
{
    delete m_keyGenerator;

    foreach (Step *s, m_stepList)
        delete s;

    m_stepList.clear();
}

void BlackBerrySetupWizard::accept()
{
    setBusy(true);
    processNextStep();
}

void BlackBerrySetupWizard::processNextStep()
{
    m_currentStep++;

    if (m_currentStep >= m_stepList.size())
        return;

    const Step *step = m_stepList.at(m_currentStep);

    m_finishPage->setProgress(step->message, m_currentStep * m_stepOffset);
    QMetaObject::invokeMethod(this, step->slot);
}

void BlackBerrySetupWizard::deviceInfoFinished(int status)
{
    const QString errorString = tr("Error reading device PIN. "
            "Please make sure that the specified device IP address is correct.");

    if (status != BlackBerryDeviceInformation::Success) {
        QMessageBox::critical(this, tr("Error"), errorString);
        reset();
        return;
    }

    m_devicePin = m_deviceInfo->devicePin();

    if (m_devicePin.isEmpty()) {
        QMessageBox::critical(this, tr("Error"), errorString);
        reset();
        return;
    }

    emit stepFinished();
}

void BlackBerrySetupWizard::registrarFinished(int status,
        const QString &errorString)
{
    if (status == BlackBerryCsjRegistrar::Error) {
        QMessageBox::critical(this, tr("Error"), errorString);
        reset();
        return;
    }

    emit stepFinished();
}

void BlackBerrySetupWizard::certificateCreated(int status)
{
    if (status == BlackBerryCertificate::Error) {
        QMessageBox::critical(this, tr("Error"), tr("Error creating developer certificate."));
        m_certificate->deleteLater();
        m_certificate = 0;
        reset();
        return;
    }

    BlackBerryConfiguration &configuration = BlackBerryConfiguration::instance();
    configuration.addCertificate(m_certificate);

    emit stepFinished();
}

void BlackBerrySetupWizard::debugTokenArrived(int status)
{
    QString errorString = tr("Failed to request debug token: ");

    switch (status) {
    case BlackBerryDebugTokenRequester::Success:
        emit stepFinished();
        return;
    case BlackBerryDebugTokenRequester::WrongCskPassword:
        errorString += tr("Wrong CSK password.");
        break;
    case BlackBerryDebugTokenRequester::WrongKeystorePassword:
        errorString += tr("Wrong keystore password.");
        break;
    case BlackBerryDebugTokenRequester::NetworkUnreachable:
        errorString += tr("Network unreachable.");
        break;
    case BlackBerryDebugTokenRequester::IllegalPin:
        errorString += tr("Illegal device PIN.");
        break;
    case BlackBerryDebugTokenRequester::FailedToStartInferiorProcess:
        errorString += tr("Failed to start inferior process.");
        break;
    case BlackBerryDebugTokenRequester::InferiorProcessTimedOut:
        errorString += tr("Inferior processes timed out.");
        break;
    case BlackBerryDebugTokenRequester::InferiorProcessCrashed:
        errorString += tr("Inferior process has crashed.");
        break;
    case BlackBerryDebugTokenRequester::InferiorProcessReadError:
    case BlackBerryDebugTokenRequester::InferiorProcessWriteError:
        errorString += tr("Failed to communicate with the inferior process.");
        break;
    case BlackBerryDebugTokenRequester::UnknownError:
        errorString += tr("An unknwon error has occurred.");
        break;
    }

    QMessageBox::critical(this, tr("Error"), errorString);

    reset();
}

void BlackBerrySetupWizard::uploaderFinished(int status)
{
    QString errorString = tr("Failed to upload debug token: ");

    switch (status) {
    case BlackBerryDebugTokenUploader::Success:
        emit stepFinished();
        return;
    case BlackBerryDebugTokenUploader::NoRouteToHost:
        errorString += tr("No route to host.");
        break;
    case BlackBerryDebugTokenUploader::AuthenticationFailed:
        errorString += tr("Authentication failed.");
        break;
    case BlackBerryDebugTokenUploader::DevelopmentModeDisabled:
        errorString += tr("Development mode is disabled on the device.");
        break;
    case BlackBerryDebugTokenUploader::FailedToStartInferiorProcess:
        errorString += tr("Failed to start inferior process.");
        break;
    case BlackBerryDebugTokenUploader::InferiorProcessTimedOut:
        errorString += tr("Inferior processes timed out.");
        break;
    case BlackBerryDebugTokenUploader::InferiorProcessCrashed:
        errorString += tr("Inferior process has crashed.");
        break;
    case BlackBerryDebugTokenUploader::InferiorProcessReadError:
    case BlackBerryDebugTokenUploader::InferiorProcessWriteError:
        errorString += tr("Failed to communicate with the inferior process.");
        break;
    case BlackBerryDebugTokenUploader::UnknownError:
        errorString += tr("An unknwon error has happened.");
        break;
    }

    QMessageBox::critical(this, tr("Error"), errorString);

    reset();
}

void BlackBerrySetupWizard::registerStep(const QByteArray &slot, const QString &message)
{
    Step *s = new Step;
    s->slot = slot;
    s->message = message;
    m_stepList << s;

    m_stepOffset = 100/m_stepList.size();
}

void BlackBerrySetupWizard::setBusy(bool busy)
{
    button(QWizard::BackButton)->setEnabled(!busy);
    button(QWizard::NextButton)->setEnabled(!busy);
    button(QWizard::FinishButton)->setEnabled(!busy);
    button(QWizard::CancelButton)->setEnabled(!busy);

    if (!busy)
        m_finishPage->setProgress(QString(), -1);
}

void BlackBerrySetupWizard::cleanupFiles() const
{
    BlackBerryConfiguration &configuration = BlackBerryConfiguration::instance();

    QFile f(configuration.barsignerCskPath());
    f.remove();

    f.setFileName(configuration.barsignerDbPath());
    f.remove();

    f.setFileName(configuration.defaultKeystorePath());
    f.remove();

    f.setFileName(configuration.defaultDebugTokenPath());
    f.remove();

    f.setFileName(privateKeyPath());
    f.remove();

    f.setFileName(publicKeyPath());
    f.remove();

    QDir::root().rmpath(storeLocation());
}

void BlackBerrySetupWizard::reset()
{
    cleanupFiles();
    setBusy(false);
    m_currentStep = -1;

    if (m_certificate) {
        BlackBerryConfiguration &configuration = BlackBerryConfiguration::instance();
        configuration.removeCertificate(m_certificate);
        m_certificate = 0;
    }
}

void BlackBerrySetupWizard::requestDevicePin()
{
    if (!isPhysicalDevice()) {
        emit stepFinished();
        return;
    }

    m_deviceInfo->setDeviceTarget(hostName(), devicePassword());
}

void BlackBerrySetupWizard::createKeys()
{
    QStringList csjFiles;
    csjFiles << rdkPath() << pbdtPath();

    m_registrar->tryRegister(csjFiles, csjPin(), password());
}

void BlackBerrySetupWizard::generateDeveloperCertificate()
{
    BlackBerryConfiguration &configuration = BlackBerryConfiguration::instance();

    m_certificate = new BlackBerryCertificate(configuration.defaultKeystorePath(),
            BlackBerryUtils::getCsjAuthor(rdkPath()), password());

    connect(m_certificate, SIGNAL(finished(int)), this, SLOT(certificateCreated(int)));

    m_certificate->store();
}

void BlackBerrySetupWizard::generateSshKeys()
{
    const bool success = m_keyGenerator->generateKeys(QSsh::SshKeyGenerator::Rsa,
            QSsh::SshKeyGenerator::Mixed, 4096,
            QSsh::SshKeyGenerator::DoNotOfferEncryption);

    if (!success) {
        QMessageBox::critical(this, tr("Key Generation Failed"), m_keyGenerator->error());
        reset();
        return;
    }

    if (!QDir::root().mkpath(storeLocation())) {
        QMessageBox::critical(this, tr("Failure to Save Key File"),
                              tr("Failed to create directory: '%1'.").arg(storeLocation()));
        reset();
        return;
    }

    if (QFileInfo(privateKeyPath()).exists()) {
        QMessageBox::critical(this, tr("Failure to Save Key File"),
                              tr("Private key file already exists: '%1'").arg(privateKeyPath()));
        reset();
        return;
    }

    if (QFileInfo(publicKeyPath()).exists()) {
        QMessageBox::critical(this, tr("Failure to Save Key File"),
                              tr("Public key file already exists: '%1'").arg(publicKeyPath()));
        reset();
        return;
    }

    Utils::FileSaver privateKeySaver(privateKeyPath());
    privateKeySaver.write(m_keyGenerator->privateKey());

    if (!privateKeySaver.finalize(this)) {
        reset();
        return; // finalize shows an error message if necessary
    }

    QFile::setPermissions(privateKeyPath(), QFile::ReadOwner | QFile::WriteOwner);

    Utils::FileSaver publicKeySaver(publicKeyPath());

    // blackberry-connect requires an @ character to be included in the RSA comment
    const QString atHost = QLatin1Char('@') + QHostInfo::localHostName();
    QByteArray publicKeyContent = m_keyGenerator->publicKey();
    publicKeyContent.append(atHost.toLocal8Bit());

    publicKeySaver.write(publicKeyContent);

    if (!publicKeySaver.finalize(this)) {
        reset();
        return;
    }

    emit stepFinished();
}

void BlackBerrySetupWizard::requestDebugToken()
{
    if (!isPhysicalDevice()) {
        emit stepFinished();
        return;
    }

    BlackBerryConfiguration &configuration = BlackBerryConfiguration::instance();

    m_requester->requestDebugToken(configuration.defaultDebugTokenPath(),
            password(), configuration.defaultKeystorePath(), password(), m_devicePin);
}

void BlackBerrySetupWizard::uploadDebugToken()
{
    if (!isPhysicalDevice()) {
        emit stepFinished();
        return;
    }

    BlackBerryConfiguration &configuration = BlackBerryConfiguration::instance();

    m_uploader->uploadDebugToken(configuration.defaultDebugTokenPath(),
            hostName(), devicePassword());
}

void BlackBerrySetupWizard::writeDeviceInformation()
{
    DeviceManager * const deviceManager = DeviceManager::instance();
    deviceManager->addDevice(device());

    QWizard::accept();
}

QString BlackBerrySetupWizard::privateKeyPath() const
{
    return storeLocation() + QLatin1String("/id_rsa");
}

QString BlackBerrySetupWizard::publicKeyPath() const
{
    return storeLocation() + QLatin1String("/id_rsa.pub");
}

QString BlackBerrySetupWizard::deviceName() const
{
    return field(QLatin1String(BlackBerrySetupWizardDevicePage::NameField)).toString();
}

QString BlackBerrySetupWizard::storeLocation() const
{
    return Core::ICore::userResourcePath() + QLatin1String("/qnx/") + deviceName();
}

QString BlackBerrySetupWizard::rdkPath() const
{
    return field(QLatin1String(BlackBerrySetupWizardKeysPage::RdkPathField)).toString();
}

QString BlackBerrySetupWizard::pbdtPath() const
{
    return field(QLatin1String(BlackBerrySetupWizardKeysPage::PbdtPathField)).toString();
}

QString BlackBerrySetupWizard::csjPin() const
{
    return field(QLatin1String(BlackBerrySetupWizardKeysPage::CsjPinField)).toString();
}

QString BlackBerrySetupWizard::password() const
{
    return field(QLatin1String(BlackBerrySetupWizardKeysPage::PasswordField)).toString();
}

QString BlackBerrySetupWizard::hostName() const
{
    return field(QLatin1String(BlackBerrySetupWizardDevicePage::IpAddressField)).toString();
}

QString BlackBerrySetupWizard::devicePassword() const
{
    return field(QLatin1String(BlackBerrySetupWizardDevicePage::PasswordField)).toString();
}

bool BlackBerrySetupWizard::isPhysicalDevice() const
{
    return field(QLatin1String(BlackBerrySetupWizardDevicePage::PhysicalDeviceField)).toBool();
}

IDevice::Ptr BlackBerrySetupWizard::device()
{
    QSsh::SshConnectionParameters sshParams;
    sshParams.options = QSsh::SshIgnoreDefaultProxy;
    sshParams.host = hostName();
    sshParams.password = devicePassword();
    sshParams.authenticationType = QSsh::SshConnectionParameters::AuthenticationByKey;
    sshParams.privateKeyFile = privateKeyPath();
    sshParams.userName = QLatin1String("devuser");
    sshParams.timeout = 10;
    sshParams.port = 22;

    IDevice::MachineType machineType = (isPhysicalDevice()) ? IDevice::Hardware : IDevice::Emulator;

    BlackBerryDeviceConfiguration::Ptr configuration = BlackBerryDeviceConfiguration::create(
            deviceName(), Core::Id(Constants::QNX_BB_OS_TYPE), machineType);

    configuration->setSshParameters(sshParams);
    configuration->setDebugToken(BlackBerryConfiguration::instance().defaultDebugTokenPath());

    return configuration;
}
