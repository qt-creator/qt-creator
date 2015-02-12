/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "remotelinuxrunconfiguration.h"

#include "remotelinuxenvironmentaspect.h"
#include "remotelinuxrunconfigurationwidget.h"

#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/project.h>
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

class RemoteLinuxRunConfigurationPrivate {
public:
    RemoteLinuxRunConfigurationPrivate(const QString &targetName)
        : targetName(targetName),
          useAlternateRemoteExecutable(false)
    {
    }

    RemoteLinuxRunConfigurationPrivate(const RemoteLinuxRunConfigurationPrivate *other)
        : targetName(other->targetName),
          arguments(other->arguments),
          useAlternateRemoteExecutable(other->useAlternateRemoteExecutable),
          alternateRemoteExecutable(other->alternateRemoteExecutable),
          workingDirectory(other->workingDirectory)
    {
    }

    QString targetName;
    QStringList arguments;
    bool useAlternateRemoteExecutable;
    QString alternateRemoteExecutable;
    QString workingDirectory;
};

} // namespace Internal

using namespace Internal;

RemoteLinuxRunConfiguration::RemoteLinuxRunConfiguration(Target *parent, Core::Id id,
        const QString &targetName)
    : AbstractRemoteLinuxRunConfiguration(parent, id),
      d(new RemoteLinuxRunConfigurationPrivate(targetName))
{
    init();
}

RemoteLinuxRunConfiguration::RemoteLinuxRunConfiguration(Target *parent,
        RemoteLinuxRunConfiguration *source)
    : AbstractRemoteLinuxRunConfiguration(parent, source),
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

    addExtraAspect(new RemoteLinuxEnvironmentAspect(this));

    connect(target(), SIGNAL(deploymentDataChanged()), SLOT(handleBuildSystemDataUpdated()));
    connect(target(), SIGNAL(applicationTargetsChanged()), SLOT(handleBuildSystemDataUpdated()));
    connect(target(), SIGNAL(kitChanged()),
            this, SLOT(handleBuildSystemDataUpdated())); // Handles device changes, etc.
}

bool RemoteLinuxRunConfiguration::isEnabled() const
{
    return true;
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
    map.insert(QLatin1String(TargetNameKey), d->targetName);
    map.insert(QLatin1String(UseAlternateExeKey), d->useAlternateRemoteExecutable);
    map.insert(QLatin1String(AlternateExeKey), d->alternateRemoteExecutable);
    map.insert(QLatin1String(WorkingDirectoryKey), d->workingDirectory);
    return map;
}

bool RemoteLinuxRunConfiguration::fromMap(const QVariantMap &map)
{
    if (!RunConfiguration::fromMap(map))
        return false;

    d->arguments = map.value(QLatin1String(ArgumentsKey)).toStringList();
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

QStringList RemoteLinuxRunConfiguration::arguments() const
{
    return d->arguments;
}

Environment RemoteLinuxRunConfiguration::environment() const
{
    RemoteLinuxEnvironmentAspect *aspect = extraAspect<RemoteLinuxEnvironmentAspect>();
    QTC_ASSERT(aspect, return Environment());
    return aspect->environment();
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
    d->arguments = QtcProcess::splitArgs(args, OsTypeLinux); // TODO: Widget should be list-based.
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
    updateEnabledState();
}

const char *RemoteLinuxRunConfiguration::IdPrefix = "RemoteLinuxRunConfiguration:";

} // namespace RemoteLinux
