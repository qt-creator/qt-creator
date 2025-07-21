// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fileapidataextractor.h"

#include "cmakeprojectconstants.h"
#include "cmakeprojectmanagertr.h"
#include "cmakespecificsettings.h"
#include "fileapiparser.h"
#include "projecttreehelper.h"

#include <cppeditor/cppeditorconstants.h>
#include <cppeditor/projectinfo.h>

#include <projectexplorer/projecttree.h>

#include <utils/algorithm.h>
#include <utils/icon.h>
#include <utils/fileutils.h>
#include <utils/mimeconstants.h>
#include <utils/mimeutils.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>

#include <QLoggingCategory>
#include <QtConcurrent>

using namespace ProjectExplorer;
using namespace Utils;
using namespace CMakeProjectManager::Internal::FileApiDetails;

namespace CMakeProjectManager::Internal {

static Q_LOGGING_CATEGORY(cmakeLogger, "qtc.cmake.fileApiExtractor", QtWarningMsg);

static const char QTC_RUNNABLE[] = "qtc_runnable";

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
              if (mimeType.matchesName(Utils::Constants::CMAKE_MIMETYPE)
                  || mimeType.matchesName(Utils::Constants::CMAKE_PROJECT_MIMETYPE)) {
                  Result<QByteArray> fileContent = sfn.fileContents();
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
        node->setIsGenerated(info.isGenerated);

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

    ConfigurationInfo codemodel;
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

static QList<FolderNode::LocationInfo> extractBacktraceInformation(
    const BacktraceInfo &backtraces,
    const FilePath &sourceDir,
    int backtraceIndex,
    unsigned int locationInfoPriority)
{
    QList<FolderNode::LocationInfo> info;
    // Set up a default target path:
    while (backtraceIndex != -1) {
        const size_t bi = static_cast<size_t>(backtraceIndex);
        QTC_ASSERT(bi < backtraces.nodes.size(), break);
        const BacktraceNode &btNode = backtraces.nodes[bi];
        backtraceIndex = btNode.parent; // advance to next node

        const size_t fileIndex = static_cast<size_t>(btNode.file);
        QTC_ASSERT(fileIndex < backtraces.files.size(), break);
        const FilePath path = sourceDir.resolvePath(backtraces.files[fileIndex]);
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

    for (const SourceInfo &si : t.sources) {
        ct.sourceFiles.append(sourceDirectory.resolvePath(si.path));
    }

    // FIXME: remove the usage of "qtc_runnable" by parsing the CMake code instead
    ct.qtcRunnable = t.folderTargetProperty == QTC_RUNNABLE;
    ct.targetFolder = t.folderTargetProperty;

    if (ct.targetType == ExecutableType) {
        FilePaths librarySeachPaths;
        // Is this a GUI application?
        ct.linksToQtGui = Utils::contains(t.link->fragments, [](const FragmentInfo &f) {
            return f.role == "libraries"
                   && (f.fragment.contains("QtGui") || f.fragment.contains("Qt5Gui")
                       || f.fragment.contains("Qt6Gui"));
        });

        // Extract library directories for executables:
        for (const FragmentInfo &f : t.link->fragments) {
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

                if (buildDir.osType() == OsTypeWindows && (f.role == "libraries")) {
                    const auto partAsFilePath = FilePath::fromUserInput(part);
                    part = partAsFilePath.fileName();

                    // Skip object libraries on Windows. This case can happen with static qml plugins
                    if (part.endsWith(".obj") || part.endsWith(".o"))
                        continue;
                }

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
        qCInfo(cmakeLogger) << "libraryDirectories for target" << ct.title << ":" << ct.libraryDirectories;

        // If there are start programs, there should also be an option to select none
        if (!t.launcherInfos.isEmpty()) {
            LauncherInfo info { "unused", Utils::FilePath(), QStringList() };
            ct.launchers.append(Launcher(info, sourceDirectory));
        }
        // if there is a test and an emulator launcher, add the emulator and
        // also a combination as the last entry, but not the "test" launcher
        // as it will not work for cross-compiled executables
        if (t.launcherInfos.size() == 2 && t.launcherInfos[0].type == "test" && t.launcherInfos[1].type == "emulator") {
            ct.launchers.append(Launcher(t.launcherInfos[1], sourceDirectory));
            ct.launchers.append(Launcher(t.launcherInfos[0], t.launcherInfos[1], sourceDirectory));
        } else if (t.launcherInfos.size() == 1) {
            Launcher launcher(t.launcherInfos[0], sourceDirectory);
            ct.launchers.append(launcher);
        }
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

        QHash<QString, QPair<int, int>> compileLanguageCountHash;
        for (const CompileInfo &ci : t.compileGroups)
            compileLanguageCountHash[ci.language].first++;

        for (const CompileInfo &ci : t.compileGroups) {
            if (ci.language != "C" && ci.language != "CXX" && ci.language != "OBJC"
                && ci.language != "OBJCXX" && ci.language != "CUDA")
                continue; // No need to bother the C++ codemodel

            // CMake users worked around Creator's inability of listing header files by creating
            // custom targets with all the header files. This target breaks the code model, so
            // keep quiet about it:-)
            if (ci.defines.empty() && ci.includes.empty() && allOf(ci.sources, [&t](const int sid) {
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
            static const QHash<QString, QString> languageToExtension
                = {{"C", ".h"}, {"CXX", ".hxx"}, {"OBJC", ".objc.h"}, {"OBJCXX", ".objcxx.hxx"}};

            if (languageToExtension.contains(ci.language)) {
                ending = "/cmake_pch" + languageToExtension[ci.language];
                qtcPchFile = "qtc_cmake_pch" + languageToExtension[ci.language];
            }

            RawProjectPart rpp;
            rpp.setProjectFileLocation(t.sourceDir.pathAppended(Constants::CMAKE_LISTS_TXT));
            rpp.setBuildSystemTarget(t.name);
            const QString postfix = compileLanguageCountHash[ci.language].first > 1
                                        ? QString("%1_%2")
                                              .arg(ci.language)
                                              .arg(++compileLanguageCountHash[ci.language].second)
                                        : ci.language;
            rpp.setDisplayName(t.name + "_" + postfix);
            rpp.setMacros(transform<QList>(ci.defines, &DefineInfo::define));
            rpp.setHeaderPaths(transform<QList>(ci.includes, &IncludeInfo::path));

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
            if (allOf(ci.sources, [&t](const auto &idx) { return t.sources.at(idx).isGenerated; }))
                continue;

            // If we are not in a pch compiler group, add all the headers that are not generated
            const bool hasPchSource = anyOf(sources, [buildDirectory](const FilePath &path) {
                return isPchFile(buildDirectory, path);
            });

            const bool hasUnitySources = allOf(sources, [buildDirectory](const FilePath &path) {
                return isUnityFile(buildDirectory, path);
            });

            const QString headerMimeType = [&]() -> QString {
                if (ci.language == "C") {
                    return Utils::Constants::C_HEADER_MIMETYPE;
                } else if (ci.language == "CXX") {
                    return Utils::Constants::CPP_HEADER_MIMETYPE;
                } else if (ci.language == "OBJC") {
                    return Utils::Constants::OBJECTIVE_C_SOURCE_MIMETYPE;
                } else if (ci.language == "OBJCXX") {
                    return Utils::Constants::OBJECTIVE_CPP_SOURCE_MIMETYPE;
                }
                return {};
            }();

            auto haveFileKindForLanguage = [&](const auto &kind) {
                if (kind == CppEditor::ProjectFile::AmbiguousHeader)
                    return true;

                if (ci.language == "C" || ci.language == "OBJC")
                    return CppEditor::ProjectFile::isC(kind);
                else if (ci.language == "CXX" || ci.language == "OBJCXX")
                    return CppEditor::ProjectFile::isCxx(kind);

                return false;
            };

            if (!hasPchSource) {
                for (const SourceInfo &si : t.sources) {
                    if (si.isGenerated)
                        continue;

                    const auto kind = CppEditor::ProjectFile::classify(si.path);
                    const bool headerType = CppEditor::ProjectFile::isHeader(kind)
                                            && haveFileKindForLanguage(kind);
                    const bool sourceUnityType = hasUnitySources
                                                     ? CppEditor::ProjectFile::isSource(kind)
                                                           && haveFileKindForLanguage(kind)
                                                     : false;
                    if (headerType || sourceUnityType)
                        sources.append(sourceDirectory.resolvePath(si.path));
                }
            }
            FilePath::removeDuplicates(sources);

            // Set project files except pch / unity files
            const FilePaths filtered = Utils::filtered(sources,
                                         [buildDirectory](const FilePath &filePath) {
                                             return !isPchFile(buildDirectory, filePath)
                                                 && !isUnityFile(buildDirectory, filePath);
                                         });

            rpp.setFiles(filtered,
                         {},
                         [headerMimeType](const FilePath &path) {
                             if (CppEditor::ProjectFile::isAmbiguousHeader(path))
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

                rpp.setPreCompiledHeaders({qtc_precompiled_header});
            }

            RawProjectPartFlags projectFlags;
            projectFlags.commandLineFlags = fragments;
            if (ci.language == "C" || ci.language == "OBJC")
                rpp.setFlagsForC(projectFlags);
            else if (ci.language == "CXX" || ci.language == "OBJCXX")
                rpp.setFlagsForCxx(projectFlags);

            const bool isExecutable = t.type == "EXECUTABLE";
            rpp.setBuildTargetType(isExecutable ? BuildTargetType::Executable
                                                : BuildTargetType::Library);
            rpps.append(rpp);
        }
    }

    return rpps;
}

static FilePath directorySourceDir(const ConfigurationInfo &c,
                                   const FilePath &sourceDir,
                                   int directoryIndex)
{
    const size_t di = static_cast<size_t>(directoryIndex);
    QTC_ASSERT(di < c.directories.size(), return {});

    return sourceDir.resolvePath(c.directories[di].sourcePath);
}

static FilePath directoryBuildDir(const ConfigurationInfo &c,
                                  const FilePath &buildDir,
                                  int directoryIndex)
{
    const size_t di = static_cast<size_t>(directoryIndex);
    QTC_ASSERT(di < c.directories.size(), return {});

    return buildDir.resolvePath(c.directories[di].buildPath);
}

static void addProjects(const QFuture<void> &cancelFuture,
                        const QHash<FilePath, ProjectNode *> &cmakeListsNodes,
                        const ConfigurationInfo &config,
                        const FilePath &sourceDir)
{
    for (const FileApiDetails::ProjectInfo &p : config.projects) {
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
        static const QRegularExpression separators("(\\\\|/)");
        const QStringList parts = sourceGroupName.split(separators);

        for (const QString &p : parts) {
            FolderNode *existingNode = currentNode->findChildFolderNode(
                [&p](const FolderNode *fn) { return fn->displayName() == p; });
            if (!existingNode) {
                auto node = createCMakeVFolder(sourceDirectory, Node::DefaultFolderPriority + 5, p);
                node->setListInProject(false);

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

        // Hide some of the QML files that we not marked as generated by the Qt QML CMake code
        const bool buildDirQmldirOrRcc = sourcePath.isChildOf(buildDirectory)
                                         && (sourcePath.parentDir().fileName() == ".rcc"
                                             || sourcePath.fileName() == "qmldir");
        const bool otherDirQmldirOrMetatypes = !sourcePath.isChildOf(buildDirectory)
                                               && !sourcePath.isChildOf(sourceDirectory)
                                               && (sourcePath.parentDir().fileName() == "metatypes"
                                                   || sourcePath.fileName() == "qmldir");
        const bool buildDirPluginCpp = sourcePath.isChildOf(buildDirectory)
                                       && td.type.endsWith("_LIBRARY")
                                       && sourcePath.fileName().startsWith(td.name)
                                       && sourcePath.fileName().endsWith("Plugin.cpp");
        const bool buildRccInitCpp = sourcePath.isChildOf(buildDirectory)
                                     && td.type.endsWith("_LIBRARY")
                                     && (sourcePath.parentDir().fileName() == "rcc")
                                     && sourcePath.fileName().startsWith("qrc_")
                                     && sourcePath.fileName().endsWith("_init.cpp");

        if (buildDirQmldirOrRcc || otherDirQmldirOrMetatypes || buildDirPluginCpp || buildRccInitCpp)
            node->setIsGenerated(true);

        const bool showSourceFolders = settings(targetRoot->getProject()).showSourceSubFolders()
                                       && defaultCMakeSourceGroupFolder(td.sourceGroups[si.sourceGroup]);

        // Where does the file node need to go?
        if (showSourceFolders && sourcePath.isChildOf(buildDirectory) && !inSourceBuild) {
            buildFileNodes.emplace_back(std::move(node));
        } else if (!showSourceFolders || sourcePath.isChildOf(sourceDirectory)) {
            sourceGroupFileNodes[si.sourceGroup].emplace_back(std::move(node));
        } else {
            otherFileNodes.emplace_back(std::move(node));
        }
    }

    for (size_t i = 0; i < sourceGroupFileNodes.size(); ++i) {
        const bool showSourceFolders = settings(targetRoot->getProject()).showSourceSubFolders()
                                       && defaultCMakeSourceGroupFolder(td.sourceGroups[i]);

        std::vector<std::unique_ptr<FileNode>> &current = sourceGroupFileNodes[i];
        FolderNode *insertNode = td.sourceGroups[i] == "TREE"
                                     ? targetRoot
                                     : createSourceGroupNode(td.sourceGroups[i],
                                                             sourceDirectory,
                                                             targetRoot);
        if (showSourceFolders) {
            FilePath baseDir = sourceDirectory.pathAppended(td.sourceGroups[i]);
            const bool caseSensitiveMatch = baseDir.nativePath()
                                            == baseDir.canonicalPath().nativePath();
            if (!baseDir.exists() || !caseSensitiveMatch)
                baseDir = sourceDirectory;
            insertNode->addNestedNodes(std::move(current), baseDir);
        } else {
            for (auto &fileNodes : current)
                insertNode->addNode(std::move(fileNodes));
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

static void addTargets(FolderNode *root,
                       const QFuture<void> &cancelFuture,
                       const QHash<FilePath, ProjectNode *> &cmakeListsNodes,
                       const ConfigurationInfo &config,
                       const std::vector<TargetDetails> &targetDetails,
                       const FilePath &sourceDir,
                       const FilePath &buildDir)
{
    QHash<QString, const TargetDetails *> targetDetailsHash;
    for (const TargetDetails &t : targetDetails)
        targetDetailsHash.insert(t.id, &t);
    const TargetDetails defaultTargetDetails;
    auto getTargetDetails = [&targetDetailsHash,
                             &defaultTargetDetails](const QString &id) -> const TargetDetails & {
        auto it = targetDetailsHash.constFind(id);
        if (it != targetDetailsHash.constEnd())
            return *it.value();
        return defaultTargetDetails;
    };

    auto createTargetNode = [](auto &cmakeListsNodes,
                               const Utils::FilePath &dir,
                               const QString &displayName) -> CMakeTargetNode * {
        auto *cmln = cmakeListsNodes.value(dir);
        QTC_ASSERT(cmln, return nullptr);

        QString targetId = displayName;

        CMakeTargetNode *tn = static_cast<CMakeTargetNode *>(
            cmln->findNode([&targetId](const Node *n) { return n->buildKey() == targetId; }));
        if (!tn) {
            auto newNode = std::make_unique<CMakeTargetNode>(dir, displayName);
            tn = newNode.get();
            cmln->addNode(std::move(newNode));
        }
        tn->setDisplayName(displayName);
        return tn;
    };

    QHash<FilePath, FolderNode *> folderNodes;

    for (const FileApiDetails::TargetInfo &t : config.targets) {
        if (cancelFuture.isCanceled())
            return;

        const TargetDetails &td = getTargetDetails(t.id);

        CMakeTargetNode *tNode{nullptr};
        const FilePath dir = directorySourceDir(config, sourceDir, t.directory);

        // FIXME: remove the usage of "qtc_runnable" by parsing the CMake code instead
        if (!td.folderTargetProperty.isEmpty() && td.folderTargetProperty != QTC_RUNNABLE) {
            const FilePath folderDir = FilePath::fromString(td.folderTargetProperty);
            if (!folderNodes.contains(folderDir))
                folderNodes.insert(
                    folderDir, createSourceGroupNode(td.folderTargetProperty, folderDir, root));

            tNode = createTargetNode(folderNodes, folderDir, t.name);

            // Set the correct source directory, not the FOLDER property value
            tNode->setFilePath(dir);
        } else {
            tNode = createTargetNode(cmakeListsNodes, dir, t.name);
        }
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

    const FileApiDetails::ProjectInfo topLevelProject
        = findOrDefault(data.codemodel.projects, equal(&FileApiDetails::ProjectInfo::parent, -1));
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

    addTargets(result.get(),
               cancelFuture,
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
                QList<FolderNode::LocationInfo> result;
                Utils::reverseForeach(bt, [&](const FolderNode::LocationInfo &i) {
                    int count = locations.count();
                    locations.insert({i.path, i.line});
                    if (count != locations.count()) {
                        result.append(i);
                    }
                });
                return result;
            };

            QList<FolderNode::LocationInfo> result = dedup(t.backtrace);
            auto dedupMulti = [&dedup](const Backtraces &bts) {
                QList<FolderNode::LocationInfo> result;
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

            if (!t.targetFolder.isEmpty() && !t.backtrace.isEmpty()
                && t.targetType != TargetType::UtilityType) {
                auto cmakeDefinition = std::make_unique<FileNode>(
                    t.backtrace.last().path, Node::fileTypeForFileName(t.backtrace.last().path));
                cmakeDefinition->setLine(t.backtrace.last().line);
                cmakeDefinition->setPriority(Node::DefaultProjectFilePriority);
                folderNode->addNode(std::move(cmakeDefinition));
            }
        }
    }
}

static void setIsGenerated(QSet<CMakeFileInfo> &cmakeFiles, Node *node, bool isGenerated)
{
    // Replace the key in a QSet by searching and inserting the updated key
    CMakeFileInfo info;
    info.path = node->path();

    auto it = cmakeFiles.find(info);
    if (it != cmakeFiles.end()) {
        info = *it;
        info.isGenerated = isGenerated;
        cmakeFiles.insert(info);
    }

    node->setIsGenerated(isGenerated);
}

static void markCMakeModulesFromPrefixPathAsGenerated(
    FileApiQtcData &result, const FilePath &sourceDir, const FilePath &buildDir)
{
    const QSet<FilePath> externlPaths = [&result]() {
        QSet<FilePath> paths;
        for (const QByteArray var : {"CMAKE_PREFIX_PATH", "CMAKE_FIND_ROOT_PATH"}) {
            const QStringList pathList = result.cache.stringValueOf(var).split(";");
            for (const QString &path : pathList)
                paths.insert(FilePath::fromUserInput(path));
        }
        return paths;
    }();

    if (!result.rootProjectNode)
        return;

    result.rootProjectNode->forEachGenericNode(
        [&externlPaths, &sourceDir, &buildDir, &result](Node *node) {
            for (const FilePath &path : externlPaths) {
                const bool isExternal = !node->path().isChildOf(sourceDir)
                                        && !node->path().isChildOf(buildDir);
                if (node->path().isChildOf(path) && isExternal) {
                    setIsGenerated(result.cmakeFiles, node, true);
                    break;
                }
            }
        });
}

static void setSubprojectBuildSupport(FileApiQtcData &result)
{
    if (!result.rootProjectNode)
        return;

    result.rootProjectNode->forEachGenericNode([&](Node *node) {
        if (auto cmakeListsNode = dynamic_cast<CMakeListsNode *>(node)) {
            cmakeListsNode->setHasSubprojectBuildSupport(
                result.cmakeGenerator.contains("Ninja")
                || result.cmakeGenerator.contains("Makefiles"));
        }
    });
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
    rootProjectNode.get()->compress();
    ProjectTree::applyTreeManager(rootProjectNode.get(), ProjectTree::AsyncPhase); // QRC nodes
    result.rootProjectNode = std::move(rootProjectNode);

    setupLocationInfoForTargets(cancelFuture, result.rootProjectNode.get(), result.buildTargets);
    if (cancelFuture.isCanceled())
        return {};

    result.ctestPath = input.replyFile.ctestExecutable;
    result.cmakeGenerator = input.replyFile.generator;
    result.isMultiConfig = input.replyFile.isMultiConfig;
    if (input.replyFile.isMultiConfig && input.replyFile.generator != "Ninja Multi-Config")
        result.usesAllCapsTargets = true;

    markCMakeModulesFromPrefixPathAsGenerated(result, sourceDir, buildDir);
    setSubprojectBuildSupport(result);

    return result;
}

} // CMakeProjectManager::Internal
