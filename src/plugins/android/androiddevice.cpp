/****************************************************************************
**
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

#include "androiddevice.h"
#include "androidconstants.h"
#include "androidsignaloperation.h"
#include "androidconfigurations.h"
#include "androidmanager.h"

#include <projectexplorer/runconfiguration.h>

#include <utils/url.h>

#include <QCoreApplication>
#include <QLoggingCategory>

using namespace ProjectExplorer;

namespace {
Q_LOGGING_CATEGORY(androidDeviceLog, "qtc.android.build.androiddevice", QtWarningMsg)
}

namespace Android {
namespace Internal {

AndroidDevice::AndroidDevice()
{
    setupId(IDevice::AutoDetected, Constants::ANDROID_DEVICE_ID);
    setType(Constants::ANDROID_DEVICE_TYPE);
    setDefaultDisplayName(QCoreApplication::translate("Android::Internal::AndroidDevice",
                                                      "Run on Android"));
    setDisplayType(QCoreApplication::translate("Android::Internal::AndroidDevice", "Android"));
    setMachineType(IDevice::Hardware);
    setOsType(Utils::OsTypeOtherUnix);

    setDeviceState(DeviceReadyToUse);
    const AndroidConfig &config = AndroidConfigurations::currentConfig();
    if (config.qtLiveApkPath().exists()) {
        QString activityPath;
        AndroidManager::apkInfo(config.qtLiveApkPath(), nullptr, nullptr, &activityPath);
        qCDebug(androidDeviceLog) << "Using Qt live apk from: " << config.qtLiveApkPath()
                                  << "Activity Path:" << activityPath;
        setQmlsceneCommand(activityPath);
    }
}

IDevice::DeviceInfo AndroidDevice::deviceInformation() const
{
    return IDevice::DeviceInfo();
}

IDeviceWidget *AndroidDevice::createWidget()
{
    return nullptr;
}

bool AndroidDevice::canAutoDetectPorts() const
{
    return true;
}

DeviceProcessSignalOperation::Ptr AndroidDevice::signalOperation() const
{
    return DeviceProcessSignalOperation::Ptr(new AndroidSignalOperation());
}

QUrl AndroidDevice::toolControlChannel(const ControlChannelHint &) const
{
    QUrl url;
    url.setScheme(Utils::urlTcpScheme());
    url.setHost("localhost");
    return url;
}


// Factory

AndroidDeviceFactory::AndroidDeviceFactory()
    : ProjectExplorer::IDeviceFactory(Constants::ANDROID_DEVICE_TYPE)
{
    setObjectName(QLatin1String("AndroidDeviceFactory"));
    setDisplayName(tr("Android Device"));
    setCombinedIcon(":/android/images/androiddevicesmall.png",
                    ":/android/images/androiddevice.png");
    setConstructionFunction(&AndroidDevice::create);
}

} // namespace Internal
} // namespace Android
