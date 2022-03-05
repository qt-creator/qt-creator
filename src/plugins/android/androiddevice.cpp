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
#include <QTimer>
#include <QRegularExpression>

#include <utils/qtcprocess.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace {
static Q_LOGGING_CATEGORY(androidDeviceLog, "qtc.android.androiddevice", QtWarningMsg)
}

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
        const QString openGlStatus = dev->openGLStatus();
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
    setOsType(OsType::OsTypeOtherUnix);
    setDeviceState(DeviceDisconnected);

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
    initAvdSettings();
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
    info.avdName = dev->extraData(Constants::AndroidAvdName).toString();
    info.serialNumber = dev->extraData(Constants::AndroidSerialNumber).toString();
    info.cpuAbi = dev->extraData(Constants::AndroidCpuAbi).toStringList();
    const QString avdPath = dev->extraData(Constants::AndroidAvdPath).toString();
    info.avdPath = FilePath::fromUserInput(avdPath);
    info.sdk = dev->extraData(Constants::AndroidSdk).toInt();
    info.type = dev->machineType();

    return info;
}

QString AndroidDevice::displayNameFromInfo(const AndroidDeviceInfo &info)
{
    return info.type == IDevice::Hardware
            ? AndroidConfigurations::currentConfig().getProductModel(info.serialNumber)
            : info.avdName;
}

Id AndroidDevice::idFromDeviceInfo(const AndroidDeviceInfo &info)
{
    const QString id = (info.type == IDevice::Hardware ? info.serialNumber : info.avdName);
    return  Id(Constants::ANDROID_DEVICE_ID).withSuffix(':' + id);
}

Id AndroidDevice::idFromAvdInfo(const CreateAvdInfo &info)
{
    return  Id(Constants::ANDROID_DEVICE_ID).withSuffix(':' + info.name);
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

    return AndroidDeviceManager::instance()->getRunningAvdsSerialNumber(avdName());
}

QString AndroidDevice::avdName() const
{
    return extraData(Constants::AndroidAvdName).toString();
}

int AndroidDevice::sdkLevel() const
{
    return extraData(Constants::AndroidSdk).toInt();
}

FilePath AndroidDevice::avdPath() const
{
    return FilePath::fromUserInput(extraData(Constants::AndroidAvdPath).toString());
}

void AndroidDevice::setAvdPath(const FilePath &path)
{
    setExtraData(Constants::AndroidAvdPath, path.toUserOutput());
    initAvdSettings();
}

QString AndroidDevice::androidVersion() const
{
    return AndroidManager::androidNameForApiLevel(sdkLevel());
}

QString AndroidDevice::deviceTypeName() const
{
    if (machineType() == Emulator)
        return tr("Emulator for \"%1\"").arg(avdSettings()->value("hw.device.name").toString());
    return tr("Physical device");
}

QString AndroidDevice::skinName() const
{
    const QString skin = avdSettings()->value("skin.name").toString();
    return skin.isEmpty() ? tr("None") : skin;
}

QString AndroidDevice::androidTargetName() const
{
    const QString target = avdSettings()->value("tag.display").toString();
    return target.isEmpty() ? tr("Unknown") : target;
}

QString AndroidDevice::sdcardSize() const
{
    const QString size = avdSettings()->value("sdcard.size").toString();
    return size.isEmpty() ? tr("Unknown") : size;
}

