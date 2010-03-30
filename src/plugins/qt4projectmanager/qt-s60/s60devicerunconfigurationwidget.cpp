/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
#include "symbiandevicemanager.h"

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

Q_DECLARE_METATYPE(SymbianUtils::SymbianDevice)

#ifdef Q_OS_WIN
enum { wantUpdateSerialDevicesButton = 0 };
#else
enum { wantUpdateSerialDevicesButton = 1 };
#endif

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
    m_argumentsLineEdit(new QLineEdit(m_runConfiguration->commandLineArguments().join(QString(QLatin1Char(' '))))),
    m_sisFileLabel(new QLabel),
    m_deviceInfoButton(new QToolButton),
    m_deviceInfoDescriptionLabel(new QLabel(tr("Device:"))),
    m_deviceInfoLabel(new QLabel),
    m_infoTimeOutTimer(0)
{
    m_detailsWidget->setState(Utils::DetailsWidget::NoSummary);
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
    formLayout->addRow(tr("Arguments:"), m_argumentsLineEdit);
    formLayout->addRow(tr("Install File:"), m_sisFileLabel);

    updateSerialDevices();
    connect(SymbianUtils::SymbianDeviceManager::instance(), SIGNAL(updated()),
            this, SLOT(updateSerialDevices()));
    // Serial devices control
    connect(m_serialPortsCombo, SIGNAL(activated(int)), this, SLOT(setSerialPort(int)));
    QHBoxLayout *serialPortHBoxLayout = new QHBoxLayout;
    serialPortHBoxLayout->addWidget(m_serialPortsCombo);
    serialPortHBoxLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Ignored));
    // Update device list: on Linux only.
    if (wantUpdateSerialDevicesButton) {
        QToolButton *updateSerialDevicesButton(new QToolButton);
        updateSerialDevicesButton->setIcon(qApp->style()->standardIcon(QStyle::SP_BrowserReload));
        connect(updateSerialDevicesButton, SIGNAL(clicked()),
                SymbianUtils::SymbianDeviceManager::instance(), SLOT(update()));
        serialPortHBoxLayout->addWidget(updateSerialDevicesButton);
    }

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

    connect(m_nameLineEdit, SIGNAL(textEdited(QString)),
            this, SLOT(displayNameEdited(QString)));
    connect(m_argumentsLineEdit, SIGNAL(textEdited(QString)),
            this, SLOT(argumentsEdited(QString)));
    connect(m_runConfiguration, SIGNAL(targetInformationChanged()),
            this, SLOT(updateTargetInformation()));
}

void S60DeviceRunConfigurationWidget::updateSerialDevices()
{
    m_serialPortsCombo->clear();
    clearDeviceInfo();
    const QString previousRunConfigurationPortName = m_runConfiguration->serialPortName();
    const QList<SymbianUtils::SymbianDevice> devices = SymbianUtils::SymbianDeviceManager::instance()->devices();
    int newIndex = -1;
    for (int i = 0; i < devices.size(); ++i) {
        const SymbianUtils::SymbianDevice &device = devices.at(i);
        m_serialPortsCombo->addItem(device.friendlyName(), qVariantFromValue(device));
        if (device.portName() == previousRunConfigurationPortName)
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
        const QString newPortName = device(newIndex).portName();
        if (newPortName != previousRunConfigurationPortName)
            m_runConfiguration->setSerialPortName(newPortName);
    }
}

SymbianUtils::SymbianDevice S60DeviceRunConfigurationWidget::device(int i) const
{
    if (i >= 0) {
        const QVariant data = m_serialPortsCombo->itemData(i);
        if (qVariantCanConvert<SymbianUtils::SymbianDevice>(data))
            return qVariantValue<SymbianUtils::SymbianDevice>(data);
    }
    return SymbianUtils::SymbianDevice();
}

SymbianUtils::SymbianDevice S60DeviceRunConfigurationWidget::currentDevice() const
{
    return device(m_serialPortsCombo->currentIndex());
}

void S60DeviceRunConfigurationWidget::displayNameEdited(const QString &text)
{
    m_runConfiguration->setDisplayName(text);
}

void S60DeviceRunConfigurationWidget::argumentsEdited(const QString &text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        m_runConfiguration->setCommandLineArguments(QStringList());
    } else {
        m_runConfiguration->setCommandLineArguments(trimmed.split(QLatin1Char(' '),
                                                                  QString::SkipEmptyParts));
    }
}

void S60DeviceRunConfigurationWidget::updateTargetInformation()
{
    m_sisFileLabel->setText(m_runConfiguration->signedPackage());
}

void S60DeviceRunConfigurationWidget::setSerialPort(int index)
{
    const SymbianUtils::SymbianDevice d = device(index);
    m_runConfiguration->setSerialPortName(d.portName());
    m_deviceInfoButton->setEnabled(index >= 0);
    clearDeviceInfo();
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
