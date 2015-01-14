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

#include "environmentaspect.h"

#include "environmentaspectwidget.h"
#include "target.h"

#include <utils/qtcassert.h>

static const char BASE_KEY[] = "PE.EnvironmentAspect.Base";
static const char CHANGES_KEY[] = "PE.EnvironmentAspect.Changes";

namespace ProjectExplorer {

// --------------------------------------------------------------------
// EnvironmentAspect:
// --------------------------------------------------------------------

EnvironmentAspect::EnvironmentAspect(RunConfiguration *runConfig)
    : IRunConfigurationAspect(runConfig), m_base(-1)
{
    setDisplayName(tr("Run Environment"));
    setId("EnvironmentAspect");
}

RunConfigWidget *EnvironmentAspect::createConfigurationWidget()
{
    return new EnvironmentAspectWidget(this);
}

int EnvironmentAspect::baseEnvironmentBase() const
{
    if (m_base == -1) {
        QList<int> bases = possibleBaseEnvironments();
        QTC_ASSERT(!bases.isEmpty(), return -1);
        foreach (int i, bases)
            QTC_CHECK(i >= 0);
        m_base = bases.at(0);
    }
    return m_base;
}

void EnvironmentAspect::setBaseEnvironmentBase(int base)
{
    QTC_ASSERT(base >= 0, return);
    QTC_ASSERT(possibleBaseEnvironments().contains(base), return);

    if (m_base != base) {
        m_base = base;
        emit baseEnvironmentChanged();
    }
}

void EnvironmentAspect::setUserEnvironmentChanges(const QList<Utils::EnvironmentItem> &diff)
{
    if (m_changes != diff) {
        m_changes = diff;
        emit userEnvironmentChangesChanged(m_changes);
        emit environmentChanged();
    }
}

Utils::Environment EnvironmentAspect::environment() const
{
    Utils::Environment env = baseEnvironment();
    env.modify(m_changes);
    return env;
}

void EnvironmentAspect::fromMap(const QVariantMap &map)
{
    m_base = map.value(QLatin1String(BASE_KEY), -1).toInt();
    m_changes = Utils::EnvironmentItem::fromStringList(map.value(QLatin1String(CHANGES_KEY)).toStringList());
}

void EnvironmentAspect::toMap(QVariantMap &data) const
{
    data.insert(QLatin1String(BASE_KEY), m_base);
    data.insert(QLatin1String(CHANGES_KEY), Utils::EnvironmentItem::toStringList(m_changes));
}

} // namespace ProjectExplorer
