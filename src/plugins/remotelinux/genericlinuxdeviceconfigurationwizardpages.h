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

#include "linuxdevice.h"
#include "remotelinux_export.h"

#include <ssh/sshconnection.h>

#include <QWizardPage>

namespace RemoteLinux {
class LinuxDevice;
namespace Internal {
class GenericLinuxDeviceConfigurationWizardSetupPagePrivate;
class GenericLinuxDeviceConfigurationWizardFinalPagePrivate;
} // namespace Internal

class REMOTELINUX_EXPORT GenericLinuxDeviceConfigurationWizardSetupPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit GenericLinuxDeviceConfigurationWizardSetupPage(QWidget *parent = nullptr);
    ~GenericLinuxDeviceConfigurationWizardSetupPage() override;

    void setDevice(const LinuxDevice::Ptr &device);

private:
    void initializePage() override;
    bool isComplete() const override;
    bool validatePage() override;

    QString configurationName() const;
    QUrl url() const;

    Internal::GenericLinuxDeviceConfigurationWizardSetupPagePrivate * const d;
};

class REMOTELINUX_EXPORT GenericLinuxDeviceConfigurationWizardKeyDeploymentPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit GenericLinuxDeviceConfigurationWizardKeyDeploymentPage(QWidget *parent = nullptr);
    ~GenericLinuxDeviceConfigurationWizardKeyDeploymentPage() override;

    void setDevice(const LinuxDevice::Ptr &device);

private:
    void initializePage() override;
    bool isComplete() const override;
    bool validatePage() override;

    QString privateKeyFilePath() const;
    void createKey();
    void deployKey();

    struct Private;
    Private * const d;
};

class REMOTELINUX_EXPORT GenericLinuxDeviceConfigurationWizardFinalPage final : public QWizardPage
{
    Q_OBJECT
public:
    GenericLinuxDeviceConfigurationWizardFinalPage(QWidget *parent);
    ~GenericLinuxDeviceConfigurationWizardFinalPage() override;

protected:
    virtual QString infoText() const;

private:
    void initializePage() override;

    Internal::GenericLinuxDeviceConfigurationWizardFinalPagePrivate * const d;
};

} // namespace RemoteLinux
