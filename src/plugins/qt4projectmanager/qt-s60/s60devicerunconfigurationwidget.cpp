/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "s60devicerunconfigurationwidget.h"
#include "s60devicerunconfiguration.h"
#include "s60runconfigbluetoothstarter.h"
#include "bluetoothlistener_gui.h"
#include "s60manager.h"
#include "launcher.h"
#include "bluetoothlistener.h"
#include "bluetoothlistener_gui.h"
#include "serialdevicelister.h"

#include <utils/detailswidget.h>
#include <utils/qtcassert.h>
#include <utils/pathchooser.h>

#include <QtCore/QTimer>
#include <QtCore/QDir>
#include <QtGui/QRadioButton>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QComboBox>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QFormLayout>
#include <QtGui/QToolButton>
#include <QtGui/QStyle>
#include <QtGui/QApplication>
#include <QtGui/QSpacerItem>
#include <QtGui/QMainWindow>
#include <QtGui/QMessageBox>

Q_DECLARE_METATYPE(Qt4ProjectManager::Internal::CommunicationDevice)

namespace Qt4ProjectManager {
namespace Internal {

S60DeviceRunConfigurationWidget::S60DeviceRunConfigurationWidget(
            S60DeviceRunConfiguration *runConfiguration,
            QWidget *parent)
    : QWidget(parent),
    m_runConfiguration(runConfiguration),
    m_detailsWidget(new Utils::DetailsWidget),
    m_serialPortsCombo(new QComboBox),
    m_nameLineEdit(new QLineEdit(m_runConfiguration->displayName())),
    m_sisxFileLabel(new QLabel),
    m_deviceInfoButton(new QToolButton),
    m_deviceInfoDescriptionLabel(new QLabel(tr("Device:"))),
    m_deviceInfoLabel(new QLabel),
    m_infoTimeOutTimer(0)
{
    updateTargetInformation();
    QVBoxLayout *mainBoxLayout = new QVBoxLayout();
    mainBoxLayout->setMargin(0);
    setLayout(mainBoxLayout);
    mainBoxLayout->addWidget(m_detailsWidget);
    QWidget *detailsContainer = new QWidget;
    m_detailsWidget->setWidget(detailsContainer);

    QVBoxLayout *detailsBoxLayout = new QVBoxLayout();
    detailsBoxLayout->setMargin(0);
    detailsContainer->setLayout(detailsBoxLayout);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setMargin(0);
    detailsBoxLayout->addLayout(formLayout);
    // Name control
    QLabel *nameLabel = new QLabel(tr("Name:"));
    nameLabel->setBuddy(m_nameLineEdit);
    formLayout->addRow(nameLabel, m_nameLineEdit);
    formLayout->addRow(tr("Install File:"), m_sisxFileLabel);

    updateSerialDevices();    
    connect(S60Manager::instance()->serialDeviceLister(), SIGNAL(updated()),
            this, SLOT(updateSerialDevices()));
    // Serial devices control
    connect(m_serialPortsCombo, SIGNAL(activated(int)), this, SLOT(setSerialPort(int)));
    QHBoxLayout *serialPortHBoxLayout = new QHBoxLayout;
    serialPortHBoxLayout->addWidget(m_serialPortsCombo);
    serialPortHBoxLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Ignored));

    formLayout->addRow(tr("Device on Serial Port:"), serialPortHBoxLayout);

    // Device Info with button. Widgets are enabled in above call to updateSerialDevices()
    QHBoxLayout *infoHBoxLayout = new QHBoxLayout;
    m_deviceInfoLabel->setWordWrap(true);
    m_deviceInfoLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    infoHBoxLayout->addWidget(m_deviceInfoLabel);
    infoHBoxLayout->addWidget(m_deviceInfoButton);
    m_deviceInfoButton->setIcon(qApp->style()->standardIcon(QStyle::SP_MessageBoxInformation));
    m_deviceInfoButton->setToolTip(tr("Queries the device for information"));
    connect(m_deviceInfoButton, SIGNAL(clicked()), this, SLOT(updateDeviceInfo()));
    formLayout->addRow(m_deviceInfoDescriptionLabel, infoHBoxLayout);

