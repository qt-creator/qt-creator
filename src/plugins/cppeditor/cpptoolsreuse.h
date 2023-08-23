// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include "clangdiagnosticconfig.h"
#include "compileroptionsbuilder.h"
#include "projectpart.h"

#include <texteditor/quickfix.h>
#include <texteditor/texteditor.h>

#include <utils/searchresultitem.h>

#include <cplusplus/ASTVisitor.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/Token.h>

namespace CPlusPlus {
class Macro;
class Symbol;
class LookupContext;
} // namespace CPlusPlus

namespace TextEditor { class AssistInterface; }
namespace Utils { namespace Text { class Range; } }

namespace CppEditor {
class CppEditorWidget;
class CppRefactoringFile;
class ProjectInfo;
class CppCompletionAssistProcessor;

void CPPEDITOR_EXPORT moveCursorToEndOfIdentifier(QTextCursor *tc);
void CPPEDITOR_EXPORT moveCursorToStartOfIdentifier(QTextCursor *tc);

bool CPPEDITOR_EXPORT isQtKeyword(QStringView text);

bool CPPEDITOR_EXPORT isValidAsciiIdentifierChar(const QChar &ch);
bool CPPEDITOR_EXPORT isValidFirstIdentifierChar(const QChar &ch);
bool CPPEDITOR_EXPORT isValidIdentifierChar(const QChar &ch);
bool CPPEDITOR_EXPORT isValidIdentifier(const QString &s);

QStringList CPPEDITOR_EXPORT identifierWordsUnderCursor(const QTextCursor &tc);
QString CPPEDITOR_EXPORT identifierUnderCursor(QTextCursor *cursor);

bool CPPEDITOR_EXPORT isOwnershipRAIIType(CPlusPlus::Symbol *symbol,
                                         const CPlusPlus::LookupContext &context);

const CPlusPlus::Macro CPPEDITOR_EXPORT *findCanonicalMacro(const QTextCursor &cursor,
                                                           CPlusPlus::Document::Ptr document);

bool CPPEDITOR_EXPORT isInCommentOrString(const TextEditor::AssistInterface *interface,
                                          CPlusPlus::LanguageFeatures features);
TextEditor::QuickFixOperations CPPEDITOR_EXPORT
quickFixOperations(const TextEditor::AssistInterface *interface);

CppCompletionAssistProcessor CPPEDITOR_EXPORT *getCppCompletionAssistProcessor();

enum class CacheUsage { ReadWrite, ReadOnly };

Utils::FilePath CPPEDITOR_EXPORT correspondingHeaderOrSource(
     const Utils::FilePath &filePath, bool *wasHeader = nullptr,
     CacheUsage cacheUsage = CacheUsage::ReadWrite);

void CPPEDITOR_EXPORT openEditor(const Utils::FilePath &filePath, bool inNextSplit,
                                 Utils::Id editorId = {});
class CppCodeModelSettings;
CppCodeModelSettings CPPEDITOR_EXPORT *codeModelSettings();

QString CPPEDITOR_EXPORT preferredCxxHeaderSuffix(ProjectExplorer::Project *project);
QString CPPEDITOR_EXPORT preferredCxxSourceSuffix(ProjectExplorer::Project *project);
bool CPPEDITOR_EXPORT preferLowerCaseFileNames(ProjectExplorer::Project *project);


QList<Utils::Text::Range> CPPEDITOR_EXPORT symbolOccurrencesInText(
    const QTextDocument &doc, QStringView text, int offset, const QString &symbolName);
Utils::SearchResultItems CPPEDITOR_EXPORT
symbolOccurrencesInDeclarationComments(const Utils::SearchResultItems &symbolOccurrencesInCode);
QList<Utils::Text::Range> CPPEDITOR_EXPORT symbolOccurrencesInDeclarationComments(
    CppEditorWidget *editorWidget, const QTextCursor &cursor);

UsePrecompiledHeaders CPPEDITOR_EXPORT getPchUsage();

int indexerFileSizeLimitInMb();
bool fileSizeExceedsLimit(const Utils::FilePath &filePath, int sizeLimitInMb);

ProjectExplorer::Project CPPEDITOR_EXPORT *projectForProjectInfo(const ProjectInfo &info);
ProjectExplorer::Project CPPEDITOR_EXPORT *projectForProjectPart(const ProjectPart &part);

class ClangDiagnosticConfigsModel;
ClangDiagnosticConfigsModel CPPEDITOR_EXPORT diagnosticConfigsModel();
ClangDiagnosticConfigsModel CPPEDITOR_EXPORT
diagnosticConfigsModel(const ClangDiagnosticConfigs &customConfigs);

class CPPEDITOR_EXPORT SymbolInfo
{
public:
    int startLine = 0;
    int startColumn = 0;
    int endLine = 0;
    int endColumn = 0;
    QString fileName;
    bool isResultOnlyForFallBack = false;
};

class ProjectPartInfo {
public:
    enum Hint {
        NoHint = 0,
        IsFallbackMatch  = 1 << 0,
        IsAmbiguousMatch = 1 << 1,
        IsPreferredMatch = 1 << 2,
        IsFromProjectMatch = 1 << 3,
        IsFromDependenciesMatch = 1 << 4,
    };
    Q_DECLARE_FLAGS(Hints, Hint)

