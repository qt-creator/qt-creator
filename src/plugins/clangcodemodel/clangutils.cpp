// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangutils.h"

#include "clangcodemodeltr.h"

#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <cppeditor/baseeditordocumentparser.h>
#include <cppeditor/clangdiagnosticconfigsmodel.h>
#include <cppeditor/compileroptionsbuilder.h>
#include <cppeditor/cppcodemodelsettings.h>
#include <cppeditor/cppmodelmanager.h>
#include <cppeditor/cpptoolsreuse.h>
#include <cppeditor/editordocumenthandle.h>
#include <cppeditor/projectpart.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <texteditor/codeassist/textdocumentmanipulatorinterface.h>

#include <cplusplus/SimpleLexer.h>
#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
#include <QTextBlock>

using namespace Core;
using namespace CppEditor;
using namespace ProjectExplorer;
using namespace Utils;

namespace ClangCodeModel {
namespace Internal {

ProjectPart::ConstPtr projectPartForFile(const FilePath &filePath)
{
    if (const auto parser = CppEditor::BaseEditorDocumentParser::get(filePath))
        return parser->projectPartInfo().projectPart;
    return ProjectPart::ConstPtr();
}

QString diagnosticCategoryPrefixRemoved(const QString &text)
{
    QString theText = text;

    // Prefixes are taken from $LLVM_SOURCE_DIR/tools/clang/lib/Frontend/TextDiagnostic.cpp,
    // function TextDiagnostic::printDiagnosticLevel (llvm-3.6.2).
    static const QStringList categoryPrefixes = {
        QStringLiteral("note: "),
        QStringLiteral("remark: "),
        QStringLiteral("warning: "),
        QStringLiteral("error: "),
        QStringLiteral("fatal error: ")
    };

    for (const QString &fullPrefix : categoryPrefixes) {
        if (theText.startsWith(fullPrefix)) {
            theText.remove(0, fullPrefix.length());
            return theText;
        }
    }

    return text;
}

static QStringList projectPartArguments(const ProjectPart &projectPart)
{
    QStringList args;
    args << projectPart.compilerFilePath.toString();
    args << "-c";
    if (projectPart.toolchainType != ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID) {
        args << "--target=" + projectPart.toolChainTargetTriple;
        if (projectPart.toolChainAbi.architecture() == Abi::X86Architecture)
            args << QLatin1String(projectPart.toolChainAbi.wordWidth() == 64 ? "-m64" : "-m32");
    }
    args << projectPart.compilerFlags;
    for (const ProjectExplorer::HeaderPath &headerPath : projectPart.headerPaths) {
        if (headerPath.type == ProjectExplorer::HeaderPathType::User) {
            args << "-I" + headerPath.path;
        } else if (headerPath.type == ProjectExplorer::HeaderPathType::System) {
            args << (projectPart.toolchainType == ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID
                         ? "-I"
                         : "-isystem")
                        + headerPath.path;
        }
    }
    for (const ProjectExplorer::Macro &macro : projectPart.projectMacros) {
        args.append(QString::fromUtf8(
            macro.toKeyValue(macro.type == ProjectExplorer::MacroType::Define ? "-D" : "-U")));
    }

    return args;
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
        args.prepend("clang"); // TODO: clang-cl for MSVC targets? Does it matter at all what we put here?
    }

