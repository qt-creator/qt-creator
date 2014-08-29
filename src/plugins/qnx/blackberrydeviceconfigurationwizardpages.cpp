/**************************************************************************
**
** Copyright (C) 2012 - 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
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
#include "blackberrydebugtokenrequestdialog.h"
#include "blackberrydebugtokenreader.h"
#include "blackberrysshkeysgenerator.h"
#include "blackberrydeviceinformation.h"
#include "ui_blackberrydeviceconfigurationwizardsetuppage.h"
#include "ui_blackberrydeviceconfigurationwizardquerypage.h"
#include "ui_blackberrydeviceconfigurationwizardconfigpage.h"
#include "blackberrydeviceconnectionmanager.h"
#include "blackberrysigningutils.h"
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
const char DEVICEHOSTNAME_FIELD_ID[] = "DeviceHostName";
const char DEVICEPASSWORD_FIELD_ID[] = "DevicePassword";
const char CONFIGURATIONNAME_FIELD_ID[] = "ConfigurationName";
const char DEBUGTOKENPATH_FIELD_ID[] = "DebugTokenPath";

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
    setTitle(tr("Connection"));

    connect(m_ui->deviceListWidget, SIGNAL(itemSelectionChanged()), this, SLOT(onDeviceSelectionChanged()));
    connect(m_deviceListDetector, SIGNAL(deviceDetected(QString,QString,bool)),
            this, SLOT(onDeviceDetected(QString,QString,bool)));
    connect(m_deviceListDetector, SIGNAL(finished()), this, SLOT(onDeviceListDetectorFinished()));
    connect(m_ui->deviceHostIp, SIGNAL(textChanged(QString)), this, SIGNAL(completeChanged()));

    registerField(QLatin1String(DEVICEHOSTNAME_FIELD_ID), m_ui->deviceHostIp);
    registerField(QLatin1String(DEVICEPASSWORD_FIELD_ID), m_ui->password);
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
        displayName.append(QLatin1String(" (")).append(hostName).append(QLatin1Char(')'));

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
    switch (itemKind) {
    case SpecifyManually:
        m_ui->deviceHostIp->setEnabled(true);
        m_ui->deviceHostIp->setText(QLatin1String("169.254.0.1"));
        m_ui->password->setEnabled(true);
        m_ui->deviceHostIp->selectAll();
        m_ui->deviceHostIp->setFocus();
        break;
    case Autodetected:
        m_ui->deviceHostIp->setEnabled(false);
        m_ui->deviceHostIp->setText(selected->data(DeviceIpRole).toString());
        m_ui->password->setEnabled(true);
        m_ui->password->setFocus();
        break;
    case PleaseWait:
    case Note:
        m_ui->deviceHostIp->setEnabled(false);
        m_ui->deviceHostIp->clear();
        m_ui->password->setEnabled(false);
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
        if (item->data(ItemKindRole).value<ItemKind>() == itemKind)
            return item;
    }
    return 0;
}

bool BlackBerryDeviceConfigurationWizardSetupPage::isComplete() const
{
    return !m_ui->deviceHostIp->text().isEmpty();
}

QString BlackBerryDeviceConfigurationWizardSetupPage::hostName() const
{
    return m_ui->deviceHostIp->text();
}

QString BlackBerryDeviceConfigurationWizardSetupPage::password() const
{
    return m_ui->password->text();
}

// ----------------------------------------------------------------------------

BlackBerryDeviceConfigurationWizardQueryPage::BlackBerryDeviceConfigurationWizardQueryPage
        (BlackBerryDeviceConfigurationWizardHolder &holder, QWidget *parent)
    : QWizardPage(parent)
    , m_ui(new Ui::BlackBerryDeviceConfigurationWizardQueryPage)
    , m_holder(holder)
    , m_deviceInformation(new BlackBerryDeviceInformation(this))
{
    m_ui->setupUi(this);
    setTitle(tr("Device Information"));
    m_ui->progressBar->setMaximum(Done);

    connect(m_deviceInformation,SIGNAL(finished(int)),this,SLOT(processQueryFinished(int)));
}

BlackBerryDeviceConfigurationWizardQueryPage::~BlackBerryDeviceConfigurationWizardQueryPage()
{
    delete m_ui;
    m_ui = 0;
}

void BlackBerryDeviceConfigurationWizardQueryPage::initializePage()
{
    m_holder.deviceInfoRetrieved = false;

    setState(Querying, tr("Querying device information. Please wait..."));

    m_deviceInformation->setDeviceTarget(
                field(QLatin1String(DEVICEHOSTNAME_FIELD_ID)).toString(),
                field(QLatin1String(DEVICEPASSWORD_FIELD_ID)).toString());
}

void BlackBerryDeviceConfigurationWizardQueryPage::processQueryFinished(int status)
{
    m_holder.deviceInfoRetrieved = status == BlackBerryDeviceInformation::Success;
    m_holder.devicePin = m_deviceInformation->devicePin();
    m_holder.scmBundle = m_deviceInformation->scmBundle();
    m_holder.deviceName = m_deviceInformation->hostName();
    if (m_holder.deviceName.isEmpty())
        m_holder.deviceName = QLatin1String("BlackBerry at ")
                + field(QLatin1String(DEVICEHOSTNAME_FIELD_ID)).toString();
    m_holder.debugTokenAuthor = m_deviceInformation->debugTokenAuthor();
    m_holder.debugTokenValid = m_deviceInformation->debugTokenValid();
    m_holder.isSimulator = m_deviceInformation->isSimulator();
    m_holder.isProductionDevice = m_deviceInformation->isProductionDevice();

    if (m_holder.deviceInfoRetrieved)
        checkAndGenerateSSHKeys();
    else
        setState(Done, tr("Cannot connect to the device. "
                "Check that the device is in development mode and has matching host name and password."));
}

void BlackBerryDeviceConfigurationWizardQueryPage::checkAndGenerateSSHKeys()
{
    if (! BlackBerryDeviceConnectionManager::instance()->hasValidSSHKeys()) {
        setState(GeneratingSshKey, tr("Generating SSH keys. Please wait..."));

        BlackBerrySshKeysGenerator *sshKeysGenerator = new BlackBerrySshKeysGenerator();
        connect(sshKeysGenerator, SIGNAL(sshKeysGenerationFailed(QString)),
                this, SLOT(sshKeysGenerationFailed(QString)), Qt::QueuedConnection);
        connect(sshKeysGenerator, SIGNAL(sshKeysGenerationFinished(QByteArray,QByteArray)),
                this, SLOT(processSshKeys(QByteArray,QByteArray)), Qt::QueuedConnection);
        sshKeysGenerator->start();
    } else
        queryDone();
}

void BlackBerryDeviceConfigurationWizardQueryPage::sshKeysGenerationFailed(const QString &error)
{
    // this slot can be called asynchronously - processing it only in GeneratingSshKey state
    if (m_state != GeneratingSshKey)
        return;

    QString message = tr("Failed generating SSH key needed for securing connection to a device. Error:");
    message += QLatin1Char(' ');
    message.append(error);
    setState(Done, message);
}

void BlackBerryDeviceConfigurationWizardQueryPage::processSshKeys(const QByteArray &privateKey,
                                                                  const QByteArray &publicKey)
{
    // this slot can be called asynchronously - processing it only in GeneratingSshKey state
    if (m_state != GeneratingSshKey)
        return;

    // condition prevents overriding already generated SSH keys
    // this may happens when an user enter the QueryPage several times before
    // the first SSH keys are generated i.e. when multiple calls of checkAndGenerateSSHKeys()
    // before processSshKeys() is called, multiple processSshKeys() calls are triggered later.
    // only the first one is allowed to write the SSH keys.
    if (! BlackBerryDeviceConnectionManager::instance()->hasValidSSHKeys()) {
        QString error;
        if (!BlackBerryDeviceConnectionManager::instance()->setSSHKeys(privateKey, publicKey, &error)) {
            QString message = tr("Failed saving SSH key needed for securing connection to a device. Error:");
            message += QLatin1Char(' ');
            message.append(error);
            setState(Done, message);
            return;
        }
    }

    queryDone();
}

void BlackBerryDeviceConfigurationWizardQueryPage::queryDone()
{
    setState(Done, tr("Device information retrieved successfully."));
}

void BlackBerryDeviceConfigurationWizardQueryPage::setState(QueryState state, const QString &message)
{
    m_state = state;
    m_ui->statusLabel->setText(message);
    m_ui->progressBar->setVisible(state != Done);
    m_ui->progressBar->setValue(state);
    emit completeChanged();

    if (isComplete())
        if (wizard()->currentPage() == this)
            wizard()->next();
}

bool BlackBerryDeviceConfigurationWizardQueryPage::isComplete() const
{
    return m_state == Done
            && m_holder.deviceInfoRetrieved
            && BlackBerryDeviceConnectionManager::instance()->hasValidSSHKeys();
}

// ----------------------------------------------------------------------------

BlackBerryDeviceConfigurationWizardConfigPage::BlackBerryDeviceConfigurationWizardConfigPage
        (BlackBerryDeviceConfigurationWizardHolder &holder, QWidget *parent)
    : QWizardPage(parent)
    , m_ui(new Ui::BlackBerryDeviceConfigurationWizardConfigPage)
    , m_holder(holder)
    , m_utils(BlackBerrySigningUtils::instance())
{
    m_ui->setupUi(this);
    setTitle(tr("Configuration"));

    m_ui->debugTokenCombo->addItems(m_utils.debugTokens());

    connect(m_ui->configurationNameField, SIGNAL(textChanged(QString)), this, SIGNAL(completeChanged()));
    connect(m_ui->debugTokenCombo, SIGNAL(currentTextChanged(QString)), this, SIGNAL(completeChanged()));
    connect(m_ui->generateButton, SIGNAL(clicked()), this, SLOT(generateDebugToken()));
    connect(m_ui->importButton, SIGNAL(clicked()), this, SLOT(importDebugToken()));

    registerField(QLatin1String(CONFIGURATIONNAME_FIELD_ID), m_ui->configurationNameField);
    registerField(QLatin1String(DEBUGTOKENPATH_FIELD_ID), m_ui->debugTokenCombo);
}

BlackBerryDeviceConfigurationWizardConfigPage::~BlackBerryDeviceConfigurationWizardConfigPage()
{
    delete m_ui;
    m_ui = 0;
}

void BlackBerryDeviceConfigurationWizardConfigPage::initializePage()
{
    QString deviceHostName = field(QLatin1String(DEVICEHOSTNAME_FIELD_ID)).toString();
    m_ui->configurationNameField->setText(m_holder.deviceName);
    m_ui->deviceHostNameField->setText(deviceHostName);
    m_ui->deviceTypeField->setText(QLatin1String (m_holder.isSimulator ? "Simulator" : "Device"));
    m_ui->debugTokenCombo->setEnabled(!m_holder.isSimulator);
    m_ui->generateButton->setEnabled(!m_holder.isSimulator);
}

bool BlackBerryDeviceConfigurationWizardConfigPage::isComplete() const
{
    bool configurationNameComplete = !m_ui->configurationNameField->text().isEmpty();
    Utils::FileName fileName = Utils::FileName::fromString(m_ui->debugTokenCombo->currentText());
    bool debugTokenComplete = m_holder.isSimulator || !m_holder.isProductionDevice
            || (!fileName.isEmpty() && fileName.toFileInfo().exists());

    return configurationNameComplete  &&  debugTokenComplete;
}

void BlackBerryDeviceConfigurationWizardConfigPage::generateDebugToken()
{
    BlackBerryDebugTokenRequestDialog dialog;
    dialog.setDevicePin(m_holder.devicePin);

    const int result = dialog.exec();

    if (result != QDialog::Accepted)
        return;

    m_utils.addDebugToken(dialog.debugToken());
    m_ui->debugTokenCombo->addItem(dialog.debugToken());
    const int index = m_ui->debugTokenCombo->findText(dialog.debugToken());
    if (index != -1)
        m_ui->debugTokenCombo->setCurrentIndex(index);
}

void BlackBerryDeviceConfigurationWizardConfigPage::importDebugToken()
{
    const QString debugToken = QFileDialog::getOpenFileName(this, tr("Select Debug Token"),
                                                            QString(), tr("BAR file (*.bar)"));

    if (debugToken.isEmpty())
        return;

    BlackBerryDebugTokenReader debugTokenReader(debugToken);
    if (!debugTokenReader.isValid()) {
        QMessageBox::warning(this, tr("Invalid Debug Token"),
                             tr("Debug token file %1 cannot be read.").arg(debugToken));
        return;
    }

    m_utils.addDebugToken(debugToken);
    m_ui->debugTokenCombo->addItem(debugToken);
    const int index = m_ui->debugTokenCombo->findText(debugToken);
    if (index != -1)
        m_ui->debugTokenCombo->setCurrentIndex(index);
}

QString BlackBerryDeviceConfigurationWizardConfigPage::configurationName() const
{
    return m_ui->configurationNameField->text();
}

QString BlackBerryDeviceConfigurationWizardConfigPage::debugToken() const
{
    return m_ui->debugTokenCombo->currentText();
}

// ----------------------------------------------------------------------------

BlackBerryDeviceConfigurationWizardFinalPage::BlackBerryDeviceConfigurationWizardFinalPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Summary"));

    QVBoxLayout *layout = new QVBoxLayout(this);
    QLabel *label = new QLabel(tr("The new device configuration will be created now."), this);
    layout->addWidget(label);
}
