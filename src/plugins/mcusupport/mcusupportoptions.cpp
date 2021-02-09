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

#include <baremetal/baremetalconstants.h>
#include <cmakeprojectmanager/cmaketoolmanager.h>
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
#include <QToolButton>
#include <QVBoxLayout>
#include <QVariant>

using namespace ProjectExplorer;
using namespace Utils;

namespace McuSupport {
namespace Internal {

static const int KIT_VERSION = 8; // Bumps up whenever details in Kit creation change

static QString packagePathFromSettings(const QString &settingsKey,
                                       QSettings::Scope scope = QSettings::UserScope,
                                       const QString &defaultPath = {})
{
    QSettings *settings = Core::ICore::settings(scope);
    const QString key = QLatin1String(Constants::SETTINGS_GROUP) + '/' +
            QLatin1String(Constants::SETTINGS_KEY_PACKAGE_PREFIX) + settingsKey;
    const QString path = settings->value(key, defaultPath).toString();
    return FilePath::fromUserInput(path).toString();
}

static bool automaticKitCreationFromSettings(QSettings::Scope scope = QSettings::UserScope)
{
    QSettings *settings = Core::ICore::settings(scope);
    const QString key = QLatin1String(Constants::SETTINGS_GROUP) + '/' +
            QLatin1String(Constants::SETTINGS_KEY_AUTOMATIC_KIT_CREATION);
    bool automaticKitCreation = settings->value(key, true).toBool();
    return automaticKitCreation;
}

static bool kitNeedsQtVersion()
{
    // Only on Windows, Qt is linked into the distributed qul Desktop libs. Also, the host tools
    // are missing the Qt runtime libraries on non-Windows.
    return !HostOsInfo::isWindowsHost();
}

McuPackage::McuPackage(const QString &label, const QString &defaultPath,
                       const QString &detectionPath, const QString &settingsKey)
    : m_label(label)
    , m_defaultPath(packagePathFromSettings(settingsKey, QSettings::SystemScope, defaultPath))
    , m_detectionPath(detectionPath)
    , m_settingsKey(settingsKey)
{
    m_path = packagePathFromSettings(settingsKey, QSettings::UserScope, m_defaultPath);
    m_automaticKitCreation = automaticKitCreationFromSettings(QSettings::UserScope);
}

QString McuPackage::basePath() const
{
    return m_fileChooser != nullptr ? m_fileChooser->filePath().toString() : m_path;
}

QString McuPackage::path() const
{
    return QFileInfo(basePath() + m_relativePathModifier).absoluteFilePath();
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
    m_fileChooser = new PathChooser;
    m_fileChooser->lineEdit()->setButtonIcon(FancyLineEdit::Right,
                                             Icons::RESET.icon());
    m_fileChooser->lineEdit()->setButtonVisible(FancyLineEdit::Right, true);
    connect(m_fileChooser->lineEdit(), &FancyLineEdit::rightButtonClicked, this, [&] {
        m_fileChooser->setPath(m_defaultPath);
    });

    auto layout = new QGridLayout(m_widget);
    layout->setContentsMargins(0, 0, 0, 0);
    m_infoLabel = new InfoLabel();

    if (!m_downloadUrl.isEmpty()) {
        auto downLoadButton = new QToolButton;
        downLoadButton->setIcon(Icons::ONLINE.icon());
        downLoadButton->setToolTip(tr("Download from \"%1\"").arg(m_downloadUrl));
        QObject::connect(downLoadButton, &QToolButton::pressed, this, [this] {
            QDesktopServices::openUrl(m_downloadUrl);
        });
        layout->addWidget(downLoadButton, 0, 2);
    }

    layout->addWidget(m_fileChooser, 0, 0, 1, 2);
    layout->addWidget(m_infoLabel, 1, 0, 1, -1);

    m_fileChooser->setPath(m_path);

    QObject::connect(this, &McuPackage::statusChanged, this, [this] {
        updateStatusUi();
    });

    QObject::connect(m_fileChooser, &PathChooser::pathChanged, this, [this] {
        updatePath();
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

void McuPackage::writeGeneralSettings() const
{
    const QString key = QLatin1String(Constants::SETTINGS_GROUP) + '/' +
            QLatin1String(Constants::SETTINGS_KEY_AUTOMATIC_KIT_CREATION);
    QSettings *settings = Core::ICore::settings();
    settings->setValue(key, m_automaticKitCreation);
}

void McuPackage::writeToSettings() const
{
    const QString key = QLatin1String(Constants::SETTINGS_GROUP) + '/' +
            QLatin1String(Constants::SETTINGS_KEY_PACKAGE_PREFIX) + m_settingsKey;
    Core::ICore::settings()->setValueWithDefault(key, m_path, m_defaultPath);
}

void McuPackage::setRelativePathModifier(const QString &path)
{
    m_relativePathModifier = path;
}

bool McuPackage::automaticKitCreationEnabled() const
{
    return m_automaticKitCreation;
}

void McuPackage::setAutomaticKitCreationEnabled(const bool enabled)
{
    m_automaticKitCreation = enabled;
}

void McuPackage::updatePath()
{
   m_path = m_fileChooser->rawPath();
   m_fileChooser->lineEdit()->button(FancyLineEdit::Right)->setEnabled(m_path != m_defaultPath);
   updateStatus();
}

void McuPackage::updateStatus()
{
    bool validPath = !m_path.isEmpty() && FilePath::fromString(m_path).exists();
    const FilePath detectionPath = FilePath::fromString(basePath() + "/" + m_detectionPath);
    const bool validPackage = m_detectionPath.isEmpty() || detectionPath.exists();

    m_status = validPath ? (validPackage ? ValidPackage : ValidPathInvalidPackage) :
        m_path.isEmpty() ? EmptyPath : InvalidPath;

    emit statusChanged();
}

void McuPackage::updateStatusUi()
{
    m_infoLabel->setType(m_status == ValidPackage ? InfoLabel::Ok : InfoLabel::NotOk);
    m_infoLabel->setText(statusText());
}

QString McuPackage::statusText() const
{
    const QString displayPackagePath = FilePath::fromString(m_path).toUserOutput();
    const QString displayDetectionPath = FilePath::fromString(m_detectionPath).toUserOutput();
    QString response;
    switch (m_status) {
    case ValidPackage:
        response = m_detectionPath.isEmpty()
                ? tr("Path %1 exists.").arg(displayPackagePath)
                : tr("Path %1 is valid, %2 was found.")
                  .arg(displayPackagePath, displayDetectionPath);
        break;
    case ValidPathInvalidPackage:
        response = tr("Path %1 exists, but does not contain %2.")
                .arg(displayPackagePath, displayDetectionPath);
        break;
    case InvalidPath:
        response = tr("Path %1 does not exist.").arg(displayPackagePath);
        break;
    case EmptyPath:
        response = m_detectionPath.isEmpty()
                ? tr("Path is empty.")
                : tr("Path is empty, %1 not found.")
                    .arg(displayDetectionPath);
        break;
    }
    return response;
}

McuToolChainPackage::McuToolChainPackage(const QString &label,
                                         const QString &defaultPath,
                                         const QString &detectionPath,
                                         const QString &settingsKey,
                                         McuToolChainPackage::Type type)
    : McuPackage(label, defaultPath, detectionPath, settingsKey)
    , m_type(type)
{
}

McuToolChainPackage::Type McuToolChainPackage::type() const
{
    return m_type;
}

bool McuToolChainPackage::isDesktopToolchain() const
{
    return m_type == TypeMSVC || m_type == TypeGCC;
}

static ToolChain *msvcToolChain(Id language)
{
    ToolChain *toolChain = ToolChainManager::toolChain([language](const ToolChain *t) {
        const Abi abi = t->targetAbi();
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

static ToolChain *iarToolChain(Id language)
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
            const QList<ToolChain*> detected = iarFactory->autoDetect(QList<ToolChain*>());
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
        tc = iarToolChain(language);
    }
    else {
        const QLatin1String compilerName(
                    language == ProjectExplorer::Constants::C_LANGUAGE_ID ? "gcc" : "g++");
        const FilePath compiler = FilePath::fromUserInput(
                    HostOsInfo::withExecutableSuffix(
                        path() + (
                            m_type == TypeArmGcc
                            ? "/bin/arm-none-eabi-%1" : "/bar/foo-keil-%1")).arg(compilerName));

        tc = armGccToolChain(compiler, language);
    }
    return tc;
}

QString McuToolChainPackage::toolChainName() const
{
    return QLatin1String(m_type == TypeArmGcc
                         ? "armgcc" : m_type == McuToolChainPackage::TypeIAR
                           ? "iar" : m_type == McuToolChainPackage::TypeKEIL
                             ? "keil" : m_type == McuToolChainPackage::TypeGHS
                               ? "ghs" : "unsupported");
}

QString McuToolChainPackage::cmakeToolChainFileName() const
{
    return toolChainName() + QLatin1String(".cmake");
}

QVariant McuToolChainPackage::debuggerId() const
{
    using namespace Debugger;

    const FilePath command = FilePath::fromUserInput(
                HostOsInfo::withExecutableSuffix(path() + (
                        m_type == TypeArmGcc
                        ? "/bin/arm-none-eabi-gdb-py" : m_type == TypeIAR
                        ? "../common/bin/CSpyBat" : "/bar/foo-keil-gdb")));
    const DebuggerItem *debugger = DebuggerItemManager::findByCommand(command);
    QVariant debuggerId;
    if (!debugger) {
        DebuggerItem newDebugger;
        newDebugger.setCommand(command);
        const QString displayName = m_type == TypeArmGcc
                                        ? McuPackage::tr("Arm GDB at %1")
                                        : m_type == TypeIAR ? QLatin1String("CSpy")
                                                            : QLatin1String("/bar/foo-keil-gdb");
        newDebugger.setUnexpandedDisplayName(displayName.arg(command.toUserOutput()));
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

McuTarget::Platform McuTarget::platform() const
{
    return m_platform;
}

bool McuTarget::isValid() const
{
    return !Utils::anyOf(packages(), [](McuPackage *package) {
        package->updateStatus();
        return package->status() != McuPackage::ValidPackage;
    });
}

void McuTarget::printPackageProblems() const
{
    for (auto package: packages()) {
        package->updateStatus();
        if (package->status() != McuPackage::ValidPackage)
            printMessage(QString("Error creating kit for target %1, package %2: %3").arg(
                             McuSupportOptions::kitName(this),
                             package->label(),
                             package->statusText()),
                         true);
    }
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
    setQulDir(FilePath::fromUserInput(qtForMCUsSdkPackage->path()));
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

void McuSupportOptions::setQulDir(const FilePath &dir)
{
    deletePackagesAndTargets();
    qtForMCUsSdkPackage->updateStatus();
    if (qtForMCUsSdkPackage->status() == McuPackage::Status::ValidPackage)
        Sdk::targetsAndPackages(dir, &packages, &mcuTargets);
    for (auto package : qAsConst(packages))
        connect(package, &McuPackage::changed, this, &McuSupportOptions::changed);

    emit changed();
}

FilePath McuSupportOptions::qulDirFromSettings()
{
    return FilePath::fromUserInput(
                packagePathFromSettings(Constants::SETTINGS_KEY_PACKAGE_QT_FOR_MCUS_SDK,
                                        QSettings::UserScope));
}

static void setKitProperties(const QString &kitName, Kit *k, const McuTarget *mcuTarget)
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
    k->setAutoDetected(true);
    k->makeSticky();
    if (mcuTarget->toolChainPackage()->isDesktopToolchain())
        k->setDeviceTypeForIcon(DEVICE_TYPE);
    k->setValue(QtSupport::SuppliesQtQuickImportPath::id(), true);
    QSet<Id> irrelevant = { SysRootKitAspect::id(), QtSupport::SuppliesQtQuickImportPath::id() };
    if (!kitNeedsQtVersion())
        irrelevant.insert(QtSupport::QtKitAspect::id());
    k->setIrrelevantAspects(irrelevant);
}

static void setKitToolchains(Kit *k, const McuToolChainPackage *tcPackage)
{
    // No Green Hills toolchain, because support for it is missing.
    if (tcPackage->type() == McuToolChainPackage::TypeUnsupported
        || tcPackage->type() == McuToolChainPackage::TypeGHS)
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
            || tcPackage->type() == McuToolChainPackage::TypeIAR)
        return;

    Debugger::DebuggerKitAspect::setDebugger(k, tcPackage->debuggerId());
}

static void setKitDevice(Kit *k, const McuTarget* mcuTarget)
{
    // "Device Type" Desktop is the default. We use that for the Qt for MCUs Desktop Kit
    if (mcuTarget->toolChainPackage()->isDesktopToolchain())
        return;

    DeviceTypeKitAspect::setDeviceTypeId(k, Constants::DEVICE_TYPE);
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

    // Clang not needed in version 1.7+
    if (mcuTarget->qulVersion() < QVersionNumber{1,7}) {
        const QString path = QLatin1String(HostOsInfo::isWindowsHost() ? "Path" : "PATH");
        pathAdditions.append("${" + path + "}");
        pathAdditions.append(QDir::toNativeSeparators(Core::ICore::libexecPath() + "/clang/bin"));
        changes.append({path, pathAdditions.join(HostOsInfo::pathListSeparator())});
    }

    if (kitNeedsQtVersion())
        changes.append({QLatin1String("LD_LIBRARY_PATH"), "%{Qt:QT_INSTALL_LIBS}"});

    EnvironmentKitAspect::setEnvironmentChanges(k, changes);
}

static void setKitCMakeOptions(Kit *k, const McuTarget* mcuTarget, const QString &qulDir)
{
    using namespace CMakeProjectManager;

    CMakeConfig config = CMakeConfigurationKitAspect::configuration(k);
    // CMake ToolChain file for ghs handles CMAKE_*_COMPILER autonomously
    if (mcuTarget->toolChainPackage()->type() != McuToolChainPackage::TypeGHS) {
        config.append(CMakeConfigItem("CMAKE_CXX_COMPILER", "%{Compiler:Executable:Cxx}"));
        config.append(CMakeConfigItem("CMAKE_C_COMPILER", "%{Compiler:Executable:C}"));
    }

    if (!mcuTarget->toolChainPackage()->isDesktopToolchain()) {
        const FilePath cMakeToolchainFile = FilePath::fromString(qulDir + "/lib/cmake/Qul/toolchain/"
                           + mcuTarget->toolChainPackage()->cmakeToolChainFileName());

        config.append(CMakeConfigItem(
                          "CMAKE_TOOLCHAIN_FILE",
                          cMakeToolchainFile.toString().toUtf8()));
        if (!cMakeToolchainFile.exists()) {
            printMessage(McuTarget::tr("Warning for target %1: missing CMake Toolchain File expected at %2.")
                  .arg(McuSupportOptions::kitName(mcuTarget), cMakeToolchainFile.toUserOutput()), false);
        }
    }

    const FilePath generatorsPath = FilePath::fromString(qulDir + "/lib/cmake/Qul/QulGenerators.cmake");
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
        return kit->isAutoDetected()
                && kit->value(KIT_MCUTARGET_KITVERSION_KEY) == KIT_VERSION
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

QList<Kit *> McuSupportOptions::outdatedKits()
{
    return Utils::filtered(KitManager::kits(), [](Kit *kit) {
        return kit->isAutoDetected()
                && !kit->value(Constants::KIT_MCUTARGET_VENDOR_KEY).isNull()
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

void printMessage(const QString &message, bool important)
{
    const QString displayMessage = QCoreApplication::translate("QtForMCUs", "Qt for MCUs: %1").arg(message);
    if (important)
        Core::MessageManager::writeFlashing(displayMessage);
    else
        Core::MessageManager::writeSilently(displayMessage);
}

void McuSupportOptions::createAutomaticKits()
{
    auto qtForMCUsPackage = Sdk::createQtForMCUsPackage();

    const auto createKits = [qtForMCUsPackage]() {
    if (qtForMCUsPackage->automaticKitCreationEnabled()) {
        qtForMCUsPackage->updateStatus();
        if (qtForMCUsPackage->status() != McuPackage::ValidPackage) {
            switch (qtForMCUsPackage->status()) {
                case McuPackage::ValidPathInvalidPackage: {
                    const QString displayPath = FilePath::fromString(qtForMCUsPackage->detectionPath())
                        .toUserOutput();
                    printMessage(tr("Path %1 exists, but does not contain %2.")
                        .arg(qtForMCUsPackage->path(), displayPath),
                                 true);
                    break;
                }
                case McuPackage::InvalidPath: {
                    printMessage(tr("Path %1 does not exist. Add the path in Tools > Options > Devices > MCU.")
                        .arg(qtForMCUsPackage->path()),
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

        auto dir = FilePath::fromUserInput(qtForMCUsPackage->path());
        QVector<McuPackage*> packages;
        QVector<McuTarget*> mcuTargets;
        Sdk::targetsAndPackages(dir, &packages, &mcuTargets);

        for (auto target: qAsConst(mcuTargets))
            if (existingKits(target).isEmpty()) {
                if (target->isValid())
                    newKit(target, qtForMCUsPackage);
                else
                    target->printPackageProblems();
            }

        qDeleteAll(packages);
        qDeleteAll(mcuTargets);
    }
   };

    createKits();
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
    }
}

} // Internal
} // McuSupport
