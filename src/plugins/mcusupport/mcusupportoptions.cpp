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

#include "mcusupportconstants.h"
#include "mcusupportoptions.h"
#include "mcusupportsdk.h"

#include <cmakeprojectmanager/cmaketoolmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/helpmanager.h>
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
#include <QToolButton>
#include <QVBoxLayout>
#include <QVariant>

namespace McuSupport {
namespace Internal {

static const int KIT_VERSION = 6; // Bumps up whenever details in Kit creation change

static QString packagePathFromSettings(const QString &settingsKey,
                                       QSettings::Scope scope = QSettings::UserScope,
                                       const QString &defaultPath = {})
{
    QSettings *s = Core::ICore::settings(scope);
    s->beginGroup(Constants::SETTINGS_GROUP);
    const QString path = s->value(QLatin1String(Constants::SETTINGS_KEY_PACKAGE_PREFIX)
                                  + settingsKey, defaultPath).toString();
    s->endGroup();
    return Utils::FilePath::fromFileInfo(path).toString();
}

McuPackage::McuPackage(const QString &label, const QString &defaultPath,
                       const QString &detectionPath, const QString &settingsKey)
    : m_label(label)
    , m_defaultPath(packagePathFromSettings(settingsKey, QSettings::SystemScope, defaultPath))
    , m_detectionPath(detectionPath)
    , m_settingsKey(settingsKey)
{
    m_path = packagePathFromSettings(settingsKey, QSettings::UserScope, m_defaultPath);
}

QString McuPackage::path() const
{
    return QFileInfo(m_fileChooser->filePath().toString() + m_relativePathModifier).absoluteFilePath();
}

QString McuPackage::label() const
{
    return m_label;
}

QString McuPackage::defaultPath() const
{
    return m_defaultPath;
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
    m_fileChooser->lineEdit()->setButtonIcon(Utils::FancyLineEdit::Right,
                                             Utils::Icons::RESET.icon());
    m_fileChooser->lineEdit()->setButtonVisible(Utils::FancyLineEdit::Right, true);
    connect(m_fileChooser->lineEdit(), &Utils::FancyLineEdit::rightButtonClicked, [&](){
        m_fileChooser->setPath(m_defaultPath);
    });

    auto layout = new QGridLayout(m_widget);
    layout->setContentsMargins(0, 0, 0, 0);
    m_infoLabel = new Utils::InfoLabel();

    if (!m_downloadUrl.isEmpty()) {
        auto downLoadButton = new QToolButton;
        downLoadButton->setIcon(Utils::Icons::ONLINE.icon());
        downLoadButton->setToolTip(tr("Download from \"%1\"").arg(m_downloadUrl));
        QObject::connect(downLoadButton, &QToolButton::pressed, [this]{
            QDesktopServices::openUrl(m_downloadUrl);
        });
        layout->addWidget(downLoadButton, 0, 2);
    }

    layout->addWidget(m_fileChooser, 0, 0, 1, 2);
    layout->addWidget(m_infoLabel, 1, 0, 1, -1);

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
    const QString key = QLatin1String(Constants::SETTINGS_GROUP) + '/' +
            QLatin1String(Constants::SETTINGS_KEY_PACKAGE_PREFIX) + m_settingsKey;
    QSettings *uS = Core::ICore::settings();
    if (m_path == m_defaultPath)
        uS->remove(key);
    else
        uS->setValue(key, m_path);
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
                m_fileChooser->filePath().toString() + "/" + m_detectionPath);
    const QString displayDetectionPath = Utils::FilePath::fromString(m_detectionPath).toUserOutput();
    const bool validPackage = m_detectionPath.isEmpty() || detectionPath.exists();

    m_status = validPath ? (validPackage ? ValidPackage : ValidPathInvalidPackage) : InvalidPath;

    m_infoLabel->setType(m_status == ValidPackage ? Utils::InfoLabel::Ok
                                                  : Utils::InfoLabel::NotOk);

