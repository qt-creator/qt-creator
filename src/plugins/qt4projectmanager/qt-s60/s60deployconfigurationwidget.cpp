/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "s60deployconfigurationwidget.h"
#include "s60deployconfiguration.h"
#include "s60devicerunconfiguration.h"

#include <symbianutils/symbiandevicemanager.h>
#include <codadevice.h>

#include <coreplugin/helpmanager.h>

#include "codaruncontrol.h"

#include <utils/detailswidget.h>
#include <utils/ipaddresslineedit.h>
#include <utils/qtcassert.h>
#include <utils/pathchooser.h>

#include <QtCore/QDir>
#include <QtCore/QTimer>
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

#include <QtNetwork/QTcpSocket>

Q_DECLARE_METATYPE(SymbianUtils::SymbianDevice)

namespace Qt4ProjectManager {
namespace Internal {

const char STARTING_DRIVE_LETTER = 'C';
const char LAST_DRIVE_LETTER = 'Z';

static const quint32 CODA_UID = 0x20021F96;
static const quint32 QTMOBILITY_UID = 0x2002AC89;
static const quint32 QTCOMPONENTS_UID = 0x200346DE;
static const quint32 QMLVIEWER_UID = 0x20021317;

QString formatDriveText(const S60DeployConfiguration::DeviceDrive &drive)
{
    char driveLetter = QChar::toUpper(static_cast<ushort>(drive.first));
    if (drive.second <= 0)
        return QString("%1:").arg(driveLetter);
    if (drive.second >= 1024)
        return QString("%1:%2 MB").arg(driveLetter).arg(drive.second);
    return QString("%1:%2 kB").arg(driveLetter).arg(drive.second);
}

void startTable(QString &text)
{
    const char startTableC[] = "<html></head><body><table>";
    if (text.contains(startTableC))
        return;
    text.append(startTableC);
}

void finishTable(QString &text)
{
    const char stopTableC[] = "</table></body></html>";
    text.replace(stopTableC, QLatin1String(""));
    text.append(stopTableC);
}

void addToTable(QTextStream &stream, const QString &key, const QString &value)
{
    const char tableRowStartC[] = "<tr><td><b>";
    const char tableRowSeparatorC[] = "</b></td><td>";
    const char tableRowEndC[] = "</td></tr>";
    stream << tableRowStartC << key << tableRowSeparatorC << value << tableRowEndC;
}

void addErrorToTable(QTextStream &stream, const QString &key, const QString &value)
{
    const char tableRowStartC[] = "<tr><td><b>";
    const char tableRowSeparatorC[] = "</b></td><td>";
    const char tableRowEndC[] = "</td></tr>";
    const char errorSpanStartC[] = "<span style=\"font-weight:600; color:red; \">";
    const char errorSpanEndC[] = "</span>";
    stream << tableRowStartC << errorSpanStartC << key << tableRowSeparatorC << value << errorSpanEndC << tableRowEndC;
}

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
      m_wlanRadioButton(new QRadioButton(tr("WLAN:"))),
      m_ipAddress(new Utils::IpAddressLineEdit),
      m_codaTimeout(new QTimer(this))
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

    bool usingTcp = m_deployConfiguration->communicationChannel() == S60DeployConfiguration::CommunicationCodaTcpConnection;
    m_serialRadioButton->setChecked(!usingTcp);
    m_wlanRadioButton->setChecked(usingTcp);

    formLayout->addRow(createCommunicationChannel());

    // Device Info with button. Widgets are enabled in above call to updateSerialDevices()
    QHBoxLayout *infoHBoxLayout = new QHBoxLayout;
    m_deviceInfoLabel->setWordWrap(true);
    m_deviceInfoLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    m_deviceInfoLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    infoHBoxLayout->addWidget(m_deviceInfoLabel);
    infoHBoxLayout->addWidget(m_deviceInfoButton);
    m_deviceInfoButton->setIcon(qApp->style()->standardIcon(QStyle::SP_MessageBoxInformation));
    m_deviceInfoButton->setToolTip(tr("Queries the device for information"));
    connect(m_deviceInfoButton, SIGNAL(clicked()), this, SLOT(updateDeviceInfo()));
    formLayout->addRow(m_deviceInfoDescriptionLabel, infoHBoxLayout);
    updateTargetInformation();
    connect(m_deployConfiguration, SIGNAL(targetInformationChanged()),
            this, SLOT(updateTargetInformation()));
    connect(m_deployConfiguration, SIGNAL(availableDeviceDrivesChanged()),
            this, SLOT(updateInstallationDrives()));
    connect(this, SIGNAL(infoCollected()),
            this, SLOT(collectingInfoFinished()));

