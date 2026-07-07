// Copyright (C) 2016 Kläralvdalens Datakonsult AB, a KDAB Group company.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakelocatorfilter.h"

#include "cmakebuildconfiguration.h"
#include "cmakebuildstep.h"
#include "cmakebuildsystem.h"
#include "cmakekitaspect.h"
#include "cmakeoutputparser.h"
#include "cmakeproject.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectmanagertr.h"
#include "cmaketoolmanager.h"
#include "cmakeutils.h"
#include "targethelper.h"
#include "testpresetshelper.h"

#include <autotest/autotestconstants.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/locator/ilocatorfilter.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/outputwindow.h>
#include <coreplugin/progressmanager/processprogress.h>

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/outputformatter.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>
#include <QPointer>

using namespace Core;
using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;

namespace CMakeProjectManager::Internal {

using BuildAcceptor = std::function<void(const BuildSystem *, const QString &, const QString &)>;

static QStringList &cmLocatorHistory()
{
    static QStringList history;
    return history;
}

static QString stripTrailingNewline(QString str)
{
    if (str.endsWith('\n'))
        str.chop(1);
    return str;
}

static void addToCmLocatorHistory(const QString &cmd)
{
    const QString trimmed = cmd.trimmed();
    if (trimmed.isEmpty())
        return;
    cmLocatorHistory().removeAll(trimmed);
    cmLocatorHistory().prepend(trimmed);
    static const int maxHistory = 100;
    while (cmLocatorHistory().size() > maxHistory)
        cmLocatorHistory().removeLast();
}

static CMakeBuildSystem *findStartupCMakeBuildSystem()
{
    const auto cmakeProject = qobject_cast<const CMakeProject *>(ProjectManager::startupProject());
    if (!cmakeProject)
        return nullptr;
    return qobject_cast<CMakeBuildSystem *>(cmakeProject->activeBuildSystem());
}

struct CmInput
{
    QString targetName;
    QStringList extraArgs;
};

static CmInput parseCmInput(const QString &input)
{
    const int doubleDashPos = input.indexOf(" -- ");
    if (doubleDashPos >= 0)
        return {
            input.left(doubleDashPos).trimmed(),
            ProcessArgs::splitArgs(input.mid(doubleDashPos + 4), HostOsInfo::hostOs())};
    return {input.trimmed(), {}};
}

class CMakeLocatorCommand final : public QObject
{
    Q_OBJECT
public:
    static void run(const QString &userArgs) { instance()->onRunRequested(userArgs); }

private:
    void onRunRequested(const QString &userArgs)
    {
        if (m_process) {
            QMessageBox msgBox;
            msgBox.setWindowTitle(Tr::tr("Kill Previous Process?"));
            msgBox.setText(
                Tr::tr(
                    "Previous cmake command is still running.\n"
                    "Do you want to kill it?"));
            auto *killButton = msgBox.addButton(Tr::tr("Kill"), QMessageBox::YesRole);
            msgBox.addButton(Tr::tr("Queue"), QMessageBox::NoRole);
            msgBox.addButton(QMessageBox::Cancel);
            msgBox.setDefaultButton(killButton);
            msgBox.setParent(ICore::dialogParent());
            msgBox.exec();
            auto *clicked = msgBox.clickedButton();
            if (!clicked)
                return;
            if (msgBox.buttonRole(clicked) == QMessageBox::NoRole) {
                m_pendingArgs = userArgs;
                return;
            }
            if (msgBox.buttonRole(clicked) != QMessageBox::YesRole)
                return;
            if (m_process) {
                m_process->deleteLater();
                m_process = nullptr;
            }
        }

        auto bs = findStartupCMakeBuildSystem();
        if (!bs)
            return;

        const auto [targetName, toolArguments] = parseCmInput(userArgs);

        if (!targetName.isEmpty() && !targetName.startsWith('-')) {
            buildTarget(bs, targetName, ProcessArgs::joinArgs(toolArguments, HostOsInfo::hostOs()));
            return;
        }

        const FilePath cmakeExe = CMakeKitAspect::cmakeExecutable(bs->kit());
        if (cmakeExe.isEmpty()) {
            MessageManager::writeDisrupting(Tr::tr("Could not find cmake executable."));
            return;
        }

        const FilePath buildDir = bs->buildConfiguration()->buildDirectory();

        // Use CMakeBuildStep's command building pattern
        Utils::CommandLine cmd{cmakeExe};

        // Resolve --trace-redirect= paths to absolute paths in the build directory
        const QString traceRedirectPrefix = "--trace-redirect=";
        QStringList cmakeArgs = Utils::ProcessArgs::splitArgs(userArgs, HostOsInfo::hostOs());
        for (auto & arg : cmakeArgs) {
            if (arg.startsWith(traceRedirectPrefix)) {
                const QString fileName = arg.mid(traceRedirectPrefix.size());
                arg = traceRedirectPrefix + (buildDir / fileName).path();
            }
        }
        cmd.addArgs(cmakeArgs);

        BuildSystem::startNewBuildSystemOutput(addCMakePrefix(
            Tr::tr("Running cmake %1 in %2.").arg(cmd.toUserOutput(), buildDir.toUserOutput())));

        m_process = new Process(this);
        m_process->setWorkingDirectory(buildDir);
        m_process->setEnvironment(bs->buildConfiguration()->environment());
        m_process->setCommand(cmd);

        m_outputFormatter = std::make_unique<OutputFormatter>();
        CMakeOutputParser *cmakeOutputParser = new CMakeOutputParser;
        cmakeOutputParser->setSourceDirectories({buildDir.parentDir(), buildDir});
        m_outputFormatter->addLineParsers({cmakeOutputParser});
        m_outputFormatter->addSearchDir(buildDir);

        m_progress = new ProcessProgress(m_process);
        m_progress->setDisplayName(Tr::tr("Running cmake"));

        TaskHub::clearTasks(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);

        const auto onStdOut = [](const QString &s) {
            BuildSystem::appendBuildSystemOutput(addCMakePrefix(stripTrailingNewline(s)));
        };

        QPointer<OutputFormatter> outputFormatterPtr = m_outputFormatter.get();
        const auto onStdErr = [outputFormatterPtr](const QString &s) {
            if (outputFormatterPtr)
                outputFormatterPtr->appendMessage(s, StdErrFormat);
            BuildSystem::appendBuildSystemOutput(addCMakePrefix(stripTrailingNewline(s)));
        };

        connect(m_process, &Process::done, this, [this]() {
            m_progress->deleteLater();
            m_progress = nullptr;
            if (m_process->exitCode() != 0) {
                TaskHub::addTask<CMakeTask>(Task::Error, m_process->exitMessage());
            }
            BuildSystem::appendBuildSystemOutput(
                addCMakePrefix(QStringList() << QString() << m_process->exitMessage()).join('\n'));
            if (m_process) {
                m_process->deleteLater();
                m_process = nullptr;
            }
            if (!m_pendingArgs.isEmpty()) {
                const QString pendingArgs = m_pendingArgs;
                m_pendingArgs.clear();
                onRunRequested(pendingArgs);
            }
        });

        m_process->setStdOutLineCallback(onStdOut);
        m_process->setStdErrLineCallback(onStdErr);
        m_process->start();
    }

