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

#include "cppeditor_global.h"

#include "clangdiagnosticconfig.h"
#include "compileroptionsbuilder.h"
#include "projectpart.h"

#include <texteditor/texteditor.h>

#include <cplusplus/ASTVisitor.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/Token.h>

QT_BEGIN_NAMESPACE
class QChar;
class QFileInfo;
class QTextCursor;
QT_END_NAMESPACE

namespace CPlusPlus {
class Macro;
class Symbol;
class LookupContext;
} // namespace CPlusPlus

namespace TextEditor { class AssistInterface; }

namespace CppEditor {
class CppRefactoringFile;
class ProjectInfo;

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

enum class CacheUsage { ReadWrite, ReadOnly };

QString CPPEDITOR_EXPORT correspondingHeaderOrSource(const QString &fileName, bool *wasHeader = nullptr,
                                                    CacheUsage cacheUsage = CacheUsage::ReadWrite);
void CPPEDITOR_EXPORT switchHeaderSource();

class CppCodeModelSettings;
CppCodeModelSettings CPPEDITOR_EXPORT *codeModelSettings();

UsePrecompiledHeaders CPPEDITOR_EXPORT getPchUsage();

int indexerFileSizeLimitInMb();
bool fileSizeExceedsLimit(const QFileInfo &fileInfo, int sizeLimitInMb);

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
