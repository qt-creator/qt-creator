// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcupackage.h"
#include "mcuhelpers.h"
#include "mcusupporttr.h"
#include "mcusupportversiondetection.h"
#include "settingshandler.h"

#include <baremetal/baremetalconstants.h>
#include <coreplugin/icore.h>
#include <debugger/debuggeritem.h>
#include <debugger/debuggeritemmanager.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>
#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/infolabel.h>
#include <utils/pathchooser.h>
#include <utils/stringutils.h>
#include <utils/utilsicons.h>

#include <QDesktopServices>
#include <QGridLayout>
#include <QToolButton>

using namespace ProjectExplorer;
using namespace Utils;

namespace McuSupport::Internal {

McuPackage::McuPackage(const SettingsHandler::Ptr &settingsHandler,
                       const QString &label,
                       const FilePath &defaultPath,
                       const Utils::FilePaths &detectionPaths,
                       const Key &settingsKey,
                       const QString &cmakeVarName,
                       const QString &envVarName,
                       const QStringList &versions,
                       const QString &downloadUrl,
                       const McuPackageVersionDetector *versionDetector,
                       const bool addToSystemPath,
                       const Utils::PathChooser::Kind &valueType,
                       const bool allowNewerVersionKey)
    : settingsHandler(settingsHandler)
    , m_label(label)
    , m_detectionPaths(detectionPaths)
    , m_settingsKey(settingsKey)
    , m_versionDetector(versionDetector)
    , m_versions(versions)
    , m_cmakeVariableName(cmakeVarName)
    , m_environmentVariableName(envVarName)
    , m_downloadUrl(downloadUrl)
    , m_addToSystemPath(addToSystemPath)
    , m_valueType(valueType)
{
    // The installer writes versioned keys as well as the plain key as found in the kits.
    // Use the versioned key in case the plain key was removed by an uninstall operation
    const Utils::Key versionedKey = settingsHandler->getVersionedKey(settingsKey,
                                                                     QSettings::SystemScope,
                                                                     versions,
                                                                     allowNewerVersionKey);
    m_defaultPath = settingsHandler->getPath(versionedKey, QSettings::SystemScope, defaultPath);
    m_path = settingsHandler->getPath(m_settingsKey, QSettings::UserScope, "");
    // The user settings may have been written with a versioned key in older versions of QtCreator
    if (m_path.isEmpty()) {
        m_path = settingsHandler->getPath(versionedKey, QSettings::UserScope, m_defaultPath);
    }
    if (m_path.isEmpty()) {
        m_path = FilePath::fromUserInput(qtcEnvironmentVariable(m_environmentVariableName));
    }
}

QString McuPackage::label() const
{
    return m_label;
}

Key McuPackage::settingsKey() const
{
    return m_settingsKey;
}

QString McuPackage::cmakeVariableName() const
{
    return m_cmakeVariableName;
}

QString McuPackage::environmentVariableName() const
{
    return m_environmentVariableName;
}

bool McuPackage::isAddToSystemPath() const
{
    return m_addToSystemPath;
}

QStringList McuPackage::versions() const
{
    return m_versions;
}

const McuPackageVersionDetector *McuPackage::getVersionDetector() const
{
    return m_versionDetector.get();
}

FilePath McuPackage::basePath() const
{
    return m_path;
}

FilePath McuPackage::path() const
{
    return basePath().cleanPath();
}

FilePath McuPackage::defaultPath() const
{
    return m_defaultPath.cleanPath();
}

QList<FilePath> McuPackage::detectionPaths() const
{
    return m_detectionPaths;
}

QString McuPackage::detectionPathsToString() const
{
    return joinStrings(transform(m_detectionPaths,
                                 [](const FilePath &filePath) { return filePath.toUserOutput(); }),
                       '|');
}

void McuPackage::setPath(const FilePath &newPath)
{
    if (m_path == newPath)
        return;

    m_path = newPath;
    updateStatus();
    emit changed();
}

void McuPackage::updateStatus()
{
    bool validPath = !m_path.isEmpty() && m_path.exists();
    bool validPackage = false;
    if (m_detectionPaths.empty()) {
        validPackage = true;
    } else {
        for (const FilePath &detectionPath : m_detectionPaths) {
            std::optional<FilePath> alternativeDetectionPath = firstMatchingPath(
                basePath() / detectionPath.path());
            if (!alternativeDetectionPath)
                continue;
            validPackage = true;
            m_usedDetectionPath = detectionPath;
            break;
        }
    }
    m_detectedVersion = validPath && validPackage && m_versionDetector
                            ? m_versionDetector->parseVersion(basePath())
                            : QString();

    const bool validVersion = m_versions.isEmpty() || m_versions.contains(m_detectedVersion);

    if (m_path.isEmpty()) {
        m_status = Status::EmptyPath;
    } else if (!validPath) {
        m_status = Status::InvalidPath;
    } else if (!validPackage) {
        m_status = Status::ValidPathInvalidPackage;
    } else if (m_versionDetector && m_detectedVersion.isEmpty()) {
        m_status = Status::ValidPackageVersionNotDetected;
    } else if (m_versionDetector && !validVersion) {
        m_status = Status::ValidPackageMismatchedVersion;
    } else {
        m_status = Status::ValidPackage;
    }

    emit statusChanged();
}

McuPackage::Status McuPackage::status() const
{
    return m_status;
}

bool McuPackage::isValidStatus() const
{
    return m_status == Status::ValidPackage || m_status == Status::ValidPackageMismatchedVersion
           || m_status == Status::ValidPackageVersionNotDetected;
}

void McuPackage::updateStatusUi()
{
    switch (m_status) {
    case Status::ValidPackage:
        m_infoLabel->setType(InfoLabel::Ok);
        break;
    case Status::ValidPackageMismatchedVersion:
    case Status::ValidPackageVersionNotDetected:
        m_infoLabel->setType(InfoLabel::Warning);
        break;
    default:
        m_infoLabel->setType(InfoLabel::NotOk);
        break;
    }
    m_infoLabel->setText(statusText());
}

QString McuPackage::statusText() const
{
    const QString displayPackagePath = m_path.toUserOutput();
    const QString displayVersions = m_versions.join(Tr::tr(" or "));
    const QStringList detectionPathsAsString = Utils::transform(m_detectionPaths, &FilePath::toUserOutput);
    const QString outDetectionPath = joinStrings(detectionPathsAsString, '|');
    const QString displayRequiredPath = m_versions.empty() ? outDetectionPath
                                                           : QString("%1 %2").arg(outDetectionPath,
                                                                                  displayVersions);
    const QString displayDetectedPath = m_versions.empty()
                                            ? m_usedDetectionPath.toString()
                                            : QString("%1 %2").arg(m_usedDetectionPath.toString(),
                                                                   m_detectedVersion);

    QString response;
    switch (m_status) {
    case Status::ValidPackage:
        response = m_detectionPaths.isEmpty()
                       ? (m_detectedVersion.isEmpty()
                              ? Tr::tr("Path %1 exists.").arg(displayPackagePath)
                              : Tr::tr("Path %1 exists. Version %2 was found.")
                                    .arg(displayPackagePath, m_detectedVersion))
                       : Tr::tr("Path %1 is valid, %2 was found.")
                             .arg(displayPackagePath, displayDetectedPath);
        break;
    case Status::ValidPackageMismatchedVersion: {
        const QString versionWarning
            = m_versions.size() == 1
                  ? Tr::tr("but only version %1 is supported").arg(m_versions.first())
                  : Tr::tr("but only versions %1 are supported").arg(displayVersions);
        response = Tr::tr("Path %1 is valid, %2 was found, %3.")
                       .arg(displayPackagePath, displayDetectedPath, versionWarning);
        break;
    }
    case Status::ValidPathInvalidPackage:
        response = Tr::tr("Path %1 exists, but does not contain %2.")
                       .arg(displayPackagePath, displayRequiredPath);
        break;
    case Status::InvalidPath:
        response = Tr::tr("Path %1 does not exist.").arg(displayPackagePath);
        break;
    case Status::EmptyPath:
        response = m_detectionPaths.isEmpty()
                       ? Tr::tr("Path is empty.")
                       : Tr::tr("Path is empty, %1 not found.").arg(displayRequiredPath);
        break;
    case Status::ValidPackageVersionNotDetected:
        response = Tr::tr("Path %1 exists, but version %2 could not be detected.")
                       .arg(displayPackagePath, displayVersions);
        break;
    }
    return response;
}

bool McuPackage::writeToSettings() const
{
    if (m_settingsKey.isEmpty()) {
        // Writing with an empty settings key will result in multiple packages writing their value
        // in the same key "Package_", with the suffix missing, overwriting each other.
        return false;
    }

    return settingsHandler->write(m_settingsKey, m_path, m_defaultPath);
}

void McuPackage::readFromSettings()
{
    setPath(settingsHandler->getPath(m_settingsKey, QSettings::UserScope, m_defaultPath));
}

QWidget *McuPackage::widget()
{
    auto *widget = new QWidget;
    m_fileChooser = new PathChooser(widget);
    m_fileChooser->setExpectedKind(m_valueType);
    m_fileChooser->lineEdit()->setButtonIcon(FancyLineEdit::Right, Icons::RESET.icon());
    m_fileChooser->lineEdit()->setButtonVisible(FancyLineEdit::Right, true);
    connect(m_fileChooser->lineEdit(), &FancyLineEdit::rightButtonClicked, this, &McuPackage::reset);

    auto layout = new QGridLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    m_infoLabel = new InfoLabel(widget);

    if (!m_downloadUrl.isEmpty()) {
        auto downLoadButton = new QToolButton(widget);
        downLoadButton->setIcon(Icons::ONLINE.icon());
        downLoadButton->setToolTip(Tr::tr("Download from \"%1\"").arg(m_downloadUrl));
        QObject::connect(downLoadButton, &QToolButton::pressed, this, [this] {
            QDesktopServices::openUrl(m_downloadUrl);
        });
        layout->addWidget(downLoadButton, 0, 2);
    }

    layout->addWidget(m_fileChooser, 0, 0, 1, 2);
    layout->addWidget(m_infoLabel, 1, 0, 1, -1);

    m_fileChooser->setFilePath(m_path);

    QObject::connect(this, &McuPackage::statusChanged, widget, [this] { updateStatusUi(); });

    QObject::connect(m_fileChooser, &PathChooser::textChanged, this, [this] {
        setPath(m_fileChooser->unexpandedFilePath());
    });

    connect(this, &McuPackage::changed, m_fileChooser, [this] {
        m_fileChooser->lineEdit()->button(FancyLineEdit::Right)->setEnabled(m_path != m_defaultPath);
        m_fileChooser->setFilePath(m_path);
    });

    updateStatus();
    return widget;
}

const QMap<QString, QString> McuPackage::packageLabelTranslations {
    //Board SDKs
    {"Board SDK for MIMXRT1050-EVK",                        Tr::tr("Board SDK for MIMXRT1050-EVK")},
    {"Board SDK MIMXRT1060-EVK",                            Tr::tr("Board SDK MIMXRT1060-EVK")},
    {"Board SDK for MIMXRT1060-EVK",                        Tr::tr("Board SDK for MIMXRT1060-EVK")},
    {"Board SDK for MIMXRT1064-EVK",                        Tr::tr("Board SDK for MIMXRT1064-EVK")},
    {"Board SDK for MIMXRT1170-EVK",                        Tr::tr("Board SDK for MIMXRT1170-EVK")},
    {"Board SDK for STM32F469I-Discovery",                  Tr::tr("Board SDK for STM32F469I-Discovery")},
    {"Board SDK for STM32F769I-Discovery",                  Tr::tr("Board SDK for STM32F769I-Discovery")},
    {"Board SDK for STM32H750B-Discovery",                  Tr::tr("Board SDK for STM32H750B-Discovery")},
    {"Board SDK",                                           Tr::tr("Board SDK")},
    {"Flexible Software Package for Renesas RA MCU Family", Tr::tr("Flexible Software Package for Renesas RA MCU Family")},
    {"Graphics Driver for Traveo II Cluster Series",        Tr::tr("Graphics Driver for Traveo II Cluster Series")},
    {"Renesas Graphics Library",                            Tr::tr("Renesas Graphics Library")},
    //Flashing tools
    {"Cypress Auto Flash Utility",                          Tr::tr("Cypress Auto Flash Utility")},
    {"MCUXpresso IDE",                                      Tr::tr("MCUXpresso IDE")},
    {"Path to SEGGER J-Link",                               Tr::tr("Path to SEGGER J-Link")},
    {"Path to Renesas Flash Programmer",                    Tr::tr("Path to Renesas Flash Programmer")},
    {"STM32CubeProgrammer",                                 Tr::tr("STM32CubeProgrammer")},
    //Compilers/Toolchains
    {"Green Hills Compiler for ARM",                        Tr::tr("Green Hills Compiler for ARM")},
    {"IAR ARM Compiler",                                    Tr::tr("IAR ARM Compiler")},
    {"Green Hills Compiler",                                Tr::tr("Green Hills Compiler")},
    {"GNU Arm Embedded Toolchain",                          Tr::tr("GNU Arm Embedded Toolchain")},
    {"GNU Toolchain",                                       Tr::tr("GNU Toolchain")},
    {"MSVC Toolchain",                                      Tr::tr("MSVC Toolchain")},
    //FreeRTOS
    {"FreeRTOS SDK for MIMXRT1050-EVK",                     Tr::tr("FreeRTOS SDK for MIMXRT1050-EVK")},
    {"FreeRTOS SDK for MIMXRT1064-EVK",                     Tr::tr("FreeRTOS SDK for MIMXRT1064-EVK")},
    {"FreeRTOS SDK for MIMXRT1170-EVK",                     Tr::tr("FreeRTOS SDK for MIMXRT1170-EVK")},
    {"FreeRTOS SDK for EK-RA6M3G",                          Tr::tr("FreeRTOS SDK for EK-RA6M3G")},
    {"FreeRTOS SDK for STM32F769I-Discovery",               Tr::tr("FreeRTOS SDK for STM32F769I-Discovery")},
    //Other
    {"Path to project for Renesas e2 Studio",               Tr::tr("Path to project for Renesas e2 Studio")}
};

McuToolchainPackage::McuToolchainPackage(const SettingsHandler::Ptr &settingsHandler,
                                         const QString &label,
                                         const FilePath &defaultPath,
                                         const QList<FilePath> &detectionPaths,
                                         const Key &settingsKey,
                                         McuToolchainPackage::ToolchainType type,
                                         const QStringList &versions,
                                         const QString &cmakeVarName,
                                         const QString &envVarName,
                                         const McuPackageVersionDetector *versionDetector)
    : McuPackage(settingsHandler,
                 label,
                 defaultPath,
                 detectionPaths,
                 settingsKey,
                 cmakeVarName,
                 envVarName,
                 versions,
                 {}, // url
                 versionDetector)
    , m_type(type)
{}

McuToolchainPackage::ToolchainType McuToolchainPackage::toolchainType() const
{
    return m_type;
}

bool McuToolchainPackage::isDesktopToolchain() const
{
    return m_type == ToolchainType::MSVC || m_type == ToolchainType::GCC
           || m_type == ToolchainType::MinGW;
}

Toolchain *McuToolchainPackage::msvcToolchain(Id language)
{
    Toolchain *toolChain = ToolchainManager::toolchain([language](const Toolchain *t) {
        const Abi abi = t->targetAbi();
        return abi.osFlavor() == Abi::WindowsMsvc2019Flavor
               && abi.architecture() == Abi::X86Architecture && abi.wordWidth() == 64
               && t->typeId() == ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID
               && t->language() == language;
    });
    return toolChain;
}

Toolchain *McuToolchainPackage::gccToolchain(Id language)
{
    Toolchain *toolChain = ToolchainManager::toolchain([language](const Toolchain *t) {
        const Abi abi = t->targetAbi();
        return abi.os() != Abi::WindowsOS && abi.architecture() == Abi::X86Architecture
               && abi.wordWidth() == 64 && t->language() == language;
    });
    return toolChain;
}

static Toolchain *mingwToolchain(const FilePath &path, Id language)
{
    Toolchain *toolChain = ToolchainManager::toolchain([&path, language](const Toolchain *t) {
        // find a MinGW toolchain having the same path from registered toolchains
        const Abi abi = t->targetAbi();
        return t->typeId() == ProjectExplorer::Constants::MINGW_TOOLCHAIN_TYPEID
               && abi.architecture() == Abi::X86Architecture && abi.wordWidth() == 64
               && t->language() == language && t->compilerCommand() == path;
    });
    if (!toolChain) {
        // if there's no MinGW toolchain having the same path,
        // a proper MinGW would be selected from the registered toolchains.
        toolChain = ToolchainManager::toolchain([language](const Toolchain *t) {
            const Abi abi = t->targetAbi();
            return t->typeId() == ProjectExplorer::Constants::MINGW_TOOLCHAIN_TYPEID
                   && abi.architecture() == Abi::X86Architecture && abi.wordWidth() == 64
                   && t->language() == language;
        });
    }
    return toolChain;
}

// FIXME: Do not register languages separately.
static Toolchain *armGccToolchain(const FilePath &path, Id language)
{
    Toolchain *toolchain = ToolchainManager::toolchain([&path, language](const Toolchain *t) {
        return t->compilerCommand() == path && t->language() == language;
    });
    if (!toolchain) {
        if (ToolchainFactory * const gccFactory = ToolchainFactory::factoryForType(
                ProjectExplorer::Constants::GCC_TOOLCHAIN_TYPEID)) {
            QList<Toolchain *> detected = gccFactory->detectForImport({path, language});
            if (!detected.isEmpty()) {
                toolchain = detected.takeFirst();
                ToolchainManager::registerToolchains({toolchain});
                toolchain->setDetection(Toolchain::ManualDetection);
                toolchain->setDisplayName("Arm GCC");
                qDeleteAll(detected);
            }
        }
    }

    return toolchain;
}

static Toolchain *iarToolchain(const FilePath &path, Id language)
{
    Toolchain *toolchain = ToolchainManager::toolchain([language](const Toolchain *t) {
        return t->typeId() == BareMetal::Constants::IAREW_TOOLCHAIN_TYPEID
               && t->language() == language;
    });
    if (!toolchain) {
        if (ToolchainFactory * const iarFactory = ToolchainFactory::factoryForType(
                BareMetal::Constants::IAREW_TOOLCHAIN_TYPEID)) {
            Toolchains detected = iarFactory->autoDetect(
                {{}, DeviceManager::defaultDesktopDevice(), {}});
            if (detected.isEmpty())
                detected = iarFactory->detectForImport({path, language});
            Toolchains toRegister;
            Toolchains toDelete;
            std::tie(toRegister, toDelete)
                = Utils::partition(detected, Utils::equal(&Toolchain::language, language));
            for (Toolchain * const tc : toRegister) {
                tc->setDetection(Toolchain::ManualDetection);
                tc->setDisplayName("IAREW");
            }
            ToolchainManager::registerToolchains(toRegister);
            qDeleteAll(toDelete);
        }
    }

    return toolchain;
}

Toolchain *McuToolchainPackage::toolChain(Id language) const
{
    switch (m_type) {
    case ToolchainType::MSVC:
        return msvcToolchain(language);
    case ToolchainType::GCC:
        return gccToolchain(language);
    case ToolchainType::MinGW: {
        const QLatin1String compilerName(
            language == ProjectExplorer::Constants::C_LANGUAGE_ID ? "gcc" : "g++");
        const FilePath compilerPath = (path() / "bin" / compilerName).withExecutableSuffix();
        return mingwToolchain(compilerPath, language);
    }
    case ToolchainType::IAR: {
        const FilePath compiler = (path() / "/bin/iccarm").withExecutableSuffix();
        return iarToolchain(compiler, language);
    }
    case ToolchainType::ArmGcc:
    case ToolchainType::KEIL:
    case ToolchainType::GHS:
    case ToolchainType::GHSArm:
    case ToolchainType::Unsupported: {
        const QLatin1String compilerName(
            language == ProjectExplorer::Constants::C_LANGUAGE_ID ? "gcc" : "g++");
        const QString comp = QLatin1String(m_type == ToolchainType::ArmGcc ? "/bin/arm-none-eabi-%1"
                                                                           : "/bar/foo-keil-%1")
                                 .arg(compilerName);
        const FilePath compiler = (path() / comp).withExecutableSuffix();

        return armGccToolchain(compiler, language);
    }
    default:
        Q_UNREACHABLE();
    }
}

QString McuToolchainPackage::toolchainName() const
{
    switch (m_type) {
    case ToolchainType::MSVC:
        return QLatin1String("msvc");
    case ToolchainType::GCC:
        return QLatin1String("gcc");
    case ToolchainType::MinGW:
        return QLatin1String("mingw");
    case ToolchainType::ArmGcc:
        return QLatin1String("armgcc");
    case ToolchainType::IAR:
        return QLatin1String("iar");
    case ToolchainType::KEIL:
        return QLatin1String("keil");
    case ToolchainType::GHS:
        return QLatin1String("ghs");
    case ToolchainType::GHSArm:
        return QLatin1String("ghs-arm");
    default:
        return QLatin1String("unsupported");
    }
}

QVariant McuToolchainPackage::debuggerId() const
{
    using namespace Debugger;

    QString sub, displayName;
    DebuggerEngineType engineType;

    switch (m_type) {
    case ToolchainType::ArmGcc: {
        sub = QString::fromLatin1("bin/arm-none-eabi-gdb-py");
        const FilePath command = (path() / sub).withExecutableSuffix();
        if (!command.exists()) {
            sub = QString::fromLatin1("bin/arm-none-eabi-gdb");
        }
        displayName = Tr::tr("Arm GDB at %1");
        engineType = Debugger::GdbEngineType;
        break;
    }
    case ToolchainType::IAR: {
        sub = QString::fromLatin1("../common/bin/CSpyBat");
        displayName = QLatin1String("CSpy");
        engineType = Debugger::NoEngineType; // support for IAR missing
        break;
    }
    case ToolchainType::KEIL: {
        sub = QString::fromLatin1("UV4/UV4");
        displayName = QLatin1String("KEIL uVision Debugger");
        engineType = Debugger::UvscEngineType;
        break;
    }
    default:
        return QVariant();
    }

    const FilePath command = (path() / sub).withExecutableSuffix();
    if (const DebuggerItem *debugger = DebuggerItemManager::findByCommand(command)) {
        return debugger->id();
    }

    DebuggerItem newDebugger;
    newDebugger.setCommand(command);
    newDebugger.setUnexpandedDisplayName(displayName.arg(command.toUserOutput()));
    newDebugger.setEngineType(engineType);
    return DebuggerItemManager::registerDebugger(newDebugger);
}

} // namespace McuSupport::Internal
