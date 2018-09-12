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

EnvironmentAspect::EnvironmentAspect()
{
    setDisplayName(tr("Run Environment"));
    setId("EnvironmentAspect");
    setConfigWidgetCreator([this] { return new EnvironmentAspectWidget(this); });
}

int EnvironmentAspect::baseEnvironmentBase() const
{
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

QList<int> EnvironmentAspect::possibleBaseEnvironments() const
{
    return m_displayNames.keys();
}

QString EnvironmentAspect::baseEnvironmentDisplayName(int base) const
{
    return m_displayNames[base];
}

void EnvironmentAspect::addSupportedBaseEnvironment(int base, const QString &displayName)
{
    m_displayNames[base] = displayName;
    if (m_base == -1)
        setBaseEnvironmentBase(base);
}

void EnvironmentAspect::addPreferredBaseEnvironment(int base, const QString &displayName)
{
    m_displayNames[base] = displayName;
    setBaseEnvironmentBase(base);
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
