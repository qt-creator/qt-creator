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

using namespace Utils;

static const char BASE_KEY[] = "PE.EnvironmentAspect.Base";
static const char CHANGES_KEY[] = "PE.EnvironmentAspect.Changes";

namespace ProjectExplorer {

// --------------------------------------------------------------------
// EnvironmentAspect:
// --------------------------------------------------------------------

EnvironmentAspect::EnvironmentAspect()
{
    setDisplayName(tr("Environment"));
    setId("EnvironmentAspect");
    setConfigWidgetCreator([this] { return new EnvironmentAspectWidget(this); });
}

int EnvironmentAspect::baseEnvironmentBase() const
{
    return m_base;
}

void EnvironmentAspect::setBaseEnvironmentBase(int base)
{
    QTC_ASSERT(base >= 0 && base < m_baseEnvironments.size(), return);
    if (m_base != base) {
        m_base = base;
        emit baseEnvironmentChanged();
    }
}

void EnvironmentAspect::setUserEnvironmentChanges(const Utils::EnvironmentItems &diff)
{
    if (m_userChanges != diff) {
        m_userChanges = diff;
        emit userEnvironmentChangesChanged(m_userChanges);
        emit environmentChanged();
    }
}

Utils::Environment EnvironmentAspect::environment() const
{
    Environment env = modifiedBaseEnvironment();
    env.modify(m_userChanges);
    return env;
}

Environment EnvironmentAspect::modifiedBaseEnvironment() const
{
    QTC_ASSERT(m_base >= 0 && m_base < m_baseEnvironments.size(), return Environment());
    Environment env = m_baseEnvironments.at(m_base).unmodifiedBaseEnvironment();
    for (const EnvironmentModifier &modifier : m_modifiers)
        modifier(env);
    return env;
}

const QStringList EnvironmentAspect::displayNames() const
{
    return Utils::transform(m_baseEnvironments, &BaseEnvironment::displayName);
}

void EnvironmentAspect::addModifier(const EnvironmentAspect::EnvironmentModifier &modifier)
{
    m_modifiers.append(modifier);
}

void EnvironmentAspect::addSupportedBaseEnvironment(const QString &displayName,
                                                    const std::function<Environment()> &getter)
{
    BaseEnvironment baseEnv;
    baseEnv.displayName = displayName;
    baseEnv.getter = getter;
    m_baseEnvironments.append(baseEnv);
    if (m_base == -1)
        setBaseEnvironmentBase(m_baseEnvironments.size() - 1);
}

void EnvironmentAspect::addPreferredBaseEnvironment(const QString &displayName,
                                                    const std::function<Environment()> &getter)
{
    BaseEnvironment baseEnv;
    baseEnv.displayName = displayName;
    baseEnv.getter = getter;
    m_baseEnvironments.append(baseEnv);
    setBaseEnvironmentBase(m_baseEnvironments.size() - 1);
}

void EnvironmentAspect::fromMap(const QVariantMap &map)
{
    m_base = map.value(QLatin1String(BASE_KEY), -1).toInt();
    m_userChanges = Utils::EnvironmentItem::fromStringList(map.value(QLatin1String(CHANGES_KEY)).toStringList());
}

void EnvironmentAspect::toMap(QVariantMap &data) const
{
    data.insert(QLatin1String(BASE_KEY), m_base);
    data.insert(QLatin1String(CHANGES_KEY), Utils::EnvironmentItem::toStringList(m_userChanges));
}

QString EnvironmentAspect::currentDisplayName() const
{
    QTC_ASSERT(m_base >= 0 && m_base < m_baseEnvironments.size(), return {});
    return m_baseEnvironments[m_base].displayName;
}

Environment EnvironmentAspect::BaseEnvironment::unmodifiedBaseEnvironment() const
{
    return getter ? getter() : Environment();
}

} // namespace ProjectExplorer
