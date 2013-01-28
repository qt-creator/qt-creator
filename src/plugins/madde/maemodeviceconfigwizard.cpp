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

#include "maemodeviceconfigwizard.h"
#include "ui_maemodeviceconfigwizardkeycreationpage.h"
#include "ui_maemodeviceconfigwizardkeydeploymentpage.h"
#include "ui_maemodeviceconfigwizardpreviouskeysetupcheckpage.h"
#include "ui_maemodeviceconfigwizardreusekeyscheckpage.h"
#include "ui_maemodeviceconfigwizardstartpage.h"

#include "maddedevice.h"
#include "maddedevicetester.h"
#include "maemoconstants.h"
#include "maemoglobal.h"

#include <remotelinux/genericlinuxdeviceconfigurationwizardpages.h>
#include <remotelinux/linuxdevicetestdialog.h>
#include <remotelinux/sshkeydeployer.h>
#include <utils/fileutils.h>
#include <ssh/sshkeygenerator.h>

#include <QDir>
#include <QFile>
#include <QButtonGroup>
#include <QDesktopServices>
#include <QMessageBox>
#include <QWizardPage>

using namespace ProjectExplorer;
using namespace RemoteLinux;
using namespace QSsh;
using namespace Utils;

namespace Madde {
namespace Internal {
namespace {

QString defaultUser()
{
    return QLatin1String("developer");
}

QString defaultHost(IDevice::MachineType type)
{
    return QLatin1String(type == IDevice::Hardware ? "192.168.2.15" : "localhost");
}

struct WizardData
{
    QString configName;
    QString hostName;
    Core::Id deviceType;
    SshConnectionParameters::AuthenticationType authType;
    IDevice::MachineType machineType;
    QString privateKeyFilePath;
    QString publicKeyFilePath;
    QString userName;
    QString password;
    int sshPort;
};

enum PageId {
    StartPageId, PreviousKeySetupCheckPageId, ReuseKeysCheckPageId, KeyCreationPageId,
    KeyDeploymentPageId, FinalPageId
};

class MaemoDeviceConfigWizardStartPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit MaemoDeviceConfigWizardStartPage(QWidget *parent = 0)
        : QWizardPage(parent), m_ui(new Ui::MaemoDeviceConfigWizardStartPage)
    {
        m_ui->setupUi(this);
        setTitle(tr("General Information"));
        setSubTitle(QLatin1String(" ")); // For Qt bug (background color)

        QButtonGroup *buttonGroup = new QButtonGroup(this);
        buttonGroup->setExclusive(true);
        buttonGroup->addButton(m_ui->hwButton);
        buttonGroup->addButton(m_ui->emulatorButton);
        connect(buttonGroup, SIGNAL(buttonClicked(int)), SLOT(handleMachineTypeChanged()));

        m_ui->nameLineEdit->setText(tr("MeeGo Device"));
        m_ui->hwButton->setChecked(true);
        handleMachineTypeChanged();
        m_ui->hostNameLineEdit->setText(defaultHost(machineType()));
        m_ui->sshPortSpinBox->setMinimum(1);
        m_ui->sshPortSpinBox->setMaximum(65535);
        m_ui->sshPortSpinBox->setValue(22);
        connect(m_ui->nameLineEdit, SIGNAL(textChanged(QString)), this,
            SIGNAL(completeChanged()));
        connect(m_ui->hostNameLineEdit, SIGNAL(textChanged(QString)), this,
            SIGNAL(completeChanged()));
    }

    void setDeviceType(Core::Id type)
    {
        m_deviceType = type;
        m_ui->nameLineEdit->setText(tr("%1 Device").arg(MaddeDevice::maddeDisplayType(m_deviceType)));
    }

    virtual bool isComplete() const
    {
        return !configName().isEmpty() && !hostName().isEmpty();
    }

    QString configName() const { return m_ui->nameLineEdit->text().trimmed(); }

