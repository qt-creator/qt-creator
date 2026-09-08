// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "harmonyosbuilddevice.h"

#include "harmonyosconfigurations.h"
#include "harmonyosconstants.h"
#include "harmonyossdk.h"
#include "harmonyossettings.h"
#include "harmonyostr.h"

#include <cmakeprojectmanager/cmakeconfigitem.h>
#include <cmakeprojectmanager/cmakekitaspect.h>

#include <coreplugin/icore.h>

#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/sshparameters.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/sysrootkitaspect.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainkitaspect.h>

#include <remote/sshdevicewizard.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/qtcprocess.h>
#include <utils/temporarydirectory.h>

#include <QDialog>
#include <QLoggingCategory>
#include <QHostAddress>
#include <QTcpSocket>

using namespace ProjectExplorer;
using namespace Utils;
using namespace std::chrono_literals;

namespace HarmonyOs::Internal {

static Q_LOGGING_CATEGORY(buildDeviceLog, "qtc.harmonyos.builddevice", QtWarningMsg)

HarmonyOsBuildDevice::HarmonyOsBuildDevice()
{
    setType(Constants::HARMONYOS_BUILD_DEVICE_TYPE);
    setDisplayType(Tr::tr("HarmonyOS Build Device"));
    setDefaultDisplayName(Tr::tr("HarmonyOS Build Device"));

    // The toolchains there are ordinary Linux ones, and everything that looks for
    // them expects a Linux device.
    setOsType(OsTypeLinux);

    SshParameters sshParams = sshParameters();
    sshParams.setPort(Constants::HARMONYOS_SSH_PORT);
    setDefaultSshParameters(sshParams);
}

// A binary reaching the device unsigned is refused by its code signing, and one
// signed twice is refused as well, so an already signed binary is left alone.
Result<> HarmonyOsBuildDevice::ensureReachable(const FilePath &other) const
{
#ifdef Q_OS_OHOS
    if (other.isLocal() && sshParameters().host() == "127.0.0.1"
        && other.path().startsWith(Constants::HARMONYOS_USER_STORAGE)) {
        return ResultOk;
    }
#endif
    return LinuxDevice::ensureReachable(other);
}

Result<QByteArray> HarmonyOsBuildDevice::prepareExecutableForUpload(const QByteArray &binary) const
{
    const FilePath signTool = Sdk::binarySignTool(settings().sdkLocation());
    if (signTool.isEmpty()) {
        // Uploading it unsigned would leave a bridge that cannot run, which the
        // caller cannot tell from one that works.
        return ResultError(Tr::tr("No HarmonyOS SDK is configured to sign the bridge with. "
                                  "Set it up in Preferences > SDKs > HarmonyOS."));
    }

    TemporaryDirectory directory("qtc-harmonyos-sign");
    if (!directory.isValid())
        return ResultError(directory.errorString());
    const FilePath unsignedFile = directory.filePath("unsigned");
    const FilePath signedFile = directory.filePath("signed");
    if (const Result<qint64> written = unsignedFile.writeFileContents(binary); !written)
        return ResultError(written.error());

    struct SignToolRun
    {
        bool succeeded = false;
        QString output;
    };
    const auto runSignTool = [signTool](const QStringList &arguments) {
        Process process;
        process.setCommand({signTool, arguments});
        process.runBlocking(30s);
        return SignToolRun{process.result() == ProcessResult::FinishedWithSuccess,
                           process.allOutput()};
    };

    // The marker decides, not the exit status: the tool may well report a missing
    // signature as a failure. But a run that neither found the marker nor
    // succeeded says nothing, and reading that as "already signed" would upload
    // the unsigned binary this function exists to prevent.
    const SignToolRun state = runSignTool({"display-sign", "-inFile", unsignedFile.nativePath()});
    if (!state.output.contains("code signature is not found")) {
        if (!state.succeeded) {
            return ResultError(Tr::tr("Could not tell whether the bridge is signed: %1")
                                   .arg(state.output.trimmed()));
        }
        return binary;
    }

    const SignToolRun signing = runSignTool({"sign", "-selfSign", "1",
                                             "-inFile", unsignedFile.nativePath(),
                                             "-outFile", signedFile.nativePath()});
    if (!signedFile.exists())
        return ResultError(Tr::tr("Failed to sign the bridge: %1").arg(signing.output.trimmed()));
    return signedFile.fileContents();
}

#ifdef Q_OS_OHOS

// What the platform installs from an application's native package is the only thing it may
// execute, and nothing searches there. Appended rather than prepended: it is where a tool
// is found when nothing else provides it, not a way to override what does.
static void addNativePackageToPath()
{
    const FilePath bin = FilePath::fromString(Constants::HARMONYOS_NATIVE_PACKAGE_BIN);
    if (Environment::systemEnvironment().pathListValue("PATH").contains(bin))
        return;
    Environment::modifySystemEnvironment(
        {{"PATH", bin.path(), EnvironmentItem::Append}});
}

// Qt Creator running on a HarmonyOS device is one process boundary away from a toolchain.
// Nothing in its own sandbox may execute a compiler - the platform refuses to execute any
// file that did not arrive in an installed package - but the terminal application's SSH
// server on loopback answers, and its environment holds a native clang and cmake. That
// connection is how anything gets built here, so the device for it is worth finding rather
// than asking for: everything about it is known except whether something is listening.
static bool somethingSpeaksSshOnLoopback()
{
    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, Constants::HARMONYOS_SSH_PORT);
    if (!socket.waitForConnected(1000) || !socket.waitForReadyRead(2000))
        return false;
    return socket.readAll().startsWith("SSH-");
}

