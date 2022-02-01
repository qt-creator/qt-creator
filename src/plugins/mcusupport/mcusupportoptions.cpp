/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "mcupackage.h"
#include "mcusupportconstants.h"
#include "mcusupportoptions.h"
#include "mcusupportsdk.h"
#include "mcusupportplugin.h"
#include "mcusupportcmakemapper.h"

#include <baremetal/baremetalconstants.h>
#include <cmakeprojectmanager/cmaketoolmanager.h>
#include <cmakeprojectmanager/cmakekitinformation.h>
#include <coreplugin/icore.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/messagemanager.h>
#include <cmakeprojectmanager/cmakekitinformation.h>
#include <debugger/debuggeritem.h>
#include <debugger/debuggeritemmanager.h>
#include <debugger/debuggerkitinformation.h>
#include <projectexplorer/abi.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionmanager.h>
#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/infolabel.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>
#include <QVariant>

using namespace ProjectExplorer;
using namespace Utils;

namespace McuSupport {
namespace Internal {

static const int KIT_VERSION = 9; // Bumps up whenever details in Kit creation change

static bool kitNeedsQtVersion()
{
    // Only on Windows, Qt is linked into the distributed qul Desktop libs. Also, the host tools
    // are missing the Qt runtime libraries on non-Windows.
    return !HostOsInfo::isWindowsHost();
}

static void remapQul2xCmakeVars(Kit *kit, const EnvironmentItems &envItems)
{
    using namespace CMakeProjectManager;
    const auto cmakeVars = mapEnvVarsToQul2xCmakeVars(envItems);
    const auto cmakeVarNames = Utils::transform(cmakeVars, &CMakeConfigItem::key);

    // First filter out all Qul2.x CMake vars
    auto config = Utils::filtered(CMakeConfigurationKitAspect::configuration(kit),
                                  [&](const auto &configItem) {
        return !cmakeVarNames.contains(configItem.key);
    });
    // Then append them with new values
    config.append(cmakeVars);
    CMakeConfigurationKitAspect::setConfiguration(kit, config);
}


static ToolChain *msvcToolChain(Id language)
{
    ToolChain *toolChain = ToolChainManager::toolChain([language](const ToolChain *t) {
        const Abi abi = t->targetAbi();
        // TODO: Should Abi::WindowsMsvc2022Flavor be added too?
        return  (abi.osFlavor() == Abi::WindowsMsvc2017Flavor || abi.osFlavor() == Abi::WindowsMsvc2019Flavor)
                && abi.architecture() == Abi::X86Architecture
                && abi.wordWidth() == 64
                && t->language() == language;
    });
    return toolChain;
}

static ToolChain *gccToolChain(Id language)
{
    ToolChain *toolChain = ToolChainManager::toolChain([language](const ToolChain *t) {
        const Abi abi = t->targetAbi();
        return  abi.os() != Abi::WindowsOS
                && abi.architecture() == Abi::X86Architecture
                && abi.wordWidth() == 64
                && t->language() == language;
    });
    return toolChain;
}

static ToolChain *armGccToolChain(const FilePath &path, Id language)
{
    ToolChain *toolChain = ToolChainManager::toolChain([&path, language](const ToolChain *t){
        return t->compilerCommand() == path && t->language() == language;
    });
    if (!toolChain) {
        ToolChainFactory *gccFactory =
            Utils::findOrDefault(ToolChainFactory::allToolChainFactories(), [](ToolChainFactory *f){
            return f->supportedToolChainType() == ProjectExplorer::Constants::GCC_TOOLCHAIN_TYPEID;
        });
        if (gccFactory) {
            const QList<ToolChain*> detected = gccFactory->detectForImport({path, language});
            if (!detected.isEmpty()) {
                toolChain = detected.first();
                toolChain->setDetection(ToolChain::ManualDetection);
                toolChain->setDisplayName("Arm GCC");
                ToolChainManager::registerToolChain(toolChain);
            }
        }
    }

    return toolChain;
}

static ToolChain *iarToolChain(const FilePath &path, Id language)
{
    ToolChain *toolChain = ToolChainManager::toolChain([language](const ToolChain *t){
        return t->typeId() == BareMetal::Constants::IAREW_TOOLCHAIN_TYPEID
               && t->language() == language;
    });
    if (!toolChain) {
        ToolChainFactory *iarFactory =
            Utils::findOrDefault(ToolChainFactory::allToolChainFactories(), [](ToolChainFactory *f){
            return f->supportedToolChainType() == BareMetal::Constants::IAREW_TOOLCHAIN_TYPEID;
        });
        if (iarFactory) {
            Toolchains detected = iarFactory->autoDetect(ToolchainDetector({}, {}));
            if (detected.isEmpty())
                detected = iarFactory->detectForImport({path, language});
            for (auto tc: detected) {
                if (tc->language() == language) {
                    toolChain = tc;
                    toolChain->setDetection(ToolChain::ManualDetection);
                    toolChain->setDisplayName("IAREW");
                    ToolChainManager::registerToolChain(toolChain);
                }
            }
        }
    }

    return toolChain;
}

ToolChain *McuToolChainPackage::toolChain(Id language) const
{
    ToolChain *tc = nullptr;
    if (m_type == TypeMSVC)
        tc = msvcToolChain(language);
    else if (m_type == TypeGCC)
        tc = gccToolChain(language);
    else if (m_type == TypeIAR) {
        const FilePath compiler = path().pathAppended("/bin/iccarm").withExecutableSuffix();
        tc = iarToolChain(compiler, language);
    }
    else {
        const QLatin1String compilerName(
                    language == ProjectExplorer::Constants::C_LANGUAGE_ID ? "gcc" : "g++");
        const QString comp = QLatin1String(m_type == TypeArmGcc ? "/bin/arm-none-eabi-%1" : "/bar/foo-keil-%1")
                    .arg(compilerName);
        const FilePath compiler = path().pathAppended(comp).withExecutableSuffix();

        tc = armGccToolChain(compiler, language);
    }
    return tc;
}

QString McuToolChainPackage::toolChainName() const
{
    switch (m_type) {
    case TypeArmGcc: return QLatin1String("armgcc");
    case TypeIAR: return QLatin1String("iar");
    case TypeKEIL: return QLatin1String("keil");
    case TypeGHS: return QLatin1String("ghs");
    case TypeGHSArm: return QLatin1String("ghs-arm");
    default: return QLatin1String("unsupported");
    }
}

QString McuToolChainPackage::cmakeToolChainFileName() const
{
    return toolChainName() + QLatin1String(".cmake");
}

QVariant McuToolChainPackage::debuggerId() const
{
    using namespace Debugger;

    QString sub, displayName;
    DebuggerEngineType engineType;

    switch (m_type) {
    case TypeArmGcc: {
        sub = QString::fromLatin1("bin/arm-none-eabi-gdb-py");
        displayName = McuPackage::tr("Arm GDB at %1");
        engineType = Debugger::GdbEngineType;
        break; }
    case TypeIAR: {
        sub = QString::fromLatin1("../common/bin/CSpyBat");
        displayName = QLatin1String("CSpy");
        engineType = Debugger::NoEngineType; // support for IAR missing
        break; }
    case TypeKEIL: {
        sub = QString::fromLatin1("UV4/UV4");
        displayName = QLatin1String("KEIL uVision Debugger");
        engineType = Debugger::UvscEngineType;
        break; }
    default: return QVariant();
    }

    const FilePath command = path().pathAppended(sub).withExecutableSuffix();
    const DebuggerItem *debugger = DebuggerItemManager::findByCommand(command);
    QVariant debuggerId;
    if (!debugger) {
        DebuggerItem newDebugger;
        newDebugger.setCommand(command);
        newDebugger.setUnexpandedDisplayName(displayName.arg(command.toUserOutput()));
        newDebugger.setEngineType(engineType);
        debuggerId = DebuggerItemManager::registerDebugger(newDebugger);
    } else {
        debuggerId = debugger->id();
    }
    return debuggerId;
}

McuTarget::McuTarget(const QVersionNumber &qulVersion,
                     const Platform &platform, OS os,
                     const QVector<McuPackage *> &packages,
                     const McuToolChainPackage *toolChainPackage)
    : m_qulVersion(qulVersion)
    , m_platform(platform)
    , m_os(os)
    , m_packages(packages)
    , m_toolChainPackage(toolChainPackage)
{
}

const QVector<McuPackage *> &McuTarget::packages() const
{
    return m_packages;
}

const McuToolChainPackage *McuTarget::toolChainPackage() const
{
    return m_toolChainPackage;
}

McuTarget::OS McuTarget::os() const
{
    return m_os;
}

const McuTarget::Platform &McuTarget::platform() const
{
    return m_platform;
}

bool McuTarget::isValid() const
{
    return Utils::allOf(packages(), [](McuPackage *package) {
        package->updateStatus();
        return package->validStatus();
    });
}

void McuTarget::printPackageProblems() const
{
    for (auto package: packages()) {
        package->updateStatus();
        if (!package->validStatus())
            printMessage(tr("Error creating kit for target %1, package %2: %3").arg(
                             McuSupportOptions::kitName(this),
                             package->label(),
                             package->statusText()),
                         true);
        if (package->status() == McuPackage::ValidPackageMismatchedVersion)
            printMessage(tr("Warning creating kit for target %1, package %2: %3").arg(
                             McuSupportOptions::kitName(this),
                             package->label(),
                             package->statusText()),
                         false);
    }
}

const QVersionNumber &McuTarget::qulVersion() const
{
    return m_qulVersion;
}

int McuTarget::colorDepth() const
{
    return m_colorDepth;
}

void McuTarget::setColorDepth(int colorDepth)
{
    m_colorDepth = colorDepth;
}

void McuSdkRepository::deletePackagesAndTargets()
{
    qDeleteAll(packages);
    packages.clear();
    qDeleteAll(mcuTargets);
    mcuTargets.clear();
}

McuSupportOptions::McuSupportOptions(QObject *parent)
    : QObject(parent)
    , qtForMCUsSdkPackage(Sdk::createQtForMCUsPackage())
{
    connect(qtForMCUsSdkPackage, &McuPackage::changed,
            this, &McuSupportOptions::populatePackagesAndTargets);
}

McuSupportOptions::~McuSupportOptions()
{
    deletePackagesAndTargets();
    delete qtForMCUsSdkPackage;
}

void McuSupportOptions::populatePackagesAndTargets()
{
    setQulDir(qtForMCUsSdkPackage->path());
}

static FilePath qulDocsDir()
{
    const FilePath qulDir = McuSupportOptions::qulDirFromSettings();
    if (qulDir.isEmpty() || !qulDir.exists())
        return {};
    const FilePath docsDir = qulDir.pathAppended("docs");
    return docsDir.exists() ? docsDir : FilePath();
}

void McuSupportOptions::registerQchFiles()
{
    const QString docsDir = qulDocsDir().toString();
    if (docsDir.isEmpty())
        return;

    const QFileInfoList qchFiles = QDir(docsDir, "*.qch").entryInfoList();
    Core::HelpManager::registerDocumentation(
                Utils::transform<QStringList>(qchFiles, [](const QFileInfo &fi){
        return fi.absoluteFilePath();
    }));
}

void McuSupportOptions::registerExamples()
{
    const FilePath docsDir = qulDocsDir();
    if (docsDir.isEmpty())
        return;

    auto examples = {
        std::make_pair(QStringLiteral("demos"), tr("Qt for MCUs Demos")),
        std::make_pair(QStringLiteral("examples"), tr("Qt for MCUs Examples"))
    };
    for (const auto &dir : examples) {
        const FilePath examplesDir =
                McuSupportOptions::qulDirFromSettings().pathAppended(dir.first);
        if (!examplesDir.exists())
            continue;

        QtSupport::QtVersionManager::registerExampleSet(dir.second, docsDir.toString(),
                                                        examplesDir.toString());
    }
}

const QVersionNumber &McuSupportOptions::minimalQulVersion()
{
    static const QVersionNumber v({1, 3});
    return v;
}

void McuSupportOptions::setQulDir(const FilePath &dir)
{
    deletePackagesAndTargets();
    qtForMCUsSdkPackage->updateStatus();
    if (qtForMCUsSdkPackage->validStatus())
        Sdk::targetsAndPackages(dir, &sdkRepository);
    for (const auto &package : qAsConst(sdkRepository.packages))
        connect(package, &McuPackage::changed, this, &McuSupportOptions::changed);

    emit changed();
}

FilePath McuSupportOptions::qulDirFromSettings()
{
    return Sdk::packagePathFromSettings(Constants::SETTINGS_KEY_PACKAGE_QT_FOR_MCUS_SDK,
                                        QSettings::UserScope);
}

static void setKitProperties(const QString &kitName, Kit *k, const McuTarget *mcuTarget,
                             const FilePath &sdkPath)
{
    using namespace Constants;

    k->setUnexpandedDisplayName(kitName);
    k->setValue(KIT_MCUTARGET_VENDOR_KEY, mcuTarget->platform().vendor);
    k->setValue(KIT_MCUTARGET_MODEL_KEY, mcuTarget->platform().name);
    k->setValue(KIT_MCUTARGET_COLORDEPTH_KEY, mcuTarget->colorDepth());
    k->setValue(KIT_MCUTARGET_SDKVERSION_KEY, mcuTarget->qulVersion().toString());
    k->setValue(KIT_MCUTARGET_KITVERSION_KEY, KIT_VERSION);
    k->setValue(KIT_MCUTARGET_OS_KEY, static_cast<int>(mcuTarget->os()));
    k->setValue(KIT_MCUTARGET_TOOCHAIN_KEY, mcuTarget->toolChainPackage()->toolChainName());
    k->setAutoDetected(false);
    k->makeSticky();
    if (mcuTarget->toolChainPackage()->isDesktopToolchain())
        k->setDeviceTypeForIcon(DEVICE_TYPE);
    k->setValue(QtSupport::SuppliesQtQuickImportPath::id(), true);
    k->setValue(QtSupport::KitQmlImportPath::id(), sdkPath.pathAppended("include/qul").toVariant());
    k->setValue(QtSupport::KitHasMergedHeaderPathsWithQmlImportPaths::id(), true);
    QSet<Id> irrelevant = {
        SysRootKitAspect::id(),
        QtSupport::SuppliesQtQuickImportPath::id(),
        QtSupport::KitQmlImportPath::id(),
        QtSupport::KitHasMergedHeaderPathsWithQmlImportPaths::id(),
    };
    if (!kitNeedsQtVersion())
        irrelevant.insert(QtSupport::QtKitAspect::id());
    k->setIrrelevantAspects(irrelevant);
}

static void setKitToolchains(Kit *k, const McuToolChainPackage *tcPackage)
{
    // No Green Hills toolchain, because support for it is missing.
    if (tcPackage->type() == McuToolChainPackage::TypeUnsupported
        || tcPackage->type() == McuToolChainPackage::TypeGHS
        || tcPackage->type() == McuToolChainPackage::TypeGHSArm)
        return;

    ToolChainKitAspect::setToolChain(k, tcPackage->toolChain(
                                     ProjectExplorer::Constants::C_LANGUAGE_ID));
    ToolChainKitAspect::setToolChain(k, tcPackage->toolChain(
                                     ProjectExplorer::Constants::CXX_LANGUAGE_ID));
}

static void setKitDebugger(Kit *k, const McuToolChainPackage *tcPackage)
{
    // Qt Creator seems to be smart enough to deduce the right Kit debugger from the ToolChain
    // We rely on that at least in the Desktop case.
    if (tcPackage->isDesktopToolchain()
            // No Green Hills and IAR debugger, because support for it is missing.
            || tcPackage->type() == McuToolChainPackage::TypeUnsupported
            || tcPackage->type() == McuToolChainPackage::TypeGHS
            || tcPackage->type() == McuToolChainPackage::TypeGHSArm
            || tcPackage->type() == McuToolChainPackage::TypeIAR)
        return;

    const QVariant debuggerId = tcPackage->debuggerId();
    if (debuggerId.isValid())
        Debugger::DebuggerKitAspect::setDebugger(k, debuggerId);
}

static void setKitDevice(Kit *k, const McuTarget* mcuTarget)
{
    // "Device Type" Desktop is the default. We use that for the Qt for MCUs Desktop Kit
    if (mcuTarget->toolChainPackage()->isDesktopToolchain())
        return;

    DeviceTypeKitAspect::setDeviceTypeId(k, Constants::DEVICE_TYPE);
}

static bool expectsCmakeVars(const McuTarget *mcuTarget)
{
    return mcuTarget->qulVersion() >= QVersionNumber{2,0};
}

static void setKitEnvironment(Kit *k, const McuTarget *mcuTarget,
                              const McuPackage *qtForMCUsSdkPackage)
{
    EnvironmentItems changes;
    QStringList pathAdditions;

    // The Desktop version depends on the Qt shared libs in Qul_DIR/bin.
    // If CMake's fileApi is avaialble, we can rely on the "Add library search path to PATH"
    // feature of the run configuration. Otherwise, we just prepend the path, here.
    if (mcuTarget->toolChainPackage()->isDesktopToolchain()
            && !CMakeProjectManager::CMakeToolManager::defaultCMakeTool()->hasFileApi())
        pathAdditions.append(qtForMCUsSdkPackage->path().pathAppended("bin").toUserOutput());

    auto processPackage = [&pathAdditions, &changes](const McuPackage *package) {
        if (package->addToPath())
            pathAdditions.append(package->path().toUserOutput());
        if (!package->environmentVariableName().isEmpty())
            changes.append({package->environmentVariableName(), package->path().toUserOutput()});
    };
    for (auto package : mcuTarget->packages())
        processPackage(package);
    processPackage(qtForMCUsSdkPackage);

    // Clang not needed in version 1.7+
    if (mcuTarget->qulVersion() < QVersionNumber{1,7}) {
        const QString path = QLatin1String(HostOsInfo::isWindowsHost() ? "Path" : "PATH");
        pathAdditions.append("${" + path + "}");
        pathAdditions.append(Core::ICore::libexecPath("clang/bin").toUserOutput());
        changes.append({path, pathAdditions.join(HostOsInfo::pathListSeparator())});
    }

    if (kitNeedsQtVersion())
        changes.append({QLatin1String("LD_LIBRARY_PATH"), "%{Qt:QT_INSTALL_LIBS}"});

    // Hack, this problem should be solved in lower layer
    if (expectsCmakeVars(mcuTarget)) {
        remapQul2xCmakeVars(k, changes);
    }

    EnvironmentKitAspect::setEnvironmentChanges(k, changes);
}

static void setKitDependencies(Kit *k, const McuTarget *mcuTarget,
                              const McuPackage *qtForMCUsSdkPackage)
{
    NameValueItems dependencies;

    auto processPackage = [&dependencies](const McuPackage *package) {
        if (!package->environmentVariableName().isEmpty())
            dependencies.append({package->environmentVariableName(),
                            QDir::toNativeSeparators(package->detectionPath())});
    };
    for (auto package : mcuTarget->packages())
        processPackage(package);
    processPackage(qtForMCUsSdkPackage);

    McuDependenciesKitAspect::setDependencies(k, dependencies);

    auto irrelevant = k->irrelevantAspects();
    irrelevant.insert(McuDependenciesKitAspect::id());
    k->setIrrelevantAspects(irrelevant);
}

static void updateKitEnvironment(Kit *k, const McuTarget *mcuTarget)
{
    EnvironmentItems changes = EnvironmentKitAspect::environmentChanges(k);
    for (auto package : mcuTarget->packages()) {
        const QString varName = package->environmentVariableName();
        if (!varName.isEmpty() && package->validStatus()) {
            const int index = Utils::indexOf(changes, [varName](const EnvironmentItem &item) {
                return item.name == varName;
            });
            const EnvironmentItem item = {package->environmentVariableName(),
                                          package->path().toUserOutput()};
            if (index != -1)
                changes.replace(index, item);
            else
                changes.append(item);
        }
    }

    // Hack, this problem should be solved in lower layer
    if (expectsCmakeVars(mcuTarget)) {
        remapQul2xCmakeVars(k, changes);
    }

    EnvironmentKitAspect::setEnvironmentChanges(k, changes);
}

static void setKitCMakeOptions(Kit *k, const McuTarget* mcuTarget, const FilePath &qulDir)
{
    using namespace CMakeProjectManager;

    CMakeConfig config = CMakeConfigurationKitAspect::configuration(k);
    // CMake ToolChain file for ghs handles CMAKE_*_COMPILER autonomously
    if (mcuTarget->toolChainPackage()->type() != McuToolChainPackage::TypeGHS &&
            mcuTarget->toolChainPackage()->type() != McuToolChainPackage::TypeGHSArm) {
        config.append(CMakeConfigItem("CMAKE_CXX_COMPILER", "%{Compiler:Executable:Cxx}"));
        config.append(CMakeConfigItem("CMAKE_C_COMPILER", "%{Compiler:Executable:C}"));
    }

    if (!mcuTarget->toolChainPackage()->isDesktopToolchain()) {
        const FilePath cMakeToolchainFile = qulDir.pathAppended("lib/cmake/Qul/toolchain/"
                           + mcuTarget->toolChainPackage()->cmakeToolChainFileName());

        config.append(CMakeConfigItem(
                          "CMAKE_TOOLCHAIN_FILE",
                          cMakeToolchainFile.toString().toUtf8()));
        if (!cMakeToolchainFile.exists()) {
            printMessage(McuTarget::tr("Warning for target %1: missing CMake toolchain file expected at %2.")
                  .arg(McuSupportOptions::kitName(mcuTarget), cMakeToolchainFile.toUserOutput()), false);
        }
    }

    const FilePath generatorsPath = qulDir.pathAppended("/lib/cmake/Qul/QulGenerators.cmake");
    config.append(CMakeConfigItem("QUL_GENERATORS",
                                  generatorsPath.toString().toUtf8()));
    if (!generatorsPath.exists()) {
        printMessage(McuTarget::tr("Warning for target %1: missing QulGenerators expected at %2.")
              .arg(McuSupportOptions::kitName(mcuTarget), generatorsPath.toUserOutput()), false);
    }

    config.append(CMakeConfigItem("QUL_PLATFORM",
                                  mcuTarget->platform().name.toUtf8()));

    if (mcuTarget->qulVersion() <= QVersionNumber{1,3} // OS variable was removed in Qul 1.4
        && mcuTarget->os() == McuTarget::OS::FreeRTOS)
        config.append(CMakeConfigItem("OS", "FreeRTOS"));
    if (mcuTarget->colorDepth() >= 0)
        config.append(CMakeConfigItem("QUL_COLOR_DEPTH",
                                      QString::number(mcuTarget->colorDepth()).toLatin1()));
    if (kitNeedsQtVersion())
        config.append(CMakeConfigItem("CMAKE_PREFIX_PATH", "%{Qt:QT_INSTALL_PREFIX}"));
    CMakeConfigurationKitAspect::setConfiguration(k, config);

    if (HostOsInfo::isWindowsHost()) {
        auto type = mcuTarget->toolChainPackage()->type();
        if (type == McuToolChainPackage::TypeGHS || type == McuToolChainPackage::TypeGHSArm) {
            // See https://bugreports.qt.io/browse/UL-4247?focusedCommentId=565802&page=com.atlassian.jira.plugin.system.issuetabpanels:comment-tabpanel#comment-565802
            // and https://bugreports.qt.io/browse/UL-4247?focusedCommentId=565803&page=com.atlassian.jira.plugin.system.issuetabpanels:comment-tabpanel#comment-565803
            CMakeGeneratorKitAspect::setGenerator(k, "NMake Makefiles JOM");
        }
    }
}

static void setKitQtVersionOptions(Kit *k)
{
    if (!kitNeedsQtVersion())
        QtSupport::QtKitAspect::setQtVersion(k, nullptr);
    // else: auto-select a Qt version
}

QString McuSupportOptions::kitName(const McuTarget *mcuTarget)
{
    QString os;
    if (mcuTarget->qulVersion() <= QVersionNumber{1,3} && mcuTarget->os() == McuTarget::OS::FreeRTOS)
        // Starting from Qul 1.4 each OS is a separate platform
        os = QLatin1String(" FreeRTOS");

    const McuToolChainPackage *tcPkg = mcuTarget->toolChainPackage();
    const QString compilerName = tcPkg && !tcPkg->isDesktopToolchain()
            ? QString::fromLatin1(" (%1)").arg(tcPkg->toolChainName().toUpper())
            : "";
    const QString colorDepth = mcuTarget->colorDepth() > 0
            ? QString::fromLatin1(" %1bpp").arg(mcuTarget->colorDepth())
            : "";
    const QString targetName = mcuTarget->platform().displayName.isEmpty()
            ? mcuTarget->platform().name
            : mcuTarget->platform().displayName;
    return QString::fromLatin1("Qt for MCUs %1.%2 - %3%4%5%6")
            .arg(QString::number(mcuTarget->qulVersion().majorVersion()),
                 QString::number(mcuTarget->qulVersion().minorVersion()),
                 targetName,
                 os,
                 colorDepth,
                 compilerName);
}

QList<Kit *> McuSupportOptions::existingKits(const McuTarget *mcuTarget)
{
    using namespace Constants;
    return Utils::filtered(KitManager::kits(), [mcuTarget](Kit *kit) {
        return kit->value(KIT_MCUTARGET_KITVERSION_KEY) == KIT_VERSION
                && (!mcuTarget || (
                        kit->value(KIT_MCUTARGET_VENDOR_KEY) == mcuTarget->platform().vendor
                        && kit->value(KIT_MCUTARGET_MODEL_KEY) == mcuTarget->platform().name
                        && kit->value(KIT_MCUTARGET_COLORDEPTH_KEY) == mcuTarget->colorDepth()
                        && kit->value(KIT_MCUTARGET_OS_KEY).toInt()
                           == static_cast<int>(mcuTarget->os())
                        && kit->value(KIT_MCUTARGET_TOOCHAIN_KEY)
                           == mcuTarget->toolChainPackage()->toolChainName()
                        ));
    });
}

QList<Kit *> McuSupportOptions::matchingKits(const McuTarget *mcuTarget, const McuPackage *qtForMCUsSdkPackage)
{
    return Utils::filtered(existingKits(mcuTarget), [mcuTarget, qtForMCUsSdkPackage](Kit *kit) {
        return kitUpToDate(kit, mcuTarget, qtForMCUsSdkPackage);
    });
}

QList<Kit *> McuSupportOptions::upgradeableKits(const McuTarget *mcuTarget, const McuPackage *qtForMCUsSdkPackage)
{
    return Utils::filtered(existingKits(mcuTarget), [mcuTarget, qtForMCUsSdkPackage](Kit *kit) {
        return !kitUpToDate(kit, mcuTarget, qtForMCUsSdkPackage);
    });
}

QList<Kit *> McuSupportOptions::kitsWithMismatchedDependencies(const McuTarget *mcuTarget)
{
    return Utils::filtered(existingKits(mcuTarget), [mcuTarget](Kit *kit) {
        const auto environment = Utils::NameValueDictionary(
                    Utils::NameValueItem::toStringList(
                        EnvironmentKitAspect::environmentChanges(kit)));
        return Utils::anyOf(mcuTarget->packages(), [&environment](const McuPackage *package) {
            return !package->environmentVariableName().isEmpty() &&
                    environment.value(package->environmentVariableName()) != package->path().toUserOutput();
        });
    });
}

QList<Kit *> McuSupportOptions::outdatedKits()
{
    return Utils::filtered(KitManager::kits(), [](Kit *kit) {
        return !kit->value(Constants::KIT_MCUTARGET_VENDOR_KEY).isNull()
                && kit->value(Constants::KIT_MCUTARGET_KITVERSION_KEY) != KIT_VERSION;
    });
}

void McuSupportOptions::removeOutdatedKits()
{
    for (auto kit : McuSupportOptions::outdatedKits())
        KitManager::deregisterKit(kit);
}

Kit *McuSupportOptions::newKit(const McuTarget *mcuTarget, const McuPackage *qtForMCUsSdk)
{
    const auto init = [mcuTarget, qtForMCUsSdk](Kit *k) {
        KitGuard kitGuard(k);

        setKitProperties(kitName(mcuTarget), k, mcuTarget, qtForMCUsSdk->path());
        setKitDevice(k, mcuTarget);
        setKitToolchains(k, mcuTarget->toolChainPackage());
        setKitDebugger(k, mcuTarget->toolChainPackage());
        setKitEnvironment(k, mcuTarget, qtForMCUsSdk);
        setKitDependencies(k, mcuTarget, qtForMCUsSdk);
        setKitCMakeOptions(k, mcuTarget, qtForMCUsSdk->path());
        setKitQtVersionOptions(k);

        k->setup();
        k->fix();
    };

    return KitManager::registerKit(init);
}

void printMessage(const QString &message, bool important)
{
    const QString displayMessage = QCoreApplication::translate("QtForMCUs", "Qt for MCUs: %1").arg(message);
    if (important)
        Core::MessageManager::writeFlashing(displayMessage);
    else
        Core::MessageManager::writeSilently(displayMessage);
}

QVersionNumber McuSupportOptions::kitQulVersion(const Kit *kit)
{
    return QVersionNumber::fromString(
                kit->value(McuSupport::Constants::KIT_MCUTARGET_SDKVERSION_KEY)
                .toString());
}

static FilePath kitDependencyPath(const Kit *kit, const QString &variableName)
{
    for (const NameValueItem &nameValueItem : EnvironmentKitAspect::environmentChanges(kit)) {
        if (nameValueItem.name == variableName)
            return FilePath::fromUserInput(nameValueItem.value);
    }
    return FilePath();
}

bool McuSupportOptions::kitUpToDate(const Kit *kit, const McuTarget *mcuTarget,
                                    const McuPackage *qtForMCUsSdkPackage)
{
    return kitQulVersion(kit) == mcuTarget->qulVersion() &&
            kitDependencyPath(kit, qtForMCUsSdkPackage->environmentVariableName()).toUserOutput() == qtForMCUsSdkPackage->path().toUserOutput();
}

void McuSupportOptions::deletePackagesAndTargets()
{
    sdkRepository.deletePackagesAndTargets();
}

McuSupportOptions::UpgradeOption McuSupportOptions::askForKitUpgrades()
{
    QMessageBox upgradePopup(Core::ICore::dialogParent());
    upgradePopup.setStandardButtons(QMessageBox::Cancel);
    QPushButton *replaceButton = upgradePopup.addButton(tr("Replace Existing Kits"),QMessageBox::NoRole);
    QPushButton *keepButton = upgradePopup.addButton(tr("Create New Kits"),QMessageBox::NoRole);
    upgradePopup.setWindowTitle(tr("Qt for MCUs"));
    upgradePopup.setText(tr("New version of Qt for MCUs detected. Upgrade existing kits?"));

    upgradePopup.exec();

    if (upgradePopup.clickedButton() == keepButton)
        return Keep;

    if (upgradePopup.clickedButton() == replaceButton)
        return Replace;

    return Ignore;
}

void McuSupportOptions::createAutomaticKits()
{
    auto qtForMCUsPackage = Sdk::createQtForMCUsPackage();

    const auto createKits = [qtForMCUsPackage]() {
    if (qtForMCUsPackage->automaticKitCreationEnabled()) {
        qtForMCUsPackage->updateStatus();
        if (!qtForMCUsPackage->validStatus()) {
            switch (qtForMCUsPackage->status()) {
                case McuPackage::ValidPathInvalidPackage: {
                    const QString displayPath = FilePath::fromString(qtForMCUsPackage->detectionPath())
                        .toUserOutput();
                    printMessage(tr("Path %1 exists, but does not contain %2.")
                        .arg(qtForMCUsPackage->path().toUserOutput(), displayPath),
                                 true);
                    break;
                }
                case McuPackage::InvalidPath: {
                    printMessage(tr("Path %1 does not exist. Add the path in Tools > Options > Devices > MCU.")
                        .arg(qtForMCUsPackage->path().toUserOutput()),
                                 true);
                    break;
                }
                case McuPackage::EmptyPath: {
                    printMessage(tr("Missing %1. Add the path in Tools > Options > Devices > MCU.")
                        .arg(qtForMCUsPackage->detectionPath()),
                                 true);
                    return;
                }
                default: break;
            }
            return;
        }

        if (CMakeProjectManager::CMakeToolManager::cmakeTools().isEmpty()) {
            printMessage(tr("No CMake tool was detected. Add a CMake tool in Tools > Options > Kits > CMake."),
                         true);
            return;
        }

        FilePath dir = qtForMCUsPackage->path();
        McuSdkRepository repo;
        Sdk::targetsAndPackages(dir, &repo);

        bool needsUpgrade = false;
        for (const auto &target: qAsConst(repo.mcuTargets)) {
            // if kit already exists, skip
            if (!matchingKits(target, qtForMCUsPackage).empty())
                continue;
            if (!upgradeableKits(target, qtForMCUsPackage).empty()) {
                // if kit exists but wrong version/path
                needsUpgrade = true;
            } else {
                // if no kits for this target, create
                if (target->isValid())
                    newKit(target, qtForMCUsPackage);
                target->printPackageProblems();
            }
        }

        repo.deletePackagesAndTargets();

        if (needsUpgrade)
            McuSupportPlugin::askUserAboutMcuSupportKitsUpgrade();
    }
   };

    createKits();
    delete qtForMCUsPackage;
}

void McuSupportOptions::checkUpgradeableKits()
{
    if (!qtForMCUsSdkPackage->validStatus() || sdkRepository.mcuTargets.length() == 0)
        return;

    if (Utils::anyOf(sdkRepository.mcuTargets, [this](const McuTarget *target) {
                     return !upgradeableKits(target, this->qtForMCUsSdkPackage).empty() &&
                     matchingKits(target, this->qtForMCUsSdkPackage).empty();
        }))
        upgradeKits(askForKitUpgrades());
}

void McuSupportOptions::upgradeKits(UpgradeOption upgradeOption)
{
    if (upgradeOption == Ignore)
        return;

    auto qtForMCUsPackage = Sdk::createQtForMCUsPackage();

    auto dir = qtForMCUsPackage->path();
    McuSdkRepository repo;
    Sdk::targetsAndPackages(dir, &repo);

    for (const auto &target: qAsConst(repo.mcuTargets)) {
        if (!matchingKits(target, qtForMCUsPackage).empty())
            // already up-to-date
            continue;

        const auto kits = upgradeableKits(target, qtForMCUsPackage);
        if (!kits.empty()) {
            if (upgradeOption == Replace)
                for (auto existingKit : kits)
                    KitManager::deregisterKit(existingKit);

            if (target->isValid())
                newKit(target, qtForMCUsPackage);
            target->printPackageProblems();
        }
    }

    repo.deletePackagesAndTargets();
    delete qtForMCUsPackage;
}

void McuSupportOptions::upgradeKitInPlace(ProjectExplorer::Kit *kit, const McuTarget *mcuTarget, const McuPackage *qtForMCUsSdk)
{
    setKitProperties(kitName(mcuTarget), kit, mcuTarget, qtForMCUsSdk->path());
    setKitEnvironment(kit, mcuTarget, qtForMCUsSdk);
    setKitDependencies(kit, mcuTarget, qtForMCUsSdk);
}

void McuSupportOptions::fixKitsDependencies()
{
    auto qtForMCUsPackage = Sdk::createQtForMCUsPackage();

    FilePath dir = qtForMCUsPackage->path();
    McuSdkRepository repo;
    Sdk::targetsAndPackages(dir, &repo);
    for (const auto &target: qAsConst(repo.mcuTargets)) {
        if (target->isValid()) {
            for (auto kit : kitsWithMismatchedDependencies(target)) {
                updateKitEnvironment(kit, target);
            }
        }
    }

    repo.deletePackagesAndTargets();
    delete qtForMCUsPackage;
}

/**
 * @brief Fix/update existing kits if needed
 */
void McuSupportOptions::fixExistingKits()
{
    for (Kit *kit : KitManager::kits()) {
        if (!kit->hasValue(Constants::KIT_MCUTARGET_KITVERSION_KEY) )
            continue;

        if (kit->isAutoDetected()) {
            kit->setAutoDetected(false);
        }

        // Check if the MCU kits are flagged as supplying a QtQuick import path, in order
        // to tell the QMLJS code-model that it won't need to add a fall-back import
        // path.
        const auto bringsQtQuickImportPath = QtSupport::SuppliesQtQuickImportPath::id();
        auto irrelevantAspects = kit->irrelevantAspects();
        if (!irrelevantAspects.contains(bringsQtQuickImportPath)) {
            irrelevantAspects.insert(bringsQtQuickImportPath);
            kit->setIrrelevantAspects(irrelevantAspects);
        }
        if (!kit->hasValue(bringsQtQuickImportPath)) {
            kit->setValue(bringsQtQuickImportPath, true);
        }

        // Check if the MCU kit supplies its import path.
        const auto kitQmlImportPath = QtSupport::KitQmlImportPath::id();
        if (!irrelevantAspects.contains(kitQmlImportPath)) {
            irrelevantAspects.insert(kitQmlImportPath);
            kit->setIrrelevantAspects(irrelevantAspects);
        }
        if (!kit->hasValue(kitQmlImportPath)) {
            auto config = CMakeProjectManager::CMakeConfigurationKitAspect::configuration(kit);
            for (const auto &cfgItem : qAsConst(config)) {
                if (cfgItem.key == "QUL_GENERATORS") {
                    auto idx = cfgItem.value.indexOf("/lib/cmake/Qul");
                    auto qulDir = cfgItem.value.left(idx);
                    kit->setValue(kitQmlImportPath, QVariant(qulDir + "/include/qul"));
                    break;
                }
            }
        }

        // Check if the MCU kit has the flag for merged header/qml-import paths set.
        const auto mergedPaths = QtSupport::KitHasMergedHeaderPathsWithQmlImportPaths::id();
        if (!irrelevantAspects.contains(mergedPaths)) {
            irrelevantAspects.insert(mergedPaths);
            kit->setIrrelevantAspects(irrelevantAspects);
        }
        if (!kit->value(mergedPaths, false).toBool()) {
            kit->setValue(mergedPaths, true);
        }
    }

    // Fix kit dependencies for known targets
    auto qtForMCUsPackage = Sdk::createQtForMCUsPackage();
    qtForMCUsPackage->updateStatus();
    if (qtForMCUsPackage->validStatus()) {
        FilePath dir = qtForMCUsPackage->path();
        McuSdkRepository repo;
        Sdk::targetsAndPackages(dir, &repo);
        for (const auto &target: qAsConst(repo.mcuTargets))
            for (auto kit: existingKits(target)) {
                if (McuDependenciesKitAspect::dependencies(kit).isEmpty()) {
                    setKitDependencies(kit, target, qtForMCUsPackage);
                }
            }

        repo.deletePackagesAndTargets();
    }
    delete qtForMCUsPackage;
}

class McuDependenciesKitAspectWidget final : public KitAspectWidget
{
    Q_DECLARE_TR_FUNCTIONS(McuSupport::McuDependenciesKitAspect)

public:
    McuDependenciesKitAspectWidget(Kit *workingCopy, const KitAspect *ki)
        : KitAspectWidget(workingCopy, ki)
    {}

