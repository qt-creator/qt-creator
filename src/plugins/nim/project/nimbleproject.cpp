// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nimbleproject.h"
#include "nimconstants.h"
#include "../nimtr.h"

#include <coreplugin/icontext.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>

#include <utils/algorithm.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

struct NimbleMetadata
{
    QStringList bin;
    QString binDir;
    QString srcDir;
};

const char C_NIMBLEPROJECT_TASKS[] = "Nim.NimbleProject.Tasks";

static QList<QByteArray> linesFromProcessOutput(Process *process)
{
    QList<QByteArray> lines = process->readAllRawStandardOutput().split('\n');
    lines = Utils::transform(lines, [](const QByteArray &line){ return line.trimmed(); });
    Utils::erase(lines, [](const QByteArray &line) { return line.isEmpty(); });
    return lines;
}

static std::vector<NimbleTask> parseTasks(const FilePath &nimblePath, const FilePath &workingDirectory)
{
    Process process;
    process.setCommand({nimblePath, {"tasks"}});
    process.setWorkingDirectory(workingDirectory);
    process.start();
    process.waitForFinished();

    std::vector<NimbleTask> result;

    if (process.exitCode() != 0) {
        TaskHub::addTask(ProjectExplorer::BuildSystemTask(Task::Error, process.cleanedStdOut()));
        return result;
    }

    const QList<QByteArray> &lines = linesFromProcessOutput(&process);

    for (const QByteArray &line : lines) {
        QList<QByteArray> tokens = line.trimmed().split(' ');
        QTC_ASSERT(!tokens.empty(), continue);
        QString taskName = QString::fromUtf8(tokens.takeFirst());
        QString taskDesc = QString::fromUtf8(tokens.join(' '));
        result.push_back({std::move(taskName), std::move(taskDesc)});
    }

    return result;
}

static NimbleMetadata parseMetadata(const FilePath &nimblePath, const FilePath &workingDirectory)
{
    Process process;
    process.setCommand({nimblePath, {"dump"}});
    process.setWorkingDirectory(workingDirectory);
    process.start();
    process.waitForFinished();

    NimbleMetadata result = {};

    if (process.exitCode() != 0) {
        TaskHub::addTask(ProjectExplorer::BuildSystemTask(Task::Error, process.cleanedStdOut()));
        return result;
    }
    const QList<QByteArray> &lines = linesFromProcessOutput(&process);

    for (const QByteArray &line : lines) {
        QList<QByteArray> tokens = line.trimmed().split(':');
        QTC_ASSERT(tokens.size() == 2, continue);
        QString name = QString::fromUtf8(tokens.takeFirst()).trimmed();
        QString value = QString::fromUtf8(tokens.takeFirst()).trimmed();
        QTC_ASSERT(value.size() >= 2, continue);
        QTC_ASSERT(value.front() == QChar('"'), continue);
        QTC_ASSERT(value.back() == QChar('"'), continue);
        value.remove(0, 1);
        value.remove(value.size() - 1, 1);

        if (name == "binDir")
            result.binDir = value;
        else if (name == "srcDir")
            result.srcDir = value;
        else if (name == "bin") {
            QStringList bin = value.split(',');
            bin = Utils::transform(bin, [](const QString &x){ return x.trimmed(); });
            Utils::erase(bin, [](const QString &x) { return x.isEmpty(); });
            result.bin = std::move(bin);
        }
    }

    return result;
}

NimbleBuildSystem::NimbleBuildSystem(BuildConfiguration *bc)
    : BuildSystem(bc), m_projectScanner(bc->project())
{
    m_projectScanner.watchProjectFilePath();

    connect(&m_projectScanner, &NimProjectScanner::fileChanged, this, [this](const FilePath &path) {
        if (path == projectFilePath())
            requestDelayedParse();
    });

    connect(&m_projectScanner, &NimProjectScanner::requestReparse,
            this, &NimbleBuildSystem::requestDelayedParse);

    connect(&m_projectScanner, &NimProjectScanner::finished, this, &NimbleBuildSystem::updateProject);

    connect(&m_projectScanner, &NimProjectScanner::directoryChanged, this, [this] (const FilePath &directory) {
        // Workaround for nimble creating temporary files in project root directory
        // when querying the list of tasks.
        // See https://github.com/nim-lang/nimble/issues/720
        if (directory != projectDirectory())
            requestDelayedParse();
    });

    connect(bc->project(), &ProjectExplorer::Project::settingsLoaded,
            this, &NimbleBuildSystem::loadSettings);
    connect(bc->project(), &ProjectExplorer::Project::aboutToSaveSettings,
            this, &NimbleBuildSystem::saveSettings);
    requestDelayedParse();
}

