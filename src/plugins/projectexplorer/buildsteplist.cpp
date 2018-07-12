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

using namespace ProjectExplorer;

namespace {

const char STEPS_COUNT_KEY[] = "ProjectExplorer.BuildStepList.StepsCount";
const char STEPS_PREFIX[] = "ProjectExplorer.BuildStepList.Step.";

} // namespace

BuildStepList::BuildStepList(QObject *parent, Core::Id id)
    : ProjectConfiguration(parent, id)
{
    if (id == Constants::BUILDSTEPS_BUILD) {
        //: Display name of the build build step list. Used as part of the labels in the project window.
        setDefaultDisplayName(tr("Build"));
    } else if (id == Constants::BUILDSTEPS_CLEAN) {
        //: Display name of the clean build step list. Used as part of the labels in the project window.
        setDefaultDisplayName(tr("Clean"));
    }
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
    QVariantMap map(ProjectConfiguration::toMap());
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

bool BuildStepList::contains(Core::Id id) const
{
    return Utils::anyOf(steps(), [id](BuildStep *bs){
        return bs->id() == id;
    });
}

bool BuildStepList::isActive() const
{
    return qobject_cast<ProjectConfiguration *>(parent())->isActive();
}

bool BuildStepList::fromMap(const QVariantMap &map)
{
    clear();

    // We need the ID set before trying to restore the steps!
    if (!ProjectConfiguration::fromMap(map))
        return false;

    const QList<BuildStepFactory *> factories = BuildStepFactory::allBuildStepFactories();

    int maxSteps = map.value(QString::fromLatin1(STEPS_COUNT_KEY), 0).toInt();
    for (int i = 0; i < maxSteps; ++i) {
        QVariantMap bsData(map.value(QString::fromLatin1(STEPS_PREFIX) + QString::number(i)).toMap());
        if (bsData.isEmpty()) {
            qWarning() << "No step data found for" << i << "(continuing).";
            continue;
        }
        bool handled = false;
        Core::Id stepId = idFromMap(bsData);
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

QList<BuildStep *> BuildStepList::steps(const std::function<bool (const BuildStep *)> &filter) const
{
    return Utils::filtered(steps(), filter);
}

void BuildStepList::insertStep(int position, BuildStep *step)
{
    m_steps.insert(position, step);
    emit stepInserted(position);
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
    m_steps.swap(position - 1, position);
    emit stepMoved(position, position - 1);
}

BuildStep *BuildStepList::at(int position)
{
    return m_steps.at(position);
}

Target *BuildStepList::target() const
{
    Q_ASSERT(parent());
    auto bc = qobject_cast<BuildConfiguration *>(parent());
    if (bc)
        return bc->target();
    auto dc = qobject_cast<DeployConfiguration *>(parent());
    if (dc)
        return dc->target();
    return nullptr;
}

Project *BuildStepList::project() const
{
    return target()->project();
}
