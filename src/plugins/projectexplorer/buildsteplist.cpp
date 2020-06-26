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

#include "buildsteplist.h"

#include "buildconfiguration.h"
#include "buildmanager.h"
#include "buildstep.h"
#include "deployconfiguration.h"
#include "projectexplorer.h"
#include "target.h"

#include <utils/algorithm.h>

namespace ProjectExplorer {

const char STEPS_COUNT_KEY[] = "ProjectExplorer.BuildStepList.StepsCount";
const char STEPS_PREFIX[] = "ProjectExplorer.BuildStepList.Step.";

BuildStepList::BuildStepList(QObject *parent, Utils::Id id)
    : QObject(parent), m_id(id)
{
    QTC_ASSERT(parent, return);
    QTC_ASSERT(parent->parent(), return);
    m_target = qobject_cast<Target *>(parent->parent());
    QTC_ASSERT(m_target, return);
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

QVariantMap BuildStepList::toMap() const
{
    QVariantMap map;

    {
        // Only written for compatibility reasons within the 4.11 cycle
        const char CONFIGURATION_ID_KEY[] = "ProjectExplorer.ProjectConfiguration.Id";
        const char DISPLAY_NAME_KEY[] = "ProjectExplorer.ProjectConfiguration.DisplayName";
        const char DEFAULT_DISPLAY_NAME_KEY[] = "ProjectExplorer.ProjectConfiguration.DefaultDisplayName";
        map.insert(QLatin1String(CONFIGURATION_ID_KEY), m_id.toSetting());
        map.insert(QLatin1String(DISPLAY_NAME_KEY), displayName());
        map.insert(QLatin1String(DEFAULT_DISPLAY_NAME_KEY), displayName());
    }

    // Save build steps
    map.insert(QString::fromLatin1(STEPS_COUNT_KEY), m_steps.count());
    for (int i = 0; i < m_steps.count(); ++i)
        map.insert(QString::fromLatin1(STEPS_PREFIX) + QString::number(i), m_steps.at(i)->toMap());

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
        return tr("Build");
    }
    if (m_id == Constants::BUILDSTEPS_CLEAN) {
        //: Display name of the clean build step list. Used as part of the labels in the project window.
        return tr("Clean");
    }
    if (m_id == Constants::BUILDSTEPS_DEPLOY) {
        //: Display name of the deploy build step list. Used as part of the labels in the project window.
        return tr("Deploy");
    }
    QTC_CHECK(false);
    return {};
}

bool BuildStepList::fromMap(const QVariantMap &map)
{
    clear();

    const QList<BuildStepFactory *> factories = BuildStepFactory::allBuildStepFactories();

    int maxSteps = map.value(QString::fromLatin1(STEPS_COUNT_KEY), 0).toInt();
    for (int i = 0; i < maxSteps; ++i) {
        QVariantMap bsData(map.value(QString::fromLatin1(STEPS_PREFIX) + QString::number(i)).toMap());
        if (bsData.isEmpty()) {
            qWarning() << "No step data found for" << i << "(continuing).";
            continue;
        }
        bool handled = false;
        Utils::Id stepId = idFromMap(bsData);
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
        if (BuildStep *step = factory->create(this, stepId)) {
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
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
    m_steps.swapItemsAt(position - 1, position);
#else
    m_steps.swap(position - 1, position);
#endif
    emit stepMoved(position, position - 1);
}

BuildStep *BuildStepList::at(int position)
{
    return m_steps.at(position);
}

} // ProjectExplorer
