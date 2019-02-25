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

#include "builddependencycollector.h"

#include "collectbuilddependencytoolaction.h"
#include "commandlinebuilder.h"

#include <environment.h>

#include <utils/smallstring.h>

#include <algorithm>
#include <iostream>
namespace ClangBackEnd {

namespace {
FilePathIds operator+(const FilePathIds &first, const FilePathIds &second)
{
    FilePathIds result = first;

    std::copy(second.begin(), second.end(), std::back_inserter(result));

    return result;
}

FilePaths operator+(FilePaths &&first, FilePaths &&second) {
    FilePaths result = std::move(first);

    std::copy(second.begin(), second.end(), std::back_inserter(result));

    return result;
}

FilePaths generatedFilePaths(const V2::FileContainers &containers) {
    FilePaths paths;
    paths.reserve(containers.size());
    std::transform(containers.begin(),
                   containers.end(),
                   std::back_inserter(paths),
                   [](const auto &container) { return container.filePath; });
    return paths;
}

} // namespace

BuildDependency BuildDependencyCollector::create(const ProjectPartContainer &projectPart)
{
    CommandLineBuilder<ProjectPartContainer, Utils::SmallStringVector>
        builder{projectPart, projectPart.toolChainArguments, InputFileType::Source};

    addFiles(projectPart.sourcePathIds, std::move(builder.commandLine));

    setExcludedFilePaths(m_filePathCache.filePaths(projectPart.headerPathIds +
                                                   projectPart.sourcePathIds) +
                         generatedFilePaths(m_generatedFiles.fileContainers()));

    addUnsavedFiles(m_generatedFiles.fileContainers());

    collect();

    auto buildDependency = std::move(m_buildDependency);

    clear();

    return buildDependency;
}

namespace {

std::size_t contentSize(const FilePaths &includes)
{
    auto countIncludeSize = [](std::size_t size, const auto &include) {
        return size + include.size();
    };

    return std::accumulate(includes.begin(), includes.end(), std::size_t(0), countIncludeSize);
}
} // namespace

Utils::SmallString BuildDependencyCollector::generateFakeFileContent(
    const FilePathIds &includeIds) const
{
    Utils::SmallString fileContent;
    const std::size_t lineTemplateSize = 12;
    auto includes = m_filePathCache.filePaths(includeIds);

    fileContent.reserve(includes.size() * lineTemplateSize + contentSize(includes));

    for (Utils::SmallStringView include : includes)
        fileContent += {"#include \"", include, "\"\n"};

    return fileContent;
}

void BuildDependencyCollector::collect()
{
    clang::tooling::ClangTool tool = m_clangTool.createTool();

    auto action = std::make_unique<CollectBuildDependencyToolAction>(m_buildDependency,
                                                                     m_filePathCache,
                                                                     m_excludedFilePaths,
                                                                     m_sourcesManager);

    tool.run(action.get());
}

void BuildDependencyCollector::setExcludedFilePaths(ClangBackEnd::FilePaths &&excludedFilePaths)
{
    if (Utils::HostOsInfo::isWindowsHost()) {
        m_excludedFilePaths.clear();
        m_excludedFilePaths.reserve(excludedFilePaths.size());
        std::transform(std::make_move_iterator(excludedFilePaths.begin()),
                       std::make_move_iterator(excludedFilePaths.end()),
                       std::back_inserter(m_excludedFilePaths),
                       [](auto &&path) {
                           path.replace("/", "\\");
                           return std::move(path);
                       });
    } else {
        m_excludedFilePaths = std::move(excludedFilePaths);
    }
}

void BuildDependencyCollector::addFiles(const FilePathIds &filePathIds,
                                        Utils::SmallStringVector &&arguments)
{
    m_clangTool.addFile(FilePath{m_environment.pchBuildDirectory().toStdString(), "dummy.cpp"},
                        generateFakeFileContent(filePathIds),
                        std::move(arguments));
    m_buildDependency.sourceFiles.insert(m_buildDependency.sourceFiles.end(),
                                         filePathIds.begin(),
                                         filePathIds.end());
}

void BuildDependencyCollector::addFile(FilePathId filePathId, Utils::SmallStringVector &&arguments)
{
    addFiles({filePathId}, std::move(arguments));
}

void BuildDependencyCollector::addFile(FilePath filePath,
                                       const FilePathIds &sourceFileIds,
                                       Utils::SmallStringVector &&arguments)
{
    m_clangTool.addFiles({filePath}, std::move(arguments));
    m_buildDependency.sourceFiles.insert(m_buildDependency.sourceFiles.end(),
                                         sourceFileIds.begin(),
                                         sourceFileIds.end());
}

void BuildDependencyCollector::addUnsavedFiles(const V2::FileContainers &unsavedFiles)
{
    m_clangTool.addUnsavedFiles(unsavedFiles);
}

void BuildDependencyCollector::clear()
{
    m_clangTool = ClangTool();
    m_buildDependency.clear();
}

} // namespace ClangBackEnd
