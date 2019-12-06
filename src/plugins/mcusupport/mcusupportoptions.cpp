/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: BlackBerry (qt@blackberry.com)
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

#include "mcusupportconstants.h"
#include "mcusupportoptions.h"

#include <coreplugin/icore.h>
#include <cmakeprojectmanager/cmakekitinformation.h>
#include <debugger/debuggeritem.h>
#include <debugger/debuggeritemmanager.h>
#include <debugger/debuggerkitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>
#include <QVariant>

namespace McuSupport {
namespace Internal {

McuPackage::McuPackage(const QString &label, const QString &defaultPath,
                       const QString &detectionPath, const QString &settingsKey)
    : m_label(label)
    , m_defaultPath(defaultPath)
    , m_detectionPath(detectionPath)
    , m_settingsKey(settingsKey)
{
    QSettings *s = Core::ICore::settings();
    s->beginGroup(Constants::SETTINGS_GROUP);
    m_path = s->value(QLatin1String(Constants::SETTINGS_KEY_PACKAGE_PREFIX) + m_settingsKey,
                      m_defaultPath).toString();
    s->endGroup();
}

QString McuPackage::path() const
{
    return QFileInfo(m_fileChooser->path() + m_relativePathModifier).absoluteFilePath();
}

QString McuPackage::label() const
{
    return m_label;
}

QString McuPackage::detectionPath() const
{
    return m_detectionPath;
}

QWidget *McuPackage::widget()
{
    if (m_widget)
        return m_widget;

    m_widget = new QWidget;
    m_fileChooser = new Utils::PathChooser;

    auto layout = new QGridLayout(m_widget);
    layout->setContentsMargins(0, 0, 0, 0);
    m_statusIcon = new QLabel;
    m_statusIcon->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::MinimumExpanding);
    m_statusIcon->setAlignment(Qt::AlignTop);
    m_statusLabel = new QLabel;
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    if (!m_downloadUrl.isEmpty()) {
        auto downLoadButton = new QToolButton;
        downLoadButton->setIcon(Utils::Icons::DOWNLOAD.icon());
        downLoadButton->setToolTip(tr("Download from \"%1\"").arg(m_downloadUrl));
        QObject::connect(downLoadButton, &QToolButton::pressed, [this]{
            QDesktopServices::openUrl(m_downloadUrl);
        });
        layout->addWidget(downLoadButton, 0, 2);
    }

    layout->addWidget(m_fileChooser, 0, 0, 1, 2);
    layout->addWidget(m_statusIcon, 1, 0);
    layout->addWidget(m_statusLabel, 1, 1, 1, -1);

    m_fileChooser->setPath(m_path);

    QObject::connect(m_fileChooser, &Utils::PathChooser::pathChanged,
                     [this](){
        updateStatus();
        emit changed();
    });

