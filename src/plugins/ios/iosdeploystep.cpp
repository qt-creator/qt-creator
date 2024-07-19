// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "iosdeploystep.h"

#include "devicectlutils.h"
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

#include <utils/qtcprocess.h>
#include <utils/temporaryfile.h>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
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
        QTC_ASSERT(m_deviceType, emit done(DoneResult::Error); return);
        QTC_ASSERT(!m_toolHandler, return);

        m_toolHandler.reset(new IosToolHandler(*m_deviceType));
        connect(m_toolHandler.get(), &IosToolHandler::isTransferringApp, this,
                [this](IosToolHandler *, const FilePath &, const QString &,
                       int progress, int maxProgress, const QString &info) {
            emit progressValueChanged(progress * 100 / maxProgress, info);
        });
        connect(
            m_toolHandler.get(),
            &IosToolHandler::errorMsg,
            this,
            [this](IosToolHandler *, const QString &message) {
                TaskHub::addTask(DeploymentTask(Task::Error, message));
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
            emit done(toDoneResult(status == IosToolHandler::Success));
        });
        connect(m_toolHandler.get(), &IosToolHandler::finished, this, [this] {
            disconnect(m_toolHandler.get(), nullptr, this, nullptr);
            m_toolHandler.release()->deleteLater();
            TaskHub::addTask(DeploymentTask(Task::Error, Tr::tr("Deployment failed.")));
            emit done(DoneResult::Error);
        });
        m_toolHandler->requestTransferApp(m_bundlePath, m_deviceType->identifier);
    }

signals:
    void done(DoneResult result);
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

GroupItem createDeviceCtlDeployTask(
    const IosDevice::ConstPtr &device,
    const FilePath &bundlePath,
    const std::function<void(int)> &progressHandler,
    const std::function<void(QString, std::optional<Task::TaskType>)> &errorHandler)
{
    const auto onSetup = [=](Process &process) {
        if (!device) {
            TaskHub::addTask(
                DeploymentTask(Task::Error, Tr::tr("Deployment failed. No iOS device found.")));
            return SetupResult::StopWithError;
        }
        process.setCommand({FilePath::fromString("/usr/bin/xcrun"),
                            {"devicectl",
                             "device",
                             "install",
                             "app",
                             "--device",
                             device->uniqueInternalDeviceId(),
                             "--quiet",
                             "--json-output",
                             "-",
                             bundlePath.path()}});
        // TODO Use process.setStdOutCallback to parse progress information.
        // Progress information looks like
        // 1%... 2%... 3%... 4%... 5%...
        progressHandler(0);
        return SetupResult::Continue;
    };
    const auto onDone = [=](const Process &process, DoneWith result) -> DoneResult {
        if (result == DoneWith::Cancel) {
            errorHandler(Tr::tr("Deployment canceled."), {});
            return DoneResult::Error;
        }
        if (process.error() != QProcess::UnknownError) {
            errorHandler(Tr::tr("Failed to run devicectl: %1.").arg(process.errorString()),
                         Task::Error);
            return DoneResult::Error;
        }
        const Utils::expected_str<QJsonValue> resultValue = parseDevicectlResult(
            process.rawStdOut());
        if (resultValue) {
            // success
            if ((*resultValue)["installedApplications"].isUndefined()) {
                // something unexpected happened ...
                errorHandler(
                    Tr::tr(
                        "devicectl returned unexpected output ... deployment might have failed."),
                    Task::Error);
                // ... proceed anyway
            }
            return DoneResult::Success;
        }
        // failure
        errorHandler(resultValue.error(), Task::Error);
        return DoneResult::Error;
    };
    return ProcessTask(onSetup, onDone);
}

class IosDeployStep final : public BuildStep
{
public:
    IosDeployStep(BuildStepList *bc, Utils::Id id);

private:
    void cleanup();

    GroupItem runRecipe() final;

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
    const QString devName = dev ? dev->displayName() : IosDevice::name();
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
    static const QString transferringMessage = Tr::tr("Transferring application");
    if (iosdevice() && iosdevice()->handler() == IosDevice::Handler::DeviceCtl) {
        const auto handleProgress = [this](int value) { emit progress(value, transferringMessage); };
        const auto handleError = [this](const QString &error,
                                        const std::optional<Task::TaskType> &taskType) {
            emit addOutput(error, OutputFormat::ErrorMessage);
            if (taskType)
                TaskHub::addTask(DeploymentTask(*taskType, error));
        };
        return createDeviceCtlDeployTask(iosdevice(), m_bundlePath, handleProgress, handleError);
    }
    // otherwise use iostool:
    const auto onSetup = [this](IosTransfer &transfer) {
        if (!m_device) {
            TaskHub::addTask(
                DeploymentTask(Task::Error, Tr::tr("Deployment failed. No iOS device found.")));
            return SetupResult::StopWithError;
        }
        transfer.setDeviceType(m_deviceType);
        transfer.setBundlePath(m_bundlePath);
        transfer.setExpectSuccess(checkProvisioningProfile());
        emit progress(0, transferringMessage);
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
    if (!iosdevice())
        return {};
    return iosdevice()->uniqueDeviceID();
}

bool IosDeployStep::checkProvisioningProfile()
{
    IosDevice::ConstPtr device = iosdevice();
    if (!device)
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
    return std::dynamic_pointer_cast<const IosDevice>(m_device);
}

IosSimulator::ConstPtr IosDeployStep::iossimulator() const
{
    return std::dynamic_pointer_cast<const IosSimulator>(m_device);
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
