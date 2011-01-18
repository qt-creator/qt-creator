/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "s60deployconfigurationwidget.h"
#include "s60deployconfiguration.h"
#include "s60devicerunconfiguration.h"

#include "s60runconfigbluetoothstarter.h"
#include <symbianutils/bluetoothlistener_gui.h>

#include <symbianutils/launcher.h>
#include <symbianutils/bluetoothlistener.h>
#include <symbianutils/symbiandevicemanager.h>

#include "trkruncontrol.h"

#include <utils/detailswidget.h>
#include <utils/ipaddresslineedit.h>
#include <utils/qtcassert.h>
#include <utils/pathchooser.h>

#include <QtCore/QDir>
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
#include <QtGui/QMessageBox>
#include <QtGui/QCheckBox>
#include <QtGui/QGroupBox>
#include <QtGui/QRadioButton>
#include <QtGui/QValidator>

Q_DECLARE_METATYPE(SymbianUtils::SymbianDevice)

namespace Qt4ProjectManager {
namespace Internal {

const char STARTING_DRIVE_LETTER = 'C';
const char LAST_DRIVE_LETTER = 'Z';

S60DeployConfigurationWidget::S60DeployConfigurationWidget(QWidget *parent)
    : ProjectExplorer::DeployConfigurationWidget(parent),
      m_detailsWidget(new Utils::DetailsWidget),
      m_serialPortsCombo(new QComboBox),
      m_sisFileLabel(new QLabel),
      m_deviceInfoButton(new QToolButton),
      m_deviceInfoDescriptionLabel(new QLabel(tr("Device:"))),
      m_deviceInfoLabel(new QLabel),
      m_installationDriveCombo(new QComboBox()),
      m_silentInstallCheckBox(new QCheckBox(tr("Silent installation"))),
      m_serialRadioButton(new QRadioButton(tr("Serial:"))),
      m_wlanRadioButton(new QRadioButton(tr("Experimental WLAN:"))), //TODO: Remove ""Experimental" when CODA is stable and official
      m_ipAddress(new Utils::IpAddressLineEdit)
{
}

S60DeployConfigurationWidget::~S60DeployConfigurationWidget()
{
}

void S60DeployConfigurationWidget::init(ProjectExplorer::DeployConfiguration *dc)
{
    m_deployConfiguration = qobject_cast<S60DeployConfiguration *>(dc);

    m_detailsWidget->setState(Utils::DetailsWidget::NoSummary);

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
    formLayout->addRow(tr("Installation file:"), m_sisFileLabel);

    // Installation Drive control
    updateInstallationDrives();

    QHBoxLayout *installationBoxLayout = new QHBoxLayout();
    m_installationDriveCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    connect(m_installationDriveCombo, SIGNAL(activated(int)), this, SLOT(setInstallationDrive(int)));
    QHBoxLayout *installationDriveHBoxLayout = new QHBoxLayout;
    installationDriveHBoxLayout->addWidget(m_installationDriveCombo);
    installationBoxLayout->addLayout(installationDriveHBoxLayout);

    // Non-silent installs are a fallback if one wants to override missing dependencies.
    m_silentInstallCheckBox->setChecked(m_deployConfiguration->silentInstall());
    m_silentInstallCheckBox->setToolTip(tr("Silent installation is an installation mode "
                                           "that does not require user's intervention. "
                                           "In case it fails the non silent installation is launched."));
    connect(m_silentInstallCheckBox, SIGNAL(stateChanged(int)), this, SLOT(silentInstallChanged(int)));
    installationBoxLayout->addWidget(m_silentInstallCheckBox);
    installationBoxLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Ignored));
    formLayout->addRow(tr("Installation drive:"), installationBoxLayout);

    updateSerialDevices();
    connect(SymbianUtils::SymbianDeviceManager::instance(), SIGNAL(updated()),
            this, SLOT(updateSerialDevices()));

