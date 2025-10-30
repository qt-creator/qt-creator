// Copyright (C) 2016 Kläralvdalens Datakonsult AB, a KDAB Group company.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakelocatorfilter.h"

#include "cmakebuildsystem.h"
#include "cmakeproject.h"
#include "cmakeprojectmanagertr.h"
#include "targethelper.h"
#include "testpresetshelper.h"

#include <autotest/autotestconstants.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/locator/ilocatorfilter.h>
#include <coreplugin/messagemanager.h>

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>

#include <utils/algorithm.h>

using namespace Core;
using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;

namespace CMakeProjectManager::Internal {

using BuildAcceptor = std::function<void(const BuildSystem *, const QString &)>;

// CMakeBuildTargetFilter

static LocatorMatcherTasks cmakeMatchers(const BuildAcceptor &acceptor)
{
    const auto onSetup = [acceptor] {
        const LocatorStorage &storage = *LocatorStorage::storage();
        const QString input = storage.input();
        const QRegularExpression regexp
            = ILocatorFilter::createRegExp(input, ILocatorFilter::caseSensitivity(input));
        if (!regexp.isValid())
            return;
        LocatorFilterEntries entries[int(ILocatorFilter::MatchLevel::Count)];

        QList<Project *> projects = ProjectManager::projects();
        // Make the active project's results at the top
        const qsizetype startupProjectIndex = projects.indexOf(ProjectManager::startupProject());
        if (startupProjectIndex > 0)
            projects.move(startupProjectIndex, 0);

        for (Project *project : projects) {
            const auto cmakeProject = qobject_cast<const CMakeProject *>(project);
            if (!cmakeProject)
                continue;
            const auto bs = qobject_cast<CMakeBuildSystem *>(cmakeProject->activeBuildSystem());
            if (!bs)
                continue;

            const QList<CMakeBuildTarget> buildTargets = bs->buildTargets();
            for (const CMakeBuildTarget &target : buildTargets) {
                if (CMakeBuildSystem::filteredOutTarget(target))
                    continue;
                const QRegularExpressionMatch match = regexp.match(target.title);
                if (match.hasMatch()) {
                    const FilePath projectPath = cmakeProject->projectFilePath();
                    const QString displayName = target.title;
                    LocatorFilterEntry entry;
                    entry.displayName = displayName;
                    if (acceptor) {
                        entry.acceptor = [bs, displayName, acceptor] {
                            acceptor(bs, displayName);
                            return AcceptResult();
                        };
                    }
                    // We want to show real targets (executables, libraries) and also
                    // custom targets defined in the project via add_custom_target.
                    bool targetOfInterest = false;
                    if (!target.backtrace.isEmpty()) {
                        const FilePath path = target.backtrace.last().path;
                        const int line = target.backtrace.last().line;
                        entry.linkForEditor = {path, line};
                        entry.extraInfo = path.shortNativePath();

                        if (target.targetType != UtilityType || path == projectPath
                            || path.isChildOf(projectPath))
                            targetOfInterest = true;
                    } else {
                        entry.extraInfo = projectPath.shortNativePath();
                    }
                    entry.highlightInfo = ILocatorFilter::highlightInfo(match);
                    entry.filePath = cmakeProject->projectFilePath();
                    if (acceptor || targetOfInterest) {
                        if (match.capturedStart() == 0)
                            entries[int(ILocatorFilter::MatchLevel::Best)].append(entry);
                        else if (match.lastCapturedIndex() == 1)
                            entries[int(ILocatorFilter::MatchLevel::Better)].append(entry);
                        else
                            entries[int(ILocatorFilter::MatchLevel::Good)].append(entry);
                    }
                }
            }
        }
        storage.reportOutput(
            std::accumulate(std::begin(entries), std::end(entries), LocatorFilterEntries()));
    };
    return {QSyncTask(onSetup)};
}

static void setupFilter(ILocatorFilter *filter)
{
    const auto projectListUpdated = [filter] {
        filter->setEnabled(Utils::contains(ProjectManager::projects(),
                           [](Project *p) { return qobject_cast<CMakeProject *>(p); }));
    };
    QObject::connect(ProjectManager::instance(), &ProjectManager::projectAdded,
                     filter, projectListUpdated);
    QObject::connect(ProjectManager::instance(), &ProjectManager::projectRemoved,
                     filter, projectListUpdated);
}

class CMakeBuildTargetFilter final : ILocatorFilter
{
public:
    CMakeBuildTargetFilter()
    {
        setId("Build CMake target");
        setDisplayName(Tr::tr("Build CMake Target"));
        setDescription(Tr::tr("Builds a target of any open CMake project."));
        setDefaultShortcutString("cm");
        setPriority(High);
        setupFilter(this);
    }

private:
    LocatorMatcherTasks matchers() final { return cmakeMatchers(&buildTarget); }
};

// OpenCMakeTargetLocatorFilter

class CMakeOpenTargetFilter final : ILocatorFilter
{
public:
    CMakeOpenTargetFilter()
    {
        setId("Open CMake target definition");
        setDisplayName(Tr::tr("Open CMake Target"));
        setDescription(Tr::tr("Locates the definition of a target of any open CMake project."));
        setDefaultShortcutString("cmo");
        setPriority(Medium);
        setupFilter(this);
    }

private:
    LocatorMatcherTasks matchers() final { return cmakeMatchers({}); }
};

class CMakeRunCTestFilter final : ILocatorFilter
{
public:
    CMakeRunCTestFilter()
    {
        setId("Run CTest Test");
        setDisplayName(Tr::tr("Run CTest Test"));
        setDescription(Tr::tr("Runs a CTest test of the current active CMake project."));
        setDefaultShortcutString("ct");
        setPriority(Medium);
        setupFilter(this);
    }

private:
    LocatorMatcherTasks matchers() final { return CTestMatchers(&runCTest); }

