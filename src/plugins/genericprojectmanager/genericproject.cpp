// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "genericproject.h"

#include "genericprojectconstants.h"
#include "genericprojectmanagertr.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>

#include <projectexplorer/abi.h>
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/customexecutablerunconfiguration.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/environmentkitaspect.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorertr.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/projectupdater.h>
#include <projectexplorer/selectablefilesmodel.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtcppkitinfo.h>
#include <qtsupport/qtkitaspect.h>

#include <utils/algorithm.h>
#include <utils/filesystemwatcher.h>
#include <utils/fileutils.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QMetaObject>
#include <QPair>
#include <QSet>
#include <QStringList>

#include <set>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace GenericProjectManager::Internal {

enum RefreshOptions {
    Files         = 0x01,
    Configuration = 0x02,
    Everything    = Files | Configuration
};

//
// GenericBuildSystem
//

class GenericBuildSystem final : public BuildSystem
{
public:
    explicit GenericBuildSystem(BuildConfiguration *bc);
    ~GenericBuildSystem();

    void triggerParsing() final;

    bool supportsAction(Node *, ProjectAction action, const Node *) const final
    {
        return action == AddNewFile
                || action == AddExistingFile
                || action == AddExistingDirectory
                || action == RemoveFile
                || action == Rename;
    }

    RemovedFilesFromProject removeFiles(Node *, const FilePaths &filePaths, FilePaths *) final;
    bool renameFiles(
        Node *,
        const Utils::FilePairs &filesToRename,
        Utils::FilePaths *notRenamed) final;
    bool addFiles(Node *, const FilePaths &filePaths, FilePaths *) final;

    FilePath filesFilePath() const { return ::FilePath::fromString(m_filesFileName); }

    void refresh(RefreshOptions options);

    bool saveRawFileList(const QStringList &rawFileList);
    bool saveRawList(const QStringList &rawList, const QString &fileName);
    void parse(RefreshOptions options);

    using SourceFile = QPair<FilePath, QStringList>;
    using SourceFiles = QList<SourceFile>;
    SourceFiles processEntries(const QStringList &paths,
                               QHash<QString, QString> *map = nullptr) const;

    FilePath findCommonSourceRoot();
    void refreshCppCodeModel();
    void updateDeploymentData();

    bool setFiles(const QStringList &filePaths);
    void removeFiles(const FilePaths &filesToRemove);

private:
    QString m_filesFileName;
    QString m_includesFileName;
    QString m_configFileName;
    QString m_cxxflagsFileName;
    QString m_cflagsFileName;
    QStringList m_rawFileList;
    SourceFiles m_files;
    QHash<QString, QString> m_rawListEntries;
    QStringList m_rawProjectIncludePaths;
    HeaderPaths m_projectIncludePaths;
    QStringList m_cxxflags;
    QStringList m_cflags;

    ProjectUpdater *m_cppCodeModelUpdater = nullptr;

    FileSystemWatcher m_deployFileWatcher;
};

//
// GenericBuildConfiguration
//

class GenericBuildConfiguration final : public BuildConfiguration
{
public:
    GenericBuildConfiguration(Target *target, Id id)
        : BuildConfiguration(target, id)
    {
        setConfigWidgetDisplayName(GenericProjectManager::Tr::tr("Generic Manager"));
        setBuildDirectoryHistoryCompleter("Generic.BuildDir.History");

        setInitializer([this](const BuildInfo &) {
            buildSteps()->appendStep(Constants::GENERIC_MS_ID);
            cleanSteps()->appendStep(Constants::GENERIC_MS_ID);
            updateCacheAndEmitEnvironmentChanged();
        });

        updateCacheAndEmitEnvironmentChanged();
    }

private:
    void addToEnvironment(Environment &env) const final
    {
        QtSupport::QtKitAspect::addHostBinariesToPath(kit(), env);
    }
};

class GenericBuildConfigurationFactory final : public BuildConfigurationFactory
{
public:
    GenericBuildConfigurationFactory()
    {
        registerBuildConfiguration<GenericBuildConfiguration>
            ("GenericProjectManager.GenericBuildConfiguration");

        setSupportedProjectType(Constants::GENERICPROJECT_ID);
        setSupportedProjectMimeTypeName(Constants::GENERICMIMETYPE);

        setBuildGenerator([](const Kit *, const FilePath &projectPath, bool forSetup) {
            BuildInfo info;
            info.typeName = ProjectExplorer::Tr::tr("Build");
            info.buildDirectory = forSetup ? projectPath.absolutePath() : projectPath;

            if (forSetup)  {
                //: The name of the build configuration created by default for a generic project.
                info.displayName = ProjectExplorer::Tr::tr("Default");
            }

            return QList<BuildInfo>{info};
        });
    }
};


