/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

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
    void handleAuthTypeChanged();

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
