// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qnxtoolchain.h"

#include "qnxconstants.h"
#include "qnxtr.h"
#include "qnxutils.h"

#include <debugger/debuggeritem.h>
#include <debugger/debuggeritemmanager.h>

#include <projectexplorer/abiwidget.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchainconfigwidget.h>

#include <utils/algorithm.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>

#include <QFormLayout>

using namespace ProjectExplorer;
using namespace Utils;

namespace Qnx::Internal {

// QnxToolchainConfigWidget

class QnxToolchainConfigWidget : public ToolchainConfigWidget
{
public:
    QnxToolchainConfigWidget(const ToolchainBundle &bundle);

private:
    void applyImpl() override;
    void makeReadOnlyImpl() override { }

    void handleSdpPathChange();

    PathChooser *m_sdpPath;
    ProjectExplorer::AbiWidget *m_abiWidget;
};

static Abis detectTargetAbis(const FilePath &sdpPath)
{
    Abis result;
    FilePath qnxTarget;

    if (!sdpPath.fileName().isEmpty()) {
        const EnvironmentItems environment = QnxUtils::qnxEnvironment(sdpPath);
        for (const EnvironmentItem &item : environment) {
            if (item.name == QLatin1String("QNX_TARGET"))
                qnxTarget = sdpPath.withNewPath(item.value);
        }
    }

    if (qnxTarget.isEmpty())
        return result;

    const QList<QnxTarget> targets = QnxUtils::findTargets(qnxTarget);
    for (const auto &target : targets) {
        if (!result.contains(target.m_abi))
            result.append(target.m_abi);
    }

    return Utils::sorted(std::move(result),
              [](const Abi &arg1, const Abi &arg2) { return arg1.toString() < arg2.toString(); });
}

static void setQnxEnvironment(Environment &env, const EnvironmentItems &qnxEnv)
{
    // We only need to set QNX_HOST, QNX_TARGET, and QNX_CONFIGURATION_EXCLUSIVE
    // needed when running qcc
    for (const EnvironmentItem &item : qnxEnv) {
        if (item.name == QLatin1String("QNX_HOST") ||
            item.name == QLatin1String("QNX_TARGET") ||
            item.name == QLatin1String("QNX_CONFIGURATION_EXCLUSIVE"))
            env.set(item.name, item.value);
    }
}

// Qcc is a multi-compiler driver, and most of the gcc options can be accomplished by using the -Wp, and -Wc
// options to pass the options directly down to the compiler
static QStringList reinterpretOptions(const QStringList &args)
{
    QStringList arguments;
    for (const QString &str : args) {
        if (str.startsWith(QLatin1String("--sysroot=")))
            continue;
        QString arg = str;
        if (arg == QLatin1String("-v")
            || arg == QLatin1String("-dM"))
                arg.prepend(QLatin1String("-Wp,"));
        arguments << arg;
    }
    return arguments;
}

QnxToolchain::QnxToolchain()
    : GccToolchain(Constants::QNX_TOOLCHAIN_ID)
{
    setOptionsReinterpreter(&reinterpretOptions);
    setTypeDisplayName(Tr::tr("QCC"));

    sdpPath.setSettingsKey("Qnx.QnxToolChain.NDKPath");
    connect(&sdpPath, &BaseAspect::changed, this, &QnxToolchain::toolChainUpdated);

    cpuDir.setSettingsKey("Qnx.QnxToolChain.CpuDir");
    connect(&cpuDir, &BaseAspect::changed, this, &QnxToolchain::toolChainUpdated);

    connect(this, &AspectContainer::fromMapFinished, this, [this] {
        // Make the ABIs QNX specific (if they aren't already).
        setSupportedAbis(QnxUtils::convertAbis(supportedAbis()));
        setTargetAbi(QnxUtils::convertAbi(targetAbi()));
    });
}

void QnxToolchain::addToEnvironment(Environment &env) const
{
    if (env.expandedValueForKey("QNX_HOST").isEmpty() ||
        env.expandedValueForKey("QNX_TARGET").isEmpty() ||
        env.expandedValueForKey("QNX_CONFIGURATION_EXCLUSIVE").isEmpty())
        setQnxEnvironment(env, QnxUtils::qnxEnvironment(sdpPath()));

    GccToolchain::addToEnvironment(env);
}

QStringList QnxToolchain::suggestedMkspecList() const
{
    return {
        "qnx-armle-v7-qcc",
        "qnx-x86-qcc",
        "qnx-aarch64le-qcc",
        "qnx-x86-64-qcc"
    };
}

GccToolchain::DetectedAbisResult QnxToolchain::detectSupportedAbis() const
{
    static const QHash<QString, Abi> qnxTargets {
        {"arm-qnx-gnu",
         Abi(Abi::ArmArchitecture, Abi::QnxOS, Abi::GenericFlavor, Abi::ElfFormat, 32)},
        {"i686-qnx-gnu",
         Abi(Abi::X86Architecture, Abi::QnxOS, Abi::GenericFlavor, Abi::ElfFormat, 32)},
        {"x86_64-qnx-gnu",
         Abi(Abi::X86Architecture, Abi::QnxOS, Abi::GenericFlavor, Abi::ElfFormat, 64)},
        {"aarch64-qnx-gnu",
         Abi(Abi::ArmArchitecture, Abi::QnxOS, Abi::GenericFlavor, Abi::ElfFormat, 64)}
    };

    for (auto itr = qnxTargets.constBegin(); itr != qnxTargets.constEnd(); ++itr) {
        if (itr.value() == targetAbi())
            return GccToolchain::DetectedAbisResult({targetAbi()}, itr.key());
    }
    return GccToolchain::DetectedAbisResult({targetAbi()}, "");
}

bool QnxToolchain::operator ==(const Toolchain &other) const
{
    if (!GccToolchain::operator ==(other))
        return false;

    auto qnxTc = static_cast<const QnxToolchain *>(&other);

    return sdpPath() == qnxTc->sdpPath() && cpuDir() == qnxTc->cpuDir();
}

//---------------------------------------------------------------------------------
// QnxToolChainConfigWidget
//---------------------------------------------------------------------------------

QnxToolchainConfigWidget::QnxToolchainConfigWidget(const ToolchainBundle &bundle)
    : ToolchainConfigWidget(bundle)
    , m_sdpPath(new PathChooser)
    , m_abiWidget(new AbiWidget)
{
    m_sdpPath->setExpectedKind(PathChooser::ExistingDirectory);
    m_sdpPath->setHistoryCompleter("Qnx.Sdp.History");
    m_sdpPath->setFilePath(bundle.get<QnxToolchain>(&QnxToolchain::sdpPath)());
    m_sdpPath->setEnabled(!bundle.detectionSource().isAutoDetected());

    const Abis abiList = detectTargetAbis(m_sdpPath->filePath());
    m_abiWidget->setAbis(abiList, bundle.targetAbi());
    m_abiWidget->setEnabled(!bundle.detectionSource().isAutoDetected() && !abiList.isEmpty());

    //: SDP refers to 'Software Development Platform'.
    m_mainLayout->addRow(Tr::tr("SDP path:"), m_sdpPath);
    m_mainLayout->addRow(Tr::tr("&ABI:"), m_abiWidget);

    connect(m_sdpPath, &PathChooser::rawPathChanged,
            this, &QnxToolchainConfigWidget::handleSdpPathChange);
    connect(m_abiWidget, &AbiWidget::abiChanged, this, &ToolchainConfigWidget::dirty);
}

void QnxToolchainConfigWidget::applyImpl()
{
    if (bundle().detectionSource().isAutoDetected())
        return;

    bundle().setTargetAbi(m_abiWidget->currentAbi());
    bundle().forEach<QnxToolchain>([this](QnxToolchain &tc) {
        tc.sdpPath.setValue(m_sdpPath->filePath());
        tc.resetToolchain(compilerCommand(tc.language()));
    });
}

void QnxToolchainConfigWidget::handleSdpPathChange()
{
    const Abi currentAbi = m_abiWidget->currentAbi();
    const bool customAbi = m_abiWidget->isCustomAbi();
    const Abis abiList = detectTargetAbis(m_sdpPath->filePath());

    m_abiWidget->setEnabled(!abiList.isEmpty());

    // Find a good ABI for the new compiler:
    Abi newAbi;
    if (customAbi || abiList.contains(currentAbi))
        newAbi = currentAbi;

    m_abiWidget->setAbis(abiList, newAbi);
    emit dirty();
}

// Resolve the SDP environment file on a device. Prefer the value of the
// QnxSdpEnvFileToolAspect, but fall back to probing the well-known SDP install
// locations on the device (newer SDP first): that aspect is populated
// asynchronously and may not be set yet while device tool detection runs.
static FilePath qnxSdpEnvFile(const IDevice::ConstPtr &device)
{
    const FilePath fromAspect = device->deviceToolPath(Constants::QNX_SDPENVFILE_TOOL_ID);
    if (!fromAspect.isEmpty())
        return fromAspect;

    for (const char *candidate : {"/opt/qnx800/qnxsdp-env.sh", "/opt/qnx710/qnxsdp-env.sh",
                                  "/opt/qnx800/qnxsdp-env.bat", "/opt/qnx710/qnxsdp-env.bat"}) {
        const FilePath f = device->rootPath().withNewPath(QLatin1String(candidate));
        if (f.exists())
            return f;
    }
    return {};
}

// Detect QNX toolchains from an SDP environment file (e.g. the one carried by a
// build device via QnxSdpEnvFileToolAspect). Creates one QnxToolchain per target
// ABI and language, skipping any that are already known.
static Toolchains autoDetectFromEnvFile(const FilePath &envFile,
                                        const Toolchains &alreadyKnown)
{
    if (envFile.isEmpty() || !envFile.exists())
        return {};

    FilePath qnxHost;
    FilePath qnxTarget;
    const EnvironmentItems qnxEnv = QnxUtils::qnxEnvironmentFromEnvFile(envFile);
    for (const EnvironmentItem &item : qnxEnv) {
        if (item.name == "QNX_HOST")
            qnxHost = envFile.withNewPath(item.value).canonicalPath();
        else if (item.name == "QNX_TARGET")
            qnxTarget = envFile.withNewPath(item.value).canonicalPath();
    }

    const FilePath qcc = qnxHost.pathAppended("usr/bin/qcc").withExecutableSuffix();
    if (!qcc.exists())
        return {};

    const FilePath sdpPath = envFile.parentDir();

    Toolchains result;
    const QList<QnxTarget> targets = QnxUtils::findTargets(qnxTarget);
    for (const QnxTarget &target : targets) {
        for (const Id language : {Id(ProjectExplorer::Constants::C_LANGUAGE_ID),
                                  Id(ProjectExplorer::Constants::CXX_LANGUAGE_ID)}) {
            const bool known = Utils::anyOf(alreadyKnown, [&](Toolchain *tc) {
                return tc->typeId() == Constants::QNX_TOOLCHAIN_ID
                       && tc->language() == language
                       && tc->targetAbi() == target.m_abi
                       && tc->compilerCommand() == qcc;
            });
            if (known)
                continue;

            auto tc = new QnxToolchain;
            tc->setLanguage(language);
            tc->setTargetAbi(target.m_abi);
            tc->setDisplayName(Tr::tr("QCC for %1").arg(target.shortDescription()));
            tc->sdpPath.setValue(sdpPath);
            tc->cpuDir.setValue(target.cpuDir());
            tc->resetToolchain(qcc);
            result.append(tc);
        }
    }
    return result;
}

// QnxToolchainFactory

class QnxToolchainFactory : public ToolchainFactory
{
public:
    QnxToolchainFactory()
    {
        setDisplayName(Tr::tr("QCC"));
        setSupportedToolchainType(Constants::QNX_TOOLCHAIN_ID);
        setSupportedLanguages({ProjectExplorer::Constants::C_LANGUAGE_ID,
                               ProjectExplorer::Constants::CXX_LANGUAGE_ID});
        setToolchainConstructor([] { return new QnxToolchain; });
        setUserCreatable(true);
    }

