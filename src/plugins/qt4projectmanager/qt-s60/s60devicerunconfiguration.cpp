/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "s60devicerunconfiguration.h"
#include "s60devicerunconfigurationwidget.h"
#include "s60deployconfiguration.h"
#include "qt4project.h"
#include "qt4target.h"
#include "s60manager.h"
#include "s60devices.h"
#include "s60runconfigbluetoothstarter.h"
#include "qt4projectmanagerconstants.h"
#include "qtoutputformatter.h"

#include <symbianutils/bluetoothlistener_gui.h>
#include <symbianutils/launcher.h>
#include <symbianutils/symbiandevicemanager.h>

#include <utils/qtcassert.h>

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <debugger/debuggerengine.h>

#include <QtGui/QMessageBox>
#include <QtGui/QMainWindow>
#include <QtCore/QFileInfo>
#include <QtCore/QDateTime>
#include <QtCore/QDir>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

namespace {
const char * const S60_DEVICE_RC_ID("Qt4ProjectManager.S60DeviceRunConfiguration");
const char * const S60_DEVICE_RC_PREFIX("Qt4ProjectManager.S60DeviceRunConfiguration.");

const char * const PRO_FILE_KEY("Qt4ProjectManager.S60DeviceRunConfiguration.ProFile");
const char * const COMMUNICATION_TYPE_KEY("Qt4ProjectManager.S60DeviceRunConfiguration.CommunicationType");
const char * const COMMAND_LINE_ARGUMENTS_KEY("Qt4ProjectManager.S60DeviceRunConfiguration.CommandLineArguments");

const int    PROGRESS_MAX = 200;

enum { debug = 0 };

// Format information about a file
static inline QString msgListFile(const QString &f)
{
    QString rc;
    const QFileInfo fi(f);
    QTextStream str(&rc);
    if (fi.exists()) {
        str << fi.size() << ' ' << fi.lastModified().toString(Qt::ISODate) << ' ' << QDir::toNativeSeparators(fi.absoluteFilePath());
    } else {
        str << "<non-existent> " << QDir::toNativeSeparators(fi.absoluteFilePath());
    }
    return rc;
}

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

}

// ======== S60DeviceRunConfiguration

S60DeviceRunConfiguration::S60DeviceRunConfiguration(Qt4Target *parent, const QString &proFilePath) :
    RunConfiguration(parent,  QLatin1String(S60_DEVICE_RC_ID)),
    m_proFilePath(proFilePath),
    m_validParse(parent->qt4Project()->validParse(proFilePath))
{
    ctor();
}

S60DeviceRunConfiguration::S60DeviceRunConfiguration(Qt4Target *target, S60DeviceRunConfiguration *source) :
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

Qt4Target *S60DeviceRunConfiguration::qt4Target() const
{
    return static_cast<Qt4Target *>(target());
}

ProjectExplorer::ToolChainType S60DeviceRunConfiguration::toolChainType(
        ProjectExplorer::BuildConfiguration *configuration) const
{
    if (Qt4BuildConfiguration *bc = qobject_cast<Qt4BuildConfiguration *>(configuration))
        return bc->toolChainType();
    return ProjectExplorer::ToolChain_INVALID;
}

ProjectExplorer::ToolChainType S60DeviceRunConfiguration::toolChainType() const
{
    if (Qt4BuildConfiguration *bc = qobject_cast<Qt4BuildConfiguration *>(target()->activeBuildConfiguration()))
        return bc->toolChainType();
    return ProjectExplorer::ToolChain_INVALID;
}

