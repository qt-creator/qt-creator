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

#include "iosrunfactories.h"

#include "iosconstants.h"
#include "iosrunconfiguration.h"
#include "iosmanager.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/target.h>

#include <qmakeprojectmanager/qmakenodes.h>
#include <qmakeprojectmanager/qmakeproject.h>

using namespace ProjectExplorer;
using namespace QmakeProjectManager;

namespace Ios {
namespace Internal {

IosRunConfigurationFactory::IosRunConfigurationFactory(QObject *parent)
    : QmakeRunConfigurationFactory(parent)
{
    setObjectName("IosRunConfigurationFactory");
    registerRunConfiguration<IosRunConfiguration>(Constants::IOS_RC_ID_PREFIX);
    setSupportedProjectType<QmakeProject>();
}

bool IosRunConfigurationFactory::canCreateHelper(Target *parent, const QString &buildTarget) const
{
    return availableBuildTargets(parent, UserCreate).contains(buildTarget);
}

QList<QString> IosRunConfigurationFactory::availableBuildTargets(Target *parent, CreationMode mode) const
{
    auto project = static_cast<QmakeProject *>(parent->project());
    return project->buildTargets(mode, {ProjectType::ApplicationTemplate,
                                        ProjectType::SharedLibraryTemplate,
                                        ProjectType::AuxTemplate});
}

QString IosRunConfigurationFactory::displayNameForBuildTarget(const QString &buildTarget) const
{
    return QFileInfo(buildTarget).completeBaseName();
}

bool IosRunConfigurationFactory::canHandle(Target *t) const
{
    return IRunConfigurationFactory::canHandle(t) && IosManager::supportsIos(t->kit());
}

QList<RunConfiguration *> IosRunConfigurationFactory::runConfigurationsForNode(Target *t, const Node *n)
{
    QList<RunConfiguration *> result;
    foreach (RunConfiguration *rc, t->runConfigurations()) {
        if (IosRunConfiguration *qt4c = qobject_cast<IosRunConfiguration *>(rc)) {
            if (qt4c->profilePath() == n->filePath())
                result << rc;
        }
    }
    return result;
}

} // namespace Internal
} // namespace Ios
