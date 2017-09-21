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
#include "includecollector.h"
#include "pchnotcreatederror.h"

#include <filepathcachinginterface.h>
#include <projectpartpch.h>

#include <QCryptographicHash>
#include <QFile>
#include <QTemporaryFile>

namespace ClangBackEnd {

PchCreator::PchCreator(Environment &environment, FilePathCachingInterface &filePathCache)
   : m_environment(environment),
     m_filePathCache(filePathCache)
{
}

PchCreator::PchCreator(V2::ProjectPartContainers &&projectsParts,
                       Environment &environment,
                       FilePathCachingInterface &filePathCache,
                       PchGeneratorInterface *pchGenerator,
                       V2::FileContainers &&generatedFiles)
    : m_projectParts(std::move(projectsParts)),
      m_generatedFiles(std::move(generatedFiles)),
      m_environment(environment),
      m_filePathCache(filePathCache),
      m_pchGenerator(pchGenerator)
{
}

void PchCreator::setGeneratedFiles(V2::FileContainers &&generatedFiles)
{
    m_generatedFiles = generatedFiles;
}

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

template <typename Source,
          typename Target>
void append(Target &target, Source &source)
{
    using ValueType = typename Target::value_type;
    target.reserve(target.size() + source.size());

    for (auto &&entry : source)
        target.push_back(ValueType(entry));
}

template <typename GetterFunction>
std::size_t globalCount(const V2::ProjectPartContainers &projectsParts,
                        GetterFunction getterFunction)
{
    auto sizeFunction = [&] (std::size_t size, const V2::ProjectPartContainer &projectContainer) {
        return size + getterFunction(projectContainer).size();
    };

    return std::accumulate(projectsParts.begin(),
                           projectsParts.end(),
                           std::size_t(0),
                           sizeFunction);
}

template <typename Container,
          typename GetterFunction>
void generateGlobal(Container &entries,
               const V2::ProjectPartContainers &projectsParts,
               GetterFunction getterFunction)
{
    entries.reserve(entries.capacity() + globalCount(projectsParts, getterFunction));

    for (const V2::ProjectPartContainer &projectPart : projectsParts) {
        const auto &projectPartPaths = getterFunction(projectPart);

        append(entries, projectPartPaths);
    };
}

template <typename Container,
          typename GetterFunction>
Utils::PathStringVector generateGlobal(
        const V2::ProjectPartContainers &projectsParts,
        GetterFunction getterFunction,
        std::size_t prereserve = 0)
{
    Container entries;
    entries.reserve(prereserve);

    generateGlobal(entries, projectsParts, getterFunction);

    return entries;
}


Utils::PathStringVector generatedFilePaths(const V2::FileContainers &generaredFiles)
{
    Utils::PathStringVector generaredFilePaths;
    generaredFilePaths.reserve(generaredFiles.size());

    for (const V2::FileContainer &generatedFile : generaredFiles)
        generaredFilePaths.push_back(generatedFile.filePath().path());

    return generaredFilePaths;
}

}

Utils::PathStringVector PchCreator::generateGlobalHeaderPaths() const
{
    auto includeFunction = [] (const V2::ProjectPartContainer &projectPart)
            -> const Utils::PathStringVector & {
        return projectPart.headerPaths();
    };

    Utils::PathStringVector headerPaths = generateGlobal<Utils::PathStringVector>(m_projectParts,
                                                                                  includeFunction,
                                                                                  m_generatedFiles.size());

    Utils::PathStringVector generatedPath = generatedFilePaths(m_generatedFiles);

    headerPaths.insert(headerPaths.end(),
                       std::make_move_iterator(generatedPath.begin()),
                       std::make_move_iterator(generatedPath.end()));

    return headerPaths;
}

Utils::PathStringVector PchCreator::generateGlobalSourcePaths() const
{
    auto sourceFunction = [] (const V2::ProjectPartContainer &projectPart)
            -> const Utils::PathStringVector & {
        return projectPart.sourcePaths();
    };

    return generateGlobal<Utils::PathStringVector>(m_projectParts, sourceFunction);
}

Utils::PathStringVector PchCreator::generateGlobalHeaderAndSourcePaths() const
{
    const auto &sourcePaths = generateGlobalSourcePaths();
    auto includePaths = generateGlobalHeaderPaths();

    append(includePaths, sourcePaths);

    return includePaths;
}

Utils::SmallStringVector PchCreator::generateGlobalArguments() const
{
    Utils::SmallStringVector arguments;

    auto argumentFunction = [] (const V2::ProjectPartContainer &projectPart)
            -> const Utils::SmallStringVector & {
        return projectPart.arguments();
    };

    generateGlobal(arguments, m_projectParts, argumentFunction);

    return arguments;
}