bool S60DeviceRunConfiguration::isEnabled(ProjectExplorer::BuildConfiguration *configuration) const
{
    if (!m_validParse)
        return false;
    const Qt4BuildConfiguration *qt4bc = static_cast<const Qt4BuildConfiguration *>(configuration);
    switch (qt4bc->toolChainType()) {
    case ProjectExplorer::ToolChain_GCCE:
    case ProjectExplorer::ToolChain_RVCT2_ARMV5:
    case ProjectExplorer::ToolChain_RVCT2_ARMV6:
    case ProjectExplorer::ToolChain_RVCT4_ARMV5:
    case ProjectExplorer::ToolChain_RVCT4_ARMV6:
    case ProjectExplorer::ToolChain_GCCE_GNUPOC:
    case ProjectExplorer::ToolChain_RVCT_ARMV5_GNUPOC:
        return true;
    default:
        break;
    }
    return false;
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
    QVariantMap map(ProjectExplorer::RunConfiguration::toMap());
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

static inline QString symbianPlatformForToolChain(ProjectExplorer::ToolChainType t)
{
    switch (t) {
    case ProjectExplorer::ToolChain_GCCE:
    case ProjectExplorer::ToolChain_GCCE_GNUPOC:
        return QLatin1String("gcce");
    case ProjectExplorer::ToolChain_RVCT2_ARMV5:
    case ProjectExplorer::ToolChain_RVCT4_ARMV5:
        return QLatin1String("armv5");
    default: // including ProjectExplorer::RVCT_ARMV6_GNUPOC:
        break;
    }
    return QLatin1String("armv6");
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
static inline QString localExecutableFromDevice(const QtVersion *qtv,
                                                const QString &symbianTarget, /* udeb/urel */
                                                const QString &targetName,
                                                ProjectExplorer::ToolChainType t)
{
    QTC_ASSERT(qtv, return QString(); )

    const S60Devices::Device device = S60Manager::instance()->deviceForQtVersion(qtv);
    QString localExecutable;
    QTextStream(&localExecutable) << device.epocRoot << "/epoc32/release/"
            << symbianPlatformForToolChain(t)
            << '/' << symbianTarget << '/' << targetName
            << ".exe";
    return localExecutable;
}

QString S60DeviceRunConfiguration::localExecutableFileName() const
{
    const ProjectExplorer::ToolChainType toolChain = toolChainType();
    switch (toolChain) {
    case ProjectExplorer::ToolChain_GCCE_GNUPOC:
    case ProjectExplorer::ToolChain_RVCT_ARMV5_GNUPOC: {
        TargetInformation ti = qt4Target()->qt4Project()->rootProjectNode()->targetInformation(projectFilePath());
        if (!ti.valid)
            return QString();
        return executableFromPackageUnix(ti.buildDir + QLatin1Char('/') + ti.target + QLatin1String("_template.pkg"));
    }
    case ProjectExplorer::ToolChain_RVCT2_ARMV5:
    case ProjectExplorer::ToolChain_RVCT2_ARMV6:
        return localExecutableFromDevice(qtVersion(), symbianTarget(), targetName(), toolChain);
        break;
    case ProjectExplorer::ToolChain_GCCE: {
        // As of 4.7.1, qmake-gcce-Raptor builds were changed to put all executables into 'armv5'
        const QtVersion *qtv = qtVersion();
        QTC_ASSERT(qtv, return QString(); )
        return qtv->isBuildWithSymbianSbsV2() ?
            localExecutableFromDevice(qtv, symbianTarget(), targetName(), ProjectExplorer::ToolChain_RVCT2_ARMV5) :
            localExecutableFromDevice(qtv, symbianTarget(), targetName(), toolChain);
    }
        break;
    default:
        break;
    }
    return QString();
}

quint32 S60DeviceRunConfiguration::executableUid() const
{
    quint32 uid = 0;
    QString executablePath(localExecutableFileName());
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
    Qt4Target *target = qobject_cast<Qt4Target *>(parent);
    if (!target ||
        target->id() != QLatin1String(Constants::S60_DEVICE_TARGET_ID))
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
    Qt4Target * t(qobject_cast<Qt4Target *>(parent));
    if (!t ||
        t->id() != QLatin1String(Constants::S60_DEVICE_TARGET_ID))
        return false;
    return t->qt4Project()->hasApplicationProFile(pathFromId(id));
}

RunConfiguration *S60DeviceRunConfigurationFactory::create(Target *parent, const QString &id)
{
    if (!canCreate(parent, id))
        return 0;

    Qt4Target *t(static_cast<Qt4Target *>(parent));
    return new S60DeviceRunConfiguration(t, pathFromId(id));
}

bool S60DeviceRunConfigurationFactory::canRestore(Target *parent, const QVariantMap &map) const
{
    Qt4Target * t(qobject_cast<Qt4Target *>(parent));
    if (!t ||
        t->id() != QLatin1String(Constants::S60_DEVICE_TARGET_ID))
        return false;
    QString id(ProjectExplorer::idFromMap(map));
    return id == QLatin1String(S60_DEVICE_RC_ID);
}

RunConfiguration *S60DeviceRunConfigurationFactory::restore(Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    Qt4Target *t(static_cast<Qt4Target *>(parent));
    S60DeviceRunConfiguration *rc(new S60DeviceRunConfiguration(t, QString()));
    if (rc->fromMap(map))
        return rc;

    delete rc;
    return 0;
}

bool S60DeviceRunConfigurationFactory::canClone(Target *parent, RunConfiguration *source) const
{
    if (!qobject_cast<Qt4Target *>(parent))
        return false;
    return source->id() == QLatin1String(S60_DEVICE_RC_ID);
}

RunConfiguration *S60DeviceRunConfigurationFactory::clone(Target *parent, RunConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    Qt4Target *t = static_cast<Qt4Target *>(parent);
    S60DeviceRunConfiguration * old(static_cast<S60DeviceRunConfiguration *>(source));
    return new S60DeviceRunConfiguration(t, old);
}

// ======== S60DeviceRunControlBase

S60DeviceRunControl::S60DeviceRunControl(RunConfiguration *runConfiguration, QString mode) :
    RunControl(runConfiguration, mode),
    m_toolChain(ProjectExplorer::ToolChain_INVALID),
    m_launcher(0)
{
    // connect for automatically reporting the "finished deploy" state to the progress manager
    connect(this, SIGNAL(finished()), this, SLOT(reportDeployFinished()));

    S60DeviceRunConfiguration *s60runConfig = qobject_cast<S60DeviceRunConfiguration *>(runConfiguration);
    const Qt4BuildConfiguration *activeBuildConf = s60runConfig->qt4Target()->activeBuildConfiguration();
    S60DeployConfiguration *activeDeployConf = qobject_cast<S60DeployConfiguration *>(s60runConfig->qt4Target()->activeDeployConfiguration());

    QTC_ASSERT(s60runConfig, return);
    m_toolChain = s60runConfig->toolChainType();
    m_serialPortName = activeDeployConf->serialPortName();
    m_serialPortFriendlyName = SymbianUtils::SymbianDeviceManager::instance()->friendlyNameForPort(m_serialPortName);
    m_targetName = s60runConfig->targetName();
    m_commandLineArguments = s60runConfig->commandLineArguments();
    m_qtDir = activeBuildConf->qtVersion()->versionInfo().value("QT_INSTALL_DATA");
    m_installationDrive = activeDeployConf->installationDrive();
    if (const QtVersion *qtv = activeDeployConf->qtVersion())
        m_qtBinPath = qtv->versionInfo().value(QLatin1String("QT_INSTALL_BINS"));
    QTC_ASSERT(!m_qtBinPath.isEmpty(), return);
    m_executableFileName = s60runConfig->localExecutableFileName();
    if (debug)
        qDebug() << "S60DeviceRunControl::CT" << m_targetName << ProjectExplorer::ToolChain::toolChainName(m_toolChain)
                 << m_serialPortName;
}

S60DeviceRunControl::~S60DeviceRunControl()
{
    if (m_launcher) {
        m_launcher->deleteLater();
        m_launcher = 0;
    }
}

void S60DeviceRunControl::start()
{
    m_launchProgress = new QFutureInterface<void>;
    Core::ICore::instance()->progressManager()->addTask(m_launchProgress->future(),
                                                        tr("Launching"),
                                                        QLatin1String("Symbian.Launch"));
    m_launchProgress->setProgressRange(0, PROGRESS_MAX);
    m_launchProgress->setProgressValue(0);
    m_launchProgress->reportStarted();
    emit started();
    if (m_serialPortName.isEmpty()) {
        m_launchProgress->reportCanceled();
        appendMessage(this, tr("No device is connected. Please connect a device and try again."), true);
        emit finished();
        return;
    }

    emit appendMessage(this, tr("Executable file: %1").arg(msgListFile(m_executableFileName)), false);

    QString errorMessage;
    QString settingsCategory;
    QString settingsPage;
    if (!checkConfiguration(&errorMessage, &settingsCategory, &settingsPage)) {
        m_launchProgress->reportCanceled();
        appendMessage(this, errorMessage, true);
        emit finished();
        Core::ICore::instance()->showWarningWithOptions(tr("Debugger for Symbian Platform"),
                                                        errorMessage, QString(),
                                                        settingsCategory, settingsPage);
        return;
    }

    startLaunching();
}

RunControl::StopResult S60DeviceRunControl::stop()
{
    if (m_launcher)
        m_launcher->terminate();
    return AsynchronousStop;
}

bool S60DeviceRunControl::isRunning() const
{
    return m_launcher && (m_launcher->state() == trk::Launcher::Connecting
                          || m_launcher->state() == trk::Launcher::Connected
                          || m_launcher->state() == trk::Launcher::WaitingForTrk);
}

void S60DeviceRunControl::startLaunching()
{
    QString errorMessage;
    if (setupLauncher(errorMessage)) {
        if (m_launchProgress)
                    m_launchProgress->setProgressValue(PROGRESS_MAX/2);
    } else {
        if (!errorMessage.isEmpty())
            appendMessage(this, errorMessage, true);
        stop();
        emit finished();
    }
}

bool S60DeviceRunControl::setupLauncher(QString &errorMessage)
{
    connect(SymbianUtils::SymbianDeviceManager::instance(), SIGNAL(deviceRemoved(const SymbianUtils::SymbianDevice)),
            this, SLOT(deviceRemoved(SymbianUtils::SymbianDevice)));
    m_launcher = trk::Launcher::acquireFromDeviceManager(m_serialPortName, 0, &errorMessage);
    if (!m_launcher)
        return false;

    connect(m_launcher, SIGNAL(finished()), this, SLOT(launcherFinished()));
    connect(m_launcher, SIGNAL(canNotConnect(QString)), this, SLOT(printConnectFailed(QString)));
    connect(m_launcher, SIGNAL(stateChanged(int)), this, SLOT(slotLauncherStateChanged(int)));
    connect(m_launcher, SIGNAL(processStopped(uint,uint,uint,QString)),
            this, SLOT(processStopped(uint,uint,uint,QString)));

    if (!m_commandLineArguments.isEmpty())
        m_launcher->setCommandLineArgs(m_commandLineArguments);

    const QString runFileName = QString::fromLatin1("%1:\\sys\\bin\\%2.exe").arg(m_installationDrive).arg(m_targetName);
    initLauncher(runFileName, m_launcher);
    const trk::PromptStartCommunicationResult src =
            S60RunConfigBluetoothStarter::startCommunication(m_launcher->trkDevice(),
                                                             0, &errorMessage);
    if (src != trk::PromptStartCommunicationConnected)
        return false;

    if (!m_launcher->startServer(&errorMessage)) {
        errorMessage = tr("Could not connect to phone on port '%1': %2\n"
                          "Check if the phone is connected and App TRK is running.").arg(m_serialPortName, errorMessage);
        return false;
    }
    return true;
}

void S60DeviceRunControl::printConnectFailed(const QString &errorMessage)
{
    emit appendMessage(this, tr("Could not connect to App TRK on device: %1. Restarting App TRK might help.").arg(errorMessage), true);
}

void S60DeviceRunControl::launcherFinished()
{
    trk::Launcher::releaseToDeviceManager(m_launcher);
    m_launcher->deleteLater();
    m_launcher = 0;
    handleLauncherFinished();
}

void S60DeviceRunControl::reportDeployFinished()
{
    if (m_launchProgress) {
        m_launchProgress->reportFinished();
        delete m_launchProgress;
        m_launchProgress = 0;
    }
}

void S60DeviceRunControl::processStopped(uint pc, uint pid, uint tid, const QString& reason)
{
    emit addToOutputWindow(this, trk::Launcher::msgStopped(pid, tid, pc, reason), false);
    m_launcher->terminate();
}

QMessageBox *S60DeviceRunControl::createTrkWaitingMessageBox(const QString &port, QWidget *parent)
{
    const QString title  = tr("Waiting for App TRK");
    const QString text = tr("Qt Creator is waiting for the TRK application to connect.<br>"
                            "Please make sure the application is running on "
                            "your mobile phone and the right port is "
                            "configured in the project settings.").arg(port);
    QMessageBox *rc = new QMessageBox(QMessageBox::Information, title, text,
                                      QMessageBox::Cancel, parent);
    return rc;
}

void S60DeviceRunControl::slotLauncherStateChanged(int s)
{
    if (s == trk::Launcher::WaitingForTrk) {
        QMessageBox *mb = S60DeviceRunControl::createTrkWaitingMessageBox(m_launcher->trkServerName(),
                                                     Core::ICore::instance()->mainWindow());
        connect(m_launcher, SIGNAL(stateChanged(int)), mb, SLOT(close()));
        connect(mb, SIGNAL(finished(int)), this, SLOT(slotWaitingForTrkClosed()));
        mb->open();
    }
}

void S60DeviceRunControl::slotWaitingForTrkClosed()
{
    if (m_launcher && m_launcher->state() == trk::Launcher::WaitingForTrk) {
        stop();
        appendMessage(this, tr("Canceled."), true);
        emit finished();
    }
}

void S60DeviceRunControl::printApplicationOutput(const QString &output)
{
    printApplicationOutput(output, false);
}

void S60DeviceRunControl::printApplicationOutput(const QString &output, bool onStdErr)
{
    emit addToOutputWindowInline(this, output, onStdErr);
}

void S60DeviceRunControl::deviceRemoved(const SymbianUtils::SymbianDevice &d)
{
    if (m_launcher && d.portName() == m_serialPortName) {
        trk::Launcher::releaseToDeviceManager(m_launcher);
        m_launcher->deleteLater();
        m_launcher = 0;
        appendMessage(this, tr("The device '%1' has been disconnected").arg(d.friendlyName()), true);
        emit finished();
    }
}

bool S60DeviceRunControl::checkConfiguration(QString * /* errorMessage */,
                                                 QString * /* settingsCategory */,
                                                 QString * /* settingsPage */) const
{
    return true;
}

void S60DeviceRunControl::initLauncher(const QString &executable, trk::Launcher *launcher)
{
     connect(launcher, SIGNAL(startingApplication()), this, SLOT(printStartingNotice()));
     connect(launcher, SIGNAL(applicationRunning(uint)), this, SLOT(applicationRunNotice(uint)));
     connect(launcher, SIGNAL(canNotRun(QString)), this, SLOT(applicationRunFailedNotice(QString)));
     connect(launcher, SIGNAL(applicationOutputReceived(QString)), this, SLOT(printApplicationOutput(QString)));
     launcher->addStartupActions(trk::Launcher::ActionRun);
     launcher->setFileName(executable);
}

void S60DeviceRunControl::handleLauncherFinished()
{
     emit finished();
     emit appendMessage(this, tr("Finished."), false);
 }

void S60DeviceRunControl::printStartingNotice()
{
    emit appendMessage(this, tr("Starting application..."), false);
}

void S60DeviceRunControl::applicationRunNotice(uint pid)
{
    emit appendMessage(this, tr("Application running with pid %1.").arg(pid), false);
    if (m_launchProgress)
        m_launchProgress->setProgressValue(PROGRESS_MAX);
}

void S60DeviceRunControl::applicationRunFailedNotice(const QString &errorMessage)
{
    emit appendMessage(this, tr("Could not start application: %1").arg(errorMessage), true);
}

// ======== S60DeviceDebugRunControl

static inline QString localExecutable(const S60DeviceRunConfiguration *rc)
{
    if (const S60DeviceRunConfiguration *s60runConfig = qobject_cast<const S60DeviceRunConfiguration *>(rc))
        return s60runConfig->localExecutableFileName();
    return QString();
}

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
Debugger::DebuggerStartParameters S60DeviceDebugRunControl::s60DebuggerStartParams(const S60DeviceRunConfiguration *rc)
{
    Debugger::DebuggerStartParameters sp;
    QTC_ASSERT(rc, return sp);

    const S60DeployConfiguration *activeDeployConf = qobject_cast<S60DeployConfiguration *>(rc->qt4Target()->activeDeployConfiguration());

    const QString debugFileName = QString::fromLatin1("%1:\\sys\\bin\\%2.exe")
            .arg(activeDeployConf->installationDrive()).arg(rc->targetName());

    sp.remoteChannel = activeDeployConf->serialPortName();
    sp.processArgs = rc->commandLineArguments();
    sp.startMode = Debugger::StartInternal;
    sp.toolChainType = rc->toolChainType();
    sp.executable = debugFileName;
    sp.executableUid = rc->executableUid();

    QTC_ASSERT(sp.executableUid, return sp);

    // Prefer the '*.sym' file over the '.exe', which should exist at the same
    // location in debug builds. This can be 'foo.exe' (ABLD) or 'foo.exe.sym' (Raptor)
    sp.symbolFileName = symbolFileFromExecutable(localExecutable(rc));
    return sp;
}

S60DeviceDebugRunControl::S60DeviceDebugRunControl(S60DeviceRunConfiguration *rc,
                                                   const QString &) :
    Debugger::DebuggerRunControl(rc, Debugger::GdbEngineType,
                                 S60DeviceDebugRunControl::s60DebuggerStartParams(rc))
{
    if (startParameters().symbolFileName.isEmpty()) {
        const QString msg = tr("Warning: Cannot locate the symbol file belonging to %1.").
                               arg(localExecutable(rc));
        emit appendMessage(this, msg, true);
    }
}

void S60DeviceDebugRunControl::start()
{
    QString errorMessage;
    QString settingsCategory;
    QString settingsPage;
    if (!Debugger::DebuggerRunControl::checkDebugConfiguration(startParameters().toolChainType,
                                                               &errorMessage, &settingsCategory, &settingsPage)) {
        appendMessage(this, errorMessage, true);
        emit finished();
        Core::ICore::instance()->showWarningWithOptions(tr("Debugger for Symbian Platform"),
                                                        errorMessage, QString(),
                                                        settingsCategory, settingsPage);
        return;
    }

    emit appendMessage(this, tr("Launching debugger..."), false);
    Debugger::DebuggerRunControl::start();
}

S60DeviceDebugRunControl::~S60DeviceDebugRunControl()
{
}
