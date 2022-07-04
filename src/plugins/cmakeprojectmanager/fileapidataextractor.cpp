/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "fileapidataextractor.h"

#include "cmakeprojectplugin.h"
#include "cmakespecificsettings.h"
#include "fileapiparser.h"
#include "projecttreehelper.h"

#include <cppeditor/cppeditorconstants.h>

#include <utils/algorithm.h>
#include <utils/mimeutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/utilsicons.h>

#include <projectexplorer/projecttree.h>

#include <QDir>

using namespace ProjectExplorer;
using namespace Utils;

namespace {

using namespace CMakeProjectManager;
using namespace CMakeProjectManager::Internal;
using namespace CMakeProjectManager::Internal::FileApiDetails;

// --------------------------------------------------------------------
// Helpers:
// --------------------------------------------------------------------

class CMakeFileResult
{
public:
    QSet<CMakeFileInfo> cmakeFiles;

    std::vector<std::unique_ptr<ProjectExplorer::FileNode>> cmakeNodesSource;
    std::vector<std::unique_ptr<ProjectExplorer::FileNode>> cmakeNodesBuild;
    std::vector<std::unique_ptr<ProjectExplorer::FileNode>> cmakeNodesOther;
    std::vector<std::unique_ptr<ProjectExplorer::FileNode>> cmakeListNodes;
};

CMakeFileResult extractCMakeFilesData(const std::vector<CMakeFileInfo> &cmakefiles,
                                      const FilePath &sourceDirectory,
                                      const FilePath &buildDirectory)
{
    CMakeFileResult result;

    for (const CMakeFileInfo &info : cmakefiles) {
        const FilePath sfn = sourceDirectory.resolvePath(info.path);
        const int oldCount = result.cmakeFiles.count();
        CMakeFileInfo absolute(info);
        absolute.path = sfn;
        result.cmakeFiles.insert(absolute);
        if (oldCount < result.cmakeFiles.count()) {
            if (info.isCMake && !info.isCMakeListsDotTxt) {
                // Skip files that cmake considers to be part of the installation -- but include
                // CMakeLists.txt files. This fixes cmake binaries running from their own
                // build directory.
                continue;
            }

            auto node = std::make_unique<FileNode>(sfn, FileType::Project);
            node->setIsGenerated(info.isGenerated
                                 && !info.isCMakeListsDotTxt); // CMakeLists.txt are never
                                                               // generated, independent
                                                               // what cmake thinks:-)

            if (info.isCMakeListsDotTxt) {
                result.cmakeListNodes.emplace_back(std::move(node));
            } else if (sfn.isChildOf(sourceDirectory)) {
                result.cmakeNodesSource.emplace_back(std::move(node));
            } else if (sfn.isChildOf(buildDirectory)) {
                result.cmakeNodesBuild.emplace_back(std::move(node));
            } else {
                result.cmakeNodesOther.emplace_back(std::move(node));
            }
        }
    }

    return result;
}

class PreprocessedData
{
public:
    CMakeProjectManager::CMakeConfig cache;

    QSet<CMakeFileInfo> cmakeFiles;

    std::vector<std::unique_ptr<ProjectExplorer::FileNode>> cmakeNodesSource;
    std::vector<std::unique_ptr<ProjectExplorer::FileNode>> cmakeNodesBuild;
    std::vector<std::unique_ptr<ProjectExplorer::FileNode>> cmakeNodesOther;
    std::vector<std::unique_ptr<ProjectExplorer::FileNode>> cmakeListNodes;

