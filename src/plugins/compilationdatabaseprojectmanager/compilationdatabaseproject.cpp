// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "compilationdatabaseproject.h"

#include "compilationdatabaseconstants.h"
#include "compilationdbparser.h"

#include <coreplugin/coreplugintr.h>
#include <coreplugin/icontext.h>

#include <cppeditor/projectinfo.h>

#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/gcctoolchain.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projectupdater.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchainkitaspect.h>
#include <projectexplorer/toolchainmanager.h>

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <utils/algorithm.h>
#include <utils/filesystemwatcher.h>
#include <utils/qtcassert.h>

#include <QFileDialog>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

using namespace ProjectExplorer;
using namespace Utils;

namespace CompilationDatabaseProjectManager::Internal {

static bool isGccCompiler(const QString &compilerName)
{
    return compilerName.contains("gcc")
           || (compilerName.contains("g++") && !compilerName.contains("clang"));
}

static bool isClCompatibleCompiler(const QString &compilerName)
{
    return compilerName.endsWith("cl");
}

static Id getCompilerId(QString compilerName)
{
    if (HostOsInfo::isWindowsHost()) {
        if (compilerName.endsWith(".exe"))
            compilerName.chop(4);
        if (isGccCompiler(compilerName))
            return ProjectExplorer::Constants::MINGW_TOOLCHAIN_TYPEID;
        if (isClCompatibleCompiler(compilerName))
            return ProjectExplorer::Constants::CLANG_CL_TOOLCHAIN_TYPEID;
        return ProjectExplorer::Constants::CLANG_TOOLCHAIN_TYPEID;
    }
    if (isGccCompiler(compilerName))
        return ProjectExplorer::Constants::GCC_TOOLCHAIN_TYPEID;

    // Default is clang
    return ProjectExplorer::Constants::CLANG_TOOLCHAIN_TYPEID;
}

static Toolchain *toolchainFromCompilerId(const Id &compilerId, const Id &language)
{
    return ToolchainManager::toolchain([&compilerId, &language](const Toolchain *tc) {
        if (!tc->isValid() || tc->language() != language)
            return false;
        return tc->typeId() == compilerId;
    });
}

static QString compilerPath(QString pathFlag)
{
    if (pathFlag.isEmpty())
        return pathFlag;
#ifdef Q_OS_WIN
    // Handle short DOS style file names (cmake can generate them).
    const DWORD pathLength
        = GetLongPathNameW(reinterpret_cast<LPCWSTR>(pathFlag.utf16()), nullptr, 0);
    if (pathLength > 0) {
        // Works only with existing paths.
        wchar_t *buffer = new wchar_t[pathLength];
        GetLongPathNameW(reinterpret_cast<LPCWSTR>(pathFlag.utf16()), buffer, pathLength);
        pathFlag = QString::fromUtf16(
            reinterpret_cast<char16_t *>(buffer), static_cast<int>(pathLength - 1));
        delete[] buffer;
    }
#endif
    return QDir::fromNativeSeparators(pathFlag);
}

static Toolchain *toolchainFromFlags(const Kit *kit, const QStringList &flags, const Id &language)
{
    Toolchain *const kitToolchain = ToolchainKitAspect::toolchain(kit, language);

    if (flags.empty())
        return kitToolchain;

    // Try exact compiler match.
    const FilePath compiler = FilePath::fromUserInput(compilerPath(flags.front()));
    Toolchain *toolchain = ToolchainManager::toolchain([&compiler, &language](const Toolchain *tc) {
        return tc->isValid() && tc->language() == language && tc->compilerCommand() == compiler;
    });
    if (toolchain)
        return toolchain;

    Id compilerId = getCompilerId(compiler.fileName());
    if (kitToolchain->isValid() && kitToolchain->typeId() == compilerId)
        return kitToolchain;
    if ((toolchain = toolchainFromCompilerId(compilerId, language)))
        return toolchain;

    if (compilerId != ProjectExplorer::Constants::CLANG_TOOLCHAIN_TYPEID
        && compilerId != ProjectExplorer::Constants::CLANG_CL_TOOLCHAIN_TYPEID) {
        compilerId = HostOsInfo::isWindowsHost()
                         ? Id(ProjectExplorer::Constants::CLANG_CL_TOOLCHAIN_TYPEID)
                         : Id(ProjectExplorer::Constants::CLANG_TOOLCHAIN_TYPEID);
        if (kitToolchain->isValid() && kitToolchain->typeId() == compilerId)
            return kitToolchain;
        if ((toolchain = toolchainFromCompilerId(compilerId, language)))
            return toolchain;
    }

    qWarning() << "No matching toolchain found, use the default.";
    return kitToolchain;
}

static void addDriverModeFlagIfNeeded(
    const Toolchain *toolchain, QStringList &flags, const QStringList &originalFlags)
{
    if (toolchain->typeId() == ProjectExplorer::Constants::CLANG_CL_TOOLCHAIN_TYPEID
        && !originalFlags.empty() && !originalFlags.front().endsWith("cl")
        && !originalFlags.front().endsWith("cl.exe")) {
        flags.prepend("--driver-mode=g++");
    }
}

static RawProjectPart makeRawProjectPart(
    const FilePath &projectFile,
    Kit *kit,
    ProjectExplorer::KitInfo &kitInfo,
    const FilePath &workingDir,
    const FilePath &filePath,
    QStringList flags)
{
    HeaderPaths headerPaths;
    Macros macros;
    CppEditor::ProjectFile::Kind fileKind = CppEditor::ProjectFile::Unclassified;

    const QStringList originalFlags = flags;
    filteredFlags(filePath, workingDir, flags, headerPaths, macros, fileKind, kitInfo.sysRootPath);

    RawProjectPart rpp;

    rpp.setProjectFileLocation(projectFile);
    rpp.setBuildSystemTarget(workingDir.path());
    rpp.setDisplayName(filePath.fileName());
    rpp.setFiles({filePath});

    rpp.setHeaderPaths(headerPaths);
    rpp.setMacros(macros);

    if (fileKind == CppEditor::ProjectFile::Kind::CHeader
        || fileKind == CppEditor::ProjectFile::Kind::CSource) {
        if (!kitInfo.cToolchain) {
            kitInfo.cToolchain
                = toolchainFromFlags(kit, originalFlags, ProjectExplorer::Constants::C_LANGUAGE_ID);
        }
        addDriverModeFlagIfNeeded(kitInfo.cToolchain, flags, originalFlags);
        rpp.setFlagsForC({kitInfo.cToolchain, flags, workingDir});
    } else {
        if (!kitInfo.cxxToolchain) {
            kitInfo.cxxToolchain
                = toolchainFromFlags(kit, originalFlags, ProjectExplorer::Constants::CXX_LANGUAGE_ID);
        }
        addDriverModeFlagIfNeeded(kitInfo.cxxToolchain, flags, originalFlags);
        rpp.setFlagsForCxx({kitInfo.cxxToolchain, flags, workingDir});
    }

    return rpp;
}

static QStringList relativeDirsList(FilePath currentPath, const FilePath &rootPath)
{
    QStringList dirsList;
    while (!currentPath.isEmpty() && currentPath != rootPath) {
        QString dirName = currentPath.fileName();
        if (dirName.isEmpty())
            dirName = currentPath.path();
        dirsList.prepend(dirName);
        currentPath = currentPath.parentDir();
    }
    return dirsList;
}

static FolderNode *addChildFolderNode(FolderNode *parent, const QString &childName)
{
    const FilePath path = parent->filePath().pathAppended(childName);
    auto node = std::make_unique<FolderNode>(path);
    FolderNode *childNode = node.get();
    childNode->setDisplayName(childName);
    parent->addNode(std::move(node));

    return childNode;
}

static FolderNode *addOrGetChildFolderNode(FolderNode *parent, const QString &childName)
{
    FolderNode *fn = parent->findChildFolderNode(
        [&](FolderNode *folder) { return folder->filePath().fileName() == childName; });
    return fn ? fn : addChildFolderNode(parent, childName);
}

// Return the node for folderPath.
static FolderNode *createFoldersIfNeeded(FolderNode *root, const FilePath &folderPath)
{
    const QStringList dirsList = relativeDirsList(folderPath, root->filePath());

    FolderNode *parent = root;
    for (const QString &dir : dirsList)
        parent = addOrGetChildFolderNode(parent, dir);

    return parent;
}

static FileType fileTypeForName(const QString &fileName)
{
    CppEditor::ProjectFile::Kind fileKind = CppEditor::ProjectFile::classify(fileName);
    if (CppEditor::ProjectFile::isHeader(fileKind))
        return FileType::Header;
    return FileType::Source;
}

static void addChild(FolderNode *root, const FilePath &fileName)
{
    FolderNode *parentNode = createFoldersIfNeeded(root, fileName.parentDir());
    if (!parentNode->fileNode(fileName)) {
        parentNode->addNode(
            std::make_unique<FileNode>(fileName, fileTypeForName(fileName.fileName())));
    }
}

static void createTree(
    std::unique_ptr<ProjectNode> &root,
    const FilePath &rootPath,
    const RawProjectParts &rpps,
    const QList<FileNode *> &scannedFiles = QList<FileNode *>())
{
    root->setAbsoluteFilePathAndLine(rootPath, -1);
    std::unique_ptr<FolderNode> secondRoot;

    for (const RawProjectPart &rpp : rpps) {
        for (const Utils::FilePath &filePath : rpp.files) {
            if (!filePath.isChildOf(rootPath)) {
                if (filePath.isChildOf(FilePath::fromString(rpp.buildSystemTarget))) {
                    if (!secondRoot)
                        secondRoot = std::make_unique<ProjectNode>(
                            FilePath::fromString(rpp.buildSystemTarget));
                    addChild(secondRoot.get(), filePath);
                }
            } else {
                addChild(root.get(), filePath);
            }
        }
    }

    for (FileNode *node : scannedFiles) {
        if (node->fileType() != FileType::Header)
            continue;

        const FilePath fileName = node->filePath();
        if (!fileName.isChildOf(rootPath))
            continue;
        FolderNode *parentNode = createFoldersIfNeeded(root.get(), fileName.parentDir());
        if (!parentNode->fileNode(fileName)) {
            std::unique_ptr<FileNode> headerNode(node->clone());
            headerNode->setEnabled(false);
            parentNode->addNode(std::move(headerNode));
        }
    }
    qDeleteAll(scannedFiles);

    if (secondRoot) {
        std::unique_ptr<ProjectNode> firstRoot = std::move(root);
        root = std::make_unique<ProjectNode>(firstRoot->filePath());
        firstRoot->setDisplayName(rootPath.fileName());
        root->addNode(std::move(firstRoot));
        root->addNode(std::move(secondRoot));
    }
}

// CompilationDatabaseBuildSystem

class CompilationDatabaseBuildSystem final : public BuildSystem
{
public:
    explicit CompilationDatabaseBuildSystem(BuildConfiguration *bc);
    ~CompilationDatabaseBuildSystem();