    QString statusText;
    switch (m_status) {
    case ValidPackage:
        statusText = m_detectionPath.isEmpty()
                ? tr("Path exists.")
                : tr("Path is valid, \"%1\" was found.").arg(displayDetectionPath);
        break;
    case ValidPathInvalidPackage:
        statusText = tr("Path exists, but does not contain \"%1\".").arg(displayDetectionPath);
        break;
    case InvalidPath:
        statusText = tr("Path does not exist.");
        break;
    }
    m_infoLabel->setText(statusText);
    m_fileChooser->lineEdit()->button(Utils::FancyLineEdit::Right)->setEnabled(
                m_path != m_defaultPath);
}

McuToolChainPackage::McuToolChainPackage(const QString &label, const QString &defaultPath,
                                         const QString &detectionPath, const QString &settingsKey,
                                         McuToolChainPackage::Type type)
    : McuPackage(label, defaultPath, detectionPath, settingsKey)
    , m_type(type)
{
}

McuToolChainPackage::Type McuToolChainPackage::type() const
{
    return m_type;
}

static ProjectExplorer::ToolChain *desktopToolChain(Utils::Id language)
{
    using namespace ProjectExplorer;

    ToolChain *toolChain = ToolChainManager::toolChain([language](const ToolChain *t) {
        const Abi abi = t->targetAbi();
        return (abi.os() != Abi::WindowsOS
                || (abi.osFlavor() == Abi::WindowsMsvc2017Flavor
                    || abi.osFlavor() == Abi::WindowsMsvc2019Flavor))
                && abi.architecture() == Abi::X86Architecture
                && abi.wordWidth() == 64
                && t->language() == language;
    });
    return toolChain;
}

static ProjectExplorer::ToolChain* armGccToolChain(const Utils::FilePath &path, Utils::Id language)
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

ProjectExplorer::ToolChain *McuToolChainPackage::toolChain(Utils::Id language) const
{
    ProjectExplorer::ToolChain *tc = nullptr;
    if (m_type == TypeDesktop) {
        tc = desktopToolChain(language);
    } else {
        const QLatin1String compilerName(
                    language == ProjectExplorer::Constants::C_LANGUAGE_ID ? "gcc" : "g++");
        const Utils::FilePath compiler = Utils::FilePath::fromUserInput(
                    Utils::HostOsInfo::withExecutableSuffix(
                        path() + (
                            m_type == TypeArmGcc
                            ? "/bin/arm-none-eabi-%1" : m_type == TypeIAR
                              ? "/foo/bar-iar-%1" : "/bar/foo-keil-%1")).arg(compilerName));

        tc = armGccToolChain(compiler, language);
    }
    return tc;
}

QString McuToolChainPackage::cmakeToolChainFileName() const
{
    return QLatin1String(m_type == TypeArmGcc
                         ? "armgcc" : m_type == McuToolChainPackage::TypeIAR
                           ? "iar" : m_type == McuToolChainPackage::TypeKEIL
                             ? "keil" : "ghs") + QLatin1String(".cmake");
}

QVariant McuToolChainPackage::debuggerId() const
{
    using namespace Debugger;

    const Utils::FilePath command = Utils::FilePath::fromUserInput(
                Utils::HostOsInfo::withExecutableSuffix(path() + (
                        m_type == TypeArmGcc
                        ? "/bin/arm-none-eabi-gdb-py" : m_type == TypeIAR
                          ? "/foo/bar-iar-gdb" : "/bar/foo-keil-gdb")));
    const DebuggerItem *debugger = DebuggerItemManager::findByCommand(command);
    QVariant debuggerId;
    if (!debugger) {
        DebuggerItem newDebugger;
        newDebugger.setCommand(command);
        const QString displayName = m_type == TypeArmGcc
                                        ? McuPackage::tr("Arm GDB at %1")
                                        : m_type == TypeIAR ? QLatin1String("/foo/bar-iar-gdb")
                                                            : QLatin1String("/bar/foo-keil-gdb");
        newDebugger.setUnexpandedDisplayName(displayName.arg(command.toUserOutput()));
        debuggerId = DebuggerItemManager::registerDebugger(newDebugger);
    } else {
        debuggerId = debugger->id();
    }
    return debuggerId;
}

McuTarget::McuTarget(const QVersionNumber &qulVersion, const QString &vendor,
                     const QString &platform, OS os,
                     const QVector<McuPackage *> &packages,
                     const McuToolChainPackage *toolChainPackage)
    : m_qulVersion(qulVersion)
    , m_vendor(vendor)
    , m_qulPlatform(platform)
    , m_os(os)
    , m_packages(packages)
    , m_toolChainPackage(toolChainPackage)
{
}

QString McuTarget::vendor() const
{
    return m_vendor;
}

QVector<McuPackage *> McuTarget::packages() const
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

QVersionNumber McuTarget::qulVersion() const
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
    setQulDir(Utils::FilePath::fromUserInput(qtForMCUsSdkPackage->path()));
}