//
// GenericProject
//

static bool writeFile(const QString &filePath, const QString &contents)
{
    FileSaver saver(FilePath::fromString(filePath),
                    QIODevice::Text | QIODevice::WriteOnly);
    return saver.write(contents.toUtf8()) && saver.finalize();
}

class GenericProject final : public Project
{
    Q_OBJECT

public:
    explicit GenericProject(const FilePath &filePath)
        : Project(Constants::GENERICMIMETYPE, filePath)
    {
        setId(Constants::GENERICPROJECT_ID);
        setProjectLanguages(Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));
        setDisplayName(filePath.completeBaseName());
        setBuildSystemCreator<GenericBuildSystem>("generic");
    }

    void editFilesTriggered();
    void removeFilesTriggered(const FilePaths &filesToRemove);

private:
    RestoreResult fromMap(const Store &map, QString *errorMessage) final;
    DeploymentKnowledge deploymentKnowledge() const final;
    void configureAsExampleProject(Kit *kit) final;
};

GenericBuildSystem::GenericBuildSystem(BuildConfiguration *bc)
    : BuildSystem(bc)
{
    m_cppCodeModelUpdater = ProjectUpdaterFactory::createCppProjectUpdater();

    connect(bc->project(), &Project::projectFileIsDirty, this, [this](const FilePath &p) {
        if (p.endsWith(".files"))
            refresh(Files);
        else if (p.endsWith(".includes") || p.endsWith(".config") || p.endsWith(".cxxflags")
                 || p.endsWith(".cflags"))
            refresh(Configuration);
        else
            refresh(Everything);
    });

    const QFileInfo fileInfo = projectFilePath().toFileInfo();
    const QDir dir = fileInfo.dir();

    const QString projectName = fileInfo.completeBaseName();

    m_filesFileName    = QFileInfo(dir, projectName + ".files").absoluteFilePath();
    m_includesFileName = QFileInfo(dir, projectName + ".includes").absoluteFilePath();
    m_configFileName = QFileInfo(dir, projectName + ".config").absoluteFilePath();

    const QFileInfo cxxflagsFileInfo(dir, projectName + ".cxxflags");
    m_cxxflagsFileName = cxxflagsFileInfo.absoluteFilePath();
    if (!cxxflagsFileInfo.exists()) {
        QTC_CHECK(writeFile(m_cxxflagsFileName, Constants::GENERICPROJECT_CXXFLAGS_FILE_TEMPLATE));
    }

    const QFileInfo cflagsFileInfo(dir, projectName + ".cflags");
    m_cflagsFileName = cflagsFileInfo.absoluteFilePath();
    if (!cflagsFileInfo.exists()) {
        QTC_CHECK(writeFile(m_cflagsFileName, Constants::GENERICPROJECT_CFLAGS_FILE_TEMPLATE));
    }

    project()->setExtraProjectFiles({FilePath::fromString(m_filesFileName),
                                     FilePath::fromString(m_includesFileName),
                                     FilePath::fromString(m_configFileName),
                                     FilePath::fromString(m_cxxflagsFileName),
                                     FilePath::fromString(m_cflagsFileName)});

    connect(&m_deployFileWatcher, &FileSystemWatcher::fileChanged,
            this, &GenericBuildSystem::updateDeploymentData);
    connect(project(), &Project::activeBuildConfigurationChanged, this, [this] {
        refresh(Everything);
    });
}

GenericBuildSystem::~GenericBuildSystem()
{
    delete m_cppCodeModelUpdater;
}

void GenericBuildSystem::triggerParsing()
{
    refresh(Everything);
}

static QStringList readLines(const QString &absoluteFileName)
{
    QStringList lines;

    QFile file(absoluteFileName);
    if (file.open(QFile::ReadOnly)) {
        QTextStream stream(&file);

        for (;;) {
            const QString line = stream.readLine();
            if (line.isNull())
                break;

            lines.append(line);
        }
    }

    return lines;
}

