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

#include "fileapiparser.h"
#include "projecttreehelper.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/utilsicons.h>

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
    QSet<FilePath> cmakeFiles;

    std::vector<std::unique_ptr<ProjectExplorer::FileNode>> cmakeNodesSource;
    std::vector<std::unique_ptr<ProjectExplorer::FileNode>> cmakeNodesBuild;
    std::vector<std::unique_ptr<ProjectExplorer::FileNode>> cmakeNodesOther;
    std::vector<std::unique_ptr<ProjectExplorer::FileNode>> cmakeListNodes;
};

CMakeFileResult extractCMakeFilesData(const std::vector<FileApiDetails::CMakeFileInfo> &cmakefiles,
                                      const FilePath &sourceDirectory,
                                      const FilePath &buildDirectory)
{
    CMakeFileResult result;

    QDir sourceDir(sourceDirectory.toString());
    QDir buildDir(buildDirectory.toString());

    for (const CMakeFileInfo &info : cmakefiles) {
        const FilePath sfn = FilePath::fromString(
            QDir::cleanPath(sourceDir.absoluteFilePath(info.path)));
        const int oldCount = result.cmakeFiles.count();
        result.cmakeFiles.insert(sfn);
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
            } else if (sfn.isChildOf(sourceDir)) {
                result.cmakeNodesSource.emplace_back(std::move(node));
            } else if (sfn.isChildOf(buildDir)) {
                result.cmakeNodesBuild.emplace_back(std::move(node));
            } else {
                result.cmakeNodesOther.emplace_back(std::move(node));
            }
        }
    }

    return result;
}

Configuration extractConfiguration(std::vector<Configuration> &codemodel, QString &errorMessage)
{
    if (codemodel.size() == 0) {
        qWarning() << "No configuration found!";
        errorMessage = "No configuration found!";
        return {};
    }
    if (codemodel.size() > 1)
        qWarning() << "Multi-configuration generator found, ignoring all but first configuration";

    Configuration result = std::move(codemodel[0]);
    codemodel.clear();

    return result;
}

class PreprocessedData
{
public:
    CMakeProjectManager::CMakeConfig cache;

    QSet<FilePath> cmakeFiles;

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
    PreprocessedData result;

    result.cache = std::move(data.cache); // Make sure this is available, even when nothing else is

    // Simplify to only one configuration:
    result.codemodel = extractConfiguration(data.codemodel, errorMessage);

