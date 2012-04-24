/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "s60devicerunconfiguration.h"

#include "qt4project.h"
#include "qt4nodes.h"
#include "qt4projectmanagerconstants.h"
#include "qt4buildconfiguration.h"
#include "s60deployconfiguration.h"
#include "s60devicerunconfigurationwidget.h"
#include "s60manager.h"
#include "symbianidevice.h"
#include "symbianidevicefactory.h"
#include "symbianqtversion.h"

#include <utils/qtcassert.h>
#include <projectexplorer/profileinformation.h>
#include <projectexplorer/target.h>
#include <qtsupport/customexecutablerunconfiguration.h>
#include <qtsupport/qtoutputformatter.h>
#include <qtsupport/qtprofileinformation.h>

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

QString pathFromId(Core::Id id)
{
    QString idstr = QString::fromUtf8(id.name());
    const QString prefix = QLatin1String(S60_DEVICE_RC_PREFIX);
    if (!idstr.startsWith(prefix))
        return QString();
    return idstr.mid(prefix.size());
}

} // anonymous namespace

// ======== S60DeviceRunConfiguration

S60DeviceRunConfiguration::S60DeviceRunConfiguration(Target *parent, Core::Id id) :
    RunConfiguration(parent,  id),
    m_proFilePath(pathFromId(id))
{
    Qt4Project *project = static_cast<Qt4Project *>(parent->project());
    m_validParse = project->validParse(m_proFilePath);
    m_parseInProgress = project->parseInProgress(m_proFilePath);

    ctor();
}

S60DeviceRunConfiguration::S60DeviceRunConfiguration(Target *parent, S60DeviceRunConfiguration *source) :
    RunConfiguration(parent, source),
    m_proFilePath(source->m_proFilePath),
    m_commandLineArguments(source->m_commandLineArguments),
    m_validParse(source->m_validParse),
    m_parseInProgress(source->m_parseInProgress)
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

    Qt4Project *pro = static_cast<Qt4Project *>(target()->project());
    connect(pro, SIGNAL(proFileUpdated(Qt4ProjectManager::Qt4ProFileNode*,bool,bool)),
            this, SLOT(proFileUpdate(Qt4ProjectManager::Qt4ProFileNode*,bool,bool)));
}

void S60DeviceRunConfiguration::proFileUpdate(Qt4ProjectManager::Qt4ProFileNode *pro, bool success, bool parseInProgress)
{
    if (m_proFilePath != pro->path())
        return;
    bool enabled = isEnabled();
    QString reason = disabledReason();
    m_validParse = success;
    m_parseInProgress = parseInProgress;
    if (enabled != isEnabled() || reason != disabledReason())
        emit enabledChanged();
    if (!parseInProgress)
        emit targetInformationChanged();
}

S60DeviceRunConfiguration::~S60DeviceRunConfiguration()
{
}

bool S60DeviceRunConfiguration::isEnabled() const
{
    return m_validParse && !m_parseInProgress;
}

QString S60DeviceRunConfiguration::disabledReason() const
{
    if (m_parseInProgress)
        return tr("The .pro file '%1' is currently being parsed.")
                .arg(QFileInfo(m_proFilePath).fileName());
    if (!m_validParse)
        return static_cast<Qt4Project *>(target()->project())->disabledReasonForRunConfiguration(m_proFilePath);
    return QString();
}

QWidget *S60DeviceRunConfiguration::createConfigurationWidget()
{
    return new S60DeviceRunConfigurationWidget(this);
}

Utils::OutputFormatter *S60DeviceRunConfiguration::createOutputFormatter() const
{
    return new QtSupport::QtOutputFormatter(target()->project());
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

    m_proFilePath = QDir::cleanPath(projectDir.filePath(map.value(QLatin1String(PRO_FILE_KEY)).toString()));
    m_commandLineArguments = map.value(QLatin1String(COMMAND_LINE_ARGUMENTS_KEY)).toString();

    if (m_proFilePath.isEmpty())
        return false;
    if (!QFileInfo(m_proFilePath).exists())
        return false;

    m_validParse = static_cast<Qt4Project *>(target()->project())->validParse(m_proFilePath);
    m_parseInProgress = static_cast<Qt4Project *>(target()->project())->parseInProgress(m_proFilePath);

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
    TargetInformation ti = static_cast<Qt4Project *>(target()->project())
            ->rootQt4ProjectNode()->targetInformation(projectFilePath());
    if (!ti.valid)
        return QString();
    return ti.target;
}

bool S60DeviceRunConfiguration::isDebug() const
{
    const Qt4BuildConfiguration *qt4bc = static_cast<const Qt4BuildConfiguration *>(target()->activeBuildConfiguration());
    return (qt4bc->qmakeBuildConfiguration() & QtSupport::BaseQtVersion::DebugBuild);
}

QString S60DeviceRunConfiguration::symbianTarget() const
{
    return isDebug() ? QLatin1String("udeb") : QLatin1String("urel");
}