bool GenericBuildSystem::saveRawFileList(const QStringList &rawFileList)
{
    bool result = saveRawList(rawFileList, m_filesFileName);
    refresh(Files);
    return result;
}

bool GenericBuildSystem::saveRawList(const QStringList &rawList, const QString &fileName)
{
    const FilePath filePath = FilePath::fromString(fileName);
    FileChangeBlocker changeGuard(filePath);
    // Make sure we can open the file for writing
    FileSaver saver(filePath, QIODevice::Text);
    if (!saver.hasError()) {
        QTextStream stream(saver.file());
        for (const QString &filePath : rawList)
            stream << filePath << '\n';
        saver.setResult(&stream);
    }
    const Result<> result = saver.finalize();
    if (!result)
        FileUtils::showError(result.error());
    return result.has_value();
}

static void insertSorted(QStringList *list, const QString &value)
{
    const auto it = std::lower_bound(list->begin(), list->end(), value);
    if (it == list->end())
        list->append(value);
    else if (*it > value)
        list->insert(it, value);
}

bool GenericBuildSystem::addFiles(Node *, const FilePaths &filePaths_, FilePaths *)
{
    const QStringList filePaths = Utils::transform(filePaths_, &FilePath::toUrlishString);
    const QDir baseDir(projectDirectory().toUrlishString());
    QStringList newList = m_rawFileList;
    if (filePaths.size() > m_rawFileList.size()) {
        newList += transform(filePaths, [&baseDir](const QString &p) {
            return baseDir.relativeFilePath(p);
        });
        sort(newList);
        newList.erase(std::unique(newList.begin(), newList.end()), newList.end());
    } else {
        for (const QString &filePath : filePaths)
            insertSorted(&newList, baseDir.relativeFilePath(filePath));
    }

    const auto includes = transform<QSet<QString>>(m_projectIncludePaths,
                                                   [](const HeaderPath &hp) { return hp.path; });
    QSet<QString> toAdd;

    for (const QString &filePath : filePaths) {
        const QFileInfo fi(filePath);
        const QString directory = fi.absolutePath();
        if (fi.fileName() == "include" && !includes.contains(directory))
            toAdd << directory;
    }

    const QDir dir(projectDirectory().toUrlishString());
    const auto candidates = toAdd;
    for (const QString &path : candidates) {
        QString relative = dir.relativeFilePath(path);
        if (relative.isEmpty())
            relative = '.';
        m_rawProjectIncludePaths.append(relative);
    }

    bool result = saveRawList(newList, m_filesFileName);
    result &= saveRawList(m_rawProjectIncludePaths, m_includesFileName);
    refresh(Everything);

    return result;
}

RemovedFilesFromProject GenericBuildSystem::removeFiles(Node *, const FilePaths &filePaths, FilePaths *)
{
    QStringList newList = m_rawFileList;

    for (const FilePath &filePath : filePaths) {
        QHash<QString, QString>::iterator i = m_rawListEntries.find(filePath.toUrlishString());
        if (i != m_rawListEntries.end())
            newList.removeOne(i.value());
    }

    return saveRawFileList(newList) ? RemovedFilesFromProject::Ok
                                    : RemovedFilesFromProject::Error;
}

bool GenericBuildSystem::setFiles(const QStringList &filePaths)
{
    QStringList newList;
    QDir baseDir(projectDirectory().toUrlishString());
    for (const QString &filePath : filePaths)
        newList.append(baseDir.relativeFilePath(filePath));
    Utils::sort(newList);

    return saveRawFileList(newList);
}

bool GenericBuildSystem::renameFiles(Node *, const FilePairs &filesToRename, FilePaths *notRenamed)
{
    QStringList newList = m_rawFileList;

    bool success = true;
    for (const auto &[oldFilePath, newFilePath] : filesToRename) {
        const auto fail = [&, oldFilePath = oldFilePath] {
            success = false;
            if (notRenamed)
                *notRenamed << oldFilePath;
        };

        const auto i = m_rawListEntries.find(oldFilePath.toUrlishString());
        if (i == m_rawListEntries.end()) {
            fail();
            continue;
        }

        const int index = newList.indexOf(i.value());
        if (index == -1) {
            fail();
            continue;
        }

        QDir baseDir(projectDirectory().toUrlishString());
        newList.removeAt(index);
        insertSorted(&newList, baseDir.relativeFilePath(newFilePath.toUrlishString()));
    }

    if (!saveRawFileList(newList)) {
        success = false;
        if (notRenamed)
            *notRenamed = firstPaths(filesToRename);
    }

    return success;
}

