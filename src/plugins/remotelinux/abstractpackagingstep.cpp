/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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
#include "abstractpackagingstep.h"

#include "deployablefile.h"
#include "deploymentinfo.h"
#include "remotelinuxdeployconfiguration.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/task.h>
#include <utils/fileutils.h>

#include <QDateTime>
#include <QFileInfo>

using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {

class AbstractPackagingStepPrivate
{
public:
    AbstractPackagingStepPrivate() : currentBuildConfiguration(0) { }

    BuildConfiguration *currentBuildConfiguration;
    QString cachedPackageFilePath;
    QString cachedPackageDirectory;
};

} // namespace Internal

AbstractPackagingStep::AbstractPackagingStep(BuildStepList *bsl, const Core::Id id)
    : BuildStep(bsl, id)
{
    ctor();
}

AbstractPackagingStep::AbstractPackagingStep(BuildStepList *bsl, AbstractPackagingStep *other)
    : BuildStep(bsl, other)
{
    ctor();
}

void AbstractPackagingStep::ctor()
{
    d = new Internal::AbstractPackagingStepPrivate;
    connect(target(), SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
        SLOT(handleBuildConfigurationChanged()));
    handleBuildConfigurationChanged();

    connect(this, SIGNAL(unmodifyDeploymentInfo()),
            this, SLOT(setDeploymentInfoUnmodified()));
}

AbstractPackagingStep::~AbstractPackagingStep()
{
    delete d;
}

void AbstractPackagingStep::handleBuildConfigurationChanged()
{
    if (d->currentBuildConfiguration)
        disconnect(d->currentBuildConfiguration, 0, this, 0);
    d->currentBuildConfiguration = target()->activeBuildConfiguration();
    if (d->currentBuildConfiguration) {
        connect(d->currentBuildConfiguration, SIGNAL(buildDirectoryChanged()), this,
            SIGNAL(packageFilePathChanged()));
    }
    emit packageFilePathChanged();
}

QString AbstractPackagingStep::cachedPackageFilePath() const
{
    return d->cachedPackageFilePath;
}

QString AbstractPackagingStep::packageFilePath() const
{
    if (packageDirectory().isEmpty())
        return QString();
    return packageDirectory() + QLatin1Char('/') + packageFileName();
}

QString AbstractPackagingStep::cachedPackageDirectory() const
{
    return d->cachedPackageDirectory;
}

QString AbstractPackagingStep::packageDirectory() const
{
    return d->currentBuildConfiguration
        ? d->currentBuildConfiguration->buildDirectory() : QString();
}

RemoteLinuxDeployConfiguration *AbstractPackagingStep::deployConfiguration() const
{
    return qobject_cast<RemoteLinuxDeployConfiguration *>(parent()->parent());
}

bool AbstractPackagingStep::isPackagingNeeded() const
{
    const DeploymentInfo * const deploymentInfo = deployConfiguration()->deploymentInfo();
    QFileInfo packageInfo(packageFilePath());
    if (!packageInfo.exists() || deploymentInfo->isModified())
        return true;

    const int deployableCount = deploymentInfo->deployableCount();
    for (int i = 0; i < deployableCount; ++i) {
        if (Utils::FileUtils::isFileNewerThan(deploymentInfo->deployableAt(i).localFilePath,
                packageInfo.lastModified()))
            return true;
    }

    return false;
}

bool AbstractPackagingStep::init()
{
    d->cachedPackageDirectory = packageDirectory();
    d->cachedPackageFilePath = packageFilePath();
    return true;
}

void AbstractPackagingStep::setPackagingStarted()
{
}

// called in ::run thread
void AbstractPackagingStep::setPackagingFinished(bool success)
{
    if (success)
        emit unmodifyDeploymentInfo();
}

// called in gui thread
void AbstractPackagingStep::setDeploymentInfoUnmodified()
{
    deployConfiguration()->deploymentInfo()->setUnmodified();
}

void AbstractPackagingStep::raiseError(const QString &errorMessage)
{
    emit addOutput(errorMessage, BuildStep::ErrorOutput);
    emit addTask(Task(Task::Error, errorMessage, Utils::FileName(), -1,
                      Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM)));
}

} // namespace RemoteLinux
