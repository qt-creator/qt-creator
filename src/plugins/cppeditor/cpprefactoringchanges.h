// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include "cppmodelmanager.h"

#include <cplusplus/CppDocument.h>

#include <texteditor/refactoringchanges.h>

#include <optional>

namespace CppEditor {
class CppRefactoringChanges;
class CppRefactoringFile;
using CppRefactoringFilePtr = QSharedPointer<CppRefactoringFile>;
using CppRefactoringFileConstPtr = QSharedPointer<const CppRefactoringFile>;

namespace Internal { class CppRefactoringChangesData; }

class CPPEDITOR_EXPORT CppRefactoringFile: public TextEditor::RefactoringFile
{
public:
    CPlusPlus::Document::Ptr cppDocument() const;
    void setCppDocument(CPlusPlus::Document::Ptr document);

    CPlusPlus::Scope *scopeAt(unsigned index) const;

    bool isCursorOn(unsigned tokenIndex) const;
    bool isCursorOn(const CPlusPlus::AST *ast) const;

    Range range(unsigned tokenIndex) const;
    Range range(const CPlusPlus::AST *ast) const;

    const CPlusPlus::Token &tokenAt(unsigned index) const;

    int startOf(unsigned index) const;
    int startOf(const CPlusPlus::AST *ast) const;
    int endOf(unsigned index) const;
    int endOf(const CPlusPlus::AST *ast) const;

    void startAndEndOf(unsigned index, int *start, int *end) const;

    std::optional<std::pair<int, int>> expansionLoc(unsigned index) const;

    QList<CPlusPlus::Token> tokensForCursor() const;
    QList<CPlusPlus::Token> tokensForCursor(const QTextCursor &cursor) const;
    QList<CPlusPlus::Token> tokensForLine(int line) const;

    using TextEditor::RefactoringFile::textOf;
    QString textOf(const CPlusPlus::AST *ast) const;

private:
    CppRefactoringFile(const Utils::FilePath &filePath,
                       const QSharedPointer<Internal::CppRefactoringChangesData> &data);
    CppRefactoringFile(QTextDocument *document, const Utils::FilePath &filePath);
    explicit CppRefactoringFile(TextEditor::TextEditorWidget *editor);

    void fileChanged() override;
    Utils::Id indenterId() const override;

    int tokenIndexForPosition(const std::vector<CPlusPlus::Token> &tokens, int pos,
                              int startIndex) const;

    mutable CPlusPlus::Document::Ptr m_cppDocument;
    QSharedPointer<Internal::CppRefactoringChangesData> m_data;

    friend class CppRefactoringChanges; // for access to constructor
};

class CPPEDITOR_EXPORT CppRefactoringChanges: public TextEditor::RefactoringFileFactory
{
public:
    explicit CppRefactoringChanges(const CPlusPlus::Snapshot &snapshot);

    static CppRefactoringFilePtr file(TextEditor::TextEditorWidget *editor,
                                      const CPlusPlus::Document::Ptr &document);
    TextEditor::RefactoringFilePtr file(const Utils::FilePath &filePath) const override;

    CppRefactoringFilePtr cppFile(const Utils::FilePath &filePath) const;

    // safe to use from non-gui threads
    CppRefactoringFileConstPtr fileNoEditor(const Utils::FilePath &filePath) const;

    const CPlusPlus::Snapshot &snapshot() const;

private:
    const QSharedPointer<Internal::CppRefactoringChangesData> m_data;
};

} // namespace CppEditor