// The server is reached with a key, and ssh has a place for those: the one it looks in
// by default, so nothing has to be configured and the file is where anyone would look for
// it. ssh-keygen travels in the native package.
static FilePath privateKey()
{
    const FilePath key = FileUtils::homePath().pathAppended(".ssh/id_ed25519");
    if (key.exists())
        return key;
    // Not checked for existence first: what the platform installs there are symlinks into
    // a directory the application may execute from but not stat, so every such check says
    // the tool is not there while running it works.
    const FilePath keygen = FilePath::fromString(Constants::HARMONYOS_NATIVE_PACKAGE_BIN)
                                .pathAppended("ssh-keygen");
    if (const Result<> created = key.parentDir().ensureWritableDir(); !created)
        return {};

    Process process;
    process.setCommand({keygen, {"-t", "ed25519", "-q", "-N", "", "-f", key.path()}});
    process.runBlocking(30s);
    if (!key.exists())
        return {};
    return key;
}

static const char loopbackDeviceId[] = "HarmonyOs.BuildDevice.Loopback";

// Completes what is there rather than only creating what is not: a device from an earlier
// run whose key never got written would otherwise stay unusable forever.
static void detectLoopbackBuildDevice()
{
    IDevice::Ptr device;
    for (int i = 0, n = DeviceManager::deviceCount(); i < n; ++i) {
        const IDevice::Ptr known = DeviceManager::deviceAt(i);
        if (!known || known->type() != Constants::HARMONYOS_BUILD_DEVICE_TYPE)
            continue;
        if (known->id() != Utils::Id(loopbackDeviceId))
            return;                     // somebody configured their own, leave it alone
        device = known;
    }
    if (!device && !somethingSpeaksSshOnLoopback())
        return;

    const bool isNew = !device;
    if (isNew)
        device = HarmonyOsBuildDevice::create();

    SshParameters parameters = device->sshParameters();
    if (!parameters.privateKeyFile().isEmpty() && parameters.privateKeyFile().exists())
        return;                         // set up already

    parameters.setHost("127.0.0.1");
    parameters.setPort(Constants::HARMONYOS_SSH_PORT);
    // That server takes any name and runs the session as the application it belongs to,
    // so there is nothing to ask the user for.
    parameters.setUserName("device");
    const FilePath key = privateKey();
    if (key.isEmpty())
        return;
    parameters.setPrivateKeyFile(key);
    parameters.setAuthenticationType(SshParameters::AuthenticationTypeSpecificKey);
    DeviceRef(device).setSshParameters(parameters);

    if (isNew) {
        device->setupId(IDevice::AutoDetected, loopbackDeviceId);
        device->setDisplayName(Tr::tr("HarmonyOS Build Device (this device)"));
        DeviceManager::addDevice(device);
    }
    // The server has to be told to accept this key, which is not this plugin's to do
    // silently: it lives in another application's configuration.
    qCWarning(buildDeviceLog) << "Build device on loopback ready to authorise; add"
                              << key.stringAppended(".pub").path()
                              << "to ~/.ssh/authorized_keys on this device.";
}

// The compiler on the device's PATH is a link into an OpenHarmony SDK.
static FilePath sdkRootFromToolchain(const Toolchain *toolchain)
{
    const FilePath compiler = toolchain->compilerCommand();
    for (const FilePath &path : {compiler, compiler.symLinkTarget()}) {
        if (path.isEmpty())
            continue;
        const FilePath sdkRoot = path.parentDir().parentDir();
        if (!Sdk::sysrootPath(sdkRoot).isEmpty())
            return sdkRoot;
    }
    return {};
}

