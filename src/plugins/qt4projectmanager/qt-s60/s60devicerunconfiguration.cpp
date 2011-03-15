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

#include "s60devicerunconfiguration.h"
#include "s60devicerunconfigurationwidget.h"
#include "s60deployconfiguration.h"
#include "qt4project.h"
#include "qt4target.h"
#include "s60manager.h"
#include "s60runconfigbluetoothstarter.h"
#include "qt4projectmanagerconstants.h"
#include "qtoutputformatter.h"
#include "qt4symbiantarget.h"

#include <utils/qtcassert.h>

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <debugger/debuggerengine.h>
#include <debugger/debuggerstartparameters.h>

#include <QtGui/QMessageBox>
#include <QtGui/QMainWindow>
#include <QtCore/QFileInfo>
#include <QtCore/QDateTime>
#include <QtCore/QDir>

#include <QtNetwork/QTcpSocket>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

namespace {

const char * const S60_DEVICE_RC_ID("Qt4ProjectManager.S60DeviceRunConfiguration");
const char * const S60_DEVICE_RC_PREFIX("Qt4ProjectManager.S60DeviceRunConfiguration.");

const char * const PRO_FILE_KEY("Qt4ProjectManager.S60DeviceRunConfiguration.ProFile");
const char * const COMMUNICATION_TYPE_KEY("Qt4ProjectManager.S60DeviceRunConfiguration.CommunicationType");
const char * const COMMAND_LINE_ARGUMENTS_KEY("Qt4ProjectManager.S60DeviceRunConfiguration.CommandLineArguments");

enum { debug = 0 };

QString pathFromId(const QString &id)
{
    if (!id.startsWith(QLatin1String(S60_DEVICE_RC_PREFIX)))
        return QString();
    return id.mid(QString::fromLatin1(S60_DEVICE_RC_PREFIX).size());
}

QString pathToId(const QString &path)
{
    return QString::fromLatin1(S60_DEVICE_RC_PREFIX) + path;
}

} // anonymous namespace

// ======== S60DeviceRunConfiguration

S60DeviceRunConfiguration::S60DeviceRunConfiguration(Qt4BaseTarget *parent, const QString &proFilePath) :
    RunConfiguration(parent,  QLatin1String(S60_DEVICE_RC_ID)),
    m_proFilePath(proFilePath),
    m_validParse(parent->qt4Project()->validParse(proFilePath))
{
    ctor();
}

S60DeviceRunConfiguration::S60DeviceRunConfiguration(Qt4BaseTarget *target, S60DeviceRunConfiguration *source) :
    RunConfiguration(target, source),
    m_proFilePath(source->m_proFilePath),
    m_commandLineArguments(source->m_commandLineArguments),
    m_validParse(source->m_validParse)
{
    ctor();
}

void S60DeviceRunConfiguration::ctor()
{
    if (!m_proFilePath.isEmpty())
        //: S60 device runconfiguration default display name, %1 is base pro-File name
        setDefaultDisplayName(tr("%1 on Symbian Device").arg(QFileInfo(m_proFilePath).completeBaseName()));
    else
        //: S60 device runconfiguration default display name (no profile set)
        setDefaultDisplayName(tr("Run on Symbian device"));

    Qt4Project *pro = qt4Target()->qt4Project();
    connect(pro, SIGNAL(proFileUpdated(Qt4ProjectManager::Internal::Qt4ProFileNode*,bool)),
            this, SLOT(proFileUpdate(Qt4ProjectManager::Internal::Qt4ProFileNode*,bool)));
    connect(pro, SIGNAL(proFileInvalidated(Qt4ProjectManager::Internal::Qt4ProFileNode *)),
            this, SLOT(proFileInvalidated(Qt4ProjectManager::Internal::Qt4ProFileNode *)));
}

void S60DeviceRunConfiguration::handleParserState(bool success)
{
    bool enabled = isEnabled();
    m_validParse = success;
    if (enabled != isEnabled())
        emit isEnabledChanged(!enabled);
}

