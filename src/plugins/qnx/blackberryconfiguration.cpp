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

#include <coreplugin/icore.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtversionmanager.h>
#include <qtsupport/qtkitinformation.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/gcctoolchain.h>
#include <projectexplorer/toolchainmanager.h>

#include <qt4projectmanager/qmakekitinformation.h>

#include <debugger/debuggerkitinformation.h>

#include <utils/persistentsettings.h>
#include <utils/hostosinfo.h>

#include <QFileInfo>
#include <QDir>
#include <QMessageBox>

namespace Qnx {
namespace Internal {

namespace {
const QLatin1String SettingsGroup("BlackBerryConfiguration");
const QLatin1String NDKLocationKey("NDKLocation");
}

BlackBerryConfiguration::BlackBerryConfiguration(QObject *parent)
    :QObject(parent)
{
    loadSetting();
    connect(Core::ICore::instance(), SIGNAL(saveSettingsRequested()), this, SLOT(saveSetting()));
}

bool BlackBerryConfiguration::setConfig(const QString &ndkPath)
{
    if (ndkPath.isEmpty())
        return false;

    m_config.ndkPath = ndkPath;
    m_config.qnxEnv = QnxUtils::parseEnvironmentFile(QnxUtils::envFilePath(ndkPath));

    QString ndkTarget = m_config.qnxEnv.value(QLatin1String("QNX_TARGET"));
    QString sep = QString::fromLatin1("%1qnx6").arg(QDir::separator());
    m_config.targetName = ndkTarget.split(sep).first().split(QDir::separator()).last();

    if (QDir(ndkTarget).exists())
        m_config.sysRoot = Utils::FileName::fromString(ndkTarget);

    QString qnxHost = m_config.qnxEnv.value(QLatin1String("QNX_HOST"));
    Utils::FileName qmakePath = QnxUtils::executableWithExtension(Utils::FileName::fromString(qnxHost + QLatin1String("/usr/bin/qmake")));
    Utils::FileName gccPath = QnxUtils::executableWithExtension(Utils::FileName::fromString(qnxHost + QLatin1String("/usr/bin/qcc")));
    Utils::FileName deviceGdbPath = QnxUtils::executableWithExtension(Utils::FileName::fromString(qnxHost + QLatin1String("/usr/bin/ntoarm-gdb")));
    Utils::FileName simulatorGdbPath = QnxUtils::executableWithExtension(Utils::FileName::fromString(qnxHost + QLatin1String("/usr/bin/ntox86-gdb")));

    if (!qmakePath.toFileInfo().exists() || !gccPath.toFileInfo().exists()
            || !deviceGdbPath.toFileInfo().exists() || !simulatorGdbPath.toFileInfo().exists() ) {
        QString errorMessage = tr("The following errors occurred while setting up BB10 Configuration:\n");
        if (!qmakePath.toFileInfo().exists())
            errorMessage += tr("- No Qt version found\n");

        if (!gccPath.toFileInfo().exists())
            errorMessage += tr("- No GCC compiler found\n");

        if (!deviceGdbPath.toFileInfo().exists())
            errorMessage += tr("- No Gdb debugger found for BB10 Device\n");

        if (!simulatorGdbPath.toFileInfo().exists())
            errorMessage += tr("- No Gdb debugger found for BB10 Simulator");

        QMessageBox::warning(0, tr("Cannot Setup BB10 Configuration"),
                             errorMessage, QMessageBox::Ok);
        return false;
    }

    m_config.qmakeBinaryFile = qmakePath;
    m_config.gccCompiler = gccPath;
    m_config.deviceDebuger = deviceGdbPath;
    m_config.simulatorDebuger = simulatorGdbPath;

    return true;
}

void BlackBerryConfiguration::setupConfiguration(const QString &ndkPath)
{
    if (ndkPath.isEmpty())
        return;

    if (setConfig(ndkPath)) {
        QtSupport::BaseQtVersion *qtVersion = createQtVersion();
        ProjectExplorer::GccToolChain *tc = createGccToolChain();
        ProjectExplorer::Kit *deviceKit = createKit(ArmLeV7, qtVersion, tc);
        ProjectExplorer::Kit *simulatorKit = createKit(X86, qtVersion, tc);
        if (qtVersion && tc && deviceKit && simulatorKit) {
            tc->setTargetAbi(qtVersion->qtAbis().first());
            // register
            QtSupport::QtVersionManager::instance()->addVersion(qtVersion);
            ProjectExplorer::ToolChainManager::instance()->registerToolChain(tc);
            ProjectExplorer::KitManager::instance()->registerKit(deviceKit);
            ProjectExplorer::KitManager::instance()->registerKit(simulatorKit);

            emit updated();
        }
    }
}

void BlackBerryConfiguration::cleanConfiguration()
{
    QtSupport::BaseQtVersion *version = QtSupport::QtVersionManager::instance()->qtVersionForQMakeBinary(m_config.qmakeBinaryFile);
    if (version) {
        foreach (ProjectExplorer::Kit *kit, ProjectExplorer::KitManager::instance()->kits()) {
            if (version == QtSupport::QtKitInformation::qtVersion(kit))
                ProjectExplorer::KitManager::instance()->deregisterKit(kit);
        }

        QtSupport::QtVersionManager::instance()->removeVersion(version);
    }

    foreach (ProjectExplorer::ToolChain* tc, ProjectExplorer::ToolChainManager::instance()->toolChains()) {
        if (tc->compilerCommand() == m_config.gccCompiler)
            ProjectExplorer::ToolChainManager::instance()->deregisterToolChain(tc);
    }

    BlackBerryConfig conf;
    m_config = conf;
    emit updated();

    clearSetting();
}

QtSupport::BaseQtVersion *BlackBerryConfiguration::createQtVersion()
{
    if (m_config.qmakeBinaryFile.isEmpty())
        return 0;

    QString cpuDir = m_config.qnxEnv.value(QLatin1String("CPUVARDIR"));
    QtSupport::BaseQtVersion *version = QtSupport::QtVersionManager::instance()->qtVersionForQMakeBinary(m_config.qmakeBinaryFile);
    if (version) {
        QMessageBox::warning(0, tr("Qt Version Already Known"),
                             tr("This Qt version was already registered"), QMessageBox::Ok);
        return version;
    }

    version = new BlackBerryQtVersion(QnxUtils::cpudirToArch(cpuDir), m_config.qmakeBinaryFile, false, QString(), m_config.ndkPath);
    if (!version) {
        QMessageBox::warning(0, tr("Invalid Qt version"),
                             tr("Unable to add BlackBerry Qt version"), QMessageBox::Ok);
        return 0;
    }

    version->setDisplayName(QString::fromLatin1("Qt BlackBerry 10 (%1)").arg(m_config.targetName));

    return version;
}

ProjectExplorer::GccToolChain *BlackBerryConfiguration::createGccToolChain()
{
    if (m_config.qmakeBinaryFile.isEmpty() || m_config.gccCompiler.isEmpty())
        return 0;

    foreach (ProjectExplorer::ToolChain* tc, ProjectExplorer::ToolChainManager::instance()->toolChains()) {
        if (tc->compilerCommand() == m_config.gccCompiler) {
            QMessageBox::warning(0, tr("Compiler Already Known"),
                                 tr("This Compiler was already registered"), QMessageBox::Ok);
            return dynamic_cast<ProjectExplorer::GccToolChain*>(tc);
        }
    }

    ProjectExplorer::GccToolChain* tc = new ProjectExplorer::GccToolChain(QLatin1String(ProjectExplorer::Constants::GCC_TOOLCHAIN_ID), false);
    tc->setDisplayName(QString::fromLatin1("GCC BlackBerry 10 (%1)").arg(m_config.targetName));
    tc->setCompilerCommand(m_config.gccCompiler);

    return tc;
}

ProjectExplorer::Kit *BlackBerryConfiguration::createKit(QnxArchitecture arch, QtSupport::BaseQtVersion *qtVersion, ProjectExplorer::GccToolChain *tc)
{
    if (!qtVersion || !tc || m_config.targetName.isEmpty())
        return 0;

    // Check if an identical kit already exists
    foreach (ProjectExplorer::Kit *kit, ProjectExplorer::KitManager::instance()->kits())
    {
        if (QtSupport::QtKitInformation::qtVersion(kit) == qtVersion && ProjectExplorer::ToolChainKitInformation::toolChain(kit) == tc
                && ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(kit) == Constants::QNX_BB_OS_TYPE
                && ProjectExplorer::SysRootKitInformation::sysRoot(kit) == m_config.sysRoot) {
            if ((arch == X86 && Qt4ProjectManager::QmakeKitInformation::mkspec(kit).toString() == QString::fromLatin1("blackberry-x86-qcc")
                 && Debugger::DebuggerKitInformation::debuggerCommand(kit) == m_config.simulatorDebuger)
                    || (arch == ArmLeV7 && Debugger::DebuggerKitInformation::debuggerCommand(kit) == m_config.deviceDebuger)) {
                QMessageBox::warning(0, tr("Kit Already Known"),
                                     tr("This Kit was already registered"), QMessageBox::Ok);
                return kit;
            }
        }
    }

    ProjectExplorer::Kit *kit = new ProjectExplorer::Kit;
    QtSupport::QtKitInformation::setQtVersion(kit, qtVersion);
    ProjectExplorer::ToolChainKitInformation::setToolChain(kit, tc);
    if (arch == X86) {
        Debugger::DebuggerKitInformation::setDebuggerCommand(kit, m_config.simulatorDebuger);
        Qt4ProjectManager::QmakeKitInformation::setMkspec(kit, Utils::FileName::fromString(QString::fromLatin1("blackberry-x86-qcc")));
        // TODO: Check if the name already exists(?)
        kit->setDisplayName(tr("BlackBerry 10 (%1) - Simulator").arg(m_config.targetName));
    } else {
        Debugger::DebuggerKitInformation::setDebuggerCommand(kit, m_config.deviceDebuger);
        kit->setDisplayName(tr("BlackBerry 10 (%1)").arg(m_config.targetName));
    }

    ProjectExplorer::DeviceTypeKitInformation::setDeviceTypeId(kit, Constants::QNX_BB_OS_TYPE);
    ProjectExplorer::SysRootKitInformation::setSysRoot(kit, m_config.sysRoot);

    return kit;
}

void BlackBerryConfiguration::loadSetting()
{
    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup(SettingsGroup);
    setConfig(settings->value(NDKLocationKey).toString());
    settings->endGroup();
}

void BlackBerryConfiguration::saveSetting()
{
    if (m_config.ndkPath.isEmpty())
        return;

    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup(SettingsGroup);
    settings->setValue(NDKLocationKey, m_config.ndkPath);
    settings->endGroup();
}

void BlackBerryConfiguration::clearSetting()
{
    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup(SettingsGroup);
    settings->remove(NDKLocationKey);
    settings->endGroup();
}

BlackBerryConfiguration &BlackBerryConfiguration::instance()
{
    if (m_instance == 0)
        m_instance = new BlackBerryConfiguration();
    return *m_instance;
}

QString BlackBerryConfiguration::ndkPath() const
{
    return m_config.ndkPath;
}

QString BlackBerryConfiguration::targetName() const
{
    return m_config.targetName;
}

BlackBerryConfig BlackBerryConfiguration::config() const
{
    return m_config;
}

Utils::FileName BlackBerryConfiguration::qmakePath() const
{
    return m_config.qmakeBinaryFile;
}

Utils::FileName BlackBerryConfiguration::gccPath() const
{
    return m_config.gccCompiler;
}

Utils::FileName BlackBerryConfiguration::deviceGdbPath() const
{
    return m_config.deviceDebuger;
}

Utils::FileName BlackBerryConfiguration::simulatorGdbPath() const
{
    return m_config.simulatorDebuger;
}

Utils::FileName BlackBerryConfiguration::sysRoot() const
{
    return m_config.sysRoot;
}

// TODO: QnxUtils::parseEnvFile() and qnxEnv() to return Util::Enviroment instead(?)
QMultiMap<QString, QString> BlackBerryConfiguration::qnxEnv() const
{
    return m_config.qnxEnv;
}

BlackBerryConfiguration* BlackBerryConfiguration::m_instance = 0;

} // namespace Internal
} // namespace Qnx
