/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "maemodeviceconfigwizard.h"
#include "ui_maemodeviceconfigwizardkeycreationpage.h"
#include "ui_maemodeviceconfigwizardpreviouskeysetupcheckpage.h"
#include "ui_maemodeviceconfigwizardreusekeyscheckpage.h"
#include "ui_maemodeviceconfigwizardstartpage.h"

#include "maemodeviceconfigurations.h"

#include <coreplugin/ssh/sshkeygenerator.h>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtGui/QButtonGroup>
#include <QtGui/QDesktopServices>
#include <QtGui/QMessageBox>
#include <QtGui/QWizardPage>

namespace Qt4ProjectManager {
namespace Internal {
namespace {

struct WizardData
{
    QString configName;
    QString hostName;
    MaemoGlobal::MaemoVersion maemoVersion;
    MaemoDeviceConfig::DeviceType deviceType;
    QString privateKeyFilePath;
    QString publicKeyFilePath;
};

enum PageId {
    StartPageId, PreviousKeySetupCheckPageId, ReuseKeysCheckPageId,
    KeyCreationPageId, KeyDeploymentPageId, FinalTestPageId
};

class MaemoDeviceConfigWizardStartPage : public QWizardPage
{
    Q_OBJECT
public:
    MaemoDeviceConfigWizardStartPage(QWidget *parent = 0)
        : QWizardPage(parent), m_ui(new Ui::MaemoDeviceConfigWizardStartPage)
    {
        m_ui->setupUi(this);
        m_ui->harmattanButton->setChecked(true);
        m_ui->hwButton->setChecked(true);
        QButtonGroup * const buttonGroup = new QButtonGroup(this);
        buttonGroup->setExclusive(true);
        buttonGroup->addButton(m_ui->hwButton);
        buttonGroup->addButton(m_ui->qemuButton);
        m_ui->nameLineEdit->setText(QLatin1String("(New Configuration)"));
        connect(buttonGroup, SIGNAL(buttonClicked(int)),
           SLOT(handleDeviceTypeChanged()));
        handleDeviceTypeChanged();
        m_ui->hostNameLineEdit->setText(MaemoDeviceConfig::defaultHost(deviceType()));
        connect(m_ui->nameLineEdit, SIGNAL(textChanged(QString)), this,
            SIGNAL(completeChanged()));
        connect(m_ui->hostNameLineEdit, SIGNAL(textChanged(QString)), this,
            SIGNAL(completeChanged()));
    }

    virtual bool isComplete() const
    {
        return !configName().isEmpty() && !hostName().isEmpty();
    }

    QString configName() const { return m_ui->nameLineEdit->text().trimmed(); }

    QString hostName() const
    {
        return deviceType() == MaemoDeviceConfig::Simulator
            ? MaemoDeviceConfig::defaultHost(MaemoDeviceConfig::Simulator)
            : m_ui->hostNameLineEdit->text().trimmed();
    }

    MaemoGlobal::MaemoVersion maemoVersion() const
    {
        return m_ui->fremantleButton->isChecked() ? MaemoGlobal::Maemo5
            : m_ui->harmattanButton->isChecked() ? MaemoGlobal::Maemo6
            : MaemoGlobal::Meego;
    }

    MaemoDeviceConfig::DeviceType deviceType() const
    {
        return m_ui->hwButton->isChecked()
            ? MaemoDeviceConfig::Physical : MaemoDeviceConfig::Simulator;
    }

private slots:
    void handleDeviceTypeChanged()
    {
        const bool enable = deviceType() == MaemoDeviceConfig::Physical;
        m_ui->hostNameLabel->setEnabled(enable);
        m_ui->hostNameLineEdit->setEnabled(enable);
    }

private:
    const QScopedPointer<Ui::MaemoDeviceConfigWizardStartPage> m_ui;
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
        m_ui->keyWasNotSetUpButton->setChecked(true);
        QButtonGroup * const buttonGroup = new QButtonGroup(this);
        buttonGroup->setExclusive(true);
        buttonGroup->addButton(m_ui->keyWasSetUpButton);
        buttonGroup->addButton(m_ui->keyWasNotSetUpButton);
        connect(buttonGroup, SIGNAL(buttonClicked(int)),
            SLOT(handleSelectionChanged()));
        handleSelectionChanged();
        connect(m_ui->privateKeyFilePathLineEdit, SIGNAL(textChanged(QString)),
            this, SIGNAL(completeChanged()));
    }