void S60DeviceRunConfiguration::proFileInvalidated(Qt4ProjectManager::Internal::Qt4ProFileNode *pro)
{
    if (m_proFilePath != pro->path())
        return;
    handleParserState(false);
}

void S60DeviceRunConfiguration::proFileUpdate(Qt4ProjectManager::Internal::Qt4ProFileNode *pro, bool success)
{
    if (m_proFilePath != pro->path())
        return;
    handleParserState(success);
    emit targetInformationChanged();
}

S60DeviceRunConfiguration::~S60DeviceRunConfiguration()
{
}

Qt4SymbianTarget *S60DeviceRunConfiguration::qt4Target() const
{
    return static_cast<Qt4SymbianTarget *>(target());
}

bool S60DeviceRunConfiguration::isEnabled(ProjectExplorer::BuildConfiguration *configuration) const
{
    if (!m_validParse)
        return false;

    Q_ASSERT(configuration->target() == target());
    Q_ASSERT(target()->id() == Constants::S60_DEVICE_TARGET_ID);

    const Qt4BuildConfiguration *qt4bc = qobject_cast<const Qt4BuildConfiguration *>(configuration);
    return qt4bc && qt4bc->toolChain();
}

QWidget *S60DeviceRunConfiguration::createConfigurationWidget()
{
    return new S60DeviceRunConfigurationWidget(this);
}

ProjectExplorer::OutputFormatter *S60DeviceRunConfiguration::createOutputFormatter() const
{
    return new QtOutputFormatter(qt4Target()->qt4Project());
}

QVariantMap S60DeviceRunConfiguration::toMap() const
{
    QVariantMap map = ProjectExplorer::RunConfiguration::toMap();
    const QDir projectDir = QDir(target()->project()->projectDirectory());

    map.insert(QLatin1String(PRO_FILE_KEY), projectDir.relativeFilePath(m_proFilePath));
    map.insert(QLatin1String(COMMAND_LINE_ARGUMENTS_KEY), m_commandLineArguments);

    return map;
}

bool S60DeviceRunConfiguration::fromMap(const QVariantMap &map)
{
    const QDir projectDir = QDir(target()->project()->projectDirectory());

    m_proFilePath = projectDir.filePath(map.value(QLatin1String(PRO_FILE_KEY)).toString());
    m_commandLineArguments = map.value(QLatin1String(COMMAND_LINE_ARGUMENTS_KEY)).toString();

    if (m_proFilePath.isEmpty())
        return false;
    if (!QFileInfo(m_proFilePath).exists())
        return false;

    m_validParse = qt4Target()->qt4Project()->validParse(m_proFilePath);

    setDefaultDisplayName(tr("%1 on Symbian Device").arg(QFileInfo(m_proFilePath).completeBaseName()));

    return RunConfiguration::fromMap(map);
}

static inline QString fixBaseNameTarget(const QString &in)
{
    if (in == QLatin1String("udeb"))
        return QLatin1String("debug");
    if (in == QLatin1String("urel"))
        return QLatin1String("release");
    return in;
}

QString S60DeviceRunConfiguration::targetName() const
{
    TargetInformation ti = qt4Target()->qt4Project()->rootProjectNode()->targetInformation(projectFilePath());
    if (!ti.valid)
        return QString();
    return ti.target;
}

const QtVersion *S60DeviceRunConfiguration::qtVersion() const
{
    if (const BuildConfiguration *bc = target()->activeBuildConfiguration())
        if (const Qt4BuildConfiguration *qt4bc = qobject_cast<const Qt4BuildConfiguration *>(bc))
            return qt4bc->qtVersion();
    return 0;
}

bool S60DeviceRunConfiguration::isDebug() const
{
    const Qt4BuildConfiguration *qt4bc = qt4Target()->activeBuildConfiguration();
    return (qt4bc->qmakeBuildConfiguration() & QtVersion::DebugBuild);
}

QString S60DeviceRunConfiguration::symbianTarget() const
{
    return isDebug() ? QLatin1String("udeb") : QLatin1String("urel");
}