    // Signature/certificate stuff.
    QWidget *signatureWidget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout();
    signatureWidget->setLayout(layout);
    detailsBoxLayout->addWidget(signatureWidget);
    QRadioButton *selfSign = new QRadioButton(tr("Self-signed certificate"));
    QHBoxLayout *customHBox = new QHBoxLayout();
    customHBox->setMargin(0);
    QVBoxLayout *radioLayout = new QVBoxLayout();
    QRadioButton *customSignature = new QRadioButton();
    radioLayout->addWidget(customSignature);
    radioLayout->addStretch(10);
    customHBox->addLayout(radioLayout);
    QFormLayout *customLayout = new QFormLayout();
    customLayout->setMargin(0);
    customLayout->setLabelAlignment(Qt::AlignRight);
    Utils::PathChooser *signaturePath = new Utils::PathChooser();
    signaturePath->setExpectedKind(Utils::PathChooser::File);
    signaturePath->setPromptDialogTitle(tr("Choose certificate file (.cer)"));
    customLayout->addRow(new QLabel(tr("Custom certificate:")), signaturePath);
    Utils::PathChooser *keyPath = new Utils::PathChooser();
    keyPath->setExpectedKind(Utils::PathChooser::File);
    keyPath->setPromptDialogTitle(tr("Choose key file (.key / .pem)"));
    customLayout->addRow(new QLabel(tr("Key file:")), keyPath);
    customHBox->addLayout(customLayout);
    customHBox->addStretch(10);
    layout->addWidget(selfSign);
    layout->addLayout(customHBox);
    layout->addStretch(10);

    switch (m_runConfiguration->signingMode()) {
    case S60DeviceRunConfiguration::SignSelf:
        selfSign->setChecked(true);
        break;
    case S60DeviceRunConfiguration::SignCustom:
        customSignature->setChecked(true);
        break;
    }

    signaturePath->setPath(m_runConfiguration->customSignaturePath());
    keyPath->setPath(m_runConfiguration->customKeyPath());

    connect(m_nameLineEdit, SIGNAL(textEdited(QString)),
            this, SLOT(displayNameEdited(QString)));
    connect(m_runConfiguration, SIGNAL(targetInformationChanged()),
            this, SLOT(updateTargetInformation()));
    connect(selfSign, SIGNAL(toggled(bool)), this, SLOT(selfSignToggled(bool)));
    connect(customSignature, SIGNAL(toggled(bool)), this, SLOT(customSignatureToggled(bool)));
    connect(signaturePath, SIGNAL(changed(QString)), this, SLOT(signaturePathChanged(QString)));
    connect(keyPath, SIGNAL(changed(QString)), this, SLOT(keyPathChanged(QString)));
    updateSummary();
}

void S60DeviceRunConfigurationWidget::updateSerialDevices()
{
    m_serialPortsCombo->clear();
    clearDeviceInfo();
    const QString previousRunConfigurationPortName = m_runConfiguration->serialPortName();
    const QList<CommunicationDevice> devices = S60Manager::instance()->serialDeviceLister()->communicationDevices();
    int newIndex = -1;
    for (int i = 0; i < devices.size(); ++i) {
        const CommunicationDevice &device = devices.at(i);
        m_serialPortsCombo->addItem(device.friendlyName, qVariantFromValue(device));
        if (device.portName == previousRunConfigurationPortName)
            newIndex = i;
    }
    // Set new index: prefer to keep old or set to 0, if available.
    if (newIndex == -1 && !devices.empty())
        newIndex = 0;
    m_serialPortsCombo->setCurrentIndex(newIndex);
    if (newIndex == -1) {
        m_deviceInfoButton->setEnabled(false);
        m_runConfiguration->setSerialPortName(QString());
    } else {
        m_deviceInfoButton->setEnabled(true);
        const QString newPortName = device(newIndex).portName;
        if (newPortName != previousRunConfigurationPortName)
            m_runConfiguration->setSerialPortName(newPortName);
    }
    updateSummary();
}

CommunicationDevice S60DeviceRunConfigurationWidget::device(int i) const
{
    if (i >= 0) {
        const QVariant data = m_serialPortsCombo->itemData(i);
        if (qVariantCanConvert<Qt4ProjectManager::Internal::CommunicationDevice>(data))
            return qVariantValue<Qt4ProjectManager::Internal::CommunicationDevice>(data);
    }
    return CommunicationDevice(SerialPortCommunication);
}

CommunicationDevice S60DeviceRunConfigurationWidget::currentDevice() const
{
    return device(m_serialPortsCombo->currentIndex());
}

void S60DeviceRunConfigurationWidget::displayNameEdited(const QString &text)
{
    m_runConfiguration->setDisplayName(text);
}

void S60DeviceRunConfigurationWidget::updateTargetInformation()
{
    m_sisxFileLabel->setText(QDir::toNativeSeparators(m_runConfiguration->basePackageFilePath()
                                                      + QLatin1String(".sisx")));
}

void S60DeviceRunConfigurationWidget::setSerialPort(int index)
{
    const CommunicationDevice d = device(index);
    m_runConfiguration->setSerialPortName(d.portName);
    m_runConfiguration->setCommunicationType(d.type);
    m_deviceInfoButton->setEnabled(index >= 0);
    clearDeviceInfo();
    updateSummary();
}

void S60DeviceRunConfigurationWidget::selfSignToggled(bool toggle)
{
    if (toggle)
        m_runConfiguration->setSigningMode(S60DeviceRunConfiguration::SignSelf);
    updateSummary();
}

