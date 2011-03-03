/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "s60deployconfiguration.h"
#include "s60deployconfigurationwidget.h"
#include "s60manager.h"
#include "qt4project.h"
#include "qt4target.h"
#include "qt4projectmanagerconstants.h"
#include "qt4buildconfiguration.h"
#include "qt4symbiantarget.h"
#include "s60createpackagestep.h"
#include "s60deploystep.h"

#include <utils/qtcassert.h>
#include <symbianutils/symbiandevicemanager.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/project.h>
#include <projectexplorer/toolchain.h>

#include <QtCore/QFileInfo>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

namespace {
const char * const S60_DC_ID("Qt4ProjectManager.S60DeployConfiguration");
const char * const S60_DC_PREFIX("Qt4ProjectManager.S60DeployConfiguration.");

const char * const SERIAL_PORT_NAME_KEY("Qt4ProjectManager.S60DeployConfiguration.SerialPortName");
const char * const INSTALLATION_DRIVE_LETTER_KEY("Qt4ProjectManager.S60DeployConfiguration.InstallationDriveLetter");
const char * const SILENT_INSTALL_KEY("Qt4ProjectManager.S60DeployConfiguration.SilentInstall");
const char * const DEVICE_ADDRESS_KEY("Qt4ProjectManager.S60DeployConfiguration.DeviceAddress");
const char * const DEVICE_PORT_KEY("Qt4ProjectManager.S60DeployConfiguration.DevicePort");
const char * const COMMUNICATION_CHANNEL_KEY("Qt4ProjectManager.S60DeployConfiguration.CommunicationChannel");

const char * const DEFAULT_TCF_TRK_TCP_PORT("65029");

QString pathFromId(const QString &id)
{
    if (!id.startsWith(QLatin1String(S60_DC_PREFIX)))
        return QString();
    return id.mid(QString::fromLatin1(S60_DC_PREFIX).size());
}

QString pathToId(const QString &path)
{
    return QString::fromLatin1(S60_DC_PREFIX) + path;
}
}

// ======== S60DeployConfiguration

S60DeployConfiguration::S60DeployConfiguration(Target *parent) :
    DeployConfiguration(parent,  QLatin1String(S60_DC_ID)),
    m_activeBuildConfiguration(0),
#ifdef Q_OS_WIN
    m_serialPortName(QLatin1String("COM5")),
#else
    m_serialPortName(QLatin1String(SymbianUtils::SymbianDeviceManager::linuxBlueToothDeviceRootC) + QLatin1Char('0')),
#endif
    m_installationDrive('C'),
    m_silentInstall(true),
    m_devicePort(QLatin1String(DEFAULT_TCF_TRK_TCP_PORT)),
    m_communicationChannel(CommunicationTrkSerialConnection)
{
    ctor();
}

S60DeployConfiguration::S60DeployConfiguration(Target *target, S60DeployConfiguration *source) :
    DeployConfiguration(target, source),
    m_activeBuildConfiguration(0),
    m_serialPortName(source->m_serialPortName),
    m_installationDrive(source->m_installationDrive),
    m_silentInstall(source->m_silentInstall),
    m_deviceAddress(source->m_deviceAddress),
    m_devicePort(source->m_devicePort),
    m_communicationChannel(source->m_communicationChannel)
{
    ctor();
}

void S60DeployConfiguration::ctor()
{
    setDefaultDisplayName(defaultDisplayName());
    // TODO disable S60 Deploy Configuration while parsing
    // requires keeping track of the parsing state of the project
//    connect(qt4Target()->qt4Project(), SIGNAL(proFileInvalidated(Qt4ProjectManager::Internal::Qt4ProFileNode*)),
//            this, SLOT(targetInformationInvalidated()));
    connect(qt4Target()->qt4Project(), SIGNAL(proFileUpdated(Qt4ProjectManager::Internal::Qt4ProFileNode*,bool)),
            this, SIGNAL(targetInformationChanged()));
    connect(qt4Target(), SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
            this, SLOT(updateActiveBuildConfiguration(ProjectExplorer::BuildConfiguration*)));
    connect(qt4Target(), SIGNAL(activeRunConfigurationChanged(ProjectExplorer::RunConfiguration*)),
            this, SLOT(updateActiveRunConfiguration(ProjectExplorer::RunConfiguration*)));
    updateActiveBuildConfiguration(qt4Target()->activeBuildConfiguration());
}

S60DeployConfiguration::~S60DeployConfiguration()
{
}

ProjectExplorer::DeployConfigurationWidget *S60DeployConfiguration::configurationWidget() const
{
    return new S60DeployConfigurationWidget();
}