/* Grep a package file for the '.exe' file. Currently for use on Linux only
 * as the '.pkg'-files on Windows do not contain drive letters, which is not
 * handled here. \code
; Executable and default resource files
"./foo.exe"    - "!:\sys\bin\foo.exe"
\endcode  */

static inline QString executableFromPackageUnix(const QString &packageFileName)
{
    QFile packageFile(packageFileName);
    if (!packageFile.open(QIODevice::ReadOnly|QIODevice::Text))
        return QString();
    QRegExp pattern(QLatin1String("^\"(.*.exe)\" *- \"!:.*.exe\"$"));
    QTC_ASSERT(pattern.isValid(), return QString());
    foreach(const QString &line, QString::fromLocal8Bit(packageFile.readAll()).split(QLatin1Char('\n')))
        if (pattern.exactMatch(line)) {
            // Expand relative paths by package file paths
            QString rc = pattern.cap(1);
            if (rc.startsWith(QLatin1String("./")))
                rc.remove(0, 2);
            const QFileInfo fi(rc);
            if (fi.isAbsolute())
                return rc;
            return QFileInfo(packageFileName).absolutePath() + QLatin1Char('/') + rc;
        }
    return QString();
}

// ABLD/Raptor: Return executable from device/EPOC
static inline QString localExecutableFromVersion(const QtVersion *qtv,
                                                const QString &symbianTarget, /* udeb/urel */
                                                const QString &targetName,
                                                const ProjectExplorer::ToolChain *tc)
{
    Q_ASSERT(qtv);
    if (!tc)
        return QString();

    QString localExecutable;
    QString platform = S60Manager::platform(tc);
    if (qtv->isBuildWithSymbianSbsV2() && platform == QLatin1String("gcce"))
        platform = "armv5";
    QTextStream(&localExecutable) << qtv->systemRoot() << "/epoc32/release/"
            << platform << '/' << symbianTarget << '/' << targetName << ".exe";
    return localExecutable;
}

QString S60DeviceRunConfiguration::localExecutableFileName() const
{
    TargetInformation ti = qt4Target()->qt4Project()->rootProjectNode()->targetInformation(projectFilePath());
    if (!ti.valid)
        return QString();

    ProjectExplorer::ToolChain *tc = target()->activeBuildConfiguration()->toolChain();
    return localExecutableFromVersion(qtVersion(), symbianTarget(), targetName(), tc);
}

quint32 S60DeviceRunConfiguration::executableUid() const
{
    quint32 uid = 0;
    QString executablePath = localExecutableFileName();
    if (!executablePath.isEmpty()) {
        QFile file(executablePath);
        if (file.open(QIODevice::ReadOnly)) {
            // executable's UID is 4 bytes starting at 8.
            const QByteArray data = file.read(12);
            if (data.size() == 12) {
                const unsigned char *d = reinterpret_cast<const unsigned char*>(data.data() + 8);
                uid = *d++;
                uid += *d++ << 8;
                uid += *d++ << 16;
                uid += *d++ << 24;
            }
        }
    }
    return uid;
}

QString S60DeviceRunConfiguration::projectFilePath() const
{
    return m_proFilePath;
}

QString S60DeviceRunConfiguration::commandLineArguments() const
{
    return m_commandLineArguments;
}

void S60DeviceRunConfiguration::setCommandLineArguments(const QString &args)
{
    m_commandLineArguments = args;
}

QString S60DeviceRunConfiguration::proFilePath() const
{
    return m_proFilePath;
}

// ======== S60DeviceRunConfigurationFactory

S60DeviceRunConfigurationFactory::S60DeviceRunConfigurationFactory(QObject *parent) :
    IRunConfigurationFactory(parent)
{
}

S60DeviceRunConfigurationFactory::~S60DeviceRunConfigurationFactory()
{
}

QStringList S60DeviceRunConfigurationFactory::availableCreationIds(Target *parent) const
{
    Qt4SymbianTarget *target = qobject_cast<Qt4SymbianTarget *>(parent);
    if (!target || target->id() != QLatin1String(Constants::S60_DEVICE_TARGET_ID))
        return QStringList();

    return target->qt4Project()->applicationProFilePathes(QLatin1String(S60_DEVICE_RC_PREFIX));
}