    ProjectPartInfo() = default;
    ProjectPartInfo(const ProjectPart::ConstPtr &projectPart,
                    const QList<ProjectPart::ConstPtr> &projectParts,
                    Hints hints)
        : projectPart(projectPart)
        , projectParts(projectParts)
        , hints(hints)
    {
    }

public:
    ProjectPart::ConstPtr projectPart;
    QList<ProjectPart::ConstPtr> projectParts; // The one above as first plus alternatives.
    Hints hints = NoHint;
};

QStringList CPPEDITOR_EXPORT getNamespaceNames(const CPlusPlus::Namespace *firstNamespace);
QStringList CPPEDITOR_EXPORT getNamespaceNames(const CPlusPlus::Symbol *symbol);

class CPPEDITOR_EXPORT NSVisitor : public CPlusPlus::ASTVisitor
{
public:
    NSVisitor(const CppRefactoringFile *file, const QStringList &namespaces, int symbolPos);

    const QStringList remainingNamespaces() const { return m_remainingNamespaces; }
    const CPlusPlus::NamespaceAST *firstNamespace() const { return m_firstNamespace; }
    const CPlusPlus::AST *firstToken() const { return m_firstToken; }
    const CPlusPlus::NamespaceAST *enclosingNamespace() const { return m_enclosingNamespace; }

private:
    bool preVisit(CPlusPlus::AST *ast) override;
    bool visit(CPlusPlus::NamespaceAST *ns) override;
    void postVisit(CPlusPlus::AST *ast) override;

    const CppRefactoringFile * const m_file;
    const CPlusPlus::NamespaceAST *m_enclosingNamespace = nullptr;
    const CPlusPlus::NamespaceAST *m_firstNamespace = nullptr;
    const CPlusPlus::AST *m_firstToken = nullptr;
    QStringList m_remainingNamespaces;
    const int m_symbolPos;
    bool m_done = false;
};

class CPPEDITOR_EXPORT NSCheckerVisitor : public CPlusPlus::ASTVisitor
{
public:
    NSCheckerVisitor(const CppRefactoringFile *file, const QStringList &namespaces, int symbolPos);

    /**
     * @brief returns the names of the namespaces that are additionally needed at the symbolPos
     * @return A list of namespace names, the outermost namespace at index 0 and the innermost
     * at the last index
     */
    const QStringList remainingNamespaces() const { return m_remainingNamespaces; }

private:
    bool preVisit(CPlusPlus::AST *ast) override;
    void postVisit(CPlusPlus::AST *ast) override;
    bool visit(CPlusPlus::NamespaceAST *ns) override;
    bool visit(CPlusPlus::UsingDirectiveAST *usingNS) override;
    void endVisit(CPlusPlus::NamespaceAST *ns) override;
    void endVisit(CPlusPlus::TranslationUnitAST *) override;

    QString getName(CPlusPlus::NamespaceAST *ns);
    CPlusPlus::NamespaceAST *currentNamespace();

    const CppRefactoringFile *const m_file;
    QStringList m_remainingNamespaces;
    const int m_symbolPos;
    std::vector<CPlusPlus::NamespaceAST *> m_enteredNamespaces;

    // track 'using namespace ...' statements
    std::unordered_map<CPlusPlus::NamespaceAST *, QStringList> m_usingsPerNamespace;

    bool m_done = false;
};

namespace Internal {
void decorateCppEditor(TextEditor::TextEditorWidget *editor);
} // namespace Internal

} // CppEditor
