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

#include "qnxconfiguration.h"
#include "qnxqtversion.h"
#include "qnxutils.h"
#include "qnxtoolchain.h"

#include "debugger/debuggeritem.h"

#include <coreplugin/icore.h>

#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtversionmanager.h>
#include <qtsupport/qtkitinformation.h>

#include <qmakeprojectmanager/qmakekitinformation.h>

#include <debugger/debuggeritem.h>
#include <debugger/debuggeritemmanager.h>
#include <debugger/debuggerkitinformation.h>

#include <coreplugin/icore.h>

#include <QMessageBox>
#include <QFileInfo>

using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Utils;
using namespace Debugger;

namespace Qnx {
namespace Internal {

const QLatin1String QNXEnvFileKey("EnvFile");
const QLatin1String QNXVersionKey("QNXVersion");
// For backward compatibility
const QLatin1String NDKEnvFileKey("NDKEnvFile");

QnxConfiguration::QnxConfiguration()
{ }

QnxConfiguration::QnxConfiguration(const FileName &sdpEnvFile)
{
    setDefaultConfiguration(sdpEnvFile);
    readInformation();
}

QnxConfiguration::QnxConfiguration(const QVariantMap &data)
{
    QString envFilePath = data.value(QNXEnvFileKey).toString();
    if (envFilePath.isEmpty())
        envFilePath = data.value(NDKEnvFileKey).toString();

    m_version = QnxVersionNumber(data.value(QNXVersionKey).toString());

    setDefaultConfiguration(FileName::fromString(envFilePath));
    readInformation();
}

FileName QnxConfiguration::envFile() const
{
    return m_envFile;
}

FileName QnxConfiguration::qnxTarget() const
{
    return m_qnxTarget;
}

FileName QnxConfiguration::qnxHost() const
{
    return m_qnxHost;
}

FileName QnxConfiguration::qccCompilerPath() const
{
    return m_qccCompiler;
}

FileName QnxConfiguration::armDebuggerPath() const
{
    return m_armlev7Debugger;
}

FileName QnxConfiguration::x86DebuggerPath() const
{
    return m_x86Debugger;
}

QList<EnvironmentItem> QnxConfiguration::qnxEnv() const
{
    return m_qnxEnv;
}

QnxVersionNumber QnxConfiguration::version() const
{
    return m_version;
}

QVariantMap QnxConfiguration::toMap() const
{
    QVariantMap data;
    data.insert(QLatin1String(QNXEnvFileKey), m_envFile.toString());
    data.insert(QLatin1String(QNXVersionKey), m_version.toString());
    return data;
}

bool QnxConfiguration::isValid() const
{
    return !m_qccCompiler.isEmpty()
            && !m_armlev7Debugger.isEmpty()
            && !m_x86Debugger.isEmpty();
}

QString QnxConfiguration::displayName() const
{
    return m_configName;
}

bool QnxConfiguration::activate()
{
    if (isActive())
        return true;

    if (!isValid()) {
        QString errorMessage
                = QCoreApplication::translate("Qnx::Internal::QnxConfiguration",
                                              "The following errors occurred while activating the QNX configuration:");
        foreach (const QString &error, validationErrors())
            errorMessage += QLatin1String("\n") + error;

        QMessageBox::warning(Core::ICore::mainWindow(),
                             QCoreApplication::translate("Qnx::Internal::QnxConfiguration",
                                                         "Cannot Set Up QNX Configuration"),
                             errorMessage, QMessageBox::Ok);
        return false;
    }

    // Create and register toolchain
    QnxToolChain *armTc = createToolChain(ArmLeV7,
                    QCoreApplication::translate("Qnx::Internal::QnxConfiguration",
                                                "QCC for %1 (armv7)").arg(displayName()),
                    sdpPath().toString());
    QnxToolChain *x86Tc = createToolChain(X86,
                    QCoreApplication::translate("Qnx::Internal::QnxConfiguration",
                                                "QCC for %1 (x86)").arg(displayName()),
                    sdpPath().toString());

    // Create and register debuggers
    QVariant armDebuggerId = createDebuggerItem(ArmLeV7,
                       QCoreApplication::translate("Qnx::Internal::QnxConfiguration",
                                                   "Debugger for %1 (armv7)").arg(displayName()));

    QVariant x86DebuggerId = createDebuggerItem(X86,
                       QCoreApplication::translate("Qnx::Internal::QnxConfiguration",
                                                   "Debugger for %1 (x86)").arg(displayName()));

    // Create and register kits
    createKit(ArmLeV7, armTc, armDebuggerId,
              QCoreApplication::translate("Qnx::Internal::QnxConfiguration",
                                          "Kit for %1 (armv7)").arg(displayName()));
    createKit(X86, x86Tc, x86DebuggerId,
              QCoreApplication::translate("Qnx::Internal::QnxConfiguration",
                                          "Kit for %1 (x86)").arg(displayName()));

    return true;
}

void QnxConfiguration::deactivate()
{
    if (!isActive())
        return;

    QList<ToolChain *> toolChainsToRemove;
    QList<DebuggerItem> debuggersToRemove;
    foreach (ToolChain *tc,
             ToolChainManager::toolChains()) {
        if (tc->compilerCommand() == qccCompilerPath())
            toolChainsToRemove.append(tc);
    }

    foreach (DebuggerItem debuggerItem,
             DebuggerItemManager::debuggers()) {
        if (debuggerItem.command() == armDebuggerPath() ||
                debuggerItem.command() == x86DebuggerPath())
            debuggersToRemove.append(debuggerItem);
    }

    foreach (Kit *kit, KitManager::kits()) {
        if (kit->isAutoDetected()
                && DeviceTypeKitInformation::deviceTypeId(kit) == Constants::QNX_QNX_OS_TYPE
                && toolChainsToRemove.contains(ToolChainKitInformation::toolChain(kit)))
            KitManager::deregisterKit(kit);
    }

    foreach (ToolChain *tc, toolChainsToRemove)
        ToolChainManager::deregisterToolChain(tc);

    foreach (DebuggerItem debuggerItem, debuggersToRemove)
        DebuggerItemManager::deregisterDebugger(debuggerItem.id());
}

bool QnxConfiguration::isActive() const
{
    bool hasToolChain = false;
    bool hasDebugger = false;
    foreach (ToolChain *tc, ToolChainManager::toolChains()) {
        if (tc->compilerCommand() == qccCompilerPath()) {
            hasToolChain = true;
            break;
        }
    }

    foreach (DebuggerItem debuggerItem, DebuggerItemManager::debuggers()) {
        if (debuggerItem.command() == armDebuggerPath() ||
                debuggerItem.command() == x86DebuggerPath()) {
            hasDebugger = true;
            break;
        }
    }

    return hasToolChain && hasDebugger;
}

bool QnxConfiguration::canCreateKits() const
{
    return isValid() && (qnxQtVersion(ArmLeV7) || qnxQtVersion(X86));
}

FileName QnxConfiguration::sdpPath() const
{
    return envFile().parentDir();
}

QnxQtVersion* QnxConfiguration::qnxQtVersion(QnxArchitecture arch) const
{
    QnxQtVersion *qnxQt;
    foreach (BaseQtVersion *version,
             QtVersionManager::instance()->versions()) {
        if (version->type() == QLatin1String(Constants::QNX_QNX_QT)) {
            qnxQt = dynamic_cast<QnxQtVersion*>(version);
            if (qnxQt && qnxQt->architecture() == arch) {
                return qnxQt;
            }
        }
    }

    return 0;
}

QVariant QnxConfiguration::createDebuggerItem(QnxArchitecture arch, const QString &displayName)
{
    FileName command = (arch == X86) ? x86DebuggerPath() : armDebuggerPath();
    Debugger::DebuggerItem debugger;
    debugger.setCommand(command);
    debugger.setEngineType(Debugger::GdbEngineType);
    debugger.setAbi(Abi(arch == Qnx::ArmLeV7 ? Abi::ArmArchitecture : Abi::X86Architecture,
                        Abi::LinuxOS, Abi::GenericLinuxFlavor, Abi::ElfFormat, 32));
    debugger.setAutoDetected(true);
    debugger.setUnexpandedDisplayName(displayName);
    return Debugger::DebuggerItemManager::registerDebugger(debugger);
}

QnxToolChain *QnxConfiguration::createToolChain(QnxArchitecture arch, const QString &displayName,
                                                const QString &ndkPath)
{
    QnxToolChain *toolChain = new QnxToolChain(ToolChain::AutoDetection);
    toolChain->resetToolChain(m_qccCompiler);
    toolChain->setTargetAbi(Abi((arch == Qnx::ArmLeV7) ? Abi::ArmArchitecture : Abi::X86Architecture,
                                Abi::LinuxOS, Abi::GenericLinuxFlavor, Abi::ElfFormat, 32));
    toolChain->setDisplayName(displayName);
    toolChain->setNdkPath(ndkPath);
    ToolChainManager::registerToolChain(toolChain);
    return toolChain;
}

Kit *QnxConfiguration::createKit(QnxArchitecture arch,
                                                  QnxToolChain *toolChain,
                                                  const QVariant &debuggerItemId,
                                                  const QString &displayName)
{
    QnxQtVersion *qnxQt = qnxQtVersion(arch);
    // Do not create incomplete kits if no qt qnx version found
    if (!qnxQt)
        return 0;

    Kit *kit = new Kit;

    QtKitInformation::setQtVersion(kit, qnxQt);
    ToolChainKitInformation::setToolChain(kit, toolChain);

    if (debuggerItemId.isValid())
        DebuggerKitInformation::setDebugger(kit, debuggerItemId);

    if (arch == X86) {
        QmakeProjectManager::QmakeKitInformation::setMkspec(
                    kit, FileName::fromLatin1("qnx-x86-qcc"));
    } else {
        if (qnxQt->qtVersion() >= QtVersionNumber(5, 3, 0)) {
            QmakeProjectManager::QmakeKitInformation::setMkspec(
                        kit, FileName::fromLatin1("qnx-armle-v7-qcc"));
        } else {
            QmakeProjectManager::QmakeKitInformation::setMkspec(
                        kit, FileName::fromLatin1("qnx-armv7le-qcc"));
        }
    }

    DeviceTypeKitInformation::setDeviceTypeId(kit, Constants::QNX_QNX_OS_TYPE);
    // TODO: Add sysroot?

    kit->setUnexpandedDisplayName(displayName);
    kit->setIconPath(FileName::fromString(QLatin1String(Constants::QNX_CATEGORY_ICON)));

    kit->setAutoDetected(true);
    kit->setAutoDetectionSource(envFile().toString());
    kit->setMutable(DeviceKitInformation::id(), true);

    kit->setSticky(ToolChainKitInformation::id(), true);
    kit->setSticky(DeviceTypeKitInformation::id(), true);
    kit->setSticky(SysRootKitInformation::id(), true);
    kit->setSticky(DebuggerKitInformation::id(), true);
    kit->setSticky(QmakeProjectManager::QmakeKitInformation::id(), true);

    // add kit with device and qt version not sticky
    KitManager::registerKit(kit);
    return kit;
}

QStringList QnxConfiguration::validationErrors() const
{
    QStringList errorStrings;
    if (m_qccCompiler.isEmpty())
        errorStrings << QCoreApplication::translate("Qnx::Internal::QnxConfiguration",
                                                    "- No GCC compiler found.");

    if (m_armlev7Debugger.isEmpty())
        errorStrings << QCoreApplication::translate("Qnx::Internal::QnxConfiguration",
                                                    "- No GDB debugger found for armvle7.");

    if (m_x86Debugger.isEmpty())
        errorStrings << QCoreApplication::translate("Qnx::Internal::QnxConfiguration",
                                                    "- No GDB debugger found for x86.");

    return errorStrings;
}

void QnxConfiguration::setVersion(const QnxVersionNumber &version)
{
    m_version = version;
}

void QnxConfiguration::readInformation()
{
    QString qConfigPath = sdpPath().toString() + QLatin1String("/.qnx/qconfig");
    QList <ConfigInstallInformation> installInfoList = QnxUtils::installedConfigs(qConfigPath);
    if (installInfoList.isEmpty())
        return;

    // TODO: For now (6.6) it should be one installation file. The code need to handle cases
    // where the SDP support many target/host installations (i.e many installation files).
    const ConfigInstallInformation installInfo = installInfoList.first();
    m_configName = installInfo.name;
    setVersion(QnxVersionNumber(installInfo.version));
}

void QnxConfiguration::setDefaultConfiguration(const Utils::FileName &envScript)
{
    QTC_ASSERT(!envScript.isEmpty(), return);
    m_envFile = envScript;
    m_qnxEnv = QnxUtils::qnxEnvironmentFromEnvFile(m_envFile.toString());
    foreach (const EnvironmentItem &item, m_qnxEnv) {
        if (item.name == QLatin1String("QNX_TARGET"))
            m_qnxTarget = FileName::fromString(item.value);

        else if (item.name == QLatin1String("QNX_HOST"))
            m_qnxHost = FileName::fromString(item.value);
    }

    FileName qccPath = FileName::fromString(HostOsInfo::withExecutableSuffix(
                                                m_qnxHost.toString() + QLatin1String("/usr/bin/qcc")));
    FileName armlev7GdbPath = FileName::fromString(HostOsInfo::withExecutableSuffix(
                                                       m_qnxHost.toString() + QLatin1String("/usr/bin/ntoarm-gdb")));
    if (!armlev7GdbPath.exists()) {
        armlev7GdbPath = FileName::fromString(HostOsInfo::withExecutableSuffix(
                                                  m_qnxHost.toString() + QLatin1String("/usr/bin/ntoarmv7-gdb")));
    }

    FileName x86GdbPath = FileName::fromString(HostOsInfo::withExecutableSuffix(
                                                   m_qnxHost.toString() + QLatin1String("/usr/bin/ntox86-gdb")));

    if (qccPath.exists())
        m_qccCompiler = qccPath;

    if (armlev7GdbPath.exists())
        m_armlev7Debugger = armlev7GdbPath;

    if (x86GdbPath.exists())
        m_x86Debugger = x86GdbPath;
}

} // namespace Internal
} // namespace Qnx
