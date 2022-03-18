/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androidavdmanager.h"
#include "androidconfigurations.h"
#include "androiddeviceinfo.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/idevicefactory.h>

#include <QFutureWatcher>
#include <QFileSystemWatcher>
#include <QSettings>

namespace Utils { class QtcProcess; }

namespace Android {
namespace Internal {

class AndroidDevice final : public ProjectExplorer::IDevice
{
    Q_DECLARE_TR_FUNCTIONS(Android::Internal::AndroidDevice)

public:
    AndroidDevice();

    static IDevice::Ptr create();
    static AndroidDeviceInfo androidDeviceInfoFromIDevice(const IDevice *dev);

    static QString displayNameFromInfo(const AndroidDeviceInfo &info);
    static Utils::Id idFromDeviceInfo(const AndroidDeviceInfo &info);
    static Utils::Id idFromAvdInfo(const CreateAvdInfo &info);

    QStringList supportedAbis() const;
    bool canSupportAbis(const QStringList &abis) const;

    bool canHandleDeployments() const;

    bool isValid() const;
    QString serialNumber() const;
    QString avdName() const;
    int sdkLevel() const;

    Utils::FilePath avdPath() const;
    void setAvdPath(const Utils::FilePath &path);

    QString deviceTypeName() const;
    QString androidVersion() const;

    // AVD specific
    QString skinName() const;
    QString androidTargetName() const;
    QString sdcardSize() const;
    QString openGLStatus() const;

protected:
    void fromMap(const QVariantMap &map) final;

private:
    void addEmulatorActionsIfNotFound();
    ProjectExplorer::IDevice::DeviceInfo deviceInformation() const override;
    ProjectExplorer::IDeviceWidget *createWidget() override;
    bool canAutoDetectPorts() const override;
    ProjectExplorer::DeviceProcessSignalOperation::Ptr signalOperation() const override;
    QUrl toolControlChannel(const ControlChannelHint &) const override;

    QSettings *avdSettings() const;
    void initAvdSettings();

    std::unique_ptr<QSettings> m_avdSettings;
};

class AndroidDeviceFactory final : public ProjectExplorer::IDeviceFactory
{
public:
    AndroidDeviceFactory();

private:
    const AndroidConfig &m_androidConfig;
};

class AndroidDeviceManager : public QObject
{
public:
    static AndroidDeviceManager *instance();
    void setupDevicesWatcher();
    void updateAvdsList();
    IDevice::DeviceState getDeviceState(const QString &serial, IDevice::MachineType type) const;
    void updateDeviceState(const ProjectExplorer::IDevice::ConstPtr &device);

    void startAvd(const ProjectExplorer::IDevice::Ptr &device, QWidget *parent = nullptr);
    void eraseAvd(const ProjectExplorer::IDevice::Ptr &device, QWidget *parent = nullptr);

    void setEmulatorArguments(QWidget *parent = nullptr);

    QString getRunningAvdsSerialNumber(const QString &name) const;

private:
    AndroidDeviceManager(QObject *parent = nullptr);
    void HandleDevicesListChange(const QString &serialNumber);
    void HandleAvdsListChange();
    void handleAvdRemoved();

    QString emulatorName(const QString &serialNumber) const;

    QFutureWatcher<AndroidDeviceInfoList> m_avdsFutureWatcher;
    QFutureWatcher<QPair<ProjectExplorer::IDevice::ConstPtr, bool>> m_removeAvdFutureWatcher;
    QFileSystemWatcher m_avdFileSystemWatcher;
    std::unique_ptr<Utils::QtcProcess> m_adbDeviceWatcherProcess;
    AndroidConfig &m_androidConfig;
    AndroidAvdManager m_avdManager;
};

} // namespace Internal
} // namespace Android
