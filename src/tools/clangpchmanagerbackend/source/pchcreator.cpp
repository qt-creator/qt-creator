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

#include "environment.h"
#include "builddependencycollector.h"
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
template <typename Source,
          typename Target>
void append(Target &target, const Source &source)
{
    using ValueType = typename Target::value_type;
    Source clonedSource = source.clone();

    target.reserve(target.size() + source.size());

    for (auto &&entry : clonedSource)
        target.push_back(ValueType(std::move(entry)));
}

void appendFilePathId(ClangBackEnd::FilePaths &target,
                      const ClangBackEnd::FilePathIds &source,
                      const ClangBackEnd::FilePathCachingInterface &filePathCache)
{
    for (FilePathId id : source)
        target.emplace_back(filePathCache.filePath(id));
}

Utils::PathStringVector generatedFilePaths(const V2::FileContainers &generaredFiles)
{
    Utils::PathStringVector generaredFilePaths;
    generaredFilePaths.reserve(generaredFiles.size());

    for (const V2::FileContainer &generatedFile : generaredFiles)
        generaredFilePaths.push_back(generatedFile.filePath.path());

    return generaredFilePaths;
}

}

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


bool PchCreator::generatePch(Utils::SmallStringVector &&compilerArguments)
{
    QProcess process;

    process.setProcessChannelMode(QProcess::ForwardedChannels);
    process.setArguments(QStringList(compilerArguments));
    process.setProgram(QString(m_environment.clangCompilerPath()));

    process.start();
    process.waitForFinished(300000);

    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}

QStringList PchCreator::convertToQStringList(const Utils::SmallStringVector &compilerArguments)
{
    QStringList qStringList;

    append(qStringList, compilerArguments);

    return qStringList;
}

namespace {

void hashProjectPart(QCryptographicHash &hash, const V2::ProjectPartContainer &projectPart)
{
    const auto &projectPartId = projectPart.projectPartId;
    hash.addData(projectPartId.data(), int(projectPartId.size()));

    for (const auto &argument : projectPart.arguments)
        hash.addData(argument.data(), int(argument.size()));
}
}

Utils::SmallStringVector PchCreator::generateProjectPartCommandLine(
        const V2::ProjectPartContainer &projectPart) const
{
    const Utils::SmallStringVector &arguments = projectPart.arguments;

    Utils::SmallStringVector commandLine;
    commandLine.reserve(arguments.size() + 1);

    commandLine.emplace_back(m_environment.clangCompilerPath());

    append(commandLine , arguments);

    return commandLine;
}

Utils::SmallString PchCreator::generateProjectPartPchFilePathWithoutExtension(
        const V2::ProjectPartContainer &projectPart) const
{
    QByteArray fileName = m_environment.pchBuildDirectory().toUtf8();
    fileName += '/';
    fileName += projectPartHash(projectPart);

    return Utils::SmallString::fromQByteArray(fileName);
}

Utils::PathStringVector PchCreator::generateProjectPartHeaders(
        const V2::ProjectPartContainer &projectPart) const
{
   Utils::PathStringVector headerPaths;
   headerPaths.reserve(projectPart.headerPathIds.size() + m_unsavedFiles.size());

   std::transform(projectPart.headerPathIds.begin(),
                  projectPart.headerPathIds.end(),
                  std::back_inserter(headerPaths),
                  [&] (FilePathId filePathId) {
       return m_filePathCache.filePath(filePathId);
   });

   Utils::PathStringVector generatedPath = generatedFilePaths(m_unsavedFiles);

   std::copy(std::make_move_iterator(generatedPath.begin()),
             std::make_move_iterator(generatedPath.end()),
             std::back_inserter(headerPaths));

   return headerPaths;
}

namespace {

std::size_t sizeOfContent(const ClangBackEnd::FilePaths &paths)
{
    return std::accumulate(paths.begin(),
                           paths.end(),
                           std::size_t(0),
                           [] (std::size_t size, const auto &path) {
        const char includeTemplate[] = "#include \"\"\n";
        return size + path.size() + sizeof(includeTemplate);
    });
}

Utils::SmallString concatContent(const ClangBackEnd::FilePaths &paths, std::size_t size)
{
    Utils::SmallString content;
    content.reserve(size);

    for (const ClangBackEnd::FilePath &path : paths) {
        content += "#include \"";
        content +=  path;
        content += "\"\n";
    };

    return content;
}

}

Utils::SmallString PchCreator::generateProjectPartSourcesContent(
        const V2::ProjectPartContainer &projectPart) const
{
    ClangBackEnd::FilePaths paths = generateProjectPartSourcePaths(projectPart);

    return concatContent(paths, sizeOfContent(paths));
}

ClangBackEnd::FilePaths PchCreator::generateProjectPartSourcePaths(
    const V2::ProjectPartContainer &projectPart) const
{
    ClangBackEnd::FilePaths includeAndSources;
    includeAndSources.reserve(projectPart.sourcePathIds.size());

    appendFilePathId(includeAndSources, projectPart.sourcePathIds, m_filePathCache);

    return includeAndSources;
}

SourceEntries PchCreator::generateProjectPartPchIncludes(
        const V2::ProjectPartContainer &projectPart) const
{
    Utils::SmallString jointedFileContent = generateProjectPartSourcesContent(projectPart);
    Utils::SmallString jointedFilePath = generateProjectPartSourceFilePath(projectPart);
    auto jointFile = generateFileWithContent(jointedFilePath, jointedFileContent);
    Utils::SmallStringVector arguments = generateProjectPartCommandLine(projectPart);
    FilePath filePath{Utils::PathString(jointedFilePath)};

    BuildDependencyCollector collector(m_filePathCache);

    collector.setExcludedFilePaths(generateProjectPartSourcePaths(projectPart));

    collector.addFile(filePath, projectPart.sourcePathIds, arguments);

    collector.addUnsavedFiles(m_unsavedFiles);

    collector.collect();

    jointFile->remove();

    return collector.includeIds();
}