    static void runCTest(BuildSystem *buildSystem, const TestCaseInfo &testInfo)
    {
        ActionContainer *testMenu = ActionManager::actionContainer(Autotest::Constants::MENU_ID);
        // If QC is started without AutoTest plugin
        if (!testMenu) {
            Core::MessageManager::writeFlashing(
                Tr::tr("AutoTest plugin needs to be loaded in order to execute tests."));
            return;
        }

        ProjectExplorer::TestCaseEnvironment testEnv;
        QStringList additionalOptions;
        if (testInfo.path.fileName() == "CMakePresets.json") {
            const auto cbs = qobject_cast<CMakeBuildSystem *>(buildSystem);
            auto preset = Utils::findOrDefault(
                cbs->project()->presetsData().testPresets,
                [testInfo](const auto &preset) { return preset.name == testInfo.name; });
            additionalOptions = presetToCTestArgs(preset);

            if (preset.environment)
                testEnv.environment = *preset.environment;
        } else {
            additionalOptions << "--output-on-failure";
        }

        emit buildSystem->testRunRequested(testInfo, additionalOptions, testEnv);
    }

    using TestAcceptor = std::function<void(BuildSystem *, const TestCaseInfo &)>;
    static LocatorMatcherTasks CTestMatchers(const TestAcceptor &acceptor)
    {
        const auto onSetup = [acceptor] {
            const LocatorStorage &storage = *LocatorStorage::storage();
            const QString input = storage.input();
            const QRegularExpression regexp
                = ILocatorFilter::createRegExp(input, ILocatorFilter::caseSensitivity(input));
            if (!regexp.isValid())
                return;
            LocatorFilterEntries entries[int(ILocatorFilter::MatchLevel::Count)];

            const auto cmakeProject = qobject_cast<const CMakeProject *>(ProjectManager::startupProject());
            if (!cmakeProject)
                return;

            const auto bs = qobject_cast<CMakeBuildSystem *>(cmakeProject->activeBuildSystem());
            if (!bs)
                return;

            // First the test presets
            const auto testPresets = cmakeProject->presetsData().testPresets;
            QList<TestCaseInfo> testCasesInfo;
            for (const auto &testPreset : testPresets) {
                TestCaseInfo testInfo;
                testInfo.name = testPreset.name;
                testInfo.path = cmakeProject->projectFilePath().parentDir().pathAppended(
                    "CMakePresets.json");
                testCasesInfo << testInfo;
            }
            auto presetDisplayName = [cmakeProject](const TestCaseInfo &testInfo) -> QString {
                auto preset = Utils::findOrDefault(
                    cmakeProject->presetsData().testPresets,
                    [testInfo](const auto &preset) { return preset.name == testInfo.name; });
                if (preset.displayName)
                    return preset.displayName.value();
                return testInfo.name;
            };

            // Then the tests themselves
            testCasesInfo << bs->testcasesInfo();

            for (const TestCaseInfo &testInfo : testCasesInfo) {
                const QRegularExpressionMatch match = regexp.match(testInfo.name);
                if (match.hasMatch()) {
                    const QString displayName = testInfo.path.fileName() == "CMakePresets.json"
                                                    ? presetDisplayName(testInfo)
                                                    : testInfo.name;
                    LocatorFilterEntry entry;
                    entry.displayName = displayName;
                    if (acceptor) {
                        entry.acceptor = [bs, testInfo, acceptor] {
                            acceptor(bs, testInfo);
                            return AcceptResult();
                        };
                    }
                    entry.extraInfo = testInfo.path.shortNativePath();
                    entry.highlightInfo = ILocatorFilter::highlightInfo(match);
                    entry.filePath = testInfo.path;
                    entry.linkForEditor = {testInfo.path, testInfo.line};

                    if (match.capturedStart() == 0)
                        entries[int(ILocatorFilter::MatchLevel::Best)].append(entry);
                    else if (match.lastCapturedIndex() == 1)
                        entries[int(ILocatorFilter::MatchLevel::Better)].append(entry);
                    else
                        entries[int(ILocatorFilter::MatchLevel::Good)].append(entry);
                }
            }
            storage.reportOutput(
                std::accumulate(std::begin(entries), std::end(entries), LocatorFilterEntries()));
        };
        return {QSyncTask(onSetup)};
    }
};

// Setup

void setupCMakeLocatorFilters()
{
    static CMakeBuildTargetFilter theCMakeBuildTargetFilter;
    static CMakeOpenTargetFilter theCMakeOpenTargetFilter;
    static CMakeRunCTestFilter theCMakeRunCTestFilter;
}

} // namespace CMakeProjectManager::Internal
