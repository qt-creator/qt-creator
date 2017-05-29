/****************************************************************************
**
** Copyright (C) 2016 Kläralvdalens Datakonsult AB, a KDAB Group company.
** Contact: Kläralvdalens Datakonsult AB (info@kdab.com)
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

#include "cmakelocatorfilter.h"
#include "cmakebuildstep.h"
#include "cmakeproject.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/buildsteplist.h>

#include <utils/algorithm.h>

using namespace CMakeProjectManager;
using namespace CMakeProjectManager::Internal;
using namespace ProjectExplorer;
using namespace Utils;

CMakeLocatorFilter::CMakeLocatorFilter()
{
    setId("Build CMake target");
    setDisplayName(tr("Build CMake target"));
    setShortcutString(QLatin1String("cm"));
    setPriority(High);

    connect(SessionManager::instance(), &SessionManager::projectAdded,
            this, &CMakeLocatorFilter::projectListUpdated);
    connect(SessionManager::instance(), &SessionManager::projectRemoved,
            this, &CMakeLocatorFilter::projectListUpdated);

    // Initialize the filter
    projectListUpdated();
}

void CMakeLocatorFilter::prepareSearch(const QString &entry)
{
    m_result.clear();
    for (Project *p : SessionManager::projects()) {
        CMakeProject *cmakeProject = qobject_cast<CMakeProject *>(p);
        if (!cmakeProject)
            continue;
        foreach (const QString &title, cmakeProject->buildTargetTitles()) {
            const int index = title.indexOf(entry);
            if (index >= 0) {
                Core::LocatorFilterEntry filterEntry(this, title, cmakeProject->projectFilePath().toString());
                filterEntry.extraInfo = FileUtils::shortNativePath(cmakeProject->projectFilePath());
                filterEntry.highlightInfo = {index, entry.length()};
                m_result.append(filterEntry);
            }
        }
    }
}

QList<Core::LocatorFilterEntry> CMakeLocatorFilter::matchesFor(QFutureInterface<Core::LocatorFilterEntry> &future, const QString &entry)
{
    Q_UNUSED(future)
    Q_UNUSED(entry)
    return m_result;
}

void CMakeLocatorFilter::accept(Core::LocatorFilterEntry selection,
                                QString *newText, int *selectionStart, int *selectionLength) const
{
    Q_UNUSED(newText)
    Q_UNUSED(selectionStart)
    Q_UNUSED(selectionLength)
    // Get the project containing the target selected
    const auto cmakeProject = qobject_cast<CMakeProject *>(
                Utils::findOrDefault(SessionManager::projects(), [selection](Project *p) {
                    return p->projectFilePath().toString() == selection.internalData.toString();
                }));
    if (!cmakeProject || !cmakeProject->activeTarget() || !cmakeProject->activeTarget()->activeBuildConfiguration())
        return;

    // Find the make step
    BuildStepList *buildStepList = cmakeProject->activeTarget()->activeBuildConfiguration()
            ->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    auto buildStep = buildStepList->firstOfType<CMakeBuildStep>();
    if (!buildStep)
        return;

    // Change the make step to build only the given target
    QString oldTarget = buildStep->buildTarget();
    buildStep->clearBuildTargets();
    buildStep->setBuildTarget(selection.displayName);

    // Build
    ProjectExplorerPlugin::buildProject(cmakeProject);
    buildStep->setBuildTarget(oldTarget);
}

void CMakeLocatorFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
}

void CMakeLocatorFilter::projectListUpdated()
{
    // Enable the filter if there's at least one CMake project
    setEnabled(Utils::contains(SessionManager::projects(), [](Project *p) { return qobject_cast<CMakeProject *>(p); }));
}
