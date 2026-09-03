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

using namespace Core;
using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;

namespace CMakeProjectManager::Internal {

using BuildAcceptor = std::function<void(const BuildSystem *, const QString &, const QString &)>;

static QString stripTrailingNewline(QString str)
{
    if (str.endsWith('\n'))
        str.chop(1);
    return str;
}

static CMakeBuildSystem *findStartupCMakeBuildSystem()
{
    const auto cmakeProject = qobject_cast<const CMakeProject *>(ProjectManager::startupProject());
    if (!cmakeProject)
        return nullptr;
    return qobject_cast<CMakeBuildSystem *>(cmakeProject->activeBuildSystem());
}

// The most recently run commands of one locator filter, persisted across sessions.
class CommandHistory
{
public:
    explicit CommandHistory(const char *settingsKey)
        : m_settingsKey(settingsKey)
    {}

    const QStringList &commands() const { return m_commands; }

    void add(const QString &command)
    {
        const QString trimmed = command.trimmed();
        if (trimmed.isEmpty())
            return;
        m_commands.removeAll(trimmed);
        m_commands.prepend(trimmed);
        while (m_commands.size() > maxCommands)
            m_commands.removeLast();
    }

    void save(QJsonObject &object) const
    {
        if (!m_commands.isEmpty())
            object.insert(QLatin1String(m_settingsKey), QJsonArray::fromStringList(m_commands));
    }

    void restore(const QJsonObject &object)
    {
        m_commands = Utils::transform(
            object.value(QLatin1String(m_settingsKey)).toArray().toVariantList(),
            &QVariant::toString);
    }

private:
    static constexpr int maxCommands = 100;

