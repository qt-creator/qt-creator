// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "iosdeploystep.h"

#include "iosconstants.h"
#include "iosdevice.h"
#include "iosrunconfiguration.h"
#include "iossimulator.h"
#include "iostoolhandler.h"
#include "iostr.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/devicesupport/devicemanager.h>

#include <solutions/tasking/tasktree.h>

#include <utils/temporaryfile.h>

#include <QFile>
#include <QSettings>

using namespace ProjectExplorer;
using namespace Tasking;
using namespace Utils;

namespace Ios::Internal {

class IosTransfer : public QObject
{
    Q_OBJECT

public:
    void setDeviceType(const IosDeviceType &deviceType) { m_deviceType = deviceType; }
    void setBundlePath(const FilePath &bundlePath) { m_bundlePath = bundlePath; }
    void setExpectSuccess(bool success) { m_expectSuccess = success; }
    void start()
    {
        QTC_ASSERT(m_deviceType, emit done(false); return);
        QTC_ASSERT(!m_toolHandler, return);

        m_toolHandler.reset(new IosToolHandler(*m_deviceType));
        connect(m_toolHandler.get(), &IosToolHandler::isTransferringApp, this,
                [this](IosToolHandler *, const FilePath &, const QString &,
                       int progress, int maxProgress, const QString &info) {
            emit progressValueChanged(progress * 100 / maxProgress, info);
        });
        connect(m_toolHandler.get(), &IosToolHandler::errorMsg, this,
                [this](IosToolHandler *, const QString &message) {
            if (message.contains(QLatin1String("AMDeviceInstallApplication returned -402653103")))
                TaskHub::addTask(DeploymentTask(Task::Warning, Tr::tr("The Info.plist might be incorrect.")));
            emit errorMessage(message);
        });
        connect(m_toolHandler.get(), &IosToolHandler::didTransferApp, this,
                [this](IosToolHandler *, const FilePath &, const QString &,
                       IosToolHandler::OpStatus status) {
            disconnect(m_toolHandler.get(), nullptr, this, nullptr);
            m_toolHandler.release()->deleteLater();
            if (status != IosToolHandler::Success && m_expectSuccess) {
                TaskHub::addTask(DeploymentTask(Task::Error, Tr::tr("Deployment failed. "
                    "The settings in the Devices window of Xcode might be incorrect.")));
            }
            emit done(status == IosToolHandler::Success);
        });
        connect(m_toolHandler.get(), &IosToolHandler::finished, this, [this] {
            disconnect(m_toolHandler.get(), nullptr, this, nullptr);
            m_toolHandler.release()->deleteLater();
            TaskHub::addTask(DeploymentTask(Task::Error, Tr::tr("Deployment failed.")));
            emit done(false);
        });
        m_toolHandler->requestTransferApp(m_bundlePath, m_deviceType->identifier);
    }

signals:
    void done(bool success);
    void progressValueChanged(int progress, const QString &info); // progress in %
    void errorMessage(const QString &message);

private:
    std::optional<IosDeviceType> m_deviceType;
    FilePath m_bundlePath;
    bool m_expectSuccess = true;
    std::unique_ptr<IosToolHandler> m_toolHandler;
};

class IosTransferTaskAdapter : public TaskAdapter<IosTransfer>
{
public:
    IosTransferTaskAdapter() { connect(task(), &IosTransfer::done, this, &TaskInterface::done); }

private:
    void start() final { task()->start(); }
};

using IosTransferTask = CustomTask<IosTransferTaskAdapter>;

class IosDeployStep final : public BuildStep
{
public:
    IosDeployStep(BuildStepList *bc, Utils::Id id);

private:
    void cleanup();

    Tasking::GroupItem runRecipe() final;

    void updateDisplayNames();

    bool init() final;
    QWidget *createConfigWidget() final;
    IosDevice::ConstPtr iosdevice() const;
    IosSimulator::ConstPtr iossimulator() const;

    QString deviceId() const;
    bool checkProvisioningProfile();

