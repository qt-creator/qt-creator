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

#include "qmlprojectenvironmentaspect.h"

#include "qmlproject.h"

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <projectexplorer/kit.h>
#include <utils/qtcassert.h>

namespace QmlProjectManager {

// --------------------------------------------------------------------
// QmlProjectEnvironmentAspect:
// --------------------------------------------------------------------

QList<int> QmlProjectEnvironmentAspect::possibleBaseEnvironments() const
{
    QList<int> ret;
    if (ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(runConfiguration()->target()->kit())
            == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE) {
        ret << SystemEnvironmentBase;
    }
    ret << CleanEnvironmentBase;
    return ret;
}

QString QmlProjectEnvironmentAspect::baseEnvironmentDisplayName(int base) const
{
    switch (base) {
    case SystemEnvironmentBase:
        return tr("System Environment");
    case CleanEnvironmentBase:
        return tr("Clean Environment");
    default:
        QTC_CHECK(false);
        return QString();
    }
}

Utils::Environment QmlProjectEnvironmentAspect::baseEnvironment() const
{
    Utils::Environment env = baseEnvironmentBase() == SystemEnvironmentBase
            ? Utils::Environment::systemEnvironment()
            : Utils::Environment();

    if (QmlProject *project = qobject_cast<QmlProject *>(runConfiguration()->target()->project()))
        env.modify(project->environment());

    return env;
}

QmlProjectEnvironmentAspect::QmlProjectEnvironmentAspect(ProjectExplorer::RunConfiguration *rc) :
    ProjectExplorer::EnvironmentAspect(rc)
{ }

} // namespace QmlProjectManager