    formLayout->addRow(createCommunicationChannel());

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
    updateTargetInformation();
    connect(m_deployConfiguration, SIGNAL(targetInformationChanged()),
            this, SLOT(updateTargetInformation()));
}

QWidget *S60DeployConfigurationWidget::createCommunicationChannel()
{
    m_serialPortsCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    connect(m_serialPortsCombo, SIGNAL(activated(int)), this, SLOT(setSerialPort(int)));
    connect(m_serialRadioButton, SIGNAL(clicked()), this, SLOT(updateCommunicationChannel()));
    connect(m_wlanRadioButton, SIGNAL(clicked()), this, SLOT(updateCommunicationChannel()));
    connect(m_ipAddress, SIGNAL(validAddressChanged(QString)), this, SLOT(updateWlanAddress(QString)));
    connect(m_ipAddress, SIGNAL(invalidAddressChanged()), this, SLOT(cleanWlanAddress()));

    QHBoxLayout *serialPortHBoxLayout = new QHBoxLayout;
    serialPortHBoxLayout->addWidget(new QLabel(tr("Serial port:")));
    serialPortHBoxLayout->addWidget(m_serialPortsCombo);
    serialPortHBoxLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Ignored));

#ifndef Q_OS_WIN
    // Update device list: on Linux only.
    QToolButton *updateSerialDevicesButton(new QToolButton);
    updateSerialDevicesButton->setIcon(qApp->style()->standardIcon(QStyle::SP_BrowserReload));
    connect(updateSerialDevicesButton, SIGNAL(clicked()),
            SymbianUtils::SymbianDeviceManager::instance(), SLOT(update()));
    serialPortHBoxLayout->addWidget(updateSerialDevicesButton);
#endif

    QGroupBox *communicationChannelGroupBox = new QGroupBox(tr("Communication channel"));
    QFormLayout *communicationChannelFormLayout = new QFormLayout();
    communicationChannelFormLayout->setWidget(0, QFormLayout::LabelRole, m_serialRadioButton);
    communicationChannelFormLayout->setWidget(1, QFormLayout::LabelRole, m_wlanRadioButton);

    m_ipAddress->setMinimumWidth(30);
    m_ipAddress->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Ignored);

    if(!m_deployConfiguration->deviceAddress().isEmpty())
        m_ipAddress->setText(QString("%1:%2")
                         .arg(m_deployConfiguration->deviceAddress())
                         .arg(m_deployConfiguration->devicePort()));

    QHBoxLayout *wlanChannelLayout = new QHBoxLayout();
    wlanChannelLayout->addWidget(new QLabel(tr("Address:")));
    wlanChannelLayout->addWidget(m_ipAddress);
    wlanChannelLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Ignored));

    communicationChannelFormLayout->setLayout(0, QFormLayout::FieldRole, serialPortHBoxLayout);
    communicationChannelFormLayout->setLayout(1, QFormLayout::FieldRole, wlanChannelLayout);

    switch (m_deployConfiguration->communicationChannel()) {
    case S60DeployConfiguration::CommunicationTrkSerialConnection:
        m_serialRadioButton->setChecked(true);
        m_ipAddress->setDisabled(true);
        m_serialPortsCombo->setDisabled(false);
        break;
    case S60DeployConfiguration::CommunicationCodaTcpConnection:
        m_wlanRadioButton->setChecked(true);
        m_ipAddress->setDisabled(false);
        m_serialPortsCombo->setDisabled(true);
        break;
    default:
        break;
    }

    communicationChannelGroupBox->setLayout(communicationChannelFormLayout);
    return communicationChannelGroupBox;
}