    QString hostName() const
    {
        return machineType() == IDevice::Emulator
            ? defaultHost(IDevice::Emulator)
            : m_ui->hostNameLineEdit->text().trimmed();
    }

    Core::Id deviceType() const
    {
        return m_deviceType;
    }

    IDevice::MachineType machineType() const
    {
        return m_ui->hwButton->isChecked() ? IDevice::Hardware : IDevice::Emulator;
    }

    int sshPort() const
    {
        return machineType() == IDevice::Emulator ? 6666 : m_ui->sshPortSpinBox->value();
    }

private slots:
    void handleMachineTypeChanged()
    {
        const bool enable = machineType() == IDevice::Hardware;
        m_ui->hostNameLabel->setEnabled(enable);
        m_ui->hostNameLineEdit->setEnabled(enable);
        m_ui->sshPortLabel->setEnabled(enable);
        m_ui->sshPortSpinBox->setEnabled(enable);
    }

private:
    const QScopedPointer<Ui::MaemoDeviceConfigWizardStartPage> m_ui;
    Core::Id m_deviceType;
};

class MaemoDeviceConfigWizardPreviousKeySetupCheckPage : public QWizardPage
{
    Q_OBJECT
public:
    MaemoDeviceConfigWizardPreviousKeySetupCheckPage(QWidget *parent)
        : QWizardPage(parent),
          m_ui(new Ui::MaemoDeviceConfigWizardCheckPreviousKeySetupPage)
    {
        m_ui->setupUi(this);
        m_ui->privateKeyFilePathChooser->setExpectedKind(PathChooser::File);
        setTitle(tr("Device Status Check"));
        setSubTitle(QLatin1String(" ")); // For Qt bug (background color)
        QButtonGroup * const buttonGroup = new QButtonGroup(this);
        buttonGroup->setExclusive(true);
        buttonGroup->addButton(m_ui->keyWasSetUpButton);
        buttonGroup->addButton(m_ui->keyWasNotSetUpButton);
        connect(buttonGroup, SIGNAL(buttonClicked(int)),
            SLOT(handleSelectionChanged()));
        connect(m_ui->privateKeyFilePathChooser, SIGNAL(changed(QString)),
            this, SIGNAL(completeChanged()));
    }

    virtual bool isComplete() const
    {
        return !keyBasedLoginWasSetup()
            || m_ui->privateKeyFilePathChooser->isValid();
    }

    virtual void initializePage()
    {
        m_ui->keyWasNotSetUpButton->setChecked(true);
        m_ui->privateKeyFilePathChooser->setPath(IDevice::defaultPrivateKeyFilePath());
        handleSelectionChanged();
    }

    bool keyBasedLoginWasSetup() const {
        return m_ui->keyWasSetUpButton->isChecked();
    }

    QString privateKeyFilePath() const {
        return m_ui->privateKeyFilePathChooser->path();
    }

private:
    Q_SLOT void handleSelectionChanged()
    {
        m_ui->privateKeyFilePathChooser->setEnabled(keyBasedLoginWasSetup());
        emit completeChanged();
    }

    const QScopedPointer<Ui::MaemoDeviceConfigWizardCheckPreviousKeySetupPage> m_ui;
};

class MaemoDeviceConfigWizardReuseKeysCheckPage : public QWizardPage
{
    Q_OBJECT
public:
    MaemoDeviceConfigWizardReuseKeysCheckPage(QWidget *parent)
        : QWizardPage(parent),
          m_ui(new Ui::MaemoDeviceConfigWizardReuseKeysCheckPage)
    {
        m_ui->setupUi(this);
        setTitle(tr("Existing Keys Check"));
        setSubTitle(QLatin1String(" ")); // For Qt bug (background color)
        m_ui->publicKeyFilePathChooser->setExpectedKind(PathChooser::File);
        m_ui->privateKeyFilePathChooser->setExpectedKind(PathChooser::File);
        QButtonGroup * const buttonGroup = new QButtonGroup(this);
        buttonGroup->setExclusive(true);
        buttonGroup->addButton(m_ui->reuseButton);
        buttonGroup->addButton(m_ui->dontReuseButton);
        connect(buttonGroup, SIGNAL(buttonClicked(int)),
            SLOT(handleSelectionChanged()));
        connect(m_ui->privateKeyFilePathChooser, SIGNAL(changed(QString)),
            this, SIGNAL(completeChanged()));
        connect(m_ui->publicKeyFilePathChooser, SIGNAL(changed(QString)),
            this, SIGNAL(completeChanged()));
    }