QString AndroidDevice::openGLStatus() const
{
    const QString openGL = avdSettings()->value("hw.gpu.enabled").toString();
    return openGL.isEmpty() ? tr("Unknown") : openGL;
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

QSettings *AndroidDevice::avdSettings() const
{
    return m_avdSettings.get();
}

void AndroidDevice::initAvdSettings()
{
    const FilePath configPath = avdPath().resolvePath(QStringLiteral("config.ini"));
    m_avdSettings.reset(new QSettings(configPath.toUserOutput(), QSettings::IniFormat));
}

void AndroidDeviceManager::updateAvdsList()
{
    if (!m_avdsFutureWatcher.isRunning() && m_androidConfig.adbToolPath().exists())
        m_avdsFutureWatcher.setFuture(m_avdManager.avdList());
}

IDevice::DeviceState AndroidDeviceManager::getDeviceState(const QString &serial,
                                                          IDevice::MachineType type) const
{
    const QStringList args = AndroidDeviceInfo::adbSelector(serial) << "shell" << "echo 1";
    const SdkToolResult result = AndroidManager::runAdbCommand(args);
    if (result.success())
        return IDevice::DeviceReadyToUse;
    else if (type == IDevice::Emulator || result.stdErr().contains("unauthorized"))
        return IDevice::DeviceConnected;

    return IDevice::DeviceDisconnected;
}

void AndroidDeviceManager::updateDeviceState(const ProjectExplorer::IDevice::ConstPtr &device)
{
    const AndroidDevice *dev = static_cast<const AndroidDevice *>(device.data());
    const QString serial = dev->serialNumber();
    DeviceManager *const devMgr = DeviceManager::instance();
    const Utils::Id id = dev->id();
    if (!serial.isEmpty())
        devMgr->setDeviceState(id, getDeviceState(serial, dev->machineType()));
    else if (dev->machineType() == IDevice::Emulator)
        devMgr->setDeviceState(id, IDevice::DeviceConnected);
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

QString AndroidDeviceManager::emulatorName(const QString &serialNumber) const
{
    QStringList args = AndroidDeviceInfo::adbSelector(serialNumber);
    args.append({"emu", "avd", "name"});
    return AndroidManager::runAdbCommand(args).stdOut();
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

QString AndroidDeviceManager::getRunningAvdsSerialNumber(const QString &name) const
{
    for (const AndroidDeviceInfo &dev : m_androidConfig.connectedDevices()) {
        if (!dev.serialNumber.startsWith("emulator"))
            continue;
        const QString stdOut = emulatorName(dev.serialNumber);
        if (stdOut.isEmpty())
            continue; // Not an avd
        const QStringList outputLines = stdOut.split('\n');
        if (outputLines.size() > 1 && outputLines.first() == name)
            return dev.serialNumber;
    }

    return {};
}

void AndroidDeviceManager::setupDevicesWatcher()
{
    if (!m_androidConfig.adbToolPath().exists()) {
        qCDebug(androidDeviceLog) << "Cannot start ADB device watcher"
                                  <<  "because adb path does not exist.";
        return;
    }

    if (!m_adbDeviceWatcherProcess)
        m_adbDeviceWatcherProcess.reset(new Utils::QtcProcess(this));

    if (m_adbDeviceWatcherProcess->isRunning()) {
        qCDebug(androidDeviceLog) << "ADB device watcher is already running.";
        return;
    }

    connect(m_adbDeviceWatcherProcess.get(), &Utils::QtcProcess::finished, this,
            []() { qCDebug(androidDeviceLog) << "ADB device watcher finished."; });

    connect(m_adbDeviceWatcherProcess.get(), &Utils::QtcProcess::errorOccurred, this,
            [this](QProcess::ProcessError) {
        qCDebug(androidDeviceLog) << "ADB device watcher encountered an error:"
                                  << m_adbDeviceWatcherProcess->errorString();
        if (!m_adbDeviceWatcherProcess->isRunning()) {
            qCDebug(androidDeviceLog) << "Restarting the ADB device watcher now.";
            QTimer::singleShot(0, m_adbDeviceWatcherProcess.get(), &Utils::QtcProcess::start);
        }
    });

    m_adbDeviceWatcherProcess->setStdErrLineCallback([](const QString &error) {
        qCDebug(androidDeviceLog) << "ADB device watcher error" << error; });
    m_adbDeviceWatcherProcess->setStdOutLineCallback([this](const QString &output) {
        HandleDevicesListChange(output);
    });

    const Utils::CommandLine command = Utils::CommandLine(m_androidConfig.adbToolPath(),
                                                          {"track-devices"});
    m_adbDeviceWatcherProcess->setCommand(command);
    m_adbDeviceWatcherProcess->setEnvironment(AndroidConfigurations::toolsEnvironment(m_androidConfig));
    m_adbDeviceWatcherProcess->start();

    // Setup AVD filesystem watcher to listen for changes when an avd is created/deleted,
    // or started/stopped
    QString avdEnvVar = qEnvironmentVariable("ANDROID_AVD_HOME");
    if (avdEnvVar.isEmpty()) {
        avdEnvVar = qEnvironmentVariable("ANDROID_SDK_HOME");
        if (avdEnvVar.isEmpty())
            avdEnvVar = qEnvironmentVariable("HOME");
        avdEnvVar.append("/.android/avd");
    }
    const Utils::FilePath avdPath = Utils::FilePath::fromUserInput(avdEnvVar);
    m_avdFileSystemWatcher.addPath(avdPath.toString());
    connect(&m_avdsFutureWatcher, &QFutureWatcherBase::finished,
            this,  &AndroidDeviceManager::HandleAvdsListChange);
    connect(&m_avdFileSystemWatcher, &QFileSystemWatcher::directoryChanged, this, [this]() {
        // If the avd list upate command is running no need to call it again.
        if (!m_avdsFutureWatcher.isRunning())
            updateAvdsList();
    });
    // Call initial update
    updateAvdsList();
}

void AndroidDeviceManager::HandleAvdsListChange()
{
    DeviceManager *const devMgr = DeviceManager::instance();

    QVector<Id> existingAvds;
    for (int i = 0; i < devMgr->deviceCount(); ++i) {
        const IDevice::ConstPtr dev = devMgr->deviceAt(i);
        const bool isEmulator = dev->machineType() == IDevice::Emulator;
        if (isEmulator && dev->type() == Constants::ANDROID_DEVICE_TYPE)
            existingAvds.append(dev->id());
    }

    QVector<Id> connectedDevs;
    for (auto item : m_avdsFutureWatcher.result()) {
        const Utils::Id deviceId = AndroidDevice::idFromDeviceInfo(item);
        const QString displayName = AndroidDevice::displayNameFromInfo(item);
        IDevice::ConstPtr dev = devMgr->find(deviceId);
        if (!dev.isNull()) {
            const auto androidDev = static_cast<const AndroidDevice *>(dev.data());
            // DeviceManager doens't seem to have a way to directly update the name, if the name
            // of the device has changed, remove it and register it again with the new name.
            // Also account for the case of an AVD registered through old QC which might have
            // invalid data by checking if the avdPath is not empty.
            if (dev->displayName() != displayName || androidDev->avdPath().toString().isEmpty()) {
                devMgr->removeDevice(dev->id());
            } else {
                // Find the state of the AVD retrieved from the AVD watcher
                const QString serial = getRunningAvdsSerialNumber(item.avdName);
                if (!serial.isEmpty()) {
                    const IDevice::DeviceState state = getDeviceState(serial, IDevice::Emulator);
                    if (dev->deviceState() != state) {
                        devMgr->setDeviceState(dev->id(), state);
                        qCDebug(androidDeviceLog, "Device id \"%s\" changed its state.",
                                dev->id().toString().toUtf8().data());
                    }
                } else {
                    devMgr->setDeviceState(dev->id(), IDevice::DeviceConnected);
                }
                connectedDevs.append(dev->id());
                continue;
            }
        }

        AndroidDevice *newDev = new AndroidDevice();
        newDev->setupId(IDevice::AutoDetected, deviceId);
        newDev->setDisplayName(displayName);
        newDev->setMachineType(item.type);
        newDev->setDeviceState(item.state);

        newDev->setExtraData(Constants::AndroidAvdName, item.avdName);
        newDev->setExtraData(Constants::AndroidSerialNumber, item.serialNumber);
        newDev->setExtraData(Constants::AndroidCpuAbi, item.cpuAbi);
        newDev->setExtraData(Constants::AndroidSdk, item.sdk);
        newDev->setAvdPath(item.avdPath);

        qCDebug(androidDeviceLog, "Registering new Android device id \"%s\".",
                newDev->id().toString().toUtf8().data());
        const IDevice::ConstPtr constNewDev = IDevice::ConstPtr(newDev);
        devMgr->addDevice(IDevice::ConstPtr(constNewDev));
        connectedDevs.append(constNewDev->id());
    }

    // Set devices no longer connected to disconnected state.
    for (const Id &id : existingAvds) {
        if (!connectedDevs.contains(id)) {
                qCDebug(androidDeviceLog, "Removing AVD id \"%s\" because it no longer exists.",
                        id.toString().toUtf8().data());
                devMgr->removeDevice(id);
        }
    }
}

void AndroidDeviceManager::HandleDevicesListChange(const QString &serialNumber)
{
    DeviceManager *const devMgr = DeviceManager::instance();
    const QStringList serialBits = serialNumber.split('\t');
    if (serialBits.size() < 2)
        return;

    // Sample output of adb track-devices, the first 4 digits are for state type
    // and sometimes 4 zeros are reported as part for the serial number.
    // 00546db0e8d7 authorizing
    // 00546db0e8d7 device
    // 0000001711201JEC207789 offline
    // emulator-5554 device
    QString dirtySerial = serialBits.first().trimmed();
    if (dirtySerial.startsWith("0000"))
        dirtySerial = dirtySerial.mid(4);
    if (dirtySerial.startsWith("00"))
        dirtySerial = dirtySerial.mid(4);
    const bool isEmulator = dirtySerial.startsWith("emulator");

    const QString &serial = dirtySerial;
    const QString stateStr = serialBits.at(1).trimmed();

    IDevice::DeviceState state;
    if (stateStr == "device")
        state = IDevice::DeviceReadyToUse;
    else if (stateStr == "offline")
        state = IDevice::DeviceDisconnected;
    else
        state = IDevice::DeviceConnected;

    if (isEmulator) {
        const QString avdName = emulatorName(serial);
        const Utils::Id avdId = Utils::Id(Constants::ANDROID_DEVICE_ID).withSuffix(':' + avdName);
        devMgr->setDeviceState(avdId, state);
    } else {
        const Utils::Id id = Utils::Id(Constants::ANDROID_DEVICE_ID).withSuffix(':' + serial);
        QString displayName = AndroidConfigurations::currentConfig().getProductModel(serial);
        // Check if the device is connected via WiFi. A sample serial of such devices can be
        // like: "192.168.1.190:5555"
        const QRegularExpression wifiSerialRegExp(
                    QLatin1String("(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}):(\\d{1,5})"));
        if (wifiSerialRegExp.match(serial).hasMatch())
            displayName += QLatin1String(" (WiFi)");

        if (IDevice::ConstPtr dev = devMgr->find(id)) {
            // DeviceManager doens't seem to have a way to directly update the name, if the name
            // of the device has changed, remove it and register it again with the new name.
            if (dev->displayName() == displayName)
                devMgr->setDeviceState(id, state);
            else
                devMgr->removeDevice(id);
        } else {
            AndroidDevice *newDev = new AndroidDevice();
            newDev->setupId(IDevice::AutoDetected, id);
            newDev->setDisplayName(displayName);
            newDev->setMachineType(IDevice::Hardware);
            newDev->setDeviceState(state);

            newDev->setExtraData(Constants::AndroidSerialNumber, serial);
            newDev->setExtraData(Constants::AndroidCpuAbi, m_androidConfig.getAbis(serial));
            newDev->setExtraData(Constants::AndroidSdk, m_androidConfig.getSDKVersion(serial));

            qCDebug(androidDeviceLog, "Registering new Android device id \"%s\".",
                    newDev->id().toString().toUtf8().data());
            devMgr->addDevice(IDevice::ConstPtr(newDev));
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
        if (m_adbDeviceWatcherProcess) {
            m_adbDeviceWatcherProcess->terminate();
            m_adbDeviceWatcherProcess->waitForFinished();
            m_adbDeviceWatcherProcess.reset();
        }
        m_avdsFutureWatcher.waitForFinished();
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
