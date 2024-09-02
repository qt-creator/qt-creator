// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "compilationdb.h"

#include "cppeditortr.h"

#include <projectexplorer/abi.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

using namespace ProjectExplorer;
namespace PEConstants = ProjectExplorer::Constants;
using namespace Utils;

namespace CppEditor {

static QStringList projectPartArguments(const ProjectPart &projectPart)
{
    QStringList args;
    args << projectPart.compilerFilePath.toString();
    args << "-c";
    if (projectPart.toolchainType != PEConstants::MSVC_TOOLCHAIN_TYPEID) {
        args << "--target=" + projectPart.toolchainTargetTriple;
        if (projectPart.toolchainAbi.architecture() == Abi::X86Architecture)
            args << QLatin1String(projectPart.toolchainAbi.wordWidth() == 64 ? "-m64" : "-m32");
    }
    args << projectPart.compilerFlags;
    for (const ProjectExplorer::HeaderPath &headerPath : projectPart.headerPaths) {
        if (headerPath.type == HeaderPathType::User) {
            args << "-I" + headerPath.path;
        } else if (headerPath.type == HeaderPathType::System) {
            args << (projectPart.toolchainType == PEConstants::MSVC_TOOLCHAIN_TYPEID
                         ? "-I"
                         : "-isystem")
                        + headerPath.path;
        }
    }
    for (const Macro &macro : projectPart.projectMacros) {
        args.append(
            QString::fromUtf8(macro.toKeyValue(macro.type == MacroType::Define ? "-D" : "-U")));
    }

    return args;
}

static QJsonArray projectPartOptions(const CppEditor::CompilerOptionsBuilder &optionsBuilder)
{
    const QStringList optionsList = optionsBuilder.options();
    QJsonArray optionsArray;
    for (const QString &opt : optionsList) {
        // These will be added later by the file-specific code, and they trigger warnings
        // if they appear twice; see QTCREATORBUG-26664.
        if (opt != "-TP" && opt != "-TC")
            optionsArray << opt;
    }
    return optionsArray;
}

static QJsonArray fullProjectPartOptions(const QJsonArray &projectPartOptions,
                                  const QJsonArray &projectOptions)
{
    QJsonArray fullProjectPartOptions = projectPartOptions;
    for (const QJsonValue &opt : projectOptions)
        fullProjectPartOptions.prepend(opt);
    return fullProjectPartOptions;
}

QJsonArray fullProjectPartOptions(const CppEditor::CompilerOptionsBuilder &optionsBuilder,
                                  const QStringList &projectOptions)
{
    return fullProjectPartOptions(projectPartOptions(optionsBuilder),
                                  QJsonArray::fromStringList(projectOptions));
}

QJsonArray clangOptionsForFile(const ProjectFile &file, const ProjectPart &projectPart,
                               const QJsonArray &generalOptions, UsePrecompiledHeaders usePch,
                               bool clStyle)
{
    CompilerOptionsBuilder optionsBuilder(projectPart);
    optionsBuilder.setClStyle(clStyle);
    ProjectFile::Kind fileKind = file.kind;
    if (fileKind == ProjectFile::AmbiguousHeader) {
        fileKind = projectPart.languageVersion <= LanguageVersion::LatestC
                       ? ProjectFile::CHeader : ProjectFile::CXXHeader;
    }
    if (usePch == UsePrecompiledHeaders::Yes
        && projectPart.precompiledHeaders.contains(file.path.path())) {
        usePch = UsePrecompiledHeaders::No;
    }
    optionsBuilder.updateFileLanguage(fileKind);
    optionsBuilder.addPrecompiledHeaderOptions(usePch);
    const QJsonArray specificOptions = QJsonArray::fromStringList(optionsBuilder.options());
    QJsonArray fullOptions = generalOptions;
    for (const QJsonValue &opt : specificOptions)
        fullOptions << opt;
    return fullOptions;
}

static QJsonObject createFileObject(const FilePath &buildDir,
                                    const QStringList &arguments,
                                    const ProjectPart &projectPart,
                                    const ProjectFile &projFile,
                                    CompilationDbPurpose purpose,
                                    const QJsonArray &projectPartOptions,
                                    UsePrecompiledHeaders usePch,
                                    bool clStyle)
{
    QJsonObject fileObject;
    fileObject["file"] = projFile.path.toString();
    QJsonArray args;

    if (purpose == CompilationDbPurpose::Project) {
        args = QJsonArray::fromStringList(arguments);

        const ProjectFile::Kind kind = ProjectFile::classify(projFile.path.path());
        if (projectPart.toolchainType == ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID
            || projectPart.toolchainType == ProjectExplorer::Constants::CLANG_CL_TOOLCHAIN_TYPEID) {
            if (!ProjectFile::isObjC(kind)) {
                if (ProjectFile::isC(kind))
                    args.append("/TC");
                else if (ProjectFile::isCxx(kind))
                    args.append("/TP");
            }
        } else {
            QStringList langOption
                = createLanguageOptionGcc(projectPart.language, kind,
                                          projectPart.languageExtensions
                                              & LanguageExtension::ObjectiveC);
            for (const QString &langOptionPart : langOption)
                args.append(langOptionPart);
        }
    } else {
        args = clangOptionsForFile(projFile, projectPart, projectPartOptions, usePch, clStyle);
        if (purpose == CompilationDbPurpose::Analysis && projFile.isHeader())
            args << "-Wno-pragma-once-outside-header";
        args.prepend("clang"); // TODO: clang-cl for MSVC targets? Does it matter at all what we put here?
    }

    args.append(projFile.path.toUserOutput());
    fileObject["arguments"] = args;
    fileObject["directory"] = buildDir.toString();
    return fileObject;
}

void generateCompilationDB(
    QPromise<GenerateCompilationDbResult> &promise,
    const QList<ProjectInfo::ConstPtr> &projectInfoList,
    const FilePath &baseDir,
    CompilationDbPurpose purpose,
    const QStringList &projectOptions,
    const GetOptionsBuilder &getOptionsBuilder)
{
    const QString fileName = "compile_commands.json";
    QTC_ASSERT(
        !baseDir.isEmpty(),
        promise.addResult(make_unexpected(Tr::tr("Invalid location for %1.").arg(fileName)));
        return);
    QTC_CHECK(baseDir.ensureWritableDir());
    QFile compileCommandsFile(baseDir.pathAppended(fileName).toFSPathString());
    const bool fileOpened = compileCommandsFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
    if (!fileOpened) {
        promise.addResult(make_unexpected(
            Tr::tr("Could not create \"%1\": %2")
                .arg(compileCommandsFile.fileName(), compileCommandsFile.errorString())));
        return;
    }
    compileCommandsFile.write("[");

    const QJsonArray jsonProjectOptions = QJsonArray::fromStringList(projectOptions);
    for (const ProjectInfo::ConstPtr &projectInfo : std::as_const(projectInfoList)) {
        QTC_ASSERT(projectInfo, continue);
        for (ProjectPart::ConstPtr projectPart : projectInfo->projectParts()) {
            QTC_ASSERT(projectPart, continue);
            QStringList args;
            const CompilerOptionsBuilder optionsBuilder = getOptionsBuilder(*projectPart);
            QJsonArray ppOptions;
            if (purpose == CompilationDbPurpose::Project) {
                args = projectPartArguments(*projectPart);
            } else {
                ppOptions = fullProjectPartOptions(projectPartOptions(optionsBuilder),
                                                   jsonProjectOptions);
            }
            for (const ProjectFile &projFile : projectPart->files) {
                if (promise.isCanceled())
                    return;
                const QJsonObject json
                    = createFileObject(baseDir,
                                       args,
                                       *projectPart,
                                       projFile,
                                       purpose,
                                       ppOptions,
                                       projectInfo->settings().usePrecompiledHeaders(),
                                       optionsBuilder.isClStyle());
                if (compileCommandsFile.size() > 1)
                    compileCommandsFile.write(",");
                compileCommandsFile.write(QJsonDocument(json).toJson(QJsonDocument::Compact));
            }
        }
    }

    compileCommandsFile.write("]");
    compileCommandsFile.close();
    promise.addResult(FilePath::fromUserInput(compileCommandsFile.fileName()));

}

} // namespace CppEditor