NimbleBuildSystem::~NimbleBuildSystem()
{
    // Trigger any pending parsingFinished signals before destroying any other build system part:
    m_guard = {};
}

void NimbleBuildSystem::triggerParsing()
{
    // Only allow one parsing run at the same time:
    auto guard = guardParsingRun();
    if (!guard.guardsProject())
        return;
    m_guard = std::move(guard);

    m_projectScanner.startScan();
}

void NimbleBuildSystem::updateProject()
{
    const FilePath projectDir = projectDirectory();
    const FilePath nimble = Nim::nimblePathFromKit(kit());

    const NimbleMetadata metadata = parseMetadata(nimble, projectDir);
    const FilePath binDir = projectDir.pathAppended(metadata.binDir);
    const FilePath srcDir = projectDir.pathAppended("src");

    QList<BuildTargetInfo> targets = Utils::transform(metadata.bin, [&](const QString &bin){
        BuildTargetInfo info = {};
        info.displayName = bin;
        info.targetFilePath = binDir.pathAppended(bin);
        info.projectFilePath = projectFilePath();
        info.workingDirectory = binDir;
        info.buildKey = bin;
        return info;
    });

    setApplicationTargets(std::move(targets));

    std::vector<NimbleTask> tasks = parseTasks(nimble, projectDir);
    if (tasks != m_tasks) {
        m_tasks = std::move(tasks);
        emit tasksChanged();
    }

    // Complete scan
    m_guard.markAsSuccess();
    m_guard = {};

    emitBuildSystemUpdated();
}

std::vector<NimbleTask> NimbleBuildSystem::tasks() const
{
    return m_tasks;
}

void NimbleBuildSystem::saveSettings()
{
    // only handles nimble specific settings - NimProjectScanner handles general settings
    QStringList result;
    for (const NimbleTask &task : m_tasks) {
        result.push_back(task.name);
        result.push_back(task.description);
    }

    project()->setNamedSettings(C_NIMBLEPROJECT_TASKS, result);
}

void NimbleBuildSystem::loadSettings()
{
    // only handles nimble specific settings - NimProjectScanner handles general settings
    QStringList list = project()->namedSettings(C_NIMBLEPROJECT_TASKS).toStringList();

    m_tasks.clear();
    if (list.size() % 2 != 0)
        return;

    for (int i = 0; i < list.size(); i += 2)
        m_tasks.push_back({list[i], list[i + 1]});
}

bool NimbleBuildSystem::supportsAction(Node *context, ProjectAction action, const Node *node) const
{
    if (node->asFileNode()) {
        return action == ProjectAction::Rename
               || action == ProjectAction::RemoveFile;
    }
    if (node->isFolderNodeType() || node->isProjectNodeType()) {
        return action == ProjectAction::AddNewFile
               || action == ProjectAction::RemoveFile
               || action == ProjectAction::AddExistingFile;
    }
    return BuildSystem::supportsAction(context, action, node);
}

bool NimbleBuildSystem::addFiles(Node *, const FilePaths &filePaths, FilePaths *)
{
    return m_projectScanner.addFiles(Utils::transform(filePaths, &FilePath::toUrlishString));
}

RemovedFilesFromProject NimbleBuildSystem::removeFiles(Node *,
                                                       const FilePaths &filePaths,
                                                       FilePaths *)
{
    return m_projectScanner.removeFiles(Utils::transform(filePaths, &FilePath::toUrlishString));
}

bool NimbleBuildSystem::deleteFiles(Node *, const FilePaths &)
{
    return true;
}

bool NimbleBuildSystem::renameFiles(Node *, const FilePairs &filesToRename, FilePaths *notRenamed)
{
    bool success = true;
    for (const auto &[oldFilePath, newFilePath] : filesToRename) {
        if (!m_projectScanner.renameFile(oldFilePath.toUrlishString(), newFilePath.toUrlishString())) {
            success = false;
            if (notRenamed)
                *notRenamed << oldFilePath;
        }
    }
    return success;
}

