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
#include "kit.h"
#include "target.h"

#include <utils/qtcassert.h>

namespace ProjectExplorer {

enum BaseEnvironmentBase {
    CleanEnvironmentBase = 0,
    SystemEnvironmentBase,
    BuildEnvironmentBase
};

Utils::Environment LocalEnvironmentAspect::baseEnvironment() const
{
    int base = baseEnvironmentBase();
    Utils::Environment env;
    if (base == static_cast<int>(BuildEnvironmentBase)) {
        if (BuildConfiguration *bc = m_target->activeBuildConfiguration()) {
            env = bc->environment();
        } else { // Fallback for targets without buildconfigurations:
            env = Utils::Environment::systemEnvironment();
            m_target->kit()->addToEnvironment(env);
        }
    } else if (base == static_cast<int>(SystemEnvironmentBase)) {
        env = Utils::Environment::systemEnvironment();
    }

    if (m_baseEnvironmentModifier)
        m_baseEnvironmentModifier(env);

    return env;
}

void LocalEnvironmentAspect::buildEnvironmentHasChanged()
{
    if (baseEnvironmentBase() == static_cast<int>(BuildEnvironmentBase))
        emit environmentChanged();
}

LocalEnvironmentAspect::LocalEnvironmentAspect(Target *target,
                                               const BaseEnvironmentModifier &modifier) :
    m_baseEnvironmentModifier(modifier),
    m_target(target)
{
    addPreferredBaseEnvironment(BuildEnvironmentBase, tr("Build Environment"));
    addSupportedBaseEnvironment(SystemEnvironmentBase, tr("System Environment"));
    addSupportedBaseEnvironment(CleanEnvironmentBase, tr("Clean Environment"));

    m_target->subscribeSignal(&BuildConfiguration::environmentChanged,
                              this, &LocalEnvironmentAspect::buildEnvironmentHasChanged);
    connect(m_target, &Target::activeBuildConfigurationChanged,
            this, &LocalEnvironmentAspect::buildEnvironmentHasChanged);
}

} // namespace ProjectExplorer