    IDevice::ConstPtr m_device;
    FilePath m_bundlePath;
    IosDeviceType m_deviceType;
};

IosDeployStep::IosDeployStep(BuildStepList *parent, Utils::Id id)
    : BuildStep(parent, id)
{
    setImmutable(true);
    updateDisplayNames();
    connect(DeviceManager::instance(), &DeviceManager::updated,
            this, &IosDeployStep::updateDisplayNames);
    connect(target(), &Target::kitChanged,
            this, &IosDeployStep::updateDisplayNames);
}

void IosDeployStep::updateDisplayNames()
{
    IDevice::ConstPtr dev = DeviceKitAspect::device(kit());
    const QString devName = dev.isNull() ? IosDevice::name() : dev->displayName();
    setDisplayName(Tr::tr("Deploy to %1").arg(devName));
}

bool IosDeployStep::init()
{
    m_device = DeviceKitAspect::device(kit());
    auto runConfig = qobject_cast<const IosRunConfiguration *>(
        this->target()->activeRunConfiguration());
    QTC_ASSERT(runConfig, return false);
    m_bundlePath = runConfig->bundleDirectory();

    if (iosdevice()) {
        m_deviceType = IosDeviceType(IosDeviceType::IosDevice, deviceId());
    } else if (iossimulator()) {
        m_deviceType = runConfig->deviceType();
    } else {
        emit addOutput(Tr::tr("Error: no device available, deploy failed."),
                       OutputFormat::ErrorMessage);
        return false;
    }
    return true;
}

GroupItem IosDeployStep::runRecipe()
{
    const auto onSetup = [this](IosTransfer &transfer) {
        if (m_device.isNull()) {
            TaskHub::addTask(
                DeploymentTask(Task::Error, Tr::tr("Deployment failed. No iOS device found.")));
            return SetupResult::StopWithError;
        }
        transfer.setDeviceType(m_deviceType);
        transfer.setBundlePath(m_bundlePath);
        transfer.setExpectSuccess(checkProvisioningProfile());
        emit progress(0, Tr::tr("Transferring application"));
        connect(&transfer, &IosTransfer::progressValueChanged, this, &IosDeployStep::progress);
        connect(&transfer, &IosTransfer::errorMessage, this, [this](const QString &message) {
            emit addOutput(message, OutputFormat::ErrorMessage);
        });
        return SetupResult::Continue;
    };
    return IosTransferTask(onSetup);
}

QWidget *IosDeployStep::createConfigWidget()
{
    auto widget = new QWidget;

    widget->setObjectName("IosDeployStepWidget");

    connect(this, &ProjectConfiguration::displayNameChanged,
            this, &BuildStep::updateSummary);

    return widget;
}

QString IosDeployStep::deviceId() const
{
    if (iosdevice().isNull())
        return QString();
    return iosdevice()->uniqueDeviceID();
}

bool IosDeployStep::checkProvisioningProfile()
{
    IosDevice::ConstPtr device = iosdevice();
    if (device.isNull())
        return true;

    const FilePath provisioningFilePath = m_bundlePath.pathAppended("embedded.mobileprovision");
    // the file is a signed plist stored in DER format
    // we simply search for start and end of the plist instead of decoding the DER payload
    if (!provisioningFilePath.exists())
        return true;

    QFile provisionFile(provisioningFilePath.toString());
    if (!provisionFile.open(QIODevice::ReadOnly))
        return true;

    const QByteArray provisionData = provisionFile.readAll();
    const int start = provisionData.indexOf("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
    int end = provisionData.indexOf("</plist>");
    if (start == -1 || end == -1)
        return true;

    end += 8;
    TemporaryFile f("iosdeploy");
    if (!f.open())
        return true;

    f.write(provisionData.mid(start, end - start));
    f.flush();
    const QSettings provisionPlist(f.fileName(), QSettings::NativeFormat);
    if (!provisionPlist.contains(QLatin1String("ProvisionedDevices")))
        return true;

    const QStringList deviceIds = provisionPlist.value("ProvisionedDevices").toStringList();
    const QString targetId = device->uniqueInternalDeviceId();
    for (const QString &deviceId : deviceIds) {
        if (deviceId == targetId)
            return true;
    }

    const QString provisioningProfile = provisionPlist.value(QLatin1String("Name")).toString();
    const QString provisioningUid = provisionPlist.value(QLatin1String("UUID")).toString();
    const CompileTask task(Task::Warning,
              Tr::tr("The provisioning profile \"%1\" (%2) used to sign the application "
                     "does not cover the device %3 (%4). Deployment to it will fail.")
              .arg(provisioningProfile, provisioningUid, device->displayName(), targetId));
    emit addTask(task);
    return false;
}

IosDevice::ConstPtr IosDeployStep::iosdevice() const
{
    return m_device.dynamicCast<const IosDevice>();
}

IosSimulator::ConstPtr IosDeployStep::iossimulator() const
{
    return m_device.dynamicCast<const IosSimulator>();
}

// IosDeployStepFactory

IosDeployStepFactory::IosDeployStepFactory()
{
    registerStep<IosDeployStep>(Constants::IOS_DEPLOY_STEP_ID);
    setDisplayName(Tr::tr("Deploy to iOS device"));
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
    setSupportedDeviceTypes({Constants::IOS_DEVICE_TYPE, Constants::IOS_SIMULATOR_TYPE});
    setRepeatable(false);
}

} // Ios::Internal

#include "iosdeploystep.moc"
