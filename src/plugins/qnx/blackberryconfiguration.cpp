/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
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
    m_qnxEnv = QnxUtils::parseEnvironmentFile(m_ndkEnvFile.toString());

    QString ndkTarget = m_qnxEnv.value(QLatin1String("QNX_TARGET"));
    QString sep = QString::fromLatin1("%1qnx6").arg(QDir::separator());
    m_targetName = ndkTarget.split(sep).first().split(QDir::separator()).last();

    if (QDir(ndkTarget).exists())
        m_sysRoot = FileName::fromString(ndkTarget);

    QString qnxHost = m_qnxEnv.value(QLatin1String("QNX_HOST"));
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
        m_deviceDebuger = deviceGdbPath;

    if (simulatorGdbPath.toFileInfo().exists())
        m_simulatorDebuger = simulatorGdbPath;

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
    BaseQtVersion *qt4Version = QtVersionManager::qtVersionForQMakeBinary(m_qmake4BinaryFile);
    BaseQtVersion *qt5Version = QtVersionManager::qtVersionForQMakeBinary(m_qmake5BinaryFile);
    return (qt4Version || qt5Version);
}

bool BlackBerryConfiguration::isValid() const
{
    return !((m_qmake4BinaryFile.isEmpty() && m_qmake5BinaryFile.isEmpty()) || m_gccCompiler.isEmpty()
            || m_deviceDebuger.isEmpty() || m_simulatorDebuger.isEmpty());
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
    return m_deviceDebuger;
}

FileName BlackBerryConfiguration::simulatorDebuger() const
{
    return m_simulatorDebuger;
}

FileName BlackBerryConfiguration::sysRoot() const
{
    return m_sysRoot;
}

QMultiMap<QString, QString> BlackBerryConfiguration::qnxEnv() const
{
    return m_qnxEnv;
}

void BlackBerryConfiguration::setupConfigurationPerQtVersion(const FileName &qmakePath, GccToolChain *tc)
{
    if (qmakePath.isEmpty() || !tc)
        return;

    BaseQtVersion *qtVersion = createQtVersion(qmakePath);
    Kit *deviceKit = createKit(ArmLeV7, qtVersion, tc);
    Kit *simulatorKit = createKit(X86, qtVersion, tc);
    if (qtVersion && tc && deviceKit && simulatorKit) {
        if (!qtVersion->qtAbis().isEmpty())
            tc->setTargetAbi(qtVersion->qtAbis().first());
        // register
        QtVersionManager::addVersion(qtVersion);
        ToolChainManager::registerToolChain(tc);
        KitManager::registerKit(deviceKit);
        KitManager::registerKit(simulatorKit);
    }
}

BaseQtVersion *BlackBerryConfiguration::createQtVersion(const FileName &qmakePath)
{
    if (qmakePath.isEmpty())
        return 0;

    QString cpuDir = m_qnxEnv.value(QLatin1String("CPUVARDIR"));
    BaseQtVersion *version = QtVersionManager::qtVersionForQMakeBinary(qmakePath);
    if (version) {
        if (!m_isAutoDetected)
            QMessageBox::warning(0, tr("Qt Version Already Known"),
                             tr("This Qt version was already registered."), QMessageBox::Ok);
        return version;
    }

    version = new BlackBerryQtVersion(QnxUtils::cpudirToArch(cpuDir), qmakePath, m_isAutoDetected, QString(), m_ndkEnvFile.toString());
    if (!version) {
        if (!m_isAutoDetected)
            QMessageBox::warning(0, tr("Invalid Qt Version"),
                             tr("Unable to add BlackBerry Qt version."), QMessageBox::Ok);
        return 0;
    }

    version->setDisplayName(QString::fromLatin1("Qt %1 BlackBerry 10 (%2)").arg(version->qtVersionString(), m_targetName));
    return version;
}

GccToolChain *BlackBerryConfiguration::createGccToolChain()
{
    if ((m_qmake4BinaryFile.isEmpty() && m_qmake5BinaryFile.isEmpty()) || m_gccCompiler.isEmpty())
        return 0;

    foreach (ToolChain *tc, ToolChainManager::toolChains()) {
        if (tc->compilerCommand() == m_gccCompiler) {
            if (!m_isAutoDetected)
                QMessageBox::warning(0, tr("Compiler Already Known"),
                                 tr("This compiler was already registered."), QMessageBox::Ok);
            return dynamic_cast<GccToolChain *>(tc);
        }
    }

    GccToolChain* tc = new GccToolChain(QLatin1String(ProjectExplorer::Constants::GCC_TOOLCHAIN_ID), m_isAutoDetected ? ToolChain::AutoDetection : ToolChain::ManualDetection);
    tc->setDisplayName(QString::fromLatin1("GCC BlackBerry 10 (%1)").arg(m_targetName));
    tc->setCompilerCommand(m_gccCompiler);

    return tc;
}

