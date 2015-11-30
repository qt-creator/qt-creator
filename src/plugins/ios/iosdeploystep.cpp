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

#include <QDir>
#include <QTemporaryFile>
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
    connect(DeviceManager::instance(), SIGNAL(updated()),
            SLOT(updateDisplayNames()));
    connect(target(), SIGNAL(kitChanged()),
            SLOT(updateDisplayNames()));
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
    if (m_device.isNull()) {
        emit addOutput(tr("Error: no device available, deploy failed."),
                       BuildStep::ErrorMessageOutput);
        return false;
    }
    return true;
}

void IosDeployStep::run(QFutureInterface<bool> &fi)
{
    m_futureInterface = fi;
    QTC_CHECK(m_transferStatus == NoTransfer);
    if (iosdevice().isNull()) {
        if (iossimulator().isNull())
            TaskHub::addTask(Task::Error, tr("Deployment failed. No iOS device found."),
                             ProjectExplorer::Constants::TASK_CATEGORY_DEPLOYMENT);
        m_futureInterface.reportResult(!iossimulator().isNull());
        cleanup();
        emit finished();
        return;
    }
    m_transferStatus = TransferInProgress;
    QTC_CHECK(m_toolHandler == 0);
    m_toolHandler = new IosToolHandler(IosDeviceType(IosDeviceType::IosDevice), this);
    m_futureInterface.setProgressRange(0, 200);
    m_futureInterface.setProgressValueAndText(0, QLatin1String("Transferring application"));
    m_futureInterface.reportStarted();
    connect(m_toolHandler, SIGNAL(isTransferringApp(Ios::IosToolHandler*,QString,QString,int,int,QString)),
            SLOT(handleIsTransferringApp(Ios::IosToolHandler*,QString,QString,int,int,QString)));
    connect(m_toolHandler, SIGNAL(didTransferApp(Ios::IosToolHandler*,QString,QString,Ios::IosToolHandler::OpStatus)),
            SLOT(handleDidTransferApp(Ios::IosToolHandler*,QString,QString,Ios::IosToolHandler::OpStatus)));
    connect(m_toolHandler, SIGNAL(finished(Ios::IosToolHandler*)),
            SLOT(handleFinished(Ios::IosToolHandler*)));
    connect(m_toolHandler, SIGNAL(errorMsg(Ios::IosToolHandler*,QString)),
            SLOT(handleErrorMsg(Ios::IosToolHandler*,QString)));
    checkProvisioningProfile();
    m_toolHandler->requestTransferApp(appBundle(), deviceId());
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
    m_toolHandler = 0;
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
                             tr("Deployment failed. The settings in the Organizer window of Xcode might be incorrect."),
                             ProjectExplorer::Constants::TASK_CATEGORY_DEPLOYMENT);
    }
    m_futureInterface.reportResult(status == IosToolHandler::Success);
}

void IosDeployStep::handleFinished(IosToolHandler *handler)
{
    switch (m_transferStatus) {
    case TransferInProgress:
        m_transferStatus = TransferFailed;
        TaskHub::addTask(Task::Error, tr("Deployment failed."),
                         ProjectExplorer::Constants::TASK_CATEGORY_DEPLOYMENT);
        m_futureInterface.reportResult(false);
        break;
    case NoTransfer:
    case TransferOk:
    case TransferFailed:
        break;
    }
    cleanup();
    handler->deleteLater();
    // move it when result is reported? (would need care to avoid problems with concurrent runs)
    emit finished();
}

void IosDeployStep::handleErrorMsg(IosToolHandler *handler, const QString &msg)
{
    Q_UNUSED(handler);
    if (msg.contains(QLatin1String("AMDeviceInstallApplication returned -402653103")))
        TaskHub::addTask(Task::Warning,
                         tr("The Info.plist might be incorrect."),
                         ProjectExplorer::Constants::TASK_CATEGORY_DEPLOYMENT);
    emit addOutput(msg, BuildStep::ErrorMessageOutput);
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

    QTemporaryFile f;
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