    const char *m_settingsKey;
    QStringList m_commands;
};

static CommandHistory &cmakeCommandHistory()
{
    static CommandHistory history("cmLocatorHistory");
    return history;
}

// Input starting with a dash is a command for the filter's tool, not a target or test name.
static bool isCommandInput(const QString &trimmedInput)
{
    return trimmedInput.startsWith('-');
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

// Runs one command line of a locator filter's tool in the startup project's build directory.
// Only one command runs at a time, a second one either replaces or follows the running one.
class LocatorCommandRunner : public QObject
{
    Q_OBJECT

public:
    void runCommand(const QString &userArgs)
    {
        if (m_process) {
            switch (askAboutRunningCommand()) {
            case Answer::Cancel:
                return;
            case Answer::Queue:
                m_pendingArgs = userArgs;
                return;
            case Answer::Kill:
                stopCommand();
                break;
            }
        }

        CMakeBuildSystem *bs = findStartupCMakeBuildSystem();
        if (!bs)
            return;

        const FilePath executable = toolExecutable(bs);
        if (executable.isEmpty()) {
            MessageManager::writeDisrupting(
                Tr::tr("Could not find %1 executable.").arg(toolName()));
            return;
        }

        const FilePath buildDir = bs->buildConfiguration()->buildDirectory();

        CommandLine cmd{executable};
        cmd.addArgs(arguments(userArgs, buildDir));

        BuildSystem::startNewBuildSystemOutput(
            addCMakePrefix(Tr::tr("Running %1 %2 in %3.")
                               .arg(toolName(), cmd.toUserOutput(), buildDir.toUserOutput())));

        m_process = new Process(this);
        m_process->setWorkingDirectory(buildDir);
        m_process->setEnvironment(bs->buildConfiguration()->environment());
        m_process->setCommand(cmd);

        prepareRun(buildDir);

        m_process->setStdOutLineCallback([](const QString &line) { appendOutput(line); });
        m_process->setStdErrLineCallback([this](const QString &line) { handleStdErr(line); });

        // Owned by the process, and reports itself finished once the process is done.
        const auto progress = new ProcessProgress(m_process);
        progress->setDisplayName(Tr::tr("Running %1").arg(toolName()));

        Process *process = m_process;
        connect(process, &Process::done, this, [this, process] {
            reportIssues(process);
            BuildSystem::appendBuildSystemOutput(
                addCMakePrefix(QStringList() << QString() << process->exitMessage()).join('\n'));
            m_process = nullptr;
            process->deleteLater();
            if (!m_pendingArgs.isEmpty()) {
                const QString pendingArgs = m_pendingArgs;
                m_pendingArgs.clear();
                runCommand(pendingArgs);
            }
        });

        m_process->start();
    }

protected:
    virtual QString toolName() const = 0;
    virtual FilePath toolExecutable(CMakeBuildSystem *buildSystem) const = 0;

    virtual QStringList arguments(const QString &userArgs, const FilePath &buildDir) const
    {
        Q_UNUSED(buildDir)
        return ProcessArgs::splitArgs(userArgs, HostOsInfo::hostOs());
    }

    virtual void prepareRun(const FilePath &buildDir) { Q_UNUSED(buildDir) }
    virtual void handleStdErr(const QString &line) { appendOutput(line); }
    virtual void reportIssues(Process *process) { Q_UNUSED(process) }

    static void appendOutput(const QString &line)
    {
        BuildSystem::appendBuildSystemOutput(addCMakePrefix(stripTrailingNewline(line)));
    }

private:
    enum class Answer { Kill, Queue, Cancel };

    Answer askAboutRunningCommand() const
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle(Tr::tr("Kill Previous Process?"));
        msgBox.setText(Tr::tr("Previous %1 command is still running.\n"
                              "Do you want to kill it?").arg(toolName()));
        auto *killButton = msgBox.addButton(Tr::tr("Kill"), QMessageBox::YesRole);
        msgBox.addButton(Tr::tr("Queue"), QMessageBox::NoRole);
        msgBox.addButton(QMessageBox::Cancel);
        msgBox.setDefaultButton(killButton);
        msgBox.setParent(ICore::dialogParent());
        msgBox.exec();

        auto *clicked = msgBox.clickedButton();
        if (!clicked)
            return Answer::Cancel;
        if (msgBox.buttonRole(clicked) == QMessageBox::NoRole)
            return Answer::Queue;
        if (msgBox.buttonRole(clicked) == QMessageBox::YesRole)
            return Answer::Kill;
        return Answer::Cancel;
    }

    void stopCommand()
    {
        // Deleting alone would neither end the process nor keep its done handler from acting
        // on the command started next, as the deletion only happens once we are back in the
        // event loop.
        m_process->disconnect(this);
        m_process->stop();
        m_process->deleteLater();
        m_process = nullptr;
    }

    Process *m_process = nullptr;
    QString m_pendingArgs;
};

class CMakeCommandRunner final : public LocatorCommandRunner
{
public:
    static void run(const QString &userArgs)
    {
        static CMakeCommandRunner theRunner;
        theRunner.runCommand(userArgs);
    }

private:
    QString toolName() const final { return "cmake"; }

    FilePath toolExecutable(CMakeBuildSystem *buildSystem) const final
    {
        return CMakeKitAspect::cmakeExecutable(buildSystem->kit());
    }

    QStringList arguments(const QString &userArgs, const FilePath &buildDir) const final
    {
        // Resolve --trace-redirect= paths to absolute paths in the build directory
        const QString traceRedirectPrefix = "--trace-redirect=";
        QStringList args = ProcessArgs::splitArgs(userArgs, HostOsInfo::hostOs());
        for (QString &arg : args) {
            if (arg.startsWith(traceRedirectPrefix)) {
                const QString fileName = arg.mid(traceRedirectPrefix.size());
                arg = traceRedirectPrefix + (buildDir / fileName).path();
            }
        }
        return args;
    }

    void prepareRun(const FilePath &buildDir) final
    {
        m_outputFormatter = std::make_unique<OutputFormatter>();
        const auto cmakeOutputParser = new CMakeOutputParser;
        cmakeOutputParser->setSourceDirectories({buildDir.parentDir(), buildDir});
        m_outputFormatter->addLineParsers({cmakeOutputParser});
        m_outputFormatter->addSearchDir(buildDir);

        TaskHub::clearTasks(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);
    }