Kit *BlackBerryConfiguration::createKit(QnxArchitecture arch, BaseQtVersion *qtVersion, GccToolChain *tc)
{
    if (!qtVersion || !tc || m_targetName.isEmpty())
        return 0;

    // Check if an identical kit already exists
    foreach (Kit *kit, KitManager::kits()) {
        if (QtKitInformation::qtVersion(kit) == qtVersion && ToolChainKitInformation::toolChain(kit) == tc
                && DeviceTypeKitInformation::deviceTypeId(kit) == Constants::QNX_BB_OS_TYPE
                && SysRootKitInformation::sysRoot(kit) == m_sysRoot) {
            if ((arch == X86 && Qt4ProjectManager::QmakeKitInformation::mkspec(kit).toString() == QString::fromLatin1("blackberry-x86-qcc")
                 && Debugger::DebuggerKitInformation::debuggerCommand(kit) == m_simulatorDebuger)
                    || (arch == ArmLeV7 && Debugger::DebuggerKitInformation::debuggerCommand(kit) == m_deviceDebuger)) {
                if (!m_isAutoDetected)
                    QMessageBox::warning(0, tr("Kit Already Known"),
                                     tr("This kit was already registered."), QMessageBox::Ok);
                setSticky(kit);
                return kit;
            }
        }
    }

    Kit *kit = new Kit;
    QtKitInformation::setQtVersion(kit, qtVersion);
    ToolChainKitInformation::setToolChain(kit, tc);

    QString versionName = QString::fromLatin1("%1 - %2").arg(qtVersion->qtVersionString(), m_targetName);

    Debugger::DebuggerItem debugger;
    debugger.setCommand(arch == X86 ? m_simulatorDebuger : m_deviceDebuger);
    debugger.setEngineType(Debugger::GdbEngineType);
    debugger.setDisplayName(arch == X86
            ? tr("BlackBerry Debugger (%1) - Simulator").arg(versionName)
            : tr("BlackBerry Debugger (%1) - Device").arg(versionName));
    debugger.setAutoDetected(true);
    debugger.setAbi(tc->targetAbi());
    Debugger::DebuggerKitInformation::setDebugger(kit, debugger);

    if (arch == X86) {
        Qt4ProjectManager::QmakeKitInformation::setMkspec(kit, FileName::fromString(QString::fromLatin1("blackberry-x86-qcc")));
        // TODO: Check if the name already exists(?)
        kit->setDisplayName(tr("BlackBerry 10 (%1) - Simulator").arg(versionName));
    } else {
        kit->setDisplayName(tr("BlackBerry 10 (%1)").arg(versionName));
    }

    kit->setAutoDetected(m_isAutoDetected);
    kit->setIconPath(FileName::fromString(QLatin1String(Constants::QNX_BB_CATEGORY_ICON)));
    kit->setMutable(DeviceKitInformation::id(), true);
    setSticky(kit);
    DeviceTypeKitInformation::setDeviceTypeId(kit, Constants::QNX_BB_OS_TYPE);
    SysRootKitInformation::setSysRoot(kit, m_sysRoot);

    return kit;
}

void BlackBerryConfiguration::setSticky(Kit *kit)
{
    kit->setSticky(QtKitInformation::id(), true);
    kit->setSticky(ToolChainKitInformation::id(), true);
    kit->setSticky(DeviceTypeKitInformation::id(), true);
    kit->setSticky(SysRootKitInformation::id(), true);
    kit->setSticky(Debugger::DebuggerKitInformation::id(), true);
    kit->setSticky(Qt4ProjectManager::QmakeKitInformation::id(), true);
}

bool BlackBerryConfiguration::activate()
{
    if (!isValid()) {
        if (m_isAutoDetected)
            return false;

        QString errorMessage = tr("The following errors occurred while activating Target: %1").arg(m_targetName);
        if (m_qmake4BinaryFile.isEmpty() && m_qmake5BinaryFile.isEmpty())
            errorMessage += QLatin1Char('\n') + tr("- No Qt version found.");

        if (m_gccCompiler.isEmpty())
            errorMessage += QLatin1Char('\n') + tr("- No GCC compiler found.");

        if (m_deviceDebuger.isEmpty())
            errorMessage += QLatin1Char('\n') + tr("- No GDB debugger found for BB10 Device.");

        if (!m_simulatorDebuger.isEmpty())
            errorMessage += QLatin1Char('\n') + tr("- No GDB debugger found for BB10 Simulator.");

        QMessageBox::warning(0, tr("Cannot Set up BB10 Configuration"),
                             errorMessage, QMessageBox::Ok);
        return false;
    }

    if (isActive() && !m_isAutoDetected)
        return true;

    GccToolChain *tc = createGccToolChain();
    if (!m_qmake4BinaryFile.isEmpty())
        setupConfigurationPerQtVersion(m_qmake4BinaryFile, tc);

    if (!m_qmake5BinaryFile.isEmpty())
        setupConfigurationPerQtVersion(m_qmake5BinaryFile, tc);

    return true;
}

void BlackBerryConfiguration::deactivate()
{
    BaseQtVersion *qt4Version = QtVersionManager::qtVersionForQMakeBinary(m_qmake4BinaryFile);
    BaseQtVersion *qt5Version = QtVersionManager::qtVersionForQMakeBinary(m_qmake5BinaryFile);
    if (qt4Version || qt5Version) {
        foreach (Kit *kit, KitManager::kits()) {
            if (qt4Version && qt4Version == QtKitInformation::qtVersion(kit))
                KitManager::deregisterKit(kit);

            else if (qt5Version && qt5Version == QtKitInformation::qtVersion(kit))
                KitManager::deregisterKit(kit);
        }

        if (qt4Version)
            QtVersionManager::removeVersion(qt4Version);

        if (qt5Version)
            QtVersionManager::removeVersion(qt5Version);

    }

    foreach (ToolChain *tc, ToolChainManager::toolChains())
        if (tc->compilerCommand() == m_gccCompiler)
            ToolChainManager::deregisterToolChain(tc);
}

} // namespace Internal
} // namespace Qnx