    args.append(projFile.path.toUserOutput());
    fileObject["arguments"] = args;
    fileObject["directory"] = buildDir.toString();
    return fileObject;
}

GenerateCompilationDbResult generateCompilationDB(QList<ProjectInfo::ConstPtr> projectInfoList,
                                                  FilePath baseDir,
                                                  CompilationDbPurpose purpose,
                                                  ClangDiagnosticConfig warningsConfig,
                                                  QStringList projectOptions,
                                                  FilePath clangIncludeDir)
{
    QTC_ASSERT(!baseDir.isEmpty(), return GenerateCompilationDbResult(QString(),
        Tr::tr("Could not retrieve build directory.")));
    QTC_ASSERT(!projectInfoList.isEmpty(),
               return GenerateCompilationDbResult(QString(), "Could not retrieve project info."));
    QTC_CHECK(baseDir.ensureWritableDir());
    QFile compileCommandsFile(baseDir.pathAppended("compile_commands.json").toFSPathString());
    const bool fileOpened = compileCommandsFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
    if (!fileOpened) {
        return GenerateCompilationDbResult(QString(), Tr::tr("Could not create \"%1\": %2")
                    .arg(compileCommandsFile.fileName(), compileCommandsFile.errorString()));
    }
    compileCommandsFile.write("[");

    const UsePrecompiledHeaders usePch = getPchUsage();
    const QJsonArray jsonProjectOptions = QJsonArray::fromStringList(projectOptions);
    for (const ProjectInfo::ConstPtr &projectInfo : std::as_const(projectInfoList)) {
        QTC_ASSERT(projectInfo, continue);
        for (ProjectPart::ConstPtr projectPart : projectInfo->projectParts()) {
            QTC_ASSERT(projectPart, continue);
            QStringList args;
            const CompilerOptionsBuilder optionsBuilder = clangOptionsBuilder(
                        *projectPart, warningsConfig, clangIncludeDir, {});
            QJsonArray ppOptions;
            if (purpose == CompilationDbPurpose::Project) {
                args = projectPartArguments(*projectPart);
            } else {
                ppOptions = fullProjectPartOptions(projectPartOptions(optionsBuilder),
                                                   jsonProjectOptions);
            }
            for (const ProjectFile &projFile : projectPart->files) {
                const QJsonObject json = createFileObject(baseDir, args, *projectPart, projFile,
                                                          purpose, ppOptions, usePch,
                                                          optionsBuilder.isClStyle());
                if (compileCommandsFile.size() > 1)
                    compileCommandsFile.write(",");
                compileCommandsFile.write(QJsonDocument(json).toJson(QJsonDocument::Compact));
            }
        }
    }

    compileCommandsFile.write("]");
    compileCommandsFile.close();
    return GenerateCompilationDbResult(compileCommandsFile.fileName(), QString());
}

FilePath currentCppEditorDocumentFilePath()
{
    FilePath filePath;

    const auto currentEditor = Core::EditorManager::currentEditor();
    if (currentEditor && CppEditor::CppModelManager::isCppEditor(currentEditor)) {
        if (const auto currentDocument = currentEditor->document())
            filePath = currentDocument->filePath();
    }

    return filePath;
}

DiagnosticTextInfo::DiagnosticTextInfo(const QString &text)
    : m_text(text)
    , m_squareBracketStartIndex(text.lastIndexOf('['))
{}

QString DiagnosticTextInfo::textWithoutOption() const
{
    if (m_squareBracketStartIndex == -1)
        return m_text;

    return m_text.mid(0, m_squareBracketStartIndex - 1);
}

QString DiagnosticTextInfo::option() const
{
    if (m_squareBracketStartIndex == -1)
        return QString();

    const int index = m_squareBracketStartIndex + 1;
    return m_text.mid(index, m_text.size() - index - 1);
}

QString DiagnosticTextInfo::category() const
{
    if (m_squareBracketStartIndex == -1)
        return QString();

    const int index = m_squareBracketStartIndex + 1;
    if (isClazyOption(m_text.mid(index)))
        return Tr::tr("Clazy Issue");
    else
        return Tr::tr("Clang-Tidy Issue");
}

bool DiagnosticTextInfo::isClazyOption(const QString &option)
{
    return option.startsWith("-Wclazy");
}

QString DiagnosticTextInfo::clazyCheckName(const QString &option)
{
    if (option.startsWith("-Wclazy"))
        return option.mid(8); // Chop "-Wclazy-"
    return option;
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

ClangDiagnosticConfig warningsConfigForProject(Project *project)
{
    return ClangdSettings(ClangdProjectSettings(project).settings()).diagnosticConfig();
}

const QStringList globalClangOptions()
{
    return ClangDiagnosticConfigsModel::globalDiagnosticOptions();
}

// 7.3.3: using typename(opt) nested-name-specifier unqualified-id ;
bool isAtUsingDeclaration(TextEditor::TextDocumentManipulatorInterface &manipulator,
                          int basePosition)
{
    using namespace CPlusPlus;
    SimpleLexer lexer;
    lexer.setLanguageFeatures(LanguageFeatures::defaultFeatures());
    const QString textToLex = textUntilPreviousStatement(manipulator, basePosition);
    const Tokens tokens = lexer(textToLex);
    if (tokens.empty())
        return false;

    // The nested-name-specifier always ends with "::", so check for this first.
    const Token lastToken = tokens[tokens.size() - 1];
    if (lastToken.kind() != T_COLON_COLON)
        return false;

    return contains(tokens, [](const Token &token) { return token.kind() == T_USING; });
}

QString textUntilPreviousStatement(TextEditor::TextDocumentManipulatorInterface &manipulator,
                                   int startPosition)
{
    static const QString stopCharacters(";{}#");

    int endPosition = 0;
    for (int i = startPosition; i >= 0 ; --i) {
        if (stopCharacters.contains(manipulator.characterAt(i))) {
            endPosition = i + 1;
            break;
        }
    }

    return manipulator.textAt(endPosition, startPosition - endPosition);
}

CompilerOptionsBuilder clangOptionsBuilder(const ProjectPart &projectPart,
                                           const ClangDiagnosticConfig &warningsConfig,
                                           const FilePath &clangIncludeDir,
                                           const Macros &extraMacros)
{
    const auto useBuildSystemWarnings = warningsConfig.useBuildSystemWarnings()
            ? UseBuildSystemWarnings::Yes
            : UseBuildSystemWarnings::No;
    CompilerOptionsBuilder optionsBuilder(projectPart, UseSystemHeader::No,
                                          UseTweakedHeaderPaths::Yes, UseLanguageDefines::No,
                                          useBuildSystemWarnings, clangIncludeDir);
    Macros fullMacroList = extraMacros;
    fullMacroList += Macro("Q_CREATOR_RUN", "1");
    optionsBuilder.provideAdditionalMacros(fullMacroList);
    optionsBuilder.build(ProjectFile::Unclassified, UsePrecompiledHeaders::No);
    optionsBuilder.add("-fmessage-length=0", /*gccOnlyOption=*/true);
    optionsBuilder.add("-fdiagnostics-show-note-include-stack", /*gccOnlyOption=*/true);
    optionsBuilder.add("-fretain-comments-from-system-headers", /*gccOnlyOption=*/true);
    optionsBuilder.add("-fmacro-backtrace-limit=0");
    optionsBuilder.add("-ferror-limit=1000");

    if (useBuildSystemWarnings == UseBuildSystemWarnings::No) {
        for (const QString &opt : warningsConfig.clangOptions())
            optionsBuilder.add(opt, true);
    }

    return optionsBuilder;
}

QJsonArray projectPartOptions(const CppEditor::CompilerOptionsBuilder &optionsBuilder)
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

QJsonArray fullProjectPartOptions(const CppEditor::CompilerOptionsBuilder &optionsBuilder,
                                  const QStringList &projectOptions)
{
    return fullProjectPartOptions(projectPartOptions(optionsBuilder),
                                  QJsonArray::fromStringList(projectOptions));
}

QJsonArray fullProjectPartOptions(const QJsonArray &projectPartOptions,
                                  const QJsonArray &projectOptions)
{
    QJsonArray fullProjectPartOptions = projectPartOptions;
    for (const QJsonValue &opt : projectOptions)
        fullProjectPartOptions.prepend(opt);
    return fullProjectPartOptions;
}

} // namespace Internal
} // namespace Clang
