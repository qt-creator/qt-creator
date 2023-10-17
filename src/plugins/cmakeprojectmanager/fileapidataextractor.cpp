// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fileapidataextractor.h"

#include "cmakeprojectconstants.h"
#include "cmakeprojectmanagertr.h"
#include "cmakespecificsettings.h"
#include "fileapiparser.h"
#include "projecttreehelper.h"

#include <cppeditor/cppeditorconstants.h>

#include <projectexplorer/projecttree.h>

#include <utils/algorithm.h>
#include <utils/icon.h>
#include <utils/mimeutils.h>
#include <utils/process.h>
#include <utils/qtcassert.h>

#include <QLoggingCategory>
#include <QtConcurrent>

using namespace ProjectExplorer;
using namespace Utils;
using namespace CMakeProjectManager::Internal::FileApiDetails;

namespace CMakeProjectManager::Internal {

static Q_LOGGING_CATEGORY(cmakeLogger, "qtc.cmake.fileApiExtractor", QtWarningMsg);

// --------------------------------------------------------------------
// Helpers:
// --------------------------------------------------------------------

class CMakeFileResult
{
public:
    QSet<CMakeFileInfo> cmakeFiles;

    std::vector<std::unique_ptr<FileNode>> cmakeNodesSource;
    std::vector<std::unique_ptr<FileNode>> cmakeNodesBuild;
    std::vector<std::unique_ptr<FileNode>> cmakeNodesOther;
    std::vector<std::unique_ptr<FileNode>> cmakeListNodes;
};

static CMakeFileResult extractCMakeFilesData(const QFuture<void> &cancelFuture,
                                             const std::vector<CMakeFileInfo> &cmakefiles,
                                             const FilePath &sourceDirectory,
                                             const FilePath &buildDirectory)
{
    if (cmakefiles.empty())
        return {};

    // Uniquify fileInfos
    std::set<CMakeFileInfo> cmakeFileSet{cmakefiles.begin(), cmakefiles.end()};

    // Load and parse cmake files. We use concurrency here to speed up the process of
    // reading many small files, which can get slow especially on remote devices.
    QFuture<CMakeFileInfo> mapResult
        = QtConcurrent::mapped(cmakeFileSet, [cancelFuture, sourceDirectory](const auto &info) {
              if (cancelFuture.isCanceled())
                  return CMakeFileInfo();
              const FilePath sfn = sourceDirectory.resolvePath(info.path);
              CMakeFileInfo absolute(info);
              absolute.path = sfn;

              const auto mimeType = Utils::mimeTypeForFile(info.path);
              if (mimeType.matchesName(Constants::CMAKE_MIMETYPE)
                  || mimeType.matchesName(Constants::CMAKE_PROJECT_MIMETYPE)) {
                  expected_str<QByteArray> fileContent = sfn.fileContents();
                  std::string errorString;
                  if (fileContent) {
                      fileContent = fileContent->replace("\r\n", "\n");
                      if (!absolute.cmakeListFile.ParseString(fileContent->toStdString(),
                                                              sfn.fileName().toStdString(),
                                                              errorString)) {
                          qCWarning(cmakeLogger) << "Failed to parse:" << sfn.path()
                                                 << QString::fromLatin1(errorString);
                      }
                  }
              }

              return absolute;
          });

    mapResult.waitForFinished();

    if (cancelFuture.isCanceled())
        return {};

    CMakeFileResult result;

    for (const auto &info : mapResult.results()) {
        if (cancelFuture.isCanceled())
            return {};

        result.cmakeFiles.insert(info);

        if (info.isCMake && !info.isCMakeListsDotTxt) {
            // Skip files that cmake considers to be part of the installation -- but include
            // CMakeLists.txt files. This fixes cmake binaries running from their own
            // build directory.
            continue;
        }

        auto node = std::make_unique<FileNode>(info.path, FileType::Project);
        node->setIsGenerated(info.isGenerated
                             && !info.isCMakeListsDotTxt); // CMakeLists.txt are never
                                                           // generated, independent
                                                           // what cmake thinks:-)

        if (info.isCMakeListsDotTxt) {
            result.cmakeListNodes.emplace_back(std::move(node));
        } else if (info.path.isChildOf(sourceDirectory)) {
            result.cmakeNodesSource.emplace_back(std::move(node));
        } else if (info.path.isChildOf(buildDirectory)) {
            result.cmakeNodesBuild.emplace_back(std::move(node));
        } else {
            result.cmakeNodesOther.emplace_back(std::move(node));
        }
    }

    return result;
}

class PreprocessedData
{
public:
    CMakeProjectManager::CMakeConfig cache;

