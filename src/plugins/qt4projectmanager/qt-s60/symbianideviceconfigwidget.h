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

#ifndef SYMBIANIDEVICECONFIGURATIONWIDGET_H
#define SYMBIANIDEVICECONFIGURATIONWIDGET_H

#include <projectexplorer/devicesupport/idevicewidget.h>

#include <QWidget>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QLabel;
class QLineEdit;
class QComboBox;
class QToolButton;
class QCheckBox;
class QRadioButton;
QT_END_NAMESPACE

namespace Utils {
    class DetailsWidget;
    class IpAddressLineEdit;
}

namespace SymbianUtils {
class SymbianDevice;
}

namespace Coda {
    class CodaDevice;
    class CodaEvent;
    struct CodaCommandResult;
}

namespace Qt4ProjectManager {

class SymbianIDevice;

namespace Internal {

class SymbianIDeviceConfigurationWidget : public ProjectExplorer::IDeviceWidget
{
    Q_OBJECT

public:
    explicit SymbianIDeviceConfigurationWidget(const ProjectExplorer::IDevice::Ptr &rawDevice, QWidget *parent = 0);
    ~SymbianIDeviceConfigurationWidget();

signals:
    void infoCollected();
    void codaConnected();

private slots:
    void updateSerialDevices();
    void setSerialPort(int index);
    void updateDeviceInfo();
    void clearDeviceInfo();
    void updateCommunicationChannel();
    void updateCommunicationChannelUi();
    void updateWlanAddress(const QString &address);
    void cleanWlanAddress();
    void codaEvent(const Coda::CodaEvent &event);
    void collectingInfoFinished();
    void codaTimeout();
    void codaCanceled();
    void codaIncreaseProgress();

private:
    SymbianIDevice *symbianDevice() const;

    inline SymbianUtils::SymbianDevice rawDevice(int i) const;
    inline SymbianUtils::SymbianDevice currentRawDevice() const;

    void setDeviceInfoLabel(const QString &message, bool isError = false);

    QWidget *createCommunicationChannel();

    void getQtVersionCommandResult(const Coda::CodaCommandResult &result);
    void getRomInfoResult(const Coda::CodaCommandResult &result);
    void getInstalledPackagesResult(const Coda::CodaCommandResult &result);
    void getHalResult(const Coda::CodaCommandResult &result);

    Utils::DetailsWidget *m_detailsWidget;
    QComboBox *m_serialPortsCombo;
    QToolButton *m_deviceInfoButton;
    QLabel *m_deviceInfoDescriptionLabel;
    QLabel *m_deviceInfoLabel;
    QRadioButton *m_serialRadioButton;
    QRadioButton *m_wlanRadioButton;
    Utils::IpAddressLineEdit *m_ipAddress;
    QSharedPointer<Coda::CodaDevice> m_codaInfoDevice;
    QString m_deviceInfo;
    QTimer *m_codaTimeout;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // SYMBIANIDEVICECONFIGURATIONWIDGET_H