Utils::SmallStringVector PchCreator::generateGlobalCommandLine() const
{
    Utils::SmallStringVector commandLine;
    commandLine.emplace_back(m_environment.clangCompilerPath());

    auto argumentFunction = [] (const V2::ProjectPartContainer &projectPart)
            -> const Utils::SmallStringVector & {
        return projectPart.arguments();
    };

    generateGlobal(commandLine, m_projectParts, argumentFunction);

    return commandLine;
}

Utils::SmallStringVector PchCreator::generateGlobalPchCompilerArguments() const
{
    Utils::SmallStringVector arguments;
    arguments.reserve(5);

    arguments.emplace_back("-x");
    arguments.emplace_back("c++-header");
    arguments.emplace_back("-Xclang");
    arguments.emplace_back("-emit-pch");
    arguments.emplace_back("-o");
    arguments.emplace_back(generateGlobalPchFilePath());
    arguments.emplace_back(generateGlobalPchHeaderFilePath());

    return arguments;
}

Utils::SmallStringVector PchCreator::generateGlobalClangCompilerArguments() const
{
    auto compilerArguments = generateGlobalArguments();
    const auto pchArguments = generateGlobalPchCompilerArguments();

    append(compilerArguments, pchArguments);

    return compilerArguments;
}

FilePathIds PchCreator::generateGlobalPchIncludeIds() const
{
    IncludeCollector collector(m_filePathCache);

    collector.setExcludedIncludes(generateGlobalHeaderAndSourcePaths());

    collector.addFiles(generateGlobalHeaderAndSourcePaths(), generateGlobalCommandLine());

    collector.addUnsavedFiles(m_generatedFiles);

    collector.collectIncludes();

    return  collector.takeIncludeIds();
}

