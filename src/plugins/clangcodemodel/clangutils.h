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

namespace ClangBackEnd { class TokenInfoContainer; }

namespace ProjectExplorer { class Project; }

namespace ClangCodeModel {
namespace Internal {

CppEditor::CppEditorDocumentHandle *cppDocument(const QString &filePath);
void setLastSentDocumentRevision(const QString &filePath, uint revision);

CppEditor::ClangDiagnosticConfig warningsConfigForProject(ProjectExplorer::Project *project);
const QStringList optionsForProject(ProjectExplorer::Project *project);

QStringList createClangOptions(const CppEditor::ProjectPart &projectPart, const QString &filePath,
                               const CppEditor::ClangDiagnosticConfig &warningsConfig,
                               const QStringList &projectOptions);

CppEditor::ProjectPart::ConstPtr projectPartForFile(const QString &filePath);
CppEditor::ProjectPart::ConstPtr projectPartForFileBasedOnProcessor(const QString &filePath);
bool isProjectPartLoaded(const CppEditor::ProjectPart::ConstPtr projectPart);
QString projectPartIdForFile(const QString &filePath);
int clangColumn(const QTextBlock &line, int cppEditorColumn);
int cppEditorColumn(const QTextBlock &line, int clangColumn);

QString currentCppEditorDocumentFilePath();

QString diagnosticCategoryPrefixRemoved(const QString &text);

Utils::CodeModelIcon::Type iconTypeForToken(const ClangBackEnd::TokenInfoContainer &token);

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
        const CppEditor::ClangDiagnosticConfig &warningsConfig, const QStringList &projectOptions);

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

} // namespace Internal
} // namespace Clang
