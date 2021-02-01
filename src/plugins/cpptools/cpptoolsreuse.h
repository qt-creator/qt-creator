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

#include "cpptools_global.h"

#include <texteditor/texteditor.h>

#include <cpptools/clangdiagnosticconfig.h>
#include <cpptools/compileroptionsbuilder.h>

#include <cplusplus/ASTVisitor.h>
#include <cplusplus/CppDocument.h>

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

namespace CppTools {
class CppRefactoringFile;

void CPPTOOLS_EXPORT moveCursorToEndOfIdentifier(QTextCursor *tc);
void CPPTOOLS_EXPORT moveCursorToStartOfIdentifier(QTextCursor *tc);

bool CPPTOOLS_EXPORT isQtKeyword(QStringView text);

bool CPPTOOLS_EXPORT isValidAsciiIdentifierChar(const QChar &ch);
bool CPPTOOLS_EXPORT isValidFirstIdentifierChar(const QChar &ch);
bool CPPTOOLS_EXPORT isValidIdentifierChar(const QChar &ch);
bool CPPTOOLS_EXPORT isValidIdentifier(const QString &s);

QStringList CPPTOOLS_EXPORT identifierWordsUnderCursor(const QTextCursor &tc);
QString CPPTOOLS_EXPORT identifierUnderCursor(QTextCursor *cursor);

bool CPPTOOLS_EXPORT isOwnershipRAIIType(CPlusPlus::Symbol *symbol,
                                         const CPlusPlus::LookupContext &context);

const CPlusPlus::Macro CPPTOOLS_EXPORT *findCanonicalMacro(const QTextCursor &cursor,
                                                           CPlusPlus::Document::Ptr document);

enum class CacheUsage { ReadWrite, ReadOnly };

QString CPPTOOLS_EXPORT correspondingHeaderOrSource(const QString &fileName, bool *wasHeader = nullptr,
                                                    CacheUsage cacheUsage = CacheUsage::ReadWrite);
void CPPTOOLS_EXPORT switchHeaderSource();

class CppCodeModelSettings;
CppCodeModelSettings CPPTOOLS_EXPORT *codeModelSettings();

UsePrecompiledHeaders CPPTOOLS_EXPORT getPchUsage();

int indexerFileSizeLimitInMb();
bool fileSizeExceedsLimit(const QFileInfo &fileInfo, int sizeLimitInMb);

class ClangDiagnosticConfigsModel;
ClangDiagnosticConfigsModel CPPTOOLS_EXPORT diagnosticConfigsModel();
ClangDiagnosticConfigsModel CPPTOOLS_EXPORT
diagnosticConfigsModel(const CppTools::ClangDiagnosticConfigs &customConfigs);


QStringList CPPTOOLS_EXPORT getNamespaceNames(const CPlusPlus::Namespace *firstNamespace);
QStringList CPPTOOLS_EXPORT getNamespaceNames(const CPlusPlus::Symbol *symbol);

class CPPTOOLS_EXPORT NSVisitor : public CPlusPlus::ASTVisitor
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

class CPPTOOLS_EXPORT NSCheckerVisitor : public CPlusPlus::ASTVisitor
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


} // CppTools