    void triggerParsing() final;

    void reparseProject();
    void updateDeploymentData();
    void buildTreeAndProjectParts();

    std::unique_ptr<ProjectUpdater> m_cppCodeModelUpdater;
    MimeBinaryCache m_mimeBinaryCache;
    QByteArray m_projectFileHash;
    CompilationDbParser *m_parser = nullptr;
    FileSystemWatcher *const m_deployFileWatcher;
};

CompilationDatabaseBuildSystem::CompilationDatabaseBuildSystem(BuildConfiguration *bc)
    : BuildSystem(bc)
    , m_cppCodeModelUpdater(ProjectUpdaterFactory::createCppProjectUpdater())
    , m_deployFileWatcher(new FileSystemWatcher(this))
{
    connect(project(), &CompilationDatabaseProject::rootProjectDirectoryChanged, this, [this] {
        m_projectFileHash.clear();
        requestDelayedParse();
    });

    requestDelayedParse();

    connect(project(), &Project::projectFileIsDirty,
            this, &CompilationDatabaseBuildSystem::reparseProject);
    connect(m_deployFileWatcher, &FileSystemWatcher::fileChanged,
            this, &CompilationDatabaseBuildSystem::updateDeploymentData);
    connect(project(), &Project::activeTargetChanged,
            this, &CompilationDatabaseBuildSystem::updateDeploymentData);
    connect(target(), &Target::activeBuildConfigurationChanged,
            this, &CompilationDatabaseBuildSystem::updateDeploymentData);
}

CompilationDatabaseBuildSystem::~CompilationDatabaseBuildSystem()
{
    if (m_parser)
        delete m_parser;
}

void CompilationDatabaseBuildSystem::triggerParsing()
{
    reparseProject();
}

void CompilationDatabaseBuildSystem::buildTreeAndProjectParts()
{
    Kit *k = kit();
    ProjectExplorer::KitInfo kitInfo(k);
    QTC_ASSERT(kitInfo.isValid(), return);
    // Reset toolchains to pick them based on the database entries.
    kitInfo.cToolchain = nullptr;
    kitInfo.cxxToolchain = nullptr;
    RawProjectParts rpps;

    QTC_ASSERT(m_parser, return);
    const DbContents dbContents = m_parser->dbContents();
    const DbEntry *prevEntry = nullptr;
    for (const DbEntry &entry : dbContents.entries) {
        if (prevEntry && prevEntry->flags == entry.flags) {
            rpps.back().files.append(entry.fileName);
            continue;
        }

        prevEntry = &entry;

        RawProjectPart rpp = makeRawProjectPart(
            projectFilePath(), k, kitInfo, entry.workingDir, entry.fileName, entry.flags);

        rpps.append(rpp);
    }

    if (!dbContents.extras.empty()) {
        const FilePath baseDir = projectFilePath().parentDir();

        FilePaths extraFiles;
        for (const QString &extra : dbContents.extras)
            extraFiles.append(baseDir.pathAppended(extra));

        RawProjectPart rppExtra;
        rppExtra.setFiles(extraFiles);
        rpps.append(rppExtra);
    }

    auto root = std::make_unique<ProjectNode>(projectDirectory());
    createTree(root, project()->rootProjectDirectory(), rpps, m_parser->scannedFiles());

    root->addNode(std::make_unique<FileNode>(projectFilePath(), FileType::Project));

    if (QFileInfo::exists(dbContents.extraFileName))
        root->addNode(std::make_unique<FileNode>(
            FilePath::fromString(dbContents.extraFileName), FileType::Project));

    setRootProjectNode(std::move(root));

    m_cppCodeModelUpdater->update({project(), kitInfo, activeParseEnvironment(), rpps});
    updateDeploymentData();
}

// CompilationDatabaseProject

CompilationDatabaseProject::CompilationDatabaseProject(const FilePath &projectFile)
    : Project(Constants::COMPILATIONDATABASEMIMETYPE, projectFile)
{
    setType(Constants::COMPILATIONDATABASEPROJECT_ID);
    setProjectLanguages(Core::Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));
    setDisplayName(projectDirectory().fileName());
    setBuildSystemCreator<CompilationDatabaseBuildSystem>("compilationdb");
    setExtraProjectFiles(
        {projectFile.stringAppended(Constants::COMPILATIONDATABASEPROJECT_FILES_SUFFIX)});
}