    Toolchains autoDetect(const ToolchainDetector &detector) const final
    {
        // QNX toolchains are detected from the SDP environment file carried by
        // the (build) device via QnxSdpEnvFileToolAspect. Without a device there
        // is no SDP to look at.
        if (!detector.device)
            return {};
        return autoDetectFromEnvFile(qnxSdpEnvFile(detector.device), detector.alreadyKnown);
    }

    std::unique_ptr<ProjectExplorer::ToolchainConfigWidget> createConfigurationWidget(
        const ToolchainBundle &bundle) const override
    {
        return std::make_unique<QnxToolchainConfigWidget>(bundle);
    }
};

void setupQnxToolchain()
{
    static QnxToolchainFactory theQnxToolChainFactory;
}

// Register the QNX debuggers (nto*-gdb) shipped in the SDP referenced by a
// device's QnxSdpEnvFileToolAspect, so device-driven kit creation can pick a
// matching debugger for the QNX kits.
static void detectQnxDebuggers(const IDevice::ConstPtr &device, const ToolDetectionLogger &logger)
{
    const FilePath envFile = qnxSdpEnvFile(device);
    if (envFile.isEmpty() || !envFile.exists())
        return;

    FilePath qnxHost;
    // The nto gdbs need QNX_HOST/QNX_TARGET/QNX_CONFIGURATION set to run and to
    // report the target ABIs; collect them from the sourced SDP environment.
    EnvironmentItems qnxVars;
    const EnvironmentItems qnxEnv = QnxUtils::qnxEnvironmentFromEnvFile(envFile);
    for (const EnvironmentItem &item : qnxEnv) {
        if (item.name == "QNX_HOST")
            qnxHost = envFile.withNewPath(item.value).canonicalPath();
        if (item.name == "QNX_HOST" || item.name == "QNX_TARGET"
            || item.name == "QNX_CONFIGURATION" || item.name == "QNX_CONFIGURATION_EXCLUSIVE")
            qnxVars.append(item);
    }
    if (qnxHost.isEmpty())
        return;

    const FilePath binDir = qnxHost.pathAppended("usr/bin");
    QString pattern = "nto*-gdb";
    if (binDir.osType() == OsTypeWindows)
        pattern += ".exe";

    Environment sysEnv = qnxHost.deviceEnvironment();
    sysEnv.modify(qnxVars);
    const FilePaths gdbs = binDir.dirEntries({{pattern}, DirFilterFlag::Files});
    for (const FilePath &gdb : gdbs) {
        const bool known = Utils::anyOf(Debugger::DebuggerItemManager::debuggers(),
                                        [&gdb](const Debugger::DebuggerItem &di) {
                                            return di.command() == gdb;
                                        });
        if (known)
            continue;

        Debugger::DebuggerItem item;
        item.createId();
        item.setCommand(gdb);
        item.reinitializeFromFile(nullptr, &sysEnv);
        item.setUnexpandedDisplayName(Tr::tr("QNX Debugger (%1)").arg(gdb.fileName()));
        item.setDetectionSource({DetectionSource::Manual, gdb.toUrlishString()});
        Debugger::DebuggerItemManager::registerDebugger(item);
        if (logger)
            logger.logItem(Tr::tr("Found QNX debugger: %1").arg(gdb.toUserOutput()));
    }
}

void setupQnxDebuggerDetection(QObject *guard)
{
    QObject::connect(DeviceManager::instance(), &DeviceManager::toolDetectionRequested, guard,
                     [](Id devId, const FilePaths &, quint64 token,
                        const ToolDetectionLogger &logger) {
        const IDevice::Ptr device = DeviceManager::find(devId);
        QTC_ASSERT(device, return);
        device->registerToolDetectionTask(token);
        if (logger)
            logger.logTopLevel(Tr::tr("Searching for QNX debuggers..."));
        detectQnxDebuggers(device, logger);
        device->deregisterToolDetectionTask(token);
    });
}

} // Qnx::Internal
