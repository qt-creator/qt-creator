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
#include "blackberryconfiguration.h"
#include "blackberrydebugtokenrequestdialog.h"
#include "blackberrysshkeysgenerator.h"
#include "ui_blackberrydeviceconfigurationwizardsetuppage.h"
#include "ui_blackberrydeviceconfigurationwizardsshkeypage.h"
#include "blackberryconfiguration.h"
#include "qnxutils.h"

#include <coreplugin/icore.h>
#include <ssh/sshkeygenerator.h>

#include <QDir>
#include <QFormLayout>
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QHostInfo>
#include <QAbstractItemModel>

using namespace ProjectExplorer;
using namespace Qnx;
using namespace Qnx::Internal;

namespace {
const char DEVICENAME_FIELD_ID[] = "DeviceName";

enum DeviceListUserRole
{
    ItemKindRole = Qt::UserRole, DeviceNameRole, DeviceIpRole, DeviceTypeRole
};
}

BlackBerryDeviceConfigurationWizardSetupPage::BlackBerryDeviceConfigurationWizardSetupPage(QWidget *parent)
    : QWizardPage(parent)
    , m_ui(new Ui::BlackBerryDeviceConfigurationWizardSetupPage)
    , m_deviceListDetector(new BlackBerryDeviceListDetector(this))

{
    m_ui->setupUi(this);
    setTitle(tr("Connection Details"));

    m_ui->debugToken->setExpectedKind(Utils::PathChooser::File);
    m_ui->debugToken->setPromptDialogFilter(QLatin1String("*.bar"));

    QString debugTokenBrowsePath = QnxUtils::dataDirPath();
    if (!QFileInfo(debugTokenBrowsePath).exists())
        debugTokenBrowsePath = QDir::homePath();
    m_ui->debugToken->setInitialBrowsePathBackup(debugTokenBrowsePath);

    connect(m_ui->deviceListWidget, SIGNAL(itemSelectionChanged()), this, SLOT(onDeviceSelectionChanged()));
    connect(m_deviceListDetector, SIGNAL(deviceDetected(QString,QString,bool)),
            this, SLOT(onDeviceDetected(QString,QString,bool)));
    connect(m_deviceListDetector, SIGNAL(finished()), this, SLOT(onDeviceListDetectorFinished()));
    connect(m_ui->deviceName, SIGNAL(textChanged(QString)), this, SIGNAL(completeChanged()));
    connect(m_ui->deviceHostIp, SIGNAL(textChanged(QString)), this, SIGNAL(completeChanged()));
    connect(m_ui->physicalDevice, SIGNAL(toggled(bool)), this, SIGNAL(completeChanged()));
    connect(m_ui->debugToken, SIGNAL(changed(QString)), this, SIGNAL(completeChanged()));
    connect(m_ui->requestButton, SIGNAL(clicked()), this, SLOT(requestDebugToken()));

    registerField(QLatin1String(DEVICENAME_FIELD_ID), m_ui->deviceName);
}

BlackBerryDeviceConfigurationWizardSetupPage::~BlackBerryDeviceConfigurationWizardSetupPage()
{
    delete m_ui;
    m_ui = 0;
}

void BlackBerryDeviceConfigurationWizardSetupPage::initializePage()
{
    m_ui->password->clear();
    refreshDeviceList();
}

void BlackBerryDeviceConfigurationWizardSetupPage::refreshDeviceList()
{
    m_ui->deviceListWidget->clear();

    QListWidgetItem *manual = createDeviceListItem(tr("Specify device manually"), SpecifyManually);
    m_ui->deviceListWidget->addItem(manual);
    manual->setSelected(true);

    QListWidgetItem *pleaseWait =
            createDeviceListItem(tr("Auto-detecting devices - please wait..."), PleaseWait);
    m_ui->deviceListWidget->addItem(pleaseWait);

    m_deviceListDetector->detectDeviceList();
}

void BlackBerryDeviceConfigurationWizardSetupPage::onDeviceListDetectorFinished()
{
    QListWidgetItem *pleaseWait = findDeviceListItem(PleaseWait);
    if (pleaseWait) {
        m_ui->deviceListWidget->removeItemWidget(pleaseWait);
        delete pleaseWait;
    }

    if (!findDeviceListItem(Autodetected)) {
        QListWidgetItem *note = createDeviceListItem(tr("No device has been auto-detected."), Note);
        note->setToolTip(tr("Device auto-detection is available in BB NDK 10.2. "
                            "Make sure that your device is in Development Mode."));
        m_ui->deviceListWidget->addItem(note);
    }
}

