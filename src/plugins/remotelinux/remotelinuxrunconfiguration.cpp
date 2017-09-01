/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "remotelinuxrunconfiguration.h"

#include "remotelinuxenvironmentaspect.h"
#include "remotelinuxrunconfigurationwidget.h"

#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runnables.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtoutputformatter.h>
#include <utils/qtcprocess.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {
namespace Internal {
namespace {
const char ArgumentsKey[] = "Qt4ProjectManager.MaemoRunConfiguration.Arguments";
const char TargetNameKey[] = "Qt4ProjectManager.MaemoRunConfiguration.TargetName";
const char UseAlternateExeKey[] = "RemoteLinux.RunConfig.UseAlternateRemoteExecutable";
const char AlternateExeKey[] = "RemoteLinux.RunConfig.AlternateRemoteExecutable";
const char WorkingDirectoryKey[] = "RemoteLinux.RunConfig.WorkingDirectory";

} // anonymous namespace

class RemoteLinuxRunConfigurationPrivate
{
public:
    QString targetName;
    QString arguments;
    bool useAlternateRemoteExecutable = false;
    QString alternateRemoteExecutable;
    QString workingDirectory;
};

} // namespace Internal

using namespace Internal;

RemoteLinuxRunConfiguration::RemoteLinuxRunConfiguration(Target *target)
    : RunConfiguration(target), d(new RemoteLinuxRunConfigurationPrivate)
{
    addExtraAspect(new RemoteLinuxEnvironmentAspect(this));

    connect(target, &Target::deploymentDataChanged,
            this, &RemoteLinuxRunConfiguration::handleBuildSystemDataUpdated);
    connect(target, &Target::applicationTargetsChanged,
            this, &RemoteLinuxRunConfiguration::handleBuildSystemDataUpdated);
    // Handles device changes, etc.
    connect(target, &Target::kitChanged,
            this, &RemoteLinuxRunConfiguration::handleBuildSystemDataUpdated);
}

void RemoteLinuxRunConfiguration::initialize(Core::Id id, const QString &targetName)
{
    RunConfiguration::initialize(id);

    d->targetName = targetName;

    setDefaultDisplayName(defaultDisplayName());
}

void RemoteLinuxRunConfiguration::copyFrom(const RemoteLinuxRunConfiguration *source)
{
    RunConfiguration::copyFrom(source);
    *d = *source->d;

    setDefaultDisplayName(defaultDisplayName());
}

RemoteLinuxRunConfiguration::~RemoteLinuxRunConfiguration()
{
    delete d;
}

QWidget *RemoteLinuxRunConfiguration::createConfigurationWidget()
{
    return new RemoteLinuxRunConfigurationWidget(this);
}

OutputFormatter *RemoteLinuxRunConfiguration::createOutputFormatter() const
{
    return new QtSupport::QtOutputFormatter(target()->project());
}

Runnable RemoteLinuxRunConfiguration::runnable() const
{
    StandardRunnable r;
    r.environment = extraAspect<RemoteLinuxEnvironmentAspect>()->environment();
    r.executable = remoteExecutableFilePath();
    r.commandLineArguments = arguments();
    r.workingDirectory = workingDirectory();
    return r;
}

QVariantMap RemoteLinuxRunConfiguration::toMap() const
{
    QVariantMap map(RunConfiguration::toMap());
    map.insert(QLatin1String(ArgumentsKey), d->arguments);
    map.insert(QLatin1String(TargetNameKey), d->targetName);
    map.insert(QLatin1String(UseAlternateExeKey), d->useAlternateRemoteExecutable);
    map.insert(QLatin1String(AlternateExeKey), d->alternateRemoteExecutable);
    map.insert(QLatin1String(WorkingDirectoryKey), d->workingDirectory);
    return map;
}

QString RemoteLinuxRunConfiguration::buildSystemTarget() const
{
    return d->targetName;
}

bool RemoteLinuxRunConfiguration::fromMap(const QVariantMap &map)
{
    if (!RunConfiguration::fromMap(map))
        return false;

    QVariant args = map.value(QLatin1String(ArgumentsKey));
    if (args.type() == QVariant::StringList) // Until 3.7 a QStringList was stored.
        d->arguments = QtcProcess::joinArgs(args.toStringList(), OsTypeLinux);
    else
        d->arguments = args.toString();
    d->targetName = map.value(QLatin1String(TargetNameKey)).toString();
    d->useAlternateRemoteExecutable = map.value(QLatin1String(UseAlternateExeKey), false).toBool();
    d->alternateRemoteExecutable = map.value(QLatin1String(AlternateExeKey)).toString();
    d->workingDirectory = map.value(QLatin1String(WorkingDirectoryKey)).toString();

    setDefaultDisplayName(defaultDisplayName());

    return true;
}

QString RemoteLinuxRunConfiguration::defaultDisplayName()
{
    if (!d->targetName.isEmpty())
        //: %1 is the name of a project which is being run on remote Linux
        return tr("%1 (on Remote Device)").arg(d->targetName);
    //: Remote Linux run configuration default display name
    return tr("Run on Remote Device");
}

QString RemoteLinuxRunConfiguration::arguments() const
{
    return d->arguments;
}

QString RemoteLinuxRunConfiguration::localExecutableFilePath() const
{
    return target()->applicationTargets().targetFilePath(d->targetName).toString();
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

void RemoteLinuxRunConfiguration::handleBuildSystemDataUpdated()
{
    emit deploySpecsChanged();
    emit targetInformationChanged();
    emit enabledChanged();
}

const char *RemoteLinuxRunConfiguration::IdPrefix = "RemoteLinuxRunConfiguration:";

} // namespace RemoteLinux