static Utils::FilePath qulDocsDir()
{
    const Utils::FilePath qulDir = McuSupportOptions::qulDirFromSettings();
    if (qulDir.isEmpty() || !qulDir.exists())
        return {};
    const Utils::FilePath docsDir = qulDir.pathAppended("docs");
    return docsDir.exists() ? docsDir : Utils::FilePath();
}

void McuSupportOptions::registerQchFiles()
{
    const QString docsDir = qulDocsDir().toString();
    if (docsDir.isEmpty())
        return;

    const QStringList qchFiles = {
        docsDir + "/quickultralite.qch",
        docsDir + "/quickultralitecmake.qch"
    };
    Core::HelpManager::registerDocumentation(
                Utils::filtered(qchFiles,
                                [](const QString &file) { return QFileInfo::exists(file); }));
}

void McuSupportOptions::registerExamples()
{
    const Utils::FilePath docsDir = qulDocsDir();
    if (docsDir.isEmpty())
        return;

    const Utils::FilePath examplesDir =
            McuSupportOptions::qulDirFromSettings().pathAppended("demos");
    if (!examplesDir.exists())
        return;

    QtSupport::QtVersionManager::registerExampleSet("Qt for MCUs", docsDir.toString(),
                                                    examplesDir.toString());
}

void McuSupportOptions::deletePackagesAndTargets()
{
    qDeleteAll(packages);
    packages.clear();
    qDeleteAll(mcuTargets);
    mcuTargets.clear();
}

const QVersionNumber &McuSupportOptions::minimalQulVersion()
{
    static const QVersionNumber v({1, 3});
    return v;
}

void McuSupportOptions::setQulDir(const Utils::FilePath &dir)
{
    deletePackagesAndTargets();
    Sdk::targetsAndPackages(dir, &packages, &mcuTargets);
    //packages.append(qtForMCUsSdkPackage);
    for (auto package : packages) {
        connect(package, &McuPackage::changed, [this](){
            emit changed();
        });
    }
    emit changed();
}

Utils::FilePath McuSupportOptions::qulDirFromSettings()
{
    return Utils::FilePath::fromUserInput(
                packagePathFromSettings(Constants::SETTINGS_KEY_PACKAGE_QT_FOR_MCUS_SDK,
                                        QSettings::UserScope));
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
    using namespace Constants;

    k->setUnexpandedDisplayName(kitName);
    k->setValue(KIT_MCUTARGET_VENDOR_KEY, mcuTarget->vendor());
    k->setValue(KIT_MCUTARGET_MODEL_KEY, mcuTarget->qulPlatform());
    k->setValue(KIT_MCUTARGET_COLORDEPTH_KEY, mcuTarget->colorDepth());
    k->setValue(KIT_MCUTARGET_SDKVERSION_KEY, mcuTarget->qulVersion().toString());
    k->setValue(KIT_MCUTARGET_KITVERSION_KEY, KIT_VERSION);
    k->setValue(KIT_MCUTARGET_OS_KEY, static_cast<int>(mcuTarget->os()));
    k->setAutoDetected(true);
    k->makeSticky();
    if (mcuTarget->toolChainPackage()->type() == McuToolChainPackage::TypeDesktop)
        k->setDeviceTypeForIcon(DEVICE_TYPE);
    QSet<Utils::Id> irrelevant = {
        SysRootKitAspect::id(),
        QtSupport::QtKitAspect::id()
    };
    if (jomExecutablePath().exists()) // TODO: add id() getter to CMakeGeneratorKitAspect
        irrelevant.insert("CMake.GeneratorKitInformation");
    k->setIrrelevantAspects(irrelevant);
}

