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

namespace TextEditor { class TextDocumentManipulatorInterface; }

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

class GenerateCompilationDbResult
{
public:
    GenerateCompilationDbResult() = default;
    GenerateCompilationDbResult(const QString &filePath, const QString &error)
        : filePath(filePath), error(error)
    {}

    QString filePath;
    QString error;
};

enum class CompilationDbPurpose { Project, CodeModel };
GenerateCompilationDbResult generateCompilationDB(QList<CppEditor::ProjectInfo::ConstPtr> projectInfo,
        Utils::FilePath baseDir, CompilationDbPurpose purpose,
        CppEditor::ClangDiagnosticConfig warningsConfig, QStringList projectOptions,
        Utils::FilePath clangIncludeDir);

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

template <class CharacterProvider>
void moveToPreviousChar(const CharacterProvider &provider, QTextCursor &cursor)
{
    cursor.movePosition(QTextCursor::PreviousCharacter);
    while (provider.characterAt(cursor.position()).isSpace())
        cursor.movePosition(QTextCursor::PreviousCharacter);
}

template <class CharacterProvider>
void moveToPreviousWord(CharacterProvider &provider, QTextCursor &cursor)
{
    cursor.movePosition(QTextCursor::PreviousWord);
    while (provider.characterAt(cursor.position()) == ':')
        cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::MoveAnchor, 2);
}

template <class CharacterProvider>
bool matchPreviousWord(const CharacterProvider &provider, QTextCursor cursor, QString pattern)
{
    cursor.movePosition(QTextCursor::PreviousWord);
    while (provider.characterAt(cursor.position()) == ':')
        cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::MoveAnchor, 2);

    int previousWordStart = cursor.position();
    cursor.movePosition(QTextCursor::NextWord);
    moveToPreviousChar(provider, cursor);
    QString toMatch = provider.textAt(previousWordStart, cursor.position() - previousWordStart + 1);

    pattern = pattern.simplified();
    while (!pattern.isEmpty() && pattern.endsWith(toMatch)) {
        pattern.chop(toMatch.length());
        if (pattern.endsWith(' '))
            pattern.chop(1);
        if (!pattern.isEmpty()) {
            cursor.movePosition(QTextCursor::StartOfWord);
            cursor.movePosition(QTextCursor::PreviousWord);
            previousWordStart = cursor.position();
            cursor.movePosition(QTextCursor::NextWord);
            moveToPreviousChar(provider, cursor);
            toMatch = provider.textAt(previousWordStart, cursor.position() - previousWordStart + 1);
        }
    }
    return pattern.isEmpty();
}

QString textUntilPreviousStatement(TextEditor::TextDocumentManipulatorInterface &manipulator,
                                   int startPosition);

bool isAtUsingDeclaration(TextEditor::TextDocumentManipulatorInterface &manipulator,
                          int basePosition);

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
