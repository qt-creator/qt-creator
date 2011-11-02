/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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
#include "qt4symbiantarget.h"
#include "qt4target.h"
#include "qt4buildconfiguration.h"
#include "s60deployconfiguration.h"
#include "s60devicerunconfigurationwidget.h"
#include "s60manager.h"
#include "symbianqtversion.h"

#include <utils/qtcassert.h>
#include <qtsupport/qtoutputformatter.h>

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

} // anonymous namespace

// ======== S60DeviceRunConfiguration

S60DeviceRunConfiguration::S60DeviceRunConfiguration(Qt4BaseTarget *parent, const QString &proFilePath) :
    RunConfiguration(parent,  QLatin1String(S60_DEVICE_RC_ID)),
    m_proFilePath(proFilePath),
    m_validParse(parent->qt4Project()->validParse(proFilePath)),
    m_parseInProgress(parent->qt4Project()->parseInProgress(proFilePath))
{
    ctor();
}

S60DeviceRunConfiguration::S60DeviceRunConfiguration(Qt4BaseTarget *target, S60DeviceRunConfiguration *source) :
    RunConfiguration(target, source),
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

    Qt4Project *pro = qt4Target()->qt4Project();
    connect(pro, SIGNAL(proFileUpdated(Qt4ProjectManager::Qt4ProFileNode*,bool,bool)),
            this, SLOT(proFileUpdate(Qt4ProjectManager::Qt4ProFileNode*,bool,bool)));
}

void S60DeviceRunConfiguration::proFileUpdate(Qt4ProjectManager::Qt4ProFileNode *pro, bool success, bool parseInProgress)
{
    if (m_proFilePath != pro->path())
        return;
    bool enabled = isEnabled();
    m_validParse = success;
    m_parseInProgress = parseInProgress;
    if (enabled != isEnabled())
        emit isEnabledChanged(!enabled);
    if (!parseInProgress)
        emit targetInformationChanged();
}

S60DeviceRunConfiguration::~S60DeviceRunConfiguration()
{
}

Qt4SymbianTarget *S60DeviceRunConfiguration::qt4Target() const
{
    return static_cast<Qt4SymbianTarget *>(target());
}

bool S60DeviceRunConfiguration::isEnabled() const
{
    return m_validParse && !m_parseInProgress;
}

QString S60DeviceRunConfiguration::disabledReason() const
{
    if (m_parseInProgress)
        return tr("The .pro file is currently being parsed.");
    if (!m_validParse)
        return tr("The .pro file could not be parsed.");
    return QString();
}

QWidget *S60DeviceRunConfiguration::createConfigurationWidget()
{
    return new S60DeviceRunConfigurationWidget(this);
}

Utils::OutputFormatter *S60DeviceRunConfiguration::createOutputFormatter() const
{
    return new QtSupport::QtOutputFormatter(qt4Target()->qt4Project());
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

    m_validParse = qt4Target()->qt4Project()->validParse(m_proFilePath);
    m_parseInProgress = qt4Target()->qt4Project()->parseInProgress(m_proFilePath);

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
    TargetInformation ti = qt4Target()->qt4Project()->rootQt4ProjectNode()->targetInformation(projectFilePath());
    if (!ti.valid)
        return QString();
    return ti.target;
}

SymbianQtVersion *S60DeviceRunConfiguration::qtVersion() const
{
    if (const BuildConfiguration *bc = target()->activeBuildConfiguration())
        if (const Qt4BuildConfiguration *qt4bc = qobject_cast<const Qt4BuildConfiguration *>(bc))
            return dynamic_cast<SymbianQtVersion *>(qt4bc->qtVersion());
    return 0;
}

bool S60DeviceRunConfiguration::isDebug() const
{
    const Qt4BuildConfiguration *qt4bc = qt4Target()->activeQt4BuildConfiguration();
    return (qt4bc->qmakeBuildConfiguration() & QtSupport::BaseQtVersion::DebugBuild);
}

QString S60DeviceRunConfiguration::symbianTarget() const
{
    return isDebug() ? QLatin1String("udeb") : QLatin1String("urel");
}

// ABLD/Raptor: Return executable from device/EPOC
static inline QString localExecutableFromVersion(const SymbianQtVersion *qtv,
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
    TargetInformation ti = qt4Target()->qt4Project()->rootQt4ProjectNode()->targetInformation(projectFilePath());
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

QString S60DeviceRunConfiguration::qmlCommandLineArguments() const
{
    QString args;
    if (useQmlDebugger()) {
        const S60DeployConfiguration *activeDeployConf =
            qobject_cast<S60DeployConfiguration *>(qt4Target()->activeDeployConfiguration());
        QTC_ASSERT(activeDeployConf, return args);

        if (activeDeployConf->communicationChannel() == S60DeployConfiguration::CommunicationCodaTcpConnection)
            args = QString("-qmljsdebugger=port:%1,block").arg(qmlDebugServerPort());
        else
            args = QString("-qmljsdebugger=ost");
    }
    return args;
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
