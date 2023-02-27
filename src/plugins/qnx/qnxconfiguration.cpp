// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qnxconfiguration.h"

#include "qnxqtversion.h"
#include "qnxutils.h"
#include "qnxtoolchain.h"
#include "qnxtr.h"

#include "debugger/debuggeritem.h"

#include <coreplugin/icore.h>

#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtversionmanager.h>
#include <qtsupport/qtkitinformation.h>

#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>

#include <debugger/debuggeritem.h>
#include <debugger/debuggeritemmanager.h>
#include <debugger/debuggerkitinformation.h>

#include <utils/algorithm.h>

#include <QDebug>
#include <QDomDocument>
#include <QMessageBox>

using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Utils;
using namespace Debugger;

namespace Qnx::Internal {

const QLatin1String QNXEnvFileKey("EnvFile");
const QLatin1String QNXVersionKey("QNXVersion");
// For backward compatibility
const QLatin1String SdpEnvFileKey("NDKEnvFile");

const QLatin1String QNXConfiguration("QNX_CONFIGURATION");
const QLatin1String QNXTarget("QNX_TARGET");
const QLatin1String QNXHost("QNX_HOST");

QnxConfiguration::QnxConfiguration() = default;

QnxConfiguration::QnxConfiguration(const FilePath &sdpEnvFile)
{
    setDefaultConfiguration(sdpEnvFile);
    readInformation();
}

QnxConfiguration::QnxConfiguration(const QVariantMap &data)
{
    QString envFilePath = data.value(QNXEnvFileKey).toString();
    if (envFilePath.isEmpty())
        envFilePath = data.value(SdpEnvFileKey).toString();

    m_version = QnxVersionNumber(data.value(QNXVersionKey).toString());

    setDefaultConfiguration(FilePath::fromString(envFilePath));
    readInformation();
}

FilePath QnxConfiguration::envFile() const
{
    return m_envFile;
}

FilePath QnxConfiguration::qnxTarget() const
{
    return m_qnxTarget;
}

FilePath QnxConfiguration::qnxHost() const
{
    return m_qnxHost;
}

FilePath QnxConfiguration::qccCompilerPath() const
{
    return m_qccCompiler;
}

EnvironmentItems QnxConfiguration::qnxEnv() const
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
    return !m_qccCompiler.isEmpty() && !m_targets.isEmpty();
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
        QMessageBox::warning(Core::ICore::dialogParent(), Tr::tr("Cannot Set Up QNX Configuration"),
                             validationErrorMessage(), QMessageBox::Ok);
        return false;
    }

    for (const Target &target : std::as_const(m_targets))
        createTools(target);

    return true;
}

void QnxConfiguration::deactivate()
{
    if (!isActive())
        return;

    const Toolchains toolChainsToRemove =
        ToolChainManager::toolchains(Utils::equal(&ToolChain::compilerCommand, qccCompilerPath()));

    QList<DebuggerItem> debuggersToRemove;
    const QList<DebuggerItem> debuggerItems = DebuggerItemManager::debuggers();
    for (const DebuggerItem &debuggerItem : debuggerItems) {
        if (findTargetByDebuggerPath(debuggerItem.command()))
            debuggersToRemove.append(debuggerItem);
    }

    const QList<Kit *> kits = KitManager::kits();
    for (Kit *kit : kits) {
        if (kit->isAutoDetected()
                && DeviceTypeKitAspect::deviceTypeId(kit) == Constants::QNX_QNX_OS_TYPE
                && toolChainsToRemove.contains(ToolChainKitAspect::cxxToolChain(kit))) {
            KitManager::deregisterKit(kit);
        }
    }

    for (ToolChain *tc : toolChainsToRemove)
        ToolChainManager::deregisterToolChain(tc);

    for (const DebuggerItem &debuggerItem : std::as_const(debuggersToRemove))
        DebuggerItemManager::deregisterDebugger(debuggerItem.id());
}

