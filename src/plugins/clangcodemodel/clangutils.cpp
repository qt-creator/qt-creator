// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangutils.h"

#include "clangcodemodeltr.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <cppeditor/baseeditordocumentparser.h>
#include <cppeditor/clangdiagnosticconfigsmodel.h>
#include <cppeditor/clangdsettings.h>
#include <cppeditor/compilationdb.h>
#include <cppeditor/compileroptionsbuilder.h>
#include <cppeditor/cppmodelmanager.h>
#include <cppeditor/cpptoolsreuse.h>
#include <cppeditor/editordocumenthandle.h>
#include <cppeditor/projectpart.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <texteditor/texteditor.h>

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

void generateCompilationDB(
    QPromise<expected_str<FilePath>> &promise,
    const QList<ProjectInfo::ConstPtr> &projectInfoList,
    const FilePath &baseDir,
    CompilationDbPurpose purpose,
    const ClangDiagnosticConfig &warningsConfig,
    const QStringList &projectOptions,
    const FilePath &clangIncludeDir)
{
    return CppEditor::generateCompilationDB(
        promise, projectInfoList, baseDir, purpose, projectOptions, [&](const ProjectPart &pp) {
            return clangOptionsBuilder(pp, warningsConfig, clangIncludeDir, {});
        });
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
{}

int DiagnosticTextInfo::getSquareBracketStartIndex() const
{
    const int offset = m_text.lastIndexOf('[');
    if (offset < m_text.length() - 1 && m_text.at(offset + 1) == ']')
        return -1;
    return offset;
}

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

ClangDiagnosticConfig warningsConfigForProject(Project *project)
{
    return ClangdSettings(ClangdProjectSettings(project).settings()).diagnosticConfig();
}

const QStringList globalClangOptions()
{
    return ClangDiagnosticConfigsModel::globalDiagnosticOptions();
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

} // namespace Internal
} // namespace Clang
