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

#include "deployconfiguration.h"

#include "buildsteplist.h"
#include "buildconfiguration.h"
#include "kitinformation.h"
#include "project.h"
#include "projectexplorer.h"
#include "target.h"

#include <extensionsystem/pluginmanager.h>

using namespace ProjectExplorer;

const char BUILD_STEP_LIST_COUNT[] = "ProjectExplorer.BuildConfiguration.BuildStepListCount";
const char BUILD_STEP_LIST_PREFIX[] = "ProjectExplorer.BuildConfiguration.BuildStepList.";

DeployConfiguration::DeployConfiguration(Target *target, Core::Id id) :
    ProjectConfiguration(target, id),
    m_stepList(0)
{
    Q_ASSERT(target);
    m_stepList = new BuildStepList(this, Core::Id(Constants::BUILDSTEPS_DEPLOY));
    //: Display name of the deploy build step list. Used as part of the labels in the project window.
    m_stepList->setDefaultDisplayName(tr("Deploy"));
    //: Default DeployConfiguration display name
    setDefaultDisplayName(tr("Deploy locally"));
    ctor();
}

DeployConfiguration::DeployConfiguration(Target *target, DeployConfiguration *source) :
    ProjectConfiguration(target, source),
    m_stepList(0)
{
    Q_ASSERT(target);
    // Do not clone stepLists here, do that in the derived constructor instead
    // otherwise BuildStepFactories might reject to set up a BuildStep for us
    // since we are not yet the derived class!
    ctor();
}

void DeployConfiguration::ctor()
{
    Utils::MacroExpander *expander = macroExpander();
    expander->setDisplayName(tr("Deploy Settings"));
    expander->setAccumulating(true);
    expander->registerSubProvider([this]() -> Utils::MacroExpander * {
        BuildConfiguration *bc = target()->activeBuildConfiguration();
        return bc ? bc->macroExpander() : target()->macroExpander();
    });
}

DeployConfiguration::~DeployConfiguration()
{
    delete m_stepList;
}

BuildStepList *DeployConfiguration::stepList() const
{
    return m_stepList;
}

QVariantMap DeployConfiguration::toMap() const
{
    QVariantMap map(ProjectConfiguration::toMap());
    map.insert(QLatin1String(BUILD_STEP_LIST_COUNT), 1);
    map.insert(QLatin1String(BUILD_STEP_LIST_PREFIX) + QLatin1Char('0'), m_stepList->toMap());
    return map;
}

NamedWidget *DeployConfiguration::createConfigWidget()
{
    return 0;
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
        delete m_stepList;
        m_stepList = new BuildStepList(this, data);
        if (m_stepList->isNull()) {
            qWarning() << "Failed to restore deploy step list";
            delete m_stepList;
            m_stepList = 0;
            return false;
        }
        m_stepList->setDefaultDisplayName(tr("Deploy"));
    } else {
        qWarning() << "No data for deploy step list found!";
        return false;
    }

    // We assume that we hold the deploy list
    Q_ASSERT(m_stepList && m_stepList->id() == Constants::BUILDSTEPS_DEPLOY);

    return true;
}

Target *DeployConfiguration::target() const
{
    return static_cast<Target *>(parent());
}

void DeployConfiguration::cloneSteps(DeployConfiguration *source)
{
    if (source == this)
        return;
    delete m_stepList;
    m_stepList = new BuildStepList(this, source->stepList());
    m_stepList->cloneSteps(source->stepList());
}

///
// DefaultDeployConfiguration
///
DefaultDeployConfiguration::DefaultDeployConfiguration(Target *target, Core::Id id)
    : DeployConfiguration(target, id)
{

}

DefaultDeployConfiguration::DefaultDeployConfiguration(Target *target, DeployConfiguration *source)
    : DeployConfiguration(target, source)
{
    cloneSteps(source);
}

///
// DeployConfigurationFactory
///

DeployConfigurationFactory::DeployConfigurationFactory(QObject *parent) :
    QObject(parent)
{ setObjectName(QLatin1String("DeployConfigurationFactory")); }

DeployConfigurationFactory::~DeployConfigurationFactory()
{ }

DeployConfigurationFactory *DeployConfigurationFactory::find(Target *parent, const QVariantMap &map)
{
    return ExtensionSystem::PluginManager::getObject<DeployConfigurationFactory>(
        [&parent, &map](DeployConfigurationFactory *factory) {
            return factory->canRestore(parent, map);
        });
}

QList<DeployConfigurationFactory *> DeployConfigurationFactory::find(Target *parent)
{
    return ExtensionSystem::PluginManager::getObjects<DeployConfigurationFactory>(
        [&parent](DeployConfigurationFactory *factory) {
            return !factory->availableCreationIds(parent).isEmpty();
        });
}

DeployConfigurationFactory *DeployConfigurationFactory::find(Target *parent, DeployConfiguration *dc)
{
    return ExtensionSystem::PluginManager::getObject<DeployConfigurationFactory>(
        [&parent, &dc](DeployConfigurationFactory *factory) {
            return factory->canClone(parent, dc);
        });
}

///
// DefaultDeployConfigurationFactory
///

QList<Core::Id> DefaultDeployConfigurationFactory::availableCreationIds(Target *parent) const
{
    if (!canHandle(parent))
        return QList<Core::Id>();
    return QList<Core::Id>() << Core::Id(Constants::DEFAULT_DEPLOYCONFIGURATION_ID);
}

QString DefaultDeployConfigurationFactory::displayNameForId(Core::Id id) const
{
    if (id == Constants::DEFAULT_DEPLOYCONFIGURATION_ID)
        //: Display name of the default deploy configuration
        return DeployConfigurationFactory::tr("Deploy Configuration");
    return QString();
}

bool DefaultDeployConfigurationFactory::canCreate(Target *parent, Core::Id id) const
{
    if (!canHandle(parent))
        return false;
    return id == Constants::DEFAULT_DEPLOYCONFIGURATION_ID;
}

DeployConfiguration *DefaultDeployConfigurationFactory::create(Target *parent, Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;
    return new DefaultDeployConfiguration(parent, id);
}

bool DefaultDeployConfigurationFactory::canRestore(Target *parent, const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map));
}

DeployConfiguration *DefaultDeployConfigurationFactory::restore(Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    DefaultDeployConfiguration *dc = new DefaultDeployConfiguration(parent, idFromMap(map));
    if (!dc->fromMap(map)) {
        delete dc;
        return 0;
    }
    return dc;
}

bool DefaultDeployConfigurationFactory::canClone(Target *parent, DeployConfiguration *product) const
{
    return canCreate(parent, product->id());
}

DeployConfiguration *DefaultDeployConfigurationFactory::clone(Target *parent, DeployConfiguration *product)
{
    if (!canClone(parent, product))
        return 0;
    return new DefaultDeployConfiguration(parent, product);
}

bool DefaultDeployConfigurationFactory::canHandle(Target *parent) const
{
    if (!parent->project()->supportsKit(parent->kit()) || parent->project()->needsSpecialDeployment())
        return false;
    return DeviceTypeKitInformation::deviceTypeId(parent->kit()) == Constants::DESKTOP_DEVICE_TYPE;
}