    virtual bool isComplete() const
    {
        return !reuseKeys() || (m_ui->publicKeyFilePathChooser->isValid()
            && m_ui->privateKeyFilePathChooser->isValid());
    }

    virtual void initializePage()
    {
        m_ui->dontReuseButton->setChecked(true);
        m_ui->privateKeyFilePathChooser->setPath(IDevice::defaultPrivateKeyFilePath());
        m_ui->publicKeyFilePathChooser->setPath(IDevice::defaultPublicKeyFilePath());
        handleSelectionChanged();
    }

    bool reuseKeys() const { return m_ui->reuseButton->isChecked(); }

    QString privateKeyFilePath() const {
        return m_ui->privateKeyFilePathChooser->path();
    }

    QString publicKeyFilePath() const {
        return m_ui->publicKeyFilePathChooser->path();
    }

private:
    Q_SLOT void handleSelectionChanged()
    {
        m_ui->privateKeyFilePathLabel->setEnabled(reuseKeys());
        m_ui->privateKeyFilePathChooser->setEnabled(reuseKeys());
        m_ui->publicKeyFilePathLabel->setEnabled(reuseKeys());
        m_ui->publicKeyFilePathChooser->setEnabled(reuseKeys());
        emit completeChanged();
    }

    const QScopedPointer<Ui::MaemoDeviceConfigWizardReuseKeysCheckPage> m_ui;
};

class MaemoDeviceConfigWizardKeyCreationPage : public QWizardPage
{
    Q_OBJECT
public:
    MaemoDeviceConfigWizardKeyCreationPage(QWidget *parent)
        : QWizardPage(parent),
          m_ui(new Ui::MaemoDeviceConfigWizardKeyCreationPage)
    {
        m_ui->setupUi(this);
        setTitle(tr("Key Creation"));
        setSubTitle(QLatin1String(" ")); // For Qt bug (background color)
        connect(m_ui->createKeysButton, SIGNAL(clicked()), SLOT(createKeys()));
    }

    QString privateKeyFilePath() const {
        return m_ui->keyDirPathChooser->path() + QLatin1String("/qtc_id_rsa");
    }

    QString publicKeyFilePath() const {
        return privateKeyFilePath() + QLatin1String(".pub");
    }

    virtual void initializePage()
    {
        m_isComplete = false;
        const QString &dir = QDesktopServices::storageLocation(QDesktopServices::HomeLocation)
           + QLatin1String("/.ssh");
        m_ui->keyDirPathChooser->setPath(dir);
        enableInput();
    }

    virtual bool isComplete() const { return m_isComplete; }

private:

    Q_SLOT void createKeys()
    {
        const QString &dirPath = m_ui->keyDirPathChooser->path();
        QFileInfo fi(dirPath);
        if (fi.exists() && !fi.isDir()) {
            QMessageBox::critical(this, tr("Cannot Create Keys"),
                tr("The path you have entered is not a directory."));
            return;
        }
        if (!fi.exists() && !QDir::root().mkpath(dirPath)) {
            QMessageBox::critical(this, tr("Cannot Create Keys"),
                tr("The directory you have entered does not exist and "
                   "cannot be created."));
            return;
        }

        m_ui->keyDirPathChooser->setEnabled(false);
        m_ui->createKeysButton->setEnabled(false);
        m_ui->statusLabel->setText(tr("Creating keys..."));
        SshKeyGenerator keyGenerator;
        if (!keyGenerator.generateKeys(SshKeyGenerator::Rsa,
             SshKeyGenerator::Mixed, 1024)) {
            QMessageBox::critical(this, tr("Cannot Create Keys"),
                tr("Key creation failed: %1").arg(keyGenerator.error()));
            enableInput();
            return;
        }

        if (!saveFile(privateKeyFilePath(), keyGenerator.privateKey())
                || !saveFile(publicKeyFilePath(), keyGenerator.publicKey())) {
            enableInput();
            return;
        }
        QFile::setPermissions(privateKeyFilePath(),
            QFile::ReadOwner | QFile::WriteOwner);

        m_ui->statusLabel->setText(m_ui->statusLabel->text() + tr("Done."));
        m_isComplete = true;
        emit completeChanged();
    }

    bool saveFile(const QString &filePath, const QByteArray &data)
    {
        Utils::FileSaver saver(filePath);
        saver.write(data);
        if (!saver.finalize()) {
            QMessageBox::critical(this, tr("Could Not Save Key File"), saver.errorString());
            return false;
        }
        return true;
    }

    void enableInput()
    {
        m_ui->keyDirPathChooser->setEnabled(true);
        m_ui->createKeysButton->setEnabled(true);
        m_ui->statusLabel->clear();
    }

    const QScopedPointer<Ui::MaemoDeviceConfigWizardKeyCreationPage> m_ui;
    bool m_isComplete;
};

class MaemoDeviceConfigWizardKeyDeploymentPage : public QWizardPage
{
    Q_OBJECT
public:
    MaemoDeviceConfigWizardKeyDeploymentPage(const WizardData &wizardData,
        QWidget *parent = 0)
            : QWizardPage(parent),
              m_ui(new Ui::MaemoDeviceConfigWizardKeyDeploymentPage),
              m_wizardData(wizardData),
              m_keyDeployer(new SshKeyDeployer(this))
    {
        m_ui->setupUi(this);
        m_instructionTextTemplate = m_ui->instructionLabel->text();
        setTitle(tr("Key Deployment"));
        setSubTitle(QLatin1String(" ")); // For Qt bug (background color)
        connect(m_ui->deviceAddressLineEdit, SIGNAL(textChanged(QString)),
            SLOT(enableOrDisableButton()));
        connect(m_ui->passwordLineEdit, SIGNAL(textChanged(QString)),
            SLOT(enableOrDisableButton()));
        connect(m_ui->deployButton, SIGNAL(clicked()), SLOT(deployKey()));
        connect(m_keyDeployer, SIGNAL(error(QString)),
            SLOT(handleKeyDeploymentError(QString)));
        connect(m_keyDeployer, SIGNAL(finishedSuccessfully()),
            SLOT(handleKeyDeploymentSuccess()));
    }

    virtual void initializePage()
    {
        m_isComplete = false;
        m_ui->deviceAddressLineEdit->setText(m_wizardData.hostName);
        m_ui->instructionLabel->setText(QString(m_instructionTextTemplate)
            .replace(QLatin1String("%%%maddev%%%"),
                MaemoGlobal::madDeveloperUiName(m_wizardData.deviceType)));
        m_ui->passwordLineEdit->clear();
        enableInput();
    }

    virtual bool isComplete() const { return m_isComplete; }

    QString hostAddress() const {
        return m_ui->deviceAddressLineEdit->text().trimmed();
    }

private:
    Q_SLOT void enableOrDisableButton()
    {
        m_ui->deployButton->setEnabled(!hostAddress().isEmpty()
            && !password().isEmpty());
    }