static FilePath rootPathFromSettings(Project *project)
{
    FilePath rootPath;
#ifndef WITH_TESTS
    rootPath = FilePath::fromString(
        project->namedSettings(ProjectExplorer::Constants::PROJECT_ROOT_PATH_KEY).toString());
#endif
    if (rootPath.isEmpty())
        rootPath = project->projectDirectory();
    return rootPath;
}

void CompilationDatabaseProject::configureAsExampleProject(Kit *kit)
{
    if (kit)
        addTargetForKit(kit);
    else if (KitManager::defaultKit())
        addTargetForKit(KitManager::defaultKit());
}

void CompilationDatabaseBuildSystem::reparseProject()
{
    if (project()->activeBuildSystem() != this)
        return;

    if (m_parser) {
        QTC_CHECK(isParsing());
        m_parser->stop();
    }
    const FilePath rootPath = rootPathFromSettings(project());
    m_parser = new CompilationDbParser(
        project()->displayName(),
        projectFilePath(),
        rootPath,
        m_mimeBinaryCache,
        guardParsingRun(),
        this);
    connect(m_parser, &CompilationDbParser::finished, this, [this](ParseResult result) {
        m_projectFileHash = m_parser->projectFileHash();
        if (result == ParseResult::Success)
            buildTreeAndProjectParts();
        m_parser = nullptr;
    });
    m_parser->setPreviousProjectFileHash(m_projectFileHash);
    m_parser->start();
}

