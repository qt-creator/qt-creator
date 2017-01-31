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

#include <projectpartpch.h>

#include <QCryptographicHash>
#include <QFile>

namespace ClangBackEnd {

PchCreator::PchCreator(Environment &environment, StringCache<Utils::SmallString> &filePathCache)
   : m_environment(environment),
     m_filePathCache(filePathCache)
{
}

PchCreator::PchCreator(V2::ProjectPartContainers &&projectsParts,
                       Environment &environment,
                       StringCache<Utils::SmallString> &filePathCache,
                       PchGeneratorInterface *pchGenerator)
    : m_projectParts(std::move(projectsParts)),
      m_environment(environment),
      m_filePathCache(filePathCache),
      m_pchGenerator(pchGenerator)
{
}

namespace {
template <typename Source,
          typename Target>
void append(Target &target, const Source &source)
{
    Source clonedSource = source.clone();

    target.reserve(target.size() + source.size());

    std::move(clonedSource.begin(),
              clonedSource.end(),
              std::back_inserter(target));
}

template <typename Source,
          typename Target>
void append(Target &target, Source &source)
{
    target.reserve(target.size() + source.size());

    std::move(source.begin(),
              source.end(),
              std::back_inserter(target));
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

template <typename GetterFunction>
void generateGlobal(Utils::SmallStringVector &entries,
               const V2::ProjectPartContainers &projectsParts,
               GetterFunction getterFunction)
{
    entries.reserve(entries.size() + globalCount(projectsParts, getterFunction));

    for (const V2::ProjectPartContainer &projectPart : projectsParts) {
        const auto &projectPartPaths = getterFunction(projectPart);

        append(entries, projectPartPaths);
    };
}

template <typename GetterFunction>
Utils::SmallStringVector generateGlobal(
        const V2::ProjectPartContainers &projectsParts,
        GetterFunction getterFunction)
{
    Utils::SmallStringVector entries;

    generateGlobal(entries, projectsParts, getterFunction);

    return entries;
}

}

Utils::SmallStringVector PchCreator::generateGlobalHeaderPaths() const
{
    auto includeFunction = [] (const V2::ProjectPartContainer &projectPart)
            -> const Utils::SmallStringVector & {
        return projectPart.headerPaths();
    };

    return generateGlobal(m_projectParts, includeFunction);
}

Utils::SmallStringVector PchCreator::generateGlobalSourcePaths() const
{
    auto sourceFunction = [] (const V2::ProjectPartContainer &projectPart)
            -> const Utils::SmallStringVector & {
        return projectPart.sourcePaths();
    };

    return generateGlobal(m_projectParts, sourceFunction);
}

Utils::SmallStringVector PchCreator::generateGlobalHeaderAndSourcePaths() const
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

std::vector<uint> PchCreator::generateGlobalPchIncludeIds() const
{
    IncludeCollector collector(m_filePathCache);

    collector.setExcludedIncludes(generateGlobalHeaderPaths());

    collector.addFiles(generateGlobalHeaderAndSourcePaths(), generateGlobalCommandLine());

    collector.collectIncludes();

    return  collector.takeIncludeIds();
}

namespace {

std::size_t contentSize(const std::vector<Utils::SmallString> &includes)
{
    auto countIncludeSize = [] (std::size_t size, const Utils::SmallString &include) {
        return size + include.size();
    };

    return std::accumulate(includes.begin(), includes.end(), std::size_t(0), countIncludeSize);
}
}

Utils::SmallString PchCreator::generatePchIncludeFileContent(
        const std::vector<uint> &includeIds) const
{
    Utils::SmallString fileContent;
    const std::size_t lineTemplateSize = 12;
    auto includes = m_filePathCache.strings(includeIds);

    fileContent.reserve(includes.size() * lineTemplateSize + contentSize(includes));

    for (const Utils::SmallString &include : includes) {
        fileContent += "#include <";
        fileContent += include;
        fileContent += ">\n";
    }

    return fileContent;
}

Utils::SmallString PchCreator::generateGlobalPchHeaderFileContent() const
{
    return generatePchIncludeFileContent(generateGlobalPchIncludeIds());
}

std::unique_ptr<QFile> PchCreator::generateGlobalPchHeaderFile()
{
    return generatePchHeaderFile(generateGlobalPchHeaderFilePath(),
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

Utils::SmallStringVector PchCreator::generateProjectPartHeaderAndSourcePaths(
        const V2::ProjectPartContainer &projectPart)
{
    Utils::SmallStringVector includeAndSources;
    includeAndSources.reserve(projectPart.headerPaths().size() + projectPart.sourcePaths().size());

    append(includeAndSources, projectPart.headerPaths());
    append(includeAndSources, projectPart.sourcePaths());

    return includeAndSources;
}

std::vector<uint> PchCreator::generateProjectPartPchIncludes(
        const V2::ProjectPartContainer &projectPart) const
{
    IncludeCollector collector(m_filePathCache);

    collector.setExcludedIncludes(projectPart.headerPaths().clone());

    collector.addFiles(generateProjectPartHeaderAndSourcePaths(projectPart),
                       generateProjectPartCommandLine(projectPart));

    collector.collectIncludes();

    return  collector.takeIncludeIds();
}

Utils::SmallString PchCreator::generateProjectPathPchHeaderFilePath(
        const V2::ProjectPartContainer &projectPart) const
{
    Utils::SmallString filePath = generateProjectPartPchFilePathWithoutExtension(projectPart);

    filePath += ".h";

    return filePath;
}

Utils::SmallString PchCreator::generateProjectPartPchFilePath(
        const V2::ProjectPartContainer &projectPart) const
{
    Utils::SmallString filePath = generateProjectPartPchFilePathWithoutExtension(projectPart);

    filePath += ".pch";

    return filePath;
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
    generatePchHeaderFile(pchIncludeFilePath, content);

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

std::unique_ptr<QFile> PchCreator::generatePchHeaderFile(
        const Utils::SmallString &filePath,
        const Utils::SmallString &content)
{
    std::unique_ptr<QFile> precompiledIncludeFile(new QFile(filePath));

    precompiledIncludeFile->open(QIODevice::WriteOnly);

    precompiledIncludeFile->write(content.data(), content.size());
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
