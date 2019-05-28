/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
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

#include "compilationdatabaseproject.h"

#include "compilationdatabaseconstants.h"
#include "compilationdbparser.h"

#include <coreplugin/icontext.h>
#include <cpptools/cppkitinfo.h>
#include <cpptools/cppprojectupdater.h>
#include <cpptools/projectinfo.h>
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/gcctoolchain.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/namedwidget.h>
#include <projectexplorer/processstep.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchainconfigwidget.h>
#include <projectexplorer/toolchainmanager.h>
#include <texteditor/textdocument.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>

#include <QFileDialog>
#include <QTimer>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

using namespace ProjectExplorer;

namespace CompilationDatabaseProjectManager {
namespace Internal {

namespace {


bool isGccCompiler(const QString &compilerName)
{
    return compilerName.contains("gcc")
           || (compilerName.contains("g++") && !compilerName.contains("clang"));
}

bool isClCompatibleCompiler(const QString &compilerName)
{
    return compilerName.endsWith("cl");
}

Core::Id getCompilerId(QString compilerName)
{
    if (Utils::HostOsInfo::isWindowsHost()) {
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

ToolChain *toolchainFromCompilerId(const Core::Id &compilerId, const Core::Id &language)
{
    return ToolChainManager::toolChain([&compilerId, &language](const ToolChain *tc) {
        if (!tc->isValid() || tc->language() != language)
            return false;
        return tc->typeId() == compilerId;
    });
}

QString compilerPath(QString pathFlag)
{
    if (pathFlag.isEmpty())
        return pathFlag;
#ifdef Q_OS_WIN
    // Handle short DOS style file names (cmake can generate them).
    const DWORD pathLength = GetLongPathNameW(reinterpret_cast<LPCWSTR>(pathFlag.utf16()),
                                              nullptr,
                                              0);
    if (pathLength > 0) {
        // Works only with existing paths.
        wchar_t *buffer = new wchar_t[pathLength];
        GetLongPathNameW(reinterpret_cast<LPCWSTR>(pathFlag.utf16()), buffer, pathLength);
        pathFlag = QString::fromUtf16(reinterpret_cast<ushort *>(buffer),
                                      static_cast<int>(pathLength - 1));
        delete[] buffer;
    }
#endif
    return QDir::fromNativeSeparators(pathFlag);
}

ToolChain *toolchainFromFlags(const Kit *kit, const QStringList &flags, const Core::Id &language)
{
    if (flags.empty())
        return ToolChainKitAspect::toolChain(kit, language);

    // Try exact compiler match.
    const Utils::FilePath compiler = Utils::FilePath::fromString(compilerPath(flags.front()));
    ToolChain *toolchain = ToolChainManager::toolChain([&compiler, &language](const ToolChain *tc) {
        return tc->isValid() && tc->language() == language && tc->compilerCommand() == compiler;
    });
    if (toolchain)
        return toolchain;

    Core::Id compilerId = getCompilerId(compiler.fileName());
    if ((toolchain = toolchainFromCompilerId(compilerId, language)))
        return toolchain;

    if (compilerId != ProjectExplorer::Constants::CLANG_TOOLCHAIN_TYPEID &&
            compilerId != ProjectExplorer::Constants::CLANG_CL_TOOLCHAIN_TYPEID) {
        compilerId = Utils::HostOsInfo::isWindowsHost()
                ? ProjectExplorer::Constants::CLANG_CL_TOOLCHAIN_TYPEID
                : ProjectExplorer::Constants::CLANG_TOOLCHAIN_TYPEID;
        if ((toolchain = toolchainFromCompilerId(compilerId, language)))
            return toolchain;
    }

    toolchain = ToolChainKitAspect::toolChain(kit, language);
    qWarning() << "No matching toolchain found, use the default.";
    return toolchain;
}

void addDriverModeFlagIfNeeded(const ToolChain *toolchain,
                               QStringList &flags,
                               const QStringList &originalFlags)
{
    if (toolchain->typeId() == ProjectExplorer::Constants::CLANG_CL_TOOLCHAIN_TYPEID
        && !originalFlags.empty() && !originalFlags.front().endsWith("cl")
        && !originalFlags.front().endsWith("cl.exe")) {
        flags.prepend("--driver-mode=g++");
    }
}

CppTools::RawProjectPart makeRawProjectPart(const Utils::FilePath &projectFile,
                                            Kit *kit,
                                            CppTools::KitInfo &kitInfo,
                                            const QString &workingDir,
                                            const Utils::FilePath &fileName,
                                            QStringList flags)
{
    HeaderPaths headerPaths;
    Macros macros;
    CppTools::ProjectFile::Kind fileKind = CppTools::ProjectFile::Unclassified;

    const QStringList originalFlags = flags;
    filteredFlags(fileName.fileName(),
                  workingDir,
                  flags,
                  headerPaths,
                  macros,
                  fileKind,
                  kitInfo.sysRootPath);

    CppTools::RawProjectPart rpp;
    rpp.setProjectFileLocation(projectFile.toString());
    rpp.setBuildSystemTarget(workingDir);
    rpp.setDisplayName(fileName.fileName());
    rpp.setFiles({fileName.toString()});
    rpp.setHeaderPaths(headerPaths);
    rpp.setMacros(macros);

    if (fileKind == CppTools::ProjectFile::Kind::CHeader
            || fileKind == CppTools::ProjectFile::Kind::CSource) {
        if (!kitInfo.cToolChain) {
            kitInfo.cToolChain = toolchainFromFlags(kit,
                                                    originalFlags,
                                                    ProjectExplorer::Constants::C_LANGUAGE_ID);
            ToolChainKitAspect::setToolChain(kit, kitInfo.cToolChain);
        }
        addDriverModeFlagIfNeeded(kitInfo.cToolChain, flags, originalFlags);
        rpp.setFlagsForC({kitInfo.cToolChain, flags});
    } else {
        if (!kitInfo.cxxToolChain) {
            kitInfo.cxxToolChain = toolchainFromFlags(kit,
                                                      originalFlags,
                                                      ProjectExplorer::Constants::CXX_LANGUAGE_ID);
            ToolChainKitAspect::setToolChain(kit, kitInfo.cxxToolChain);
        }
        addDriverModeFlagIfNeeded(kitInfo.cxxToolChain, flags, originalFlags);
        rpp.setFlagsForCxx({kitInfo.cxxToolChain, flags});
    }

    return rpp;
}

QStringList relativeDirsList(Utils::FilePath currentPath, const Utils::FilePath &rootPath)
{
    QStringList dirsList;
    while (!currentPath.isEmpty() && currentPath != rootPath) {
        QString dirName = currentPath.fileName();
        if (dirName.isEmpty())
            dirName = currentPath.toString();
        dirsList.prepend(dirName);
        currentPath = currentPath.parentDir();
    }
    return dirsList;
}

FolderNode *addChildFolderNode(FolderNode *parent, const QString &childName)
{
    const Utils::FilePath path = parent->filePath().pathAppended(childName);
    auto node = std::make_unique<FolderNode>(path);
    FolderNode *childNode = node.get();
    childNode->setDisplayName(childName);
    parent->addNode(std::move(node));

    return childNode;
}

FolderNode *addOrGetChildFolderNode(FolderNode *parent, const QString &childName)
{
    for (FolderNode *folder : parent->folderNodes()) {
        if (folder->filePath().fileName() == childName) {
            return folder;
        }
    }

    return addChildFolderNode(parent, childName);
}

    // Return the node for folderPath.
FolderNode *createFoldersIfNeeded(FolderNode *root, const Utils::FilePath &folderPath)
{
    const QStringList dirsList = relativeDirsList(folderPath, root->filePath());

    FolderNode *parent = root;
    for (const QString &dir : dirsList)
        parent = addOrGetChildFolderNode(parent, dir);

    return parent;
}

FileType fileTypeForName(const QString &fileName)
{
    CppTools::ProjectFile::Kind fileKind = CppTools::ProjectFile::classify(fileName);
    if (CppTools::ProjectFile::isHeader(fileKind))
        return FileType::Header;
    return FileType::Source;
}

void addChild(FolderNode *root, const Utils::FilePath &fileName)
{
    FolderNode *parentNode = createFoldersIfNeeded(root, fileName.parentDir());
    if (!parentNode->fileNode(fileName)) {
        parentNode->addNode(
            std::make_unique<FileNode>(fileName, fileTypeForName(fileName.fileName())));
    }
}

void createTree(std::unique_ptr<ProjectNode> &root,
                const Utils::FilePath &rootPath,
                const CppTools::RawProjectParts &rpps,
                const QList<FileNode *> &scannedFiles = QList<FileNode *>())
{
    root->setAbsoluteFilePathAndLine(rootPath, -1);
    std::unique_ptr<FolderNode> secondRoot;

    for (const CppTools::RawProjectPart &rpp : rpps) {
        for (const QString &filePath : rpp.files) {
            Utils::FilePath fileName = Utils::FilePath::fromString(filePath);
            if (!fileName.isChildOf(rootPath)) {
                if (fileName.isChildOf(Utils::FilePath::fromString(rpp.buildSystemTarget))) {
                    if (!secondRoot)
                        secondRoot = std::make_unique<ProjectNode>(
                            Utils::FilePath::fromString(rpp.buildSystemTarget));
                    addChild(secondRoot.get(), fileName);
                }
            } else {
                addChild(root.get(), fileName);
            }
        }
    }

    for (FileNode *node : scannedFiles) {
        if (node->fileType() != FileType::Header)
            continue;

        const Utils::FilePath fileName = node->filePath();
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


} // anonymous namespace

void CompilationDatabaseProject::buildTreeAndProjectParts()
{
    CppTools::KitInfo kitInfo(this);
    QTC_ASSERT(kitInfo.isValid(), return);
    // Reset toolchains to pick them based on the database entries.
    kitInfo.cToolChain = nullptr;
    kitInfo.cxxToolChain = nullptr;
    CppTools::RawProjectParts rpps;

    QTC_ASSERT(m_parser, return);
    const DbContents dbContents = m_parser->dbContents();
    const DbEntry *prevEntry = nullptr;
    for (const DbEntry &entry : dbContents.entries) {
        if (prevEntry && prevEntry->flags == entry.flags) {
            rpps.back().files.append(entry.fileName.toString());
            continue;
        }

        prevEntry = &entry;

        CppTools::RawProjectPart rpp = makeRawProjectPart(projectFilePath(),
                                                          m_kit.get(),
                                                          kitInfo,
                                                          entry.workingDir,
                                                          entry.fileName,
                                                          entry.flags);

        rpps.append(rpp);
    }

    if (!dbContents.extras.empty()) {
        const Utils::FilePath baseDir = projectFilePath().parentDir();

        QStringList extraFiles;
        for (const QString &extra : dbContents.extras)
            extraFiles.append(baseDir.pathAppended(extra).toString());

        CppTools::RawProjectPart rppExtra;
        rppExtra.setFiles(extraFiles);
        rpps.append(rppExtra);
    }


    auto root = std::make_unique<ProjectNode>(projectDirectory());
    createTree(root, rootProjectDirectory(), rpps, m_parser->scannedFiles());

    root->addNode(std::make_unique<FileNode>(projectFilePath(), FileType::Project));

    if (QFile::exists(dbContents.extraFileName))
        root->addNode(std::make_unique<FileNode>(Utils::FilePath::fromString(dbContents.extraFileName),
                                                 FileType::Project));

    setRootProjectNode(std::move(root));

    m_cppCodeModelUpdater->update({this, kitInfo, rpps});
}

CompilationDatabaseProject::CompilationDatabaseProject(const Utils::FilePath &projectFile)
    : Project(Constants::COMPILATIONDATABASEMIMETYPE, projectFile)
    , m_cppCodeModelUpdater(std::make_unique<CppTools::CppProjectUpdater>())
    , m_parseDelay(new QTimer(this))
{
    setId(Constants::COMPILATIONDATABASEPROJECT_ID);
    setProjectLanguages(Core::Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));
    setDisplayName(projectDirectory().fileName());
    setRequiredKitPredicate([](const Kit *) { return false; });
    setPreferredKitPredicate([](const Kit *) { return false; });

    m_kit.reset(KitManager::defaultKit()->clone());
    connect(this, &CompilationDatabaseProject::parsingFinished, this, [this]() {
        if (!m_hasTarget) {
            addTarget(createTarget(m_kit.get()));
            m_hasTarget = true;
        }
    });

    connect(this, &CompilationDatabaseProject::rootProjectDirectoryChanged,
            m_parseDelay, QOverload<>::of(&QTimer::start));

    m_fileSystemWatcher.addFile(projectFile.toString(), Utils::FileSystemWatcher::WatchModifiedDate);
    m_fileSystemWatcher.addFile(projectFile.toString() + Constants::COMPILATIONDATABASEPROJECT_FILES_SUFFIX,
                                Utils::FileSystemWatcher::WatchModifiedDate);
    connect(&m_fileSystemWatcher, &Utils::FileSystemWatcher::fileChanged,
            m_parseDelay, QOverload<>::of(&QTimer::start));
    connect(m_parseDelay, &QTimer::timeout, this, &CompilationDatabaseProject::reparseProject);
    m_parseDelay->setSingleShot(true);
    m_parseDelay->setInterval(1000);
}

Utils::FilePath CompilationDatabaseProject::rootPathFromSettings() const
{
#ifdef WITH_TESTS
    return Utils::FilePath::fromString(projectDirectory().fileName());
#else
    return Utils::FileName::fromString(
        namedSettings(ProjectExplorer::Constants::PROJECT_ROOT_PATH_KEY).toString());
#endif
}

Project::RestoreResult CompilationDatabaseProject::fromMap(const QVariantMap &map,
                                                           QString *errorMessage)
{
    Project::RestoreResult result = Project::fromMap(map, errorMessage);
    if (result == Project::RestoreResult::Ok) {
        const Utils::FilePath rootPath = rootPathFromSettings();
        if (rootPath.isEmpty())
            changeRootProjectDirectory(); // This triggers reparse itself.
        else
            reparseProject();
    }

    return result;
}

void CompilationDatabaseProject::reparseProject()
{
    if (m_parser) {
        QTC_CHECK(isParsing());
        m_parser->stop();
        emitParsingFinished(false);
    }
    m_parser = new CompilationDbParser(displayName(), projectFilePath(), rootPathFromSettings(),
                                       m_mimeBinaryCache, this);
    connect(m_parser, &CompilationDbParser::finished, this, [this](bool success) {
        if (success)
            buildTreeAndProjectParts();
        m_parser = nullptr;
        emitParsingFinished(success);
    });
    emitParsingStarted();
    m_parser->start();
}

CompilationDatabaseProject::~CompilationDatabaseProject()
{
    m_parserWatcher.cancel();
    m_parserWatcher.waitForFinished();
}

static TextEditor::TextDocument *createCompilationDatabaseDocument()
{
    auto doc = new TextEditor::TextDocument;
    doc->setId(Constants::COMPILATIONDATABASEPROJECT_ID);
    doc->setMimeType(Constants::COMPILATIONDATABASEMIMETYPE);
    return doc;
}

CompilationDatabaseEditorFactory::CompilationDatabaseEditorFactory()
{
    setId(Constants::COMPILATIONDATABASEPROJECT_ID);
    setDisplayName("Compilation Database");
    addMimeType(Constants::COMPILATIONDATABASEMIMETYPE);

    setEditorCreator([]() { return new TextEditor::BaseTextEditor; });
    setEditorWidgetCreator([]() { return new TextEditor::TextEditorWidget; });
    setDocumentCreator(createCompilationDatabaseDocument);
    setUseGenericHighlighter(true);
    setCommentDefinition(Utils::CommentDefinition::HashStyle);
    setCodeFoldingSupported(true);
}

CompilationDatabaseBuildConfiguration::CompilationDatabaseBuildConfiguration(
    ProjectExplorer::Target *target, Core::Id id)
    : ProjectExplorer::BuildConfiguration(target, id)
{
    target->setApplicationTargets({BuildTargetInfo()});
}

void CompilationDatabaseBuildConfiguration::initialize(const ProjectExplorer::BuildInfo &info)
{
    ProjectExplorer::BuildConfiguration::initialize(info);
    BuildStepList *buildSteps = stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    buildSteps->appendStep(new ProjectExplorer::ProcessStep(buildSteps));
}

ProjectExplorer::NamedWidget *CompilationDatabaseBuildConfiguration::createConfigWidget()
{
    return new ProjectExplorer::NamedWidget();
}

ProjectExplorer::BuildConfiguration::BuildType CompilationDatabaseBuildConfiguration::buildType() const
{
    return ProjectExplorer::BuildConfiguration::Release;
}

CompilationDatabaseBuildConfigurationFactory::CompilationDatabaseBuildConfigurationFactory()
{
    registerBuildConfiguration<CompilationDatabaseBuildConfiguration>(
        "CompilationDatabase.CompilationDatabaseBuildConfiguration");

    setSupportedProjectType(Constants::COMPILATIONDATABASEPROJECT_ID);
    setSupportedProjectMimeTypeName(Constants::COMPILATIONDATABASEMIMETYPE);
}

static QList<ProjectExplorer::BuildInfo> defaultBuildInfos(
    const ProjectExplorer::BuildConfigurationFactory *factory, const QString &name)
{
    ProjectExplorer::BuildInfo info(factory);
    info.typeName = name;
    info.displayName = name;
    info.buildType = BuildConfiguration::Release;
    QList<ProjectExplorer::BuildInfo> buildInfos;
    buildInfos << info;
    return buildInfos;
}

QList<ProjectExplorer::BuildInfo> CompilationDatabaseBuildConfigurationFactory::availableBuilds(
    const ProjectExplorer::Target * /*parent*/) const
{
    return defaultBuildInfos(this, tr("Release"));
}

QList<ProjectExplorer::BuildInfo> CompilationDatabaseBuildConfigurationFactory::availableSetups(
    const ProjectExplorer::Kit * /*k*/, const QString & /*projectPath*/) const
{
    return defaultBuildInfos(this, tr("Release"));
}

} // namespace Internal
} // namespace CompilationDatabaseProjectManager
