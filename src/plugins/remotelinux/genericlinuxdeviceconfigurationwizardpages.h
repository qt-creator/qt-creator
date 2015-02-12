/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#ifndef GENERICLINUXDEVICECONFIGURATIONWIZARDPAGES_H
#define GENERICLINUXDEVICECONFIGURATIONWIZARDPAGES_H

#include "remotelinux_export.h"

#include <ssh/sshconnection.h>

#include <QWizardPage>

namespace RemoteLinux {
namespace Internal {
class GenericLinuxDeviceConfigurationWizardSetupPagePrivate;
class GenericLinuxDeviceConfigurationWizardFinalPagePrivate;
} // namespace Internal

class REMOTELINUX_EXPORT GenericLinuxDeviceConfigurationWizardSetupPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit GenericLinuxDeviceConfigurationWizardSetupPage(QWidget *parent = 0);
    ~GenericLinuxDeviceConfigurationWizardSetupPage();

    void initializePage();
    bool isComplete() const;

    QString configurationName() const;
    QString hostName() const;
    QString userName() const;
    QSsh::SshConnectionParameters::AuthenticationType authenticationType() const;
    QString password() const;
    QString privateKeyFilePath() const;

    virtual QString defaultConfigurationName() const;
    virtual QString defaultHostName() const;
    virtual QString defaultUserName() const;
    virtual QString defaultPassWord() const;

private:
    Q_SLOT void handleAuthTypeChanged();

    Internal::GenericLinuxDeviceConfigurationWizardSetupPagePrivate * const d;
};


class REMOTELINUX_EXPORT GenericLinuxDeviceConfigurationWizardFinalPage : public QWizardPage
{
    Q_OBJECT
public:
    GenericLinuxDeviceConfigurationWizardFinalPage(QWidget *parent);
    ~GenericLinuxDeviceConfigurationWizardFinalPage();

    void initializePage();

protected:
    virtual QString infoText() const;

private:
    Internal::GenericLinuxDeviceConfigurationWizardFinalPagePrivate * const d;
};

} // namespace RemoteLinux

#endif // GENERICLINUXDEVICECONFIGURATIONWIZARDPAGES_H
