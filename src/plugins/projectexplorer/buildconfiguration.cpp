/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "buildconfiguration.h"

#include "buildmanager.h"
#include "buildsteplist.h"
#include "projectexplorer.h"
#include "projectexplorerconstants.h"
#include "target.h"
#include "toolchainmanager.h"
#include "project.h"

#include <coreplugin/variablemanager.h>

#include <QtCore/QProcess>

using namespace ProjectExplorer;

namespace {

const char * const BUILD_STEP_LIST_COUNT("ProjectExplorer.BuildConfiguration.BuildStepListCount");
const char * const BUILD_STEP_LIST_PREFIX("ProjectExplorer.BuildConfiguration.BuildStepList.");
const char * const CLEAR_SYSTEM_ENVIRONMENT_KEY("ProjectExplorer.BuildConfiguration.ClearSystemEnvironment");
const char * const USER_ENVIRONMENT_CHANGES_KEY("ProjectExplorer.BuildConfiguration.UserEnvironmentChanges");
const char * const TOOLCHAIN_KEY("ProjectExplorer.BuildCOnfiguration.ToolChain");

} // namespace

BuildConfiguration::BuildConfiguration(Target *target, const QString &id) :
    ProjectConfiguration(target, id),
    m_clearSystemEnvironment(false),
    m_macroExpander(this),
    m_toolChain(0)
{
    Q_ASSERT(target);
    BuildStepList *bsl = new BuildStepList(this, QLatin1String(Constants::BUILDSTEPS_BUILD));
    //: Display name of the build build step list. Used as part of the labels in the project window.
    bsl->setDefaultDisplayName(tr("Build"));
    m_stepLists.append(bsl);
    bsl = new BuildStepList(this, QLatin1String(Constants::BUILDSTEPS_CLEAN));
    //: Display name of the clean build step list. Used as part of the labels in the project window.
    bsl->setDefaultDisplayName(tr("Clean"));
    m_stepLists.append(bsl);
}

BuildConfiguration::BuildConfiguration(Target *target, BuildConfiguration *source) :
    ProjectConfiguration(target, source),
    m_clearSystemEnvironment(source->m_clearSystemEnvironment),
    m_userEnvironmentChanges(source->m_userEnvironmentChanges),
    m_macroExpander(this),
    m_toolChain(source->m_toolChain)
{
    Q_ASSERT(target);
    // Do not clone stepLists here, do that in the derived constructor instead
    // otherwise BuildStepFactories might reject to set up a BuildStep for us
    // since we are not yet the derived class!
}

BuildConfiguration::~BuildConfiguration()
{ }

QStringList BuildConfiguration::knownStepLists() const
{
    QStringList result;
    foreach (BuildStepList *list, m_stepLists)
        result.append(list->id());
    return result;
}

BuildStepList *BuildConfiguration::stepList(const QString &id) const
{
    foreach (BuildStepList *list, m_stepLists)
        if (id == list->id())
            return list;
    return 0;
}

QVariantMap BuildConfiguration::toMap() const
{
    QVariantMap map(ProjectConfiguration::toMap());
    map.insert(QLatin1String(CLEAR_SYSTEM_ENVIRONMENT_KEY), m_clearSystemEnvironment);
    map.insert(QLatin1String(USER_ENVIRONMENT_CHANGES_KEY), Utils::EnvironmentItem::toStringList(m_userEnvironmentChanges));

    map.insert(QLatin1String(BUILD_STEP_LIST_COUNT), m_stepLists.count());
    for (int i = 0; i < m_stepLists.count(); ++i)
        map.insert(QLatin1String(BUILD_STEP_LIST_PREFIX) + QString::number(i), m_stepLists.at(i)->toMap());

    map.insert(QLatin1String(TOOLCHAIN_KEY), m_toolChain ? m_toolChain->id() : QLatin1String("INVALID"));

    return map;
}

