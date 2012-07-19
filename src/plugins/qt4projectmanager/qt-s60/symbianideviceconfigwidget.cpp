/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "symbianideviceconfigwidget.h"

#include "symbianidevice.h"

#include <symbianutils/symbiandevicemanager.h>
#include <codadevice.h>

#include "codaruncontrol.h"

#include <utils/detailswidget.h>
#include <utils/ipaddresslineedit.h>
#include <utils/qtcassert.h>
#include <utils/pathchooser.h>

#include <QTimer>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QToolButton>
#include <QStyle>
#include <QApplication>
#include <QSpacerItem>
#include <QMessageBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QRadioButton>
#include <QValidator>

#include <QTcpSocket>

Q_DECLARE_METATYPE(SymbianUtils::SymbianDevice)

static const quint32 CODA_UID = 0x20021F96;

static const quint32 QTMOBILITY_UID = 0x2002AC89;
static const quint32 QTCOMPONENTS_UID = 0x200346DE;
static const quint32 QMLVIEWER_UID = 0x20021317;

static void startTable(QString &text)
{
    const char startTableC[] = "<html></head><body><table>";
    if (!text.contains(QLatin1String(startTableC)))
        text.append(QLatin1String(startTableC));
}

static void finishTable(QString &text)
{
    const QString stopTable = QLatin1String("</table></body></html>");
    text.remove(stopTable);
    text.append(stopTable);
}

static void addToTable(QTextStream &stream, const QString &key, const QString &value)
{
    const char tableRowStartC[] = "<tr><td><b>";
    const char tableRowSeparatorC[] = "</b></td><td>";
    const char tableRowEndC[] = "</td></tr>";
    stream << tableRowStartC << key << tableRowSeparatorC << value << tableRowEndC;
}

static void addErrorToTable(QTextStream &stream, const QString &key, const QString &value)
{
    const char tableRowStartC[] = "<tr><td><b>";
    const char tableRowSeparatorC[] = "</b></td><td>";
    const char tableRowEndC[] = "</td></tr>";
    const char errorSpanStartC[] = "<span style=\"font-weight:600; color:red; \">";
    const char errorSpanEndC[] = "</span>";
    stream << tableRowStartC << errorSpanStartC << key << tableRowSeparatorC << value << errorSpanEndC << tableRowEndC;
}

namespace Qt4ProjectManager {
namespace Internal {

SymbianIDeviceConfigurationWidget::SymbianIDeviceConfigurationWidget(const ProjectExplorer::IDevice::Ptr &device,
                                                                     QWidget *parent)
    : ProjectExplorer::IDeviceWidget(device, parent),
      m_detailsWidget(new Utils::DetailsWidget),
      m_serialPortsCombo(new QComboBox),
      m_deviceInfoButton(new QToolButton),
      m_deviceInfoDescriptionLabel(new QLabel(tr("Device:"))),
      m_deviceInfoLabel(new QLabel),
      m_serialRadioButton(new QRadioButton(tr("Serial:"))),
      m_wlanRadioButton(new QRadioButton(tr("WLAN:"))),
      m_ipAddress(new Utils::IpAddressLineEdit),
      m_codaTimeout(new QTimer(this))
{
    m_detailsWidget->setState(Utils::DetailsWidget::NoSummary);

    QVBoxLayout *mainBoxLayout = new QVBoxLayout();
    mainBoxLayout->setMargin(0);
    setLayout(mainBoxLayout);
    mainBoxLayout->addWidget(m_detailsWidget);
    QWidget *detailsContainer = new QWidget;
    m_detailsWidget->setWidget(detailsContainer);

    QFormLayout *detailsLayout = new QFormLayout(detailsContainer);

    updateSerialDevices();
    connect(SymbianUtils::SymbianDeviceManager::instance(), SIGNAL(updated()),
            this, SLOT(updateSerialDevices()));

    bool usingTcp = symbianDevice()->communicationChannel() == SymbianIDevice::CommunicationCodaTcpConnection;
    m_serialRadioButton->setChecked(!usingTcp);
    m_wlanRadioButton->setChecked(usingTcp);

    detailsLayout->addRow(createCommunicationChannel());

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
    detailsLayout->addRow(m_deviceInfoDescriptionLabel, infoHBoxLayout);

    connect(this, SIGNAL(infoCollected()),
            this, SLOT(collectingInfoFinished()));

    m_codaTimeout->setSingleShot(true);
    connect(m_codaTimeout, SIGNAL(timeout()), this, SLOT(codaTimeout()));
}

SymbianIDeviceConfigurationWidget::~SymbianIDeviceConfigurationWidget()
{ }

QWidget *SymbianIDeviceConfigurationWidget::createCommunicationChannel()
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

