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

#ifndef S60DEPLOYCONFIGURATIONWIDGET_H
#define S60DEPLOYCONFIGURATIONWIDGET_H

#include <projectexplorer/deployconfiguration.h>

#include <QtGui/QWidget>
#include <QtCore/QPointer>

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

namespace trk {
    class Launcher;
}

namespace SymbianUtils {
class SymbianDevice;
}

namespace Coda {
    class CodaDevice;
    struct CodaCommandResult;
}

namespace Qt4ProjectManager {
namespace Internal {

class S60DeployConfiguration;

/* Configuration widget for S60 devices on serial ports that are
 * provided by the SerialDeviceLister class. Has an info/test
 * button connecting to the device and showing info. */
class S60DeployConfigurationWidget : public ProjectExplorer::DeployConfigurationWidget
{
    Q_OBJECT
public:
    explicit S60DeployConfigurationWidget(QWidget *parent = 0);
    ~S60DeployConfigurationWidget();

    void init(ProjectExplorer::DeployConfiguration *dc);

private slots:
    void updateTargetInformation();
    void updateInstallationDrives();
    void updateSerialDevices();
    void setInstallationDrive(int index);
    void setSerialPort(int index);
    void updateDeviceInfo();
    void clearDeviceInfo();
    void slotLauncherStateChanged(int);
    void slotWaitingForTrkClosed();
    void silentInstallChanged(int);
    void updateCommunicationChannel();
    void updateDebugClient();
    void updateWlanAddress(const QString &address);
    void cleanWlanAddress();

private:
    inline SymbianUtils::SymbianDevice device(int i) const;
    inline SymbianUtils::SymbianDevice currentDevice() const;

    void setDeviceInfoLabel(const QString &message, bool isError = false);

    QWidget * createCommunicationChannel();

    void getQtVersionCommandResult(const Coda::CodaCommandResult &result);
    void getRomInfoResult(const Coda::CodaCommandResult &result);
    void getInstalledPackagesResult(const Coda::CodaCommandResult &result);
    void getHalResult(const Coda::CodaCommandResult &result);

    S60DeployConfiguration *m_deployConfiguration;
    Utils::DetailsWidget *m_detailsWidget;
    QComboBox *m_serialPortsCombo;
    QLabel *m_sisFileLabel;
    QToolButton *m_deviceInfoButton;
    QLabel *m_deviceInfoDescriptionLabel;
    QLabel *m_deviceInfoLabel;
    QPointer<trk::Launcher> m_infoLauncher;
    QComboBox *m_installationDriveCombo;
    QCheckBox *m_silentInstallCheckBox;
    QRadioButton *m_serialRadioButton;
    QRadioButton *m_wlanRadioButton;
    Utils::IpAddressLineEdit *m_ipAddress;
    QRadioButton *m_trkRadioButton;
    QRadioButton *m_codaRadioButton;
    QSharedPointer<Coda::CodaDevice> m_codaInfoDevice;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // S60DEPLOYCONFIGURATIONWIDGET_H