void BlackBerryDeviceConfigurationWizardSetupPage::onDeviceDetected(
        const QString &deviceName, const QString &hostName, bool isSimulator)
{
    QString displayName(deviceName);
    if (displayName != hostName)
        displayName.append(QLatin1String(" (")).append(hostName).append(QLatin1String(")"));

    QListWidgetItem *device = createDeviceListItem(displayName, Autodetected);
    device->setData(DeviceNameRole, displayName);
    device->setData(DeviceIpRole, hostName);
    device->setData(DeviceTypeRole, isSimulator);
    QListWidgetItem *pleaseWait = findDeviceListItem(PleaseWait);
    int row = pleaseWait ? m_ui->deviceListWidget->row(pleaseWait) : m_ui->deviceListWidget->count();
    m_ui->deviceListWidget->insertItem(row, device);
}

void BlackBerryDeviceConfigurationWizardSetupPage::onDeviceSelectionChanged()
{
    QList<QListWidgetItem *> selectedItems = m_ui->deviceListWidget->selectedItems();
    const QListWidgetItem *selected = selectedItems.count() == 1 ? selectedItems[0] : 0;
    const ItemKind itemKind = selected ? selected->data(ItemKindRole).value<ItemKind>() : Note;
    const bool isSimulator = selected && itemKind == Autodetected
            ? selected->data(DeviceTypeRole).toBool() : false;
    switch (itemKind) {
    case SpecifyManually:
        m_ui->deviceName->setEnabled(true);
        m_ui->deviceName->setText(tr("BlackBerry Device"));
        m_ui->deviceHostIp->setEnabled(true);
        m_ui->deviceHostIp->setText(QLatin1String("169.254.0.1"));
        m_ui->physicalDevice->setEnabled(true);
        m_ui->physicalDevice->setChecked(true);
        m_ui->simulator->setEnabled(true);
        m_ui->simulator->setChecked(false);
        m_ui->deviceHostIp->selectAll();
        m_ui->deviceHostIp->setFocus();
        break;
    case Autodetected:
        m_ui->deviceName->setEnabled(true);
        m_ui->deviceName->setText(selected->data(DeviceNameRole).toString());
        m_ui->deviceHostIp->setEnabled(false);
        m_ui->deviceHostIp->setText(selected->data(DeviceIpRole).toString());
        m_ui->physicalDevice->setEnabled(false);
        m_ui->physicalDevice->setChecked(!isSimulator);
        m_ui->simulator->setEnabled(false);
        m_ui->simulator->setChecked(isSimulator);
        m_ui->password->setFocus();
        break;
    case PleaseWait:
    case Note:
        m_ui->deviceName->setEnabled(false);
        m_ui->deviceName->clear();
        m_ui->deviceHostIp->setEnabled(false);
        m_ui->deviceHostIp->clear();
        m_ui->physicalDevice->setEnabled(false);
        m_ui->physicalDevice->setChecked(false);
        m_ui->simulator->setEnabled(false);
        m_ui->simulator->setChecked(false);
        break;
    }
}

QListWidgetItem *BlackBerryDeviceConfigurationWizardSetupPage::createDeviceListItem(
        const QString &displayName, ItemKind itemKind) const
{
    QListWidgetItem *item = new QListWidgetItem(displayName);
    if (itemKind == PleaseWait || itemKind == Note) {
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        QFont font = item->font();
        font.setItalic(true);
        item->setFont(font);
    }
    item->setData(ItemKindRole, QVariant::fromValue(itemKind));
    return item;
}