    void makeReadOnly() override {}
    void refresh() override {}
    void addToLayout(Utils::LayoutBuilder &) override {}
};

} // Internal

McuDependenciesKitAspect::McuDependenciesKitAspect()
{
    setObjectName(QLatin1String("McuDependenciesKitAspect"));
    setId(McuDependenciesKitAspect::id());
    setDisplayName(tr("MCU Dependencies"));
    setDescription(tr("Paths to 3rd party dependencies"));
    setPriority(28500);
}

Tasks McuDependenciesKitAspect::validate(const Kit *k) const
{
    Tasks result;
    QTC_ASSERT(k, return result);

    const QVariant checkFormat = k->value(McuDependenciesKitAspect::id());
    if (!checkFormat.isNull() && !checkFormat.canConvert(QVariant::List))
        return { BuildSystemTask(Task::Error, tr("The MCU dependencies setting value is invalid.")) };

    const QVariant envStringList = k->value(EnvironmentKitAspect::id());
    if (!envStringList.isNull() && !envStringList.canConvert(QVariant::List))
         return { BuildSystemTask(Task::Error, tr("The environment setting value is invalid.")) };

    const auto environment = Utils::NameValueDictionary(envStringList.toStringList());
    for (const auto &dependency: dependencies(k)) {
        if (!environment.hasKey(dependency.name)) {
            result << BuildSystemTask(Task::Warning, tr("Environment variable %1 not defined.").arg(dependency.name));
        } else {
            const auto path = Utils::FilePath::fromUserInput(
                        environment.value(dependency.name) + "/" + dependency.value);
            if (!path.exists()) {
                result << BuildSystemTask(Task::Warning, tr("%1 not found.").arg(path.toUserOutput()));
            }
        }
    }

    return result;
}

void McuDependenciesKitAspect::fix(Kit *k)
{
    QTC_ASSERT(k, return);

    const QVariant variant = k->value(McuDependenciesKitAspect::id());
    if (!variant.isNull() && !variant.canConvert(QVariant::List)) {
        qWarning("Kit \"%s\" has a wrong mcu dependencies value set.", qPrintable(k->displayName()));
        setDependencies(k, Utils::NameValueItems());
    }
}

KitAspectWidget *McuDependenciesKitAspect::createConfigWidget(Kit *k) const
{
    QTC_ASSERT(k, return nullptr);
    return new Internal::McuDependenciesKitAspectWidget(k, this);
}

KitAspect::ItemList McuDependenciesKitAspect::toUserOutput(const Kit *k) const
{
    Q_UNUSED(k);
    return {};
}

Utils::Id McuDependenciesKitAspect::id()
{
    return "PE.Profile.McuDependencies";
}


Utils::NameValueItems McuDependenciesKitAspect::dependencies(const Kit *k)
{
     if (k)
         return Utils::NameValueItem::fromStringList(k->value(McuDependenciesKitAspect::id()).toStringList());
     return Utils::NameValueItems();
}

void McuDependenciesKitAspect::setDependencies(Kit *k, const Utils::NameValueItems &dependencies)
{
    if (k)
        k->setValue(McuDependenciesKitAspect::id(), Utils::NameValueItem::toStringList(dependencies));
}

} // McuSupport
