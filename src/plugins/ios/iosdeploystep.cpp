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

#include "iosdeploystepwidget.h"
#include "iosdeploystep.h"
#include "iosbuildstep.h"
#include "iosconstants.h"
#include "iosrunconfiguration.h"
#include "iosmanager.h"
#include "iostoolhandler.h"

#include <coreplugin/messagemanager.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/devicesupport/devicemanager.h>

#include <qtsupport/qtkitinformation.h>

#include <utils/temporaryfile.h>

#include <QDir>
#include <QFile>
#include <QSettings>

#define ASSERT_STATE(state) ASSERT_STATE_GENERIC(State, state, m_state)

using namespace ProjectExplorer;

namespace Ios {
namespace Internal {

const Core::Id IosDeployStep::Id("Qt4ProjectManager.IosDeployStep");

IosDeployStep::IosDeployStep(BuildStepList *parent)
    : BuildStep(parent, Id)
    , m_expectFail(false)
{
    ctor();
}

IosDeployStep::IosDeployStep(BuildStepList *parent,
    IosDeployStep *other)
    : BuildStep(parent, other)
    , m_expectFail(false)
{
    ctor();
}

void IosDeployStep::ctor()
{
    m_toolHandler = 0;
    m_transferStatus = NoTransfer;
    cleanup();
    updateDisplayNames();
    connect(DeviceManager::instance(), &DeviceManager::updated,
            this, &IosDeployStep::updateDisplayNames);
    connect(target(), &Target::kitChanged,
            this, &IosDeployStep::updateDisplayNames);
}

void IosDeployStep::updateDisplayNames()
{
    IDevice::ConstPtr dev =
            DeviceKitInformation::device(target()->kit());
    const QString devName = dev.isNull() ? IosDevice::name() : dev->displayName();
    setDefaultDisplayName(tr("Deploy to %1").arg(devName));
    setDisplayName(tr("Deploy to %1").arg(devName));
}

bool IosDeployStep::init(QList<const BuildStep *> &earlierSteps)
{
    Q_UNUSED(earlierSteps);
    QTC_ASSERT(m_transferStatus == NoTransfer, return false);
    m_device = DeviceKitInformation::device(target()->kit());
    IosRunConfiguration * runConfig = qobject_cast<IosRunConfiguration *>(
                this->target()->activeRunConfiguration());
    QTC_ASSERT(runConfig, return false);
    m_bundlePath = runConfig->bundleDirectory().toString();

    if (iosdevice()) {
        m_deviceType = IosDeviceType(IosDeviceType::IosDevice, deviceId());
    } else if (iossimulator()) {
        m_deviceType = runConfig->deviceType();
    } else {
        emit addOutput(tr("Error: no device available, deploy failed."),
                       BuildStep::OutputFormat::ErrorMessage);
        return false;
    }
    return true;
}

void IosDeployStep::run(QFutureInterface<bool> &fi)
{
    m_futureInterface = fi;
    QTC_CHECK(m_transferStatus == NoTransfer);
    if (device().isNull()) {
        TaskHub::addTask(Task::Error, tr("Deployment failed. No iOS device found."),
                         ProjectExplorer::Constants::TASK_CATEGORY_DEPLOYMENT);
        reportRunResult(m_futureInterface, !iossimulator().isNull());
        cleanup();
        return;
    }
    m_toolHandler = new IosToolHandler(m_deviceType, this);
    m_transferStatus = TransferInProgress;
    m_futureInterface.setProgressRange(0, 200);
    m_futureInterface.setProgressValueAndText(0, QLatin1String("Transferring application"));
    m_futureInterface.reportStarted();
    connect(m_toolHandler, &IosToolHandler::isTransferringApp,
            this, &IosDeployStep::handleIsTransferringApp);
    connect(m_toolHandler, &IosToolHandler::didTransferApp,
            this, &IosDeployStep::handleDidTransferApp);
    connect(m_toolHandler, &IosToolHandler::finished,
            this, &IosDeployStep::handleFinished);
    connect(m_toolHandler, &IosToolHandler::errorMsg,
            this, &IosDeployStep::handleErrorMsg);
    checkProvisioningProfile();
    m_toolHandler->requestTransferApp(appBundle(), m_deviceType.identifier);
}

void IosDeployStep::cancel()
{
    if (m_toolHandler)
        m_toolHandler->stop();
}

void IosDeployStep::cleanup()
{
    QTC_CHECK(m_transferStatus != TransferInProgress);
    m_transferStatus = NoTransfer;
    m_device.clear();
    m_toolHandler = nullptr;
    m_expectFail = false;
}

void IosDeployStep::handleIsTransferringApp(IosToolHandler *handler, const QString &bundlePath,
                                            const QString &deviceId, int progress, int maxProgress,
                                            const QString &info)
{
    Q_UNUSED(handler); Q_UNUSED(bundlePath); Q_UNUSED(deviceId);
    QTC_CHECK(m_transferStatus == TransferInProgress);
    m_futureInterface.setProgressRange(0, maxProgress);
    m_futureInterface.setProgressValueAndText(progress, info);
}

void IosDeployStep::handleDidTransferApp(IosToolHandler *handler, const QString &bundlePath,
                                         const QString &deviceId, IosToolHandler::OpStatus status)
{
    Q_UNUSED(handler); Q_UNUSED(bundlePath); Q_UNUSED(deviceId);
    QTC_CHECK(m_transferStatus == TransferInProgress);
    if (status == IosToolHandler::Success) {
        m_transferStatus = TransferOk;
    } else {
        m_transferStatus = TransferFailed;
        if (!m_expectFail)
            TaskHub::addTask(Task::Error,
                             tr("Deployment failed. The settings in the Devices window of Xcode might be incorrect."),
                             ProjectExplorer::Constants::TASK_CATEGORY_DEPLOYMENT);
    }
    reportRunResult(m_futureInterface, status == IosToolHandler::Success);
}

void IosDeployStep::handleFinished(IosToolHandler *handler)
{
    switch (m_transferStatus) {
    case TransferInProgress:
        m_transferStatus = TransferFailed;
        TaskHub::addTask(Task::Error, tr("Deployment failed."),
                         ProjectExplorer::Constants::TASK_CATEGORY_DEPLOYMENT);
        reportRunResult(m_futureInterface, false);
        break;
    case NoTransfer:
    case TransferOk:
    case TransferFailed:
        break;
    }
    cleanup();
    handler->deleteLater();
    // move it when result is reported? (would need care to avoid problems with concurrent runs)
}

void IosDeployStep::handleErrorMsg(IosToolHandler *handler, const QString &msg)
{
    Q_UNUSED(handler);
    if (msg.contains(QLatin1String("AMDeviceInstallApplication returned -402653103")))
        TaskHub::addTask(Task::Warning,
                         tr("The Info.plist might be incorrect."),
                         ProjectExplorer::Constants::TASK_CATEGORY_DEPLOYMENT);
    emit addOutput(msg, BuildStep::OutputFormat::ErrorMessage);
}

BuildStepConfigWidget *IosDeployStep::createConfigWidget()
{
    return new IosDeployStepWidget(this);
}

bool IosDeployStep::fromMap(const QVariantMap &map)
{
    return BuildStep::fromMap(map);
}

QVariantMap IosDeployStep::toMap() const
{
    QVariantMap map = BuildStep::toMap();
    return map;
}

QString IosDeployStep::deviceId() const
{
    if (iosdevice().isNull())
        return QString();
    return iosdevice()->uniqueDeviceID();
}

QString IosDeployStep::appBundle() const
{
    return m_bundlePath;
}

void IosDeployStep::raiseError(const QString &errorString)
{
    emit addTask(Task(Task::Error, errorString, Utils::FileName::fromString(QString()), -1,
        ProjectExplorer::Constants::TASK_CATEGORY_DEPLOYMENT));
}

void IosDeployStep::writeOutput(const QString &text, OutputFormat format)
{
    emit addOutput(text, format);
}

void IosDeployStep::checkProvisioningProfile()
{
    IosDevice::ConstPtr device = iosdevice();
    if (device.isNull())
        return;

    Utils::FileName provisioningFilePath = Utils::FileName::fromString(appBundle());
    provisioningFilePath.appendPath(QLatin1String("embedded.mobileprovision"));

    // the file is a signed plist stored in DER format
    // we simply search for start and end of the plist instead of decoding the DER payload
    if (!provisioningFilePath.exists())
        return;
    QFile provisionFile(provisioningFilePath.toString());
    if (!provisionFile.open(QIODevice::ReadOnly))
        return;
    QByteArray provisionData = provisionFile.readAll();
    int start = provisionData.indexOf("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
    int end = provisionData.indexOf("</plist>");
    if (start == -1 || end == -1)
        return;
    end += 8;

    Utils::TemporaryFile f("iosdeploy");
    if (!f.open())
        return;
    f.write(provisionData.mid(start, end - start));
    f.flush();
    QSettings provisionPlist(f.fileName(), QSettings::NativeFormat);

    if (!provisionPlist.contains(QLatin1String("ProvisionedDevices")))
        return;
    QStringList deviceIds = provisionPlist.value(QLatin1String("ProvisionedDevices")).toStringList();
    QString targetId = device->uniqueDeviceID();
    foreach (const QString &deviceId, deviceIds) {
        if (deviceId == targetId)
            return;
    }

    m_expectFail = true;
    QString provisioningProfile = provisionPlist.value(QLatin1String("Name")).toString();
    QString provisioningUid = provisionPlist.value(QLatin1String("UUID")).toString();
    Task task(Task::Warning,
              tr("The provisioning profile \"%1\" (%2) used to sign the application "
                 "does not cover the device %3 (%4). Deployment to it will fail.")
              .arg(provisioningProfile, provisioningUid, device->displayName(),
                   targetId),
              Utils::FileName(), /* filename */
              -1, /* line */
              ProjectExplorer::Constants::TASK_CATEGORY_COMPILE);
    emit addTask(task);
}

IDevice::ConstPtr IosDeployStep::device() const
{
    return m_device;
}

IosDevice::ConstPtr IosDeployStep::iosdevice() const
{
    return m_device.dynamicCast<const IosDevice>();
}

IosSimulator::ConstPtr IosDeployStep::iossimulator() const
{
    return m_device.dynamicCast<const IosSimulator>();
}

} // namespace Internal
} // namespace Ios
