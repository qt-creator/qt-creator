/**************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
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

#include "qnxtoolchain.h"
#include "qnxutils.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/gcctoolchain.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtversionmanager.h>
#include <qtsupport/qtkitinformation.h>

#include <qmakeprojectmanager/qmakekitinformation.h>

#include <debugger/debuggeritemmanager.h>
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
    foreach (const Utils::EnvironmentItem &item, m_qnxEnv) {
        if (item.name == QLatin1String("QNX_TARGET"))
            ndkTarget = item.value;

        else if (item.name == QLatin1String("QNX_HOST"))
            m_qnxHost = item.value;

    }

    // The QNX_TARGET value is using Unix-like separator on all platforms.
    QString sep = QString::fromLatin1("/qnx6");
    m_targetName = ndkTarget.split(sep).first().split(QLatin1Char('/')).last();

    if (QDir(ndkTarget).exists())
        m_sysRoot = FileName::fromString(ndkTarget);

    FileName qmake4Path = QnxUtils::executableWithExtension(FileName::fromString(m_qnxHost + QLatin1String("/usr/bin/qmake")));
    FileName qmake5Path = QnxUtils::executableWithExtension(FileName::fromString(m_qnxHost + QLatin1String("/usr/bin/qt5/qmake")));
    FileName gccPath = QnxUtils::executableWithExtension(FileName::fromString(m_qnxHost + QLatin1String("/usr/bin/qcc")));
    FileName deviceGdbPath = QnxUtils::executableWithExtension(FileName::fromString(m_qnxHost + QLatin1String("/usr/bin/ntoarm-gdb")));
    FileName simulatorGdbPath = QnxUtils::executableWithExtension(FileName::fromString(m_qnxHost + QLatin1String("/usr/bin/ntox86-gdb")));

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

QString BlackBerryConfiguration::qnxHost() const
{
    return m_qnxHost;
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

QnxAbstractQtVersion *BlackBerryConfiguration::createQtVersion(
        const FileName &qmakePath, Qnx::QnxArchitecture arch, const QString &versionName)
{
    QnxAbstractQtVersion *version = new BlackBerryQtVersion(
            arch, qmakePath, true, QString(), m_ndkEnvFile.toString());
    version->setDisplayName(tr("Qt %1 for %2").arg(version->qtVersionString(), versionName));
    QtVersionManager::addVersion(version);
    return version;
}

QnxToolChain *BlackBerryConfiguration::createToolChain(
        ProjectExplorer::Abi abi, const QString &versionName)
{
    QnxToolChain* toolChain = new QnxToolChain(ToolChain::AutoDetection);
    toolChain->setDisplayName(tr("QCC for %1").arg(versionName));
    toolChain->setCompilerCommand(m_gccCompiler);
    toolChain->setNdkPath(ndkPath());
    if (abi.isValid())
        toolChain->setTargetAbi(abi);
    ToolChainManager::registerToolChain(toolChain);
    return toolChain;
}

QVariant BlackBerryConfiguration::createDebuggerItem(
        QList<ProjectExplorer::Abi> abis, Qnx::QnxArchitecture arch, const QString &versionName)
{
    Utils::FileName command = arch == X86 ? m_simulatorDebugger : m_deviceDebugger;
    DebuggerItem debugger;
    debugger.setCommand(command);
    debugger.setEngineType(GdbEngineType);
    debugger.setAutoDetected(true);
    debugger.setAbis(abis);
    debugger.setDisplayName(tr("Debugger for %1").arg(versionName));
    return DebuggerItemManager::registerDebugger(debugger);
}

Kit *BlackBerryConfiguration::createKit(
        QnxAbstractQtVersion *version, QnxToolChain *toolChain, const QVariant &debuggerItemId)
{
    Kit *kit = new Kit;
    bool isSimulator = version->architecture() == X86;

    QtKitInformation::setQtVersion(kit, version);
    ToolChainKitInformation::setToolChain(kit, toolChain);

    if (debuggerItemId.isValid())
        DebuggerKitInformation::setDebugger(kit, debuggerItemId);

    if (isSimulator)
        QmakeProjectManager::QmakeKitInformation::setMkspec(
                 kit, FileName::fromString(QLatin1String("blackberry-x86-qcc")));

    DeviceTypeKitInformation::setDeviceTypeId(kit, Constants::QNX_BB_OS_TYPE);
    SysRootKitInformation::setSysRoot(kit, m_sysRoot);

    kit->setDisplayName(version->displayName());
    kit->setIconPath(FileName::fromString(QLatin1String(Constants::QNX_BB_CATEGORY_ICON)));

    kit->setAutoDetected(true);
    kit->setMutable(DeviceKitInformation::id(), true);

    kit->setSticky(QtKitInformation::id(), true);
    kit->setSticky(ToolChainKitInformation::id(), true);
    kit->setSticky(DeviceTypeKitInformation::id(), true);
    kit->setSticky(SysRootKitInformation::id(), true);
    kit->setSticky(DebuggerKitInformation::id(), true);
    kit->setSticky(QmakeProjectManager::QmakeKitInformation::id(), true);

    KitManager::registerKit(kit);
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

    QString armVersionName = tr("BlackBerry Device - %1").arg(m_targetName);
    QString x86VersionName = tr("BlackBerry Simulator - %1").arg(m_targetName);

    // create versions
    QnxAbstractQtVersion *qt4ArmVersion = 0;
    QnxAbstractQtVersion *qt4X86Version = 0;
    QnxAbstractQtVersion *qt5ArmVersion = 0;
    QnxAbstractQtVersion *qt5X86Version = 0;
    QList<Abi> armAbis;
    QList<Abi> x86Abis;

    if (!m_qmake4BinaryFile.isEmpty()) {
        qt4ArmVersion = createQtVersion(m_qmake4BinaryFile, Qnx::ArmLeV7, armVersionName);
        armAbis << qt4ArmVersion->qtAbis();
        qt4X86Version = createQtVersion(m_qmake4BinaryFile, Qnx::X86, x86VersionName);
        x86Abis << qt4X86Version->qtAbis();
    }
    if (!m_qmake5BinaryFile.isEmpty()) {
        qt5ArmVersion = createQtVersion(m_qmake5BinaryFile, Qnx::ArmLeV7, armVersionName);
        foreach (Abi abi, qt5ArmVersion->qtAbis())
            if (!armAbis.contains(abi))
                armAbis << abi;
        qt5X86Version = createQtVersion(m_qmake5BinaryFile, Qnx::X86, x86VersionName);
        foreach (Abi abi, qt5X86Version->qtAbis())
            if (!x86Abis.contains(abi))
                x86Abis << abi;
    }

    // create toolchains
    QnxToolChain *armToolChain = createToolChain(
                !armAbis.isEmpty() ? armAbis.first() : Abi(), armVersionName);
    QnxToolChain *x86ToolChain = createToolChain(
                !x86Abis.isEmpty() ? x86Abis.first() : Abi(), x86VersionName);

    // create debuggers
    QVariant armDebuggerItemId = createDebuggerItem(armAbis, Qnx::ArmLeV7, armVersionName);
    QVariant x86DebuggerItemId = createDebuggerItem(x86Abis, Qnx::X86, x86VersionName);

    // create kits
    if (qt4ArmVersion)
        createKit(qt4ArmVersion, armToolChain, armDebuggerItemId);
    if (qt4X86Version)
        createKit(qt4X86Version, x86ToolChain, x86DebuggerItemId);
    if (qt5ArmVersion)
        createKit(qt5ArmVersion, armToolChain, armDebuggerItemId);
    if (qt5X86Version)
        createKit(qt5X86Version, x86ToolChain, x86DebuggerItemId);

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
                DebuggerItemManager::deregisterDebugger(item.id());

    foreach (ToolChain *toolChain, ToolChainManager::toolChains())
        if (toolChain->isAutoDetected()
                && (toolChains.contains(toolChain) || toolChain->compilerCommand() == m_gccCompiler))
            ToolChainManager::deregisterToolChain(toolChain);

    foreach (BaseQtVersion *version, versions)
        QtVersionManager::removeVersion(version);

}

} // namespace Internal
} // namespace Qnx
