// Copyright (C) 2016 Kläralvdalens Datakonsult AB, a KDAB Group company.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakelocatorfilter.h"

#include "cmakebuildstep.h"
#include "cmakebuildsystem.h"
#include "cmakeproject.h"
#include "cmakeprojectmanagertr.h"

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

static LocatorMatcherTasks cmakeMatchers(const CMakeTargetLocatorFilter::BuildAcceptor &acceptor)
{
    using namespace Tasking;

    TreeStorage<LocatorStorage> storage;

    const auto onSetup = [storage, acceptor] {
        const QString input = storage->input();
        const QList<Project *> projects = ProjectManager::projects();
        LocatorFilterEntries entries;
        for (Project *project : projects) {
            const auto cmakeProject = qobject_cast<const CMakeProject *>(project);
            if (!cmakeProject || !cmakeProject->activeTarget())
                continue;
            const auto bs = qobject_cast<CMakeBuildSystem *>(
                cmakeProject->activeTarget()->buildSystem());
            if (!bs)
                continue;

            const QList<CMakeBuildTarget> buildTargets = bs->buildTargets();
            for (const CMakeBuildTarget &target : buildTargets) {
                if (CMakeBuildSystem::filteredOutTarget(target))
                    continue;
                const int index = target.title.indexOf(input, 0, Qt::CaseInsensitive);
                if (index >= 0) {
                    const FilePath path = target.backtrace.isEmpty()
                                              ? cmakeProject->projectFilePath()
                                              : target.backtrace.last().path;
                    const int line = target.backtrace.isEmpty() ? 0 : target.backtrace.last().line;
                    const FilePath projectPath = cmakeProject->projectFilePath();
                    const QString displayName = target.title;
                    LocatorFilterEntry entry;
                    entry.displayName = displayName;
                    if (acceptor) {
                        entry.acceptor = [projectPath, displayName, acceptor] {
                            acceptor(projectPath, displayName);
                            return AcceptResult();
                        };
                    }
                    entry.linkForEditor = {path, line};
                    entry.extraInfo = path.shortNativePath();
                    entry.highlightInfo = {index, int(input.length())};
                    entry.filePath = cmakeProject->projectFilePath();
                    entries.append(entry);
                }
            }
        }
        storage->reportOutput(entries);
    };
    return {{Sync(onSetup), storage}};
}

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
                const FilePath projectPath = cmakeProject->projectFilePath();
                const QString displayName = target.title;
                LocatorFilterEntry filterEntry;
                filterEntry.displayName = displayName;
                if (m_acceptor) {
                    filterEntry.acceptor = [projectPath, displayName, acceptor = m_acceptor] {
                        acceptor(projectPath, displayName);
                        return AcceptResult();
                    };
                }
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

static void buildAcceptor(const Utils::FilePath &projectPath, const QString &displayName)
{
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
    buildStep->setBuildTargets({displayName});

    // Build
    BuildManager::buildProjectWithDependencies(cmakeProject);
    buildStep->setBuildTargets(oldTargets);
}

BuildCMakeTargetLocatorFilter::BuildCMakeTargetLocatorFilter()
{
    setId("Build CMake target");
    setDisplayName(Tr::tr("Build CMake Target"));
    setDescription(Tr::tr("Builds a target of any open CMake project."));
    setDefaultShortcutString("cm");
    setPriority(High);
    setBuildAcceptor(&buildAcceptor);
}

Core::LocatorMatcherTasks BuildCMakeTargetLocatorFilter::matchers()
{
    return cmakeMatchers(&buildAcceptor);
}

// --------------------------------------------------------------------
// OpenCMakeTargetLocatorFilter:
// --------------------------------------------------------------------

OpenCMakeTargetLocatorFilter::OpenCMakeTargetLocatorFilter()
{
    setId("Open CMake target definition");
    setDisplayName(Tr::tr("Open CMake Target"));
    setDescription(Tr::tr("Locates the definition of a target of any open CMake project."));
    setDefaultShortcutString("cmo");
    setPriority(Medium);
}

Core::LocatorMatcherTasks OpenCMakeTargetLocatorFilter::matchers()
{
    return cmakeMatchers({});
}

} // CMakeProjectManager::Internal
