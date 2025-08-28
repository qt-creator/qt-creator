// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "deployconfiguration.h"

#include "buildconfiguration.h"
#include "buildsteplist.h"
#include "deploymentdataview.h"
#include "devicesupport/devicekitaspects.h"
#include "project.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"
#include "target.h"

#include <utils/algorithm.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>
#include <utils/variablechooser.h>

#include <QDebug>

using namespace Utils;

namespace ProjectExplorer {

const char BUILD_STEP_LIST_COUNT[] = "ProjectExplorer.BuildConfiguration.BuildStepListCount";
const char BUILD_STEP_LIST_PREFIX[] = "ProjectExplorer.BuildConfiguration.BuildStepList.";
const char USES_DEPLOYMENT_DATA[] = "ProjectExplorer.DeployConfiguration.CustomDataEnabled";
const char DEPLOYMENT_DATA[] = "ProjectExplorer.DeployConfiguration.CustomData";

DeployConfiguration::DeployConfiguration(BuildConfiguration *bc, Id id)
    : ProjectConfiguration(bc->target(), id)
    , m_buildConfiguration(bc)
    , m_stepList(this, Constants::BUILDSTEPS_DEPLOY)
{
    //: Default DeployConfiguration display name
    setDefaultDisplayName(Tr::tr("Deploy locally"));

    // Allow to use Device::SshPort, KeyFile and so on on the custom deployment steps
    MacroExpander &expander = *macroExpander();
    expander.setDisplayName(Tr::tr("Run Settings"));
    expander.setAccumulating(true);
    expander.registerSubProvider([bc] { return bc->macroExpander(); });
}

BuildStepList *DeployConfiguration::stepList()
{
    return &m_stepList;
}

const BuildStepList *DeployConfiguration::stepList() const
{
    return &m_stepList;
}

QWidget *DeployConfiguration::createConfigWidget()
{
    if (!m_configWidgetCreator)
        return nullptr;
    QWidget *widget = m_configWidgetCreator(this);
    VariableChooser::addSupportForChildWidgets(widget, macroExpander());
    return widget;
}

void DeployConfiguration::toMap(Store &map) const
{
    ProjectConfiguration::toMap(map);
    map.insert(BUILD_STEP_LIST_COUNT, 1);
    map.insert(Key(BUILD_STEP_LIST_PREFIX) + '0', variantFromStore(m_stepList.toMap()));
    map.insert(USES_DEPLOYMENT_DATA, usesCustomDeploymentData());
    Store deployData;
    for (int i = 0; i < m_customDeploymentData.fileCount(); ++i) {
        const DeployableFile &f = m_customDeploymentData.fileAt(i);
        deployData.insert(keyFromString(f.localFilePath().toUrlishString()), f.remoteDirectory());
    }
    map.insert(DEPLOYMENT_DATA, variantFromStore(deployData));
}

void DeployConfiguration::fromMap(const Store &map)
{
    ProjectConfiguration::fromMap(map);
    if (hasError())
        return;

    int maxI = map.value(BUILD_STEP_LIST_COUNT, 0).toInt();
    if (maxI != 1) {
        reportError();
        return;
    }
    Store data = storeFromVariant(map.value(Key(BUILD_STEP_LIST_PREFIX) + '0'));
    if (!data.isEmpty()) {
        m_stepList.clear();
        if (!m_stepList.fromMap(data)) {
            qWarning() << "Failed to restore deploy step list";
            m_stepList.clear();
            reportError();
            return;
        }
    } else {
        qWarning() << "No data for deploy step list found!";
        reportError();
        return;
    }

    m_usesCustomDeploymentData = map.value(USES_DEPLOYMENT_DATA, false).toBool();
    const Store deployData = storeFromVariant(map.value(DEPLOYMENT_DATA));
    for (auto it = deployData.begin(); it != deployData.end(); ++it)
        m_customDeploymentData.addFile(FilePath::fromString(stringFromKey(it.key())), it.value().toString());
}

bool DeployConfiguration::isActive() const
{
    return project()->activeDeployConfiguration() == this;
}

BuildSystem *DeployConfiguration::buildSystem() const
{
    return buildConfiguration()->buildSystem();
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

Id DeployConfigurationFactory::creationId() const
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
                    RunDeviceTypeKitAspect::deviceTypeId(target->kit())))
            return false;
    }

    return true;
}

void DeployConfigurationFactory::setConfigWidgetCreator(const DeployConfiguration::WidgetCreator &configWidgetCreator)
{
    m_configWidgetCreator = configWidgetCreator;
}

void DeployConfigurationFactory::setUseDeploymentDataView()
{
    m_configWidgetCreator = [](DeployConfiguration *dc) {
        return new Internal::DeploymentDataView(dc);
    };
}

void DeployConfigurationFactory::setConfigBaseId(Id deployConfigBaseId)
{
    m_deployConfigBaseId = deployConfigBaseId;
}

DeployConfiguration *DeployConfigurationFactory::createDeployConfiguration(BuildConfiguration *bc)
{
    auto dc = new DeployConfiguration(bc, m_deployConfigBaseId);
    dc->setDefaultDisplayName(m_defaultDisplayName);
    dc->m_configWidgetCreator = m_configWidgetCreator;
    return dc;
}

DeployConfiguration *DeployConfigurationFactory::create(BuildConfiguration *bc)
{
    QTC_ASSERT(canHandle(bc->target()), return nullptr);
    DeployConfiguration *dc = createDeployConfiguration(bc);
    QTC_ASSERT(dc, return nullptr);
    BuildStepList *stepList = dc->stepList();
    for (const BuildStepList::StepCreationInfo &info : std::as_const(m_initialSteps)) {
        if (!info.condition || info.condition(bc))
            stepList->appendStep(info.stepId);
    }
    return dc;
}

DeployConfiguration *DeployConfigurationFactory::clone(BuildConfiguration *bc,
                                                       const DeployConfiguration *source)
{
    Store map;
    source->toMap(map);
    return restore(bc, map);
}

DeployConfiguration *DeployConfigurationFactory::restore(BuildConfiguration *bc, const Store &map)
{
    const Id id = idFromMap(map);
    DeployConfigurationFactory *factory = Utils::findOrDefault(g_deployConfigurationFactories,
        [bc, id](DeployConfigurationFactory *f) {
            if (!f->canHandle(bc->target()))
                return false;
            return id.name().startsWith(f->m_deployConfigBaseId.name());
        });
    if (!factory)
        return nullptr;
    DeployConfiguration *dc = factory->createDeployConfiguration(bc);
    QTC_ASSERT(dc, return nullptr);
    dc->fromMap(map);
    if (dc->hasError()) {
        delete dc;
        dc = nullptr;
    } else if (factory->postRestore()) {
        factory->postRestore()(dc, map);
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

void DeployConfigurationFactory::addSupportedTargetDeviceType(Utils::Id id)
{
    m_supportedTargetDeviceTypes.append(id);
}

void DeployConfigurationFactory::setDefaultDisplayName(const QString &defaultDisplayName)
{
    m_defaultDisplayName = defaultDisplayName;
}

void DeployConfigurationFactory::setSupportedProjectType(Utils::Id id)
{
    m_supportedProjectType = id;
}

void DeployConfigurationFactory::addInitialStep(
    Utils::Id stepId, const std::function<bool(BuildConfiguration *)> &condition)
{
    m_initialSteps.append({stepId, condition});
}

} // namespace ProjectExplorer