    virtual bool isComplete() const {
        return !keyBasedLoginWasSetup() || !privateKeyFilePath().isEmpty();
    }

    bool keyBasedLoginWasSetup() const {
        return m_ui->keyWasSetUpButton->isChecked();
    }

    QString privateKeyFilePath() const {
        return m_ui->privateKeyFilePathLineEdit->text().trimmed();
    }

private:
    Q_SLOT void handleSelectionChanged()
    {
        m_ui->privateKeyFilePathLineEdit->setEnabled(keyBasedLoginWasSetup());
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
        m_ui->dontReuseButton->setChecked(true);
        QButtonGroup * const buttonGroup = new QButtonGroup(this);
        buttonGroup->setExclusive(true);
        buttonGroup->addButton(m_ui->reuseButton);
        buttonGroup->addButton(m_ui->dontReuseButton);
        connect(buttonGroup, SIGNAL(buttonClicked(int)),
            SLOT(handleSelectionChanged()));
        handleSelectionChanged();
        connect(m_ui->privateKeyFilePathLineEdit, SIGNAL(textChanged(QString)),
            this, SIGNAL(completeChanged()));
        connect(m_ui->publicKeyFilePathLineEdit, SIGNAL(textChanged(QString)),
            this, SIGNAL(completeChanged()));
    }

    virtual bool isComplete()
    {
        return !reuseKeys() || (!privateKeyFilePath().isEmpty()
            && !publicKeyFilePath().isEmpty());
    }

    bool reuseKeys() const { return m_ui->reuseButton->isChecked(); }
    QString privateKeyFilePath() const {
        return m_ui->privateKeyFilePathLineEdit->text().trimmed();
    }
    QString publicKeyFilePath() const {
        return m_ui->publicKeyFilePathLineEdit->text().trimmed();
    }

private:
    Q_SLOT void handleSelectionChanged()
    {
        m_ui->privateKeyFilePathLabel->setEnabled(reuseKeys());
        m_ui->privateKeyFilePathLineEdit->setEnabled(reuseKeys());
        m_ui->publicKeyFilePathLabel->setEnabled(reuseKeys());
        m_ui->publicKeyFilePathLineEdit->setEnabled(reuseKeys());
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
          m_ui(new Ui::MaemoDeviceConfigWizardKeyCreationPage),
          m_isComplete(false)
    {
        m_ui->setupUi(this);
        const QString &homeDir
            = QDesktopServices::storageLocation(QDesktopServices::HomeLocation);
        m_ui->directoryLineEdit->setText(homeDir);
        connect(m_ui->directoryLineEdit, SIGNAL(textChanged(QString)),
            SLOT(enableOrDisableButton()));
    }

    QString privateKeyFilePath() const {
        return m_ui->directoryLineEdit->text() + QLatin1String("/qtc_id_rsa");
    }

    QString publicKeyFilePath() const {
        return privateKeyFilePath() + QLatin1String(".pub");
    }

    virtual bool isComplete() const { return m_isComplete; }

private:
    Q_SLOT void enableOrDisableButton()
    {
        const QString &dir = m_ui->directoryLineEdit->text().trimmed();
        m_ui->createKeysButton->setEnabled(!dir.isEmpty());
    }

    Q_SLOT void createKeys()
    {
        const QString &dirName = m_ui->directoryLineEdit->text();
        QDir dir(dirName);
        if (!dir.exists() || !QFileInfo(dirName).isWritable()) {
            QMessageBox::critical(this, tr("Can't Create Keys"),
                tr("You have not entered a writable directory."));
            return;
        }

        m_ui->directoryLineEdit->setEnabled(false);
        m_ui->createKeysButton->setEnabled(false);
        Core::SshKeyGenerator keyGenerator;
        if (!keyGenerator.generateKeys(Core::SshKeyGenerator::Rsa,
             Core::SshKeyGenerator::OpenSsl, 1024)) {
            QMessageBox::critical(this, tr("Can't Create Keys"),
                tr("Key creation failed: %1").arg(keyGenerator.error()));
            m_ui->directoryLineEdit->setEnabled(true);
            m_ui->createKeysButton->setEnabled(true);
            return;
        }

        if (!saveFile(privateKeyFilePath(), keyGenerator.privateKey())
                || !saveFile(publicKeyFilePath(), keyGenerator.publicKey())) {
            m_ui->directoryLineEdit->setEnabled(true);
            m_ui->createKeysButton->setEnabled(true);
            return;
        }

        m_isComplete = true;
        emit completeChanged();
    }

    bool saveFile(const QString &filePath, const QByteArray &data)
    {
        QFile file(filePath);
        const bool canOpen = file.open(QIODevice::WriteOnly);
        if (canOpen)
            file.write(data);
        if (!canOpen || file.error() != QFile::NoError) {
            QMessageBox::critical(this, tr("Could Not Save File"),
                tr("Failed to save key file %1: %1").arg(filePath, file.errorString()));
            return false;
        }
        return true;
    }

    const QScopedPointer<Ui::MaemoDeviceConfigWizardKeyCreationPage> m_ui;
    bool m_isComplete;
};

} // anonymous namespace

struct MaemoDeviceConfigWizardPrivate
{
    MaemoDeviceConfigWizardPrivate(QWidget *parent)
        : startPage(parent),
          previousKeySetupPage(parent),
          reuseKeysCheckPage(parent),
          keyCreationPage(parent)
    {
    }

