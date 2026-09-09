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

#include <projectexplorer/devicesupport/desktopdevice.h>
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
#include <utils/qtcprocess.h>
#include <utils/temporarydirectory.h>

#include <QDialog>
#include <QLoggingCategory>

using namespace ProjectExplorer;
using namespace Utils;
using namespace std::chrono_literals;

namespace HarmonyOs::Internal {

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

static Q_LOGGING_CATEGORY(buildDeviceLog, "qtc.harmonyos.builddevice", QtWarningMsg)

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

// The string is what settings from before the toolchain moved into Qt Creator's own
// package hold, when this device was an SSH server on loopback.
static const char thisDeviceId[] = "HarmonyOs.BuildDevice.Loopback";

// Everything a build needs travels in Qt Creator's own native package, which is also the
// only place the platform lets it execute anything from. So building here happens in this
// process, and the device to build on is this one: what it answers about files, processes
// and its environment is what any local device answers.
class HarmonyOsThisDevice final : public DesktopDevice
{
public:
    using Ptr = std::shared_ptr<HarmonyOsThisDevice>;

    static Ptr create() { return Ptr(new HarmonyOsThisDevice); }

    FilePath rootPath() const final { return filePath("/"); }

    // DesktopDevice detects its tools here, and no kit can be created while the settings
    // are still being read. Public because the device this plugin makes itself is not made
    // by IDeviceFactory, which would call it.
    void initDeviceToolAspects() final { IDevice::initDeviceToolAspects(); }

private:
    HarmonyOsThisDevice()
    {
        setupId(IDevice::AutoDetected, thisDeviceId);
        setType(Constants::HARMONYOS_BUILD_DEVICE_TYPE);
        setDisplayType(Tr::tr("HarmonyOS Build Device"));
        setDefaultDisplayName(Tr::tr("HarmonyOS Build Device (this device)"));
        setOsType(OsTypeLinux);
        // Made again on every start, which leaves the settings free to hold a device
        // someone configured themselves, and that is what the factory constructs.
        setPersistent(false);
        DeviceManager::setDeviceState(id(), IDevice::DeviceReadyToUse, false);
    }
};

// Nothing is probed for: a device to build on exists for as long as Qt Creator runs here.
static void detectThisBuildDevice()
{
    for (int i = 0, n = DeviceManager::deviceCount(); i < n; ++i) {
        const IDevice::Ptr known = DeviceManager::deviceAt(i);
        if (!known || known->type() != Constants::HARMONYOS_BUILD_DEVICE_TYPE)
            continue;
        if (known->id() != Utils::Id(thisDeviceId))
            return;                     // somebody configured their own, leave it alone
        // Settings written before this device stopped being saved hold it as the SSH
        // server it used to be, and what the factory made of that is not this device.
        DeviceManager::removeDevice(known->id());
        break;
    }
    const HarmonyOsThisDevice::Ptr device = HarmonyOsThisDevice::create();
    device->initDeviceToolAspects();
    DeviceManager::addDevice(device);
}

// The compiler on the device's PATH sits in a native package laid out like the OpenHarmony
// SDK it was taken from, one directory below the root the sysroot and the toolchain file
// are found under.
static FilePath sdkRootFromToolchain(const Toolchain *toolchain)
{
    const FilePath compiler = toolchain->compilerCommand();
    FilePaths candidates;
    for (const FilePath &path : {compiler, compiler.symLinkTarget()}) {
        if (!path.isEmpty())
            candidates.append(path.parentDir().parentDir());
    }
    // "/data/app/bin" is where the platform links what the installed native packages
    // provide. An entry there may be executed but neither read nor followed, so a compiler
    // detected under that name tells nothing about where its sysroot is. The package Qt
    // Creator carries it in does.
    candidates.append(FilePath::fromString(Constants::HARMONYOS_NATIVE_PACKAGE_BIN).parentDir());
    for (const FilePath &sdkRoot : candidates) {
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
    // The toolchain file picks the compiler itself. A kit naming it as well differs from
    // what lands in the cache forever, and every build asks whether to apply the
    // difference, so leave the choice where it is made.
    config.remove("CMAKE_C_COMPILER");
    config.remove("CMAKE_CXX_COMPILER");
    if (config != before)
        CMakeConfigurationKitAspect::setConfiguration(kit, config);
}

static bool isThisDeviceKit(const Kit *kit)
{
    return BuildDeviceKitAspect::deviceId(kit) == Utils::Id(thisDeviceId);
}

static void detectToolsOnThisDevice()
{
    const IDevice::Ptr device = DeviceManager::find(Utils::Id(thisDeviceId));
    if (!device)
        return;

    // Kits that the generic kit setup created do not go through KitManager::registerKit(),
    // so no signal announced them.
    const QList<Kit *> kits = Utils::filtered(KitManager::kits(), &isThisDeviceKit);
    if (!kits.isEmpty()) {
        for (Kit *kit : kits)
            completeKit(kit);
        return;
    }

    const ToolDetectionLogger logger([](const QString &message) {
        qCDebug(buildDeviceLog) << "detecting:" << message;
    });
    device->runAutoDetect(logger, [] {
        qCDebug(buildDeviceLog) << "detection done," << KitManager::kits().size() << "kits";
    });
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
        detectThisBuildDevice();
        detectToolsOnThisDevice();
    };
    QObject::connect(KitManager::instance(), &KitManager::kitAdded,
                     KitManager::instance(), [](Kit *kit) {
        if (isThisDeviceKit(kit))
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
