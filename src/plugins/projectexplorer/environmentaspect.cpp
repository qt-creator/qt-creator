// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "environmentaspect.h"

#include "buildconfiguration.h"
#include "environmentaspectwidget.h"
#include "kit.h"
#include "projectexplorertr.h"
#include "target.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

using namespace Utils;

namespace ProjectExplorer {
const char PRINT_ON_RUN_KEY[] = "PE.EnvironmentAspect.PrintOnRun";

// --------------------------------------------------------------------
// EnvironmentAspect:
// --------------------------------------------------------------------

EnvironmentAspect::EnvironmentAspect(AspectContainer *container)
    : BaseAspect(container)
{
    setDisplayName(Tr::tr("Environment"));
    setId("EnvironmentAspect");
    setConfigWidgetCreator([this] { return new EnvironmentAspectWidget(this); });
    addDataExtractor(this, &EnvironmentAspect::environment, &Data::environment);
}

void EnvironmentAspect::setDeviceSelector(Target *target, DeviceSelector selector)
{
    m_target = target;
    m_selector = selector;
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

int EnvironmentAspect::addSupportedBaseEnvironment(const QString &displayName,
                                                   const std::function<Environment()> &getter)
{
    BaseEnvironment baseEnv;
    baseEnv.displayName = displayName;
    baseEnv.getter = getter;
    m_baseEnvironments.append(baseEnv);
    const int index = m_baseEnvironments.size() - 1;
    if (m_base == -1)
        setBaseEnvironmentBase(index);

    return index;
}

int EnvironmentAspect::addPreferredBaseEnvironment(const QString &displayName,
                                                   const std::function<Environment()> &getter)
{
    BaseEnvironment baseEnv;
    baseEnv.displayName = displayName;
    baseEnv.getter = getter;
    m_baseEnvironments.append(baseEnv);
    const int index = m_baseEnvironments.size() - 1;
    setBaseEnvironmentBase(index);

    return index;
}

void EnvironmentAspect::setSupportForBuildEnvironment(Target *target)
{
    setIsLocal(true);
    addSupportedBaseEnvironment(Tr::tr("Clean Environment"), {});

    addSupportedBaseEnvironment(Tr::tr("System Environment"), [] {
        return Environment::systemEnvironment();
    });
    addPreferredBaseEnvironment(Tr::tr("Build Environment"), [target] {
        if (BuildConfiguration *bc = target->activeBuildConfiguration())
            return bc->environment();
        // Fallback for targets without buildconfigurations:
        return target->kit()->buildEnvironment();
    });

    connect(target, &Target::activeBuildConfigurationChanged,
            this, &EnvironmentAspect::environmentChanged);
    connect(target, &Target::buildEnvironmentChanged,
            this, &EnvironmentAspect::environmentChanged);
}

void EnvironmentAspect::fromMap(const Store &map)
{
    m_base = map.value(BASE_KEY, -1).toInt();
    m_userChanges = EnvironmentItem::fromStringList(map.value(CHANGES_KEY).toStringList());
    m_printOnRun = map.value(PRINT_ON_RUN_KEY).toBool();
}

void EnvironmentAspect::toMap(Store &data) const
{
    data.insert(BASE_KEY, m_base);
    data.insert(CHANGES_KEY, EnvironmentItem::toStringList(m_userChanges));
    data.insert(PRINT_ON_RUN_KEY, m_printOnRun);
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
