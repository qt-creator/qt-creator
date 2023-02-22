// Copyright (C) 2016 Kläralvdalens Datakonsult AB, a KDAB Group company.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakelocatorfilter.h"

#include "cmakebuildstep.h"
#include "cmakebuildsystem.h"
#include "cmakeproject.h"
#include "cmakeprojectmanagertr.h"

#include <coreplugin/editormanager/editormanager.h>

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>

#include <utils/algorithm.h>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager::Internal {

// --------------------------------------------------------------------
// CMakeTargetLocatorFilter:
// --------------------------------------------------------------------

CMakeTargetLocatorFilter::CMakeTargetLocatorFilter()
{
    connect(ProjectManager::instance(), &ProjectManager::projectAdded,
            this, &CMakeTargetLocatorFilter::projectListUpdated);
    connect(ProjectManager::instance(), &ProjectManager::projectRemoved,
            this, &CMakeTargetLocatorFilter::projectListUpdated);

    // Initialize the filter
    projectListUpdated();
}

void CMakeTargetLocatorFilter::prepareSearch(const QString &entry)
{
    m_result.clear();
    const QList<Project *> projects = ProjectManager::projects();
    for (Project *p : projects) {
        auto cmakeProject = qobject_cast<const CMakeProject *>(p);
        if (!cmakeProject || !cmakeProject->activeTarget())
            continue;
        auto bs = qobject_cast<CMakeBuildSystem *>(cmakeProject->activeTarget()->buildSystem());
        if (!bs)
            continue;

        const QList<CMakeBuildTarget> buildTargets = bs->buildTargets();
        for (const CMakeBuildTarget &target : buildTargets) {
            if (CMakeBuildSystem::filteredOutTarget(target))
                continue;
            const int index = target.title.indexOf(entry, 0, Qt::CaseInsensitive);
            if (index >= 0) {
                const FilePath path = target.backtrace.isEmpty() ? cmakeProject->projectFilePath()
                                                                 : target.backtrace.last().path;
                const int line = target.backtrace.isEmpty() ? 0 : target.backtrace.last().line;

                LocatorFilterEntry filterEntry(this, target.title);
                filterEntry.linkForEditor = {path, line};
                filterEntry.extraInfo = path.shortNativePath();
                filterEntry.highlightInfo = {index, int(entry.length())};
                filterEntry.filePath = cmakeProject->projectFilePath();

                m_result.append(filterEntry);
            }
        }
    }
}

QList<LocatorFilterEntry> CMakeTargetLocatorFilter::matchesFor(
    QFutureInterface<LocatorFilterEntry> &future, const QString &entry)
{
    Q_UNUSED(future)
    Q_UNUSED(entry)
    return m_result;
}

void CMakeTargetLocatorFilter::projectListUpdated()
{
    // Enable the filter if there's at least one CMake project
    setEnabled(Utils::contains(ProjectManager::projects(),
                               [](Project *p) { return qobject_cast<CMakeProject *>(p); }));
}

// --------------------------------------------------------------------
// BuildCMakeTargetLocatorFilter:
// --------------------------------------------------------------------

BuildCMakeTargetLocatorFilter::BuildCMakeTargetLocatorFilter()
{
    setId("Build CMake target");
    setDisplayName(Tr::tr("Build CMake target"));
    setDescription(Tr::tr("Builds a target of any open CMake project."));
    setDefaultShortcutString("cm");
    setPriority(High);
}

void BuildCMakeTargetLocatorFilter::accept(const LocatorFilterEntry &selection, QString *newText,
                                           int *selectionStart, int *selectionLength) const
{
    Q_UNUSED(newText)
    Q_UNUSED(selectionStart)
    Q_UNUSED(selectionLength)

    const FilePath projectPath = selection.filePath;

    // Get the project containing the target selected
    const auto cmakeProject = qobject_cast<CMakeProject *>(
        Utils::findOrDefault(ProjectManager::projects(), [projectPath](Project *p) {
            return p->projectFilePath() == projectPath;
        }));
    if (!cmakeProject || !cmakeProject->activeTarget()
        || !cmakeProject->activeTarget()->activeBuildConfiguration())
        return;

    // Find the make step
    BuildStepList *buildStepList =
            cmakeProject->activeTarget()->activeBuildConfiguration()->buildSteps();
    auto buildStep = buildStepList->firstOfType<CMakeBuildStep>();
    if (!buildStep)
        return;

    // Change the make step to build only the given target
    QStringList oldTargets = buildStep->buildTargets();
    buildStep->setBuildTargets({selection.displayName});

    // Build
    BuildManager::buildProjectWithDependencies(cmakeProject);
    buildStep->setBuildTargets(oldTargets);
}

// --------------------------------------------------------------------
// OpenCMakeTargetLocatorFilter:
// --------------------------------------------------------------------

OpenCMakeTargetLocatorFilter::OpenCMakeTargetLocatorFilter()
{
    setId("Open CMake target definition");
    setDisplayName(Tr::tr("Open CMake target"));
    setDescription(Tr::tr("Jumps to the definition of a target of any open CMake project."));
    setDefaultShortcutString("cmo");
    setPriority(Medium);
}

void OpenCMakeTargetLocatorFilter::accept(const LocatorFilterEntry &selection,
                                          QString *newText,
                                          int *selectionStart,
                                          int *selectionLength) const
{
    Q_UNUSED(newText)
    Q_UNUSED(selectionStart)
    Q_UNUSED(selectionLength)
    EditorManager::openEditor(selection);
}

} // CMakeProjectManager::Internal
