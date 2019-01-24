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


bool PchCreator::generatePch()
{
    clang::tooling::ClangTool tool = m_clangTool.createOutputTool();

    auto action = std::make_unique<GeneratePCHActionFactory>();

    return tool.run(action.get()) != 1;
}

FilePath PchCreator::generatePchHeaderFilePath() const
{
    std::uniform_int_distribution<std::mt19937_64::result_type> distribution;

    return FilePathView{Utils::PathString{Utils::SmallString(m_environment.pchBuildDirectory()),
                                          "/",
                                          std::to_string(distribution(randomNumberGenator)),
                                          ".h"}};
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

std::vector<std::string> PchCreator::generateClangCompilerArguments(
    const PchTask &pchTask,
    FilePathView sourceFilePath,
    FilePathView pchOutputPath)
{
    CommandLineBuilder<PchTask> builder{pchTask,
                                        pchTask.toolChainArguments,
                                        sourceFilePath,
                                        pchOutputPath,
                                        pchTask.systemPchPath};

    return builder.commandLine;
}

void PchCreator::generatePch(PchTask &&pchTask)
{
    long long lastModified = QDateTime::currentSecsSinceEpoch();
    auto content = generatePchIncludeFileContent(pchTask.includes);
    auto pchSourceFilePath = generatePchHeaderFilePath();
    auto pchOutputPath = generatePchFilePath();
    generateFileWithContent(pchSourceFilePath, content);

    m_clangTool.addFile(
        pchSourceFilePath.directory(),
        pchSourceFilePath.name(),
        "",
        generateClangCompilerArguments(pchTask, pchSourceFilePath, pchOutputPath));

    bool success = generatePch();

    m_projectPartPch.projectPartId = pchTask.projectPartId();

    if (success) {
        m_allInclues = pchTask.allIncludes;
        m_projectPartPch.pchPath = std::move(pchOutputPath);
        m_projectPartPch.lastModified = lastModified;
    }
}

const ProjectPartPch &PchCreator::projectPartPch()
{
    return m_projectPartPch;
}

void PchCreator::setUnsavedFiles(const V2::FileContainers &fileContainers)
{
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
}

void PchCreator::doInMainThreadAfterFinished()
{
    m_clangPathwatcher.updateIdPaths({{m_projectPartPch.projectPartId, m_allInclues}});
    m_pchManagerClient.precompiledHeadersUpdated(ProjectPartPchs{m_projectPartPch});
}

const FilePathCaching &PchCreator::filePathCache()
{
    return m_filePathCache;
}

std::unique_ptr<QFile> PchCreator::generateFileWithContent(const Utils::SmallString &filePath,
                                                           const Utils::SmallString &content)
{
    std::unique_ptr<QFile> precompiledIncludeFile(new QFile(QString(filePath)));

    precompiledIncludeFile->open(QIODevice::WriteOnly);

    precompiledIncludeFile->write(content.data(), qint64(content.size()));
    precompiledIncludeFile->close();

    return precompiledIncludeFile;
}

} // namespace ClangBackEnd
