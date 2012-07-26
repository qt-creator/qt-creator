/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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

#ifndef REMOTELINUX_GENERICLINUXDEVICECONFIGURATIONWIDGET_H
#define REMOTELINUX_GENERICLINUXDEVICECONFIGURATIONWIDGET_H

#include <projectexplorer/devicesupport/idevicewidget.h>

#include "remotelinux_export.h"

namespace RemoteLinux {

namespace Ui {
class GenericLinuxDeviceConfigurationWidget;
}

class REMOTELINUX_EXPORT GenericLinuxDeviceConfigurationWidget
        : public ProjectExplorer::IDeviceWidget
{
    Q_OBJECT

public:
    explicit GenericLinuxDeviceConfigurationWidget(
        const ProjectExplorer::IDevice::Ptr &deviceConfig, QWidget *parent = 0);
    ~GenericLinuxDeviceConfigurationWidget();

private slots:
    void authenticationTypeChanged();
    void hostNameEditingFinished();
    void sshPortEditingFinished();
    void timeoutEditingFinished();
    void userNameEditingFinished();
    void passwordEditingFinished();
    void keyFileEditingFinished();
    void showPassword(bool showClearText);
    void handleFreePortsChanged();
    void setPrivateKey(const QString &path);
    void createNewKey();

private:
    void updatePortsWarningLabel();
    void initGui();

    Ui::GenericLinuxDeviceConfigurationWidget *m_ui;
};

} // namespace RemoteLinux

#endif // REMOTELINUX_GENERICLINUXDEVICECONFIGURATIONWIDGET_H
