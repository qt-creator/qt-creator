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
#include "mcusupportsdk.h"

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
    m_infoLabel = new Utils::InfoLabel();

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

    m_infoLabel->setType(m_status == ValidPackage ? Utils::InfoLabel::Ok
                                                  : Utils::InfoLabel::NotOk);

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
    m_infoLabel->setText(statusText);
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

static ProjectExplorer::ToolChain* armGccToolChain(const Utils::FilePath &path, Core::Id language)
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

ProjectExplorer::ToolChain *McuToolChainPackage::toolChain(Core::Id language) const
{
    const QLatin1String compilerName(
                language == ProjectExplorer::Constants::C_LANGUAGE_ID ? "gcc" : "g++");
    const Utils::FilePath compiler = Utils::FilePath::fromUserInput(
                Utils::HostOsInfo::withExecutableSuffix(
                path() + (
                    m_type == TypeArmGcc
                    ? "/bin/arm-none-eabi-%1" : m_type == TypeIAR
                      ? "/foo/bar-iar-%1" : "/bar/foo-keil-%1")).arg(compilerName));

    ProjectExplorer::ToolChain *tc = armGccToolChain(compiler, language);
    return tc;
}

QString McuToolChainPackage::cmakeToolChainFileName() const
{
    return QLatin1String(m_type == TypeArmGcc
                         ? "armgcc.cmake" : m_type == McuToolChainPackage::TypeIAR
                           ? "iar.cmake" : "keil.cmake");
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

McuTarget::McuTarget(const QString &vendor, const QString &platform,
                     const QVector<McuPackage *> &packages, McuToolChainPackage *toolChainPackage)
    : m_vendor(vendor)
    , m_qulPlatform(platform)
    , m_packages(packages)
    , m_toolChainPackage(toolChainPackage)
{
    QTC_CHECK(m_toolChainPackage == nullptr || m_packages.contains(m_toolChainPackage));
}

QString McuTarget::vendor() const
{
    return m_vendor;
}

QVector<McuPackage *> McuTarget::packages() const
{
    return m_packages;
}

McuToolChainPackage *McuTarget::toolChainPackage() const
{
    return m_toolChainPackage;
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

void McuSupportOptions::deletePackagesAndTargets()
{
    qDeleteAll(packages);
    packages.clear();
    qDeleteAll(mcuTargets);
    mcuTargets.clear();
}

void McuSupportOptions::setQulDir(const Utils::FilePath &dir)
{
    deletePackagesAndTargets();
    Sdk::hardcodedTargetsAndPackages(dir, &packages, &mcuTargets);
    //packages.append(qtForMCUsSdkPackage);
    for (auto package : packages) {
        connect(package, &McuPackage::changed, [this](){
            emit changed();
        });
    }
    emit changed();
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
    k->setValue(Constants::KIT_MCUTARGET_MODEL_KEY, mcuTarget->qulPlatform());
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

static void setKitToolchains(ProjectExplorer::Kit *k, const McuToolChainPackage *tcPackage)
{
    ProjectExplorer::ToolChainKitAspect::setToolChain(k, tcPackage->toolChain(
                                         ProjectExplorer::Constants::C_LANGUAGE_ID));
    ProjectExplorer::ToolChainKitAspect::setToolChain(k, tcPackage->toolChain(
                                         ProjectExplorer::Constants::CXX_LANGUAGE_ID));
}

static void setKitDebugger(ProjectExplorer::Kit *k, const McuToolChainPackage *tcPackage)
{
    Debugger::DebuggerKitAspect::setDebugger(k, tcPackage->debuggerId());
}

static void setKitDevice(ProjectExplorer::Kit *k)
{
    ProjectExplorer::DeviceTypeKitAspect::setDeviceTypeId(k, Constants::DEVICE_TYPE);
}

static void setKitEnvironment(ProjectExplorer::Kit *k, const McuTarget* mcuTarget,
                              McuPackage *qtForMCUsSdkPackage)
{
    using namespace ProjectExplorer;

    Utils::EnvironmentItems changes;
    QStringList pathAdditions;

    QVector<McuPackage *> packagesIncludingSdk;
    packagesIncludingSdk.reserve(mcuTarget->packages().size() + 1);
    packagesIncludingSdk.append(mcuTarget->packages());
    packagesIncludingSdk.append(qtForMCUsSdkPackage);

    for (auto package : packagesIncludingSdk) {
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
    if (mcuTarget->toolChainPackage())
        config.append(CMakeConfigItem(
                          "CMAKE_TOOLCHAIN_FILE",
                          (qulDir + "/lib/cmake/Qul/toolchain/"
                           + mcuTarget->toolChainPackage()->cmakeToolChainFileName()).toUtf8()));
    config.append(CMakeConfigItem("QUL_GENERATORS",
                                  (qulDir + "/lib/cmake/Qul/QulGenerators.cmake").toUtf8()));
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
    return QString::fromLatin1("Qt for MCUs - %1%2")
            .arg(mcuTarget->qulPlatform(), colorDepth);
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

    const auto init = [this, mcuTarget](Kit *k) {
        KitGuard kitGuard(k);

        setKitProperties(kitName(mcuTarget), k, mcuTarget);
        if (!mcuTargetIsDesktop(mcuTarget)) {
            setKitToolchains(k, mcuTarget->toolChainPackage());
            setKitDebugger(k, mcuTarget->toolChainPackage());
            setKitDevice(k);
        }
        setKitEnvironment(k, mcuTarget, qtForMCUsSdkPackage);
        setKitCMakeOptions(k, mcuTarget, qtForMCUsSdkPackage->path());

        k->setup();
        k->fix();
    };

    return KitManager::registerKit(init);
}

} // Internal
} // McuSupport
