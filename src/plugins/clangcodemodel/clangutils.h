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
const QStringList optionsForProject(ProjectExplorer::Project *project,
                                    const CppEditor::ClangDiagnosticConfig &warningsConfig);

CppEditor::CompilerOptionsBuilder clangOptionsBuilder(
        const CppEditor::ProjectPart &projectPart,
        const CppEditor::ClangDiagnosticConfig &warningsConfig,
        const Utils::FilePath &clangIncludeDir);
QJsonArray projectPartOptions(const CppEditor::CompilerOptionsBuilder &optionsBuilder);
QJsonArray fullProjectPartOptions(const CppEditor::CompilerOptionsBuilder &optionsBuilder,
                                  const QStringList &projectOptions);
QJsonArray fullProjectPartOptions(const QJsonArray &projectPartOptions,
                                  const QJsonArray &projectOptions);
QJsonArray clangOptionsForFile(const CppEditor::ProjectFile &file,
                               const CppEditor::ProjectPart &projectPart,
                               const QJsonArray &generalOptions,
                               CppEditor::UsePrecompiledHeaders usePch);

CppEditor::ProjectPart::ConstPtr projectPartForFile(const QString &filePath);

QString currentCppEditorDocumentFilePath();

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
GenerateCompilationDbResult generateCompilationDB(const CppEditor::ProjectInfo::ConstPtr projectInfo,
        const Utils::FilePath &baseDir, CompilationDbPurpose purpose,
        const QPair<CppEditor::ClangDiagnosticConfig, QStringList> &configAndOptions,
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
