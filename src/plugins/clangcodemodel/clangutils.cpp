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

#include "clangutils.h"

#include "clangeditordocumentprocessor.h"
#include "clangmodelmanagersupport.h"
#include "clangprojectsettings.h"

#include <clangsupport/tokeninfocontainer.h>

#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <cppeditor/baseeditordocumentparser.h>
#include <cppeditor/compileroptionsbuilder.h>
#include <cppeditor/cppcodemodelsettings.h>
#include <cppeditor/cppmodelmanager.h>
#include <cppeditor/cpptoolsreuse.h>
#include <cppeditor/editordocumenthandle.h>
#include <cppeditor/projectpart.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/kitinformation.h>
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

class LibClangOptionsBuilder final : public CompilerOptionsBuilder
{
public:
    LibClangOptionsBuilder(const ProjectPart &projectPart,
                           UseBuildSystemWarnings useBuildSystemWarnings)
        : CompilerOptionsBuilder(projectPart,
                                 UseSystemHeader::No,
                                 UseTweakedHeaderPaths::Yes,
                                 UseLanguageDefines::No,
                                 useBuildSystemWarnings,
                                 QString(CLANG_VERSION),
                                 FilePath(CLANG_INCLUDE_DIR))
    {
    }

    void addProjectMacros() final
    {
        addMacros({ProjectExplorer::Macro("Q_CREATOR_RUN", "1")});
        CompilerOptionsBuilder::addProjectMacros();
    }

    void addExtraOptions() final
    {
        addDummyUiHeaderOnDiskIncludePath();
        add("-fmessage-length=0", /*gccOnlyOption=*/true);
        add("-fdiagnostics-show-note-include-stack", /*gccOnlyOption=*/true);
        add("-fretain-comments-from-system-headers", /*gccOnlyOption=*/true);
        add("-fmacro-backtrace-limit=0");
        add("-ferror-limit=1000");
    }

private:
    void addDummyUiHeaderOnDiskIncludePath()
    {
        const QString path = ClangModelManagerSupport::instance()->dummyUiHeaderOnDiskDirPath();
        if (!path.isEmpty()) {
            prepend(QDir::toNativeSeparators(path));
            prepend("-I");
        }
    }
};

ProjectPart::ConstPtr projectPartForFile(const QString &filePath)
{
    if (const auto parser = CppEditor::BaseEditorDocumentParser::get(filePath))
        return parser->projectPartInfo().projectPart;
    return ProjectPart::ConstPtr();
}

ProjectPart::ConstPtr projectPartForFileBasedOnProcessor(const QString &filePath)
{
    if (const auto processor = ClangEditorDocumentProcessor::get(filePath))
        return processor->projectPart();
    return ProjectPart::ConstPtr();
}

bool isProjectPartLoaded(const ProjectPart::ConstPtr projectPart)
{
    if (projectPart)
        return !CppModelManager::instance()->projectPartForId(projectPart->id()).isNull();
    return false;
}

QString projectPartIdForFile(const QString &filePath)
{
    const ProjectPart::ConstPtr projectPart = projectPartForFile(filePath);

    if (isProjectPartLoaded(projectPart))
        return projectPart->id(); // OK, Project Part is still loaded
    return QString();
}

CppEditorDocumentHandle *cppDocument(const QString &filePath)
{
    return CppEditor::CppModelManager::instance()->cppEditorDocument(filePath);
}

void setLastSentDocumentRevision(const QString &filePath, uint revision)
{
    if (CppEditorDocumentHandle *document = cppDocument(filePath))
        document->sendTracker().setLastSentRevision(int(revision));
}

int clangColumn(const QTextBlock &line, int cppEditorColumn)
{
    // (1) cppEditorColumn is the actual column shown by CppEditor.
    // (2) The return value is the column in Clang which is the utf8 byte offset from the beginning
    //     of the line.
    // Here we convert column from (1) to (2).
    // '- 1' and '+ 1' are because of 1-based columns
    return line.text().left(cppEditorColumn - 1).toUtf8().size() + 1;
}