void S60DeployConfigurationWidget::updateInstallationDrives()
{
    m_installationDriveCombo->clear();
    for (int i = STARTING_DRIVE_LETTER; i <= LAST_DRIVE_LETTER; ++i) {
        m_installationDriveCombo->addItem(QString("%1:").arg(static_cast<char>(i)), qVariantFromValue(i));
    }
    int index = QChar::toUpper(static_cast<ushort>(m_deployConfiguration->installationDrive())) - STARTING_DRIVE_LETTER;

    Q_ASSERT(index >= 0 && index <= LAST_DRIVE_LETTER-STARTING_DRIVE_LETTER);

    m_installationDriveCombo->setCurrentIndex(index);
}

void S60DeployConfigurationWidget::silentInstallChanged(int state)
{
    m_deployConfiguration->setSilentInstall(state == Qt::Checked);
}

void S60DeployConfigurationWidget::updateSerialDevices()
{
    m_serialPortsCombo->clear();
    clearDeviceInfo();
    const QString previouPortName = m_deployConfiguration->serialPortName();
    const QList<SymbianUtils::SymbianDevice> devices = SymbianUtils::SymbianDeviceManager::instance()->devices();
    int newIndex = -1;
    for (int i = 0; i < devices.size(); ++i) {
        const SymbianUtils::SymbianDevice &device = devices.at(i);
        m_serialPortsCombo->addItem(device.friendlyName(), qVariantFromValue(device));
        if (device.portName() == previouPortName)
            newIndex = i;
    }
    // Set new index: prefer to keep old or set to 0, if available.
    if (newIndex == -1 && !devices.empty())
        newIndex = 0;
    m_serialPortsCombo->setCurrentIndex(newIndex);
    if (newIndex == -1) {
        m_deviceInfoButton->setEnabled(false);
        m_deployConfiguration->setSerialPortName(QString());
    } else {
        m_deviceInfoButton->setEnabled(true);
        const QString newPortName = device(newIndex).portName();
        m_deployConfiguration->setSerialPortName(newPortName);
    }
    if (m_deployConfiguration->communicationChannel() != S60DeployConfiguration::CommunicationTrkSerialConnection)
        m_deviceInfoButton->setEnabled(false);
}

SymbianUtils::SymbianDevice S60DeployConfigurationWidget::device(int i) const
{
    const QVariant data = m_serialPortsCombo->itemData(i);
    if (data.isValid() && qVariantCanConvert<SymbianUtils::SymbianDevice>(data))
        return qVariantValue<SymbianUtils::SymbianDevice>(data);
    return SymbianUtils::SymbianDevice();
}

SymbianUtils::SymbianDevice S60DeployConfigurationWidget::currentDevice() const
{
    return device(m_serialPortsCombo->currentIndex());
}

void S60DeployConfigurationWidget::updateTargetInformation()
{
    QString package;
    for (int i = 0; i < m_deployConfiguration->signedPackages().count(); ++i)
        package += m_deployConfiguration->signedPackages()[i] + QLatin1String("\n");
    if (!package.isEmpty())
        package.remove(package.length()-1, 1);
    m_sisFileLabel->setText(QDir::toNativeSeparators(package));
}

void S60DeployConfigurationWidget::setInstallationDrive(int index)
{
    m_deployConfiguration->setInstallationDrive(static_cast<char>(STARTING_DRIVE_LETTER + index));
}

void S60DeployConfigurationWidget::setSerialPort(int index)
{
    const SymbianUtils::SymbianDevice d = device(index);
    m_deployConfiguration->setSerialPortName(d.portName());
    m_deviceInfoButton->setEnabled(index >= 0);
    clearDeviceInfo();
}

void S60DeployConfigurationWidget::updateCommunicationChannel()
{
    if (m_serialRadioButton->isChecked()) {
        m_ipAddress->setDisabled(true);
        m_serialPortsCombo->setDisabled(false);
        m_deployConfiguration->setCommunicationChannel(S60DeployConfiguration::CommunicationTrkSerialConnection);
        updateSerialDevices();
    } else if(m_wlanRadioButton->isChecked()) {
        QMessageBox::information(this, tr("CODA required"),
                                 tr("You need to have CODA v4.0.14 (or newer) installed on your device "
                                    "in order to use the WLAN functionality.")); //TODO: Remove this when CODA is stable and official
        m_ipAddress->setDisabled(false);
        m_serialPortsCombo->setDisabled(true);
        m_deviceInfoButton->setEnabled(false);
        m_deployConfiguration->setCommunicationChannel(S60DeployConfiguration::CommunicationCodaTcpConnection);
    }
}

