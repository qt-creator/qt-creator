/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "androidavdmanager.h"
#include "androidconfigurations.h"
#include "androidconstants.h"
#include "androidmanager.h"
#include "androidsignaloperation.h"
#include "avddialog.h"

#include <coreplugin/icore.h>

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/idevicewidget.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <utils/qtcprocess.h>
#include <utils/runextensions.h>
#include <utils/url.h>

#include <QEventLoop>
#include <QFormLayout>
#include <QInputDialog>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QPushButton>

using namespace ProjectExplorer;

namespace {
static Q_LOGGING_CATEGORY(androidDeviceLog, "qtc.android.androiddevice", QtWarningMsg)
}

// interval for updating the list of connected Android devices and emulators
constexpr int deviceUpdaterMsInterval = 30000;

namespace Android {
namespace Internal {

class AndroidDeviceWidget : public IDeviceWidget
{
public:
    AndroidDeviceWidget(const ProjectExplorer::IDevice::Ptr &device);

    void updateDeviceFromUi() final {}
    static QString dialogTitle();
    static bool criticalDialog(const QString &error, QWidget *parent = nullptr);
    static bool questionDialog(const QString &question, QWidget *parent = nullptr);
};

AndroidDeviceWidget::AndroidDeviceWidget(const IDevice::Ptr &device)
    : IDeviceWidget(device)
{
    const auto dev = qSharedPointerCast<AndroidDevice>(device);
    const auto formLayout = new QFormLayout(this);
    formLayout->setFormAlignment(Qt::AlignLeft);
    formLayout->setContentsMargins(0, 0, 0, 0);
    setLayout(formLayout);
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    if (!dev->isValid())
        return;

    formLayout->addRow(AndroidDevice::tr("Device name:"), new QLabel(dev->displayName()));
    formLayout->addRow(AndroidDevice::tr("Device type:"), new QLabel(dev->deviceTypeName()));

    const QString serialNumber = dev->serialNumber();
    const QString printableSerialNumber = serialNumber.isEmpty() ? AndroidDevice::tr("Unknown")
                                                                 : serialNumber;
    formLayout->addRow(AndroidDevice::tr("Serial number:"), new QLabel(printableSerialNumber));

    const QString abis = dev->supportedAbis().join(", ");
    formLayout->addRow(AndroidDevice::tr("CPU architecture:"), new QLabel(abis));

    const auto osString = QString("%1 (SDK %2)").arg(dev->androidVersion()).arg(dev->sdkLevel());
    formLayout->addRow(AndroidDevice::tr("OS version:"), new QLabel(osString));

    if (dev->machineType() == IDevice::Hardware) {
        const QString authorizedStr = dev->deviceState() == IDevice::DeviceReadyToUse
                                          ? AndroidDevice::tr("Yes")
                                          : AndroidDevice::tr("No");
        formLayout->addRow(AndroidDevice::tr("Authorized:"), new QLabel(authorizedStr));
    }

    if (dev->machineType() == IDevice::Emulator) {
        const QString targetName = dev->androidTargetName();
        formLayout->addRow(AndroidDevice::tr("Android target flavor:"), new QLabel(targetName));
        formLayout->addRow(AndroidDevice::tr("SD card size:"), new QLabel(dev->sdcardSize()));
        formLayout->addRow(AndroidDevice::tr("Skin type:"), new QLabel(dev->skinName()));
        const QString openGlStatus = dev->openGlStatusString();
        formLayout->addRow(AndroidDevice::tr("OpenGL status:"), new QLabel(openGlStatus));
    }
}

QString AndroidDeviceWidget::dialogTitle()
{
    return AndroidDevice::tr("Android Device Manager");
}

bool AndroidDeviceWidget::criticalDialog(const QString &error, QWidget *parent)
{
    qCDebug(androidDeviceLog) << error;
    QMessageBox box(parent ? parent : Core::ICore::dialogParent());
    box.QDialog::setWindowTitle(dialogTitle());
    box.setText(error);
    box.setIcon(QMessageBox::Critical);
    box.setWindowFlag(Qt::WindowTitleHint);
    return box.exec();
}

bool AndroidDeviceWidget::questionDialog(const QString &question, QWidget *parent)
{
    QMessageBox box(parent ? parent : Core::ICore::dialogParent());
    box.QDialog::setWindowTitle(dialogTitle());
    box.setText(question);
    box.setIcon(QMessageBox::Question);
    QPushButton *YesButton = box.addButton(QMessageBox::Yes);
    box.addButton(QMessageBox::No);
    box.setWindowFlag(Qt::WindowTitleHint);
    box.exec();

    if (box.clickedButton() == YesButton)
        return true;
    return false;
}

AndroidDevice::AndroidDevice()
{
    setupId(IDevice::AutoDetected, Constants::ANDROID_DEVICE_ID);
    setType(Constants::ANDROID_DEVICE_TYPE);
    setDefaultDisplayName(tr("Run on Android"));
    setDisplayType(tr("Android"));
    setMachineType(IDevice::Hardware);
    setOsType(Utils::OsTypeOtherUnix);
    setDeviceState(DeviceConnected);

    addDeviceAction({tr("Refresh"), [](const IDevice::Ptr &device, QWidget *parent) {
        Q_UNUSED(parent)
        AndroidDeviceManager::instance()->updateDeviceState(device);
    }});

    addEmulatorActionsIfNotFound();
}

void AndroidDevice::addEmulatorActionsIfNotFound()
{
    static const QString startAvdAction = tr("Start AVD");
    static const QString eraseAvdAction = tr("Erase AVD");
    static const QString avdArgumentsAction = tr("AVD Arguments");

    bool hasStartAction = false;
    bool hasEraseAction = false;
    bool hasAvdArgumentsAction = false;

    for (const DeviceAction &item : deviceActions()) {
        if (item.display == startAvdAction)
            hasStartAction = true;
        else if (item.display == eraseAvdAction)
            hasEraseAction = true;
        else if (item.display == avdArgumentsAction)
            hasAvdArgumentsAction = true;
    }

    if (machineType() == Emulator) {
        if (!hasStartAction) {
            addDeviceAction({startAvdAction, [](const IDevice::Ptr &device, QWidget *parent) {
                AndroidDeviceManager::instance()->startAvd(device, parent);
            }});
        }

        if (!hasEraseAction) {
            addDeviceAction({eraseAvdAction, [](const IDevice::Ptr &device, QWidget *parent) {
                AndroidDeviceManager::instance()->eraseAvd(device, parent);
            }});
        }

        if (!hasAvdArgumentsAction) {
            addDeviceAction({avdArgumentsAction, [](const IDevice::Ptr &device, QWidget *parent) {
                Q_UNUSED(device)
                AndroidDeviceManager::instance()->setEmulatorArguments(parent);
            }});
        }
    }
}

void AndroidDevice::fromMap(const QVariantMap &map)
{
    IDevice::fromMap(map);
    // Add Actions for Emulator is not added already.
    // This is needed because actions for Emulators and physical devices are not the same.
    addEmulatorActionsIfNotFound();
}

IDevice::Ptr AndroidDevice::create()
{
    return IDevice::Ptr(new AndroidDevice);
}

AndroidDeviceInfo AndroidDevice::androidDeviceInfoFromIDevice(const IDevice *dev)
{
    AndroidDeviceInfo info;
    info.state = dev->deviceState();
    info.avdname = dev->extraData(Constants::AndroidAvdName).toString();
    info.serialNumber = dev->extraData(Constants::AndroidSerialNumber).toString();
    info.cpuAbi = dev->extraData(Constants::AndroidCpuAbi).toStringList();
    info.avdTarget = dev->extraData(Constants::AndroidAvdTarget).toString();
    info.avdDevice = dev->extraData(Constants::AndroidAvdDevice).toString();
    info.avdSkin = dev->extraData(Constants::AndroidAvdSkin).toString();
    info.avdSdcardSize = dev->extraData(Constants::AndroidAvdSdcard).toString();
    info.sdk = dev->extraData(Constants::AndroidSdk).toInt();
    info.type = dev->machineType();

    return info;
}

void AndroidDevice::setAndroidDeviceInfoExtras(IDevice *dev, const AndroidDeviceInfo &info)
{
    dev->setExtraData(Constants::AndroidAvdName, info.avdname);
    dev->setExtraData(Constants::AndroidSerialNumber, info.serialNumber);
    dev->setExtraData(Constants::AndroidCpuAbi, info.cpuAbi);
    dev->setExtraData(Constants::AndroidAvdTarget, info.avdTarget);
    dev->setExtraData(Constants::AndroidAvdDevice, info.avdDevice);
    dev->setExtraData(Constants::AndroidAvdSkin, info.avdSkin);
    dev->setExtraData(Constants::AndroidAvdSdcard, info.avdSdcardSize);
    dev->setExtraData(Constants::AndroidSdk, info.sdk);
}

QString AndroidDevice::displayNameFromInfo(const AndroidDeviceInfo &info)
{
    return info.type == IDevice::Hardware
            ? AndroidConfigurations::currentConfig().getProductModel(info.serialNumber)
            : info.avdname;
}

Utils::Id AndroidDevice::idFromDeviceInfo(const AndroidDeviceInfo &info)
{
    const QString id = (info.type == IDevice::Hardware ? info.serialNumber : info.avdname);
    return  Utils::Id(Constants::ANDROID_DEVICE_ID).withSuffix(':' + id);
}

Utils::Id AndroidDevice::idFromAvdInfo(const CreateAvdInfo &info)
{
    return  Utils::Id(Constants::ANDROID_DEVICE_ID).withSuffix(':' + info.name);
}

QStringList AndroidDevice::supportedAbis() const
{
    return extraData(Constants::AndroidCpuAbi).toStringList();
}

bool AndroidDevice::canSupportAbis(const QStringList &abis) const
{
    // If the list is empty, no valid decision can be made, this means something is wrong
    // somewhere, but let's not stop deployment.
    QTC_ASSERT(!abis.isEmpty(), return true);

    const QStringList ourAbis = supportedAbis();
    QTC_ASSERT(!ourAbis.isEmpty(), return false);

    for (const QString &abi : abis)
        if (ourAbis.contains(abi))
            return true; // it's enough if only one abi match is found

    // If no exact match is found, let's take ABI backward compatibility into account
    // https://developer.android.com/ndk/guides/abis#android-platform-abi-support
    // arm64 usually can run {arm, armv7}, x86 can support {arm, armv7}, and 64-bit devices
    // can support their 32-bit variants.
    using namespace ProjectExplorer::Constants;
    const bool isTheirsArm = abis.contains(ANDROID_ABI_ARMEABI)
                                || abis.contains(ANDROID_ABI_ARMEABI_V7A);
    // The primary ABI at the first index
    const bool oursSupportsArm = ourAbis.first() == ANDROID_ABI_ARM64_V8A
                                || ourAbis.first() == ANDROID_ABI_X86;
    // arm64 and x86 can run armv7 and arm
    if (isTheirsArm && oursSupportsArm)
        return true;
    // x64 can run x86
    if (ourAbis.first() == ANDROID_ABI_X86_64 && abis.contains(ANDROID_ABI_X86))
        return true;

    return false;
}

bool AndroidDevice::canHandleDeployments() const
{
    // If hardware and disconned, it wouldn't be possilbe to start it, unlike an emulator
    if (machineType() == Hardware && deviceState() == DeviceDisconnected)
        return false;
    return true;
}

bool AndroidDevice::isValid() const
{
    return !serialNumber().isEmpty() || !avdName().isEmpty();
}

QString AndroidDevice::serialNumber() const
{
    const QString serialNumber = extraData(Constants::AndroidSerialNumber).toString();
    if (machineType() == Hardware)
        return serialNumber;

    return AndroidConfigurations::currentConfig().getRunningAvdsSerialNumber(avdName());
}

QString AndroidDevice::avdName() const
{
    return extraData(Constants::AndroidAvdName).toString();
}

int AndroidDevice::sdkLevel() const
{
    return extraData(Constants::AndroidSdk).toInt();
}

QString AndroidDevice::androidVersion() const
{
    return AndroidManager::androidNameForApiLevel(sdkLevel());
}

QString AndroidDevice::deviceTypeName() const
{
    if (machineType() == Emulator)
        return tr("Emulator for ") + extraData(Constants::AndroidAvdDevice).toString();
    return tr("Physical device");
}

QString AndroidDevice::skinName() const
{
    const QString skin = extraData(Constants::AndroidAvdSkin).toString();
    return skin.isEmpty() ? tr("None") : skin;
}

QString AndroidDevice::androidTargetName() const
{
    const QString target = extraData(Constants::AndroidAvdTarget).toString();
    return target.isEmpty() ? tr("Unknown") : target;
}

QString AndroidDevice::sdcardSize() const
{
    const QString size = extraData(Constants::AndroidAvdSdcard).toString();
    return size.isEmpty() ? tr("Unknown") : size;
}

AndroidConfig::OpenGl AndroidDevice::openGlStatus() const
{
    return AndroidConfigurations::currentConfig().getOpenGLEnabled(displayName());
}

QString AndroidDevice::openGlStatusString() const
{
    const AndroidConfig::OpenGl glStatus = AndroidConfigurations::currentConfig()
            .getOpenGLEnabled(displayName());
    switch (glStatus) {
    case (AndroidConfig::OpenGl::Enabled):
        return tr("Enabled");
    case (AndroidConfig::OpenGl::Disabled):
        return tr("Disabled");
    case (AndroidConfig::OpenGl::Unknown):
        return tr("Unknown");
    }
    return tr("Unknown");
}

IDevice::DeviceInfo AndroidDevice::deviceInformation() const
{
    return IDevice::DeviceInfo();
}

IDeviceWidget *AndroidDevice::createWidget()
{
    return new AndroidDeviceWidget(sharedFromThis());
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

void AndroidDeviceManager::updateDevicesList()
{
    // If a non-Android Kit is currently active, skip the device list update
    const Target *startupTarget = SessionManager::startupTarget();
    if (!startupTarget)
        return;

    const Kit *kit = startupTarget->kit();
    if (!kit)
        return;

    if (DeviceTypeKitAspect::deviceTypeId(kit) != Constants::ANDROID_DEVICE_TYPE)
        return;

    updateDevicesListOnce();
}

void AndroidDeviceManager::updateDevicesListOnce()
{
    if (!m_avdsFutureWatcher.isRunning() && m_androidConfig.adbToolPath().exists()) {
        m_avdsFutureWatcher.setFuture(m_avdManager.avdList());
        m_devicesFutureWatcher.setFuture(Utils::runAsync([this]() {
            return m_androidConfig.connectedDevices();
        }));
    }
}

void AndroidDeviceManager::updateDeviceState(const ProjectExplorer::IDevice::Ptr &device)
{
    const AndroidDevice *dev = static_cast<AndroidDevice *>(device.data());
    const QString serial = dev->serialNumber();
    DeviceManager *const devMgr = DeviceManager::instance();
    const Utils::Id id = dev->id();
    if (serial.isEmpty() && dev->machineType() == IDevice::Emulator) {
        devMgr->setDeviceState(id, IDevice::DeviceConnected);
        return;
    }

    const QStringList args = AndroidDeviceInfo::adbSelector(serial) << "shell" << "echo" << "1";
    const SdkToolResult result = AndroidManager::runAdbCommand(args);
    const int success = result.success();
    if (success)
        devMgr->setDeviceState(id, IDevice::DeviceReadyToUse);
    else if (dev->machineType() == IDevice::Emulator || result.stdErr().contains("unauthorized"))
        devMgr->setDeviceState(id, IDevice::DeviceConnected);
    else
        devMgr->setDeviceState(id, IDevice::DeviceDisconnected);
}

void AndroidDeviceManager::startAvd(const ProjectExplorer::IDevice::Ptr &device, QWidget *parent)
{
    Q_UNUSED(parent)
    const AndroidDevice *androidDev = static_cast<const AndroidDevice *>(device.data());
    const QString name = androidDev->avdName();
    qCDebug(androidDeviceLog, "Starting Android AVD id \"%s\".", qPrintable(name));
    Utils::runAsync([this, name, device]() {
        const QString serialNumber = m_avdManager.startAvd(name);
        // Mark the AVD as ReadyToUse once we know it's started
        if (!serialNumber.isEmpty()) {
            DeviceManager *const devMgr = DeviceManager::instance();
            devMgr->setDeviceState(device->id(), IDevice::DeviceReadyToUse);
        }
    });
}

void AndroidDeviceManager::eraseAvd(const IDevice::Ptr &device, QWidget *parent)
{
    if (device.isNull())
        return;

    if (device->machineType() == IDevice::Hardware)
        return;

    const QString name = static_cast<const AndroidDevice *>(device.data())->avdName();
    const QString question
        = AndroidDevice::tr("Erase the Android AVD \"%1\"?\nThis cannot be undone.").arg(name);
    if (!AndroidDeviceWidget::questionDialog(question, parent))
        return;

    qCDebug(androidDeviceLog) << QString("Erasing Android AVD \"%1\" from the system.").arg(name);
    m_removeAvdFutureWatcher.setFuture(Utils::runAsync([this, name, device]() {
        QPair<IDevice::ConstPtr, bool> pair;
        pair.first = device;
        pair.second = false;
        if (m_avdManager.removeAvd(name))
            pair.second = true;
        return pair;
    }));
}

void AndroidDeviceManager::handleAvdRemoved()
{
    const QPair<IDevice::ConstPtr, bool> result = m_removeAvdFutureWatcher.result();
    const QString name = result.first->displayName();
    if (result.second) {
        qCDebug(androidDeviceLog, "Android AVD id \"%s\" removed from the system.", qPrintable(name));
        // Remove the device from QtC after it's been removed using avdmanager.
        DeviceManager::instance()->removeDevice(result.first->id());
    } else {
        AndroidDeviceWidget::criticalDialog(QObject::tr("An error occurred while removing the "
                "Android AVD \"%1\" using avdmanager tool.").arg(name));
    }
}

void AndroidDeviceManager::setEmulatorArguments(QWidget *parent)
{
    const QString helpUrl =
            "https://developer.android.com/studio/run/emulator-commandline#startup-options";

    QInputDialog dialog(parent ? parent : Core::ICore::dialogParent());
    dialog.setWindowTitle(AndroidDevice::tr("Emulator Command-line Startup Options"));
    dialog.setLabelText(AndroidDevice::tr("Emulator command-line startup options "
                                          "(<a href=\"%1\">Help Web Page</a>):")
                            .arg(helpUrl));
    dialog.setTextValue(m_androidConfig.emulatorArgs().join(' '));

    if (auto label = dialog.findChild<QLabel*>()) {
        label->setOpenExternalLinks(true);
        label->setMinimumWidth(500);
    }

    if (dialog.exec() != QDialog::Accepted)
        return;

    m_androidConfig.setEmulatorArgs(Utils::ProcessArgs::splitArgs(dialog.textValue()));
}

void AndroidDeviceManager::setupDevicesWatcher()
{
    if (!m_devicesUpdaterTimer.isActive()) {
        // The call to avdmanager is always slower than the call to adb devices,
        // so connecting the slot to the slower call should be enough.
        connect(&m_avdsFutureWatcher, &QFutureWatcherBase::finished,
                this,  &AndroidDeviceManager::devicesListUpdated);
        connect(&m_devicesUpdaterTimer, &QTimer::timeout, this, [this]() {
            updateDevicesList();
        });
        m_devicesUpdaterTimer.start(deviceUpdaterMsInterval);
    }
    updateDevicesListOnce();
}

void AndroidDeviceManager::devicesListUpdated()
{
    QVector<AndroidDeviceInfo> connectedDevicesInfos;
    connectedDevicesInfos = m_devicesFutureWatcher.result();

    // For checking the state of avds, since running avds are assigned a serial number of
    // the form emulator-xxxx, thus we have to manually check for the names.
    const QStringList runningAvds = m_androidConfig.getRunningAvdsFromDevices(connectedDevicesInfos);

    AndroidDeviceInfoList devices = m_avdsFutureWatcher.result();
    const QSet<QString> startedAvds = Utils::transform<QSet>(connectedDevicesInfos,
                                                             &AndroidDeviceInfo::avdname);
    for (const AndroidDeviceInfo &dev : devices)
        if (!startedAvds.contains(dev.avdname))
            connectedDevicesInfos << dev;

    DeviceManager *const devMgr = DeviceManager::instance();

    QVector<IDevice::ConstPtr> existingDevs;
    QVector<IDevice::ConstPtr> connectedDevs;

    for (int i = 0; i < devMgr->deviceCount(); ++i) {
        const IDevice::ConstPtr dev = devMgr->deviceAt(i);
        if (dev->id().toString().startsWith(Constants::ANDROID_DEVICE_ID)) {
            existingDevs.append(dev);
        }
    }

    for (auto item : connectedDevicesInfos) {
        const Utils::Id deviceId = AndroidDevice::idFromDeviceInfo(item);
        const QString displayName = AndroidDevice::displayNameFromInfo(item);
        IDevice::ConstPtr dev = devMgr->find(deviceId);
        if (!dev.isNull()) {
            if (dev->displayName() == displayName) {
                IDevice::DeviceState newState;
                // If an AVD is not already running set its state to Connected instead of
                // ReadyToUse.
                if (dev->machineType() == IDevice::Emulator && !runningAvds.contains(displayName))
                    newState = IDevice::DeviceConnected;
                else
                    newState = item.state;
                if (dev->deviceState() != newState) {
                    qCDebug(androidDeviceLog, "Device id \"%s\" changed its state.",
                            dev->id().toString().toUtf8().data());
                    devMgr->setDeviceState(dev->id(), newState);
                }
                connectedDevs.append(dev);
                continue;
            } else {
                // DeviceManager doens't seem to hav a way to directly update the name, if the name
                // of the device has changed, remove it and register it again with the new name.
                devMgr->removeDevice(dev->id());
            }
        }

        AndroidDevice *newDev = new AndroidDevice();
        newDev->setupId(IDevice::AutoDetected, deviceId);
        newDev->setDisplayName(displayName);
        newDev->setMachineType(item.type);
        newDev->setDeviceState(item.state);
        AndroidDevice::setAndroidDeviceInfoExtras(newDev, item);
        qCDebug(androidDeviceLog, "Registering new Android device id \"%s\".",
                newDev->id().toString().toUtf8().data());
        const IDevice::ConstPtr constNewDev = IDevice::ConstPtr(newDev);
        devMgr->addDevice(constNewDev);
        connectedDevs.append(constNewDev);
    }

    // Set devices no longer connected to disconnected state.
    for (const IDevice::ConstPtr &dev : existingDevs) {
        if (dev->id() != Constants::ANDROID_DEVICE_ID && !connectedDevs.contains(dev)
                && dev->deviceState() != IDevice::DeviceDisconnected) {
            qCDebug(androidDeviceLog, "Device id \"%s\" is no longer connected.",
                    dev->id().toString().toUtf8().data());
            devMgr->setDeviceState(dev->id(), IDevice::DeviceDisconnected);
        }
    }
}

AndroidDeviceManager *AndroidDeviceManager::instance()
{
    static AndroidDeviceManager obj;
    return &obj;
}

AndroidDeviceManager::AndroidDeviceManager(QObject *parent)
    : QObject(parent),
      m_androidConfig(AndroidConfigurations::currentConfig()),
      m_avdManager(m_androidConfig)
{
    connect(qApp, &QCoreApplication::aboutToQuit, this, [this]() {
        m_devicesUpdaterTimer.stop();
        m_avdsFutureWatcher.waitForFinished();
        m_devicesFutureWatcher.waitForFinished();
        m_removeAvdFutureWatcher.waitForFinished();
    });

    connect(&m_removeAvdFutureWatcher, &QFutureWatcherBase::finished,
            this, &AndroidDeviceManager::handleAvdRemoved);
}

// Factory

AndroidDeviceFactory::AndroidDeviceFactory()
    : IDeviceFactory(Constants::ANDROID_DEVICE_TYPE),
      m_androidConfig(AndroidConfigurations::currentConfig())
{
    setDisplayName(AndroidDevice::tr("Android Device"));
    setCombinedIcon(":/android/images/androiddevicesmall.png",
                    ":/android/images/androiddevice.png");

    setConstructionFunction(&AndroidDevice::create);
    if (m_androidConfig.sdkToolsOk()) {
        setCreator([this] {
            AvdDialog dialog = AvdDialog(m_androidConfig, Core::ICore::dialogParent());
            if (dialog.exec() != QDialog::Accepted)
                return IDevice::Ptr();

            const IDevice::Ptr dev = dialog.device();
            if (const auto androidDev = static_cast<AndroidDevice *>(dev.data())) {
                qCDebug(androidDeviceLog, "Created new Android AVD id \"%s\".",
                        qPrintable(androidDev->avdName()));
            } else {
                AndroidDeviceWidget::criticalDialog(
                        AndroidDevice::tr("The device info returned from AvdDialog is invalid."));
            }

            return IDevice::Ptr(dev);
        });
    }
}

} // namespace Internal
} // namespace Android
