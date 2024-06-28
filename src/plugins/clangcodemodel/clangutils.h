// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cplusplus/Icons.h>

#include <cppeditor/projectinfo.h>
#include <cppeditor/compileroptionsbuilder.h>

#include <utils/link.h>

#include <QPair>
#include <QTextCursor>

QT_BEGIN_NAMESPACE
class QTextBlock;
QT_END_NAMESPACE

namespace CppEditor {
class ClangDiagnosticConfig;
class CppEditorDocumentHandle;
}

namespace ProjectExplorer { class Project; }

namespace ClangCodeModel {
namespace Internal {

CppEditor::ClangDiagnosticConfig warningsConfigForProject(ProjectExplorer::Project *project);
const QStringList globalClangOptions();

CppEditor::CompilerOptionsBuilder clangOptionsBuilder(
        const CppEditor::ProjectPart &projectPart,
        const CppEditor::ClangDiagnosticConfig &warningsConfig,
        const Utils::FilePath &clangIncludeDir,
        const ProjectExplorer::Macros &extraMacros);
QJsonArray projectPartOptions(const CppEditor::CompilerOptionsBuilder &optionsBuilder);
QJsonArray fullProjectPartOptions(const CppEditor::CompilerOptionsBuilder &optionsBuilder,
                                  const QStringList &projectOptions);
QJsonArray fullProjectPartOptions(const QJsonArray &projectPartOptions,
                                  const QJsonArray &projectOptions);
QJsonArray clangOptionsForFile(const CppEditor::ProjectFile &file,
                               const CppEditor::ProjectPart &projectPart,
                               const QJsonArray &generalOptions,
                               CppEditor::UsePrecompiledHeaders usePch, bool clStyle);

CppEditor::ProjectPart::ConstPtr projectPartForFile(const Utils::FilePath &filePath);

Utils::FilePath currentCppEditorDocumentFilePath();

QString diagnosticCategoryPrefixRemoved(const QString &text);

using GenerateCompilationDbResult = Utils::expected_str<Utils::FilePath>;
enum class CompilationDbPurpose { Project, CodeModel };
void generateCompilationDB(
    QPromise<GenerateCompilationDbResult> &promise,
    const QList<CppEditor::ProjectInfo::ConstPtr> &projectInfoList,
    const Utils::FilePath &baseDir,
    CompilationDbPurpose purpose,
    const CppEditor::ClangDiagnosticConfig &warningsConfig,
    const QStringList &projectOptions,
    const Utils::FilePath &clangIncludeDir);

class DiagnosticTextInfo
{
public:
    DiagnosticTextInfo(const QString &text);

    QString textWithoutOption() const;
    QString option() const;
    QString category() const;

    static bool isClazyOption(const QString &option);
    static QString clazyCheckName(const QString &option);

private:
    const QString m_text;
    const int m_squareBracketStartIndex;
};

class ClangSourceRange
{
public:
    ClangSourceRange(const Utils::Link &start, const Utils::Link &end) : start(start), end(end) {}

    bool contains(int line, int column) const
    {
        if (line < start.targetLine || line > end.targetLine)
            return false;
        if (line == start.targetLine && column < start.targetLine)
            return false;
        if (line == end.targetLine && column > end.targetColumn)
            return false;
        return true;
    }

    bool contains(const Utils::Link &sourceLocation) const
    {
        return contains(sourceLocation.targetLine, sourceLocation.targetColumn);
    }

    Utils::Link start;
    Utils::Link end;
};

inline bool operator==(const ClangSourceRange &first, const ClangSourceRange &second)
{
    return first.start == second.start && first.end == second.end;
}

class ClangFixIt
{
public:
    ClangFixIt(const QString &text, const ClangSourceRange &range) : range(range), text(text) {}

    ClangSourceRange range;
    QString text;
};

inline bool operator==(const ClangFixIt &first, const ClangFixIt &second)
{
    return first.text == second.text && first.range == second.range;
}

class ClangDiagnostic
{
public:
    enum class Severity { Ignored, Note, Warning, Error, Fatal };

    Utils::Link location;
    QString text;
    QString category;
    QString enableOption;
    QString disableOption;
    QList<ClangDiagnostic> children;
    QList<ClangFixIt> fixIts;
    Severity severity = Severity::Ignored;
};

inline bool operator==(const ClangDiagnostic &first, const ClangDiagnostic &second)
{
    return first.text == second.text && first.location == second.location;
}
inline bool operator!=(const ClangDiagnostic &first, const ClangDiagnostic &second)
{
    return !(first == second);
}

} // namespace Internal
} // namespace Clang