void S60DeployConfigurationWidget::updateWlanAddress(const QString &address)
{
    QStringList addressList = address.split(QLatin1String(":"), QString::SkipEmptyParts);
    if (addressList.count() > 0) {
        m_deployConfiguration->setDeviceAddress(addressList.at(0));
        if (addressList.count() > 1)
            m_deployConfiguration->setDevicePort(addressList.at(1));
        else
            m_deployConfiguration->setDevicePort(QString());
    }
}

void S60DeployConfigurationWidget::cleanWlanAddress()
{
    if (!m_deployConfiguration->deviceAddress().isEmpty())
        m_deployConfiguration->setDeviceAddress(QString());

    if (!m_deployConfiguration->devicePort().isEmpty())
        m_deployConfiguration->setDevicePort(QString());
}

void S60DeployConfigurationWidget::clearDeviceInfo()
{
    // Restore text & color
    m_deviceInfoLabel->clear();
    m_deviceInfoLabel->setStyleSheet(QString());
}

void S60DeployConfigurationWidget::setDeviceInfoLabel(const QString &message, bool isError)
{
    m_deviceInfoLabel->setStyleSheet(isError ?
                                         QString(QLatin1String("background-color: red;")) :
                                         QString());
    m_deviceInfoLabel->setText(message);
    m_deviceInfoLabel->adjustSize();
}

void S60DeployConfigurationWidget::slotLauncherStateChanged(int s)
{
    switch (s) {
    case trk::Launcher::WaitingForTrk: {
        // Entered trk wait state..open message box
        QMessageBox *mb = TrkRunControl::createTrkWaitingMessageBox(m_infoLauncher->trkServerName(), this);
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

void S60DeployConfigurationWidget::slotWaitingForTrkClosed()
{
    if (m_infoLauncher && m_infoLauncher->state() == trk::Launcher::WaitingForTrk) {
        m_infoLauncher->deleteLater();
        clearDeviceInfo();
        m_deviceInfoButton->setEnabled(true);
    }
}

void S60DeployConfigurationWidget::updateDeviceInfo()
{
    //TODO: No CODA device info! Implement it when it is available
    if (m_deployConfiguration->communicationChannel() == S60DeployConfiguration::CommunicationTrkSerialConnection) {
        QTC_ASSERT(!m_infoLauncher, return)
                setDeviceInfoLabel(tr("Connecting..."));
        // Do a launcher run with the ping protocol. Prompt to connect and
        // go asynchronous afterwards to pop up launch trk box if a timeout occurs.
        QString message;
        const SymbianUtils::SymbianDevice commDev = currentDevice();
        m_infoLauncher = trk::Launcher::acquireFromDeviceManager(commDev.portName(), this, &message);
        if (!m_infoLauncher) {
            setDeviceInfoLabel(message, true);
            return;
        }
        connect(m_infoLauncher, SIGNAL(stateChanged(int)), this, SLOT(slotLauncherStateChanged(int)));

        m_infoLauncher->setSerialFrame(commDev.type() == SymbianUtils::SerialPortCommunication);
        m_infoLauncher->setTrkServerName(commDev.portName());

        // Prompt user
        const trk::PromptStartCommunicationResult src =
                S60RunConfigBluetoothStarter::startCommunication(m_infoLauncher->trkDevice(),
                                                                 this, &message);
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
    } else
        setDeviceInfoLabel(tr("Currently there is no information about device for CODA connection type."), true);
}

} // namespace Internal
} // namespace Qt4ProjectManager
