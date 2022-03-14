/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
#include "mcusupportversiondetection.h"
#include "mcusupportsdk.h"

#include <baremetal/baremetalconstants.h>
#include <coreplugin/icore.h>
#include <utils/algorithm.h>
#include <utils/infolabel.h>
#include <utils/pathchooser.h>
#include <utils/utilsicons.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>
#include <debugger/debuggeritem.h>
#include <debugger/debuggeritemmanager.h>

#include <QDesktopServices>
#include <QGridLayout>
#include <QToolButton>

using namespace ProjectExplorer;
using namespace Utils;

namespace McuSupport::Internal {

McuPackage::McuPackage(const QString &label,
                       const FilePath &defaultPath,
                       const FilePath &detectionPath,
                       const QString &settingsKey,
                       const QString &envVarName,
                       const QString &downloadUrl,
                       const McuPackageVersionDetector *versionDetector,
                       const bool addToSystemPath,
                       const FilePath &relativePathModifier)
    : m_label(label)
    , m_defaultPath(Sdk::packagePathFromSettings(settingsKey, QSettings::SystemScope, defaultPath))
    , m_detectionPath(detectionPath)
    , m_settingsKey(settingsKey)
    , m_versionDetector(versionDetector)
    , m_relativePathModifier(relativePathModifier)
    , m_environmentVariableName(envVarName)
    , m_downloadUrl(downloadUrl)
    , m_addToSystemPath(addToSystemPath)
{
    m_path = Sdk::packagePathFromSettings(settingsKey, QSettings::UserScope, m_defaultPath);
}

QString McuPackage::label() const
{
    return m_label;
}

const QString &McuPackage::environmentVariableName() const
{
    return m_environmentVariableName;
}

bool McuPackage::isAddToSystemPath() const
{
    return m_addToSystemPath;
}

void McuPackage::setVersions(const QStringList &versions)
{
    m_versions = versions;
}

FilePath McuPackage::basePath() const
{
    return m_fileChooser != nullptr ? m_fileChooser->filePath() : m_path;
}

FilePath McuPackage::path() const
{
    return basePath().pathAppended(m_relativePathModifier.path()).absoluteFilePath();
}

FilePath McuPackage::defaultPath() const
{
    return m_defaultPath;
}

FilePath McuPackage::detectionPath() const
{
    return m_detectionPath;
}

void McuPackage::updatePath()
{
    m_path = m_fileChooser->rawFilePath();
    m_fileChooser->lineEdit()->button(FancyLineEdit::Right)->setEnabled(m_path != m_defaultPath);
    updateStatus();
}

void McuPackage::updateStatus()
{
    bool validPath = !m_path.isEmpty() && m_path.exists();
    const FilePath detectionPath = basePath().pathAppended(m_detectionPath.path());
    const bool validPackage = m_detectionPath.isEmpty() || detectionPath.exists();
    m_detectedVersion = validPath && validPackage && m_versionDetector
                            ? m_versionDetector->parseVersion(basePath().toString())
                            : QString();
    const bool validVersion = m_detectedVersion.isEmpty() || m_versions.isEmpty()
                              || m_versions.contains(m_detectedVersion);

    m_status = validPath          ? (validPackage ? (validVersion ? Status::ValidPackage
                                                                  : Status::ValidPackageMismatchedVersion)
                                                  : Status::ValidPathInvalidPackage)
               : m_path.isEmpty() ? Status::EmptyPath
                                  : Status::InvalidPath;

    emit statusChanged();
}

McuPackage::Status McuPackage::status() const
{
    return m_status;
}

bool McuPackage::isValidStatus() const
{
    return m_status == Status::ValidPackage || m_status == Status::ValidPackageMismatchedVersion;
}


void McuPackage::updateStatusUi()
{
    switch (m_status) {
    case Status::ValidPackage:
        m_infoLabel->setType(InfoLabel::Ok);
        break;
    case Status::ValidPackageMismatchedVersion:
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
    const QString displayVersions = m_versions.join(" or ");
    const QString outDetectionPath = m_detectionPath.toUserOutput();
    const QString displayRequiredPath = m_versions.empty() ? outDetectionPath
                                                           : QString("%1 %2").arg(outDetectionPath,
                                                                                  displayVersions);
    const QString displayDetectedPath = m_versions.empty()
                                            ? outDetectionPath
                                            : QString("%1 %2").arg(outDetectionPath,
                                                                   m_detectedVersion);

    QString response;
    switch (m_status) {
    case Status::ValidPackage:
        response = m_detectionPath.isEmpty()
                       ? (m_detectedVersion.isEmpty()
                              ? tr("Path %1 exists.").arg(displayPackagePath)
                              : tr("Path %1 exists. Version %2 was found.")
                                    .arg(displayPackagePath, m_detectedVersion))
                       : tr("Path %1 is valid, %2 was found.")
                             .arg(displayPackagePath, displayDetectedPath);
        break;
    case Status::ValidPackageMismatchedVersion: {
        const QString versionWarning
            = m_versions.size() == 1
                  ? tr("but only version %1 is supported").arg(m_versions.first())
                  : tr("but only versions %1 are supported").arg(displayVersions);
        response = tr("Path %1 is valid, %2 was found, %3.")
                       .arg(displayPackagePath, displayDetectedPath, versionWarning);
        break;
    }
    case Status::ValidPathInvalidPackage:
        response = tr("Path %1 exists, but does not contain %2.")
                       .arg(displayPackagePath, displayRequiredPath);
        break;
    case Status::InvalidPath:
        response = tr("Path %1 does not exist.").arg(displayPackagePath);
        break;
    case Status::EmptyPath:
        response = m_detectionPath.isEmpty()
                       ? tr("Path is empty.")
                       : tr("Path is empty, %1 not found.").arg(displayRequiredPath);
        break;
    }
    return response;
}

bool McuPackage::writeToSettings() const
{
    const FilePath savedPath = Sdk::packagePathFromSettings(m_settingsKey,
                                                            QSettings::UserScope,
                                                            m_defaultPath);
    const QString key = QLatin1String(Constants::SETTINGS_GROUP) + '/'
                        + QLatin1String(Constants::SETTINGS_KEY_PACKAGE_PREFIX) + m_settingsKey;
    Core::ICore::settings()->setValueWithDefault(key, m_path.toString(), m_defaultPath.toString());

    return savedPath != m_path;
}

QWidget *McuPackage::widget()
{
    if (m_widget)
        return m_widget;

    m_widget = new QWidget;
    m_fileChooser = new PathChooser;
    m_fileChooser->lineEdit()->setButtonIcon(FancyLineEdit::Right, Icons::RESET.icon());
    m_fileChooser->lineEdit()->setButtonVisible(FancyLineEdit::Right, true);
    connect(m_fileChooser->lineEdit(), &FancyLineEdit::rightButtonClicked, this, [&] {
        m_fileChooser->setFilePath(m_defaultPath);
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

    m_fileChooser->setFilePath(m_path);

    QObject::connect(this, &McuPackage::statusChanged, this, [this] { updateStatusUi(); });

    QObject::connect(m_fileChooser, &PathChooser::pathChanged, this, [this] {
        updatePath();
        emit changed();
    });

    updateStatus();
    return m_widget;
}


McuToolChainPackage::McuToolChainPackage(const QString &label,
                                         const FilePath &defaultPath,
                                         const FilePath &detectionPath,
                                         const QString &settingsKey,
                                         McuToolChainPackage::ToolChainType type,
                                         const QString &envVarName,
                                         const McuPackageVersionDetector *versionDetector)
    : McuPackage(label, defaultPath, detectionPath, settingsKey, envVarName, {}, versionDetector)
    , m_type(type)
{}

McuToolChainPackage::ToolChainType McuToolChainPackage::toolchainType() const
{
    return m_type;
}

bool McuToolChainPackage::isDesktopToolchain() const
{
    return m_type == ToolChainType::MSVC || m_type == ToolChainType::GCC;
}

static ToolChain *msvcToolChain(Id language)
{
    ToolChain *toolChain = ToolChainManager::toolChain([language](const ToolChain *t) {
        const Abi abi = t->targetAbi();
        // TODO: Should Abi::WindowsMsvc2022Flavor be added too?
        return (abi.osFlavor() == Abi::WindowsMsvc2017Flavor
                || abi.osFlavor() == Abi::WindowsMsvc2019Flavor)
               && abi.architecture() == Abi::X86Architecture && abi.wordWidth() == 64
               && t->language() == language;
    });
    return toolChain;
}

static ToolChain *gccToolChain(Id language)
{
    ToolChain *toolChain = ToolChainManager::toolChain([language](const ToolChain *t) {
        const Abi abi = t->targetAbi();
        return abi.os() != Abi::WindowsOS && abi.architecture() == Abi::X86Architecture
               && abi.wordWidth() == 64 && t->language() == language;
    });
    return toolChain;
}

static ToolChain *armGccToolChain(const FilePath &path, Id language)
{
    ToolChain *toolChain = ToolChainManager::toolChain([&path, language](const ToolChain *t) {
        return t->compilerCommand() == path && t->language() == language;
    });
    if (!toolChain) {
        ToolChainFactory *gccFactory
            = Utils::findOrDefault(ToolChainFactory::allToolChainFactories(),
                                   [](ToolChainFactory *f) {
                                       return f->supportedToolChainType()
                                              == ProjectExplorer::Constants::GCC_TOOLCHAIN_TYPEID;
                                   });
        if (gccFactory) {
            const QList<ToolChain *> detected = gccFactory->detectForImport({path, language});
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
    ToolChain *toolChain = ToolChainManager::toolChain([language](const ToolChain *t) {
        return t->typeId() == BareMetal::Constants::IAREW_TOOLCHAIN_TYPEID
               && t->language() == language;
    });
    if (!toolChain) {
        ToolChainFactory *iarFactory
            = Utils::findOrDefault(ToolChainFactory::allToolChainFactories(),
                                   [](ToolChainFactory *f) {
                                       return f->supportedToolChainType()
                                              == BareMetal::Constants::IAREW_TOOLCHAIN_TYPEID;
                                   });
        if (iarFactory) {
            Toolchains detected = iarFactory->autoDetect(ToolchainDetector({}, {}, {}));
            if (detected.isEmpty())
                detected = iarFactory->detectForImport({path, language});
            for (auto tc : detected) {
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
    switch (m_type) {
    case ToolChainType::MSVC:
        return msvcToolChain(language);
    case ToolChainType::GCC:
        return gccToolChain(language);
    case ToolChainType::IAR: {
        const FilePath compiler = path().pathAppended("/bin/iccarm").withExecutableSuffix();
        return iarToolChain(compiler, language);
    }
    case ToolChainType::ArmGcc:
    case ToolChainType::KEIL:
    case ToolChainType::GHS:
    case ToolChainType::GHSArm:
    case ToolChainType::Unsupported: {
        const QLatin1String compilerName(
            language == ProjectExplorer::Constants::C_LANGUAGE_ID ? "gcc" : "g++");
        const QString comp = QLatin1String(m_type == ToolChainType::ArmGcc ? "/bin/arm-none-eabi-%1"
                                                                  : "/bar/foo-keil-%1")
                                 .arg(compilerName);
        const FilePath compiler = path().pathAppended(comp).withExecutableSuffix();

        return armGccToolChain(compiler, language);
    }
    default:
        Q_UNREACHABLE();
    }
}

QString McuToolChainPackage::toolChainName() const
{
    switch (m_type) {
    case ToolChainType::ArmGcc:
        return QLatin1String("armgcc");
    case ToolChainType::IAR:
        return QLatin1String("iar");
    case ToolChainType::KEIL:
        return QLatin1String("keil");
    case ToolChainType::GHS:
        return QLatin1String("ghs");
    case ToolChainType::GHSArm:
        return QLatin1String("ghs-arm");
    default:
        return QLatin1String("unsupported");
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
    case ToolChainType::ArmGcc: {
        sub = QString::fromLatin1("bin/arm-none-eabi-gdb-py");
        displayName = McuPackage::tr("Arm GDB at %1");
        engineType = Debugger::GdbEngineType;
        break;
    }
    case ToolChainType::IAR: {
        sub = QString::fromLatin1("../common/bin/CSpyBat");
        displayName = QLatin1String("CSpy");
        engineType = Debugger::NoEngineType; // support for IAR missing
        break;
    }
    case ToolChainType::KEIL: {
        sub = QString::fromLatin1("UV4/UV4");
        displayName = QLatin1String("KEIL uVision Debugger");
        engineType = Debugger::UvscEngineType;
        break;
    }
    default:
        return QVariant();
    }

    const FilePath command = path().pathAppended(sub).withExecutableSuffix();
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