    WizardData wizardData;
    MaemoDeviceConfigWizardStartPage startPage;
    MaemoDeviceConfigWizardPreviousKeySetupCheckPage previousKeySetupPage;
    MaemoDeviceConfigWizardReuseKeysCheckPage reuseKeysCheckPage;
    MaemoDeviceConfigWizardKeyCreationPage keyCreationPage;
};


MaemoDeviceConfigWizard::MaemoDeviceConfigWizard(QWidget *parent) :
    QWizard(parent), d(new MaemoDeviceConfigWizardPrivate(this))
{
    setPage(StartPageId, &d->startPage);
    setPage(PreviousKeySetupCheckPageId, &d->previousKeySetupPage);
    setPage(ReuseKeysCheckPageId, &d->reuseKeysCheckPage);
    setPage(KeyCreationPageId, &d->keyCreationPage);
}

MaemoDeviceConfigWizard::~MaemoDeviceConfigWizard() {}

int MaemoDeviceConfigWizard::nextId() const
{
    switch (currentId()) {
    case StartPageId:
        // TODO: Make unique (needs list of devices)
        d->wizardData.configName = d->startPage.configName();

        d->wizardData.maemoVersion = d->startPage.maemoVersion();
        d->wizardData.deviceType = d->startPage.deviceType();
        d->wizardData.hostName = d->startPage.hostName();

        // TODO: Different paths for Qemu/HW?
        return PreviousKeySetupCheckPageId;
    case PreviousKeySetupCheckPageId:
        if (d->previousKeySetupPage.keyBasedLoginWasSetup()) {
            d->wizardData.privateKeyFilePath
                = d->previousKeySetupPage.privateKeyFilePath();
            return FinalTestPageId;
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
    case KeyDeploymentPageId: return FinalTestPageId;
    case FinalTestPageId: return -1;
    default:
        Q_ASSERT(false);
        return -1;
    }
}

} // namespace Internal
} // namespace Qt4ProjectManager

#include "maemodeviceconfigwizard.moc"
