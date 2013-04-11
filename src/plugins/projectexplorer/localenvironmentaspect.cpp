/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
    if (baseEnvironmentBase() == static_cast<int>(BuildEnvironmentBase)) {
        emit baseEnvironmentChanged();
        emit environmentChanged();
    }
}

LocalEnvironmentAspect::LocalEnvironmentAspect(RunConfiguration *rc) :
    EnvironmentAspect(rc)
{
    connect(rc->target(), SIGNAL(environmentChanged()),
            this, SLOT(buildEnvironmentHasChanged()));
}

LocalEnvironmentAspect::LocalEnvironmentAspect(const LocalEnvironmentAspect *other,
                                               ProjectExplorer::RunConfiguration *parent) :
    EnvironmentAspect(other, parent)
{ }

LocalEnvironmentAspect *LocalEnvironmentAspect::clone(RunConfiguration *parent) const
{
    Q_UNUSED(parent);
    return new LocalEnvironmentAspect(this, parent);
}

} // namespace ProjectExplorer