Utils::SmallString PchCreator::generateProjectPathPchHeaderFilePath(
        const V2::ProjectPartContainer &projectPart) const
{
    return Utils::SmallString{generateProjectPartPchFilePathWithoutExtension(projectPart), ".h"};
}

Utils::SmallString PchCreator::generateProjectPartPchFilePath(
        const V2::ProjectPartContainer &projectPart) const
{
    return Utils::SmallString{generateProjectPartPchFilePathWithoutExtension(projectPart), ".pch"};
}

Utils::SmallString PchCreator::generateProjectPartSourceFilePath(const V2::ProjectPartContainer &projectPart) const
{
    return Utils::SmallString{generateProjectPartPchFilePathWithoutExtension(projectPart), ".cpp"};
}

Utils::SmallStringVector PchCreator::generateProjectPartPchCompilerArguments(
        const V2::ProjectPartContainer &projectPart) const
{
    Utils::SmallStringVector arguments;
    arguments.reserve(5);

    arguments.emplace_back("-x");
    arguments.emplace_back("c++-header");
    arguments.emplace_back("-Xclang");
    arguments.emplace_back("-emit-pch");
    arguments.emplace_back("-o");
    arguments.emplace_back(generateProjectPartPchFilePath(projectPart));
    arguments.emplace_back(generateProjectPathPchHeaderFilePath(projectPart));

    return arguments;
}

Utils::SmallStringVector PchCreator::generateProjectPartClangCompilerArguments(
        const V2::ProjectPartContainer &projectPart) const
{
    Utils::SmallStringVector compilerArguments = projectPart.arguments.clone();
    const auto pchArguments = generateProjectPartPchCompilerArguments(projectPart);

    append(compilerArguments, pchArguments);

    return compilerArguments;
}

IdPaths PchCreator::generateProjectPartPch(const V2::ProjectPartContainer &projectPart)
{
    long long lastModified = QDateTime::currentSecsSinceEpoch();
    auto includes = generateProjectPartPchIncludes(projectPart);
    auto content = generatePchIncludeFileContent(topIncludeIds(includes));
    auto pchIncludeFilePath = generateProjectPathPchHeaderFilePath(projectPart);
    auto pchFilePath = generateProjectPartPchFilePath(projectPart);
    generateFileWithContent(pchIncludeFilePath, content);

    bool success = generatePch(generateProjectPartClangCompilerArguments(projectPart));

    m_projectPartPch.projectPartId = projectPart.projectPartId;

    if (success) {
        m_projectPartPch.pchPath = std::move(pchFilePath);
        m_projectPartPch.lastModified = lastModified;
    }

    return {projectPart.projectPartId.clone(), allIncludeIds(includes)};
}

void PchCreator::generatePch(const V2::ProjectPartContainer &projectPart)
{
    m_projectIncludeIds = generateProjectPartPch(projectPart);
}

IdPaths PchCreator::takeProjectIncludes()
{
    return std::move(m_projectIncludeIds);
}

const ProjectPartPch &PchCreator::projectPartPch()
{
    return m_projectPartPch;
}

void PchCreator::setUnsavedFiles(const V2::FileContainers &fileContainers)
{
    m_unsavedFiles = fileContainers;
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
    m_projectPartPch = ProjectPartPch{};
    m_projectIncludeIds = IdPaths{};
}

void PchCreator::doInMainThreadAfterFinished()
{
    m_pchManagerClient.precompiledHeadersUpdated(ProjectPartPchs{m_projectPartPch});
    m_fileSystemWatcher.updateIdPaths({takeProjectIncludes()});
}

const IdPaths &PchCreator::projectIncludes() const
{
    return m_projectIncludeIds;
}

const FilePathCaching &PchCreator::filePathCache()
{
    return m_filePathCache;
}

std::unique_ptr<QFile> PchCreator::generateFileWithContent(
        const Utils::SmallString &filePath,
        const Utils::SmallString &content)
{
    std::unique_ptr<QFile> precompiledIncludeFile(new QFile(QString(filePath)));

    precompiledIncludeFile->open(QIODevice::WriteOnly);

    precompiledIncludeFile->write(content.data(), qint64(content.size()));
    precompiledIncludeFile->close();

    return precompiledIncludeFile;
}

FilePathIds PchCreator::topIncludeIds(const SourceEntries &includes)
{
    FilePathIds topIncludes;
    topIncludes.reserve(includes.size());

    for (SourceEntry include : includes) {
        if (include.sourceType == SourceType::TopInclude)
            topIncludes.push_back(include.sourceId);
    }

    return topIncludes;
}

FilePathIds PchCreator::allIncludeIds(const SourceEntries &includes)
{
    FilePathIds allIncludes;
    allIncludes.reserve(includes.size());

    std::transform(includes.begin(),
                   includes.end(),
                   std::back_inserter(allIncludes),
                   [](auto &&entry) { return entry.sourceId; });

    return allIncludes;
}

QByteArray PchCreator::projectPartHash(const V2::ProjectPartContainer &projectPart)
{
    QCryptographicHash hash(QCryptographicHash::Sha1);

    hashProjectPart(hash, projectPart);

    auto result = hash.result();

    return result.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
}

} // namespace ClangBackEnd