int cppEditorColumn(const QTextBlock &line, int clangColumn)
{
    // (1) clangColumn is the column in Clang which is the utf8 byte offset from the beginning
    //     of the line.
    // (2) The return value is the actual column shown by CppEditor.
    // Here we convert column from (1) to (2).
    // '- 1' and '+ 1' are because of 1-based columns
    return QString::fromUtf8(line.text().toUtf8().left(clangColumn - 1)).size() + 1;
}

CodeModelIcon::Type iconTypeForToken(const ClangBackEnd::TokenInfoContainer &token)
{
    const ClangBackEnd::ExtraInfo &extraInfo = token.extraInfo;
    if (extraInfo.signal)
        return CodeModelIcon::Signal;

    ClangBackEnd::AccessSpecifier access = extraInfo.accessSpecifier;
    if (extraInfo.slot) {
        switch (access) {
        case ClangBackEnd::AccessSpecifier::Public:
        case ClangBackEnd::AccessSpecifier::Invalid:
            return CodeModelIcon::SlotPublic;
        case ClangBackEnd::AccessSpecifier::Protected:
            return CodeModelIcon::SlotProtected;
        case ClangBackEnd::AccessSpecifier::Private:
            return CodeModelIcon::SlotPrivate;
        }
    }

    ClangBackEnd::HighlightingType mainType = token.types.mainHighlightingType;

    if (mainType == ClangBackEnd::HighlightingType::QtProperty)
        return CodeModelIcon::Property;

    if (mainType == ClangBackEnd::HighlightingType::PreprocessorExpansion
            || mainType == ClangBackEnd::HighlightingType::PreprocessorDefinition) {
        return CodeModelIcon::Macro;
    }

    if (mainType == ClangBackEnd::HighlightingType::Enumeration)
        return CodeModelIcon::Enumerator;

    if (mainType == ClangBackEnd::HighlightingType::Type
            || mainType == ClangBackEnd::HighlightingType::Keyword) {
        const ClangBackEnd::MixinHighlightingTypes &types = token.types.mixinHighlightingTypes;
        if (types.contains(ClangBackEnd::HighlightingType::Enum))
            return CodeModelIcon::Enum;
        if (types.contains(ClangBackEnd::HighlightingType::Struct))
            return CodeModelIcon::Struct;
        if (types.contains(ClangBackEnd::HighlightingType::Namespace))
            return CodeModelIcon::Namespace;
        if (types.contains(ClangBackEnd::HighlightingType::Class))
            return CodeModelIcon::Class;
        if (mainType == ClangBackEnd::HighlightingType::Keyword)
            return CodeModelIcon::Keyword;
        return CodeModelIcon::Class;
    }

    ClangBackEnd::StorageClass storageClass = extraInfo.storageClass;
    if (mainType == ClangBackEnd::HighlightingType::VirtualFunction
            || mainType == ClangBackEnd::HighlightingType::Function
            || token.types.mixinHighlightingTypes.contains(
                ClangBackEnd::HighlightingType::Operator)) {
        if (storageClass != ClangBackEnd::StorageClass::Static) {
            switch (access) {
            case ClangBackEnd::AccessSpecifier::Public:
            case ClangBackEnd::AccessSpecifier::Invalid:
                return CodeModelIcon::FuncPublic;
            case ClangBackEnd::AccessSpecifier::Protected:
                return CodeModelIcon::FuncProtected;
            case ClangBackEnd::AccessSpecifier::Private:
                return CodeModelIcon::FuncPrivate;
            }
        } else {
            switch (access) {
            case ClangBackEnd::AccessSpecifier::Public:
            case ClangBackEnd::AccessSpecifier::Invalid:
                return CodeModelIcon::FuncPublicStatic;
            case ClangBackEnd::AccessSpecifier::Protected:
                return CodeModelIcon::FuncProtectedStatic;
            case ClangBackEnd::AccessSpecifier::Private:
                return CodeModelIcon::FuncPrivateStatic;
            }
        }
    }
    if (mainType == ClangBackEnd::HighlightingType::GlobalVariable
            || mainType == ClangBackEnd::HighlightingType::Field) {
        if (storageClass != ClangBackEnd::StorageClass::Static) {
            switch (access) {
            case ClangBackEnd::AccessSpecifier::Public:
            case ClangBackEnd::AccessSpecifier::Invalid:
                return CodeModelIcon::VarPublic;
            case ClangBackEnd::AccessSpecifier::Protected:
                return CodeModelIcon::VarProtected;
            case ClangBackEnd::AccessSpecifier::Private:
                return CodeModelIcon::VarPrivate;
            }
        } else {
            switch (access) {
            case ClangBackEnd::AccessSpecifier::Public:
            case ClangBackEnd::AccessSpecifier::Invalid:
                return CodeModelIcon::VarPublicStatic;
            case ClangBackEnd::AccessSpecifier::Protected:
                return CodeModelIcon::VarProtectedStatic;
            case ClangBackEnd::AccessSpecifier::Private:
                return CodeModelIcon::VarPrivateStatic;
            }
        }
    }

    return CodeModelIcon::Unknown;
}

