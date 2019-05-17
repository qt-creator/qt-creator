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
#include "compilationdatabaseutils.h"

#include <coreplugin/icontext.h>
#include <coreplugin/progressmanager/progressmanager.h>
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
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

using namespace ProjectExplorer;

namespace CompilationDatabaseProjectManager {
namespace Internal {

namespace {

QStringList jsonObjectFlags(const QJsonObject &object, QSet<QString> &flagsCache)
{
    QStringList flags;
    const QJsonArray arguments = object["arguments"].toArray();
    if (arguments.isEmpty()) {
        flags = splitCommandLine(object["command"].toString(), flagsCache);
    } else {
        flags.reserve(arguments.size());
        for (const QJsonValue &arg : arguments) {
            auto flagIt = flagsCache.insert(arg.toString());
            flags.append(*flagIt);
        }
    }

    return flags;
}

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
    const Utils::FileName compiler = Utils::FileName::fromString(compilerPath(flags.front()));
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

Utils::FileName jsonObjectFilename(const QJsonObject &object)
{
    const QString workingDir = QDir::fromNativeSeparators(object["directory"].toString());
    Utils::FileName fileName = Utils::FileName::fromString(
                QDir::fromNativeSeparators(object["file"].toString()));
    if (fileName.toFileInfo().isRelative()) {
        fileName = Utils::FileUtils::canonicalPath(
                    Utils::FileName::fromString(workingDir + "/" + fileName.toString()));
    }
    return fileName;
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

CppTools::RawProjectPart makeRawProjectPart(const Utils::FileName &projectFile,
                                            Kit *kit,
                                            CppTools::KitInfo &kitInfo,
                                            const QString &workingDir,
                                            const Utils::FileName &fileName,
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

QStringList relativeDirsList(Utils::FileName currentPath, const Utils::FileName &rootPath)
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
    const Utils::FileName path = parent->filePath().pathAppended(childName);
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
FolderNode *createFoldersIfNeeded(FolderNode *root, const Utils::FileName &folderPath)
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

void addChild(FolderNode *root, const Utils::FileName &fileName)
{
    FolderNode *parentNode = createFoldersIfNeeded(root, fileName.parentDir());
    if (!parentNode->fileNode(fileName)) {
        parentNode->addNode(
            std::make_unique<FileNode>(fileName, fileTypeForName(fileName.fileName())));
    }
}

void createTree(std::unique_ptr<ProjectNode> &root,
                const Utils::FileName &rootPath,
                const CppTools::RawProjectParts &rpps,
                const QList<FileNode *> &scannedFiles = QList<FileNode *>())
{
    root->setAbsoluteFilePathAndLine(rootPath, -1);
    std::unique_ptr<FolderNode> secondRoot;

    for (const CppTools::RawProjectPart &rpp : rpps) {
        for (const QString &filePath : rpp.files) {
            Utils::FileName fileName = Utils::FileName::fromString(filePath);
            if (!fileName.isChildOf(rootPath)) {
                if (fileName.isChildOf(Utils::FileName::fromString(rpp.buildSystemTarget))) {
                    if (!secondRoot)
                        secondRoot = std::make_unique<ProjectNode>(
                            Utils::FileName::fromString(rpp.buildSystemTarget));
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

        const Utils::FileName fileName = node->filePath();
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

struct Entry
{
    QStringList flags;
    Utils::FileName fileName;
    QString workingDir;
};

std::vector<Entry> readJsonObjects(const QString &filePath)
{
    std::vector<Entry> result;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return result;

    const QByteArray contents = file.readAll();
    int objectStart = contents.indexOf('{');
    int objectEnd = contents.indexOf('}', objectStart + 1);

    QSet<QString> flagsCache;
    while (objectStart >= 0 && objectEnd >= 0) {
        const QJsonDocument document = QJsonDocument::fromJson(
                    contents.mid(objectStart, objectEnd - objectStart + 1));
        if (document.isNull()) {
            // The end was found incorrectly, search for the next one.
            objectEnd = contents.indexOf('}', objectEnd + 1);
            continue;
        }

        const QJsonObject object = document.object();
        const Utils::FileName fileName = jsonObjectFilename(object);
        const QStringList flags = filterFromFileName(jsonObjectFlags(object, flagsCache),
                                                     fileName.toFileInfo().baseName());
        result.push_back({flags, fileName, object["directory"].toString()});

        objectStart = contents.indexOf('{', objectEnd + 1);
        objectEnd = contents.indexOf('}', objectStart + 1);
    }

    return result;
}

QStringList readExtraFiles(const QString &filePath)
{
    QStringList result;

    QFile file(filePath);
    if (file.open(QFile::ReadOnly)) {
        QTextStream stream(&file);

        while (!stream.atEnd()) {
            QString line = stream.readLine();
            line = line.trimmed();

            if (line.isEmpty() || line.startsWith('#'))
                continue;

            result.push_back(line);
        }
    }

    return result;
}

} // anonymous namespace

void CompilationDatabaseProject::buildTreeAndProjectParts(const Utils::FileName &projectFile)
{
    std::vector<Entry> array = readJsonObjects(projectFilePath().toString());
    const QString jsonExtraFilename = projectFilePath().toString() +
                                      Constants::COMPILATIONDATABASEPROJECT_FILES_SUFFIX;
    const QStringList &extras = readExtraFiles(jsonExtraFilename);

    if (array.empty() && extras.empty()) {
        emitParsingFinished(false);
        return;
    }

    auto root = std::make_unique<ProjectNode>(projectDirectory());

    CppTools::KitInfo kitInfo(this);
    QTC_ASSERT(kitInfo.isValid(), return);
    // Reset toolchains to pick them based on the database entries.
    kitInfo.cToolChain = nullptr;
    kitInfo.cxxToolChain = nullptr;
    CppTools::RawProjectParts rpps;

    std::sort(array.begin(), array.end(), [](const Entry &lhs, const Entry &rhs) {
        return std::lexicographical_compare(lhs.flags.begin(), lhs.flags.end(),
                                            rhs.flags.begin(), rhs.flags.end());
    });

    const Entry *prevEntry = nullptr;
    for (const Entry &entry : array) {
        if (prevEntry && prevEntry->flags == entry.flags) {
            rpps.back().files.append(entry.fileName.toString());
            continue;
        }

        prevEntry = &entry;

        CppTools::RawProjectPart rpp = makeRawProjectPart(projectFile,
                                                          m_kit.get(),
                                                          kitInfo,
                                                          entry.workingDir,
                                                          entry.fileName,
                                                          entry.flags);

        rpps.append(rpp);
    }

    if (!extras.empty()) {
        const Utils::FileName baseDir = projectFile.parentDir();

        QStringList extraFiles;
        for (const QString &extra : extras)
            extraFiles.append(baseDir.pathAppended(extra).toString());

        CppTools::RawProjectPart rppExtra;
        rppExtra.setFiles(extraFiles);
        rpps.append(rppExtra);
    }

    m_treeScanner.future().waitForFinished();
    QCoreApplication::processEvents();

    if (m_treeScanner.future().isCanceled())
        createTree(root, rootProjectDirectory(), rpps);
    else
        createTree(root, rootProjectDirectory(), rpps, m_treeScanner.release());

    root->addNode(std::make_unique<FileNode>(projectFile, FileType::Project));

    if (QFile::exists(jsonExtraFilename))
        root->addNode(std::make_unique<FileNode>(Utils::FileName::fromString(jsonExtraFilename),
                                                 FileType::Project));

    setRootProjectNode(std::move(root));

    m_cppCodeModelUpdater->update({this, kitInfo, rpps});

    emitParsingFinished(true);
}

CompilationDatabaseProject::CompilationDatabaseProject(const Utils::FileName &projectFile)
    : Project(Constants::COMPILATIONDATABASEMIMETYPE, projectFile)
    , m_cppCodeModelUpdater(std::make_unique<CppTools::CppProjectUpdater>())
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

    m_treeScanner.setFilter([this](const Utils::MimeType &mimeType, const Utils::FileName &fn) {
        // Mime checks requires more resources, so keep it last in check list
        auto isIgnored = fn.toString().startsWith(projectFilePath().toString() + ".user")
                         || TreeScanner::isWellKnownBinary(mimeType, fn);

        // Cache mime check result for speed up
        if (!isIgnored) {
            auto it = m_mimeBinaryCache.find(mimeType.name());
            if (it != m_mimeBinaryCache.end()) {
                isIgnored = *it;
            } else {
                isIgnored = TreeScanner::isMimeBinary(mimeType, fn);
                m_mimeBinaryCache[mimeType.name()] = isIgnored;
            }
        }

        return isIgnored;
    });
    m_treeScanner.setTypeFactory([](const Utils::MimeType &mimeType, const Utils::FileName &fn) {
        return TreeScanner::genericFileType(mimeType, fn);
    });

    connect(this,
            &CompilationDatabaseProject::rootProjectDirectoryChanged,
            this,
            &CompilationDatabaseProject::reparseProject);

    m_fileSystemWatcher.addFile(projectFile.toString(), Utils::FileSystemWatcher::WatchModifiedDate);
    m_fileSystemWatcher.addFile(projectFile.toString() + Constants::COMPILATIONDATABASEPROJECT_FILES_SUFFIX,
                                Utils::FileSystemWatcher::WatchModifiedDate);
    connect(&m_fileSystemWatcher,
            &Utils::FileSystemWatcher::fileChanged,
            this,
            &CompilationDatabaseProject::reparseProject);
}

Utils::FileName CompilationDatabaseProject::rootPathFromSettings() const
{
#ifdef WITH_TESTS
    return Utils::FileName::fromString(projectDirectory().fileName());
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
        const Utils::FileName rootPath = rootPathFromSettings();
        if (rootPath.isEmpty())
            changeRootProjectDirectory(); // This triggers reparse itself.
        else
            reparseProject();
    }

    return result;
}

void CompilationDatabaseProject::reparseProject()
{
    emitParsingStarted();

    const Utils::FileName rootPath = rootPathFromSettings();
    if (!rootPath.isEmpty()) {
        m_treeScanner.asyncScanForFiles(rootPath);

        Core::ProgressManager::addTask(m_treeScanner.future(),
                                       tr("Scan \"%1\" project tree").arg(displayName()),
                                       "CompilationDatabase.Scan.Tree");
    }

    const QFuture<void> future = ::Utils::runAsync(
        [this]() { buildTreeAndProjectParts(projectFilePath()); });
    Core::ProgressManager::addTask(future,
                                   tr("Parse \"%1\" project").arg(displayName()),
                                   "CompilationDatabase.Parse");
    m_parserWatcher.setFuture(future);
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