// The headers of EGL and the other platform libraries sit in that SDK's sysroot, which
// CMake looks at only once the kit names it. The Qt on the device is a native build, so
// unlike a cross-built one its qt.toolchain.cmake says nothing about OpenHarmony: without
// the SDK's own toolchain file chain-loaded behind it CMake produces a plain Linux
// executable rather than the application module the platform loads, and no deployment
// settings for the run configuration to read the application from.
static void completeKit(Kit *kit)
{
    const Toolchain *toolchain = ToolchainKitAspect::cxxToolchain(kit);
    if (!toolchain)
        return;
    const FilePath sdkRoot = sdkRootFromToolchain(toolchain);
    if (sdkRoot.isEmpty())
        return;

    if (SysRootKitAspect::sysRoot(kit).isEmpty())
        SysRootKitAspect::setSysRoot(kit, Sdk::sysrootPath(sdkRoot));

    using namespace CMakeProjectManager;
    const FilePath toolchainFile = Sdk::cmakeToolchainFile(sdkRoot);
    if (toolchainFile.isEmpty())
        return;
    const CMakeConfig before = CMakeConfigurationKitAspect::configuration(kit);
    CMakeConfig config = before;
    if (config.valueOf("QT_CHAINLOAD_TOOLCHAIN_FILE").isEmpty()) {
        config.insert(CMakeConfigItem("QT_CHAINLOAD_TOOLCHAIN_FILE", CMakeConfigItem::FILEPATH,
                                      toolchainFile.path().toUtf8()));
        // That toolchain file picks no architecture of its own and stops without one.
        config.insert(CMakeConfigItem("OHOS_ARCH", CMakeConfigItem::STRING,
                                      ohosAbiName(toolchain->targetAbi()).toUtf8()));
    }
    // The compiler that toolchain file picks is the SDK's own, and the one on the device's
    // PATH a link to it. A kit naming the link differs from what lands in the cache
    // forever, and every build asks whether to apply the difference, so leave the choice
    // where it is made.
    config.remove("CMAKE_C_COMPILER");
    config.remove("CMAKE_CXX_COMPILER");
    if (config != before)
        CMakeConfigurationKitAspect::setConfiguration(kit, config);
}

static bool isLoopbackKit(const Kit *kit)
{
    return BuildDeviceKitAspect::deviceId(kit) == Utils::Id(loopbackDeviceId);
}

static void detectToolsOnLoopbackDevice()
{
    const IDevice::Ptr device = DeviceManager::find(Utils::Id(loopbackDeviceId));
    if (!device || !somethingSpeaksSshOnLoopback())
        return;

    const ToolDetectionLogger logger([](const QString &message) {
        qCDebug(buildDeviceLog) << "detecting:" << message;
    });
    // Nothing can be read from the device before it is connected: until then it has no
    // file access, and every query about a path on it answers as if it did not exist.
    device->tryToConnect({device.get(), [device, logger](const Result<> &connected) {
        if (!connected) {
            qCWarning(buildDeviceLog) << "the build device refused the connection:"
                                      << connected.error();
            return;
        }
        // Kits that the generic kit setup created do not go through
        // KitManager::registerKit(), so no signal announced them.
        const QList<Kit *> kits = Utils::filtered(KitManager::kits(), &isLoopbackKit);
        if (!kits.isEmpty()) {
            for (Kit *kit : kits)
                completeKit(kit);
            return;
        }
        device->runAutoDetect(logger, [] {
            qCDebug(buildDeviceLog) << "detection done," << KitManager::kits().size() << "kits";
        });
    }});
}

#endif // Q_OS_OHOS

HarmonyOsBuildDeviceFactory::HarmonyOsBuildDeviceFactory()
    : IDeviceFactory(Constants::HARMONYOS_BUILD_DEVICE_TYPE)
{
    setDisplayName(Tr::tr("HarmonyOS Build Device"));
    setQuickCreationAllowed(true);
    setConstructionFunction([] { return HarmonyOsBuildDevice::create(); });
    setCreator([]() -> IDevice::Ptr {
        const HarmonyOsBuildDevice::Ptr device = HarmonyOsBuildDevice::create();
        Remote::SshDeviceWizard wizard(Tr::tr("New HarmonyOS Build Device Configuration Setup"),
                                       IDevice::Ptr(device));
        if (wizard.exec() != QDialog::Accepted)
            return {};
        return device;
    });
}

void setupHarmonyOsBuildDevice()
{
    static HarmonyOsBuildDeviceFactory theHarmonyOsBuildDeviceFactory;
#ifdef Q_OS_OHOS
    addNativePackageToPath();
    // Adding a device before the saved ones are restored loses it: the restore replaces
    // the list. By the time this plugin is initialized they may or may not be there yet.
    const auto whenRestored = [] {
        if (!DeviceManager::isLoaded() || !KitManager::isLoaded())
            return;
        detectLoopbackBuildDevice();
        detectToolsOnLoopbackDevice();
    };
    QObject::connect(KitManager::instance(), &KitManager::kitAdded,
                     KitManager::instance(), [](Kit *kit) {
        if (isLoopbackKit(kit))
            completeKit(kit);
    });
    QObject::connect(DeviceManager::instance(), &DeviceManager::devicesLoaded,
                     DeviceManager::instance(), whenRestored, Qt::SingleShotConnection);
    QObject::connect(KitManager::instance(), &KitManager::kitsLoaded,
                     KitManager::instance(), whenRestored, Qt::SingleShotConnection);
    whenRestored();
#endif
}

} // namespace HarmonyOs::Internal