QString S60DeviceRunConfigurationFactory::displayNameForId(const QString &id) const
{
    if (!pathFromId(id).isEmpty())
        return tr("%1 on Symbian Device").arg(QFileInfo(pathFromId(id)).completeBaseName());
    return QString();
}

bool S60DeviceRunConfigurationFactory::canCreate(Target *parent, const QString &id) const
{
    Qt4SymbianTarget *t = qobject_cast<Qt4SymbianTarget *>(parent);
    if (!t || t->id() != QLatin1String(Constants::S60_DEVICE_TARGET_ID))
        return false;
    return t->qt4Project()->hasApplicationProFile(pathFromId(id));
}

RunConfiguration *S60DeviceRunConfigurationFactory::create(Target *parent, const QString &id)
{
    if (!canCreate(parent, id))
        return 0;

    Qt4SymbianTarget *t = static_cast<Qt4SymbianTarget *>(parent);
    return new S60DeviceRunConfiguration(t, pathFromId(id));
}

bool S60DeviceRunConfigurationFactory::canRestore(Target *parent, const QVariantMap &map) const
{
    Qt4SymbianTarget *t = qobject_cast<Qt4SymbianTarget *>(parent);
    if (!t || t->id() != QLatin1String(Constants::S60_DEVICE_TARGET_ID))
        return false;
    QString id = ProjectExplorer::idFromMap(map);
    return id == QLatin1String(S60_DEVICE_RC_ID);
}

RunConfiguration *S60DeviceRunConfigurationFactory::restore(Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    Qt4SymbianTarget *t = static_cast<Qt4SymbianTarget *>(parent);
    S60DeviceRunConfiguration *rc = new S60DeviceRunConfiguration(t, QString());
    if (rc->fromMap(map))
        return rc;

    delete rc;
    return 0;
}

bool S60DeviceRunConfigurationFactory::canClone(Target *parent, RunConfiguration *source) const
{
    if (!qobject_cast<Qt4SymbianTarget *>(parent))
        return false;
    return source->id() == QLatin1String(S60_DEVICE_RC_ID);
}

RunConfiguration *S60DeviceRunConfigurationFactory::clone(Target *parent, RunConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    Qt4SymbianTarget *t = static_cast<Qt4SymbianTarget *>(parent);
    S60DeviceRunConfiguration *old = static_cast<S60DeviceRunConfiguration *>(source);
    return new S60DeviceRunConfiguration(t, old);
}

// ======== S60DeviceDebugRunControl

// Return symbol file which should co-exist with the executable.
// location in debug builds. This can be 'foo.sym' (ABLD) or 'foo.exe.sym' (Raptor)
static inline QString symbolFileFromExecutable(const QString &executable)
{
    // 'foo.exe.sym' (Raptor)
    const QFileInfo raptorSymFi(executable + QLatin1String(".sym"));
    if (raptorSymFi.isFile())
        return raptorSymFi.absoluteFilePath();
    // 'foo.sym' (ABLD)
    const int lastDotPos = executable.lastIndexOf(QLatin1Char('.'));
    if (lastDotPos != -1) {
        const QString symbolFileName = executable.mid(0, lastDotPos) + QLatin1String(".sym");
        const QFileInfo symbolFileNameFi(symbolFileName);
        if (symbolFileNameFi.isFile())
            return symbolFileNameFi.absoluteFilePath();
    }
    return QString();
}