    Q_SLOT void deployKey()
    {
        using namespace QSsh;
        m_ui->deviceAddressLineEdit->setEnabled(false);
        m_ui->passwordLineEdit->setEnabled(false);
        m_ui->deployButton->setEnabled(false);
        SshConnectionParameters sshParams;
        sshParams.authenticationType = SshConnectionParameters::AuthenticationByPassword;
        sshParams.host = hostAddress();
        sshParams.port = m_wizardData.sshPort;
        sshParams.password = password();
        sshParams.timeout = 10;
        sshParams.userName = defaultUser();
        m_ui->statusLabel->setText(tr("Deploying..."));
        m_keyDeployer->deployPublicKey(sshParams, m_wizardData.publicKeyFilePath);
    }

    Q_SLOT void handleKeyDeploymentError(const QString&errorMsg)
    {
        QMessageBox::critical(this, tr("Key Deployment Failure"), errorMsg);
        enableInput();
    }

    Q_SLOT void handleKeyDeploymentSuccess()
    {
        QMessageBox::information(this, tr("Key Deployment Success"),
            tr("The key was successfully deployed. You may now close "
               "the \"%1\" application and continue.")
               .arg(MaemoGlobal::madDeveloperUiName(m_wizardData.deviceType)));
        m_ui->statusLabel->setText(m_ui->statusLabel->text() + tr("Done."));
        m_isComplete = true;
        emit completeChanged();
    }

    void enableInput()
    {
        m_ui->deviceAddressLineEdit->setEnabled(true);
        m_ui->passwordLineEdit->setEnabled(true);
        m_ui->statusLabel->clear();
        enableOrDisableButton();
    }

    QString password() const {
        return m_ui->passwordLineEdit->text().trimmed();
    }


    const QScopedPointer<Ui::MaemoDeviceConfigWizardKeyDeploymentPage> m_ui;
    bool m_isComplete;
    const WizardData &m_wizardData;
    SshKeyDeployer * const m_keyDeployer;
    QString m_instructionTextTemplate;
};

class MaemoDeviceConfigWizardFinalPage : public GenericLinuxDeviceConfigurationWizardFinalPage
{
    Q_OBJECT
public:
    MaemoDeviceConfigWizardFinalPage(const WizardData &wizardData, QWidget *parent)
        : GenericLinuxDeviceConfigurationWizardFinalPage(parent), m_wizardData(wizardData)
    {
    }

private:
    QString infoText() const
    {
        if (m_wizardData.machineType == IDevice::Emulator)
            return tr("The new device configuration will now be created.");
        return GenericLinuxDeviceConfigurationWizardFinalPage::infoText();
    }

    const WizardData &m_wizardData;
};

} // anonymous namespace

struct MaemoDeviceConfigWizardPrivate
{
    MaemoDeviceConfigWizardPrivate(QWidget *parent)
        : startPage(parent),
          previousKeySetupPage(parent),
          reuseKeysCheckPage(parent),
          keyCreationPage(parent),
          keyDeploymentPage(wizardData, parent),
          finalPage(wizardData, parent)
    {
    }