    Configuration codemodel;
    std::vector<TargetDetails> targetDetails;
};

PreprocessedData preprocess(FileApiData &data,
                            const FilePath &sourceDirectory,
                            const FilePath &buildDirectory,
                            QString &errorMessage)
{
    Q_UNUSED(errorMessage)

    PreprocessedData result;

    result.cache = std::move(data.cache); // Make sure this is available, even when nothing else is

    result.codemodel = std::move(data.codemodel);

    CMakeFileResult cmakeFileResult = extractCMakeFilesData(data.cmakeFiles,
                                                            sourceDirectory,
                                                            buildDirectory);

    result.cmakeFiles = std::move(cmakeFileResult.cmakeFiles);
    result.cmakeNodesSource = std::move(cmakeFileResult.cmakeNodesSource);
    result.cmakeNodesBuild = std::move(cmakeFileResult.cmakeNodesBuild);
    result.cmakeNodesOther = std::move(cmakeFileResult.cmakeNodesOther);
    result.cmakeListNodes = std::move(cmakeFileResult.cmakeListNodes);

    result.targetDetails = std::move(data.targetDetails);

    return result;
}

QVector<FolderNode::LocationInfo> extractBacktraceInformation(const BacktraceInfo &backtraces,
                                                              const QDir &sourceDir,
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
        const FilePath path = FilePath::fromString(
            sourceDir.absoluteFilePath(backtraces.files[fileIndex]));

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

static bool isChildOf(const FilePath &path, const QStringList &prefixes)
{
    for (const QString &prefix : prefixes)
        if (path.isChildOf(FilePath::fromString(prefix)))
            return true;
    return false;
}

QList<CMakeBuildTarget> generateBuildTargets(const PreprocessedData &input,
                                             const FilePath &sourceDirectory,
                                             const FilePath &buildDirectory,
                                             bool haveLibrariesRelativeToBuildDirectory)
{
    QDir sourceDir(sourceDirectory.toString());

    const QList<CMakeBuildTarget> result = transform<QList>(input.targetDetails,
        [&sourceDir, &sourceDirectory, &buildDirectory,
         &haveLibrariesRelativeToBuildDirectory](const TargetDetails &t) {
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

            ct.backtrace = extractBacktraceInformation(t.backtraceGraph, sourceDir, t.backtrace, 0);

            for (const DependencyInfo &d : t.dependencies) {
                ct.dependencyDefinitions.append(
                    extractBacktraceInformation(t.backtraceGraph, sourceDir, d.backtrace, 100));
            }
            for (const SourceInfo &si : t.sources) {
                ct.sourceDefinitions.append(
                    extractBacktraceInformation(t.backtraceGraph, sourceDir, si.backtrace, 200));
            }
            for (const CompileInfo &ci : t.compileGroups) {
                for (const IncludeInfo &ii : ci.includes) {
                    ct.includeDefinitions.append(
                        extractBacktraceInformation(t.backtraceGraph, sourceDir, ii.backtrace, 300));
                }
                for (const DefineInfo &di : ci.defines) {
                    ct.defineDefinitions.append(
                        extractBacktraceInformation(t.backtraceGraph, sourceDir, di.backtrace, 400));
                }
            }
            for (const InstallDestination &id : t.installDestination) {
                ct.installDefinitions.append(
                    extractBacktraceInformation(t.backtraceGraph, sourceDir, id.backtrace, 500));
            }

            if (ct.targetType == ExecutableType) {
                Utils::FilePaths librarySeachPaths;
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
                    const QStringList parts = ProcessArgs::splitArgs(f.fragment);
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

                        const FilePath buildDir = haveLibrariesRelativeToBuildDirectory ? buildDirectory : currentBuildDir;
                        FilePath tmp = buildDir.resolvePath(FilePath::fromUserInput(part));

                        if (f.role == "libraries")
                            tmp = tmp.parentDir();

                        if (!tmp.isEmpty() && tmp.isDir()) {
                            // f.role is libraryPath or frameworkPath
                            // On Linux, exclude sub-paths from "/lib(64)", "/usr/lib(64)" and
                            // "/usr/local/lib" since these are usually in the standard search
                            // paths. There probably are more, but the naming schemes are arbitrary
                            // so we'd need to ask the linker ("ld --verbose | grep SEARCH_DIR").
                            if (!HostOsInfo::isLinuxHost()
                                || !isChildOf(tmp,
                                              {"/lib",
                                               "/lib64",
                                               "/usr/lib",
                                               "/usr/lib64",
                                               "/usr/local/lib"})) {
                                librarySeachPaths.append(tmp);
                                // Libraries often have their import libs in ../lib and the
                                // actual dll files in ../bin on windows. Qt is one example of that.
                                if (tmp.fileName() == "lib" && HostOsInfo::isWindowsHost()) {
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
        });
    return result;
}

static QStringList splitFragments(const QStringList &fragments)
{
    QStringList result;
    for (const QString &f : fragments) {
        result += ProcessArgs::splitArgs(f);
    }
    return result;
}

bool isPchFile(const FilePath &buildDirectory, const FilePath &path)
{
    return path.isChildOf(buildDirectory) && path.fileName().startsWith("cmake_pch");
}

RawProjectParts generateRawProjectParts(const PreprocessedData &input,
                                        const FilePath &sourceDirectory,
                                        const FilePath &buildDirectory)
{
    RawProjectParts rpps;

    int counter = 0;
    for (const TargetDetails &t : input.targetDetails) {
        QDir sourceDir(sourceDirectory.toString());
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

            ++counter;
            RawProjectPart rpp;
            rpp.setProjectFileLocation(t.sourceDir.pathAppended("CMakeLists.txt").toString());
            rpp.setBuildSystemTarget(t.name);
            const QString postfix = needPostfix ? "_cg" + QString::number(count) : QString();
            rpp.setDisplayName(t.id + postfix);
            rpp.setMacros(transform<QVector>(ci.defines, &DefineInfo::define));
            rpp.setHeaderPaths(transform<QVector>(ci.includes, &IncludeInfo::path));

            QStringList fragments = splitFragments(ci.fragments);

            // Get all sources from the compiler group, except generated sources
            QStringList sources;
            for (auto idx: ci.sources) {
                SourceInfo si = t.sources.at(idx);
                if (si.isGenerated)
                    continue;
                sources.push_back(sourceDir.absoluteFilePath(si.path));
            }

            // If we are not in a pch compiler group, add all the headers that are not generated
            const bool hasPchSource = anyOf(sources, [buildDirectory](const QString &path) {
                return isPchFile(buildDirectory, FilePath::fromString(path));
            });

            QString headerMimeType;
            if (ci.language == "C")
                headerMimeType = CppEditor::Constants::C_HEADER_MIMETYPE;
            else if (ci.language == "CXX")
                headerMimeType = CppEditor::Constants::CPP_HEADER_MIMETYPE;
            if (!hasPchSource) {
                for (const SourceInfo &si : t.sources) {
                    if (si.isGenerated)
                        continue;
                    const auto mimeTypes = Utils::mimeTypesForFileName(si.path);
                    for (const auto &mime : mimeTypes)
                        if (mime.name() == headerMimeType)
                            sources.push_back(sourceDir.absoluteFilePath(si.path));
                }
            }

            // Set project files except pch files
            rpp.setFiles(Utils::filtered(sources, [buildDirectory](const QString &path) {
                             return !isPchFile(buildDirectory, FilePath::fromString(path));
                         }), {}, [headerMimeType](const QString &path) {
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
                if (precompiled_header.toFileInfo().isRelative()) {
                    const FilePath parentDir = FilePath::fromString(sourceDir.absolutePath());
                    precompiled_header = parentDir.pathAppended(precompiled_header.toString());
                }

                // Remove the CMake PCH usage command line options in order to avoid the case
                // when the build system would produce a .pch/.gch file that would be treated
                // by the Clang code model as its own and fail.
                auto remove = [&](const QStringList &args) {
                    auto foundPos = std::search(fragments.begin(), fragments.end(),
                                                args.begin(), args.end());
                    if (foundPos != fragments.end())
                        fragments.erase(foundPos, std::next(foundPos, args.size()));
                };

                remove({"-Xclang", "-include-pch", "-Xclang", precompiled_header.toString() + ".gch"});
                remove({"-Xclang", "-include-pch", "-Xclang", precompiled_header.toString() + ".pch"});
                remove({"-Xclang", "-include", "-Xclang", precompiled_header.toString()});
                remove({"-include", precompiled_header.toString()});
                remove({"/FI", precompiled_header.toString()});

                // Make a copy of the CMake PCH header and use it instead
                FilePath qtc_precompiled_header = precompiled_header.parentDir().pathAppended(qtcPchFile);
                FileUtils::copyIfDifferent(precompiled_header, qtc_precompiled_header);

                rpp.setPreCompiledHeaders({qtc_precompiled_header.toString()});
            }

            RawProjectPartFlags cProjectFlags;
            cProjectFlags.commandLineFlags = fragments;
            rpp.setFlagsForC(cProjectFlags);

            RawProjectPartFlags cxxProjectFlags;
            cxxProjectFlags.commandLineFlags = cProjectFlags.commandLineFlags;
            rpp.setFlagsForCxx(cxxProjectFlags);

            const bool isExecutable = t.type == "EXECUTABLE";
            rpp.setBuildTargetType(isExecutable ? ProjectExplorer::BuildTargetType::Executable
                                                : ProjectExplorer::BuildTargetType::Library);
            rpps.append(rpp);
            ++count;
        }
    }

    return rpps;
}

FilePath directorySourceDir(const Configuration &c, const FilePath &sourceDir, int directoryIndex)
{
    const size_t di = static_cast<size_t>(directoryIndex);
    QTC_ASSERT(di < c.directories.size(), return FilePath());

    return sourceDir.resolvePath(c.directories[di].sourcePath).cleanPath();
}

FilePath directoryBuildDir(const Configuration &c, const FilePath &buildDir, int directoryIndex)
{
    const size_t di = static_cast<size_t>(directoryIndex);
    QTC_ASSERT(di < c.directories.size(), return FilePath());

    return buildDir.resolvePath(c.directories[di].buildPath).cleanPath();
}

void addProjects(const QHash<Utils::FilePath, ProjectNode *> &cmakeListsNodes,
                 const Configuration &config,
                 const FilePath &sourceDir)
{
    for (const FileApiDetails::Project &p : config.projects) {
        if (p.parent == -1)
            continue; // Top-level project has already been covered
        FilePath dir = directorySourceDir(config, sourceDir, p.directories[0]);
        createProjectNode(cmakeListsNodes, dir, p.name);
    }
}

FolderNode *createSourceGroupNode(const QString &sourceGroupName,
                                  const FilePath &sourceDirectory,
                                  FolderNode *targetRoot)
{
    FolderNode *currentNode = targetRoot;

    if (!sourceGroupName.isEmpty()) {
        const QStringList parts = sourceGroupName.split("\\");

        for (const QString &p : parts) {
            FolderNode *existingNode = Utils::findOrDefault(currentNode->folderNodes(),
                                                            [&p](const FolderNode *fn) {
                                                                return fn->displayName() == p;
                                                            });

            if (!existingNode) {
                auto node = createCMakeVFolder(sourceDirectory, Node::DefaultFolderPriority + 5, p);
                node->setListInProject(false);
                node->setIcon(
                    [] { return QIcon::fromTheme("edit-copy", ::Utils::Icons::COPY.icon()); });

                existingNode = node.get();

                currentNode->addNode(std::move(node));
            }

            currentNode = existingNode;
        }
    }
    return currentNode;
}

void addCompileGroups(ProjectNode *targetRoot,
                      const Utils::FilePath &topSourceDirectory,
                      const Utils::FilePath &sourceDirectory,
                      const Utils::FilePath &buildDirectory,
                      const TargetDetails &td)
{
    CMakeSpecificSettings *settings = CMakeProjectPlugin::projectTypeSpecificSettings();
    const bool showSourceFolders = settings->showSourceSubFolders.value();

    const bool inSourceBuild = (sourceDirectory == buildDirectory);

    std::vector<std::unique_ptr<FileNode>> toList;
    QSet<Utils::FilePath> alreadyListed;

    // Files already added by other configurations:
    targetRoot->forEachGenericNode(
        [&alreadyListed](const Node *n) { alreadyListed.insert(n->filePath()); });

    std::vector<std::unique_ptr<FileNode>> buildFileNodes;
    std::vector<std::unique_ptr<FileNode>> otherFileNodes;
    std::vector<std::vector<std::unique_ptr<FileNode>>> sourceGroupFileNodes{td.sourceGroups.size()};

    for (const SourceInfo &si : td.sources) {
        const FilePath sourcePath = topSourceDirectory.resolvePath(si.path).cleanPath();

        // Filter out already known files:
        const int count = alreadyListed.count();
        alreadyListed.insert(sourcePath);
        if (count == alreadyListed.count())
            continue;

        // Create FileNodes from the file
        auto node = std::make_unique<FileNode>(sourcePath, Node::fileTypeForFileName(sourcePath));
        node->setIsGenerated(si.isGenerated);

        // CMake pch files are generated at configured time, but not marked as generated
        // so that a "clean" step won't remove them and at a subsequent build they won't exist.
        if (isPchFile(buildDirectory, sourcePath))
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
                baseDirectory = Utils::FileUtils::commonPath(baseDirectory, fn->filePath());
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
                    QCoreApplication::translate("CMakeProjectManager::Internal::FileApi",
                                                "<Build Directory>"),
                    std::move(buildFileNodes));
    addCMakeVFolder(targetRoot,
                    Utils::FilePath(),
                    10,
                    QCoreApplication::translate("CMakeProjectManager::Internal::FileApi",
                                                "<Other Locations>"),
                    std::move(otherFileNodes));
}

void addTargets(const QHash<Utils::FilePath, ProjectExplorer::ProjectNode *> &cmakeListsNodes,
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
        const TargetDetails &td = getTargetDetails(t.id);

        const FilePath dir = directorySourceDir(config, sourceDir, t.directory);

        CMakeTargetNode *tNode = createTargetNode(cmakeListsNodes, dir, t.name);
        QTC_ASSERT(tNode, continue);

        tNode->setTargetInformation(td.artifacts, td.type);
        tNode->setBuildDirectory(directoryBuildDir(config, buildDir, t.directory));

        addCompileGroups(tNode, sourceDir, dir, tNode->buildDirectory(), td);
    }
}

std::unique_ptr<CMakeProjectNode> generateRootProjectNode(
    PreprocessedData &data, const FilePath &sourceDirectory, const FilePath &buildDirectory)
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

    addProjects(cmakeListsNodes, data.codemodel, sourceDirectory);

    addTargets(cmakeListsNodes,
               data.codemodel,
               data.targetDetails,
               sourceDirectory,
               buildDirectory);

    if (!data.cmakeNodesSource.empty() || !data.cmakeNodesBuild.empty()
        || !data.cmakeNodesOther.empty())
        addCMakeInputs(result.get(),
                       sourceDirectory,
                       buildDirectory,
                       std::move(data.cmakeNodesSource),
                       std::move(data.cmakeNodesBuild),
                       std::move(data.cmakeNodesOther));

    data.cmakeNodesSource.clear(); // Remove all the nullptr in the vector...
    data.cmakeNodesBuild.clear();  // Remove all the nullptr in the vector...
    data.cmakeNodesOther.clear();  // Remove all the nullptr in the vector...

    return result;
}

void setupLocationInfoForTargets(CMakeProjectNode *rootNode, const QList<CMakeBuildTarget> &targets)
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
        FolderNode *folderNode = buildKeyToNode.value(t.title);
        if (folderNode) {
            QSet<std::pair<FilePath, int>> locations;
            auto dedup = [&locations](const Backtrace &bt) {
                QVector<FolderNode::LocationInfo> result;
                for (const FolderNode::LocationInfo &i : bt) {
                    int count = locations.count();
                    locations.insert(std::make_pair(i.path, i.line));
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

} // namespace

namespace CMakeProjectManager {
namespace Internal {

using namespace FileApiDetails;

// --------------------------------------------------------------------
// extractData:
// --------------------------------------------------------------------

FileApiQtcData extractData(FileApiData &input,
                           const FilePath &sourceDirectory,
                           const FilePath &buildDirectory)
{
    FileApiQtcData result;

    // Preprocess our input:
    PreprocessedData data = preprocess(input, sourceDirectory, buildDirectory, result.errorMessage);
    result.cache = std::move(data.cache); // Make sure this is available, even when nothing else is
    if (!result.errorMessage.isEmpty()) {
        return {};
    }

    // Ninja generator from CMake version 3.20.5 has libraries relative to build directory
    const bool haveLibrariesRelativeToBuildDirectory =
            input.replyFile.generator.startsWith("Ninja")
         && input.replyFile.cmakeVersion >= QVersionNumber(3, 20, 5);

    result.buildTargets = generateBuildTargets(data, sourceDirectory, buildDirectory, haveLibrariesRelativeToBuildDirectory);
    result.cmakeFiles = std::move(data.cmakeFiles);
    result.projectParts = generateRawProjectParts(data, sourceDirectory, buildDirectory);

    auto rootProjectNode = generateRootProjectNode(data, sourceDirectory, buildDirectory);
    ProjectTree::applyTreeManager(rootProjectNode.get(), ProjectTree::AsyncPhase); // QRC nodes
    result.rootProjectNode = std::move(rootProjectNode);

    setupLocationInfoForTargets(result.rootProjectNode.get(), result.buildTargets);

    result.ctestPath = input.replyFile.ctestExecutable;
    result.isMultiConfig = input.replyFile.isMultiConfig;
    if (input.replyFile.isMultiConfig && input.replyFile.generator != "Ninja Multi-Config")
        result.usesAllCapsTargets = true;

    return result;
}

} // namespace Internal
} // namespace CMakeProjectManager
