/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: KDAB (info@kdab.com)
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

#include "qnxrunconfigurationfactory.h"

#include "qnxconstants.h"
#include "qnxrunconfiguration.h"
#include "qnxdevicefactory.h"

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <qmakeprojectmanager/qmakeproject.h>

namespace Qnx {
namespace Internal {

QnxRunConfigurationFactory::QnxRunConfigurationFactory(QObject *parent) :
    ProjectExplorer::IRunConfigurationFactory(parent)
{
    registerRunConfiguration<QnxRunConfiguration>(Constants::QNX_QNX_RUNCONFIGURATION_PREFIX);
    setSupportedTargetDeviceTypes({Constants::QNX_QNX_OS_TYPE});
}

QList<QString> QnxRunConfigurationFactory::availableBuildTargets(ProjectExplorer::Target *parent, CreationMode mode) const
{
    auto project = qobject_cast<QmakeProjectManager::QmakeProject *>(parent->project());
    if (!project)
        return {};
    return project->buildTargets(mode);
}

QString QnxRunConfigurationFactory::displayNameForBuildTarget(const QString &buildTarget) const
{
    if (buildTarget.isEmpty())
        return QString();
    return tr("%1 on QNX Device").arg(QFileInfo(buildTarget).completeBaseName());
}

bool QnxRunConfigurationFactory::canCreateHelper(ProjectExplorer::Target *parent,
                                                 const QString &buildTarget) const
{
    auto project = qobject_cast<QmakeProjectManager::QmakeProject *>(parent->project());
    return project->hasApplicationProFile(Utils::FileName::fromString(buildTarget));
}

} // namespace Internal
} // namespace Qnx