    QSet<CMakeFileInfo> cmakeFiles;

    std::vector<std::unique_ptr<FileNode>> cmakeNodesSource;
    std::vector<std::unique_ptr<FileNode>> cmakeNodesBuild;
    std::vector<std::unique_ptr<FileNode>> cmakeNodesOther;
    std::vector<std::unique_ptr<FileNode>> cmakeListNodes;

    Configuration codemodel;
    std::vector<TargetDetails> targetDetails;
};

static PreprocessedData preprocess(const QFuture<void> &cancelFuture, FileApiData &data,
                                   const FilePath &sourceDirectory, const FilePath &buildDirectory)
{
    PreprocessedData result;

    result.cache = std::move(data.cache); // Make sure this is available, even when nothing else is

    result.codemodel = std::move(data.codemodel);

    CMakeFileResult cmakeFileResult = extractCMakeFilesData(cancelFuture, data.cmakeFiles,
                                                            sourceDirectory, buildDirectory);

    result.cmakeFiles = std::move(cmakeFileResult.cmakeFiles);
    result.cmakeNodesSource = std::move(cmakeFileResult.cmakeNodesSource);
    result.cmakeNodesBuild = std::move(cmakeFileResult.cmakeNodesBuild);
    result.cmakeNodesOther = std::move(cmakeFileResult.cmakeNodesOther);
    result.cmakeListNodes = std::move(cmakeFileResult.cmakeListNodes);

    result.targetDetails = std::move(data.targetDetails);

    return result;
}

static QVector<FolderNode::LocationInfo> extractBacktraceInformation(
    const BacktraceInfo &backtraces,
    const FilePath &sourceDir,
    int backtraceIndex,
    unsigned int locationInfoPriority)
{
    QVector<FolderNode::LocationInfo> info;
    // Set up a default target path:
    while (backtraceIndex != -1) {
        const size_t bi = static_cast<size_t>(backtraceIndex);
        QTC_ASSERT(bi < backtraces.nodes.size(), break);
        const BacktraceNode &btNode = backtraces.nodes[bi];
        backtraceIndex = btNode.parent; // advance to next node

        const size_t fileIndex = static_cast<size_t>(btNode.file);
        QTC_ASSERT(fileIndex < backtraces.files.size(), break);
        const FilePath path = sourceDir.pathAppended(backtraces.files[fileIndex]).absoluteFilePath();

        if (btNode.command < 0) {
            // No command, skip: The file itself is already covered:-)
            continue;
        }

        const size_t commandIndex = static_cast<size_t>(btNode.command);
        QTC_ASSERT(commandIndex < backtraces.commands.size(), break);

        const QString command = backtraces.commands[commandIndex];

        info.append(FolderNode::LocationInfo(command, path, btNode.line, locationInfoPriority));
    }
    return info;
}

static bool isChildOf(const FilePath &path, const FilePaths &prefixes)
{
    for (const FilePath &prefix : prefixes)
        if (path == prefix || path.isChildOf(prefix))
            return true;
    return false;
}

static CMakeBuildTarget toBuildTarget(const TargetDetails &t,
                                      const FilePath &sourceDirectory,
                                      const FilePath &buildDirectory,
                                      bool relativeLibs)
{
    const FilePath currentBuildDir = buildDirectory.resolvePath(t.buildDir);

    CMakeBuildTarget ct;
    ct.title = t.name;
    if (!t.artifacts.isEmpty())
        ct.executable = buildDirectory.resolvePath(t.artifacts.at(0));
    TargetType type = UtilityType;
    if (t.type == "EXECUTABLE")
        type = ExecutableType;
    else if (t.type == "STATIC_LIBRARY")
        type = StaticLibraryType;
    else if (t.type == "OBJECT_LIBRARY")
        type = ObjectLibraryType;
    else if (t.type == "MODULE_LIBRARY" || t.type == "SHARED_LIBRARY")
        type = DynamicLibraryType;
    else
        type = UtilityType;
    ct.targetType = type;
    ct.workingDirectory = ct.executable.isEmpty()
                              ? currentBuildDir.absolutePath()
                              : ct.executable.parentDir();
    ct.sourceDirectory = sourceDirectory.resolvePath(t.sourceDir);

    ct.backtrace = extractBacktraceInformation(t.backtraceGraph, sourceDirectory, t.backtrace, 0);

    for (const DependencyInfo &d : t.dependencies) {
        ct.dependencyDefinitions.append(
            extractBacktraceInformation(t.backtraceGraph, sourceDirectory, d.backtrace, 100));
    }
    for (const SourceInfo &si : t.sources) {
        ct.sourceDefinitions.append(
            extractBacktraceInformation(t.backtraceGraph, sourceDirectory, si.backtrace, 200));
    }
    for (const CompileInfo &ci : t.compileGroups) {
        for (const IncludeInfo &ii : ci.includes) {
            ct.includeDefinitions.append(
                extractBacktraceInformation(t.backtraceGraph, sourceDirectory, ii.backtrace, 300));
        }
        for (const DefineInfo &di : ci.defines) {
            ct.defineDefinitions.append(
                extractBacktraceInformation(t.backtraceGraph, sourceDirectory, di.backtrace, 400));
        }
    }
    for (const InstallDestination &id : t.installDestination) {
        ct.installDefinitions.append(
            extractBacktraceInformation(t.backtraceGraph, sourceDirectory, id.backtrace, 500));
    }

    if (ct.targetType == ExecutableType) {
        FilePaths librarySeachPaths;
        // Is this a GUI application?
        ct.linksToQtGui = Utils::contains(t.link.value().fragments,
                                          [](const FragmentInfo &f) {
                                              return f.role == "libraries"
                                                     && (f.fragment.contains("QtGui")
                                                         || f.fragment.contains("Qt5Gui")
                                                         || f.fragment.contains("Qt6Gui"));
                                          });

        ct.qtcRunnable = t.folderTargetProperty == "qtc_runnable";

        // Extract library directories for executables:
        for (const FragmentInfo &f : t.link.value().fragments) {
            if (f.role == "flags") // ignore all flags fragments
                continue;

            // CMake sometimes mixes several shell-escaped pieces into one fragment. Disentangle that again:
            const QStringList parts = ProcessArgs::splitArgs(f.fragment, HostOsInfo::hostOs());
            for (QString part : parts) {
                // Library search paths that are added with target_link_directories are added as
                // -LIBPATH:... (Windows/MSVC), or
                // -L... (Unix/GCC)
                // with role "libraryPath"
                if (f.role == "libraryPath") {
                    if (part.startsWith("-LIBPATH:"))
                        part = part.mid(9);
                    else if (part.startsWith("-L"))
                        part = part.mid(2);
                }

                // Some projects abuse linking to libraries to pass random flags to the linker, so ignore
                // flags mixed into a fragment
                if (part.startsWith("-"))
                    continue;

                const FilePath buildDir = relativeLibs ? buildDirectory : currentBuildDir;
                FilePath tmp = buildDir.resolvePath(part);

                if (f.role == "libraries")
                    tmp = tmp.parentDir();

                if (!tmp.isEmpty() && tmp.isDir()) {
                    // f.role is libraryPath or frameworkPath
                    // On *nix, exclude sub-paths from "/lib(64)", "/usr/lib(64)" and
                    // "/usr/local/lib" since these are usually in the standard search
                    // paths. There probably are more, but the naming schemes are arbitrary
                    // so we'd need to ask the linker ("ld --verbose | grep SEARCH_DIR").
                    if (buildDir.osType() == OsTypeWindows
                        || !isChildOf(tmp,
                                      {"/lib",
                                       "/lib64",
                                       "/usr/lib",
                                       "/usr/lib64",
                                       "/usr/local/lib"})) {
                        librarySeachPaths.append(tmp);
                        // Libraries often have their import libs in ../lib and the
                        // actual dll files in ../bin on windows. Qt is one example of that.
                        if (tmp.fileName() == "lib" && buildDir.osType() == OsTypeWindows) {
                            const FilePath path = tmp.parentDir().pathAppended("bin");
                            if (path.isDir())
                                librarySeachPaths.append(path);
                        }
                    }
                }
            }
        }
        ct.libraryDirectories = filteredUnique(librarySeachPaths);
    }
    return ct;
}

static QList<CMakeBuildTarget> generateBuildTargets(const QFuture<void> &cancelFuture,
                                                    const PreprocessedData &input,
                                                    const FilePath &sourceDirectory,
                                                    const FilePath &buildDirectory,
                                                    bool relativeLibs)
{
    QList<CMakeBuildTarget> result;
    result.reserve(input.targetDetails.size());
    for (const TargetDetails &t : input.targetDetails) {
        if (cancelFuture.isCanceled())
            return {};
        result.append(toBuildTarget(t, sourceDirectory, buildDirectory, relativeLibs));
    }
    return result;
}

static QStringList splitFragments(const QStringList &fragments)
{
    QStringList result;
    for (const QString &f : fragments) {
        result += ProcessArgs::splitArgs(f, HostOsInfo::hostOs());
    }
    return result;
}

static bool isPchFile(const FilePath &buildDirectory, const FilePath &path)
{
    return path.fileName().startsWith("cmake_pch") && path.isChildOf(buildDirectory);
}

static bool isUnityFile(const FilePath &buildDirectory, const FilePath &path)
{
    return path.fileName().startsWith("unity_") && path.isChildOf(buildDirectory)
           && path.parentDir().fileName() == "Unity";
}

static RawProjectParts generateRawProjectParts(const QFuture<void> &cancelFuture,
                                               const PreprocessedData &input,
                                               const FilePath &sourceDirectory,
                                               const FilePath &buildDirectory)
{
    RawProjectParts rpps;

    for (const TargetDetails &t : input.targetDetails) {
        if (cancelFuture.isCanceled())
            return {};

        bool needPostfix = t.compileGroups.size() > 1;
        int count = 1;
        for (const CompileInfo &ci : t.compileGroups) {
            if (ci.language != "C" && ci.language != "CXX" && ci.language != "CUDA")
                continue; // No need to bother the C++ codemodel

            // CMake users worked around Creator's inability of listing header files by creating
            // custom targets with all the header files. This target breaks the code model, so
            // keep quiet about it:-)
            if (ci.defines.empty() && ci.includes.empty() && allOf(ci.sources, [t](const int sid) {
                    const SourceInfo &source = t.sources[static_cast<size_t>(sid)];
                    return Node::fileTypeForFileName(FilePath::fromString(source.path))
                           == FileType::Header;
                })) {
                qWarning() << "Not reporting all-header compilegroup of target" << t.name
                           << "to code model.";
                continue;
            }

            QString ending;
            QString qtcPchFile;
            if (ci.language == "C") {
                ending = "/cmake_pch.h";
                qtcPchFile = "qtc_cmake_pch.h";
            }
            else if (ci.language == "CXX") {
                ending = "/cmake_pch.hxx";
                qtcPchFile = "qtc_cmake_pch.hxx";
            }

            RawProjectPart rpp;
            rpp.setProjectFileLocation(t.sourceDir.pathAppended("CMakeLists.txt").toString());
            rpp.setBuildSystemTarget(t.name);
            const QString postfix = needPostfix ? QString("_%1_%2").arg(ci.language).arg(count)
                                                : QString();
            rpp.setDisplayName(t.id + postfix);
            rpp.setMacros(transform<QVector>(ci.defines, &DefineInfo::define));
            rpp.setHeaderPaths(transform<QVector>(ci.includes, &IncludeInfo::path));

            QStringList fragments = splitFragments(ci.fragments);

            // Get all sources from the compiler group, except generated sources
            FilePaths sources;

            for (auto idx: ci.sources) {
                SourceInfo si = t.sources.at(idx);
                if (si.isGenerated)
                    continue;
                sources.append(sourceDirectory.resolvePath(si.path));
            }

            // Skip groups with only generated source files e.g. <build-dir>/.rcc/qrc_<target>.cpp
            if (allOf(ci.sources, [t](const auto &idx) { return t.sources.at(idx).isGenerated; }))
                continue;

            // If we are not in a pch compiler group, add all the headers that are not generated
            const bool hasPchSource = anyOf(sources, [buildDirectory](const FilePath &path) {
                return isPchFile(buildDirectory, path);
            });

            const bool hasUnitySources = allOf(sources, [buildDirectory](const FilePath &path) {
                return isUnityFile(buildDirectory, path);
            });

            QString headerMimeType;
            QString sourceMimeType;
            if (ci.language == "C") {
                headerMimeType = CppEditor::Constants::C_HEADER_MIMETYPE;
                sourceMimeType = CppEditor::Constants::C_SOURCE_MIMETYPE;
            } else if (ci.language == "CXX") {
                headerMimeType = CppEditor::Constants::CPP_HEADER_MIMETYPE;
                sourceMimeType = CppEditor::Constants::CPP_SOURCE_MIMETYPE;
            }
            if (!hasPchSource) {
                for (const SourceInfo &si : t.sources) {
                    if (si.isGenerated)
                        continue;
                    const auto mimeTypes = Utils::mimeTypesForFileName(si.path);
                    for (const auto &mime : mimeTypes) {
                        const bool headerType = mime.inherits(headerMimeType);
                        const bool sourceUnityType = hasUnitySources ? mime.inherits(sourceMimeType)
                                                                     : false;
                        if (headerType || sourceUnityType)
                            sources.append(sourceDirectory.resolvePath(si.path));
                    }
                }
            }
            FilePath::removeDuplicates(sources);

            // Set project files except pch / unity files
            const FilePaths filtered = Utils::filtered(sources,
                                         [buildDirectory](const FilePath &filePath) {
                                             return !isPchFile(buildDirectory, filePath)
                                                 && !isUnityFile(buildDirectory, filePath);
                                         });

            rpp.setFiles(Utils::transform(filtered, &FilePath::toFSPathString),
                         {},
                         [headerMimeType](const QString &path) {
                             // Similar to ProjectFile::classify but classify headers with language
                             // of compile group instead of ambiguous header
                             if (path.endsWith(".h"))
                                 return headerMimeType;
                             return Utils::mimeTypeForFile(path).name();
                         });

            FilePath precompiled_header
                = FilePath::fromString(findOrDefault(t.sources, [&ending](const SourceInfo &si) {
                                           return si.path.endsWith(ending);
                                       }).path);
            if (!precompiled_header.isEmpty()) {
                precompiled_header = sourceDirectory.resolvePath(precompiled_header);

                // Remove the CMake PCH usage command line options in order to avoid the case
                // when the build system would produce a .pch/.gch file that would be treated
                // by the Clang code model as its own and fail.
                auto remove = [&](const QStringList &args) {
                    auto foundPos = std::search(fragments.begin(), fragments.end(),
                                                args.begin(), args.end());
                    if (foundPos != fragments.end())
                        fragments.erase(foundPos, std::next(foundPos, args.size()));
                };

                remove({"-Xclang", "-include-pch", "-Xclang", precompiled_header.path() + ".gch"});
                remove({"-Xclang", "-include-pch", "-Xclang", precompiled_header.path() + ".pch"});
                remove({"-Xclang", "-include", "-Xclang", precompiled_header.path()});
                remove({"-include", precompiled_header.path()});
                remove({"/FI", precompiled_header.path()});

                // Make a copy of the CMake PCH header and use it instead
                FilePath qtc_precompiled_header = precompiled_header.parentDir().pathAppended(qtcPchFile);
                FileUtils::copyIfDifferent(precompiled_header, qtc_precompiled_header);

                rpp.setPreCompiledHeaders({qtc_precompiled_header.path()});
            }

            RawProjectPartFlags cProjectFlags;
            cProjectFlags.commandLineFlags = fragments;
            rpp.setFlagsForC(cProjectFlags);

            RawProjectPartFlags cxxProjectFlags;
            cxxProjectFlags.commandLineFlags = cProjectFlags.commandLineFlags;
            rpp.setFlagsForCxx(cxxProjectFlags);

            const bool isExecutable = t.type == "EXECUTABLE";
            rpp.setBuildTargetType(isExecutable ? BuildTargetType::Executable
                                                : BuildTargetType::Library);
            rpps.append(rpp);
            ++count;
        }
    }

    return rpps;
}

static FilePath directorySourceDir(const Configuration &c,
                                   const FilePath &sourceDir,
                                   int directoryIndex)
{
    const size_t di = static_cast<size_t>(directoryIndex);
    QTC_ASSERT(di < c.directories.size(), return {});

    return sourceDir.resolvePath(c.directories[di].sourcePath);
}

static FilePath directoryBuildDir(const Configuration &c,
                                  const FilePath &buildDir,
                                  int directoryIndex)
{
    const size_t di = static_cast<size_t>(directoryIndex);
    QTC_ASSERT(di < c.directories.size(), return {});

    return buildDir.resolvePath(c.directories[di].buildPath);
}

static void addProjects(const QFuture<void> &cancelFuture,
                        const QHash<FilePath, ProjectNode *> &cmakeListsNodes,
                        const Configuration &config,
                        const FilePath &sourceDir)
{
    for (const FileApiDetails::Project &p : config.projects) {
        if (cancelFuture.isCanceled())
            return;

        if (p.parent == -1)
            continue; // Top-level project has already been covered
        FilePath dir = directorySourceDir(config, sourceDir, p.directories[0]);
        createProjectNode(cmakeListsNodes, dir, p.name);
    }
}

static FolderNode *createSourceGroupNode(const QString &sourceGroupName,
                                         const FilePath &sourceDirectory,
                                         FolderNode *targetRoot)
{
    FolderNode *currentNode = targetRoot;

    if (!sourceGroupName.isEmpty()) {
        const QStringList parts = sourceGroupName.split("\\");

        for (const QString &p : parts) {
            FolderNode *existingNode = currentNode->findChildFolderNode(
                [&p](const FolderNode *fn) { return fn->displayName() == p; });
            if (!existingNode) {
                auto node = createCMakeVFolder(sourceDirectory, Node::DefaultFolderPriority + 5, p);
                node->setListInProject(false);
                node->setIcon([] { return Icon::fromTheme("edit-copy"); });

                existingNode = node.get();

                currentNode->addNode(std::move(node));
            }

            currentNode = existingNode;
        }
    }
    return currentNode;
}

static void addCompileGroups(ProjectNode *targetRoot,
                             const FilePath &topSourceDirectory,
                             const FilePath &sourceDirectory,
                             const FilePath &buildDirectory,
                             const TargetDetails &td)
{
    const bool showSourceFolders = settings().showSourceSubFolders();
    const bool inSourceBuild = (sourceDirectory == buildDirectory);

    QSet<FilePath> alreadyListed;

    // Files already added by other configurations:
    targetRoot->forEachGenericNode(
        [&alreadyListed](const Node *n) { alreadyListed.insert(n->filePath()); });

    std::vector<std::unique_ptr<FileNode>> buildFileNodes;
    std::vector<std::unique_ptr<FileNode>> otherFileNodes;
    std::vector<std::vector<std::unique_ptr<FileNode>>> sourceGroupFileNodes{td.sourceGroups.size()};

    for (const SourceInfo &si : td.sources) {
        const FilePath sourcePath = topSourceDirectory.resolvePath(si.path);

        // Filter out already known files:
        const int count = alreadyListed.count();
        alreadyListed.insert(sourcePath);
        if (count == alreadyListed.count())
            continue;

        // Create FileNodes from the file
        auto node = std::make_unique<FileNode>(sourcePath, Node::fileTypeForFileName(sourcePath));
        node->setIsGenerated(si.isGenerated);

        // CMake pch / unity files are generated at configured time, but not marked as generated
        // so that a "clean" step won't remove them and at a subsequent build they won't exist.
        if (isPchFile(buildDirectory, sourcePath) || isUnityFile(buildDirectory, sourcePath))
            node->setIsGenerated(true);

        // Where does the file node need to go?
        if (showSourceFolders && sourcePath.isChildOf(buildDirectory) && !inSourceBuild) {
            buildFileNodes.emplace_back(std::move(node));
        } else if (!showSourceFolders || sourcePath.isChildOf(sourceDirectory)) {
            sourceGroupFileNodes[si.sourceGroup].emplace_back(std::move(node));
        } else {
            otherFileNodes.emplace_back(std::move(node));
        }
    }

    // Calculate base directory for source groups:
    for (size_t i = 0; i < sourceGroupFileNodes.size(); ++i) {
        std::vector<std::unique_ptr<FileNode>> &current = sourceGroupFileNodes[i];
        FilePath baseDirectory;
        // All the sourceGroupFileNodes are below sourceDirectory, so this is safe:
        for (const std::unique_ptr<FileNode> &fn : current) {
            if (baseDirectory.isEmpty()) {
                baseDirectory = fn->filePath().parentDir();
            } else {
                baseDirectory = FileUtils::commonPath(baseDirectory, fn->filePath());
            }
        }

        FolderNode *insertNode = createSourceGroupNode(td.sourceGroups[i],
                                                       baseDirectory,
                                                       targetRoot);

        if (showSourceFolders) {
            insertNode->addNestedNodes(std::move(current), baseDirectory);
        } else {
            for (auto &fileNodes : current) {
                insertNode->addNode(std::move(fileNodes));
            }
        }
    }

    addCMakeVFolder(targetRoot,
                    buildDirectory,
                    100,
                    Tr::tr("<Build Directory>"),
                    std::move(buildFileNodes));
    addCMakeVFolder(targetRoot,
                    FilePath(),
                    10,
                    Tr::tr("<Other Locations>"),
                    std::move(otherFileNodes));
}

static void addGeneratedFilesNode(ProjectNode *targetRoot, const FilePath &topLevelBuildDir,
                                  const TargetDetails &td)
{
    if (td.artifacts.isEmpty())
        return;
    FileType type = FileType::Unknown;
    if (td.type == "EXECUTABLE")
        type = FileType::App;
    else if (td.type == "SHARED_LIBRARY" || td.type == "STATIC_LIBRARY")
        type = FileType::Lib;
    if (type == FileType::Unknown)
        return;
    std::vector<std::unique_ptr<FileNode>> nodes;
    const FilePath buildDir = topLevelBuildDir.resolvePath(td.buildDir);
    for (const FilePath &artifact : td.artifacts) {
        nodes.emplace_back(new FileNode(buildDir.resolvePath(artifact), type));
        type = FileType::Unknown;
        nodes.back()->setIsGenerated(true);
    }
    addCMakeVFolder(targetRoot, buildDir, 10, Tr::tr("<Generated Files>"), std::move(nodes));
}

static void addTargets(const QFuture<void> &cancelFuture,
                       const QHash<FilePath, ProjectNode *> &cmakeListsNodes,
                       const Configuration &config,
                       const std::vector<TargetDetails> &targetDetails,
                       const FilePath &sourceDir,
                       const FilePath &buildDir)
{
    QHash<QString, const TargetDetails *> targetDetailsHash;
    for (const TargetDetails &t : targetDetails)
        targetDetailsHash.insert(t.id, &t);
    const TargetDetails defaultTargetDetails;
    auto getTargetDetails = [&targetDetailsHash, &defaultTargetDetails](const QString &id)
            -> const TargetDetails & {
        auto it = targetDetailsHash.constFind(id);
        if (it != targetDetailsHash.constEnd())
            return *it.value();
        return defaultTargetDetails;
    };

    for (const FileApiDetails::Target &t : config.targets) {
        if (cancelFuture.isCanceled())
            return;

        const TargetDetails &td = getTargetDetails(t.id);

        const FilePath dir = directorySourceDir(config, sourceDir, t.directory);

        CMakeTargetNode *tNode = createTargetNode(cmakeListsNodes, dir, t.name);
        QTC_ASSERT(tNode, continue);

        tNode->setTargetInformation(td.artifacts, td.type);
        tNode->setBuildDirectory(directoryBuildDir(config, buildDir, t.directory));

        addCompileGroups(tNode, sourceDir, dir, tNode->buildDirectory(), td);
        addGeneratedFilesNode(tNode, buildDir, td);
    }
}

static std::unique_ptr<CMakeProjectNode> generateRootProjectNode(const QFuture<void> &cancelFuture,
                                                                 PreprocessedData &data,
                                                                 const FilePath &sourceDirectory,
                                                                 const FilePath &buildDirectory)
{
    std::unique_ptr<CMakeProjectNode> result = std::make_unique<CMakeProjectNode>(sourceDirectory);

    const FileApiDetails::Project topLevelProject
        = findOrDefault(data.codemodel.projects, equal(&FileApiDetails::Project::parent, -1));
    if (!topLevelProject.name.isEmpty())
        result->setDisplayName(topLevelProject.name);
    else
        result->setDisplayName(sourceDirectory.fileName());

    QHash<FilePath, ProjectNode *> cmakeListsNodes = addCMakeLists(result.get(),
                                                                   std::move(data.cmakeListNodes));
    data.cmakeListNodes.clear(); // Remove all the nullptr in the vector...

    addProjects(cancelFuture, cmakeListsNodes, data.codemodel, sourceDirectory);
    if (cancelFuture.isCanceled())
        return {};

    addTargets(cancelFuture,
               cmakeListsNodes,
               data.codemodel,
               data.targetDetails,
               sourceDirectory,
               buildDirectory);
    if (cancelFuture.isCanceled())
        return {};

    if (!data.cmakeNodesSource.empty() || !data.cmakeNodesBuild.empty()
        || !data.cmakeNodesOther.empty()) {
        addCMakeInputs(result.get(),
                       sourceDirectory,
                       buildDirectory,
                       std::move(data.cmakeNodesSource),
                       std::move(data.cmakeNodesBuild),
                       std::move(data.cmakeNodesOther));
    }
    if (cancelFuture.isCanceled())
        return {};

    addCMakePresets(result.get(), sourceDirectory);
    if (cancelFuture.isCanceled())
        return {};

    data.cmakeNodesSource.clear(); // Remove all the nullptr in the vector...
    data.cmakeNodesBuild.clear();  // Remove all the nullptr in the vector...
    data.cmakeNodesOther.clear();  // Remove all the nullptr in the vector...

    return result;
}

static void setupLocationInfoForTargets(const QFuture<void> &cancelFuture,
                                        CMakeProjectNode *rootNode,
                                        const QList<CMakeBuildTarget> &targets)
{
    const QSet<QString> titles = Utils::transform<QSet>(targets, &CMakeBuildTarget::title);
    QHash<QString, FolderNode *> buildKeyToNode;
    rootNode->forEachGenericNode([&buildKeyToNode, &titles](Node *node) {
        FolderNode *folderNode = node->asFolderNode();
        const QString &buildKey = node->buildKey();
        if (folderNode && titles.contains(buildKey))
            buildKeyToNode.insert(buildKey, folderNode);
    });
    for (const CMakeBuildTarget &t : targets) {
        if (cancelFuture.isCanceled())
            return;

        FolderNode *folderNode = buildKeyToNode.value(t.title);
        if (folderNode) {
            QSet<std::pair<FilePath, int>> locations;
            auto dedup = [&locations](const Backtrace &bt) {
                QVector<FolderNode::LocationInfo> result;
                for (const FolderNode::LocationInfo &i : bt) {
                    int count = locations.count();
                    locations.insert({i.path, i.line});
                    if (count != locations.count()) {
                        result.append(i);
                    }
                }
                return result;
            };

            QVector<FolderNode::LocationInfo> result = dedup(t.backtrace);
            auto dedupMulti = [&dedup](const Backtraces &bts) {
                QVector<FolderNode::LocationInfo> result;
                for (const Backtrace &bt : bts) {
                    result.append(dedup(bt));
                }
                return result;
            };
            result += dedupMulti(t.dependencyDefinitions);
            result += dedupMulti(t.includeDefinitions);
            result += dedupMulti(t.defineDefinitions);
            result += dedupMulti(t.sourceDefinitions);
            result += dedupMulti(t.installDefinitions);

            folderNode->setLocationInfo(result);
        }
    }
}

// --------------------------------------------------------------------
// extractData:
// --------------------------------------------------------------------

FileApiQtcData extractData(const QFuture<void> &cancelFuture, FileApiData &input,
                           const FilePath &sourceDir, const FilePath &buildDir)
{
    FileApiQtcData result;

    // Preprocess our input:
    PreprocessedData data = preprocess(cancelFuture, input, sourceDir, buildDir);
    if (cancelFuture.isCanceled())
        return {};

    result.cache = std::move(data.cache); // Make sure this is available, even when nothing else is
    if (!result.errorMessage.isEmpty())
        return {};

    // Ninja generator from CMake version 3.20.5 has libraries relative to build directory
    const bool haveLibrariesRelativeToBuildDirectory =
            input.replyFile.generator.startsWith("Ninja")
         && input.replyFile.cmakeVersion >= QVersionNumber(3, 20, 5);

    result.buildTargets = generateBuildTargets(cancelFuture, data, sourceDir, buildDir,
                                               haveLibrariesRelativeToBuildDirectory);
    if (cancelFuture.isCanceled())
        return {};
    result.cmakeFiles = std::move(data.cmakeFiles);
    result.projectParts = generateRawProjectParts(cancelFuture, data, sourceDir, buildDir);
    if (cancelFuture.isCanceled())
        return {};

    auto rootProjectNode = generateRootProjectNode(cancelFuture, data, sourceDir, buildDir);
    if (cancelFuture.isCanceled())
        return {};
    ProjectTree::applyTreeManager(rootProjectNode.get(), ProjectTree::AsyncPhase); // QRC nodes
    result.rootProjectNode = std::move(rootProjectNode);

    setupLocationInfoForTargets(cancelFuture, result.rootProjectNode.get(), result.buildTargets);
    if (cancelFuture.isCanceled())
        return {};

    result.ctestPath = input.replyFile.ctestExecutable;
    result.isMultiConfig = input.replyFile.isMultiConfig;
    if (input.replyFile.isMultiConfig && input.replyFile.generator != "Ninja Multi-Config")
        result.usesAllCapsTargets = true;

    return result;
}

} // CMakeProjectManager::Internal