    if (!symbianDevice()->address().isEmpty())
        m_ipAddress->setText(symbianDevice()->address() + QLatin1Char(':')
                             + symbianDevice()->port());

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

void SymbianIDeviceConfigurationWidget::updateSerialDevices()
{
    m_serialPortsCombo->clear();
    const QString previouPortName = symbianDevice()->serialPortName();
    const QList<SymbianUtils::SymbianDevice> devices = SymbianUtils::SymbianDeviceManager::instance()->devices();
    int newIndex = -1;
    for (int i = 0; i < devices.size(); ++i) {
        const SymbianUtils::SymbianDevice &device = devices.at(i);
        m_serialPortsCombo->addItem(device.friendlyName(), qVariantFromValue(device));
        if (device.portName() == previouPortName)
            newIndex = i;
    }

    if (symbianDevice()->communicationChannel()
            == SymbianIDevice::CommunicationCodaTcpConnection) {
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
        symbianDevice()->setSerialPortName(QString());
    } else {
        m_deviceInfoButton->setEnabled(true);
        const QString newPortName = rawDevice(newIndex).portName();
        symbianDevice()->setSerialPortName(newPortName);
    }
}

SymbianUtils::SymbianDevice SymbianIDeviceConfigurationWidget::rawDevice(int i) const
{
    const QVariant data = m_serialPortsCombo->itemData(i);
    if (data.isValid() && qVariantCanConvert<SymbianUtils::SymbianDevice>(data))
        return qVariantValue<SymbianUtils::SymbianDevice>(data);
    return SymbianUtils::SymbianDevice();
}

SymbianUtils::SymbianDevice SymbianIDeviceConfigurationWidget::currentRawDevice() const
{
    return rawDevice(m_serialPortsCombo->currentIndex());
}

void SymbianIDeviceConfigurationWidget::setSerialPort(int index)
{
    const SymbianUtils::SymbianDevice d = rawDevice(index);
    symbianDevice()->setSerialPortName(d.portName());
    m_deviceInfoButton->setEnabled(index >= 0);
    clearDeviceInfo();
}

void SymbianIDeviceConfigurationWidget::updateCommunicationChannelUi()
{
    SymbianIDevice::CommunicationChannel channel = symbianDevice()->communicationChannel();
    if (channel == SymbianIDevice::CommunicationCodaTcpConnection) {
        m_ipAddress->setDisabled(false);
        m_serialPortsCombo->setDisabled(true);
        m_deviceInfoButton->setEnabled(true);
    } else {
        m_ipAddress->setDisabled(true);
        m_serialPortsCombo->setDisabled(false);
        updateSerialDevices();
    }
}

void SymbianIDeviceConfigurationWidget::updateCommunicationChannel()
{
    if (!m_wlanRadioButton->isChecked() && !m_serialRadioButton->isChecked())
        m_serialRadioButton->setChecked(true);

    if (m_wlanRadioButton->isChecked()) {
        m_ipAddress->setDisabled(false);
        m_serialPortsCombo->setDisabled(true);
        symbianDevice()->setCommunicationChannel(SymbianIDevice::CommunicationCodaTcpConnection);
        m_deviceInfoButton->setEnabled(true);
    } else {
        m_ipAddress->setDisabled(true);
        m_serialPortsCombo->setDisabled(false);
        symbianDevice()->setCommunicationChannel(SymbianIDevice::CommunicationCodaSerialConnection);
        updateSerialDevices();
    }
}

void SymbianIDeviceConfigurationWidget::updateWlanAddress(const QString &address)
{
    QStringList addressList = address.split(QLatin1String(":"), QString::SkipEmptyParts);
    if (addressList.count() > 0) {
        symbianDevice()->setAddress(addressList.at(0));
        if (addressList.count() > 1)
            symbianDevice()->setPort(addressList.at(1));
        else
            symbianDevice()->setPort(QString());
    }
}

void SymbianIDeviceConfigurationWidget::cleanWlanAddress()
{
    if (!symbianDevice()->address().isEmpty())
        symbianDevice()->setAddress(QString());

    if (!symbianDevice()->port().isEmpty())
        symbianDevice()->setPort(QString());
}

void SymbianIDeviceConfigurationWidget::clearDeviceInfo()
{
    // Restore text & color
    m_deviceInfoLabel->clear();
    m_deviceInfoLabel->setStyleSheet(QString());
}

void SymbianIDeviceConfigurationWidget::setDeviceInfoLabel(const QString &message, bool isError)
{
    m_deviceInfoLabel->setStyleSheet(isError ?
                                         QString(QLatin1String("background-color: red;")) :
                                         QString());
    m_deviceInfoLabel->setText(message);
    m_deviceInfoLabel->adjustSize();
}

void SymbianIDeviceConfigurationWidget::updateDeviceInfo()
{
    setDeviceInfoLabel(tr("Connecting"));
    if (symbianDevice()->communicationChannel() == SymbianIDevice::CommunicationCodaSerialConnection) {
        const SymbianUtils::SymbianDevice commDev = currentRawDevice();
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
        m_codaInfoDevice->sendSymbianOsDataGetQtVersionCommand(Coda::CodaCallback(this, &SymbianIDeviceConfigurationWidget::getQtVersionCommandResult));
        m_deviceInfoButton->setEnabled(false);
        m_codaTimeout->start(1000);
    } else if (symbianDevice()->communicationChannel() == SymbianIDevice::CommunicationCodaTcpConnection) {
        // collectingInfoFinished, which deletes m_codaDevice, can get called from within a coda callback, so need to use deleteLater
        m_codaInfoDevice =  QSharedPointer<Coda::CodaDevice>(new Coda::CodaDevice, &QObject::deleteLater);
        connect(m_codaInfoDevice.data(), SIGNAL(codaEvent(Coda::CodaEvent)), this, SLOT(codaEvent(Coda::CodaEvent)));

        const QSharedPointer<QTcpSocket> codaSocket(new QTcpSocket);
        m_codaInfoDevice->setDevice(codaSocket);
        codaSocket->connectToHost(symbianDevice()->address(),
                                  symbianDevice()->port().toInt());
        m_deviceInfoButton->setEnabled(false);
        m_codaTimeout->start(1500);
    } else
        setDeviceInfoLabel(tr("Currently there is no information about the device for this connection type."), true);
}

void SymbianIDeviceConfigurationWidget::codaEvent(const Coda::CodaEvent &event)
{
    switch (event.type()) {
    case Coda::CodaEvent::LocatorHello: // Commands accepted now
        codaIncreaseProgress();
        if (m_codaInfoDevice)
            m_codaInfoDevice->sendSymbianOsDataGetQtVersionCommand(Coda::CodaCallback(this, &SymbianIDeviceConfigurationWidget::getQtVersionCommandResult));
        break;
    default:
        break;
    }
}

void SymbianIDeviceConfigurationWidget::getQtVersionCommandResult(const Coda::CodaCommandResult &result)
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
            QString ver = obj.value(QLatin1String("qVersion")).toString();

