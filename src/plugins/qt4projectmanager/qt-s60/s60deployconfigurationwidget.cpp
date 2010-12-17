/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include <utils/detailswidget.h>
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
      m_silentInstallCheckBox(new QCheckBox(tr("Silent installation")))
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
    // Serial devices control
    m_serialPortsCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    connect(m_serialPortsCombo, SIGNAL(activated(int)), this, SLOT(setSerialPort(int)));
    QHBoxLayout *serialPortHBoxLayout = new QHBoxLayout;
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

    formLayout->addRow(tr("Device on serial port:"), serialPortHBoxLayout);

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
        QMessageBox *mb = S60DeviceRunControl::createTrkWaitingMessageBox(m_infoLauncher->trkServerName(), this);
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
    return;
}

} // namespace Internal
} // namespace Qt4ProjectManager
