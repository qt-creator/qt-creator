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

#ifndef S60DEPLOYCONFIGURATIONWIDGET_H
#define S60DEPLOYCONFIGURATIONWIDGET_H

#include <projectexplorer/deployconfiguration.h>

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

class S60DeployConfiguration;

namespace Internal {

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
    void setInstallationDrive(int index);
    void silentInstallChanged(int);

private:
    void setDeviceInfoLabel(const QString &message, bool isError = false);

    S60DeployConfiguration *m_deployConfiguration;
    Utils::DetailsWidget *m_detailsWidget;
    QLabel *m_sisFileLabel;
    QToolButton *m_deviceInfoButton;
    QComboBox *m_installationDriveCombo;
    QCheckBox *m_silentInstallCheckBox;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // S60DEPLOYCONFIGURATIONWIDGET_H