// Create start parameters from run configuration
static Debugger::DebuggerStartParameters s60DebuggerStartParams(const S60DeviceRunConfiguration *rc)
{
    Debugger::DebuggerStartParameters sp;
    QTC_ASSERT(rc, return sp);

    const S60DeployConfiguration *activeDeployConf =
        qobject_cast<S60DeployConfiguration *>(rc->qt4Target()->activeDeployConfiguration());

    const QString debugFileName = QString::fromLatin1("%1:\\sys\\bin\\%2.exe")
            .arg(activeDeployConf->installationDrive()).arg(rc->targetName());

    sp.remoteChannel = activeDeployConf->serialPortName();
    sp.processArgs = rc->commandLineArguments();
    sp.startMode = Debugger::StartInternal;
    sp.toolChainAbi = rc->abi();
    sp.executable = debugFileName;
    sp.executableUid = rc->executableUid();
    sp.serverAddress = activeDeployConf->deviceAddress();
    sp.serverPort = activeDeployConf->devicePort().toInt();

    sp.communicationChannel = activeDeployConf->communicationChannel() == S60DeployConfiguration::CommunicationCodaTcpConnection?
                Debugger::DebuggerStartParameters::CommunicationChannelTcpIp:
                Debugger::DebuggerStartParameters::CommunicationChannelUsb;

    sp.debugClient = activeDeployConf->communicationChannel() == S60DeployConfiguration::CommunicationTrkSerialConnection?
                Debugger::DebuggerStartParameters::DebugClientTrk:
                Debugger::DebuggerStartParameters::DebugClientCoda;

    QTC_ASSERT(sp.executableUid, return sp);

    // Prefer the '*.sym' file over the '.exe', which should exist at the same
    // location in debug builds. This can be 'foo.exe' (ABLD) or 'foo.exe.sym' (Raptor)
    sp.symbolFileName = symbolFileFromExecutable(rc->localExecutableFileName());
    return sp;
}

S60DeviceDebugRunControl::S60DeviceDebugRunControl(S60DeviceRunConfiguration *rc,
                                                   const Debugger::DebuggerStartParameters &sp,
                                                   const QPair<Debugger::DebuggerEngineType, Debugger::DebuggerEngineType> &masterSlaveEngineTypes) :
    Debugger::DebuggerRunControl(rc, sp, masterSlaveEngineTypes)
{
    if (startParameters().symbolFileName.isEmpty()) {
        const QString msg = tr("Warning: Cannot locate the symbol file belonging to %1.").
                               arg(rc->localExecutableFileName());
        appendMessage(msg, ErrorMessageFormat);
    }
}

void S60DeviceDebugRunControl::start()
{
    appendMessage(tr("Launching debugger..."), NormalMessageFormat);
    Debugger::DebuggerRunControl::start();
}

bool S60DeviceDebugRunControl::promptToStop(bool *) const
{
    // We override the settings prompt
    return Debugger::DebuggerRunControl::promptToStop(0);
}

S60DeviceDebugRunControlFactory::S60DeviceDebugRunControlFactory(QObject *parent) :
    IRunControlFactory(parent)
{
}

bool S60DeviceDebugRunControlFactory::canRun(ProjectExplorer::RunConfiguration *runConfiguration, const QString &mode) const
{
    return mode == QLatin1String(Debugger::Constants::DEBUGMODE)
            && qobject_cast<S60DeviceRunConfiguration *>(runConfiguration) != 0;
}

ProjectExplorer::RunControl* S60DeviceDebugRunControlFactory::create(ProjectExplorer::RunConfiguration *runConfiguration, const QString &mode)
{
    S60DeviceRunConfiguration *rc = qobject_cast<S60DeviceRunConfiguration *>(runConfiguration);
    QTC_ASSERT(rc && mode == QLatin1String(Debugger::Constants::DEBUGMODE), return 0);
    const Debugger::DebuggerStartParameters startParameters = s60DebuggerStartParams(rc);
    const Debugger::ConfigurationCheck check = Debugger::checkDebugConfiguration(startParameters);
    if (!check) {
        Core::ICore::instance()->showWarningWithOptions(S60DeviceDebugRunControl::tr("Debugger for Symbian Platform"),
            check.errorMessage, check.errorDetailsString(), check.settingsCategory, check.settingsPage);
        return 0;
    }
    return new S60DeviceDebugRunControl(rc, startParameters, check.masterSlaveEngineTypes);
}

QString S60DeviceDebugRunControlFactory::displayName() const
{
    return S60DeviceDebugRunControl::tr("Debug on Device");
}

ProjectExplorer::RunConfigWidget *S60DeviceDebugRunControlFactory::createConfigurationWidget(RunConfiguration* /*runConfiguration */)
{
    return 0;
}