    WizardData wizardData;
    MaemoDeviceConfigWizardStartPage startPage;
    MaemoDeviceConfigWizardPreviousKeySetupCheckPage previousKeySetupPage;
    MaemoDeviceConfigWizardReuseKeysCheckPage reuseKeysCheckPage;
    MaemoDeviceConfigWizardKeyCreationPage keyCreationPage;
    MaemoDeviceConfigWizardKeyDeploymentPage keyDeploymentPage;
    MaemoDeviceConfigWizardFinalPage finalPage;
};


MaemoDeviceConfigWizard::MaemoDeviceConfigWizard(Core::Id id, QWidget *parent)
    : QWizard(parent), d(new MaemoDeviceConfigWizardPrivate(this))
{
    setWindowTitle(tr("New Device Configuration Setup"));
    setPage(StartPageId, &d->startPage);
    d->startPage.setDeviceType(id);
    setPage(PreviousKeySetupCheckPageId, &d->previousKeySetupPage);
    setPage(ReuseKeysCheckPageId, &d->reuseKeysCheckPage);
    setPage(KeyCreationPageId, &d->keyCreationPage);
    setPage(KeyDeploymentPageId, &d->keyDeploymentPage);
    setPage(FinalPageId, &d->finalPage);
    d->finalPage.setCommitPage(true);
}

MaemoDeviceConfigWizard::~MaemoDeviceConfigWizard()
{
    delete d;
}

IDevice::Ptr MaemoDeviceConfigWizard::device()
{
    bool doTest;
    QString freePortsSpec;
    QSsh::SshConnectionParameters sshParams;
    sshParams.userName = defaultUser();
    sshParams.host = d->wizardData.hostName;
    sshParams.port = d->wizardData.sshPort;
    if (d->wizardData.machineType == IDevice::Emulator) {
        sshParams.authenticationType = QSsh::SshConnectionParameters::AuthenticationByPassword;
        sshParams.password = QString();
        sshParams.timeout = 30;
        freePortsSpec = QLatin1String("13219,14168");
        doTest = false;
    } else {
        sshParams.authenticationType = QSsh::SshConnectionParameters::AuthenticationByKey;
        sshParams.privateKeyFile = d->wizardData.privateKeyFilePath;
        sshParams.timeout = 10;
        freePortsSpec = QLatin1String("10000-10100");
        doTest = true;
    }
    const MaddeDevice::Ptr device = MaddeDevice::create(d->wizardData.configName,
        d->wizardData.deviceType, d->wizardData.machineType);
    device->setFreePorts(PortList::fromString(freePortsSpec));
    device->setSshParameters(sshParams);
    if (doTest) {
        // Might be called after accept.
        QWidget *parent = isVisible() ? this : static_cast<QWidget *>(0);
        LinuxDeviceTestDialog dlg(device, new MaddeDeviceTester(this), parent);
        dlg.exec();
    }
    return device;
}

int MaemoDeviceConfigWizard::nextId() const
{
    switch (currentId()) {
    case StartPageId:
        d->wizardData.configName = d->startPage.configName();
        d->wizardData.deviceType = d->startPage.deviceType();
        d->wizardData.machineType = d->startPage.machineType();
        d->wizardData.hostName = d->startPage.hostName();
        d->wizardData.sshPort = d->startPage.sshPort();
        if (d->wizardData.machineType == IDevice::Emulator)
            return FinalPageId;
        return PreviousKeySetupCheckPageId;
    case PreviousKeySetupCheckPageId:
        if (d->previousKeySetupPage.keyBasedLoginWasSetup()) {
            d->wizardData.privateKeyFilePath
                = d->previousKeySetupPage.privateKeyFilePath();
            return FinalPageId;
        } else {
            return ReuseKeysCheckPageId;
        }
    case ReuseKeysCheckPageId:
        if (d->reuseKeysCheckPage.reuseKeys()) {
            d->wizardData.privateKeyFilePath
                = d->reuseKeysCheckPage.privateKeyFilePath();
            d->wizardData.publicKeyFilePath
                = d->reuseKeysCheckPage.publicKeyFilePath();
            return KeyDeploymentPageId;
        } else {
            return KeyCreationPageId;
        }
    case KeyCreationPageId:
        d->wizardData.privateKeyFilePath
            = d->keyCreationPage.privateKeyFilePath();
        d->wizardData.publicKeyFilePath
            = d->keyCreationPage.publicKeyFilePath();
        return KeyDeploymentPageId;
    case KeyDeploymentPageId:
        d->wizardData.hostName = d->keyDeploymentPage.hostAddress();
        return FinalPageId;
    case FinalPageId: return -1;
    default:
        Q_ASSERT(false);
        return -1;
    }
}

} // namespace Internal
} // namespace Madde

#include "maemodeviceconfigwizard.moc"
