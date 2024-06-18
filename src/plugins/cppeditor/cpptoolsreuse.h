// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include <cplusplus/CppDocument.h>
#include <texteditor/quickfix.h>
#include <utils/id.h>
#include <utils/searchresultitem.h>

namespace CPlusPlus {
class Macro;
class Symbol;
class LookupContext;
} // namespace CPlusPlus
namespace ProjectExplorer { class Project; }
namespace TextEditor {
class AssistInterface;
class TextEditorWidget;
}
namespace Utils { namespace Text { class Range; } }

namespace CppEditor {
class CppEditorWidget;
class CppRefactoringFile;
class ProjectInfo;
class CppCompletionAssistProcessor;

enum class FollowSymbolMode { Exact, Fuzzy };

void CPPEDITOR_EXPORT moveCursorToEndOfIdentifier(QTextCursor *tc);
void CPPEDITOR_EXPORT moveCursorToStartOfIdentifier(QTextCursor *tc);

bool CPPEDITOR_EXPORT isQtKeyword(QStringView text);

bool CPPEDITOR_EXPORT isValidAsciiIdentifierChar(const QChar &ch);
bool CPPEDITOR_EXPORT isValidFirstIdentifierChar(const QChar &ch);
bool CPPEDITOR_EXPORT isValidIdentifierChar(const QChar &ch);
bool CPPEDITOR_EXPORT isValidIdentifier(const QString &s);

int CPPEDITOR_EXPORT activeArgumenForPrefix(const QString &prefix);

QStringList CPPEDITOR_EXPORT identifierWordsUnderCursor(const QTextCursor &tc);
QString CPPEDITOR_EXPORT identifierUnderCursor(QTextCursor *cursor);

const CPlusPlus::Macro CPPEDITOR_EXPORT *findCanonicalMacro(const QTextCursor &cursor,
                                                           CPlusPlus::Document::Ptr document);

bool CPPEDITOR_EXPORT isInCommentOrString(const TextEditor::AssistInterface *interface,
                                          CPlusPlus::LanguageFeatures features);
bool CPPEDITOR_EXPORT isInCommentOrString(const QTextCursor &cursor,
                                          CPlusPlus::LanguageFeatures features);
TextEditor::QuickFixOperations CPPEDITOR_EXPORT
quickFixOperations(const TextEditor::AssistInterface *interface);

CppCompletionAssistProcessor CPPEDITOR_EXPORT *getCppCompletionAssistProcessor();

QString CPPEDITOR_EXPORT
deriveHeaderGuard(const Utils::FilePath &filePath, ProjectExplorer::Project *project);

enum class CacheUsage { ReadWrite, ReadOnly };

Utils::FilePath CPPEDITOR_EXPORT correspondingHeaderOrSource(
     const Utils::FilePath &filePath, bool *wasHeader = nullptr,
     CacheUsage cacheUsage = CacheUsage::ReadWrite);

void CPPEDITOR_EXPORT openEditor(const Utils::FilePath &filePath, bool inNextSplit,
                                 Utils::Id editorId = {});

QString CPPEDITOR_EXPORT preferredCxxHeaderSuffix(ProjectExplorer::Project *project);
QString CPPEDITOR_EXPORT preferredCxxSourceSuffix(ProjectExplorer::Project *project);
bool CPPEDITOR_EXPORT preferLowerCaseFileNames(ProjectExplorer::Project *project);

QList<Utils::Text::Range> CPPEDITOR_EXPORT symbolOccurrencesInText(
    const QTextDocument &doc, QStringView text, int offset, const QString &symbolName);
Utils::SearchResultItems CPPEDITOR_EXPORT
symbolOccurrencesInDeclarationComments(const Utils::SearchResultItems &symbolOccurrencesInCode);
QList<Utils::Text::Range> CPPEDITOR_EXPORT symbolOccurrencesInDeclarationComments(
    CppEditorWidget *editorWidget, const QTextCursor &cursor);

bool fileSizeExceedsLimit(const Utils::FilePath &filePath, int sizeLimitInMb);

namespace Internal {
void decorateCppEditor(TextEditor::TextEditorWidget *editor);
} // namespace Internal

} // CppEditor
