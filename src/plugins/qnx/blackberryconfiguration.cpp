/**************************************************************************
**
** Copyright (C) 2013 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "blackberryconfiguration.h"
#include "blackberryqtversion.h"

#include "qnxutils.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/toolchainmanager.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtversionmanager.h>
#include <qtsupport/qtkitinformation.h>

#include <qt4projectmanager/qmakekitinformation.h>

#include <debugger/debuggerkitinformation.h>

#include <QFileInfo>
#include <QDir>
#include <QMessageBox>

using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Utils;
using namespace Debugger;

namespace Qnx {
namespace Internal {

BlackBerryConfiguration::BlackBerryConfiguration(const FileName &ndkEnvFile, bool isAutoDetected,
                                                 const QString &displayName)
{
    Q_ASSERT(!QFileInfo(ndkEnvFile.toString()).isDir());
    m_ndkEnvFile = ndkEnvFile;
    m_isAutoDetected = isAutoDetected;
    QString ndkPath = ndkEnvFile.parentDir().toString();
    m_displayName = displayName.isEmpty() ? ndkPath.split(QDir::separator()).last() : displayName;
    m_qnxEnv = QnxUtils::qnxEnvironmentFromNdkFile(m_ndkEnvFile.toString());

    QString ndkTarget;
    QString qnxHost;
    foreach (const Utils::EnvironmentItem &item, m_qnxEnv) {
        if (item.name == QLatin1String("QNX_TARGET"))
            ndkTarget = item.value;

        else if (item.name == QLatin1String("QNX_HOST"))
            qnxHost = item.value;

    }

    QString sep = QString::fromLatin1("%1qnx6").arg(QDir::separator());
    m_targetName = ndkTarget.split(sep).first().split(QDir::separator()).last();

    if (QDir(ndkTarget).exists())
        m_sysRoot = FileName::fromString(ndkTarget);

    FileName qmake4Path = QnxUtils::executableWithExtension(FileName::fromString(qnxHost + QLatin1String("/usr/bin/qmake")));
    FileName qmake5Path = QnxUtils::executableWithExtension(FileName::fromString(qnxHost + QLatin1String("/usr/bin/qt5/qmake")));
    FileName gccPath = QnxUtils::executableWithExtension(FileName::fromString(qnxHost + QLatin1String("/usr/bin/qcc")));
    FileName deviceGdbPath = QnxUtils::executableWithExtension(FileName::fromString(qnxHost + QLatin1String("/usr/bin/ntoarm-gdb")));
    FileName simulatorGdbPath = QnxUtils::executableWithExtension(FileName::fromString(qnxHost + QLatin1String("/usr/bin/ntox86-gdb")));

    if (qmake4Path.toFileInfo().exists())
        m_qmake4BinaryFile = qmake4Path;

    if (qmake5Path.toFileInfo().exists())
        m_qmake5BinaryFile = qmake5Path;

    if (gccPath.toFileInfo().exists())
        m_gccCompiler = gccPath;

    if (deviceGdbPath.toFileInfo().exists())
        m_deviceDebugger = deviceGdbPath;

    if (simulatorGdbPath.toFileInfo().exists())
        m_simulatorDebugger = simulatorGdbPath;
}

QString BlackBerryConfiguration::ndkPath() const
{
    return m_ndkEnvFile.parentDir().toString();
}

QString BlackBerryConfiguration::displayName() const
{
    return m_displayName;
}

QString BlackBerryConfiguration::targetName() const
{
    return m_targetName;
}

bool BlackBerryConfiguration::isAutoDetected() const
{
    return m_isAutoDetected;
}

bool BlackBerryConfiguration::isActive() const
{
    return !findRegisteredQtVersions().isEmpty();
}

bool BlackBerryConfiguration::isValid() const
{
    return !((m_qmake4BinaryFile.isEmpty() && m_qmake5BinaryFile.isEmpty()) || m_gccCompiler.isEmpty()
            || m_deviceDebugger.isEmpty() || m_simulatorDebugger.isEmpty());
}

FileName BlackBerryConfiguration::ndkEnvFile() const
{
    return m_ndkEnvFile;
}

FileName BlackBerryConfiguration::qmake4BinaryFile() const
{
    return m_qmake4BinaryFile;
}

FileName BlackBerryConfiguration::qmake5BinaryFile() const
{
    return m_qmake5BinaryFile;
}

FileName BlackBerryConfiguration::gccCompiler() const
{
    return m_gccCompiler;
}

FileName BlackBerryConfiguration::deviceDebuger() const
{
    return m_deviceDebugger;
}

FileName BlackBerryConfiguration::simulatorDebuger() const
{
    return m_simulatorDebugger;
}

FileName BlackBerryConfiguration::sysRoot() const
{
    return m_sysRoot;
}

QList<Utils::EnvironmentItem> BlackBerryConfiguration::qnxEnv() const
{
    return m_qnxEnv;
}

void BlackBerryConfiguration::createConfigurationPerQtVersion(
        const FileName &qmakePath, Qnx::QnxArchitecture arch)
{
    QnxAbstractQtVersion *version = createQtVersion(qmakePath, arch);
    ToolChain *toolChain = createGccToolChain(version);
    Kit *kit = createKit(version, toolChain);

    if (version && toolChain && kit) {
        // register
        QtVersionManager::addVersion(version);
        ToolChainManager::registerToolChain(toolChain);
        KitManager::registerKit(kit);
    }
}

QnxAbstractQtVersion *BlackBerryConfiguration::createQtVersion(
        const FileName &qmakePath, Qnx::QnxArchitecture arch)
{
    QnxAbstractQtVersion *version = new BlackBerryQtVersion(
            arch, qmakePath, true, QString(), m_ndkEnvFile.toString());
    version->setDisplayName(tr("Qt %1 for %2 %3 - %4").arg(
            version->qtVersionString(), version->platformDisplayName(),
            version->archString(), m_targetName));
    return version;
}

GccToolChain *BlackBerryConfiguration::createGccToolChain(QnxAbstractQtVersion *version)
{
    GccToolChain* toolChain = new GccToolChain(
            QLatin1String(ProjectExplorer::Constants::GCC_TOOLCHAIN_ID), ToolChain::AutoDetection);
    //: QCC is the compiler for QNX.
    toolChain->setDisplayName(tr("QCC for Qt %1 for %2 %3 - %4").arg(
            version->qtVersionString(), version->platformDisplayName(),
            version->archString(), m_targetName));
    toolChain->setCompilerCommand(m_gccCompiler);
    QList<Abi> abis = version->qtAbis();
    if (!abis.isEmpty())
        toolChain->setTargetAbi(abis.first());
    return toolChain;
}

Kit *BlackBerryConfiguration::createKit(QnxAbstractQtVersion *version, ToolChain *toolChain)
{
    Kit *kit = new Kit;
    bool isSimulator = version->architecture() == X86;

    QtKitInformation::setQtVersion(kit, version);
    ToolChainKitInformation::setToolChain(kit, toolChain);

    DebuggerItem debugger;
    debugger.setCommand(isSimulator ? m_simulatorDebugger : m_deviceDebugger);
    debugger.setEngineType(GdbEngineType);
    debugger.setAutoDetected(true);
    debugger.setAbi(toolChain->targetAbi());
    debugger.setDisplayName(tr("Debugger for Qt %1 for %2 %3 - %4").arg(
            version->qtVersionString(), version->platformDisplayName(),
            version->archString(), m_targetName));

    QVariant id = DebuggerItemManager::registerDebugger(debugger);
    DebuggerKitInformation::setDebugger(kit, id);

    if (isSimulator)
        QmakeProjectManager::QmakeKitInformation::setMkspec(
                 kit, FileName::fromString(QLatin1String("blackberry-x86-qcc")));

    DeviceTypeKitInformation::setDeviceTypeId(kit, Constants::QNX_BB_OS_TYPE);
    SysRootKitInformation::setSysRoot(kit, m_sysRoot);

    kit->setDisplayName(tr("%1 %2 - Qt %3 - %4").arg(
            version->platformDisplayName(), isSimulator ? tr("Simulator") : tr("Device"),
            version->qtVersionString(), m_targetName));
    kit->setIconPath(FileName::fromString(QLatin1String(Constants::QNX_BB_CATEGORY_ICON)));

    kit->setAutoDetected(true);
    kit->setMutable(DeviceKitInformation::id(), true);

    kit->setSticky(QtKitInformation::id(), true);
    kit->setSticky(ToolChainKitInformation::id(), true);
    kit->setSticky(DeviceTypeKitInformation::id(), true);
    kit->setSticky(SysRootKitInformation::id(), true);
    kit->setSticky(DebuggerKitInformation::id(), true);
    kit->setSticky(QmakeProjectManager::QmakeKitInformation::id(), true);

    return kit;
}

bool BlackBerryConfiguration::activate()
{
    if (!isValid()) {
        if (m_isAutoDetected)
            return false;

        QString errorMessage = tr("The following errors occurred while activating target: %1").arg(m_targetName);
        if (m_qmake4BinaryFile.isEmpty() && m_qmake5BinaryFile.isEmpty())
            errorMessage += QLatin1Char('\n') + tr("- No Qt version found.");

        if (m_gccCompiler.isEmpty())
            errorMessage += QLatin1Char('\n') + tr("- No GCC compiler found.");

        if (m_deviceDebugger.isEmpty())
            errorMessage += QLatin1Char('\n') + tr("- No GDB debugger found for BB10 Device.");

        if (!m_simulatorDebugger.isEmpty())
            errorMessage += QLatin1Char('\n') + tr("- No GDB debugger found for BB10 Simulator.");

        QMessageBox::warning(0, tr("Cannot Set up BB10 Configuration"),
                             errorMessage, QMessageBox::Ok);
        return false;
    }

    if (isActive())
        return true;

    deactivate(); // cleaning-up artifacts autodetected by old QtCreator versions

    if (!m_qmake4BinaryFile.isEmpty()) {
        createConfigurationPerQtVersion(m_qmake4BinaryFile, Qnx::ArmLeV7);
        createConfigurationPerQtVersion(m_qmake4BinaryFile, Qnx::X86);
    }

    if (!m_qmake5BinaryFile.isEmpty()) {
        createConfigurationPerQtVersion(m_qmake5BinaryFile, Qnx::ArmLeV7);
        createConfigurationPerQtVersion(m_qmake5BinaryFile, Qnx::X86);
    }

    return true;
}

QList<BaseQtVersion *> BlackBerryConfiguration::findRegisteredQtVersions() const
{
    QList<BaseQtVersion *> versions;
    foreach (BaseQtVersion *version, QtVersionManager::versions()) {
        if (version->type() == QLatin1String(Constants::QNX_BB_QT)) {
            QnxAbstractQtVersion *qnxVersion = dynamic_cast<QnxAbstractQtVersion *>(version);
            if (qnxVersion  &&  qnxVersion->isAutodetected()
                    && (qnxVersion->qmakeCommand() == qmake4BinaryFile()
                    || qnxVersion->qmakeCommand() == qmake5BinaryFile()))
                versions << qnxVersion;
        }
    }
    return versions;
}

void BlackBerryConfiguration::deactivate()
{
    QList<BaseQtVersion *> versions = findRegisteredQtVersions();
    QList<ToolChain *> toolChains;
    foreach (Kit *kit, KitManager::kits()) {
        if (kit->isAutoDetected()) {
            BaseQtVersion *version = QtKitInformation::qtVersion(kit);
            if (versions.contains(version)) {
                ToolChain *toolChain = ToolChainKitInformation::toolChain(kit);
                if (toolChain)
                    toolChains << toolChain;
                KitManager::deregisterKit(kit);
            }
        }
    }

    foreach (const DebuggerItem &item, DebuggerItemManager::debuggers())
        if (item.isAutoDetected() &&
                (item.command() == m_simulatorDebugger || item.command() == m_deviceDebugger))
                DebuggerItemManager::deregisterDebugger(item);

    foreach (ToolChain *toolChain, ToolChainManager::toolChains())
        if (toolChain->isAutoDetected()
                && (toolChains.contains(toolChain) || toolChain->compilerCommand() == m_gccCompiler))
            ToolChainManager::deregisterToolChain(toolChain);

    foreach (BaseQtVersion *version, versions)
        QtVersionManager::removeVersion(version);

}

} // namespace Internal
} // namespace Qnx
