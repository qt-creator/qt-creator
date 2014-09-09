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

#include "blackberryapilevelconfiguration.h"
#include "blackberryconfigurationmanager.h"
#include "blackberryqtversion.h"

#include "qnxtoolchain.h"
#include "qnxconstants.h"

#include <utils/qtcassert.h>

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

#include <coreplugin/icore.h>

#include <QFileInfo>
#include <QDir>
#include <QMessageBox>

using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Utils;
using namespace Debugger;

namespace Qnx {
namespace Internal {

const QLatin1String NDKPathKey("NDKPath");
const QLatin1String NDKDisplayNameKey("NDKDisplayName");
const QLatin1String NDKTargetKey("NDKTarget");
const QLatin1String NDKHostKey("NDKHost");
const QLatin1String NDKAutoDetectionSourceKey("NDKAutoDetectionSource");
const QLatin1String NDKAutoDetectedKey("NDKAutoDetectedKey");

#ifdef WITH_TESTS
bool BlackBerryApiLevelConfiguration::m_fakeConfig = false;
#endif

BlackBerryApiLevelConfiguration::BlackBerryApiLevelConfiguration(const ConfigInstallInformation &ndkInstallInfo)
    : QnxBaseConfiguration(FileName::fromString(
                               QnxUtils::envFilePath(ndkInstallInfo.path, ndkInstallInfo.version)))
{
    m_displayName = ndkInstallInfo.name;
    QString sep = QString::fromLatin1("/qnx6");
    // The QNX_TARGET value is using Unix-like separator on all platforms.
    m_targetName = ndkInstallInfo.target.split(sep).first().split(QLatin1Char('/')).last();
    m_sysRoot = FileName::fromString(ndkInstallInfo.target);
    m_autoDetectionSource = Utils::FileName::fromString(ndkInstallInfo.installationXmlFilePath);
    setVersion(QnxVersionNumber(ndkInstallInfo.version));
    ctor();
}

BlackBerryApiLevelConfiguration::BlackBerryApiLevelConfiguration(const FileName &ndkEnvFile)
    : QnxBaseConfiguration(ndkEnvFile)
{
    m_displayName = ndkPath().split(QDir::separator()).last();
    QString ndkTarget = qnxTarget().toString();
    // The QNX_TARGET value is using Unix-like separator on all platforms.
    QString sep = QString::fromLatin1("/qnx6");
    m_targetName = ndkTarget.split(sep).first().split(QLatin1Char('/')).last();
    if (QDir(ndkTarget).exists())
        m_sysRoot = FileName::fromString(ndkTarget);

    setVersion(QnxVersionNumber::fromNdkEnvFileName(QFileInfo(envFile().toString()).baseName()));
    if (version().isEmpty())
        setVersion(QnxVersionNumber::fromTargetName(m_targetName));

    ctor();
}

BlackBerryApiLevelConfiguration::BlackBerryApiLevelConfiguration(const QVariantMap &data)
    : QnxBaseConfiguration(data)
{
    m_displayName = data.value(NDKDisplayNameKey).toString();
    QString sep = QString::fromLatin1("/qnx6");
    // The QNX_TARGET value is using Unix-like separator on all platforms.
    m_targetName = data.value(NDKTargetKey).toString().split(sep).first().split(QLatin1Char('/')).last();
    m_sysRoot = FileName::fromString(data.value(NDKTargetKey).toString());
    if (data.value(QLatin1String(NDKAutoDetectedKey)).toBool())
        m_autoDetectionSource = Utils::FileName::fromString(data.value(NDKAutoDetectionSourceKey).toString());

    ctor();
}

void BlackBerryApiLevelConfiguration::ctor()
{
    QString host = qnxHost().toString();
    FileName qmake4Path = FileName::fromString(Utils::HostOsInfo::withExecutableSuffix(host + QLatin1String("/usr/bin/qmake")));
    FileName qmake5Path = FileName::fromString(Utils::HostOsInfo::withExecutableSuffix(host + QLatin1String("/usr/bin/qt5/qmake")));
    if (qmake4Path.toFileInfo().exists())
        m_qmake4BinaryFile = qmake4Path;

    if (qmake5Path.toFileInfo().exists())
        m_qmake5BinaryFile = qmake5Path;
}

QString BlackBerryApiLevelConfiguration::ndkPath() const
{
    return envFile().parentDir().toString();
}

QString BlackBerryApiLevelConfiguration::displayName() const
{
    return m_displayName;
}

QString BlackBerryApiLevelConfiguration::targetName() const
{
    return m_targetName;
}

bool BlackBerryApiLevelConfiguration::isAutoDetected() const
{
    return !m_autoDetectionSource.isEmpty();
}

Utils::FileName BlackBerryApiLevelConfiguration::autoDetectionSource() const
{
    return m_autoDetectionSource;
}

bool BlackBerryApiLevelConfiguration::isActive() const
{
    foreach (Kit *kit, KitManager::kits()) {
        if (kit->isAutoDetected() &&
                kit->autoDetectionSource() == envFile().toString())
            return true;
    }

    return false;
}

bool BlackBerryApiLevelConfiguration::isValid() const
{
#ifdef WITH_TESTS
    if (BlackBerryApiLevelConfiguration::fakeConfig())
        return true;
#endif

    return QnxBaseConfiguration::isValid() &&
            ((!m_qmake4BinaryFile.isEmpty() || !m_qmake5BinaryFile.isEmpty())
            && (m_autoDetectionSource.isEmpty() ||
                m_autoDetectionSource.toFileInfo().exists())
            && (!m_sysRoot.isEmpty() && m_sysRoot.toFileInfo().exists()));
}


FileName BlackBerryApiLevelConfiguration::qmake4BinaryFile() const
{
    return m_qmake4BinaryFile;
}

FileName BlackBerryApiLevelConfiguration::qmake5BinaryFile() const
{
    return m_qmake5BinaryFile;
}

FileName BlackBerryApiLevelConfiguration::sysRoot() const
{
    return m_sysRoot;
}

QVariantMap BlackBerryApiLevelConfiguration::toMap() const
{
    QVariantMap data = QnxBaseConfiguration::toMap();
    data.insert(QLatin1String(Qnx::Constants::QNX_BB_KEY_CONFIGURATION_TYPE),
                QLatin1String(Qnx::Constants::QNX_BB_APILEVEL_TYPE));
    data.insert(QLatin1String(NDKDisplayNameKey), m_displayName);
    data.insert(QLatin1String(NDKPathKey), ndkPath());
    data.insert(QLatin1String(NDKTargetKey), m_sysRoot.toString());
    data.insert(QLatin1String(NDKAutoDetectionSourceKey), m_autoDetectionSource.toString());
    data.insert(QLatin1String(NDKAutoDetectedKey), isAutoDetected());
    return data;
}

QnxAbstractQtVersion *BlackBerryApiLevelConfiguration::createQtVersion(
        const FileName &qmakePath, Qnx::QnxArchitecture arch, const QString &versionName)
{
    QnxAbstractQtVersion *version = new BlackBerryQtVersion(
            arch, qmakePath, true, QString(), envFile().toString());
    version->setUnexpandedDisplayName(tr("Qt %{Qt:version} for %2")
                                      .arg(version->qtVersionString(), versionName));
    QtVersionManager::addVersion(version);
    return version;
}

Kit *BlackBerryApiLevelConfiguration::createKit(
        QnxAbstractQtVersion *version, QnxToolChain *toolChain, const QVariant &debuggerItemId)
{
    Kit *kit = new Kit;
    bool isSimulator = version->architecture() == X86;

    QtKitInformation::setQtVersion(kit, version);
    ToolChainKitInformation::setToolChain(kit, toolChain);

    if (debuggerItemId.isValid())
        DebuggerKitInformation::setDebugger(kit, debuggerItemId);

    if (version->qtVersion().majorVersion == 4) {
        if (isSimulator) {
            QmakeProjectManager::QmakeKitInformation::setMkspec(
                     kit, FileName::fromLatin1("blackberry-x86-qcc"));
        } else {
            QmakeProjectManager::QmakeKitInformation::setMkspec(
                     kit, FileName::fromLatin1("blackberry-armv7le-qcc"));
        }
    }

    DeviceTypeKitInformation::setDeviceTypeId(kit, Constants::QNX_BB_OS_TYPE);
    SysRootKitInformation::setSysRoot(kit, m_sysRoot);

    kit->setUnexpandedDisplayName(version->displayName());
    kit->setIconPath(FileName::fromString(QLatin1String(Constants::QNX_BB_CATEGORY_ICON)));

    kit->setAutoDetected(true);
    kit->setAutoDetectionSource(envFile().toString());
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

QStringList BlackBerryApiLevelConfiguration::validationErrors() const
{
    QStringList errorStrings = QnxBaseConfiguration::validationErrors();
    if (m_qmake4BinaryFile.isEmpty() && m_qmake5BinaryFile.isEmpty())
        errorStrings << tr("- No Qt version found.");

    if (!m_autoDetectionSource.isEmpty() && !m_autoDetectionSource.toFileInfo().exists())
        errorStrings << tr("- No auto detection source found.");

    if (m_sysRoot.isEmpty() && m_sysRoot.toFileInfo().exists())
        errorStrings << tr("- No sysroot found.");


    return errorStrings;
}

bool BlackBerryApiLevelConfiguration::activate()
{
    if (!isValid()) {
        if (!m_autoDetectionSource.isEmpty())
            return false;

        QString errorMessage = tr("The following errors occurred while activating target \"%1\":\n").arg(m_targetName);
        errorMessage.append(validationErrors().join(QLatin1Char('\n')));
        QMessageBox::warning(Core::ICore::mainWindow(), tr("Cannot Set up BB10 Configuration"),
                             errorMessage, QMessageBox::Ok);
        return false;
    }

    if (isActive())
        return true;

    deactivate(); // cleaning-up artifacts autodetected by old QtCreator versions

    QString armVersionName = tr("BlackBerry %1 Device").arg(version().toString());
    QString x86VersionName = tr("BlackBerry %1 Simulator").arg(version().toString());

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

    // Create tool chains
    QnxToolChain *armvle7ToolChain = createToolChain(ArmLeV7,
                                                     tr("QCC for %1").arg(armVersionName),
                                                     ndkPath());
    QnxToolChain *x86ToolChain = createToolChain(X86,
                                                 tr("QCC for %1").arg(x86VersionName),
                                                 ndkPath());
    // Create debuggers
    QVariant armDebuggerId = createDebuggerItem(ArmLeV7,
                                                tr("Debugger for %1").arg(armVersionName));

    QVariant x86DebuggerId = createDebuggerItem(X86,
                                                tr("Debugger for %1").arg(x86VersionName));

    // create kits
    if (qt4ArmVersion)
        createKit(qt4ArmVersion, armvle7ToolChain, armDebuggerId);
    if (qt4X86Version)
        createKit(qt4X86Version, x86ToolChain, x86DebuggerId);
    if (qt5ArmVersion)
        createKit(qt5ArmVersion, armvle7ToolChain, armDebuggerId);
    if (qt5X86Version)
        createKit(qt5X86Version, x86ToolChain, x86DebuggerId);

    BlackBerryConfigurationManager::instance()->emitSettingsChanged();

    return true;
}

void BlackBerryApiLevelConfiguration::deactivate()
{
    QList<BaseQtVersion *> qtvToRemove;
    QList<ToolChain *> tcToRemove;
    QList<const DebuggerItem *> dbgToRemove;

    foreach (Kit *kit, KitManager::kits()) {
        if (kit->isAutoDetected() &&
                kit->autoDetectionSource() == envFile().toString()) {
            BaseQtVersion *version = QtKitInformation::qtVersion(kit);
            ToolChain *toolChain = ToolChainKitInformation::toolChain(kit);
            const DebuggerItem *debugger = DebuggerKitInformation::debugger(kit);
            // Kit's Qt version, tool chain or debugger might be used by other BB kits
            // generated for the same API level that are not yet unregistered. This triggers warning outputs.
            // Let's unregistered/removed them later once all API level kits are unregistered.
            if (version && !qtvToRemove.contains(version))
                qtvToRemove << version;
            if (toolChain && !tcToRemove.contains(toolChain))
                tcToRemove << toolChain;
            if (debugger && !dbgToRemove.contains(debugger))
                dbgToRemove << debugger;

            KitManager::deregisterKit(kit);
        }
    }

    foreach (BaseQtVersion *qtv, qtvToRemove)
        QtVersionManager::removeVersion(qtv);

    foreach (ToolChain *tc, tcToRemove)
        ToolChainManager::deregisterToolChain(tc);

    foreach (const DebuggerItem *debugger, dbgToRemove)
        DebuggerItemManager::deregisterDebugger(debugger->id());

    BlackBerryConfigurationManager::instance()->emitSettingsChanged();
}

#ifdef WITH_TESTS
void BlackBerryApiLevelConfiguration::setFakeConfig(bool fakeConfig)
{
    m_fakeConfig = fakeConfig;
}

bool BlackBerryApiLevelConfiguration::fakeConfig()
{
    return m_fakeConfig;
}

#endif

} // namespace Internal
} // namespace Qnx
