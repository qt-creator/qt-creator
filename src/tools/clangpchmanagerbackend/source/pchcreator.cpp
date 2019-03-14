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

#include "pchcreator.h"

#include "builddependencycollector.h"
#include "commandlinebuilder.h"
#include "environment.h"
#include "generatepchactionfactory.h"
#include "pchnotcreatederror.h"

#include <builddependenciesstorageinterface.h>
#include <clangpathwatcherinterface.h>
#include <filepathcachinginterface.h>
#include <generatedfiles.h>
#include <pchmanagerclientinterface.h>
#include <precompiledheadersupdatedmessage.h>
#include <projectpartpch.h>

#include <QCryptographicHash>
#include <QDateTime>
#include <QFile>
#include <QProcess>
#include <QTemporaryFile>

namespace ClangBackEnd {

namespace {

std::size_t contentSize(const FilePaths &includes)
{
    auto countIncludeSize = [] (std::size_t size, const auto &include) {
        return size + include.size();
    };

    return std::accumulate(includes.begin(), includes.end(), std::size_t(0), countIncludeSize);
}
}

Utils::SmallString PchCreator::generatePchIncludeFileContent(const FilePathIds &includeIds) const
{
    Utils::SmallString fileContent;
    const std::size_t lineTemplateSize = 12;
    auto includes = m_filePathCache.filePaths(includeIds);

    fileContent.reserve(includes.size() * lineTemplateSize + contentSize(includes));

    for (Utils::SmallStringView include : includes)
        fileContent += {"#include \"", include, "\"\n"};

    return fileContent;
}

bool PchCreator::generatePch(NativeFilePathView path, Utils::SmallStringView content)
{
    clang::tooling::ClangTool tool = m_clangTool.createOutputTool();

    auto action = std::make_unique<GeneratePCHActionFactory>(llvm::StringRef{path.data(),
                                                                             path.size()},
                                                             llvm::StringRef{content.data(),
                                                                             content.size()});

    return tool.run(action.get()) != 1;
}

FilePath PchCreator::generatePchFilePath() const
{
    std::uniform_int_distribution<std::uint_fast64_t> distribution(
        1, std::numeric_limits<std::uint_fast64_t>::max());

    return FilePathView{Utils::PathString{Utils::SmallString(m_environment.pchBuildDirectory()),
                                          "/",
                                          std::to_string(distribution(randomNumberGenator)),
                                          ".pch"}};
}

Utils::SmallStringVector PchCreator::generateClangCompilerArguments(const PchTask &pchTask,
                                                                    FilePathView pchOutputPath)
{
    CommandLineBuilder<PchTask> builder{pchTask,
                                        pchTask.toolChainArguments,
                                        InputFileType::Header,
                                        {},
                                        pchOutputPath,
                                        pchTask.systemPchPath};

    return builder.commandLine;
}

void PchCreator::generatePch(PchTask &&pchTask)
{
    m_projectPartPch.lastModified = QDateTime::currentSecsSinceEpoch();
    auto content = generatePchIncludeFileContent(pchTask.includes);
    auto pchOutputPath = generatePchFilePath();

    FilePath headerFilePath{m_environment.pchBuildDirectory().toStdString(), "dummy.h"};
    Utils::SmallStringVector commandLine = generateClangCompilerArguments(pchTask, pchOutputPath);

    m_clangTool.addFile(std::move(headerFilePath), content.clone(), std::move(commandLine));
    bool success = generatePch(NativeFilePath{headerFilePath}, content);

    m_projectPartPch.projectPartId = pchTask.projectPartId();

    if (success) {
        m_sources = pchTask.sources;
        m_projectPartPch.pchPath = std::move(pchOutputPath);
    }
}

const ProjectPartPch &PchCreator::projectPartPch()
{
    return m_projectPartPch;
}

void PchCreator::setUnsavedFiles(const V2::FileContainers &fileContainers)
{
    m_generatedFilePathIds.clear();
    m_generatedFilePathIds.reserve(fileContainers.size());
    std::transform(fileContainers.begin(),
                   fileContainers.end(),
                   std::back_inserter(m_generatedFilePathIds),
                   [&](const V2::FileContainer &fileContainer) {
                       return m_filePathCache.filePathId(fileContainer.filePath);
                   });
    std::sort(m_generatedFilePathIds.begin(), m_generatedFilePathIds.end());

    m_clangTool.addUnsavedFiles(fileContainers);
}

void PchCreator::setIsUsed(bool isUsed)
{
    m_isUsed = isUsed;
}

bool PchCreator::isUsed() const
{
    return m_isUsed;
}

void PchCreator::clear()
{
    m_clangTool = ClangTool{};
    m_projectPartPch = {};
    m_sources.clear();
}

void PchCreator::doInMainThreadAfterFinished()
{
    FilePathIds existingSources;
    existingSources.reserve(m_sources.size());
    std::set_difference(m_sources.begin(),
                        m_sources.end(),
                        m_generatedFilePathIds.begin(),
                        m_generatedFilePathIds.end(),
                        std::back_inserter(existingSources));
    m_buildDependenciesStorage.updatePchCreationTimeStamp(m_projectPartPch.lastModified,
                                                          m_projectPartPch.projectPartId);
    m_clangPathwatcher.updateIdPaths({{m_projectPartPch.projectPartId, existingSources}});
    m_pchManagerClient.precompiledHeadersUpdated(ProjectPartPchs{m_projectPartPch});
}

const FilePathCaching &PchCreator::filePathCache()
{
    return m_filePathCache;
}

} // namespace ClangBackEnd