void CompilationDatabaseBuildSystem::updateDeploymentData()
{
    if (project()->activeBuildSystem() != this)
        return;

    const FilePath deploymentFilePath = projectDirectory().pathAppended("QtCreatorDeployment.txt");
    DeploymentData deploymentData;
    deploymentData.addFilesFromDeploymentFile(deploymentFilePath, projectDirectory());
    setDeploymentData(deploymentData);
    if (m_deployFileWatcher->files() != FilePaths{deploymentFilePath}) {
        m_deployFileWatcher->clear();
        m_deployFileWatcher->addFile(deploymentFilePath, FileSystemWatcher::WatchModifiedDate);
    }

    emitBuildSystemUpdated();
}

static TextEditor::TextDocument *createCompilationDatabaseDocument()
{
    auto doc = new TextEditor::TextDocument;
    doc->setId(Constants::COMPILATIONDATABASEPROJECT_ID);
    doc->setMimeType(Constants::COMPILATIONDATABASEMIMETYPE);
    return doc;
}

class CompilationDatabaseEditorFactory final : public TextEditor::TextEditorFactory
{
public:
    CompilationDatabaseEditorFactory()
    {
        setId(Constants::COMPILATIONDATABASEPROJECT_ID);
        setDisplayName(::Core::Tr::tr("Compilation Database"));
        addMimeType(Constants::COMPILATIONDATABASEMIMETYPE);

        setEditorCreator([] { return new TextEditor::BaseTextEditor; });
        setEditorWidgetCreator([] { return new TextEditor::TextEditorWidget; });
        setDocumentCreator(createCompilationDatabaseDocument);
        setUseGenericHighlighter(true);
        setCommentDefinition(Utils::CommentDefinition::HashStyle);
        setCodeFoldingSupported(true);
    }
};