// ABLD/Raptor: Return executable from device/EPOC
static inline QString localExecutableFromVersion(const ProjectExplorer::Profile *p,
                                                 const QString &symbianTarget, /* udeb/urel */
                                                 const QString &targetName)
{
    Q_ASSERT(p);

    ToolChain *tc = ToolChainProfileInformation::toolChain(p);
    SymbianQtVersion *qtv = dynamic_cast<SymbianQtVersion *>(QtSupport::QtProfileInformation::qtVersion(p));
    QString root;
    if (SysRootProfileInformation::hasSysRoot(p))
        root = SysRootProfileInformation::sysRoot(p).toString();

    if (!tc)
        return QString();

    QString localExecutable;
    QString platform = S60Manager::platform(tc);
    if (qtv->isBuildWithSymbianSbsV2() && platform == QLatin1String("gcce"))
        platform = QLatin1String("armv5");
    QTextStream(&localExecutable) << root << "/epoc32/release/"
            << platform << '/' << symbianTarget << '/' << targetName << ".exe";
    return localExecutable;
}

QString S60DeviceRunConfiguration::localExecutableFileName() const
{
    TargetInformation ti = static_cast<Qt4Project *>(target()->project())->rootQt4ProjectNode()->targetInformation(projectFilePath());
    if (!ti.valid)
        return QString();

    return localExecutableFromVersion(target()->profile(), symbianTarget(), targetName());
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

QString S60DeviceRunConfiguration::qmlCommandLineArguments() const
{
    QString args;
    if (debuggerAspect()->useQmlDebugger()) {
        const S60DeployConfiguration *activeDeployConf =
            qobject_cast<const S60DeployConfiguration *>(target()->activeDeployConfiguration());
        QTC_ASSERT(activeDeployConf, return args);

        QSharedPointer<const SymbianIDevice> dev = activeDeployConf->device().dynamicCast<const SymbianIDevice>();
        if (dev->communicationChannel() == SymbianIDevice::CommunicationCodaTcpConnection)
            args = QString::fromLatin1("-qmljsdebugger=port:%1,block").arg(debuggerAspect()->qmlDebugServerPort());
        else
            args = QLatin1String("-qmljsdebugger=ost");
    }
    return args;
}

QString S60DeviceRunConfiguration::proFilePath() const
{
    return m_proFilePath;
}

// ======== S60DeviceRunConfigurationFactory

S60DeviceRunConfigurationFactory::S60DeviceRunConfigurationFactory(QObject *parent) :
    QmakeRunConfigurationFactory(parent)
{ setObjectName(QLatin1String("S60DeviceRunConfigurationFactory"));}

S60DeviceRunConfigurationFactory::~S60DeviceRunConfigurationFactory()
{ }

QList<Core::Id> S60DeviceRunConfigurationFactory::availableCreationIds(Target *parent) const
{
    QList<Core::Id> result;
    if (!canHandle(parent))
        return result;

    Qt4Project *project = static_cast<Qt4Project *>(parent->project());
    QStringList proFiles = project->applicationProFilePathes(QLatin1String(S60_DEVICE_RC_PREFIX));
    foreach (const QString &pf, proFiles)
        result << Core::Id(pf);
    return result;
}

QString S60DeviceRunConfigurationFactory::displayNameForId(const Core::Id id) const
{
    if (!pathFromId(id).isEmpty())
        return tr("%1 on Symbian Device").arg(QFileInfo(pathFromId(id)).completeBaseName());
    return QString();
}

bool S60DeviceRunConfigurationFactory::canHandle(Target *t) const
{
    if (!t->project()->supportsProfile(t->profile()))
        return false;
    if (!qobject_cast<Qt4Project *>(t->project()))
        return false;

    Core::Id deviceType = ProjectExplorer::DeviceTypeProfileInformation::deviceTypeId(t->profile());
    return deviceType == SymbianIDeviceFactory::deviceType();
}

QList<RunConfiguration *> S60DeviceRunConfigurationFactory::runConfigurationsForNode(Target *t, ProjectExplorer::Node *n)
{
    QList<ProjectExplorer::RunConfiguration *> result;
    foreach (ProjectExplorer::RunConfiguration *rc, t->runConfigurations()) {
        if (S60DeviceRunConfiguration *s60rc = qobject_cast<S60DeviceRunConfiguration *>(rc))
            if (s60rc->proFilePath() == n->path())
                result << rc;
    }
    return result;
}

bool S60DeviceRunConfigurationFactory::canCreate(Target *parent, const Core::Id id) const
{
    if (!canHandle(parent))
        return false;
    Qt4Project *project = static_cast<Qt4Project *>(parent->project());
    return project->hasApplicationProFile(pathFromId(id));
}

RunConfiguration *S60DeviceRunConfigurationFactory::create(Target *parent, const Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;

    return new S60DeviceRunConfiguration(parent, id);
}

bool S60DeviceRunConfigurationFactory::canRestore(Target *parent, const QVariantMap &map) const
{
    if (!canHandle(parent))
        return false;

    const Core::Id id = ProjectExplorer::idFromMap(map);
    return id == Core::Id(S60_DEVICE_RC_ID);
}

RunConfiguration *S60DeviceRunConfigurationFactory::restore(Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    S60DeviceRunConfiguration *rc = new S60DeviceRunConfiguration(parent, idFromMap(map));
    if (rc->fromMap(map))
        return rc;

    delete rc;
    return 0;
}

bool S60DeviceRunConfigurationFactory::canClone(Target *parent, RunConfiguration *source) const
{
    if (!canHandle(parent))
        return false;
    return source->id() == Core::Id(S60_DEVICE_RC_ID);
}

RunConfiguration *S60DeviceRunConfigurationFactory::clone(Target *parent, RunConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    S60DeviceRunConfiguration *old = static_cast<S60DeviceRunConfiguration *>(source);
    return new S60DeviceRunConfiguration(parent, old);
}
