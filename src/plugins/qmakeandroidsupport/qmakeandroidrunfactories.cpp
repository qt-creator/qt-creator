/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "qmakeandroidrunfactories.h"
#include "qmakeandroidrunconfiguration.h"

#include <android/androidconstants.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <qmakeprojectmanager/qmakenodes.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>

#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>

using namespace Android;
using namespace ProjectExplorer;
using namespace QmakeProjectManager;

namespace QmakeAndroidSupport {
namespace Internal {

static const char ANDROID_RC_ID_PREFIX[] = "Qt4ProjectManager.AndroidRunConfiguration:";

QmakeAndroidRunConfigurationFactory::QmakeAndroidRunConfigurationFactory(QObject *parent)
    : IRunConfigurationFactory(parent)
{
    registerRunConfiguration<QmakeAndroidRunConfiguration>(ANDROID_RC_ID_PREFIX);
    addSupportedProjectType(QmakeProjectManager::Constants::QMAKEPROJECT_ID);
    setSupportedTargetDeviceTypes({Android::Constants::ANDROID_DEVICE_TYPE});
}

QList<BuildTargetInfo>
    QmakeAndroidRunConfigurationFactory::availableBuildTargets(Target *parent, CreationMode mode) const
{
    auto project = qobject_cast<QmakeProject *>(parent->project());
    QTC_ASSERT(project, return {});
    return project->buildTargets(mode, {ProjectType::ApplicationTemplate, ProjectType::SharedLibraryTemplate});
}

} // namespace Internal
} // namespace Android
