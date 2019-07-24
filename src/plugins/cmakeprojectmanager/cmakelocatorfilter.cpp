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
#include "cmakebuildconfiguration.h"
#include "cmakebuildstep.h"
#include "cmakeproject.h"

#include <coreplugin/editormanager/editormanager.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <utils/algorithm.h>

using namespace CMakeProjectManager;
using namespace CMakeProjectManager::Internal;
using namespace ProjectExplorer;
using namespace Utils;

// --------------------------------------------------------------------
// CMakeTargetLocatorFilter:
// --------------------------------------------------------------------

CMakeTargetLocatorFilter::CMakeTargetLocatorFilter()
{
    connect(SessionManager::instance(), &SessionManager::projectAdded,
            this, &CMakeTargetLocatorFilter::projectListUpdated);
    connect(SessionManager::instance(), &SessionManager::projectRemoved,
            this, &CMakeTargetLocatorFilter::projectListUpdated);

    // Initialize the filter
    projectListUpdated();
}

void CMakeTargetLocatorFilter::prepareSearch(const QString &entry)
{
    m_result.clear();
    const QList<Project *> projects = SessionManager::projects();
    for (Project *p : projects) {
        auto cmakeProject = qobject_cast<const CMakeProject *>(p);
        if (!cmakeProject || !cmakeProject->activeTarget())
            continue;
        auto bc = qobject_cast<CMakeBuildConfiguration *>(
            cmakeProject->activeTarget()->activeBuildConfiguration());
        if (!bc)
            continue;

        const QList<CMakeBuildTarget> buildTargets = bc->buildTargets();
        for (const CMakeBuildTarget &target : buildTargets) {
            const int index = target.title.indexOf(entry);
            if (index >= 0) {
                const FilePath path = target.definitionFile.isEmpty()
                                          ? cmakeProject->projectFilePath()
                                          : target.definitionFile;
                QVariantMap extraData;
                extraData.insert("project", cmakeProject->projectFilePath().toString());
                extraData.insert("line", target.definitionLine);
                extraData.insert("file", path.toString());

                Core::LocatorFilterEntry filterEntry(this, target.title, extraData);
                filterEntry.extraInfo = path.shortNativePath();
                filterEntry.highlightInfo = {index, entry.length()};
                filterEntry.fileName = path.toString();

                m_result.append(filterEntry);
            }
        }
    }
}

QList<Core::LocatorFilterEntry> CMakeTargetLocatorFilter::matchesFor(QFutureInterface<Core::LocatorFilterEntry> &future, const QString &entry)
{
    Q_UNUSED(future)
    Q_UNUSED(entry)
    return m_result;
}

void CMakeTargetLocatorFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
}

void CMakeTargetLocatorFilter::projectListUpdated()
{
    // Enable the filter if there's at least one CMake project
    setEnabled(Utils::contains(SessionManager::projects(), [](Project *p) { return qobject_cast<CMakeProject *>(p); }));
}

// --------------------------------------------------------------------
// BuildCMakeTargetLocatorFilter:
// --------------------------------------------------------------------

BuildCMakeTargetLocatorFilter::BuildCMakeTargetLocatorFilter()
{
    setId("Build CMake target");
    setDisplayName(tr("Build CMake target"));
    setShortcutString("cm");
    setPriority(High);
}

void BuildCMakeTargetLocatorFilter::accept(Core::LocatorFilterEntry selection,
                                           QString *newText,
                                           int *selectionStart,
                                           int *selectionLength) const
{
    Q_UNUSED(newText)
    Q_UNUSED(selectionStart)
    Q_UNUSED(selectionLength)

    const QVariantMap extraData = selection.internalData.toMap();
    const FilePath projectPath = FilePath::fromString(extraData.value("project").toString());

    // Get the project containing the target selected
    const auto cmakeProject = qobject_cast<CMakeProject *>(
        Utils::findOrDefault(SessionManager::projects(), [projectPath](Project *p) {
            return p->projectFilePath() == projectPath;
        }));
    if (!cmakeProject || !cmakeProject->activeTarget()
        || !cmakeProject->activeTarget()->activeBuildConfiguration())
        return;

    // Find the make step
    BuildStepList *buildStepList = cmakeProject->activeTarget()->activeBuildConfiguration()->stepList(
        ProjectExplorer::Constants::BUILDSTEPS_BUILD);
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

// --------------------------------------------------------------------
// OpenCMakeTargetLocatorFilter:
// --------------------------------------------------------------------

OpenCMakeTargetLocatorFilter::OpenCMakeTargetLocatorFilter()
{
    setId("Open CMake target definition");
    setDisplayName(tr("Open CMake target"));
    setShortcutString("cmo");
    setPriority(Medium);
}

void OpenCMakeTargetLocatorFilter::accept(Core::LocatorFilterEntry selection,
                                          QString *newText,
                                          int *selectionStart,
                                          int *selectionLength) const
{
    Q_UNUSED(newText)
    Q_UNUSED(selectionStart)
    Q_UNUSED(selectionLength)

    const QVariantMap extraData = selection.internalData.toMap();
    const int line = extraData.value("line").toInt();
    const QString file = extraData.value("file").toString();

    if (line >= 0)
        Core::EditorManager::openEditorAt(file, line);
    else
        Core::EditorManager::openEditor(file);
}
