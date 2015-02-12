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

#include "localenvironmentaspect.h"

#include "buildconfiguration.h"
#include "environmentaspectwidget.h"
#include "localapplicationrunconfiguration.h"
#include "kit.h"
#include "target.h"

#include <utils/qtcassert.h>

namespace ProjectExplorer {

// --------------------------------------------------------------------
// LocalEnvironmentAspect:
// --------------------------------------------------------------------

QList<int> LocalEnvironmentAspect::possibleBaseEnvironments() const
{
    return QList<int>() << static_cast<int>(BuildEnvironmentBase)
                        << static_cast<int>(SystemEnvironmentBase)
                        << static_cast<int>(CleanEnvironmentBase);
}

QString LocalEnvironmentAspect::baseEnvironmentDisplayName(int base) const
{
    if (base == static_cast<int>(BuildEnvironmentBase))
        return tr("Build Environment");
    if (base == static_cast<int>(SystemEnvironmentBase))
        return tr("System Environment");
    if (base == static_cast<int>(CleanEnvironmentBase))
        return tr("Clean Environment");
    return QString();
}

Utils::Environment LocalEnvironmentAspect::baseEnvironment() const
{
    int base = baseEnvironmentBase();
    Utils::Environment env;
    if (base == static_cast<int>(BuildEnvironmentBase)) {
        if (BuildConfiguration *bc = runConfiguration()->target()->activeBuildConfiguration()) {
            env = bc->environment();
        } else { // Fallback for targets without buildconfigurations:
            env = Utils::Environment::systemEnvironment();
            runConfiguration()->target()->kit()->addToEnvironment(env);
        }
    } else if (base == static_cast<int>(SystemEnvironmentBase)) {
        env = Utils::Environment::systemEnvironment();
    }

    if (const LocalApplicationRunConfiguration *rc = qobject_cast<const LocalApplicationRunConfiguration *>(runConfiguration()))
        rc->addToBaseEnvironment(env);

    return env;
}

void LocalEnvironmentAspect::buildEnvironmentHasChanged()
{
    if (baseEnvironmentBase() == static_cast<int>(BuildEnvironmentBase))
        emit environmentChanged();
}

LocalEnvironmentAspect::LocalEnvironmentAspect(RunConfiguration *parent) :
    EnvironmentAspect(parent)
{
    connect(parent->target(), SIGNAL(environmentChanged()),
            this, SLOT(buildEnvironmentHasChanged()));
}

LocalEnvironmentAspect *LocalEnvironmentAspect::create(RunConfiguration *parent) const
{
    LocalEnvironmentAspect *result = new LocalEnvironmentAspect(parent);
    result->setUserEnvironmentChanges(userEnvironmentChanges());
    return result;
}

} // namespace ProjectExplorer