void S60DeviceRunConfigurationWidget::customSignatureToggled(bool toggle)
{
    if (toggle)
        m_runConfiguration->setSigningMode(S60DeviceRunConfiguration::SignCustom);
    updateSummary();
}

void S60DeviceRunConfigurationWidget::signaturePathChanged(const QString &path)
{
    m_runConfiguration->setCustomSignaturePath(path);
    updateSummary();
}

void S60DeviceRunConfigurationWidget::keyPathChanged(const QString &path)
{
    m_runConfiguration->setCustomKeyPath(path);
    updateSummary();
}

void S60DeviceRunConfigurationWidget::updateSummary()
{
    //: Summary text of S60 device run configuration
    const QString device = m_serialPortsCombo->currentIndex() != -1 ?
                           m_serialPortsCombo->currentText() :
                           tr("<No Device>");
    const QString signature = m_runConfiguration->signingMode() == S60DeviceRunConfiguration::SignCustom ?
                              tr("(custom certificate)") :
                              tr("(self-signed certificate)");    
    m_detailsWidget->setSummaryText(tr("Summary: Run on '%1' %2").arg(device, signature));
}

void S60DeviceRunConfigurationWidget::clearDeviceInfo()
{
    // Restore text & color
    m_deviceInfoLabel->clear();
    m_deviceInfoLabel->setStyleSheet(QString());
}

void S60DeviceRunConfigurationWidget::setDeviceInfoLabel(const QString &message, bool isError)
{
    m_deviceInfoLabel->setStyleSheet(isError ?
                                     QString(QLatin1String("background-color: red;")) :
                                     QString());
    m_deviceInfoLabel->setText(message);
    m_deviceInfoLabel->adjustSize();
}

void S60DeviceRunConfigurationWidget::slotLauncherStateChanged(int s)
{
    switch (s) {
    case trk::Launcher::WaitingForTrk: {
        // Entered trk wait state..open message box
        QMessageBox *mb = S60DeviceRunControlBase::createTrkWaitingMessageBox(m_infoLauncher->trkServerName(), this);
        connect(m_infoLauncher, SIGNAL(stateChanged(int)), mb, SLOT(close()));
        connect(mb, SIGNAL(finished(int)), this, SLOT(slotWaitingForTrkClosed()));
        mb->open();
    }
        break;
    case trk::Launcher::DeviceDescriptionReceived: // All ok, done
        setDeviceInfoLabel(m_infoLauncher->deviceDescription());
        m_deviceInfoButton->setEnabled(true);
        m_infoLauncher->deleteLater();
        break;
    }
}

void S60DeviceRunConfigurationWidget::slotWaitingForTrkClosed()
{
    if (m_infoLauncher && m_infoLauncher->state() == trk::Launcher::WaitingForTrk) {
        m_infoLauncher->deleteLater();
        clearDeviceInfo();
        m_deviceInfoButton->setEnabled(true);
    }
}

void S60DeviceRunConfigurationWidget::updateDeviceInfo()
{
    QTC_ASSERT(!m_infoLauncher, return)
    setDeviceInfoLabel(tr("Connecting..."));
    // Do a launcher run with the ping protocol. Prompt to connect and
    // go asynchronous afterwards to pop up launch trk box if a timeout occurs.
    m_infoLauncher = new trk::Launcher(trk::Launcher::ActionPingOnly, QSharedPointer<trk::TrkDevice>(), this);
    connect(m_infoLauncher, SIGNAL(stateChanged(int)), this, SLOT(slotLauncherStateChanged(int)));
    const CommunicationDevice commDev = currentDevice();
    m_infoLauncher->setSerialFrame(commDev.type == SerialPortCommunication);
    m_infoLauncher->setTrkServerName(commDev.portName);
    // Prompt user
    QString message;
    const trk::PromptStartCommunicationResult src =
            S60RunConfigBluetoothStarter::startCommunication(m_infoLauncher->trkDevice(),
                                                             commDev.portName,
                                                             commDev.type, this,
                                                             &message);
    switch (src) {
    case trk::PromptStartCommunicationConnected:
        break;
    case trk::PromptStartCommunicationCanceled:
        clearDeviceInfo();
        m_infoLauncher->deleteLater();
        return;
    case trk::PromptStartCommunicationError:
        setDeviceInfoLabel(message, true);
        m_infoLauncher->deleteLater();
        return;
    };
    if (!m_infoLauncher->startServer(&message)) {
        setDeviceInfoLabel(message, true);
        m_infoLauncher->deleteLater();        
        return;
    }
    // Wait for either timeout or results
    m_deviceInfoButton->setEnabled(false);
    return;
}

} // namespace Internal
} // namespace Qt4ProjectManager