bool S60DeployConfiguration::isStaticLibrary(const Qt4ProFileNode &projectNode) const
{
    if (projectNode.projectType() == LibraryTemplate) {
        const QStringList &config(projectNode.variableValue(ConfigVar));
        if (config.contains(QLatin1String("static")) || config.contains(QLatin1String("staticlib")))
            return true;
    }
    return false;
}

QStringList S60DeployConfiguration::signedPackages() const
{
    QList<Qt4ProFileNode *> list = qt4Target()->qt4Project()->allProFiles();
    QStringList result;
    foreach (Qt4ProFileNode *node, list) {
        if (isStaticLibrary(*node)) //no sis package
            continue;
        TargetInformation ti = node->targetInformation();
        if (ti.valid)
            result << ti.buildDir + QLatin1Char('/') + createPackageName(ti.target);
    }
    return result;
}

QString S60DeployConfiguration::createPackageName(const QString &baseName) const
{
    QString name(baseName);
    name += isSigned() ? QLatin1String("") : QLatin1String("_unsigned");
    name += runSmartInstaller() ? QLatin1String("_installer") : QLatin1String("");
    name += QLatin1String(".sis");
    return name;
}

QStringList S60DeployConfiguration::packageFileNamesWithTargetInfo() const
{
    QList<Qt4ProFileNode *> leafs = qt4Target()->qt4Project()->allProFiles();
    QStringList result;
    foreach (Qt4ProFileNode *qt4ProFileNode, leafs) {
        if (isStaticLibrary(*qt4ProFileNode)) //no sis package
            continue;
        TargetInformation ti = qt4ProFileNode->targetInformation();
        if (!ti.valid)
            continue;
        QString baseFileName = ti.buildDir + QLatin1Char('/') + ti.target;
        baseFileName += QLatin1Char('_')
                + (isDebug() ? QLatin1String("debug") : QLatin1String("release"))
                + QLatin1Char('-') + S60Manager::platform(qt4Target()->activeBuildConfiguration()->toolChain()) + QLatin1String(".sis");
        result << baseFileName;
    }
    return result;
}

QStringList S60DeployConfiguration::packageTemplateFileNames() const
{
    QList<Qt4ProFileNode *> list = qt4Target()->qt4Project()->allProFiles();
    QStringList result;
    foreach (Qt4ProFileNode *node, list) {
        if (isStaticLibrary(*node)) //no sis package
            continue;
        TargetInformation ti = node->targetInformation();
        if (ti.valid)
            result << ti.buildDir + QLatin1Char('/') + ti.target + QLatin1String("_template.pkg");
    }
    return result;
}

QStringList S60DeployConfiguration::appPackageTemplateFileNames() const
{
    QList<Qt4ProFileNode *> list = qt4Target()->qt4Project()->allProFiles();
    QStringList result;
    foreach (Qt4ProFileNode *node, list) {
        if (isStaticLibrary(*node)) //no sis package
            continue;
        TargetInformation ti = node->targetInformation();
        if (ti.valid)
            result << ti.buildDir + QLatin1Char('/') + ti.target + QLatin1String("_template.pkg");
    }
    return result;
}

bool S60DeployConfiguration::runSmartInstaller() const
{
    DeployConfiguration *dc = target()->activeDeployConfiguration();
    QTC_ASSERT(dc, return false);
    BuildStepList *bsl = dc->stepList();
    QTC_ASSERT(bsl, return false);
    QList<BuildStep *> steps = bsl->steps();
    foreach (const BuildStep *step, steps) {
        if (const S60CreatePackageStep *packageStep = qobject_cast<const S60CreatePackageStep *>(step)) {
            return packageStep->createsSmartInstaller();
        }
    }
    return false;
}

bool S60DeployConfiguration::isSigned() const
{
    DeployConfiguration *dc = target()->activeDeployConfiguration();
    QTC_ASSERT(dc, return false);
    BuildStepList *bsl = dc->stepList();
    QTC_ASSERT(bsl, return false);
    QList<BuildStep *> steps = bsl->steps();
    foreach (const BuildStep *step, steps) {
        if (const S60CreatePackageStep *packageStep = qobject_cast<const S60CreatePackageStep *>(step)) {
            return packageStep->signingMode() != S60CreatePackageStep::NotSigned;
        }
    }
    return false;
}

ProjectExplorer::ToolChain *S60DeployConfiguration::toolChain() const
{
    if (Qt4BuildConfiguration *bc = qobject_cast<Qt4BuildConfiguration *>(target()->activeBuildConfiguration()))
        return bc->toolChain();
    return 0;
}