bool QnxConfiguration::isActive() const
{
    const bool hasToolChain = ToolChainManager::toolChain(Utils::equal(&ToolChain::compilerCommand,
                                                                       qccCompilerPath()));
    const bool hasDebugger = Utils::contains(DebuggerItemManager::debuggers(), [this](const DebuggerItem &di) {
        return findTargetByDebuggerPath(di.command());
    });

    return hasToolChain && hasDebugger;
}

FilePath QnxConfiguration::sdpPath() const
{
    return envFile().parentDir();
}

QnxQtVersion *QnxConfiguration::qnxQtVersion(const Target &target) const
{
    const QtVersions versions = QtVersionManager::versions(
                Utils::equal(&QtVersion::type, QString::fromLatin1(Constants::QNX_QNX_QT)));
    for (QtVersion *version : versions) {
        auto qnxQt = dynamic_cast<QnxQtVersion *>(version);
        if (qnxQt && qnxQt->sdpPath() == sdpPath()) {
            const Abis abis = version->qtAbis();
            for (const Abi &qtAbi : abis) {
                if ((qtAbi == target.m_abi) && (qnxQt->cpuDir() == target.cpuDir()))
                    return qnxQt;
            }
        }
    }
    return nullptr;
}

QList<ToolChain *> QnxConfiguration::autoDetect(const QList<ToolChain *> &alreadyKnown)
{
    QList<ToolChain *> result;

    for (const Target &target : std::as_const(m_targets))
        result += findToolChain(alreadyKnown, target.m_abi);

    return result;
}

void QnxConfiguration::createTools(const Target &target)
{
    QnxToolChainMap toolchainMap = createToolChain(target);
    QVariant debuggerId = createDebugger(target);
    createKit(target, toolchainMap, debuggerId);
}

QVariant QnxConfiguration::createDebugger(const Target &target)
{
    Environment sysEnv = m_qnxHost.deviceEnvironment();
    sysEnv.modify(qnxEnvironmentItems());

    Debugger::DebuggerItem debugger;
    debugger.setCommand(target.m_debuggerPath);
    debugger.reinitializeFromFile(nullptr, &sysEnv);
    debugger.setUnexpandedDisplayName(Tr::tr("Debugger for %1 (%2)")
                .arg(displayName())
                .arg(target.shortDescription()));
    return Debugger::DebuggerItemManager::registerDebugger(debugger);
}

QnxConfiguration::QnxToolChainMap QnxConfiguration::createToolChain(const Target &target)
{
    QnxToolChainMap toolChainMap;

    for (auto language : { ProjectExplorer::Constants::C_LANGUAGE_ID,
                           ProjectExplorer::Constants::CXX_LANGUAGE_ID}) {
        auto toolChain = new QnxToolChain;
        toolChain->setDetection(ToolChain::AutoDetection);
        toolChain->setLanguage(language);
        toolChain->setTargetAbi(target.m_abi);
        toolChain->setDisplayName(Tr::tr("QCC for %1 (%2)")
                    .arg(displayName())
                    .arg(target.shortDescription()));
        toolChain->setSdpPath(sdpPath());
        toolChain->setCpuDir(target.cpuDir());
        toolChain->resetToolChain(qccCompilerPath());
        ToolChainManager::registerToolChain(toolChain);

        toolChainMap.insert({language, toolChain});
    }

    return toolChainMap;
}

QList<ToolChain *> QnxConfiguration::findToolChain(const QList<ToolChain *> &alreadyKnown,
                                                   const Abi &abi)
{
    return Utils::filtered(alreadyKnown, [this, abi](ToolChain *tc) {
                                             return tc->typeId() == Constants::QNX_TOOLCHAIN_ID
                                                 && tc->targetAbi() == abi
                                                 && tc->compilerCommand() == m_qccCompiler;
                                         });
}