static QStringList readFlags(const QString &filePath)
{
    const QStringList lines = readLines(filePath);
    if (lines.isEmpty())
        return {};
    QStringList flags;
    for (const auto &line : lines)
        flags.append(ProcessArgs::splitArgs(line, HostOsInfo::hostOs()));
    return flags;
}

void GenericBuildSystem::parse(RefreshOptions options)
{
    if (options & Files) {
        m_rawListEntries.clear();
        m_rawFileList = readLines(m_filesFileName);
        m_files = processEntries(m_rawFileList, &m_rawListEntries);
    }

    if (options & Configuration) {
        m_rawProjectIncludePaths = readLines(m_includesFileName);
        QStringList normalPaths;
        QStringList frameworkPaths;
        const auto baseDir = FilePath::fromString(m_includesFileName).parentDir();
        for (const QString &rawPath : std::as_const(m_rawProjectIncludePaths)) {
            if (rawPath.startsWith("-F"))
                frameworkPaths << rawPath.mid(2);
            else
                normalPaths << rawPath;
        }
        const auto expandedPaths = [this](const QStringList &paths) {
            return Utils::transform(processEntries(paths), [](const auto &pair) {
                return pair.first;
            });
        };
        m_projectIncludePaths = toUserHeaderPaths(expandedPaths(normalPaths));
        m_projectIncludePaths << toFrameworkHeaderPaths(expandedPaths(frameworkPaths));
        m_cxxflags = readFlags(m_cxxflagsFileName);
        m_cflags = readFlags(m_cflagsFileName);
    }
}

FilePath GenericBuildSystem::findCommonSourceRoot()
{
    if (m_files.isEmpty())
        return FilePath::fromFileInfo(QFileInfo(m_filesFileName));

    QString root = m_files.front().first.toUrlishString();
    for (const SourceFile &sourceFile : std::as_const(m_files)) {
        const QString item = sourceFile.first.toUrlishString();
        if (root.length() > item.length())
            root.truncate(item.length());

        for (int i = 0; i < root.length(); ++i) {
            if (root[i] != item[i]) {
                root.truncate(i);
                break;
            }
        }
    }
    return FilePath::fromString(QFileInfo(root).absolutePath());
}

void GenericBuildSystem::refresh(RefreshOptions options)
{
    // TODO: This stanza will have to appear in every BuildSystem and should eventually
    //       be centralized.
    if (this != project()->activeBuildSystem())
        return;

    ParseGuard guard = guardParsingRun();
    parse(options);

    if (options & Files) {
        auto newRoot = std::make_unique<ProjectNode>(projectDirectory());
        newRoot->setDisplayName(projectFilePath().completeBaseName());

        // find the common base directory of all source files
        FilePath baseDir = findCommonSourceRoot();

        std::vector<std::unique_ptr<FileNode>> fileNodes;
        for (const SourceFile &f : std::as_const(m_files)) {
            FileType fileType = FileType::Source; // ### FIXME
            if (f.first.endsWith(".qrc"))
                fileType = FileType::Resource;
            fileNodes.emplace_back(std::make_unique<FileNode>(f.first, fileType));
        }
        newRoot->addNestedNodes(std::move(fileNodes), baseDir);

        newRoot->addNestedNode(std::make_unique<FileNode>(FilePath::fromString(m_filesFileName),
                                                          FileType::Project));
        newRoot->addNestedNode(std::make_unique<FileNode>(FilePath::fromString(m_includesFileName),
                                                          FileType::Project));
        newRoot->addNestedNode(std::make_unique<FileNode>(FilePath::fromString(m_configFileName),
                                                          FileType::Project));
        newRoot->addNestedNode(std::make_unique<FileNode>(FilePath::fromString(m_cxxflagsFileName),
                                                          FileType::Project));
        newRoot->addNestedNode(std::make_unique<FileNode>(FilePath::fromString(m_cflagsFileName),
                                                          FileType::Project));

        newRoot->compress();
        setRootProjectNode(std::move(newRoot));
    }

    refreshCppCodeModel();
    updateDeploymentData();
    guard.markAsSuccess();

    emitBuildSystemUpdated();
}