namespace {

std::size_t contentSize(const FilePaths &includes)
{
    auto countIncludeSize = [] (std::size_t size, const Utils::PathString &include) {
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

    for (const Utils::PathString &include : includes)
        fileContent += {"#include \"", include, "\"\n"};

    return fileContent;
}

Utils::SmallString PchCreator::generateGlobalPchHeaderFileContent() const
{
    return generatePchIncludeFileContent(generateGlobalPchIncludeIds());
}

std::unique_ptr<QFile> PchCreator::generateGlobalPchHeaderFile()
{
    return generateFileWithContent(generateGlobalPchHeaderFilePath(),
                                 generateGlobalPchHeaderFileContent());
}

void PchCreator::generatePch(Utils::SmallStringVector &&compilerArguments,
                             ProjectPartPch &&projectPartPch)
{
    m_pchGenerator->startTask(std::move(compilerArguments), std::move(projectPartPch));
}

void PchCreator::generateGlobalPch()
{
    generateGlobalPchHeaderFile();

    generatePch(generateGlobalClangCompilerArguments(), ProjectPartPch());
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
    const auto &projectPartId = projectPart.projectPartId();
    hash.addData(projectPartId.data(), projectPartId.size());

    for (const auto &argument : projectPart.arguments())
        hash.addData(argument.data(), argument.size());
}
}

QByteArray PchCreator::globalProjectHash() const
{
    QCryptographicHash hash(QCryptographicHash::Sha1);

    for (const auto &projectPart : m_projectParts)
        hashProjectPart(hash, projectPart);

    auto result = hash.result();

    return result.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
}

Utils::SmallString PchCreator::generateGlobalPchFilePathWithoutExtension() const
{
    QByteArray fileName = m_environment.pchBuildDirectory().toUtf8();
    fileName += '/';
    fileName += globalProjectHash();

    return Utils::SmallString::fromQByteArray(fileName);
}

Utils::SmallString PchCreator::generateGlobalPchHeaderFilePath() const
{
    Utils::SmallString filePath = generateGlobalPchFilePathWithoutExtension();

    filePath += ".h";

    return filePath;
}

Utils::SmallString PchCreator::generateGlobalPchFilePath() const
{
    Utils::SmallString filePath = generateGlobalPchFilePathWithoutExtension();

    filePath += ".pch";

    return filePath;
}

Utils::SmallStringVector PchCreator::generateProjectPartCommandLine(
        const V2::ProjectPartContainer &projectPart) const
{
    const Utils::SmallStringVector &arguments = projectPart.arguments();

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
   headerPaths.reserve(projectPart.headerPaths().size() + m_generatedFiles.size());

   std::copy(projectPart.headerPaths().begin(),
             projectPart.headerPaths().end(),
             std::back_inserter(headerPaths));

   Utils::PathStringVector generatedPath = generatedFilePaths(m_generatedFiles);

   std::copy(std::make_move_iterator(generatedPath.begin()),
             std::make_move_iterator(generatedPath.end()),
             std::back_inserter(headerPaths));

   return headerPaths;
}

namespace {

std::size_t sizeOfContent(const Utils::PathStringVector &paths)
{
    return std::accumulate(paths.begin(),
                           paths.end(),
                           std::size_t(0),
                           [] (std::size_t size, const Utils::PathString &path) {
        const char includeTemplate[] = "#include \"\"\n";
        return size + path.size() + sizeof(includeTemplate);
    });
}

Utils::SmallString concatContent(const Utils::PathStringVector &paths, std::size_t size)
{
    Utils::SmallString content;
    content.reserve(size);

    for (const Utils::PathString &path : paths) {
        content += "#include \"";
        content +=  path;
        content += "\"\n";
    };

    return content;
}

}

Utils::SmallString PchCreator::generateProjectPartHeaderAndSourcesContent(
        const V2::ProjectPartContainer &projectPart)
{
    Utils::PathStringVector paths = generateProjectPartHeaderAndSourcePaths(projectPart);

    return concatContent(paths, sizeOfContent(paths));
}

Utils::PathStringVector PchCreator::generateProjectPartHeaderAndSourcePaths(
        const V2::ProjectPartContainer &projectPart)
{
    Utils::PathStringVector includeAndSources;
    includeAndSources.reserve(projectPart.headerPaths().size() + projectPart.sourcePaths().size());

    append(includeAndSources, projectPart.headerPaths());
    append(includeAndSources, projectPart.sourcePaths());

    return includeAndSources;
}

FilePathIds PchCreator::generateProjectPartPchIncludes(
        const V2::ProjectPartContainer &projectPart) const
{
    Utils::SmallString jointedFileContent = generateProjectPartHeaderAndSourcesContent(projectPart);
    Utils::SmallString jointedFilePath = generateProjectPartSourceFilePath(projectPart);
    auto jointFile = generateFileWithContent(jointedFilePath, jointedFileContent);
    Utils::SmallStringVector arguments = generateProjectPartCommandLine(projectPart);
    arguments.push_back(jointedFilePath);
    FilePath filePath{Utils::PathString(jointedFilePath)};

    IncludeCollector collector(m_filePathCache);

    collector.setExcludedIncludes(generateProjectPartHeaderAndSourcePaths(projectPart));

    collector.addFile(std::string(filePath.directory()),
                      std::string(filePath.name()),
                      {},
                      arguments);

    collector.addUnsavedFiles(m_generatedFiles);

    collector.collectIncludes();

    jointFile->remove();

    return  collector.takeIncludeIds();
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
//    arguments.emplace_back("-Xclang");
//    arguments.emplace_back("-include-pch");
//    arguments.emplace_back("-Xclang");
//    arguments.emplace_back(generateGlobalPchFilePath());
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
    Utils::SmallStringVector compilerArguments = projectPart.arguments().clone();
    const auto pchArguments = generateProjectPartPchCompilerArguments(projectPart);

    append(compilerArguments, pchArguments);

    return compilerArguments;
}

IdPaths PchCreator::generateProjectPartPch(const V2::ProjectPartContainer &projectPart)
{
    auto includes = generateProjectPartPchIncludes(projectPart);
    auto content = generatePchIncludeFileContent(includes);
    auto pchIncludeFilePath = generateProjectPathPchHeaderFilePath(projectPart);
    auto pchFilePath = generateProjectPartPchFilePath(projectPart);
    generateFileWithContent(pchIncludeFilePath, content);

    generatePch(generateProjectPartClangCompilerArguments(projectPart),
                {projectPart.projectPartId().clone(), std::move(pchFilePath)});

    return {projectPart.projectPartId().clone(), std::move(includes)};
}

void PchCreator::generatePchs()
{
    for (const V2::ProjectPartContainer &projectPart : m_projectParts) {
        auto includePaths = generateProjectPartPch(projectPart);
        m_projectsIncludeIds.push_back(std::move(includePaths));
    }
}

void PchCreator::generatePchs(V2::ProjectPartContainers &&projectsParts)
{
    m_projectParts = std::move(projectsParts);

    generatePchs();
}

std::vector<IdPaths> PchCreator::takeProjectsIncludes()
{
    return std::move(m_projectsIncludeIds);
}

void PchCreator::setGenerator(PchGeneratorInterface *pchGenerator)
{
    m_pchGenerator = pchGenerator;
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

QByteArray PchCreator::projectPartHash(const V2::ProjectPartContainer &projectPart)
{
    QCryptographicHash hash(QCryptographicHash::Sha1);

    hashProjectPart(hash, projectPart);

    auto result = hash.result();

    return result.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
}

} // namespace ClangBackEnd
