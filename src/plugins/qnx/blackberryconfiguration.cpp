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

namespace Qnx {
namespace Internal {

BlackBerryConfiguration::BlackBerryConfiguration(const QString &ndkPath, bool isAutoDetected, const QString &displayName)
{
    m_ndkPath = ndkPath;
    m_isAutoDetected = isAutoDetected;
    m_displayName = displayName.isEmpty() ? m_ndkPath.split(QDir::separator()).last() : displayName;
    m_qnxEnv = QnxUtils::parseEnvironmentFile(QnxUtils::envFilePath(m_ndkPath));

    QString ndkTarget = m_qnxEnv.value(QLatin1String("QNX_TARGET"));
    QString sep = QString::fromLatin1("%1qnx6").arg(QDir::separator());
    m_targetName = ndkTarget.split(sep).first().split(QDir::separator()).last();

    if (QDir(ndkTarget).exists())
        m_sysRoot = Utils::FileName::fromString(ndkTarget);

    QString qnxHost = m_qnxEnv.value(QLatin1String("QNX_HOST"));
    Utils::FileName qmake4Path = QnxUtils::executableWithExtension(Utils::FileName::fromString(qnxHost + QLatin1String("/usr/bin/qmake")));
    Utils::FileName qmake5Path = QnxUtils::executableWithExtension(Utils::FileName::fromString(qnxHost + QLatin1String("/usr/bin/qt5/qmake")));
    Utils::FileName gccPath = QnxUtils::executableWithExtension(Utils::FileName::fromString(qnxHost + QLatin1String("/usr/bin/qcc")));
    Utils::FileName deviceGdbPath = QnxUtils::executableWithExtension(Utils::FileName::fromString(qnxHost + QLatin1String("/usr/bin/ntoarm-gdb")));
    Utils::FileName simulatorGdbPath = QnxUtils::executableWithExtension(Utils::FileName::fromString(qnxHost + QLatin1String("/usr/bin/ntox86-gdb")));

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
    return m_ndkPath;
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
    QtSupport::BaseQtVersion *qt4Version = QtSupport::QtVersionManager::instance()->qtVersionForQMakeBinary(m_qmake4BinaryFile);
    QtSupport::BaseQtVersion *qt5Version = QtSupport::QtVersionManager::instance()->qtVersionForQMakeBinary(m_qmake5BinaryFile);
    return (qt4Version || qt5Version);
}

bool BlackBerryConfiguration::isValid() const
{
    return !((m_qmake4BinaryFile.isEmpty() && m_qmake5BinaryFile.isEmpty()) || m_gccCompiler.isEmpty()
            || m_deviceDebuger.isEmpty() || m_simulatorDebuger.isEmpty());
}

Utils::FileName BlackBerryConfiguration::qmake4BinaryFile() const
{
    return m_qmake4BinaryFile;
}

Utils::FileName BlackBerryConfiguration::qmake5BinaryFile() const
{
    return m_qmake5BinaryFile;
}

Utils::FileName BlackBerryConfiguration::gccCompiler() const
{
    return m_gccCompiler;
}

Utils::FileName BlackBerryConfiguration::deviceDebuger() const
{
    return m_deviceDebuger;
}

Utils::FileName BlackBerryConfiguration::simulatorDebuger() const
{
    return m_simulatorDebuger;
}

Utils::FileName BlackBerryConfiguration::sysRoot() const
{
    return m_sysRoot;
}

QMultiMap<QString, QString> BlackBerryConfiguration::qnxEnv() const
{
    return m_qnxEnv;
}

void BlackBerryConfiguration::setupConfigurationPerQtVersion(const Utils::FileName &qmakePath, ProjectExplorer::GccToolChain *tc)
{
    if (qmakePath.isEmpty() || !tc)
        return;

    QtSupport::BaseQtVersion *qtVersion = createQtVersion(qmakePath);
    ProjectExplorer::Kit *deviceKit = createKit(ArmLeV7, qtVersion, tc);
    ProjectExplorer::Kit *simulatorKit = createKit(X86, qtVersion, tc);
    if (qtVersion && tc && deviceKit && simulatorKit) {
        if (!qtVersion->qtAbis().isEmpty())
            tc->setTargetAbi(qtVersion->qtAbis().first());
        // register
        QtSupport::QtVersionManager::instance()->addVersion(qtVersion);
        ProjectExplorer::ToolChainManager::instance()->registerToolChain(tc);
        ProjectExplorer::KitManager::instance()->registerKit(deviceKit);
        ProjectExplorer::KitManager::instance()->registerKit(simulatorKit);
    }
}

QtSupport::BaseQtVersion *BlackBerryConfiguration::createQtVersion(const Utils::FileName &qmakePath)
{
    if (qmakePath.isEmpty())
        return 0;

    QString cpuDir = m_qnxEnv.value(QLatin1String("CPUVARDIR"));
    QtSupport::BaseQtVersion *version = QtSupport::QtVersionManager::instance()->qtVersionForQMakeBinary(qmakePath);
    if (version) {
        if (!m_isAutoDetected)
            QMessageBox::warning(0, QObject::tr("Qt Version Already Known"),
                             QObject::tr("This Qt version was already registered."), QMessageBox::Ok);
        return version;
    }

    version = new BlackBerryQtVersion(QnxUtils::cpudirToArch(cpuDir), qmakePath, m_isAutoDetected, QString(), m_ndkPath);
    if (!version) {
        if (!m_isAutoDetected)
            QMessageBox::warning(0, QObject::tr("Invalid Qt Version"),
                             QObject::tr("Unable to add BlackBerry Qt version."), QMessageBox::Ok);
        return 0;
    }

    version->setDisplayName(QString::fromLatin1("Qt %1 BlackBerry 10 (%2)").arg(version->qtVersionString(), m_targetName));
    return version;
}

ProjectExplorer::GccToolChain *BlackBerryConfiguration::createGccToolChain()
{
    if ((m_qmake4BinaryFile.isEmpty() && m_qmake5BinaryFile.isEmpty()) || m_gccCompiler.isEmpty())
        return 0;

    foreach (ProjectExplorer::ToolChain* tc, ProjectExplorer::ToolChainManager::instance()->toolChains()) {
        if (tc->compilerCommand() == m_gccCompiler) {
            if (!m_isAutoDetected)
                QMessageBox::warning(0, QObject::tr("Compiler Already Known"),
                                 QObject::tr("This compiler was already registered."), QMessageBox::Ok);
            return dynamic_cast<ProjectExplorer::GccToolChain*>(tc);
        }
    }

    ProjectExplorer::GccToolChain* tc = new ProjectExplorer::GccToolChain(QLatin1String(ProjectExplorer::Constants::GCC_TOOLCHAIN_ID), m_isAutoDetected);
    tc->setDisplayName(QString::fromLatin1("GCC BlackBerry 10 (%1)").arg(m_targetName));
    tc->setCompilerCommand(m_gccCompiler);

    return tc;
}

ProjectExplorer::Kit *BlackBerryConfiguration::createKit(QnxArchitecture arch, QtSupport::BaseQtVersion *qtVersion, ProjectExplorer::GccToolChain *tc)
{
    if (!qtVersion || !tc || m_targetName.isEmpty())
        return 0;

    // Check if an identical kit already exists
    foreach (ProjectExplorer::Kit *kit, ProjectExplorer::KitManager::instance()->kits())
    {
        if (QtSupport::QtKitInformation::qtVersion(kit) == qtVersion && ProjectExplorer::ToolChainKitInformation::toolChain(kit) == tc
                && ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(kit) == Constants::QNX_BB_OS_TYPE
                && ProjectExplorer::SysRootKitInformation::sysRoot(kit) == m_sysRoot) {
            if ((arch == X86 && Qt4ProjectManager::QmakeKitInformation::mkspec(kit).toString() == QString::fromLatin1("blackberry-x86-qcc")
                 && Debugger::DebuggerKitInformation::debuggerCommand(kit) == m_simulatorDebuger)
                    || (arch == ArmLeV7 && Debugger::DebuggerKitInformation::debuggerCommand(kit) == m_deviceDebuger)) {
                if (!m_isAutoDetected)
                    QMessageBox::warning(0, QObject::tr("Kit Already Known"),
                                     QObject::tr("This kit was already registered."), QMessageBox::Ok);
                setSticky(kit);
                return kit;
            }
        }
    }

    ProjectExplorer::Kit *kit = new ProjectExplorer::Kit;
    QtSupport::QtKitInformation::setQtVersion(kit, qtVersion);
    ProjectExplorer::ToolChainKitInformation::setToolChain(kit, tc);
    if (arch == X86) {
        Debugger::DebuggerKitInformation::setDebuggerCommand(kit, m_simulatorDebuger);
        Qt4ProjectManager::QmakeKitInformation::setMkspec(kit, Utils::FileName::fromString(QString::fromLatin1("blackberry-x86-qcc")));
        // TODO: Check if the name already exists(?)
        kit->setDisplayName(QObject::tr("BlackBerry 10 (%1 - %2) - Simulator").arg(qtVersion->qtVersionString(), m_targetName));
    } else {
        Debugger::DebuggerKitInformation::setDebuggerCommand(kit, m_deviceDebuger);
        kit->setDisplayName(QObject::tr("BlackBerry 10 (%1 - %2)").arg(qtVersion->qtVersionString(), m_targetName));
    }


    kit->setAutoDetected(m_isAutoDetected);
    kit->setIconPath(QLatin1String(Constants::QNX_BB_CATEGORY_ICON));
    setSticky(kit);
    ProjectExplorer::DeviceTypeKitInformation::setDeviceTypeId(kit, Constants::QNX_BB_OS_TYPE);
    ProjectExplorer::SysRootKitInformation::setSysRoot(kit, m_sysRoot);

    return kit;
}

void BlackBerryConfiguration::setSticky(ProjectExplorer::Kit *kit)
{
    kit->makeSticky(Core::Id("QtSupport.QtInformation"));
    kit->makeSticky(Core::Id("PE.Profile.ToolChain"));
    kit->makeSticky(Core::Id("PE.Profile.SysRoot"));
    kit->makeSticky(Core::Id("PE.Profile.DeviceType"));
    kit->makeSticky(Core::Id("Debugger.Information"));
    kit->makeSticky(Core::Id("QtPM4.mkSpecInformation"));
}

bool BlackBerryConfiguration::activate()
{
    if (!isValid()) {
        if (m_isAutoDetected)
            return false;

        QString errorMessage = QObject::tr("The following errors occurred while activating NDK: %1").arg(m_ndkPath);
        if (m_qmake4BinaryFile.isEmpty() && m_qmake4BinaryFile.isEmpty())
            errorMessage += QLatin1Char('\n') + QObject::tr("- No Qt version found.");

        if (m_gccCompiler.isEmpty())
            errorMessage += QLatin1Char('\n') + QObject::tr("- No GCC compiler found.");

        if (m_deviceDebuger.isEmpty())
            errorMessage += QLatin1Char('\n') + QObject::tr("- No GDB debugger found for BB10 Device.");

        if (!m_simulatorDebuger.isEmpty())
            errorMessage += QLatin1Char('\n') + QObject::tr("- No GDB debugger found for BB10 Simulator.");

        QMessageBox::warning(0, QObject::tr("Cannot Set up BB10 Configuration"),
                             errorMessage, QMessageBox::Ok);
        return false;
    }

    if (isActive() && !m_isAutoDetected)
        return true;

    ProjectExplorer::GccToolChain *tc = createGccToolChain();
    if (!m_qmake4BinaryFile.isEmpty())
        setupConfigurationPerQtVersion(m_qmake4BinaryFile, tc);

    if (!m_qmake4BinaryFile.isEmpty())
        setupConfigurationPerQtVersion(m_qmake5BinaryFile, tc);

    return true;
}

void BlackBerryConfiguration::deactivate()
{
    QtSupport::BaseQtVersion *qt4Version = QtSupport::QtVersionManager::instance()->qtVersionForQMakeBinary(m_qmake4BinaryFile);
    QtSupport::BaseQtVersion *qt5Version = QtSupport::QtVersionManager::instance()->qtVersionForQMakeBinary(m_qmake5BinaryFile);
    if (qt4Version || qt5Version) {
        foreach (ProjectExplorer::Kit *kit, ProjectExplorer::KitManager::instance()->kits()) {
            if (qt4Version && qt4Version == QtSupport::QtKitInformation::qtVersion(kit))
                ProjectExplorer::KitManager::instance()->deregisterKit(kit);

            else if (qt5Version && qt5Version == QtSupport::QtKitInformation::qtVersion(kit))
                ProjectExplorer::KitManager::instance()->deregisterKit(kit);
        }

        if (qt4Version)
            QtSupport::QtVersionManager::instance()->removeVersion(qt4Version);

        if (qt5Version)
            QtSupport::QtVersionManager::instance()->removeVersion(qt5Version);

    }

    foreach (ProjectExplorer::ToolChain* tc, ProjectExplorer::ToolChainManager::instance()->toolChains()) {
        if (tc->compilerCommand() == m_gccCompiler)
            ProjectExplorer::ToolChainManager::instance()->deregisterToolChain(tc);
    }
}

} // namespace Internal
} // namespace Qnx