/**
 * Expands environment variables and converts the path from relative to the
 * project to an absolute path.
 *
 * The \a map variable is an optional argument that will map the returned
 * absolute paths back to their original \a entries.
 */
GenericBuildSystem::SourceFiles GenericBuildSystem::processEntries(
        const QStringList &paths, QHash<QString, QString> *map) const
{
    const Environment buildEnv = buildConfiguration()->environment();
    const MacroExpander *expander = buildConfiguration()->macroExpander();

    const QDir projectDir(projectDirectory().toUrlishString());

    QFileInfo fileInfo;
    SourceFiles sourceFiles;
    std::set<QString> seenFiles;
    for (const QString &path : paths) {
        QString trimmedPath = path.trimmed();
        if (trimmedPath.isEmpty())
            continue;

        trimmedPath = buildEnv.expandVariables(trimmedPath);
        trimmedPath = expander->expand(trimmedPath);

        trimmedPath = FilePath::fromUserInput(trimmedPath).toUrlishString();

        QStringList tagsForFile;
        const int tagListPos = trimmedPath.indexOf('|');
        if (tagListPos != -1) {
            tagsForFile = trimmedPath.mid(tagListPos + 1).simplified()
                    .split(' ', Qt::SkipEmptyParts);
            trimmedPath = trimmedPath.left(tagListPos).trimmed();
        }

        if (!seenFiles.insert(trimmedPath).second)
            continue;

        fileInfo.setFile(projectDir, trimmedPath);
        if (fileInfo.exists()) {
            const QString absPath = fileInfo.absoluteFilePath();
            sourceFiles.append({FilePath::fromString(absPath), tagsForFile});
            if (map)
                map->insert(absPath, trimmedPath);
        }
    }
    return sourceFiles;
}

void GenericBuildSystem::refreshCppCodeModel()
{
    if (!m_cppCodeModelUpdater)
        return;
    QtSupport::CppKitInfo kitInfo(kit());
    QTC_ASSERT(kitInfo.isValid(), return);

    RawProjectPart rpp;
    rpp.setDisplayName(project()->displayName());
    rpp.setProjectFileLocation(projectFilePath());
    rpp.setQtVersion(kitInfo.projectPartQtVersion);
    rpp.setHeaderPaths(m_projectIncludePaths);
    rpp.setConfigFileName(m_configFileName);
    rpp.setFlagsForCxx({nullptr, m_cxxflags, projectDirectory()});
    rpp.setFlagsForC({nullptr, m_cflags, projectDirectory()});

    static const auto sourceFilesToStringList = [](const SourceFiles &sourceFiles) {
        return Utils::transform(sourceFiles, [](const SourceFile &f) {
            return f.first.toUrlishString();
        });
    };
    rpp.setFiles(sourceFilesToStringList(m_files));
    rpp.setPreCompiledHeaders(sourceFilesToStringList(
        Utils::filtered(m_files, [](const SourceFile &f) { return f.second.contains("pch"); })));

    m_cppCodeModelUpdater->update({project(), kitInfo, activeParseEnvironment(), {rpp}});
}

void GenericBuildSystem::updateDeploymentData()
{
    static const QString fileName("QtCreatorDeployment.txt");
    FilePath deploymentFilePath;
    deploymentFilePath = buildConfiguration()->buildDirectory().pathAppended(fileName);

    bool hasDeploymentData = deploymentFilePath.exists();
    if (!hasDeploymentData) {
        deploymentFilePath = projectDirectory().pathAppended(fileName);
        hasDeploymentData = deploymentFilePath.exists();
    }
    if (hasDeploymentData) {
        DeploymentData deploymentData;
        deploymentData.addFilesFromDeploymentFile(deploymentFilePath, projectDirectory());
        setDeploymentData(deploymentData);
        if (m_deployFileWatcher.files() != FilePaths{deploymentFilePath}) {
            m_deployFileWatcher.clear();
            m_deployFileWatcher.addFile(deploymentFilePath,
                                        FileSystemWatcher::WatchModifiedDate);
        }
    }
}

void GenericBuildSystem::removeFiles(const FilePaths &filesToRemove)
{
    if (removeFiles(nullptr, filesToRemove, nullptr) == RemovedFilesFromProject::Error) {
        TaskHub::addTask(BuildSystemTask(Task::Error,
                                         Tr::tr("Project files list update failed."),
                                         filesFilePath()));
    }
}

