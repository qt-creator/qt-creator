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
#include "deploymentdataview.h"
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

BuildStepList *DeployConfiguration::stepList()
{
    return &m_stepList;
}

const BuildStepList *DeployConfiguration::stepList() const
{
    return &m_stepList;
}

NamedWidget *DeployConfiguration::createConfigWidget() const
{
    if (!m_configWidgetCreator)
        return nullptr;
    return m_configWidgetCreator(target());
}

QVariantMap DeployConfiguration::toMap() const
{
    QVariantMap map(ProjectConfiguration::toMap());
    map.insert(QLatin1String(BUILD_STEP_LIST_COUNT), 1);
    map.insert(QLatin1String(BUILD_STEP_LIST_PREFIX) + QLatin1Char('0'), m_stepList.toMap());
    return map;
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

Core::Id DeployConfigurationFactory::creationId() const
{
    return m_deployConfigBaseId;
}

QString DeployConfigurationFactory::defaultDisplayName() const
{
    return m_defaultDisplayName;
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

void DeployConfigurationFactory::setConfigWidgetCreator(const std::function<NamedWidget *(Target *)> &configWidgetCreator)
{
    m_configWidgetCreator = configWidgetCreator;
}

void DeployConfigurationFactory::setUseDeploymentDataView()
{
    m_configWidgetCreator = [](Target *target) { return new DeploymentDataView(target); };
}

void DeployConfigurationFactory::setConfigBaseId(Core::Id deployConfigBaseId)
{
    m_deployConfigBaseId = deployConfigBaseId;
}

DeployConfiguration *DeployConfigurationFactory::createDeployConfiguration(Target *t)
{
    auto dc = new DeployConfiguration(t, m_deployConfigBaseId);
    dc->setDefaultDisplayName(m_defaultDisplayName);
    dc->m_configWidgetCreator = m_configWidgetCreator;
    return dc;
}

DeployConfiguration *DeployConfigurationFactory::create(Target *parent)
{
    QTC_ASSERT(canHandle(parent), return nullptr);
    DeployConfiguration *dc = createDeployConfiguration(parent);
    QTC_ASSERT(dc, return nullptr);
    dc->stepList()->appendSteps(m_initialSteps);
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
    DeployConfiguration *dc = factory->createDeployConfiguration(parent);
    QTC_ASSERT(dc, return nullptr);
    if (!dc->fromMap(map)) {
        delete dc;
        dc = nullptr;
    }
    return dc;
}

const QList<DeployConfigurationFactory *> DeployConfigurationFactory::find(Target *parent)
{
    return Utils::filtered(g_deployConfigurationFactories,
        [&parent](DeployConfigurationFactory *factory) {
            return factory->canHandle(parent);
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

void DeployConfigurationFactory::addInitialStep(Core::Id stepId, const std::function<bool (Target *)> &condition)
{
    m_initialSteps.append({stepId, condition});
}

///
// DefaultDeployConfigurationFactory
///

DefaultDeployConfigurationFactory::DefaultDeployConfigurationFactory()
{
    setConfigBaseId("ProjectExplorer.DefaultDeployConfiguration");
    addSupportedTargetDeviceType(Constants::DESKTOP_DEVICE_TYPE);
    //: Display name of the default deploy configuration
    setDefaultDisplayName(DeployConfiguration::tr("Deploy Configuration"));
}

} // namespace ProjectExplorer
