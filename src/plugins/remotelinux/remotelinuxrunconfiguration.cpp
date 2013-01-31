/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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

#include "remotelinuxrunconfiguration.h"

#include "remotelinuxdeployconfiguration.h"
#include "remotelinuxrunconfigurationwidget.h"

#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <qtsupport/qtoutputformatter.h>

#include <utils/portlist.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace QSsh;
using namespace Utils;

namespace RemoteLinux {
namespace Internal {
namespace {
const char ArgumentsKey[] = "Qt4ProjectManager.MaemoRunConfiguration.Arguments";
const char ProFileKey[] = "Qt4ProjectManager.MaemoRunConfiguration.ProFile";
const char BaseEnvironmentBaseKey[] = "Qt4ProjectManager.MaemoRunConfiguration.BaseEnvironmentBase";
const char UserEnvironmentChangesKey[]
    = "Qt4ProjectManager.MaemoRunConfiguration.UserEnvironmentChanges";
const char UseAlternateExeKey[] = "RemoteLinux.RunConfig.UseAlternateRemoteExecutable";
const char AlternateExeKey[] = "RemoteLinux.RunConfig.AlternateRemoteExecutable";
const char WorkingDirectoryKey[] = "RemoteLinux.RunConfig.WorkingDirectory";

} // anonymous namespace

class RemoteLinuxRunConfigurationPrivate {
public:
    RemoteLinuxRunConfigurationPrivate(const QString &projectFilePath)
        : projectFilePath(projectFilePath),
          baseEnvironmentType(RemoteLinuxRunConfiguration::RemoteBaseEnvironment),
          useAlternateRemoteExecutable(false)
    {
    }

    RemoteLinuxRunConfigurationPrivate(const RemoteLinuxRunConfigurationPrivate *other)
        : projectFilePath(other->projectFilePath),
          gdbPath(other->gdbPath),
          arguments(other->arguments),
          baseEnvironmentType(other->baseEnvironmentType),
          remoteEnvironment(other->remoteEnvironment),
          userEnvironmentChanges(other->userEnvironmentChanges),
          useAlternateRemoteExecutable(other->useAlternateRemoteExecutable),
          alternateRemoteExecutable(other->alternateRemoteExecutable),
          workingDirectory(other->workingDirectory)
    {
    }

