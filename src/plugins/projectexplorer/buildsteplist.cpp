// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "buildsteplist.h"

#include "buildmanager.h"
#include "buildstep.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"
#include "target.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QDebug>

using namespace Utils;

namespace ProjectExplorer {

const char STEPS_COUNT_KEY[] = "ProjectExplorer.BuildStepList.StepsCount";
const char STEPS_PREFIX[] = "ProjectExplorer.BuildStepList.Step.";

BuildStepList::BuildStepList(ProjectConfiguration *config, Utils::Id id)
    : m_projectConfiguration(config), m_id(id)
{
    QTC_CHECK(config);
}

BuildStepList::~BuildStepList()
{
    clear();
}

void BuildStepList::clear()
{
    qDeleteAll(m_steps);
    m_steps.clear();
}

Target *BuildStepList::target() const
{
    return m_projectConfiguration->target();
}

Store BuildStepList::toMap() const
{
    Store map;

    {
        // Only written for compatibility reasons within the 4.11 cycle
        const char CONFIGURATION_ID_KEY[] = "ProjectExplorer.ProjectConfiguration.Id";
        const char DISPLAY_NAME_KEY[] = "ProjectExplorer.ProjectConfiguration.DisplayName";
        const char DEFAULT_DISPLAY_NAME_KEY[] = "ProjectExplorer.ProjectConfiguration.DefaultDisplayName";
        map.insert(CONFIGURATION_ID_KEY, m_id.toSetting());
        map.insert(DISPLAY_NAME_KEY, displayName());
        map.insert(DEFAULT_DISPLAY_NAME_KEY, displayName());
    }

    // Save build steps
    map.insert(STEPS_COUNT_KEY, m_steps.count());
    for (int i = 0; i < m_steps.count(); ++i) {
        Store data;
        m_steps.at(i)->toMap(data);
        map.insert(numberedKey(STEPS_PREFIX, i), variantFromStore(data));
    }

    return map;
}

int BuildStepList::count() const
{
    return m_steps.count();
}

bool BuildStepList::isEmpty() const
{
    return m_steps.isEmpty();
}

bool BuildStepList::contains(Utils::Id id) const
{
    return Utils::anyOf(steps(), [id](BuildStep *bs){
        return bs->id() == id;
    });
}

QString BuildStepList::displayName() const
{
    if (m_id == Constants::BUILDSTEPS_BUILD) {
        //: Display name of the build build step list. Used as part of the labels in the project window.
        return Tr::tr("Build");
    }
    if (m_id == Constants::BUILDSTEPS_CLEAN) {
        //: Display name of the clean build step list. Used as part of the labels in the project window.
        return Tr::tr("Clean");
    }
    if (m_id == Constants::BUILDSTEPS_DEPLOY) {
        //: Display name of the deploy build step list. Used as part of the labels in the project window.
        return Tr::tr("Deploy");
    }
    QTC_CHECK(false);
    return {};
}

bool BuildStepList::fromMap(const Store &map)
{
    clear();

    const QList<BuildStepFactory *> factories = BuildStepFactory::allBuildStepFactories();

    int maxSteps = map.value(STEPS_COUNT_KEY, 0).toInt();
    for (int i = 0; i < maxSteps; ++i) {
        Store bsData = storeFromVariant(map.value(numberedKey(STEPS_PREFIX, i)));
        if (bsData.isEmpty()) {
            qWarning() << "No step data found for" << i << "(continuing).";
            continue;
        }
        bool handled = false;
        Utils::Id stepId = idFromMap(bsData);

        // pre-8.0 compat
        if (stepId == "RemoteLinux.CheckForFreeDiskSpaceStep")
            continue;

        for (BuildStepFactory *factory : factories) {
            if (factory->stepId() == stepId) {
                if (factory->canHandle(this)) {
                    if (BuildStep *bs = factory->restore(this, bsData)) {
                        appendStep(bs);
                        handled = true;
                    } else {
                        qWarning() << "Restoration of step" << i << "failed (continuing).";
                    }
                }
            }
        }
        QTC_ASSERT(handled, qDebug() << "No factory for build step" << stepId.toString() << "found.");
    }
    return true;
}

QList<BuildStep *> BuildStepList::steps() const
{
    return m_steps;
}

BuildStep *BuildStepList::firstStepWithId(Utils::Id id) const
{
    return Utils::findOrDefault(m_steps, Utils::equal(&BuildStep::id, id));
}

void BuildStepList::insertStep(int position, BuildStep *step)
{
    m_steps.insert(position, step);
    emit stepInserted(position);
}

void BuildStepList::insertStep(int position, Utils::Id stepId)
{
    for (BuildStepFactory *factory : BuildStepFactory::allBuildStepFactories()) {
        if (factory->stepId() == stepId) {
            BuildStep *step = factory->create(this);
            QTC_ASSERT(step, break);
            insertStep(position, step);
            return;
        }
    }
    QTC_ASSERT(false, qDebug() << "No factory for build step" << stepId.toString() << "found.");
}

bool BuildStepList::removeStep(int position)
{
    BuildStep *bs = at(position);
    if (BuildManager::isBuilding(bs))
        return false;

    emit aboutToRemoveStep(position);
    m_steps.removeAt(position);
    delete bs;
    emit stepRemoved(position);
    return true;
}

void BuildStepList::moveStepUp(int position)
{
    m_steps.swapItemsAt(position - 1, position);
    emit stepMoved(position, position - 1);
}

BuildStep *BuildStepList::at(int position) const
{
    return m_steps.at(position);
}

} // ProjectExplorer