static void setKitToolchains(ProjectExplorer::Kit *k, const McuToolChainPackage *tcPackage)
{
    // No Green Hills toolchain, because support for it is missing.
    if (tcPackage->type() == McuToolChainPackage::TypeGHS)
        return;

    ProjectExplorer::ToolChainKitAspect::setToolChain(k, tcPackage->toolChain(
                                         ProjectExplorer::Constants::C_LANGUAGE_ID));
    ProjectExplorer::ToolChainKitAspect::setToolChain(k, tcPackage->toolChain(
                                         ProjectExplorer::Constants::CXX_LANGUAGE_ID));
}

static void setKitDebugger(ProjectExplorer::Kit *k, const McuToolChainPackage *tcPackage)
{
    // Qt Creator seems to be smart enough to deduce the right Kit debugger from the ToolChain
    // We rely on that at least in the Desktop case.
    if (tcPackage->type() == McuToolChainPackage::TypeDesktop
            // No Green Hills debugger, because support for it is missing.
            || tcPackage->type() == McuToolChainPackage::TypeGHS)
        return;

    Debugger::DebuggerKitAspect::setDebugger(k, tcPackage->debuggerId());
}

static void setKitDevice(ProjectExplorer::Kit *k, const McuTarget* mcuTarget)
{
    // "Device Type" Desktop is the default. We use that for the Qt for MCUs Desktop Kit
    if (mcuTarget->toolChainPackage()->type() == McuToolChainPackage::TypeDesktop)
        return;

    ProjectExplorer::DeviceTypeKitAspect::setDeviceTypeId(k, Constants::DEVICE_TYPE);
}