QListWidgetItem *BlackBerryDeviceConfigurationWizardSetupPage::findDeviceListItem(ItemKind itemKind) const
{
    int count = m_ui->deviceListWidget->count();
    for (int i = 0; i < count; ++i) {
        QListWidgetItem *item = m_ui->deviceListWidget->item(i);
        if (item->data(ItemKindRole).value<ItemKind>() == itemKind) {
            return item;
        }
    }
    return 0;
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

void BlackBerryDeviceConfigurationWizardSetupPage::requestDebugToken()
{
    BlackBerryDebugTokenRequestDialog dialog;

    if (!m_ui->deviceHostIp->text().isEmpty() && !m_ui->password->text().isEmpty())
        dialog.setTargetDetails(m_ui->deviceHostIp->text(), m_ui->password->text());

    const int result = dialog.exec();

    if (result != QDialog::Accepted)
        return;

    m_ui->debugToken->setPath(dialog.debugToken());
}

// ----------------------------------------------------------------------------


BlackBerryDeviceConfigurationWizardSshKeyPage::BlackBerryDeviceConfigurationWizardSshKeyPage(QWidget *parent)
    : QWizardPage(parent)
    , m_ui(new Ui::BlackBerryDeviceConfigurationWizardSshKeyPage)
{
    m_ui->setupUi(this);

    m_ui->privateKey->setExpectedKind(Utils::PathChooser::File);
    m_ui->progressBar->hide();

    QString initialBrowsePath = QnxUtils::dataDirPath();
    if (!QFileInfo(initialBrowsePath).exists())
        initialBrowsePath = QDir::homePath();
    m_ui->privateKey->setInitialBrowsePathBackup(initialBrowsePath);

    setTitle(tr("SSH Key Setup"));
    setSubTitle(tr("Please select an existing <b>4096</b>-bit key or click <b>Generate</b> to create a new one."));

    connect(m_ui->privateKey, SIGNAL(changed(QString)), this, SLOT(findMatchingPublicKey(QString)));
    connect(m_ui->privateKey, SIGNAL(changed(QString)), this, SIGNAL(completeChanged()));
    connect(m_ui->generate, SIGNAL(clicked()), this, SLOT(generateSshKeys()));
}

BlackBerryDeviceConfigurationWizardSshKeyPage::~BlackBerryDeviceConfigurationWizardSshKeyPage()
{
    delete m_ui;
    m_ui = 0;
}

void BlackBerryDeviceConfigurationWizardSshKeyPage::initializePage()
{
}

bool BlackBerryDeviceConfigurationWizardSshKeyPage::isComplete() const
{
    QFileInfo privateKeyFi(m_ui->privateKey->fileName().toString());
    QFileInfo publicKeyFi(m_ui->publicKey->text());

    return privateKeyFi.exists() && publicKeyFi.exists();
}

QString BlackBerryDeviceConfigurationWizardSshKeyPage::privateKey() const
{
    return m_ui->privateKey->fileName().toString();
}

QString BlackBerryDeviceConfigurationWizardSshKeyPage::publicKey() const
{
    return m_ui->publicKey->text();
}

void BlackBerryDeviceConfigurationWizardSshKeyPage::findMatchingPublicKey(const QString &privateKeyPath)
{
    const QString candidate = privateKeyPath + QLatin1String(".pub");
    if (QFileInfo(candidate).exists())
        m_ui->publicKey->setText(QDir::toNativeSeparators(candidate));
    else
        m_ui->publicKey->clear();
}

void BlackBerryDeviceConfigurationWizardSshKeyPage::sshKeysGenerationFailed(const QString &error)
{
    setBusy(false);
    QMessageBox::critical(this, tr("Key Generation Failed"), error);
}

void BlackBerryDeviceConfigurationWizardSshKeyPage::processSshKeys(const QString &privateKeyPath, const QByteArray &privateKey, const QByteArray &publicKey)
{
    setBusy(false);

    const QString publicKeyPath = privateKeyPath + QLatin1String(".pub");

    if (!saveKeys(privateKey, publicKey, privateKeyPath, publicKeyPath)) // saveKeys(..) will show an error message if necessary
        return;

    m_ui->privateKey->setFileName(Utils::FileName::fromString(privateKeyPath));
    m_ui->publicKey->setText(QDir::toNativeSeparators(publicKeyPath));

    emit completeChanged();
}

bool BlackBerryDeviceConfigurationWizardSshKeyPage::saveKeys(const QByteArray &privateKey, const QByteArray &publicKey, const QString &privateKeyPath, const QString &publicKeyPath)
{
    Utils::FileSaver privSaver(privateKeyPath);
    privSaver.write(privateKey);
    if (!privSaver.finalize(this))
        return false; // finalize shows an error message if necessary
    QFile::setPermissions(privateKeyPath, QFile::ReadOwner | QFile::WriteOwner);

    Utils::FileSaver pubSaver(publicKeyPath);

    pubSaver.write(publicKey);
    if (!pubSaver.finalize(this))
        return false;

    return true;
}

void BlackBerryDeviceConfigurationWizardSshKeyPage::generateSshKeys()
{
    QString lookInDir = QnxUtils::dataDirPath();
    if (!QFileInfo(lookInDir).exists())
        lookInDir = QDir::homePath();

    QString privateKeyPath = QFileDialog::getSaveFileName(this, tr("Choose Private Key File Name"), lookInDir);
    if (privateKeyPath.isEmpty())
        return;

    setBusy(true);
    BlackBerrySshKeysGenerator *sshKeysGenerator = new BlackBerrySshKeysGenerator(privateKeyPath);
    connect(sshKeysGenerator, SIGNAL(sshKeysGenerationFailed(QString)), this, SLOT(sshKeysGenerationFailed(QString)), Qt::QueuedConnection);
    connect(sshKeysGenerator, SIGNAL(sshKeysGenerationFinished(QString,QByteArray,QByteArray)), this, SLOT(processSshKeys(QString,QByteArray,QByteArray)), Qt::QueuedConnection);
    sshKeysGenerator->start();
}

void BlackBerryDeviceConfigurationWizardSshKeyPage::setBusy(bool busy)
{
    m_ui->privateKey->setEnabled(!busy);
    m_ui->publicKey->setEnabled(!busy);
    m_ui->generate->setEnabled(!busy);
    m_ui->progressBar->setVisible(busy);

    wizard()->button(QWizard::BackButton)->setEnabled(!busy);
    wizard()->button(QWizard::NextButton)->setEnabled(!busy);
    wizard()->button(QWizard::FinishButton)->setEnabled(!busy);
    wizard()->button(QWizard::CancelButton)->setEnabled(!busy);
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