    void handleStdErr(const QString &line) final
    {
        m_outputFormatter->appendMessage(line, StdErrFormat);
        LocatorCommandRunner::handleStdErr(line);
    }

    void reportIssues(Process *process) final
    {
        if (process->exitCode() != 0)
            TaskHub::addTask<CMakeTask>(Task::Error, process->exitMessage());
    }

    std::unique_ptr<OutputFormatter> m_outputFormatter;
};

using CommandAcceptor = void (*)(const QString &);

// Offers the commands from the history that match the input, plus the input itself when it is
// a command. The latter keeps new and modified commands available next to the older ones.
static LocatorFilterEntries commandEntries(const QString &input, CommandHistory *history,
                                           const CommandAcceptor &acceptor)
{
    const QString trimmedInput = input.trimmed();
    const bool isCommand = isCommandInput(trimmedInput);
    const int highlightLength = static_cast<int>(trimmedInput.size());

    const auto entryFor = [history, acceptor, highlightLength](const QString &command,
                                                               int highlightStart) {
        LocatorFilterEntry entry;
        entry.uniquifier = entry.displayName = command;
        entry.acceptor = [history, acceptor, command] {
            history->add(command);
            acceptor(command);
            return AcceptResult();
        };
        entry.highlightInfo = {highlightStart, highlightLength};
        return entry;
    };

    LocatorFilterEntries entries;
    const Qt::CaseSensitivity cs = ILocatorFilter::caseSensitivity(input);
    for (const QString &command : history->commands()) {
        if (isCommand) {
            if (command.startsWith(trimmedInput) && command != trimmedInput)
                entries.append(entryFor(command, 0));
            continue;
        }
        const int index = command.indexOf(trimmedInput, 0, cs);
        if (index >= 0)
            entries.append(entryFor(command, index));
    }

    if (isCommand)
        entries.append(entryFor(trimmedInput, 0));

    return entries;
}

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

        const QString trimmedInput = input.trimmed();

        LocatorFilterEntries entries[int(ILocatorFilter::MatchLevel::Count)];

        // When allowCmakeCommand is false, always search for targets regardless of input
        const bool searchTargets = allowCmakeCommand ? !isCommandInput(trimmedInput) : true;
        if (searchTargets) {
            QList<Project *> projects = ProjectManager::projects();
            // Make the active project's results at the top
            const qsizetype startupProjectIndex = projects.indexOf(ProjectManager::startupProject());
            if (startupProjectIndex > 0)
                projects.move(startupProjectIndex, 0);

            const bool showProjectName = Utils::count(projects, [](const Project *p) {
                return qobject_cast<const CMakeProject *>(p) != nullptr;
            }) > 1;

            for (Project *project : std::as_const(projects)) {
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
                        entry.uniquifier = entry.displayName = displayName;
                        if (showProjectName)
                            entry.displayExtra = cmakeProject->displayName();
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
                                || path.isChildOf(projectPath.parentDir()))
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

        LocatorFilterEntries commands;
        if (allowCmakeCommand)
            commands = commandEntries(input, &cmakeCommandHistory(), &CMakeCommandRunner::run);

        storage.reportOutput(
            std::accumulate(std::begin(entries), std::end(entries), LocatorFilterEntries())
            + commands);
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

    void saveState(QJsonObject &object) const final { cmakeCommandHistory().save(object); }
    void restoreState(const QJsonObject &object) final { cmakeCommandHistory().restore(object); }
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

            for (const TestCaseInfo &testInfo : std::as_const(testCasesInfo)) {
                const QRegularExpressionMatch match = regexp.match(testInfo.name);
                if (match.hasMatch()) {
                    const QString displayName = testInfo.path.fileName() == "CMakePresets.json"
                                                    ? presetDisplayName(testInfo)
                                                    : testInfo.name;
                    LocatorFilterEntry entry;
                    entry.uniquifier = entry.displayName = displayName;
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