    m_codaTimeout->setSingleShot(true);
    connect(m_codaTimeout, SIGNAL(timeout()), this, SLOT(codaTimeout()));
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

#if !defined(Q_OS_WIN) && !defined(Q_OS_MACX)
    // Update device list: only needed on linux.
    QToolButton *updateSerialDevicesButton(new QToolButton);
    updateSerialDevicesButton->setIcon(qApp->style()->standardIcon(QStyle::SP_BrowserReload));
    connect(updateSerialDevicesButton, SIGNAL(clicked()),
            SymbianUtils::SymbianDeviceManager::instance(), SLOT(update()));
    serialPortHBoxLayout->addWidget(updateSerialDevicesButton);
#endif

    QGroupBox *communicationChannelGroupBox = new QGroupBox(tr("Communication Channel"));
    QGridLayout *communicationChannelGridLayout = new QGridLayout;
    communicationChannelGridLayout->addWidget(m_serialRadioButton, 0, 0);
    communicationChannelGridLayout->addWidget(m_wlanRadioButton, 1, 0);

    m_ipAddress->setMinimumWidth(30);
    m_ipAddress->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Ignored);

    if( !m_deployConfiguration->deviceAddress().isEmpty())
        m_ipAddress->setText(QString("%1:%2")
                             .arg(m_deployConfiguration->deviceAddress())
                             .arg(m_deployConfiguration->devicePort()));

    QHBoxLayout *wlanChannelLayout = new QHBoxLayout();
    wlanChannelLayout->addWidget(new QLabel(tr("Address:")));
    wlanChannelLayout->addWidget(m_ipAddress);
    wlanChannelLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Ignored));

    communicationChannelGridLayout->addLayout(serialPortHBoxLayout, 0, 1);
    communicationChannelGridLayout->addLayout(wlanChannelLayout, 1, 1);

    communicationChannelGroupBox->setLayout(communicationChannelGridLayout);

    updateCommunicationChannelUi();

    return communicationChannelGroupBox;
}

void S60DeployConfigurationWidget::updateInstallationDrives()
{
    m_installationDriveCombo->clear();
    const QList<S60DeployConfiguration::DeviceDrive> &availableDrives(m_deployConfiguration->availableDeviceDrives());
    int index = 0;
    char currentDrive = QChar::toUpper(static_cast<ushort>(m_deployConfiguration->installationDrive()));
    if (availableDrives.isEmpty()) {
        for (int i = STARTING_DRIVE_LETTER; i <= LAST_DRIVE_LETTER; ++i) {
            m_installationDriveCombo->addItem(QString("%1:").arg(static_cast<char>(i)), QChar(i));
        }
        index = currentDrive - STARTING_DRIVE_LETTER;
    } else {
        for (int i = 0; i < availableDrives.count(); ++i) {
            const S60DeployConfiguration::DeviceDrive& drive(availableDrives.at(i));
            char driveLetter = QChar::toUpper(static_cast<ushort>(drive.first));
            m_installationDriveCombo->addItem(formatDriveText(drive),
                                              QChar(driveLetter));
            if (currentDrive == driveLetter)
                index = i;
        }
    }
    QTC_ASSERT(index >= 0 && index <= m_installationDriveCombo->count(), return);

    m_installationDriveCombo->setCurrentIndex(index);
    setInstallationDrive(index);
}

void S60DeployConfigurationWidget::silentInstallChanged(int state)
{
    m_deployConfiguration->setSilentInstall(state == Qt::Checked);
}

void S60DeployConfigurationWidget::updateSerialDevices()
{
    m_serialPortsCombo->clear();
    const QString previouPortName = m_deployConfiguration->serialPortName();
    const QList<SymbianUtils::SymbianDevice> devices = SymbianUtils::SymbianDeviceManager::instance()->devices();
    int newIndex = -1;
    for (int i = 0; i < devices.size(); ++i) {
        const SymbianUtils::SymbianDevice &device = devices.at(i);
        m_serialPortsCombo->addItem(device.friendlyName(), qVariantFromValue(device));
        if (device.portName() == previouPortName)
            newIndex = i;
    }

    if(m_deployConfiguration->communicationChannel()
            == S60DeployConfiguration::CommunicationCodaTcpConnection) {
        m_deviceInfoButton->setEnabled(true);
        return;
    }

    clearDeviceInfo();
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
    QTC_ASSERT(index >= 0, return);
    QTC_ASSERT(index < m_installationDriveCombo->count(), return);

    QChar driveLetter(m_installationDriveCombo->itemData(index).toChar());
    m_deployConfiguration->setInstallationDrive(driveLetter.toAscii());
}