bool S60DeployConfiguration::isDebug() const
{
    const Qt4BuildConfiguration *qt4bc = qt4Target()->activeBuildConfiguration();
    return (qt4bc->qmakeBuildConfiguration() & QtVersion::DebugBuild);
}

QString S60DeployConfiguration::symbianTarget() const
{
    return isDebug() ? QLatin1String("udeb") : QLatin1String("urel");
}

const QtVersion *S60DeployConfiguration::qtVersion() const
{
    if (const Qt4BuildConfiguration *qt4bc = qt4Target()->activeBuildConfiguration())
        return qt4bc->qtVersion();
    return 0;
}

void S60DeployConfiguration::updateActiveBuildConfiguration(ProjectExplorer::BuildConfiguration *buildConfiguration)
{
    if (m_activeBuildConfiguration)
        disconnect(m_activeBuildConfiguration, SIGNAL(s60CreatesSmartInstallerChanged()),
                   this, SIGNAL(targetInformationChanged()));
    m_activeBuildConfiguration = buildConfiguration;
    if (m_activeBuildConfiguration)
        connect(m_activeBuildConfiguration, SIGNAL(s60CreatesSmartInstallerChanged()),
                this, SIGNAL(targetInformationChanged()));
}

void S60DeployConfiguration::updateActiveRunConfiguration(ProjectExplorer::RunConfiguration *runConfiguration)
{
    Q_UNUSED(runConfiguration);
    setDefaultDisplayName(defaultDisplayName());
}

QVariantMap S60DeployConfiguration::toMap() const
{
    QVariantMap map(ProjectExplorer::DeployConfiguration::toMap());
    map.insert(QLatin1String(SERIAL_PORT_NAME_KEY), m_serialPortName);
    map.insert(QLatin1String(INSTALLATION_DRIVE_LETTER_KEY), QChar(m_installationDrive));
    map.insert(QLatin1String(SILENT_INSTALL_KEY), QVariant(m_silentInstall));
    map.insert(QLatin1String(DEVICE_ADDRESS_KEY), QVariant(m_deviceAddress));
    map.insert(QLatin1String(DEVICE_PORT_KEY), m_devicePort);
    map.insert(QLatin1String(COMMUNICATION_CHANNEL_KEY), QVariant(m_communicationChannel));

    return map;
}

QString S60DeployConfiguration::defaultDisplayName() const
{
    QList<Qt4ProFileNode *> list = qt4Target()->qt4Project()->allProFiles();
    foreach (Qt4ProFileNode *node, list) {
        TargetInformation ti = node->targetInformation();
        if (ti.valid && !ti.buildDir.isEmpty())
            return tr("Deploy %1 to Symbian device").arg(QFileInfo(ti.buildDir).completeBaseName());
    }
    return tr("Deploy to Symbian device");
}

bool S60DeployConfiguration::fromMap(const QVariantMap &map)
{
    if (!DeployConfiguration::fromMap(map))
        return false;
    m_serialPortName = map.value(QLatin1String(SERIAL_PORT_NAME_KEY)).toString().trimmed();
    m_installationDrive = map.value(QLatin1String(INSTALLATION_DRIVE_LETTER_KEY), QChar('C'))
                          .toChar().toAscii();
    m_silentInstall = map.value(QLatin1String(SILENT_INSTALL_KEY), QVariant(true)).toBool();
    m_deviceAddress = map.value(QLatin1String(DEVICE_ADDRESS_KEY)).toString();
    m_devicePort = map.value(QLatin1String(DEVICE_PORT_KEY), QString(QLatin1String(DEFAULT_TCF_TRK_TCP_PORT))).toString();
    m_communicationChannel = static_cast<CommunicationChannel>(map.value(QLatin1String(COMMUNICATION_CHANNEL_KEY),
                                                                         QVariant(CommunicationTrkSerialConnection)).toInt());

    setDefaultDisplayName(defaultDisplayName());
    return true;
}

Qt4SymbianTarget *S60DeployConfiguration::qt4Target() const
{
    return static_cast<Qt4SymbianTarget *>(target());
}

QString S60DeployConfiguration::serialPortName() const
{
    return m_serialPortName;
}

void S60DeployConfiguration::setSerialPortName(const QString &name)
{
    const QString &candidate = name.trimmed();
    if (m_serialPortName == candidate)
        return;
    m_serialPortName = candidate;
    emit serialPortNameChanged();
}

char S60DeployConfiguration::installationDrive() const
{
    return m_installationDrive;
}

void S60DeployConfiguration::setInstallationDrive(char drive)
{
    if (m_installationDrive == drive)
        return;
    m_installationDrive = drive;
    emit installationDriveChanged();
}

bool S60DeployConfiguration::silentInstall() const
{
    return m_silentInstall;
}

