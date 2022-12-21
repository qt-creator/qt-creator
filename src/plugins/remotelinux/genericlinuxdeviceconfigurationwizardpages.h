// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "remotelinux_export.h"

#include "linuxdevice.h"

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

    Utils::FilePath privateKeyFilePath() const;
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