class NimbleBuildConfiguration : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT

    friend class ProjectExplorer::BuildConfigurationFactory;

    NimbleBuildConfiguration(ProjectExplorer::Target *target, Utils::Id id)
        : BuildConfiguration(target, id)
    {
        setConfigWidgetDisplayName(Tr::tr("General"));
        setConfigWidgetHasFrame(true);
        setBuildDirectorySettingsKey("Nim.NimbleBuildConfiguration.BuildDirectory");
        appendInitialBuildStep(Constants::C_NIMBLEBUILDSTEP_ID);

        setInitializer([this](const BuildInfo &info) {
            setBuildType(info.buildType);
            setBuildDirectory(project()->projectDirectory());
        });
    }

    BuildType buildType() const override { return m_buildType; }

    void fromMap(const Utils::Store &map) override
    {
        m_buildType = static_cast<BuildType>(
            map[Constants::C_NIMBLEBUILDCONFIGURATION_BUILDTYPE].toInt());
        BuildConfiguration::fromMap(map);
    }

    void toMap(Utils::Store &map) const override
    {
        BuildConfiguration::toMap(map);
        map[Constants::C_NIMBLEBUILDCONFIGURATION_BUILDTYPE] = buildType();
    }

private:
    void setBuildType(BuildType buildType)
    {
        if (buildType == m_buildType)
            return;
        m_buildType = buildType;
        emit buildTypeChanged();
    }

    BuildType m_buildType = ProjectExplorer::BuildConfiguration::Unknown;
};

class NimbleBuildConfigurationFactory final : public ProjectExplorer::BuildConfigurationFactory
{
public:
    NimbleBuildConfigurationFactory()
    {
        registerBuildConfiguration<NimbleBuildConfiguration>(Constants::C_NIMBLEBUILDCONFIGURATION_ID);
        setSupportedProjectType(Constants::C_NIMBLEPROJECT_ID);
        setSupportedProjectMimeTypeName(Constants::C_NIMBLE_MIMETYPE);

        setBuildGenerator([](const Kit *, const FilePath &projectPath, bool forSetup) {
            const auto oneBuild = [&](BuildConfiguration::BuildType buildType, const QString &typeName) {
                BuildInfo info;
                info.buildType = buildType;
                info.typeName = typeName;
                if (forSetup) {
                    info.displayName = info.typeName;
                    info.buildDirectory = projectPath.parentDir();
                }
                info.enabledByDefault = buildType == BuildConfiguration::Debug;
                return info;
            };
            return QList<BuildInfo>{
                oneBuild(BuildConfiguration::Debug, Tr::tr("Debug")),
                oneBuild(BuildConfiguration::Release, Tr::tr("Release"))
            };
        });
    }
};

NimbleProject::NimbleProject(const FilePath &fileName)
    : Project(Constants::C_NIMBLE_MIMETYPE, fileName)
{
    setId(Constants::C_NIMBLEPROJECT_ID);
    setDisplayName(fileName.completeBaseName());
    // ensure debugging is enabled (Nim plugin translates nim code to C code)
    setProjectLanguages(Core::Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));
    setBuildSystemCreator<NimbleBuildSystem>("nimble");
}

void NimbleProject::toMap(Store &map) const
{
    Project::toMap(map);
    map[Constants::C_NIMPROJECT_EXCLUDEDFILES] = m_excludedFiles;
}

Project::RestoreResult NimbleProject::fromMap(const Store &map, QString *errorMessage)
{
    auto result = Project::fromMap(map, errorMessage);
    m_excludedFiles = map.value(Constants::C_NIMPROJECT_EXCLUDEDFILES).toStringList();
    return result;
}

QStringList NimbleProject::excludedFiles() const
{
    return m_excludedFiles;
}

void NimbleProject::setExcludedFiles(const QStringList &excludedFiles)
{
    m_excludedFiles = excludedFiles;
}

// Setup

void setupNimbleProject()
{
    static const NimbleBuildConfigurationFactory nimbleBuildConfigFactory;
    ProjectManager::registerProjectType<NimbleProject>(Constants::C_NIMBLE_MIMETYPE);
}

} // Nim

#include <nimbleproject.moc>