void QnxConfiguration::createKit(const Target &target, const QnxToolChainMap &toolChainMap,
                                 const QVariant &debugger)
{
    QnxQtVersion *qnxQt = qnxQtVersion(target); // nullptr is ok.

    const auto init = [&](Kit *k) {
        QtKitAspect::setQtVersion(k, qnxQt);
        ToolChainKitAspect::setToolChain(k, toolChainMap.at(ProjectExplorer::Constants::C_LANGUAGE_ID));
        ToolChainKitAspect::setToolChain(k, toolChainMap.at(ProjectExplorer::Constants::CXX_LANGUAGE_ID));

        if (debugger.isValid())
            DebuggerKitAspect::setDebugger(k, debugger);

        DeviceTypeKitAspect::setDeviceTypeId(k, Constants::QNX_QNX_OS_TYPE);
        // TODO: Add sysroot?

        k->setUnexpandedDisplayName(Tr::tr("Kit for %1 (%2)")
                    .arg(displayName())
                    .arg(target.shortDescription()));

        k->setAutoDetected(true);
        k->setAutoDetectionSource(envFile().toString());
        k->setMutable(DeviceKitAspect::id(), true);

        k->setSticky(ToolChainKitAspect::id(), true);
        k->setSticky(DeviceTypeKitAspect::id(), true);
        k->setSticky(SysRootKitAspect::id(), true);
        k->setSticky(DebuggerKitAspect::id(), true);
        k->setSticky(QmakeProjectManager::Constants::KIT_INFORMATION_ID, true);

        EnvironmentKitAspect::setEnvironmentChanges(k, qnxEnvironmentItems());
    };

    // add kit with device and qt version not sticky
    KitManager::registerKit(init);
}

QString QnxConfiguration::validationErrorMessage() const
{
    if (isValid())
        return {};

    QStringList errorStrings
            = {Tr::tr("The following errors occurred while activating the QNX configuration:")};
    if (m_qccCompiler.isEmpty())
        errorStrings << Tr::tr("- No GCC compiler found.");
    if (m_targets.isEmpty())
        errorStrings << Tr::tr("- No targets found.");
    return errorStrings.join('\n');
}

void QnxConfiguration::setVersion(const QnxVersionNumber &version)
{
    m_version = version;
}

void QnxConfiguration::readInformation()
{
    const FilePath configPath = m_qnxConfiguration / "qconfig";
    if (!configPath.isDir())
        return;

    configPath.iterateDirectory([this, configPath](const FilePath &sdpFile) {
        QFile xmlFile(sdpFile.toFSPathString());
        if (!xmlFile.open(QIODevice::ReadOnly))
            return IterationPolicy::Continue;

        QDomDocument doc;
        if (!doc.setContent(&xmlFile))  // Skip error message
            return IterationPolicy::Continue;

        QDomElement docElt = doc.documentElement();
        if (docElt.tagName() != QLatin1String("qnxSystemDefinition"))
            return IterationPolicy::Continue;

        QDomElement childElt = docElt.firstChildElement(QLatin1String("installation"));
        // The file contains only one installation node
        if (childElt.isNull()) // The file contains only one base node
            return IterationPolicy::Continue;

        FilePath host = configPath.withNewPath(
            childElt.firstChildElement(QLatin1String("host")).text()).canonicalPath();
        if (m_qnxHost != host)
            return IterationPolicy::Continue;

        FilePath target = configPath.withNewPath(
            childElt.firstChildElement(QLatin1String("target")).text()).canonicalPath();
        if (m_qnxTarget != target)
            return IterationPolicy::Continue;

        m_configName = childElt.firstChildElement(QLatin1String("name")).text();
        QString version = childElt.firstChildElement(QLatin1String("version")).text();
        setVersion(QnxVersionNumber(version));
        return IterationPolicy::Stop;
    }, {{"*.xml"}, QDir::Files});
}