Project::RestoreResult GenericProject::fromMap(const Store &map, QString *errorMessage)
{
    const RestoreResult result = Project::fromMap(map, errorMessage);
    if (result != RestoreResult::Ok)
        return result;

    if (!activeTarget())
        addTargetForDefaultKit();

    // Sanity check: We need both a buildconfiguration and a runconfiguration!
    const QList<Target *> targetList = targets();
    if (targetList.isEmpty())
        return RestoreResult::Error;

    for (Target *t : targetList) {
        if (!t->activeBuildConfiguration()) {
            removeTarget(t);
            continue;
        }
        for (BuildConfiguration * const bc : t->buildConfigurations()) {
            if (!bc->activeRunConfiguration())
                bc->addRunConfiguration(new CustomExecutableRunConfiguration(bc));
        }
    }

    if (auto bs = activeBuildSystem())
        static_cast<GenericBuildSystem *>(bs)->refresh(Everything);

    return RestoreResult::Ok;
}

DeploymentKnowledge GenericProject::deploymentKnowledge() const
{
    return DeploymentKnowledge::Approximative;
}

void GenericProject::configureAsExampleProject(Kit *kit)
{
    QList<BuildInfo> infoList;
    const QList<Kit *> kits(kit != nullptr ? QList<Kit *>({kit}) : KitManager::kits());
    for (Kit *k : kits) {
        if (auto factory = BuildConfigurationFactory::find(k, projectFilePath())) {
            for (int i = 0; i < 5; ++i) {
                BuildInfo buildInfo;
                buildInfo.displayName = Tr::tr("Build %1").arg(i + 1);
                buildInfo.factory = factory;
                buildInfo.kitId = k->id();
                buildInfo.buildDirectory = projectFilePath();
                infoList << buildInfo;
            }
        }
    }
    setup(infoList);
}

void GenericProject::editFilesTriggered()
{
    SelectableFilesDialogEditFiles sfd(projectDirectory(),
                                       files(Project::AllFiles),
                                       ICore::dialogParent());
    if (sfd.exec() == QDialog::Accepted) {
        if (auto bs = static_cast<GenericBuildSystem *>(activeBuildSystem()))
            bs->setFiles(transform(sfd.selectedFiles(), &FilePath::toUrlishString));
    }
}

void GenericProject::removeFilesTriggered(const FilePaths &filesToRemove)
{
    if (const auto bs = activeBuildSystem())
        static_cast<GenericBuildSystem *>(bs)->removeFiles(filesToRemove);
}

void setupGenericProject(QObject *guard)
{
    static GenericBuildConfigurationFactory theGenericBuildConfigurationFactory;

    namespace PEC = ProjectExplorer::Constants;

    ProjectManager::registerProjectType<GenericProject>(Constants::GENERICMIMETYPE);

    ActionBuilder editAction(guard, "GenericProjectManager.EditFiles");
    editAction.setContext(Constants::GENERICPROJECT_ID);
    editAction.setText(Tr::tr("Edit Files..."));
    editAction.setCommandAttribute(Command::CA_Hide);
    editAction.addToContainer(PEC::M_PROJECTCONTEXT, PEC::G_PROJECT_FILES);
    editAction.addOnTriggered([] {
        if (auto genericProject = qobject_cast<GenericProject *>(ProjectTree::currentProject()))
            genericProject->editFilesTriggered();
    });

    ActionBuilder removeDirAction(guard, "GenericProject.RemoveDir");
    removeDirAction.setContext(Constants::GENERICPROJECT_ID);
    removeDirAction.setText(Tr::tr("Remove Directory"));
    removeDirAction.addToContainer(PEC::M_FOLDERCONTEXT, PEC::G_FOLDER_OTHER);
    removeDirAction.addOnTriggered([] {
        const auto folderNode = ProjectTree::currentNode()->asFolderNode();
        QTC_ASSERT(folderNode, return);
        const auto project = qobject_cast<GenericProject *>(folderNode->getProject());
        QTC_ASSERT(project, return);
        const FilePaths filesToRemove = transform(
            folderNode->findNodes([](const Node *node) { return node->asFileNode(); }),
            [](const Node *node) { return node->filePath();});
        project->removeFilesTriggered(filesToRemove);
    });
}

} // GenericProjectManager::Internal

#include "genericproject.moc"