static void setKitEnvironment(ProjectExplorer::Kit *k, const McuTarget* mcuTarget,
                              const McuPackage *qtForMCUsSdkPackage)
{
    using namespace ProjectExplorer;

    Utils::EnvironmentItems changes;
    QStringList pathAdditions;

    // The Desktop version depends on the Qt shared libs in Qul_DIR/bin.
    // If CMake's fileApi is avaialble, we can rely on the "Add library search path to PATH"
    // feature of the run configuration. Otherwise, we just prepend the path, here.
    if (mcuTarget->toolChainPackage()->type() == McuToolChainPackage::TypeDesktop
            && !CMakeProjectManager::CMakeToolManager::defaultCMakeTool()->hasFileApi())
        pathAdditions.append(QDir::toNativeSeparators(qtForMCUsSdkPackage->path() + "/bin"));

    auto processPackage = [&pathAdditions, &changes](const McuPackage *package) {
        if (package->addToPath())
            pathAdditions.append(QDir::toNativeSeparators(package->path()));
        if (!package->environmentVariableName().isEmpty())
            changes.append({package->environmentVariableName(),
                            QDir::toNativeSeparators(package->path())});
    };
    for (auto package : mcuTarget->packages())
        processPackage(package);
    processPackage(qtForMCUsSdkPackage);

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
    // CMake ToolChain file for ghs handles CMAKE_*_COMPILER autonomously
    if (mcuTarget->toolChainPackage()->type() != McuToolChainPackage::TypeGHS) {
        config.append(CMakeConfigItem("CMAKE_CXX_COMPILER", "%{Compiler:Executable:Cxx}"));
        config.append(CMakeConfigItem("CMAKE_C_COMPILER", "%{Compiler:Executable:C}"));
    }
    if (mcuTarget->toolChainPackage()->type() != McuToolChainPackage::TypeDesktop)
        config.append(CMakeConfigItem(
                          "CMAKE_TOOLCHAIN_FILE",
                          (qulDir + "/lib/cmake/Qul/toolchain/"
                           + mcuTarget->toolChainPackage()->cmakeToolChainFileName()).toUtf8()));
    config.append(CMakeConfigItem("QUL_GENERATORS",
                                  (qulDir + "/lib/cmake/Qul/QulGenerators.cmake").toUtf8()));
    config.append(CMakeConfigItem("QUL_PLATFORM",
                                  mcuTarget->qulPlatform().toUtf8()));
    if (mcuTarget->os() == McuTarget::OS::FreeRTOS)
        config.append(CMakeConfigItem("OS", "FreeRTOS"));
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

static void setKitQtVersionOptions(ProjectExplorer::Kit *k)
{
    QtSupport::QtKitAspect::setQtVersion(k, nullptr);
}

QString McuSupportOptions::kitName(const McuTarget *mcuTarget)
{
    const QString os = QLatin1String(mcuTarget->os()
                                     == McuTarget::OS::FreeRTOS ? " FreeRTOS" : "");
    const QString colorDepth = mcuTarget->colorDepth() > 0
            ? QString::fromLatin1(" %1bpp").arg(mcuTarget->colorDepth())
            : "";
    // Hack: Use the platform name in the kit name. Exception for the "Qt" platform: use "Desktop"
    const QString targetName =
            mcuTarget->toolChainPackage()->type() == McuToolChainPackage::TypeDesktop
            ? "Desktop"
            : mcuTarget->qulPlatform();
    return QString::fromLatin1("Qt for MCUs %1 - %2%3%4")
            .arg(mcuTarget->qulVersion().toString(), targetName, os, colorDepth);
}

QList<ProjectExplorer::Kit *> McuSupportOptions::existingKits(const McuTarget *mcuTarget)
{
    using namespace ProjectExplorer;
    using namespace Constants;
    return Utils::filtered(KitManager::kits(), [mcuTarget](Kit *kit) {
        return kit->isAutoDetected()
                && kit->value(KIT_MCUTARGET_KITVERSION_KEY) == KIT_VERSION
                && (!mcuTarget || (
                        kit->value(KIT_MCUTARGET_VENDOR_KEY) == mcuTarget->vendor()
                        && kit->value(KIT_MCUTARGET_MODEL_KEY) == mcuTarget->qulPlatform()
                        && kit->value(KIT_MCUTARGET_COLORDEPTH_KEY) == mcuTarget->colorDepth()
                        && kit->value(KIT_MCUTARGET_OS_KEY).toInt()
                           == static_cast<int>(mcuTarget->os())
                        ));
    });
}

QList<ProjectExplorer::Kit *> McuSupportOptions::outdatedKits()
{
    return Utils::filtered(ProjectExplorer::KitManager::kits(), [](ProjectExplorer::Kit *kit) {
        return kit->isAutoDetected()
                && !kit->value(Constants::KIT_MCUTARGET_VENDOR_KEY).isNull()
                && kit->value(Constants::KIT_MCUTARGET_KITVERSION_KEY) != KIT_VERSION;
    });
}

void McuSupportOptions::removeOutdatedKits()
{
    for (auto kit : McuSupportOptions::outdatedKits())
        ProjectExplorer::KitManager::deregisterKit(kit);
}

ProjectExplorer::Kit *McuSupportOptions::newKit(const McuTarget *mcuTarget,
                                                const McuPackage *qtForMCUsSdk)
{
    using namespace ProjectExplorer;

    const auto init = [mcuTarget, qtForMCUsSdk](Kit *k) {
        KitGuard kitGuard(k);

        setKitProperties(kitName(mcuTarget), k, mcuTarget);
        setKitDevice(k, mcuTarget);
        setKitToolchains(k, mcuTarget->toolChainPackage());
        setKitDebugger(k, mcuTarget->toolChainPackage());
        setKitEnvironment(k, mcuTarget, qtForMCUsSdk);
        setKitCMakeOptions(k, mcuTarget, qtForMCUsSdk->path());
        setKitQtVersionOptions(k);

        k->setup();
        k->fix();
    };

    return KitManager::registerKit(init);
}

} // Internal
} // McuSupport