void S60DeployConfiguration::setSilentInstall(bool silent)
{
    m_silentInstall = silent;
}

QString S60DeployConfiguration::deviceAddress() const
{
    return m_deviceAddress;
}

void S60DeployConfiguration::setDeviceAddress(const QString &address)
{
    if (m_deviceAddress != address) {
        m_deviceAddress = address;
        emit deviceAddressChanged();
    }
}

QString S60DeployConfiguration::devicePort() const
{
    return m_devicePort;
}

void S60DeployConfiguration::setDevicePort(const QString &port)
{
    if (m_devicePort != port) {
        if (port.isEmpty()) //setup the default CODA's port
            m_devicePort = QLatin1String(DEFAULT_TCF_TRK_TCP_PORT);
        else
            m_devicePort = port;
        emit devicePortChanged();
    }
}

S60DeployConfiguration::CommunicationChannel S60DeployConfiguration::communicationChannel() const
{
    return m_communicationChannel;
}

void S60DeployConfiguration::setCommunicationChannel(CommunicationChannel channel)
{
    if (m_communicationChannel != channel) {
        m_communicationChannel = channel;
        emit communicationChannelChanged();
    }
}

void S60DeployConfiguration::setAvailableDeviceDrives(QList<DeviceDrive> drives)
{
    m_availableDeviceDrives = drives;
    emit availableDeviceDrivesChanged();
}

const QList<S60DeployConfiguration::DeviceDrive> &S60DeployConfiguration::availableDeviceDrives() const
{
    return m_availableDeviceDrives;
}

// ======== S60DeployConfigurationFactory

S60DeployConfigurationFactory::S60DeployConfigurationFactory(QObject *parent) :
    DeployConfigurationFactory(parent)
{
}

S60DeployConfigurationFactory::~S60DeployConfigurationFactory()
{
}

QStringList S60DeployConfigurationFactory::availableCreationIds(Target *parent) const
{
    Qt4SymbianTarget *target = qobject_cast<Qt4SymbianTarget *>(parent);
    if (!target ||
        target->id() != QLatin1String(Constants::S60_DEVICE_TARGET_ID))
        return QStringList();

    return target->qt4Project()->applicationProFilePathes(QLatin1String(S60_DC_PREFIX));
}

QString S60DeployConfigurationFactory::displayNameForId(const QString &id) const
{
    if (!pathFromId(id).isEmpty())
        return tr("%1 on Symbian Device").arg(QFileInfo(pathFromId(id)).completeBaseName());
    return QString();
}

DeployConfiguration *S60DeployConfigurationFactory::create(Target *parent, const QString &id)
{
    if (!canCreate(parent, id))
        return 0;

    Qt4SymbianTarget *t = static_cast<Qt4SymbianTarget *>(parent);
    S60DeployConfiguration *dc = new S60DeployConfiguration(t);

    dc->setDefaultDisplayName(tr("Deploy to Symbian device"));
    dc->stepList()->insertStep(0, new S60CreatePackageStep(dc->stepList()));
    dc->stepList()->insertStep(1, new S60DeployStep(dc->stepList()));
    return dc;
}

bool S60DeployConfigurationFactory::canCreate(Target *parent, const QString& /*id*/) const
{
    Qt4SymbianTarget * t = qobject_cast<Qt4SymbianTarget *>(parent);
    if (!t || t->id() != QLatin1String(Constants::S60_DEVICE_TARGET_ID))
        return false;
    return true;
}

bool S60DeployConfigurationFactory::canRestore(Target *parent, const QVariantMap& /*map*/) const
{
    Qt4SymbianTarget * t = qobject_cast<Qt4SymbianTarget *>(parent);
    return t && t->id() == QLatin1String(Constants::S60_DEVICE_TARGET_ID);
}

DeployConfiguration *S60DeployConfigurationFactory::restore(Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    Qt4SymbianTarget *t = static_cast<Qt4SymbianTarget *>(parent);
    S60DeployConfiguration *dc = new S60DeployConfiguration(t);
    if (dc->fromMap(map))
        return dc;

    delete dc;
    return 0;
}

bool S60DeployConfigurationFactory::canClone(Target *parent, DeployConfiguration *source) const
{
    if (!qobject_cast<Qt4SymbianTarget *>(parent))
        return false;
    return source->id() == QLatin1String(S60_DC_ID);
}

DeployConfiguration *S60DeployConfigurationFactory::clone(Target *parent, DeployConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    Qt4SymbianTarget *t = static_cast<Qt4SymbianTarget *>(parent);
    S60DeployConfiguration * old = static_cast<S60DeployConfiguration *>(source);
    return new S60DeployConfiguration(t, old);
}