void QnxConfiguration::setDefaultConfiguration(const FilePath &envScript)
{
    QTC_ASSERT(!envScript.isEmpty(), return);
    m_envFile = envScript;
    m_qnxEnv = QnxUtils::qnxEnvironmentFromEnvFile(m_envFile);
    for (const EnvironmentItem &item : std::as_const(m_qnxEnv)) {
        if (item.name == QNXConfiguration)
            m_qnxConfiguration = envScript.withNewPath(item.value).canonicalPath();
        else if (item.name == QNXTarget)
            m_qnxTarget = envScript.withNewPath(item.value).canonicalPath();
        else if (item.name == QNXHost)
            m_qnxHost = envScript.withNewPath(item.value).canonicalPath();
    }

    const FilePath qccPath = m_qnxHost.pathAppended("usr/bin/qcc").withExecutableSuffix();
    if (qccPath.exists())
        m_qccCompiler = qccPath;

    // Some fall back in case the qconfig dir with .xml files is not found later
    if (m_configName.isEmpty())
        m_configName = QString("%1 - %2").arg(m_qnxHost.fileName(), m_qnxTarget.fileName());

    updateTargets();
    assignDebuggersToTargets();

    // Remove debuggerless targets.
    Utils::erase(m_targets, [](const Target &target) {
        if (target.m_debuggerPath.isEmpty())
            qWarning() << "No debugger found for" << target.m_path << "... discarded";
        return target.m_debuggerPath.isEmpty();
    });
}

EnvironmentItems QnxConfiguration::qnxEnvironmentItems() const
{
    Utils::EnvironmentItems envList;
    envList.push_back(EnvironmentItem(QNXConfiguration, m_qnxConfiguration.path()));
    envList.push_back(EnvironmentItem(QNXTarget, m_qnxTarget.path()));
    envList.push_back(EnvironmentItem(QNXHost, m_qnxHost.path()));

    return envList;
}

const QnxConfiguration::Target *QnxConfiguration::findTargetByDebuggerPath(
        const FilePath &path) const
{
    const auto it = std::find_if(m_targets.begin(), m_targets.end(),
                           [path](const Target &target) { return target.m_debuggerPath == path; });
    return it == m_targets.end() ? nullptr : &(*it);
}

void QnxConfiguration::updateTargets()
{
    m_targets.clear();
    const QList<QnxTarget> targets = QnxUtils::findTargets(m_qnxTarget);
    for (const QnxTarget &target : targets)
        m_targets.append(Target(target.m_abi, target.m_path));
}

void QnxConfiguration::assignDebuggersToTargets()
{
    const FilePath hostUsrBinDir = m_qnxHost.pathAppended("usr/bin");
    QString pattern = "nto*-gdb";
    if (m_qnxHost.osType() == Utils::OsTypeWindows)
        pattern += ".exe";

    const FilePaths debuggerNames = hostUsrBinDir.dirEntries({{pattern}, QDir::Files});
    Environment sysEnv = m_qnxHost.deviceEnvironment();
    sysEnv.modify(qnxEnvironmentItems());

    for (const FilePath &debuggerPath : debuggerNames) {
        DebuggerItem item;
        item.setCommand(debuggerPath);
        item.reinitializeFromFile(nullptr, &sysEnv);
        bool found = false;
        for (const Abi &abi : item.abis()) {
            for (Target &target : m_targets) {
                if (target.m_abi.isCompatibleWith(abi)) {
                    found = true;

                    if (target.m_debuggerPath.isEmpty()) {
                        target.m_debuggerPath = debuggerPath;
                    } else {
                        qWarning() << debuggerPath << "has the same ABI as" << target.m_debuggerPath
                                   << "... discarded";
                        break;
                    }
                }
            }
        }
        if (!found)
            qWarning() << "No target found for" << debuggerPath.toUserOutput() << "... discarded";
    }
}

QString QnxConfiguration::Target::shortDescription() const
{
    return QnxUtils::cpuDirShortDescription(cpuDir());
}

QString QnxConfiguration::Target::cpuDir() const
{
    return m_path.fileName();
}

} // Qnx::Internal