QString diagnosticCategoryPrefixRemoved(const QString &text)
{
    QString theText = text;

    // Prefixes are taken from $LLVM_SOURCE_DIR/tools/clang/lib/Frontend/TextDiagnostic.cpp,
    // function TextDiagnostic::printDiagnosticLevel (llvm-3.6.2).
    static const QStringList categoryPrefixes = {
        QStringLiteral("note"),
        QStringLiteral("remark"),
        QStringLiteral("warning"),
        QStringLiteral("error"),
        QStringLiteral("fatal error")
    };

    for (const QString &prefix : categoryPrefixes) {
        const QString fullPrefix = prefix + QStringLiteral(": ");
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
        args << (projectPart.toolChainWordWidth == ProjectPart::WordWidth64Bit
                     ? QLatin1String("-m64")
                     : QLatin1String("-m32"));
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
                                    const ClangDiagnosticConfig &warningsConfig,
                                    const QStringList &projectOptions)
{
    QJsonObject fileObject;
    fileObject["file"] = projFile.path;
    QJsonArray args;

    if (purpose == CompilationDbPurpose::Project) {
        args = QJsonArray::fromStringList(arguments);

        const ProjectFile::Kind kind = ProjectFile::classify(projFile.path);
        if (projectPart.toolchainType == ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID
                || projectPart.toolchainType == ProjectExplorer::Constants::CLANG_CL_TOOLCHAIN_TYPEID) {
            if (ProjectFile::isC(kind))
                args.append("/TC");
            else if (ProjectFile::isCxx(kind))
                args.append("/TP");
        } else {
            QStringList langOption
                    = createLanguageOptionGcc(kind,
                                              projectPart.languageExtensions
                                              & LanguageExtension::ObjectiveC);
            for (const QString &langOptionPart : langOption)
                args.append(langOptionPart);
        }
    } else {
        // TODO: Do we really need to re-calculate the project part options per source file?
        args = QJsonArray::fromStringList(createClangOptions(projectPart, projFile.path,
                                                             warningsConfig, projectOptions));
        args.prepend("clang"); // TODO: clang-cl for MSVC targets? Does it matter at all what we put here?
    }

    args.append(QDir::toNativeSeparators(projFile.path));
    fileObject["arguments"] = args;
    fileObject["directory"] = buildDir.toString();
    return fileObject;
}

GenerateCompilationDbResult generateCompilationDB(const CppEditor::ProjectInfo::ConstPtr projectInfo,
                                                  const Utils::FilePath &baseDir,
                                                  CompilationDbPurpose purpose,
                                                  const ClangDiagnosticConfig &warningsConfig,
                                                  const QStringList &projectOptions)
{
    QTC_ASSERT(!baseDir.isEmpty(), return GenerateCompilationDbResult(QString(),
        QCoreApplication::translate("ClangUtils", "Could not retrieve build directory.")));
    QTC_ASSERT(projectInfo, return GenerateCompilationDbResult(QString(),
        "Could not retrieve project info."));
    QTC_CHECK(baseDir.ensureWritableDir());
    QFile compileCommandsFile(baseDir.toString() + "/compile_commands.json");
    const bool fileOpened = compileCommandsFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
    if (!fileOpened) {
        return GenerateCompilationDbResult(QString(),
                QCoreApplication::translate("ClangUtils", "Could not create \"%1\": %2")
                    .arg(compileCommandsFile.fileName(), compileCommandsFile.errorString()));
    }
    compileCommandsFile.write("[");

    for (ProjectPart::ConstPtr projectPart : projectInfo->projectParts()) {
        QStringList args;
        if (purpose == CompilationDbPurpose::Project)
            args = projectPartArguments(*projectPart);
        for (const ProjectFile &projFile : projectPart->files) {
            const QJsonObject json = createFileObject(baseDir, args, *projectPart, projFile,
                                                      purpose, warningsConfig, projectOptions);
            if (compileCommandsFile.size() > 1)
                compileCommandsFile.write(",");
            compileCommandsFile.write('\n' + QJsonDocument(json).toJson().trimmed());
        }
    }

    compileCommandsFile.write("\n]");
    compileCommandsFile.close();
    return GenerateCompilationDbResult(compileCommandsFile.fileName(), QString());
}

QString currentCppEditorDocumentFilePath()
{
    QString filePath;

    const auto currentEditor = Core::EditorManager::currentEditor();
    if (currentEditor && CppEditor::CppModelManager::isCppEditor(currentEditor)) {
        const auto currentDocument = currentEditor->document();
        if (currentDocument)
            filePath = currentDocument->filePath().toString();
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
    return m_text.mid(index, m_text.count() - index - 1);
}

QString DiagnosticTextInfo::category() const
{
    if (m_squareBracketStartIndex == -1)
        return QString();

    const int index = m_squareBracketStartIndex + 1;
    if (isClazyOption(m_text.mid(index)))
        return QCoreApplication::translate("ClangDiagnosticWidget", "Clazy Issue");
    else
        return QCoreApplication::translate("ClangDiagnosticWidget", "Clang-Tidy Issue");
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


namespace {
static ClangProjectSettings &getProjectSettings(ProjectExplorer::Project *project)
{
    QTC_CHECK(project);
    return ClangModelManagerSupport::instance()->projectSettings(project);
}

// TODO: Can we marry this with CompilerOptionsBuilder?
class FileOptionsBuilder
{
public:
    FileOptionsBuilder(const QString &filePath, const CppEditor::ProjectPart &projectPart,
                       const ClangDiagnosticConfig &warningsConfig,
                       const QStringList &projectOptions)
        : m_filePath(filePath)
        , m_projectPart(projectPart)
        , m_warningsConfig(warningsConfig)
        , m_builder(projectPart)
    {
        // Determine the driver mode from toolchain and flags.
        m_builder.evaluateCompilerFlags();
        m_isClMode = m_builder.isClStyle();

        addLanguageOptions();
        addGlobalDiagnosticOptions(); // Before addDiagnosticOptions() so users still can overwrite.
        addDiagnosticOptions();
        m_options.append(projectOptions);
        addPrecompiledHeaderOptions();
    }

    const QStringList &options() const { return m_options; }
    CppEditor::UseBuildSystemWarnings useBuildSystemWarnings() const
    {
        return m_useBuildSystemWarnings;
    }

private:
    void addLanguageOptions()
    {
        // Determine file kind with respect to ambiguous headers.
        CppEditor::ProjectFile::Kind fileKind = CppEditor::ProjectFile::Unclassified;
        if (!m_filePath.isEmpty())
            fileKind = CppEditor::ProjectFile::classify(m_filePath);
        if (fileKind == CppEditor::ProjectFile::AmbiguousHeader) {
            fileKind = m_projectPart.languageVersion <= ::Utils::LanguageVersion::LatestC
                 ? CppEditor::ProjectFile::CHeader
                 : CppEditor::ProjectFile::CXXHeader;
        }

        m_builder.reset();
        m_builder.updateFileLanguage(fileKind);

        m_options.append(m_builder.options());
    }

    void addDiagnosticOptions()
    {
        addDiagnosticOptionsForConfig(m_warningsConfig);
    }

    void addDiagnosticOptionsForConfig(const CppEditor::ClangDiagnosticConfig &diagnosticConfig)
    {
        m_useBuildSystemWarnings = diagnosticConfig.useBuildSystemWarnings()
                                       ? CppEditor::UseBuildSystemWarnings::Yes
                                       : CppEditor::UseBuildSystemWarnings::No;

        const QStringList options = m_isClMode
                                        ? CppEditor::clangArgsForCl(diagnosticConfig.clangOptions())
                                        : diagnosticConfig.clangOptions();
        m_options.append(options);
    }

    void addGlobalDiagnosticOptions()
    {
        m_options += CppEditor::ClangDiagnosticConfigsModel::globalDiagnosticOptions();
    }

    void addPrecompiledHeaderOptions()
    {
        using namespace CppEditor;

        if (getPchUsage() == UsePrecompiledHeaders::No)
            return;

        if (m_projectPart.precompiledHeaders.contains(m_filePath))
            return;

        m_builder.reset();
        m_builder.addPrecompiledHeaderOptions(UsePrecompiledHeaders::Yes);

        m_options.append(m_builder.options());
    }

private:
    const QString &m_filePath;
    const CppEditor::ProjectPart &m_projectPart;
    const ClangDiagnosticConfig &m_warningsConfig;

    CppEditor::UseBuildSystemWarnings m_useBuildSystemWarnings = CppEditor::UseBuildSystemWarnings::No;
    CppEditor::CompilerOptionsBuilder m_builder;
    bool m_isClMode = false;
    QStringList m_options;
};
} // namespace

QStringList createClangOptions(const ProjectPart &projectPart, const QString &filePath,
                               const ClangDiagnosticConfig &warningsConfig,
                               const QStringList &projectOptions)
{
    const FileOptionsBuilder fileOptions(filePath, projectPart, warningsConfig, projectOptions);
    LibClangOptionsBuilder optionsBuilder(projectPart, fileOptions.useBuildSystemWarnings());
    const QStringList projectPartOptions = optionsBuilder.build(CppEditor::ProjectFile::Unsupported,
                                                                UsePrecompiledHeaders::No);

    // FIXME: Sanitize FileOptionsBuilder instead.
    QStringList fileArgs = fileOptions.options();
    if (projectPartOptions.contains(QLatin1String("-TP")))
        fileArgs.removeAll(QLatin1String("/TP"));
    if (projectPartOptions.contains(QLatin1String("-TC")))
        fileArgs.removeAll(QLatin1String("/TC"));

    return projectPartOptions + fileArgs;
}

ClangDiagnosticConfig warningsConfigForProject(Project *project)
{
    if (project) {
        ClangProjectSettings &projectSettings = ClangModelManagerSupport::instance()
                ->projectSettings(project);
        if (!projectSettings.useGlobalConfig()) {
            const Utils::Id warningConfigId = projectSettings.warningConfigId();
            const CppEditor::ClangDiagnosticConfigsModel configsModel
                    = CppEditor::diagnosticConfigsModel();
            if (configsModel.hasConfigWithId(warningConfigId))
                return configsModel.configWithId(warningConfigId);
        }
    }
    return CppEditor::codeModelSettings()->clangDiagnosticConfig();
}

const QStringList optionsForProject(ProjectExplorer::Project *project)
{
    if (project)
        return getProjectSettings(project).commandLineOptions();
    return ClangProjectSettings::globalCommandLineOptions();
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

} // namespace Internal
} // namespace Clang