void S60DeployConfigurationWidget::setSerialPort(int index)
{
    const SymbianUtils::SymbianDevice d = device(index);
    m_deployConfiguration->setSerialPortName(d.portName());
    m_deviceInfoButton->setEnabled(index >= 0);
    clearDeviceInfo();
}

void S60DeployConfigurationWidget::updateCommunicationChannelUi()
{
    S60DeployConfiguration::CommunicationChannel channel = m_deployConfiguration->communicationChannel();
    if (channel == S60DeployConfiguration::CommunicationCodaTcpConnection) {
        m_ipAddress->setDisabled(false);
        m_serialPortsCombo->setDisabled(true);
        m_deviceInfoButton->setEnabled(true);
    } else {
        m_ipAddress->setDisabled(true);
        m_serialPortsCombo->setDisabled(false);
        updateSerialDevices();
    }
}

void S60DeployConfigurationWidget::updateCommunicationChannel()
{
    if (!m_wlanRadioButton->isChecked() && !m_serialRadioButton->isChecked())
        m_serialRadioButton->setChecked(true);

    if (m_wlanRadioButton->isChecked()) {
        m_ipAddress->setDisabled(false);
        m_serialPortsCombo->setDisabled(true);
        m_deployConfiguration->setCommunicationChannel(S60DeployConfiguration::CommunicationCodaTcpConnection);
        m_deviceInfoButton->setEnabled(true);
    } else {
        m_ipAddress->setDisabled(true);
        m_serialPortsCombo->setDisabled(false);
        m_deployConfiguration->setCommunicationChannel(S60DeployConfiguration::CommunicationCodaSerialConnection);
        updateSerialDevices();
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

void S60DeployConfigurationWidget::updateDeviceInfo()
{
    setDeviceInfoLabel(tr("Connecting"));
    if (m_deployConfiguration->communicationChannel() == S60DeployConfiguration::CommunicationCodaSerialConnection) {
        const SymbianUtils::SymbianDevice commDev = currentDevice();
        m_codaInfoDevice = SymbianUtils::SymbianDeviceManager::instance()->getCodaDevice(commDev.portName());
        if (m_codaInfoDevice.isNull()) {
            setDeviceInfoLabel(tr("Unable to create CODA connection. Please try again."), true);
            return;
        }
        if (!m_codaInfoDevice->device()->isOpen()) {
            setDeviceInfoLabel(m_codaInfoDevice->device()->errorString(), true);
            return;
        }
        //TODO error handling - for now just throw the command at coda
        m_codaInfoDevice->sendSymbianOsDataGetQtVersionCommand(Coda::CodaCallback(this, &S60DeployConfigurationWidget::getQtVersionCommandResult));
        m_deviceInfoButton->setEnabled(false);
        m_codaTimeout->start(1000);
    } else if(m_deployConfiguration->communicationChannel() == S60DeployConfiguration::CommunicationCodaTcpConnection) {
        // collectingInfoFinished, which deletes m_codaDevice, can get called from within a coda callback, so need to use deleteLater
        m_codaInfoDevice =  QSharedPointer<Coda::CodaDevice>(new Coda::CodaDevice, &QObject::deleteLater);
        connect(m_codaInfoDevice.data(), SIGNAL(codaEvent(Coda::CodaEvent)), this, SLOT(codaEvent(Coda::CodaEvent)));

        const QSharedPointer<QTcpSocket> codaSocket(new QTcpSocket);
        m_codaInfoDevice->setDevice(codaSocket);
        codaSocket->connectToHost(m_deployConfiguration->deviceAddress(),
                                  m_deployConfiguration->devicePort().toInt());
        m_deviceInfoButton->setEnabled(false);
        m_codaTimeout->start(1500);
    } else
        setDeviceInfoLabel(tr("Currently there is no information about the device for this connection type."), true);
}

void S60DeployConfigurationWidget::codaEvent(const Coda::CodaEvent &event)
{
    switch (event.type()) {
    case Coda::CodaEvent::LocatorHello: // Commands accepted now
        codaIncreaseProgress();
        if (m_codaInfoDevice)
            m_codaInfoDevice->sendSymbianOsDataGetQtVersionCommand(Coda::CodaCallback(this, &S60DeployConfigurationWidget::getQtVersionCommandResult));
        break;
    default:
        break;
    }
}

void S60DeployConfigurationWidget::getQtVersionCommandResult(const Coda::CodaCommandResult &result)
{
    m_deviceInfo.clear();
    if (result.type == Coda::CodaCommandResult::FailReply) {
        setDeviceInfoLabel(tr("No device information available"), true);
        SymbianUtils::SymbianDeviceManager::instance()->releaseCodaDevice(m_codaInfoDevice);
        m_deviceInfoButton->setEnabled(true);
        m_codaTimeout->stop();
        return;
    } else if (result.type == Coda::CodaCommandResult::CommandErrorReply){
        startTable(m_deviceInfo);
        QTextStream str(&m_deviceInfo);
        addErrorToTable(str, tr("Qt version: "), tr("Not installed on device"));
        finishTable(m_deviceInfo);
    } else {
        if (result.values.count()) {
            QHash<QString, QVariant> obj = result.values[0].toVariant().toHash();
            QString ver = obj.value("qVersion").toString();

            startTable(m_deviceInfo);
            QTextStream str(&m_deviceInfo);
            addToTable(str, tr("Qt version:"), ver);
            QString systemVersion;

            int symVer = obj.value("symbianVersion").toInt();
            // Ugh why won't QSysInfo define these on non-symbian builds...
            switch (symVer) {
            case 10:
                systemVersion.append("Symbian OS v9.2");
                break;
            case 20:
                systemVersion.append("Symbian OS v9.3");
                break;
            case 30:
                systemVersion.append("Symbian OS v9.4 / Symbian^1");
                break;
            case 40:
                systemVersion.append("Symbian^2");
                break;
            case 50:
                systemVersion.append("Symbian^3");
                break;
            case 60:
                systemVersion.append("Symbian^4");
                break;
            case 70:
                systemVersion.append("Symbian^3"); // TODO: might change
                break;
            default:
                systemVersion.append(tr("Unrecognised Symbian version 0x%1").arg(symVer, 0, 16));
                break;
            }
            systemVersion.append(", ");
            int s60Ver = obj.value("s60Version").toInt();
            switch (s60Ver) {
            case 10:
                systemVersion.append("S60 3rd Edition Feature Pack 1");
                break;
            case 20:
                systemVersion.append("S60 3rd Edition Feature Pack 2");
                break;
            case 30:
                systemVersion.append("S60 5th Edition");
                break;
            case 40:
                systemVersion.append("S60 5th Edition Feature Pack 1");
                break;
            case 50:
                systemVersion.append("S60 5th Edition Feature Pack 2");
                break;
            case 70:
                systemVersion.append("S60 5th Edition Feature Pack 3"); // TODO: might change
                break;
            default:
                systemVersion.append(tr("Unrecognised S60 version 0x%1").arg(symVer, 0, 16));
                break;
            }
            addToTable(str, tr("OS version:"), systemVersion);
            finishTable(m_deviceInfo);
        }
    }
    codaIncreaseProgress();
    if (m_codaInfoDevice)
        m_codaInfoDevice->sendSymbianOsDataGetRomInfoCommand(Coda::CodaCallback(this, &S60DeployConfigurationWidget::getRomInfoResult));
}

void S60DeployConfigurationWidget::getRomInfoResult(const Coda::CodaCommandResult &result)
{
    codaIncreaseProgress();
    if (result.type == Coda::CodaCommandResult::SuccessReply && result.values.count()) {
        startTable(m_deviceInfo);
        QTextStream str(&m_deviceInfo);

        QVariantHash obj = result.values[0].toVariant().toHash();
        QString romVersion = obj.value("romVersion", tr("unknown")).toString();
        romVersion.replace('\n', " "); // The ROM string is split across multiple lines, for some reason.
        addToTable(str, tr("ROM version:"), romVersion);

        QString pr = obj.value("prInfo").toString();
        if (pr.length())
            addToTable(str, tr("Release:"), pr);
        finishTable(m_deviceInfo);
    }

    QList<quint32> packagesOfInterest;
    packagesOfInterest.append(CODA_UID);
    packagesOfInterest.append(QTMOBILITY_UID);
    packagesOfInterest.append(QTCOMPONENTS_UID);
    packagesOfInterest.append(QMLVIEWER_UID);
    if (m_codaInfoDevice)
        m_codaInfoDevice->sendSymbianInstallGetPackageInfoCommand(Coda::CodaCallback(this, &S60DeployConfigurationWidget::getInstalledPackagesResult), packagesOfInterest);
}

void S60DeployConfigurationWidget::getInstalledPackagesResult(const Coda::CodaCommandResult &result)
{
    codaIncreaseProgress();
    if (result.type == Coda::CodaCommandResult::SuccessReply && result.values.count()) {
        startTable(m_deviceInfo);
        QTextStream str(&m_deviceInfo);

        QVariantList resultsList = result.values[0].toVariant().toList();
        foreach (const QVariant& var, resultsList) {
            QVariantHash obj = var.toHash();
            bool ok = false;
            uint uid = obj.value("uid").toString().toUInt(&ok, 16);
            if (ok) {
                bool error = !obj.value("error").isNull();
                QString versionString;
                if (!error) {
                    QVariantList version = obj.value("version").toList();
                    versionString = QString("%1.%2.%3").arg(version[0].toInt())
                            .arg(version[1].toInt())
                            .arg(version[2].toInt());
                }
                switch (uid) {
                case CODA_UID: {
                    if (error) {
                        // How can coda not be installed? Presumably some UID wrongness...
                        addErrorToTable(str, tr("CODA version: "), tr("Error reading CODA version"));
                    } else
                        addToTable(str, tr("CODA version: "), versionString);
                }
                break;
                case QTMOBILITY_UID: {
                    if (error)
                        addErrorToTable(str, tr("Qt Mobility version: "), tr("Error reading Qt Mobility version"));
                    else
                        addToTable(str, tr("Qt Mobility version: "), versionString);
                }
                break;
                case QTCOMPONENTS_UID: {
                    addToTable(str, tr("Qt Quick components version: "), error ? tr("Not installed") : versionString);
                }
                break;
                case QMLVIEWER_UID: {
                    addToTable(str, tr("QML Viewer version: "), error ? tr("Not installed") : versionString);
                }
                break;
                default: break;
                }
            }
        }
        finishTable(m_deviceInfo);
    }

    QStringList keys;
    keys << QLatin1String("EDisplayXPixels");
    keys << QLatin1String("EDisplayYPixels");

    if (m_codaInfoDevice)
        m_codaInfoDevice->sendSymbianOsDataGetHalInfoCommand(Coda::CodaCallback(this, &S60DeployConfigurationWidget::getHalResult), keys);
}

void S60DeployConfigurationWidget::getHalResult(const Coda::CodaCommandResult &result)
{
    codaIncreaseProgress();
    if (result.type == Coda::CodaCommandResult::SuccessReply && result.values.count()) {
        QVariantList resultsList = result.values[0].toVariant().toList();
        int x = 0;
        int y = 0;
        foreach (const QVariant& var, resultsList) {
            QVariantHash obj = var.toHash();
            if (obj.value("name").toString() == "EDisplayXPixels")
                x = obj.value("value").toInt();
            else if (obj.value("name").toString() == "EDisplayYPixels")
                y = obj.value("value").toInt();
        }
        if (x && y) {
            startTable(m_deviceInfo);
            QTextStream str(&m_deviceInfo);
            addToTable(str, tr("Screen size:"), QString("%1x%2").arg(x).arg(y));
            finishTable(m_deviceInfo);
        }
    }

    // Done with collecting info
    emit infoCollected();
}

void S60DeployConfigurationWidget::codaIncreaseProgress()
{
    m_codaTimeout->start();
    setDeviceInfoLabel(m_deviceInfoLabel->text() + '.');
}

void S60DeployConfigurationWidget::collectingInfoFinished()
{
    m_codaTimeout->stop();
    emit codaConnected();
    m_deviceInfoButton->setEnabled(true);
    setDeviceInfoLabel(m_deviceInfo);
    SymbianUtils::SymbianDeviceManager::instance()->releaseCodaDevice(m_codaInfoDevice);
}

void S60DeployConfigurationWidget::codaTimeout()
{
    QMessageBox *mb = CodaRunControl::createCodaWaitingMessageBox(this);
    connect(this, SIGNAL(codaConnected()), mb, SLOT(close()));
    connect(mb, SIGNAL(finished(int)), this, SLOT(codaCanceled()));
    mb->open();
}

void S60DeployConfigurationWidget::codaCanceled()
{
    clearDeviceInfo();
    m_deviceInfoButton->setEnabled(true);
    SymbianUtils::SymbianDeviceManager::instance()->releaseCodaDevice(m_codaInfoDevice);
}

} // namespace Internal
} // namespace Qt4ProjectManager
