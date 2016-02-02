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

#include "localenvironmentaspect.h"

#include "buildconfiguration.h"
#include "environmentaspectwidget.h"
#include "runnables.h"
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

    if (m_baseEnvironmentModifier)
        m_baseEnvironmentModifier(runConfiguration(), env);

    return env;
}

void LocalEnvironmentAspect::buildEnvironmentHasChanged()
{
    if (baseEnvironmentBase() == static_cast<int>(BuildEnvironmentBase))
        emit environmentChanged();
}

LocalEnvironmentAspect::LocalEnvironmentAspect(RunConfiguration *parent,
                                               const BaseEnvironmentModifier &modifier) :
    EnvironmentAspect(parent), m_baseEnvironmentModifier(modifier)
{
    connect(parent->target(), &Target::environmentChanged,
            this, &LocalEnvironmentAspect::buildEnvironmentHasChanged);
}

LocalEnvironmentAspect *LocalEnvironmentAspect::create(RunConfiguration *parent) const
{
    auto result = new LocalEnvironmentAspect(parent, m_baseEnvironmentModifier);
    result->setUserEnvironmentChanges(userEnvironmentChanges());
    return result;
}

} // namespace ProjectExplorer