void setupCompilationDatabaseEditor()
{
    static CompilationDatabaseEditorFactory theCompilationDatabaseEditorFactory;
}

// CompilationDatabaseBuildConfigurationFactory

class CompilationDatabaseBuildConfiguration final : public BuildConfiguration
{
public:
    CompilationDatabaseBuildConfiguration(Target *target, Id id)
        : BuildConfiguration(target, id)
    {}
};

class CompilationDatabaseBuildConfigurationFactory final : public BuildConfigurationFactory
{
public:
    CompilationDatabaseBuildConfigurationFactory()
    {
        registerBuildConfiguration<CompilationDatabaseBuildConfiguration>(
            "CompilationDatabase.CompilationDatabaseBuildConfiguration");

        setSupportedProjectType(Constants::COMPILATIONDATABASEPROJECT_ID);
        setSupportedProjectMimeTypeName(Constants::COMPILATIONDATABASEMIMETYPE);

        setBuildGenerator([](const Kit *, const FilePath &projectPath, bool) {
            const QString name = QCoreApplication::translate("QtC::ProjectExplorer", "Release");
            ProjectExplorer::BuildInfo info;
            info.typeName = name;
            info.displayName = name;
            info.buildType = BuildConfiguration::Release;
            info.buildDirectory = projectPath.parentDir();
            return QList<BuildInfo>{info};
        });
    }
};

void setupCompilationDatabaseBuildConfiguration()
{
    static CompilationDatabaseBuildConfigurationFactory theCDBuildConfigurationFactory;
}

} // namespace CompilationDatabaseProjectManager::Internal