bool BuildConfiguration::fromMap(const QVariantMap &map)
{
    m_clearSystemEnvironment = map.value(QLatin1String(CLEAR_SYSTEM_ENVIRONMENT_KEY)).toBool();
    m_userEnvironmentChanges = Utils::EnvironmentItem::fromStringList(map.value(QLatin1String(USER_ENVIRONMENT_CHANGES_KEY)).toStringList());

    qDeleteAll(m_stepLists);
    m_stepLists.clear();

    int maxI = map.value(QLatin1String(BUILD_STEP_LIST_COUNT), 0).toInt();
    for (int i = 0; i < maxI; ++i) {
        QVariantMap data = map.value(QLatin1String(BUILD_STEP_LIST_PREFIX) + QString::number(i)).toMap();
        if (data.isEmpty()) {
            qWarning() << "No data for build step list" << i << "found!";
            continue;
        }
        BuildStepList *list = new BuildStepList(this, data);
        if (list->isNull()) {
            qWarning() << "Failed to restore build step list" << i;
            delete list;
            return false;
        }
        m_stepLists.append(list);
    }

    QString id = map.value(QLatin1String(TOOLCHAIN_KEY)).toString();
    m_toolChain = ToolChainManager::instance()->findToolChain(id);

    // TODO: We currently assume there to be at least a clean and build list!
    Q_ASSERT(knownStepLists().contains(QLatin1String(ProjectExplorer::Constants::BUILDSTEPS_BUILD)));
    Q_ASSERT(knownStepLists().contains(QLatin1String(ProjectExplorer::Constants::BUILDSTEPS_CLEAN)));

    return ProjectConfiguration::fromMap(map);
}

Target *BuildConfiguration::target() const
{
    return static_cast<Target *>(parent());
}

ProjectExplorer::ToolChain *BuildConfiguration::toolChain() const
{
    return m_toolChain;
}

void BuildConfiguration::setToolChain(ProjectExplorer::ToolChain *tc)
{
    if (m_toolChain == tc)
        return;
    m_toolChain = tc;
    emit toolChainChanged();
}

Utils::Environment BuildConfiguration::baseEnvironment() const
{
    Utils::Environment result;
    if (useSystemEnvironment())
        result = Utils::Environment(QProcess::systemEnvironment());
    return result;
}

QString BuildConfiguration::baseEnvironmentText() const
{
    if (useSystemEnvironment())
        return tr("System Environment");
    else
        return tr("Clean Environment");
}

Utils::Environment BuildConfiguration::environment() const
{
    Utils::Environment env = baseEnvironment();
    env.modify(userEnvironmentChanges());
    return env;
}

void BuildConfiguration::setUseSystemEnvironment(bool b)
{
    if (useSystemEnvironment() == b)
        return;
    m_clearSystemEnvironment = !b;
    emit environmentChanged();
}

bool BuildConfiguration::useSystemEnvironment() const
{
    return !m_clearSystemEnvironment;
}

QList<Utils::EnvironmentItem> BuildConfiguration::userEnvironmentChanges() const
{
    return m_userEnvironmentChanges;
}

void BuildConfiguration::setUserEnvironmentChanges(const QList<Utils::EnvironmentItem> &diff)
{
    if (m_userEnvironmentChanges == diff)
        return;
    m_userEnvironmentChanges = diff;
    emit environmentChanged();
}

void BuildConfiguration::cloneSteps(BuildConfiguration *source)
{
    qDeleteAll(m_stepLists);
    m_stepLists.clear();
    foreach (BuildStepList *bsl, source->m_stepLists) {
        BuildStepList *newBsl = new BuildStepList(this, bsl);
        newBsl->cloneSteps(bsl);
        m_stepLists.append(newBsl);
    }
}

bool BuildConfiguration::isEnabled() const
{
    return true;
}

bool BuildConfigMacroExpander::resolveMacro(const QString &name, QString *ret)
{
    if (name == QLatin1String("sourceDir")) {
        *ret = QDir::toNativeSeparators(m_bc->target()->project()->projectDirectory());
        return true;
    }
    if (name == QLatin1String("buildDir")) {
        *ret = QDir::toNativeSeparators(m_bc->buildDirectory());
        return true;
    }
    *ret = Core::VariableManager::instance()->value(name);
    return !ret->isEmpty();
}

///
// IBuildConfigurationFactory
///

IBuildConfigurationFactory::IBuildConfigurationFactory(QObject *parent) :
    QObject(parent)
{ }

IBuildConfigurationFactory::~IBuildConfigurationFactory()
{ }