    static CMakeLocatorCommand *instance()
    {
        static CMakeLocatorCommand s_instance;
        return &s_instance;
    }

    Process *m_process = nullptr;
    ProcessProgress *m_progress = nullptr;
    std::unique_ptr<OutputFormatter> m_outputFormatter;
    QString m_pendingArgs;
};

// CMakeBuildTargetFilter

static LocatorMatcherTasks cmakeMatchers(const BuildAcceptor &acceptor, bool allowCmakeCommand = true)
{
    const auto onSetup = [acceptor, allowCmakeCommand] {
        const LocatorStorage &storage = *LocatorStorage::storage();
        const QString input = storage.input();
        const QRegularExpression regexp
            = ILocatorFilter::createRegExp(input, ILocatorFilter::caseSensitivity(input));
        if (!regexp.isValid())
            return;

        // Check if input looks like a cmake command (starts with -)
        const QString trimmedInput = input.trimmed();
        bool isCmakeCommand = !trimmedInput.isEmpty()
                              && (trimmedInput.startsWith("--") || trimmedInput.startsWith("-"));

        LocatorFilterEntries entries[int(ILocatorFilter::MatchLevel::Count)];
        LocatorFilterEntries historyEntries;

        // When allowCmakeCommand is false, always search for targets regardless of input
        const bool searchTargets = allowCmakeCommand ? !isCmakeCommand : true;
        if (searchTargets) {
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

                const auto [targetName, extraArgs] = parseCmInput(trimmedInput);
                const QRegularExpression targetRegexp = ILocatorFilter::createRegExp(
                    targetName, ILocatorFilter::caseSensitivity(input));

                const QList<CMakeBuildTarget> buildTargets = bs->buildTargets();
                for (const CMakeBuildTarget &target : buildTargets) {
                    if (CMakeBuildSystem::filteredOutTarget(target))
                        continue;
                    const QRegularExpressionMatch match = targetRegexp.match(target.title);
                    if (match.hasMatch()) {
                        const FilePath projectPath = cmakeProject->projectFilePath();
                        const QString displayName = target.title;
                        LocatorFilterEntry entry;
                        entry.displayName = displayName;
                        const QStringList capturedExtraArgs = extraArgs;
                        if (acceptor) {
                            entry.acceptor = [bs, displayName, capturedExtraArgs, acceptor] {
                                acceptor(
                                    bs,
                                    displayName,
                                    ProcessArgs::joinArgs(capturedExtraArgs, HostOsInfo::hostOs()));
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
        }

        if (allowCmakeCommand) {
            // Add cmake command from history as suggestion
            const Qt::CaseSensitivity cs = ILocatorFilter::caseSensitivity(input);
            for (const QString &cmd : std::as_const(cmLocatorHistory())) {
                const int index = cmd.indexOf(input, 0, cs);
                if (index >= 0) {
                    LocatorFilterEntry entry;
                    entry.displayName = cmd;
                    const QString capturedCmd = cmd;
                    entry.acceptor = [capturedCmd] {
                        addToCmLocatorHistory(capturedCmd);
                        CMakeLocatorCommand::run(capturedCmd);
                        return AcceptResult();
                    };
                    entry.highlightInfo = {index, static_cast<int>(input.size())};
                    historyEntries.append(entry);
                }
            }

            // Show current input as a fresh entry when there are no target matches and
            // the input looks like a cmake command. This ensures new/modified commands are
            // always available even when old history entries with the same substring exist.
            if (!trimmedInput.isEmpty() && isCmakeCommand) {
                historyEntries = Utils::filtered(
                    historyEntries, [&trimmedInput](const LocatorFilterEntry &e) {
                        return e.displayName.startsWith(trimmedInput) && e.displayName != trimmedInput;
                    });

                LocatorFilterEntry entry;
                entry.displayName = trimmedInput;
                const QString capturedCmd = trimmedInput;
                entry.acceptor = [capturedCmd] {
                    addToCmLocatorHistory(capturedCmd);
                    CMakeLocatorCommand::run(capturedCmd);
                    return AcceptResult();
                };
                entry.highlightInfo = {0, static_cast<int>(input.size())};
                historyEntries.append(entry);
            }
        }

        storage.reportOutput(
            std::accumulate(std::begin(entries), std::end(entries), LocatorFilterEntries())
            + historyEntries);
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
        setDescription(Tr::tr("Builds a target of any open CMake project or runs cmake commands."));
        setDefaultShortcutString("cm");
        setPriority(High);
        setupFilter(this);
    }

private:
    LocatorMatcherTasks matchers() final { return cmakeMatchers(&buildTarget); }

    void saveState(QJsonObject &object) const final
    {
        if (!cmLocatorHistory().isEmpty())
            object.insert(QLatin1String(historyKey), QJsonArray::fromStringList(cmLocatorHistory()));
    }

    void restoreState(const QJsonObject &object) final
    {
        cmLocatorHistory() = Utils::transform(
            object.value(QLatin1String(historyKey)).toArray().toVariantList(),
            &QVariant::toString);
    }

    static const char historyKey[];
};

const char CMakeBuildTargetFilter::historyKey[] = "cmLocatorHistory";

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
    LocatorMatcherTasks matchers() final { return cmakeMatchers({}, false); }
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

            // Set the current test target as build target, just like the "cm" locator does
            auto getBuildStep = [buildSystem]() {
                const auto buildStepList = buildSystem->buildConfiguration()->buildSteps();
                const auto buildStep = buildStepList->firstOfType<Internal::CMakeBuildStep>();
                return buildStep;
            };
            const auto buildStep = getBuildStep();
            if (buildStep) {
                const QStringList oldTargets = buildStep->buildTargets();
                buildStep->setBuildTargets({testInfo.name});

                testEnv.onTestsRunFinished = [getBuildStep, oldTargets] {
                    const auto buildStep = getBuildStep();
                    if (buildStep)
                        buildStep->setBuildTargets(oldTargets);
                };
            }
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
                    return *preset.displayName;
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

#include "cmakelocatorfilter.moc"