    updateStatus();
    return m_widget;
}

McuPackage::Status McuPackage::status() const
{
    return m_status;
}

void McuPackage::setDownloadUrl(const QString &url)
{
    m_downloadUrl = url;
}

void McuPackage::setEnvironmentVariableName(const QString &name)
{
    m_environmentVariableName = name;
}

QString McuPackage::environmentVariableName() const
{
    return m_environmentVariableName;
}

void McuPackage::setAddToPath(bool addToPath)
{
    m_addToPath = addToPath;
}

bool McuPackage::addToPath() const
{
    return m_addToPath;
}

void McuPackage::writeToSettings() const
{
    if (m_path.compare(m_defaultPath) == 0)
        return;
    QSettings *s = Core::ICore::settings();
    s->beginGroup(Constants::SETTINGS_GROUP);
    s->setValue(QLatin1String(Constants::SETTINGS_KEY_PACKAGE_PREFIX) + m_settingsKey, m_path);
    s->endGroup();
}

void McuPackage::setRelativePathModifier(const QString &path)
{
    m_relativePathModifier = path;
}

void McuPackage::updateStatus()
{
    m_path = m_fileChooser->rawPath();
    const bool validPath = m_fileChooser->isValid();
    const Utils::FilePath detectionPath = Utils::FilePath::fromString(
                m_fileChooser->path() + "/" + m_detectionPath);
    const QString displayDetectionPath = Utils::FilePath::fromString(m_detectionPath).toUserOutput();
    const bool validPackage = detectionPath.exists();

    m_status = validPath ? (validPackage ? ValidPackage : ValidPathInvalidPackage) : InvalidPath;

    static const QPixmap okIcon = Utils::Icons::OK.pixmap();
    static const QPixmap notOkIcon = Utils::Icons::BROKEN.pixmap();
    m_statusIcon->setPixmap(m_status == ValidPackage ? okIcon : notOkIcon);

    QString statusText;
    switch (m_status) {
    case ValidPackage:
        statusText = tr("Path is valid, \"%1\" was found.").arg(displayDetectionPath);
        break;
    case ValidPathInvalidPackage:
        statusText = tr("Path exists, but does not contain \"%1\".").arg(displayDetectionPath);
        break;
    case InvalidPath:
        statusText = tr("Path does not exist.");
        break;
    }
    m_statusLabel->setText(statusText);
}

McuTarget::McuTarget(const QString &vendor, const QString &model,
                     const QVector<McuPackage*> &packages)
    : m_vendor(vendor)
    , m_model(model)
    , m_packages(packages)
{
}

QString McuTarget::vendor() const
{
    return m_vendor;
}

QString McuTarget::model() const
{
    return m_model;
}

QVector<McuPackage *> McuTarget::packages() const
{
    return m_packages;
}

void McuTarget::setToolChainFile(const QString &toolChainFile)
{
    m_toolChainFile = toolChainFile;
}

QString McuTarget::toolChainFile() const
{
    return m_toolChainFile;
}

void McuTarget::setQulPlatform(const QString &qulPlatform)
{
    m_qulPlatform = qulPlatform;
}

QString McuTarget::qulPlatform() const
{
    return m_qulPlatform;
}

bool McuTarget::isValid() const
{
    return !Utils::anyOf(packages(), [](McuPackage *package) {
        return package->status() != McuPackage::ValidPackage;
    });
}

int McuTarget::colorDepth() const
{
    return m_colorDepth;
}

void McuTarget::setColorDepth(int colorDepth)
{
    m_colorDepth = colorDepth;
}

static QString findInProgramFiles(const QString &folder)
{
    for (auto envVar : {"ProgramFiles", "ProgramFiles(x86)", "ProgramW6432"}) {
        if (!qEnvironmentVariableIsSet(envVar))
            continue;
        const Utils::FilePath dir =
                Utils::FilePath::fromUserInput(qEnvironmentVariable(envVar) + "/" + folder);
        if (dir.exists())
            return dir.toString();
    }
    return {};
}

static McuPackage *createQtForMCUsPackage()
{
    auto result = new McuPackage(
                McuPackage::tr("Qt for MCUs SDK"),
                QDir::homePath(),
                Utils::HostOsInfo::withExecutableSuffix("bin/qmltocpp"),
                "QtForMCUsSdk");
    result->setEnvironmentVariableName("Qul_DIR");
    return result;
}

static McuPackage *createArmGccPackage()
{
    const char envVar[] = "ARMGCC_DIR";

    QString defaultPath;
    if (qEnvironmentVariableIsSet(envVar))
        defaultPath = qEnvironmentVariable(envVar);
    if (defaultPath.isEmpty() && Utils::HostOsInfo::isWindowsHost()) {
        const QDir installDir(findInProgramFiles("/GNU Tools ARM Embedded/"));
        if (installDir.exists()) {
            // If GNU Tools installation dir has only one sub dir,
            // select the sub dir, otherwise the installation dir.
            const QFileInfoList subDirs =
                    installDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
            if (subDirs.count() == 1)
                defaultPath = subDirs.first().filePath() + '/';
        }
    }
    if (defaultPath.isEmpty())
        defaultPath = QDir::homePath();

    auto result = new McuPackage(
                McuPackage::tr("GNU Arm Embedded Toolchain"),
                defaultPath,
                Utils::HostOsInfo::withExecutableSuffix("bin/arm-none-eabi-g++"),
                "GNUArmEmbeddedToolchain");
    result->setDownloadUrl(
                "https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads");
    result->setEnvironmentVariableName(envVar);
    return result;
}

static McuPackage *createStm32CubeFwF7SdkPackage()
{
    auto result = new McuPackage(
                McuPackage::tr("STM32Cube SDK"),
                "%{Env:STM32Cube_FW_F7_SDK_PATH}",
                "Drivers/STM32F7xx_HAL_Driver",
                "Stm32CubeFwF7Sdk");
    result->setDownloadUrl(
                "https://www.st.com/content/st_com/en/products/embedded-software/mcus-embedded-software/stm32-embedded-software/stm32cube-mcu-packages/stm32cubef7.html");
    result->setEnvironmentVariableName("STM32Cube_FW_F7_SDK_PATH");
    return result;
}

static McuPackage *createStm32CubeProgrammerPackage()
{

    QString defaultPath = QDir::homePath();
    if (Utils::HostOsInfo::isWindowsHost()) {
        const QString programPath =
                findInProgramFiles("/STMicroelectronics/STM32Cube/STM32CubeProgrammer/");
        if (!programPath.isEmpty())
            defaultPath = programPath;
    }
    auto result = new McuPackage(
                McuPackage::tr("STM32CubeProgrammer"),
                defaultPath,
                QLatin1String(Utils::HostOsInfo::isWindowsHost() ? "/bin/STM32_Programmer_CLI.exe"
                                                                 : "/bin/STM32_Programmer.sh"),
                "Stm32CubeProgrammer");
    result->setRelativePathModifier("/bin");
    result->setDownloadUrl(
                "https://www.st.com/en/development-tools/stm32cubeprog.html");
    result->setAddToPath(true);
    return result;
}

static McuPackage *createEvkbImxrt1050SdkPackage()
{
    auto result = new McuPackage(
                McuPackage::tr("NXP i.MXRT SDK"),
                "%{Env:EVKB_IMXRT1050_SDK_PATH}", // TODO: Try to not use 1050 specifics
                "EVKB-IMXRT1050_manifest_v3_5.xml",
                "EvkbImxrt1050Sdk");
    result->setDownloadUrl("https://mcuxpresso.nxp.com/en/welcome");
    return result;
}

static McuPackage *createSeggerJLinkPackage()
{
    QString defaultPath = QString("%{Env:SEGGER_JLINK_SOFTWARE_AND_DOCUMENTATION_PATH}");
    if (Utils::HostOsInfo::isWindowsHost()) {
        const QString programPath = findInProgramFiles("/SEGGER/JLink");
        if (!programPath.isEmpty())
            defaultPath = programPath;
    }
    auto result = new McuPackage(
                McuPackage::tr("SEGGER JLink"),
                defaultPath,
                Utils::HostOsInfo::withExecutableSuffix("JLink"),
                "SeggerJLink");
    result->setDownloadUrl("https://www.segger.com/downloads/jlink");
    result->setEnvironmentVariableName("SEGGER_JLINK_SOFTWARE_AND_DOCUMENTATION_PATH");
    return result;
}

McuSupportOptions::McuSupportOptions(QObject *parent)
    : QObject(parent)
{
    qtForMCUsSdkPackage = createQtForMCUsPackage();
    armGccPackage = createArmGccPackage();
    McuPackage* stm32CubeFwF7SdkPackage = createStm32CubeFwF7SdkPackage();
    McuPackage* stm32CubeProgrammerPackage = createStm32CubeProgrammerPackage();
    McuPackage* evkbImxrt1050SdkPackage = createEvkbImxrt1050SdkPackage();
    McuPackage* seggerJLinkPackage = createSeggerJLinkPackage();

    auto stmEvalPackages = {
        armGccPackage, stm32CubeProgrammerPackage, qtForMCUsSdkPackage};
    auto stmEngPackages = {
        armGccPackage, stm32CubeFwF7SdkPackage, stm32CubeProgrammerPackage, qtForMCUsSdkPackage};
    auto nxpEvalPackages = {
        armGccPackage, seggerJLinkPackage, qtForMCUsSdkPackage};
    auto nxpEngPackages = {
        armGccPackage, evkbImxrt1050SdkPackage, seggerJLinkPackage, qtForMCUsSdkPackage};
    auto desktopPackages = {
        qtForMCUsSdkPackage};
    packages = {
        armGccPackage, stm32CubeFwF7SdkPackage, stm32CubeProgrammerPackage, evkbImxrt1050SdkPackage,
        seggerJLinkPackage, qtForMCUsSdkPackage};

    const QString vendorStm = "STM";
    const QString vendorNxp = "NXP";
    const QString vendorQt = "Qt";

    // STM
    auto mcuTarget = new McuTarget(vendorStm, "stm32f7508", stmEvalPackages);
    mcuTarget->setToolChainFile("CMake/stm32f7508-discovery.cmake");
    mcuTarget->setColorDepth(32);
    mcuTargets.append(mcuTarget);

    mcuTarget = new McuTarget(vendorStm, "stm32f7508", stmEvalPackages);
    mcuTarget->setToolChainFile("CMake/stm32f7508-discovery.cmake");
    mcuTarget->setColorDepth(16);
    mcuTargets.append(mcuTarget);

    mcuTarget = new McuTarget(vendorStm, "stm32f769i", stmEvalPackages);
    mcuTarget->setToolChainFile("CMake/stm32f769i-discovery.cmake");
    mcuTargets.append(mcuTarget);

    mcuTarget = new McuTarget(vendorStm, "Engineering", stmEngPackages);
    mcuTargets.append(mcuTarget);

    // NXP
    mcuTarget = new McuTarget(vendorNxp, "evkbimxrt1050", nxpEvalPackages);
    mcuTarget->setToolChainFile("CMake/evkbimxrt1050-toolchain.cmake");
    mcuTargets.append(mcuTarget);

    mcuTarget = new McuTarget(vendorNxp, "Engineering",  nxpEngPackages);
    mcuTargets.append(mcuTarget);

    // Desktop
    mcuTarget = new McuTarget(vendorQt, "Desktop", desktopPackages);
    mcuTarget->setQulPlatform("Qt");
    mcuTarget->setColorDepth(32);
    mcuTargets.append(mcuTarget);

    for (auto package : packages)
        connect(package, &McuPackage::changed, [this](){
            emit changed();
        });
}

McuSupportOptions::~McuSupportOptions()
{
    qDeleteAll(packages);
    packages.clear();
    qDeleteAll(mcuTargets);
    mcuTargets.clear();
}

static ProjectExplorer::ToolChain* armGccToolchain(const Utils::FilePath &path, Core::Id language)
{
    using namespace ProjectExplorer;

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

static bool mcuTargetIsDesktop(const McuTarget* mcuTarget)
{
    return mcuTarget->qulPlatform() == "Qt";
}

static Utils::FilePath jomExecutablePath()
{
    return Utils::HostOsInfo::isWindowsHost() ?
                Utils::FilePath::fromUserInput(Core::ICore::libexecPath() + "/jom.exe")
              : Utils::FilePath();
}

static void setKitProperties(const QString &kitName, ProjectExplorer::Kit *k,
                             const McuTarget* mcuTarget)
{
    using namespace ProjectExplorer;

    k->setUnexpandedDisplayName(kitName);
    k->setValue(Constants::KIT_MCUTARGET_VENDOR_KEY, mcuTarget->vendor());
    k->setValue(Constants::KIT_MCUTARGET_MODEL_KEY, mcuTarget->model());
    k->setAutoDetected(true);
    k->makeSticky();
    if (mcuTargetIsDesktop(mcuTarget)) {
        k->setDeviceTypeForIcon(Constants::DEVICE_TYPE);
    } else {
        QSet<Core::Id> irrelevant = {
            SysRootKitAspect::id(),
            "QtSupport.QtInformation" // QtKitAspect::id()
        };
        if (jomExecutablePath().exists()) // TODO: add id() getter to CMakeGeneratorKitAspect
            irrelevant.insert("CMake.GeneratorKitInformation");
        k->setIrrelevantAspects(irrelevant);
    }
}

static void setKitToolchains(ProjectExplorer::Kit *k, const QString &armGccPath)
{
    using namespace ProjectExplorer;

    const QString compileNameScheme = Utils::HostOsInfo::withExecutableSuffix(
                armGccPath + "/bin/arm-none-eabi-%1");
    ToolChain *cTc = armGccToolchain(
                Utils::FilePath::fromUserInput(compileNameScheme.arg("gcc")),
                ProjectExplorer::Constants::C_LANGUAGE_ID);
    ToolChain *cxxTc = armGccToolchain(
                Utils::FilePath::fromUserInput(compileNameScheme.arg("g++")),
                ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    ToolChainKitAspect::setToolChain(k, cTc);
    ToolChainKitAspect::setToolChain(k, cxxTc);
}

static void setKitDebugger(ProjectExplorer::Kit *k, const QString &armGccPath)
{
    using namespace Debugger;

    const Utils::FilePath command = Utils::FilePath::fromUserInput(
                Utils::HostOsInfo::withExecutableSuffix(armGccPath + "/bin/arm-none-eabi-gdb-py"));
    const DebuggerItem *debugger = DebuggerItemManager::findByCommand(command);
    QVariant debuggerId;
    if (!debugger) {
        DebuggerItem newDebugger;
        newDebugger.setCommand(command);
        newDebugger.setUnexpandedDisplayName(
                    McuPackage::tr("Arm GDB at %1").arg(command.toUserOutput()));
        debuggerId = DebuggerItemManager::registerDebugger(newDebugger);
    } else {
        debuggerId = debugger->id();
    }

    DebuggerKitAspect::setDebugger(k, debuggerId);
}

static void setKitDevice(ProjectExplorer::Kit *k)
{
    using namespace ProjectExplorer;

    DeviceTypeKitAspect::setDeviceTypeId(k, Constants::DEVICE_TYPE);
}

static void setKitEnvironment(ProjectExplorer::Kit *k, const McuTarget* mcuTarget)
{
    using namespace ProjectExplorer;

    Utils::EnvironmentItems changes;
    QStringList pathAdditions;
    for (auto package : mcuTarget->packages()) {
        if (package->addToPath())
            pathAdditions.append(QDir::toNativeSeparators(package->path()));
        if (!package->environmentVariableName().isEmpty())
            changes.append({package->environmentVariableName(),
                            QDir::toNativeSeparators(package->path())});
    }
    pathAdditions.append("${Path}");
    pathAdditions.append(QDir::toNativeSeparators(Core::ICore::libexecPath() + "/clang/bin"));
    const QString path = QLatin1String(Utils::HostOsInfo().isWindowsHost() ? "Path" : "PATH");
    changes.append({path, pathAdditions.join(Utils::HostOsInfo::pathListSeparator())});
    EnvironmentKitAspect::setEnvironmentChanges(k, changes);
}

static void setKitCMakeOptions(ProjectExplorer::Kit *k, const McuTarget* mcuTarget,
                               const QString &qulDir)
{
    using namespace CMakeProjectManager;

    CMakeConfig config = CMakeConfigurationKitAspect::configuration(k);
    config.append(CMakeConfigItem("CMAKE_CXX_COMPILER", "%{Compiler:Executable:Cxx}"));
    config.append(CMakeConfigItem("CMAKE_C_COMPILER", "%{Compiler:Executable:C}"));
    if (!mcuTarget->toolChainFile().isEmpty())
        config.append(CMakeConfigItem("CMAKE_TOOLCHAIN_FILE",
                                      (qulDir + "/" + mcuTarget->toolChainFile()).toUtf8()));
    if (!mcuTarget->qulPlatform().isEmpty())
        config.append(CMakeConfigItem("QUL_PLATFORM",
                                      mcuTarget->qulPlatform().toUtf8()));
    if (mcuTargetIsDesktop(mcuTarget))
        config.append(CMakeConfigItem("CMAKE_PREFIX_PATH", "%{Qt:QT_INSTALL_PREFIX}"));
    if (mcuTarget->colorDepth() >= 0)
        config.append(CMakeConfigItem("QUL_COLOR_DEPTH",
                                      QString::number(mcuTarget->colorDepth()).toLatin1()));
    const Utils::FilePath jom = jomExecutablePath();
    if (jom.exists()) {
        config.append(CMakeConfigItem("CMAKE_MAKE_PROGRAM", jom.toString().toLatin1()));
        CMakeGeneratorKitAspect::setGenerator(k, "NMake Makefiles JOM");
    }
    CMakeConfigurationKitAspect::setConfiguration(k, config);
}

QString McuSupportOptions::kitName(const McuTarget *mcuTarget) const
{
    // TODO: get version from qulSdkPackage and insert into name
    const QString colorDepth = mcuTarget->colorDepth() > 0
            ? QString::fromLatin1(" %1bpp").arg(mcuTarget->colorDepth())
            : "";
    return QString::fromLatin1("Qt for MCUs - %1 %2%3")
            .arg(mcuTarget->vendor(), mcuTarget->model(), colorDepth);
}

QList<ProjectExplorer::Kit *> McuSupportOptions::existingKits(const McuTarget *mcuTargt)
{
    using namespace ProjectExplorer;
    const QString mcuTargetKitName = kitName(mcuTargt);
    return Utils::filtered(KitManager::kits(), [&mcuTargetKitName](Kit *kit) {
            return kit->isAutoDetected() && kit->unexpandedDisplayName() == mcuTargetKitName;
    });
}

ProjectExplorer::Kit *McuSupportOptions::newKit(const McuTarget *mcuTarget)
{
    using namespace ProjectExplorer;

    const QString armGccPath = armGccPackage->path();
    const QString qulDir = qtForMCUsSdkPackage->path();
    const auto init = [this, mcuTarget](Kit *k) {
        KitGuard kitGuard(k);

        setKitProperties(kitName(mcuTarget), k, mcuTarget);
        if (!mcuTargetIsDesktop(mcuTarget)) {
            setKitToolchains(k, armGccPackage->path());
            setKitDebugger(k, armGccPackage->path());
            setKitDevice(k);
        }
        setKitEnvironment(k, mcuTarget);
        setKitCMakeOptions(k, mcuTarget, qtForMCUsSdkPackage->path());

        k->setup();
        k->fix();
    };

    return KitManager::registerKit(init);
}

} // Internal
} // McuSupport
