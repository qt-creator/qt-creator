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

PackageOptions::PackageOptions(const QString &label, const QString &defaultPath,
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

QString PackageOptions::path() const
{
    return QFileInfo(m_fileChooser->path() + m_relativePathModifier).absoluteFilePath();
}

QString PackageOptions::label() const
{
    return m_label;
}

QString PackageOptions::detectionPath() const
{
    return m_detectionPath;
}

QWidget *PackageOptions::widget()
{
    if (m_widget)
        return m_widget;

    m_widget = new QWidget;
    m_fileChooser = new Utils::PathChooser;
    QObject::connect(m_fileChooser, &Utils::PathChooser::pathChanged,
                     [this](){
        updateStatus();
        emit changed();
    });

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

    m_fileChooser->setPath(m_path); // Triggers updateStatus() call
    return m_widget;
}

PackageOptions::Status PackageOptions::status() const
{
    return m_status;
}

void PackageOptions::setDownloadUrl(const QString &url)
{
    m_downloadUrl = url;
}

void PackageOptions::setEnvironmentVariableName(const QString &name)
{
    m_environmentVariableName = name;
}

QString PackageOptions::environmentVariableName() const
{
    return m_environmentVariableName;
}

void PackageOptions::setAddToPath(bool addToPath)
{
    m_addToPath = addToPath;
}

bool PackageOptions::addToPath() const
{
    return m_addToPath;
}

void PackageOptions::writeToSettings() const
{
    if (m_path.compare(m_defaultPath) == 0)
        return;
    QSettings *s = Core::ICore::settings();
    s->beginGroup(Constants::SETTINGS_GROUP);
    s->setValue(QLatin1String(Constants::SETTINGS_KEY_PACKAGE_PREFIX) + m_settingsKey, m_path);
    s->endGroup();
}

void PackageOptions::setRelativePathModifier(const QString &path)
{
    m_relativePathModifier = path;
}

void PackageOptions::updateStatus()
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

BoardOptions::BoardOptions(const QString &model, const QString &toolChainFileName,
                           const QVector<PackageOptions*> &packages)
    : m_model(model)
    , m_toolChainFile(toolChainFileName)
    , m_packages(packages)
{
}

QString BoardOptions::model() const
{
    return m_model;
}

QString BoardOptions::toolChainFile() const
{
    return m_toolChainFile;
}

QVector<PackageOptions *> BoardOptions::packages() const
{
    return m_packages;
}

static PackageOptions *createQulPackage()
{
    auto result = new PackageOptions(
                PackageOptions::tr("Qt MCU SDK"),
                QDir::homePath(),
                Utils::HostOsInfo::withExecutableSuffix("bin/qmltocpp"),
                "qulSdk");
    result->setEnvironmentVariableName("Qul_DIR");
    return result;
}

static PackageOptions *createArmGccPackage()
{
    const QString defaultPath =
            Utils::HostOsInfo::isWindowsHost() ?
                QDir::fromNativeSeparators(qEnvironmentVariable("ProgramFiles(x86)"))
                + "/GNU Tools ARM Embedded/"
              : QString("%{Env:ARMGCC_DIR}");
    auto result = new PackageOptions(
                PackageOptions::tr("GNU Arm Embedded Toolchain"),
                defaultPath,
                Utils::HostOsInfo::withExecutableSuffix("bin/arm-none-eabi-g++"),
                Constants::SETTINGS_KEY_PACKAGE_ARMGCC);
    result->setDownloadUrl(
                "https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads");
    result->setEnvironmentVariableName("ARMGCC_DIR");
    return result;
}

static PackageOptions *createStm32CubeFwF7SdkPackage()
{
    auto result = new PackageOptions(
                PackageOptions::tr("STM32Cube SDK"),
                "%{Env:STM32Cube_FW_F7_SDK_PATH}",
                "Drivers/STM32F7xx_HAL_Driver",
                "stm32CubeFwF7Sdk");
    result->setDownloadUrl(
                "https://www.st.com/content/st_com/en/products/embedded-software/mcus-embedded-software/stm32-embedded-software/stm32cube-mcu-packages/stm32cubef7.html");
    result->setEnvironmentVariableName("STM32Cube_FW_F7_SDK_PATH");
    return result;
}

static PackageOptions *createStm32CubeProgrammerPackage()
{
    const QString defaultPath =
            Utils::HostOsInfo::isWindowsHost() ?
                QDir::fromNativeSeparators(qEnvironmentVariable("ProgramFiles"))
                + "/STMicroelectronics/STM32Cube/STM32CubeProgrammer/"
              : QDir::homePath();
    auto result = new PackageOptions(
                PackageOptions::tr("STM32CubeProgrammer"),
                defaultPath,
                QLatin1String(Utils::HostOsInfo::isWindowsHost() ? "/bin/STM32_Programmer_CLI.exe"
                                                                 : "/bin/STM32_Programmer.sh"),
                "stm32CubeProgrammer");
    result->setRelativePathModifier("/bin");
    result->setDownloadUrl(
                "https://www.st.com/en/development-tools/stm32cubeprog.html");
    result->setAddToPath(true);
    return result;
}

static PackageOptions *createEvkbImxrt1050SdkPackage()
{
    auto result = new PackageOptions(
                PackageOptions::tr("NXP i.MXRT SDK"),
                "%{Env:EVKB_IMXRT1050_SDK_PATH}", // TODO: Try to not use 1050 specifics
                "EVKB-IMXRT1050_manifest_v3_5.xml",
                "evkbImxrt1050Sdk");
    result->setDownloadUrl("https://mcuxpresso.nxp.com/en/welcome");
    return result;
}

static PackageOptions *createSeggerJLinkPackage()
{
    const QString defaultPath =
            Utils::HostOsInfo::isWindowsHost() ?
                QDir::fromNativeSeparators(qEnvironmentVariable("ProgramFiles")) + "/SEGGER/JLink"
              : QString("%{Env:SEGGER_JLINK_SOFTWARE_AND_DOCUMENTATION_PATH}");
    auto result = new PackageOptions(
                PackageOptions::tr("SEGGER JLink"),
                defaultPath,
                Utils::HostOsInfo::withExecutableSuffix("JLink"),
                "seggerJLink");
    result->setDownloadUrl("https://www.segger.com/downloads/jlink");
    result->setEnvironmentVariableName("SEGGER_JLINK_SOFTWARE_AND_DOCUMENTATION_PATH");
    return result;
}

McuSupportOptions::McuSupportOptions(QObject *parent)
    : QObject(parent)
{
    PackageOptions* qulPackage = createQulPackage();
    PackageOptions* armGccPackage = createArmGccPackage();
    PackageOptions* stm32CubeFwF7SdkPackage = createStm32CubeFwF7SdkPackage();
    PackageOptions* stm32CubeProgrammerPackage = createStm32CubeProgrammerPackage();
    PackageOptions* evkbImxrt1050SdkPackage = createEvkbImxrt1050SdkPackage();
    PackageOptions* seggerJLinkPackage = createSeggerJLinkPackage();

    toolchainPackage = armGccPackage;


    auto stmPackages = {armGccPackage, stm32CubeFwF7SdkPackage, stm32CubeProgrammerPackage,
                        qulPackage};
    auto nxpPackages = {armGccPackage, evkbImxrt1050SdkPackage, seggerJLinkPackage,
                        qulPackage};
    packages = {armGccPackage, stm32CubeFwF7SdkPackage, stm32CubeProgrammerPackage,
                evkbImxrt1050SdkPackage, seggerJLinkPackage, qulPackage};

    boards.append(new BoardOptions(
                      "stm32f7508", "CMake/stm32f7508-discovery.cmake", stmPackages));
    boards.append(new BoardOptions(
                      "stm32f769i", "CMake/stm32f769i-discovery.cmake", stmPackages));
    boards.append(new BoardOptions(
                      "evkbimxrt1050", "CMake/evkbimxrt1050-toolchain.cmake", nxpPackages));

    for (auto package : packages)
        connect(package, &PackageOptions::changed, [this](){
            emit changed();
        });
}

McuSupportOptions::~McuSupportOptions()
{
    qDeleteAll(packages);
    packages.clear();
    qDeleteAll(boards);
    boards.clear();
}

QVector<BoardOptions *> McuSupportOptions::validBoards() const
{
    return Utils::filtered(boards, [](BoardOptions *board){
        return !Utils::anyOf(board->packages(), [](PackageOptions *package){
            return package->status() != PackageOptions::ValidPackage;});
    });
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

static void setKitProperties(ProjectExplorer::Kit *k, const BoardOptions* board)
{
    using namespace ProjectExplorer;

    k->setUnexpandedDisplayName("Qt MCU - " + board->model());
    k->setValue(Constants::KIT_BOARD_MODEL_KEY, board->model());
    k->setAutoDetected(false);
    k->setIrrelevantAspects({
        SysRootKitAspect::id(),
        "QtSupport.QtInformation" // QtKitAspect::id()
    });
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
                    PackageOptions::tr("Arm GDB at %1").arg(command.toUserOutput()));
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

static void setKitEnvironment(ProjectExplorer::Kit *k, const BoardOptions* board)
{
    using namespace ProjectExplorer;

    Utils::EnvironmentItems changes;
    QStringList pathAdditions;
    for (auto package : board->packages()) {
        if (package->addToPath())
            pathAdditions.append(QDir::toNativeSeparators(package->path()));
        if (!package->environmentVariableName().isEmpty())
            changes.append({package->environmentVariableName(),
                            QDir::toNativeSeparators(package->path())});
    }
    if (!pathAdditions.isEmpty()) {
        pathAdditions.append("${Path}");
        changes.append({"Path", pathAdditions.join(Utils::HostOsInfo::pathListSeparator())});
    }
    EnvironmentKitAspect::setEnvironmentChanges(k, changes);
}

static void setKitCMakeOptions(ProjectExplorer::Kit *k, const BoardOptions* board)
{
    using namespace CMakeProjectManager;

    CMakeConfig config = CMakeConfigurationKitAspect::configuration(k);
    config.append(CMakeConfigItem("CMAKE_TOOLCHAIN_FILE",
                                  ("%{CurrentBuild:Env:Qul_DIR}/" +
                                   board->toolChainFile()).toUtf8()));
    CMakeConfigurationKitAspect::setConfiguration(k, config);
}

ProjectExplorer::Kit *McuSupportOptions::kit(const BoardOptions* board)
{
    using namespace ProjectExplorer;

    Kit *kit = KitManager::kit([board](const Kit *k){
        return board->model() == k->value(Constants::KIT_BOARD_MODEL_KEY).toString();
    });
    if (!kit) {
        const QString armGccPath = toolchainPackage->path();
        const auto init = [board, &armGccPath](Kit *k) {
            KitGuard kitGuard(k);

            setKitProperties(k, board);
            setKitToolchains(k, armGccPath);
            setKitDebugger(k, armGccPath);
            setKitDevice(k);
            setKitEnvironment(k, board);
            setKitCMakeOptions(k, board);

            k->setup();
            k->fix();
        };
        kit = KitManager::registerKit(init);
    }
    return kit;
}

} // Internal
} // McuSupport
