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

#include "deployconfiguration.h"

#include "buildsteplist.h"
#include "buildconfiguration.h"
#include "kitinformation.h"
#include "project.h"
#include "projectexplorer.h"
#include "target.h"

#include <utils/algorithm.h>

namespace ProjectExplorer {

const char BUILD_STEP_LIST_COUNT[] = "ProjectExplorer.BuildConfiguration.BuildStepListCount";
const char BUILD_STEP_LIST_PREFIX[] = "ProjectExplorer.BuildConfiguration.BuildStepList.";

DeployConfiguration::DeployConfiguration(Target *target, Core::Id id)
    : ProjectConfiguration(target, id),
      m_stepList(this, Constants::BUILDSTEPS_DEPLOY)
{
    Utils::MacroExpander *expander = macroExpander();
    expander->setDisplayName(tr("Deploy Settings"));
    expander->setAccumulating(true);
    expander->registerSubProvider([target] {
        BuildConfiguration *bc = target->activeBuildConfiguration();
        return bc ? bc->macroExpander() : target->macroExpander();
    });

    //: Display name of the deploy build step list. Used as part of the labels in the project window.
    m_stepList.setDefaultDisplayName(tr("Deploy"));
    //: Default DeployConfiguration display name
    setDefaultDisplayName(tr("Deploy locally"));
}

void DeployConfiguration::initialize()
{
}

BuildStepList *DeployConfiguration::stepList()
{
    return &m_stepList;
}

const BuildStepList *DeployConfiguration::stepList() const
{
    return &m_stepList;
}

QVariantMap DeployConfiguration::toMap() const
{
    QVariantMap map(ProjectConfiguration::toMap());
    map.insert(QLatin1String(BUILD_STEP_LIST_COUNT), 1);
    map.insert(QLatin1String(BUILD_STEP_LIST_PREFIX) + QLatin1Char('0'), m_stepList.toMap());
    return map;
}

NamedWidget *DeployConfiguration::createConfigWidget()
{
    return nullptr;
}

bool DeployConfiguration::isEnabled() const
{
    return false;
}

QString DeployConfiguration::disabledReason() const
{
    return QString();
}

bool DeployConfiguration::fromMap(const QVariantMap &map)
{
    if (!ProjectConfiguration::fromMap(map))
        return false;

    int maxI = map.value(QLatin1String(BUILD_STEP_LIST_COUNT), 0).toInt();
    if (maxI != 1)
        return false;
    QVariantMap data = map.value(QLatin1String(BUILD_STEP_LIST_PREFIX) + QLatin1Char('0')).toMap();
    if (!data.isEmpty()) {
        m_stepList.clear();
        if (!m_stepList.fromMap(data)) {
            qWarning() << "Failed to restore deploy step list";
            m_stepList.clear();
            return false;
        }
        m_stepList.setDefaultDisplayName(tr("Deploy"));
    } else {
        qWarning() << "No data for deploy step list found!";
        return false;
    }

    return true;
}

Target *DeployConfiguration::target() const
{
    return static_cast<Target *>(parent());
}

Project *DeployConfiguration::project() const
{
    return target()->project();
}

bool DeployConfiguration::isActive() const
{
    return target()->isActive() && target()->activeDeployConfiguration() == this;
}


///
// DeployConfigurationFactory
///

static QList<DeployConfigurationFactory *> g_deployConfigurationFactories;

DeployConfigurationFactory::DeployConfigurationFactory()
{
    g_deployConfigurationFactories.append(this);
}

DeployConfigurationFactory::~DeployConfigurationFactory()
{
    g_deployConfigurationFactories.removeOne(this);
}

QList<Core::Id> DeployConfigurationFactory::availableCreationIds(Target *parent) const
{
    if (!canHandle(parent))
        return {};
    return Utils::transform(availableBuildTargets(parent), [this](const QString &suffix) {
        return m_deployConfigBaseId.withSuffix(suffix);
    });
}

QList<QString> DeployConfigurationFactory::availableBuildTargets(Target *) const
{
    return {QString()};
}

QString DeployConfigurationFactory::displayNameForBuildTarget(const QString &) const
{
    return m_defaultDisplayName;
}

QString DeployConfigurationFactory::displayNameForId(Core::Id id) const
{
    return displayNameForBuildTarget(id.suffixAfter(m_deployConfigBaseId));
}

bool DeployConfigurationFactory::canHandle(Target *target) const
{
    if (m_supportedProjectType.isValid()) {
        if (target->project()->id() != m_supportedProjectType)
            return false;
    }

    if (containsType(target->project()->projectIssues(target->kit()), Task::TaskType::Error))
        return false;

    if (!m_supportedTargetDeviceTypes.isEmpty()) {
        if (!m_supportedTargetDeviceTypes.contains(
                    DeviceTypeKitInformation::deviceTypeId(target->kit())))
            return false;
    }

    return true;
}

bool DeployConfigurationFactory::canCreate(Target *parent, Core::Id id) const
{
    if (!canHandle(parent))
        return false;
    if (!id.name().startsWith(m_deployConfigBaseId.name()))
        return false;
    return true;
}

DeployConfiguration *DeployConfigurationFactory::create(Target *parent, Core::Id id)
{
    if (!canCreate(parent, id))
        return nullptr;
    QTC_ASSERT(m_creator, return nullptr);
    DeployConfiguration *dc = m_creator(parent);
    if (!dc)
        return nullptr;
    dc->initialize();
    return dc;
}

DeployConfiguration *DeployConfigurationFactory::clone(Target *parent,
                                                       const DeployConfiguration *source)
{
    return restore(parent, source->toMap());
}

DeployConfiguration *DeployConfigurationFactory::restore(Target *parent, const QVariantMap &map)
{
    const Core::Id id = idFromMap(map);
    DeployConfigurationFactory *factory = Utils::findOrDefault(g_deployConfigurationFactories,
        [parent, id](DeployConfigurationFactory *f) {
            if (!f->canHandle(parent))
                return false;
            return id.name().startsWith(f->m_deployConfigBaseId.name());
        });
    if (!factory)
        return nullptr;
    QTC_ASSERT(factory->m_creator, return nullptr);
    DeployConfiguration *dc = factory->m_creator(parent);
    QTC_ASSERT(dc, return nullptr);
    if (!dc->fromMap(map)) {
        delete dc;
        dc = nullptr;
    }
    return dc;
}

QList<DeployConfigurationFactory *> DeployConfigurationFactory::find(Target *parent)
{
    return Utils::filtered(g_deployConfigurationFactories,
        [&parent](DeployConfigurationFactory *factory) {
            return !factory->availableCreationIds(parent).isEmpty();
        });
}

void DeployConfigurationFactory::addSupportedTargetDeviceType(Core::Id id)
{
    m_supportedTargetDeviceTypes.append(id);
}

void DeployConfigurationFactory::setDefaultDisplayName(const QString &defaultDisplayName)
{
    m_defaultDisplayName = defaultDisplayName;
}

void DeployConfigurationFactory::setSupportedProjectType(Core::Id id)
{
    m_supportedProjectType = id;
}

///
// DefaultDeployConfigurationFactory
///

DefaultDeployConfigurationFactory::DefaultDeployConfigurationFactory()
{
    registerDeployConfiguration<DeployConfiguration>("ProjectExplorer.DefaultDeployConfiguration");
    addSupportedTargetDeviceType(Constants::DESKTOP_DEVICE_TYPE);
    //: Display name of the default deploy configuration
    setDefaultDisplayName(DeployConfiguration::tr("Deploy Configuration"));
}

bool DefaultDeployConfigurationFactory::canHandle(Target *parent) const
{
    return DeployConfigurationFactory::canHandle(parent)
            && !parent->project()->needsSpecialDeployment();
}

} // namespace ProjectExplorer