            startTable(m_deviceInfo);
            QTextStream str(&m_deviceInfo);
            addToTable(str, tr("Qt version:"), ver);
            QString systemVersion;

            const int symVer = obj.value(QLatin1String("symbianVersion")).toInt();
            // Ugh why won't QSysInfo define these on non-symbian builds...
            switch (symVer) {
            case 10:
                systemVersion.append(QLatin1String("Symbian OS v9.2"));
                break;
            case 20:
                systemVersion.append(QLatin1String("Symbian OS v9.3"));
                break;
            case 30:
                systemVersion.append(QLatin1String("Symbian OS v9.4 / Symbian^1"));
                break;
            case 40:
                systemVersion.append(QLatin1String("Symbian^2"));
                break;
            case 50:
                systemVersion.append(QLatin1String("Symbian^3"));
                break;
            case 60:
                systemVersion.append(QLatin1String("Symbian^4"));
                break;
            case 70:
                systemVersion.append(QLatin1String("Symbian^3")); // TODO: might change
                break;
            default:
                systemVersion.append(tr("Unrecognised Symbian version 0x%1").arg(symVer, 0, 16));
                break;
            }
            systemVersion.append(QLatin1String(", "));
            int s60Ver = obj.value(QLatin1String("s60Version")).toInt();
            switch (s60Ver) {
            case 10:
                systemVersion.append(QLatin1String("S60 3rd Edition Feature Pack 1"));
                break;
            case 20:
                systemVersion.append(QLatin1String("S60 3rd Edition Feature Pack 2"));
                break;
            case 30:
                systemVersion.append(QLatin1String("S60 5th Edition"));
                break;
            case 40:
                systemVersion.append(QLatin1String("S60 5th Edition Feature Pack 1"));
                break;
            case 50:
                systemVersion.append(QLatin1String("S60 5th Edition Feature Pack 2"));
                break;
            case 70:
                systemVersion.append(QLatin1String("S60 5th Edition Feature Pack 3")); // TODO: might change
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
        m_codaInfoDevice->sendSymbianOsDataGetRomInfoCommand(Coda::CodaCallback(this, &SymbianIDeviceConfigurationWidget::getRomInfoResult));
}

void SymbianIDeviceConfigurationWidget::getRomInfoResult(const Coda::CodaCommandResult &result)
{
    codaIncreaseProgress();
    if (result.type == Coda::CodaCommandResult::SuccessReply && result.values.count()) {
        startTable(m_deviceInfo);
        QTextStream str(&m_deviceInfo);

        QVariantHash obj = result.values[0].toVariant().toHash();
        QString romVersion = obj.value(QLatin1String("romVersion"), tr("unknown")).toString();
        romVersion.replace(QLatin1Char('\n'), QLatin1Char(' ')); // The ROM string is split across multiple lines, for some reason.
        addToTable(str, tr("ROM version:"), romVersion);

        QString pr = obj.value(QLatin1String("prInfo")).toString();
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
        m_codaInfoDevice->sendSymbianInstallGetPackageInfoCommand(Coda::CodaCallback(this, &SymbianIDeviceConfigurationWidget::getInstalledPackagesResult), packagesOfInterest);
}

void SymbianIDeviceConfigurationWidget::getInstalledPackagesResult(const Coda::CodaCommandResult &result)
{
    codaIncreaseProgress();
    if (result.type == Coda::CodaCommandResult::SuccessReply && result.values.count()) {
        startTable(m_deviceInfo);
        QTextStream str(&m_deviceInfo);

        QVariantList resultsList = result.values[0].toVariant().toList();
        const QString uidKey = QLatin1String("uid");
        const QString errorKey = QLatin1String("error");
        const QString versionKey = QLatin1String("version");
        foreach (const QVariant &var, resultsList) {
            QVariantHash obj = var.toHash();
            bool ok = false;
            uint uid = obj.value(uidKey).toString().toUInt(&ok, 16);
            if (ok) {
                const bool error = !obj.value(errorKey).isNull();
                QString versionString;
                if (!error) {
                    QVariantList version = obj.value(versionKey).toList();
                    versionString = QString::fromLatin1("%1.%2.%3").arg(version[0].toInt())
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
        m_codaInfoDevice->sendSymbianOsDataGetHalInfoCommand(Coda::CodaCallback(this, &SymbianIDeviceConfigurationWidget::getHalResult), keys);
}

void SymbianIDeviceConfigurationWidget::getHalResult(const Coda::CodaCommandResult &result)
{
    codaIncreaseProgress();
    if (result.type == Coda::CodaCommandResult::SuccessReply && result.values.count()) {
        QVariantList resultsList = result.values[0].toVariant().toList();
        int x = 0;
        int y = 0;
        const QString nameKey = QLatin1String("name");
        const QString valueKey = QLatin1String("value");
        foreach (const QVariant &var, resultsList) {
            QVariantHash obj = var.toHash();
            const QString name = obj.value(nameKey).toString();
            if (name == QLatin1String("EDisplayXPixels"))
                x = obj.value(valueKey).toInt();
            else if (name == QLatin1String("EDisplayYPixels"))
                y = obj.value(valueKey).toInt();
        }
        if (x && y) {
            startTable(m_deviceInfo);
            QTextStream str(&m_deviceInfo);
            addToTable(str, tr("Screen size:"), QString::number(x) + QLatin1Char('x') + QString::number(y));
            finishTable(m_deviceInfo);
        }
    }

    // Done with collecting info
    emit infoCollected();
}

void SymbianIDeviceConfigurationWidget::codaIncreaseProgress()
{
    m_codaTimeout->start();
    setDeviceInfoLabel(m_deviceInfoLabel->text() + QLatin1Char('.'));
}

SymbianIDevice *SymbianIDeviceConfigurationWidget::symbianDevice() const
{
    return dynamic_cast<SymbianIDevice *>(device().data());
}

void SymbianIDeviceConfigurationWidget::collectingInfoFinished()
{
    m_codaTimeout->stop();
    emit codaConnected();
    m_deviceInfoButton->setEnabled(true);
    setDeviceInfoLabel(m_deviceInfo);
    SymbianUtils::SymbianDeviceManager::instance()->releaseCodaDevice(m_codaInfoDevice);
}

void SymbianIDeviceConfigurationWidget::codaTimeout()
{
    QMessageBox *mb = CodaRunControl::createCodaWaitingMessageBox(this);
    connect(this, SIGNAL(codaConnected()), mb, SLOT(close()));
    connect(mb, SIGNAL(finished(int)), this, SLOT(codaCanceled()));
    mb->open();
}

void SymbianIDeviceConfigurationWidget::codaCanceled()
{
    clearDeviceInfo();
    m_deviceInfoButton->setEnabled(true);
    SymbianUtils::SymbianDeviceManager::instance()->releaseCodaDevice(m_codaInfoDevice);
}

} // namespace Internal
} // namespace Qt4ProjectManager