    QString projectFilePath;
    QString gdbPath;
    QString arguments;
    RemoteLinuxRunConfiguration::BaseEnvironmentType baseEnvironmentType;
    Environment remoteEnvironment;
    QList<EnvironmentItem> userEnvironmentChanges;
    QString disabledReason;
    bool useAlternateRemoteExecutable;
    QString alternateRemoteExecutable;
    QString workingDirectory;
};

} // namespace Internal

using namespace Internal;

RemoteLinuxRunConfiguration::RemoteLinuxRunConfiguration(Target *parent, const Core::Id id,
        const QString &proFilePath)
    : RunConfiguration(parent, id),
      d(new RemoteLinuxRunConfigurationPrivate(proFilePath))
{
    init();
}

RemoteLinuxRunConfiguration::RemoteLinuxRunConfiguration(ProjectExplorer::Target *parent,
        RemoteLinuxRunConfiguration *source)
    : RunConfiguration(parent, source),
      d(new RemoteLinuxRunConfigurationPrivate(source->d))
{
    init();
}

RemoteLinuxRunConfiguration::~RemoteLinuxRunConfiguration()
{
    delete d;
}

void RemoteLinuxRunConfiguration::init()
{
    setDefaultDisplayName(defaultDisplayName());
    debuggerAspect()->suppressQmlDebuggingSpinbox();

    connect(target(), SIGNAL(deploymentDataChanged()), SLOT(handleBuildSystemDataUpdated()));
    connect(target(), SIGNAL(applicationTargetsChanged()), SLOT(handleBuildSystemDataUpdated()));
    connect(target(), SIGNAL(kitChanged()),
            this, SLOT(handleBuildSystemDataUpdated())); // Handles device changes, etc.
}

bool RemoteLinuxRunConfiguration::isEnabled() const
{
    if (remoteExecutableFilePath().isEmpty()) {
        d->disabledReason = tr("Don't know what to run.");
        return false;
    }
    d->disabledReason.clear();
    return true;
}

QString RemoteLinuxRunConfiguration::disabledReason() const
{
    return d->disabledReason;
}

QWidget *RemoteLinuxRunConfiguration::createConfigurationWidget()
{
    return new RemoteLinuxRunConfigurationWidget(this);
}

OutputFormatter *RemoteLinuxRunConfiguration::createOutputFormatter() const
{
    return new QtSupport::QtOutputFormatter(target()->project());
}

QVariantMap RemoteLinuxRunConfiguration::toMap() const
{
    QVariantMap map(RunConfiguration::toMap());
    map.insert(QLatin1String(ArgumentsKey), d->arguments);
    const QDir dir = QDir(target()->project()->projectDirectory());
    map.insert(QLatin1String(ProFileKey), dir.relativeFilePath(d->projectFilePath));
    map.insert(QLatin1String(BaseEnvironmentBaseKey), d->baseEnvironmentType);
    map.insert(QLatin1String(UserEnvironmentChangesKey),
        EnvironmentItem::toStringList(d->userEnvironmentChanges));
    map.insert(QLatin1String(UseAlternateExeKey), d->useAlternateRemoteExecutable);
    map.insert(QLatin1String(AlternateExeKey), d->alternateRemoteExecutable);
    map.insert(QLatin1String(WorkingDirectoryKey), d->workingDirectory);
    return map;
}

bool RemoteLinuxRunConfiguration::fromMap(const QVariantMap &map)
{
    if (!RunConfiguration::fromMap(map))
        return false;

    d->arguments = map.value(QLatin1String(ArgumentsKey)).toString();
    const QDir dir = QDir(target()->project()->projectDirectory());
    d->projectFilePath
            = QDir::cleanPath(dir.filePath(map.value(QLatin1String(ProFileKey)).toString()));
    d->userEnvironmentChanges =
        EnvironmentItem::fromStringList(map.value(QLatin1String(UserEnvironmentChangesKey))
        .toStringList());
    d->baseEnvironmentType = static_cast<BaseEnvironmentType>(map.value(QLatin1String(BaseEnvironmentBaseKey),
        RemoteBaseEnvironment).toInt());
    d->useAlternateRemoteExecutable = map.value(QLatin1String(UseAlternateExeKey), false).toBool();
    d->alternateRemoteExecutable = map.value(QLatin1String(AlternateExeKey)).toString();
    d->workingDirectory = map.value(QLatin1String(WorkingDirectoryKey)).toString();

    setDefaultDisplayName(defaultDisplayName());

    return true;
}

QString RemoteLinuxRunConfiguration::defaultDisplayName()
{
    if (!d->projectFilePath.isEmpty())
        //: %1 is the name of a project which is being run on remote Linux
        return tr("%1 (on Remote Device)").arg(QFileInfo(d->projectFilePath).completeBaseName());
    //: Remote Linux run configuration default display name
    return tr("Run on Remote Device");
}

QString RemoteLinuxRunConfiguration::arguments() const
{
    return d->arguments;
}

QString RemoteLinuxRunConfiguration::environmentPreparationCommand() const
{
    QString command;
    const QStringList filesToSource = QStringList() << QLatin1String("/etc/profile")
        << QLatin1String("$HOME/.profile");
    foreach (const QString &filePath, filesToSource)
        command += QString::fromLatin1("test -f %1 && source %1;").arg(filePath);
    if (!workingDirectory().isEmpty())
        command += QLatin1String("cd ") + workingDirectory();
    else
        command.chop(1); // Trailing semicolon.
    return command;
}

QString RemoteLinuxRunConfiguration::commandPrefix() const
{
    return QString::fromLatin1("%1; DISPLAY=:0.0 %2")
        .arg(environmentPreparationCommand(), userEnvironmentChangesAsString());
}

QString RemoteLinuxRunConfiguration::localExecutableFilePath() const
{
    return target()->applicationTargets()
            .targetForProject(Utils::FileName::fromString(d->projectFilePath)).toString();
}

QString RemoteLinuxRunConfiguration::defaultRemoteExecutableFilePath() const
{
    return target()->deploymentData().deployableForLocalFile(localExecutableFilePath())
            .remoteFilePath();
}

QString RemoteLinuxRunConfiguration::remoteExecutableFilePath() const
{
    return d->useAlternateRemoteExecutable
        ? alternateRemoteExecutable() : defaultRemoteExecutableFilePath();
}

void RemoteLinuxRunConfiguration::setArguments(const QString &args)
{
    d->arguments = args;
}

QString RemoteLinuxRunConfiguration::workingDirectory() const
{
    return d->workingDirectory;
}

void RemoteLinuxRunConfiguration::setWorkingDirectory(const QString &wd)
{
    d->workingDirectory = wd;
}

void RemoteLinuxRunConfiguration::setUseAlternateExecutable(bool useAlternate)
{
    d->useAlternateRemoteExecutable = useAlternate;
}

bool RemoteLinuxRunConfiguration::useAlternateExecutable() const
{
    return d->useAlternateRemoteExecutable;
}

void RemoteLinuxRunConfiguration::setAlternateRemoteExecutable(const QString &exe)
{
    d->alternateRemoteExecutable = exe;
}

QString RemoteLinuxRunConfiguration::alternateRemoteExecutable() const
{
    return d->alternateRemoteExecutable;
}

int RemoteLinuxRunConfiguration::portsUsedByDebuggers() const
{
    int ports = 0;
    if (debuggerAspect()->useQmlDebugger())
        ++ports;
    if (debuggerAspect()->useCppDebugger())
        ++ports;

    return ports;
}

void RemoteLinuxRunConfiguration::handleBuildSystemDataUpdated()
{
    emit deploySpecsChanged();
    emit targetInformationChanged();
    updateEnabledState();
}

QString RemoteLinuxRunConfiguration::baseEnvironmentText() const
{
    if (d->baseEnvironmentType == CleanBaseEnvironment)
        return tr("Clean Environment");
    else  if (d->baseEnvironmentType == RemoteBaseEnvironment)
        return tr("System Environment");
    return QString();
}

RemoteLinuxRunConfiguration::BaseEnvironmentType RemoteLinuxRunConfiguration::baseEnvironmentType() const
{
    return d->baseEnvironmentType;
}

void RemoteLinuxRunConfiguration::setBaseEnvironmentType(BaseEnvironmentType env)
{
    if (d->baseEnvironmentType != env) {
        d->baseEnvironmentType = env;
        emit baseEnvironmentChanged();
    }
}

Environment RemoteLinuxRunConfiguration::environment() const
{
    Environment env = baseEnvironment();
    env.modify(userEnvironmentChanges());
    return env;
}

Environment RemoteLinuxRunConfiguration::baseEnvironment() const
{
    return (d->baseEnvironmentType == RemoteBaseEnvironment ? remoteEnvironment()
        : Environment());
}

QList<EnvironmentItem> RemoteLinuxRunConfiguration::userEnvironmentChanges() const
{
    return d->userEnvironmentChanges;
}

QString RemoteLinuxRunConfiguration::userEnvironmentChangesAsString() const
{
    QString env;
    QString placeHolder = QLatin1String("%1=%2 ");
    foreach (const EnvironmentItem &item, userEnvironmentChanges())
        env.append(placeHolder.arg(item.name, item.value));
    return env.mid(0, env.size() - 1);
}

void RemoteLinuxRunConfiguration::setUserEnvironmentChanges(
    const QList<EnvironmentItem> &diff)
{
    if (d->userEnvironmentChanges != diff) {
        d->userEnvironmentChanges = diff;
        emit userEnvironmentChangesChanged(diff);
    }
}

Environment RemoteLinuxRunConfiguration::remoteEnvironment() const
{
    return d->remoteEnvironment;
}

void RemoteLinuxRunConfiguration::setRemoteEnvironment(const Environment &environment)
{
    if (d->remoteEnvironment.size() == 0 || d->remoteEnvironment != environment) {
        d->remoteEnvironment = environment;
        emit remoteEnvironmentChanged();
    }
}

QString RemoteLinuxRunConfiguration::projectFilePath() const
{
    return d->projectFilePath;
}

void RemoteLinuxRunConfiguration::setDisabledReason(const QString &reason) const
{
    d->disabledReason = reason;
}

const QString RemoteLinuxRunConfiguration::IdPrefix = QLatin1String("RemoteLinuxRunConfiguration:");

} // namespace RemoteLinux