    if (!errorMessage.isEmpty()) {
        return result;
    }

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

QList<CMakeBuildTarget> generateBuildTargets(const PreprocessedData &input,
                                             const FilePath &sourceDirectory,
                                             const FilePath &buildDirectory)
{
    QDir sourceDir(sourceDirectory.toString());
    QDir buildDir(buildDirectory.toString());

    const QList<CMakeBuildTarget> result = transform<QList>(
        input.targetDetails, [&sourceDir, &buildDir](const TargetDetails &t) -> CMakeBuildTarget {
            const auto currentBuildDir = QDir(buildDir.absoluteFilePath(t.buildDir.toString()));

            CMakeBuildTarget ct;
            ct.title = t.name;
            ct.executable = t.artifacts.isEmpty()
                                ? FilePath()
                                : FilePath::fromString(QDir::cleanPath(
                                    buildDir.absoluteFilePath(t.artifacts.at(0).toString())));
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
                                      ? FilePath::fromString(currentBuildDir.absolutePath())
                                      : ct.executable.parentDir();
            ct.sourceDirectory = FilePath::fromString(
                QDir::cleanPath(sourceDir.absoluteFilePath(t.sourceDir.toString())));

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
                ct.includeDefinitions.append(
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

                // Extract library directories for executables:
                for (const FragmentInfo &f : t.link.value().fragments) {
                    if (f.role == "flags") // ignore all flags fragments
                        continue;

                    // CMake sometimes mixes several shell-escaped pieces into one fragment. Disentangle that again:
                    const QStringList parts = QtcProcess::splitArgs(f.fragment);
                    for (const QString &part : parts) {
                        // Some projects abuse linking to libraries to pass random flags to the linker, so ignore
                        // flags mixed into a fragment
                        if (part.startsWith("-"))
                            continue;

                        FilePath tmp = FilePath::fromString(
                            currentBuildDir.absoluteFilePath(QDir::fromNativeSeparators(part)));

                        if (f.role == "libraries")
                            tmp = tmp.parentDir();

                        if (!tmp.isEmpty()
                            && tmp.isDir()) { // f.role is libraryPath or frameworkPath
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
        result += QtcProcess::splitArgs(f);
    }
    return result;
}

RawProjectParts generateRawProjectParts(const PreprocessedData &input,
                                        const FilePath &sourceDirectory)
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
            if (ci.language == "C")
                ending = "/cmake_pch.h";
            else if (ci.language == "CXX")
                ending = "/cmake_pch.hxx";

            ++counter;
            RawProjectPart rpp;
            rpp.setProjectFileLocation(t.sourceDir.pathAppended("CMakeLists.txt").toString());
            rpp.setBuildSystemTarget(t.name);
            const QString postfix = needPostfix ? "_cg" + QString::number(count) : QString();
            rpp.setDisplayName(t.id + postfix);
            rpp.setMacros(transform<QVector>(ci.defines, &DefineInfo::define));
            rpp.setHeaderPaths(transform<QVector>(ci.includes, &IncludeInfo::path));

            RawProjectPartFlags cProjectFlags;
            cProjectFlags.commandLineFlags = splitFragments(ci.fragments);
            rpp.setFlagsForC(cProjectFlags);

            RawProjectPartFlags cxxProjectFlags;
            cxxProjectFlags.commandLineFlags = cProjectFlags.commandLineFlags;
            rpp.setFlagsForCxx(cxxProjectFlags);

            FilePath precompiled_header
                = FilePath::fromString(findOrDefault(t.sources, [&ending](const SourceInfo &si) {
                                           return si.path.endsWith(ending);
                                       }).path);

            rpp.setFiles(transform<QList>(ci.sources, [&t, &sourceDir](const int si) {
                return sourceDir.absoluteFilePath(t.sources[static_cast<size_t>(si)].path);
            }));
            if (!precompiled_header.isEmpty()) {
                if (precompiled_header.toFileInfo().isRelative()) {
                    const FilePath parentDir = FilePath::fromString(sourceDir.absolutePath());
                    precompiled_header = parentDir.pathAppended(precompiled_header.toString());
                }
                rpp.setPreCompiledHeaders({precompiled_header.toString()});
            }

            const bool isExecutable = t.type == "EXECUTABLE";
            rpp.setBuildTargetType(isExecutable ? ProjectExplorer::BuildTargetType::Executable
                                                : ProjectExplorer::BuildTargetType::Library);
            rpps.append(rpp);
            ++count;
        }
    }

    return rpps;
}

FilePath directorySourceDir(const Configuration &c, const QDir &sourceDir, int directoryIndex)
{
    const size_t di = static_cast<size_t>(directoryIndex);
    QTC_ASSERT(di < c.directories.size(), return FilePath());

    return FilePath::fromString(
        QDir::cleanPath(sourceDir.absoluteFilePath(c.directories[di].sourcePath)));
}

FilePath directoryBuildDir(const Configuration &c, const QDir &buildDir, int directoryIndex)
{
    const size_t di = static_cast<size_t>(directoryIndex);
    QTC_ASSERT(di < c.directories.size(), return FilePath());

    return FilePath::fromString(
        QDir::cleanPath(buildDir.absoluteFilePath(c.directories[di].buildPath)));
}

void addProjects(const QHash<Utils::FilePath, ProjectNode *> &cmakeListsNodes,
                 const Configuration &config,
                 const QDir &sourceDir)
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
                node->setIcon(QIcon::fromTheme("edit-copy", ::Utils::Icons::COPY.icon()));

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
                      const TargetDetails &td,
                      QSet<FilePath> &knownHeaderNodes)
{
    const bool inSourceBuild = (sourceDirectory == buildDirectory);
    const QDir currentSourceDir(sourceDirectory.toString());

    std::vector<std::unique_ptr<FileNode>> toList;
    QSet<Utils::FilePath> alreadyListed;

    // Files already added by other configurations:
    targetRoot->forEachGenericNode(
        [&alreadyListed](const Node *n) { alreadyListed.insert(n->filePath()); });

    const QDir topSourceDir(topSourceDirectory.toString());

    std::vector<std::unique_ptr<FileNode>> buildFileNodes;
    std::vector<std::unique_ptr<FileNode>> otherFileNodes;
    std::vector<std::vector<std::unique_ptr<FileNode>>> sourceGroupFileNodes{td.sourceGroups.size()};

    for (const SourceInfo &si : td.sources) {
        const FilePath sourcePath = FilePath::fromString(
            QDir::cleanPath(topSourceDir.absoluteFilePath(si.path)));

        // Filter out already known files:
        const int count = alreadyListed.count();
        alreadyListed.insert(sourcePath);
        if (count == alreadyListed.count())
            continue;

        // Create FileNodes from the file
        auto node = std::make_unique<FileNode>(sourcePath, Node::fileTypeForFileName(sourcePath));
        node->setIsGenerated(si.isGenerated);

        // Register headers:
        if (node->fileType() == FileType::Header)
            knownHeaderNodes.insert(node->filePath());

        // Where does the file node need to go?
        if (sourcePath.isChildOf(buildDirectory) && !inSourceBuild) {
            buildFileNodes.emplace_back(std::move(node));
        } else if (sourcePath.isChildOf(sourceDirectory)) {
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
        insertNode->addNestedNodes(std::move(current), baseDirectory);
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
                const FilePath &topSourceDir,
                const QDir &sourceDir,
                const QDir &buildDir,
                QSet<FilePath> &knownHeaderNodes)
{
    for (const FileApiDetails::Target &t : config.targets) {
        const TargetDetails &td = Utils::findOrDefault(targetDetails,
                                                       Utils::equal(&TargetDetails::id, t.id));

        const FilePath dir = directorySourceDir(config, sourceDir, t.directory);

        CMakeTargetNode *tNode = createTargetNode(cmakeListsNodes, dir, t.name);
        QTC_ASSERT(tNode, continue);

        tNode->setTargetInformation(td.artifacts, td.type);
        tNode->setBuildDirectory(directoryBuildDir(config, buildDir, t.directory));

        addCompileGroups(tNode, topSourceDir, dir, tNode->buildDirectory(), td, knownHeaderNodes);
    }
}

std::pair<std::unique_ptr<CMakeProjectNode>, QSet<FilePath>> generateRootProjectNode(
    PreprocessedData &data, const FilePath &sourceDirectory, const FilePath &buildDirectory)
{
    std::pair<std::unique_ptr<CMakeProjectNode>, QSet<FilePath>> result;
    result.first = std::make_unique<CMakeProjectNode>(sourceDirectory);

    const QDir sourceDir(sourceDirectory.toString());
    const QDir buildDir(buildDirectory.toString());

    const FileApiDetails::Project topLevelProject
        = findOrDefault(data.codemodel.projects, equal(&FileApiDetails::Project::parent, -1));
    if (!topLevelProject.name.isEmpty())
        result.first->setDisplayName(topLevelProject.name);
    else
        result.first->setDisplayName(sourceDirectory.fileName());

    QHash<FilePath, ProjectNode *> cmakeListsNodes = addCMakeLists(result.first.get(),
                                                                   std::move(data.cmakeListNodes));
    data.cmakeListNodes.clear(); // Remove all the nullptr in the vector...

    QSet<FilePath> knownHeaders;
    addProjects(cmakeListsNodes, data.codemodel, sourceDir);

    addTargets(cmakeListsNodes,
               data.codemodel,
               data.targetDetails,
               sourceDirectory,
               sourceDir,
               buildDir,
               knownHeaders);

    // addHeaderNodes(root.get(), knownHeaders, allFiles);

    if (!data.cmakeNodesSource.empty() || !data.cmakeNodesBuild.empty()
        || !data.cmakeNodesOther.empty())
        addCMakeInputs(result.first.get(),
                       sourceDirectory,
                       buildDirectory,
                       std::move(data.cmakeNodesSource),
                       std::move(data.cmakeNodesBuild),
                       std::move(data.cmakeNodesOther));

    data.cmakeNodesSource.clear(); // Remove all the nullptr in the vector...
    data.cmakeNodesBuild.clear();  // Remove all the nullptr in the vector...
    data.cmakeNodesOther.clear();  // Remove all the nullptr in the vector...

    result.second = knownHeaders;

    return result;
}

void setupLocationInfoForTargets(CMakeProjectNode *rootNode, const QList<CMakeBuildTarget> &targets)
{
    for (const CMakeBuildTarget &t : targets) {
        FolderNode *folderNode = static_cast<FolderNode *>(
            rootNode->findNode(Utils::equal(&Node::buildKey, t.title)));
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

    result.buildTargets = generateBuildTargets(data, sourceDirectory, buildDirectory);
    result.cmakeFiles = std::move(data.cmakeFiles);
    result.projectParts = generateRawProjectParts(data, sourceDirectory);

    auto pair = generateRootProjectNode(data, sourceDirectory, buildDirectory);
    result.rootProjectNode = std::move(pair.first);
    result.knownHeaders = std::move(pair.second);

    setupLocationInfoForTargets(result.rootProjectNode.get(), result.buildTargets);

    return result;
}

} // namespace Internal
} // namespace CMakeProjectManager
