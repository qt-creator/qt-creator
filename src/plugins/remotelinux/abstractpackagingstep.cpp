/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/
#include "abstractpackagingstep.h"

#include "deployablefile.h"
#include "deploymentinfo.h"
#include "remotelinuxdeployconfiguration.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <utils/fileutils.h>

#include <QtCore/QDateTime>
#include <QtCore/QFileInfo>

using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {

class AbstractPackagingStepPrivate
{
public:
    AbstractPackagingStepPrivate() : currentBuildConfiguration(0), running(false) { }

    BuildConfiguration *currentBuildConfiguration;
    bool running;
    QString cachedPackageDirectory;
};

} // namespace Internal

AbstractPackagingStep::AbstractPackagingStep(BuildStepList *bsl, const QString &id)
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
    m_d = new Internal::AbstractPackagingStepPrivate;
    connect(target(), SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
        SLOT(handleBuildConfigurationChanged()));
    handleBuildConfigurationChanged();
}

AbstractPackagingStep::~AbstractPackagingStep()
{
    delete m_d;
}

void AbstractPackagingStep::handleBuildConfigurationChanged()
{
    if (m_d->currentBuildConfiguration)
        disconnect(m_d->currentBuildConfiguration, 0, this, 0);
    m_d->currentBuildConfiguration = target()->activeBuildConfiguration();
    if (m_d->currentBuildConfiguration) {
        connect(m_d->currentBuildConfiguration, SIGNAL(buildDirectoryChanged()), this,
            SIGNAL(packageFilePathChanged()));
    }
    emit packageFilePathChanged();
}

QString AbstractPackagingStep::packageFilePath() const
{
    if (packageDirectory().isEmpty())
        return QString();
    return packageDirectory() + QLatin1Char('/') + packageFileName();
}

QString AbstractPackagingStep::packageDirectory() const
{
    if (m_d->running)
        return m_d->cachedPackageDirectory;
    return m_d->currentBuildConfiguration
        ? m_d->currentBuildConfiguration->buildDirectory() : QString();
}

RemoteLinuxDeployConfiguration *AbstractPackagingStep::deployConfiguration() const
{
    return qobject_cast<RemoteLinuxDeployConfiguration *>(parent()->parent());
}

bool AbstractPackagingStep::isPackagingNeeded() const
{
    const QSharedPointer<DeploymentInfo> &deploymentInfo = deployConfiguration()->deploymentInfo();
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
    m_d->cachedPackageDirectory = packageDirectory();
    return true;
}

void AbstractPackagingStep::setPackagingStarted()
{
    m_d->running = true;
}

void AbstractPackagingStep::setPackagingFinished(bool success)
{
    m_d->running = false;
    if (success)
        deployConfiguration()->deploymentInfo()->setUnmodified();
}

void AbstractPackagingStep::raiseError(const QString &errorMessage)
{
    emit addOutput(errorMessage, BuildStep::ErrorOutput);
    emit addTask(Task(Task::Error, errorMessage, QString(), -1,
        Constants::TASK_CATEGORY_BUILDSYSTEM));
}

} // namespace RemoteLinux
