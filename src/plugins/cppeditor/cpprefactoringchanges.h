// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include "cppmodelmanager.h"
#include "cppworkingcopy.h"

#include <cplusplus/CppDocument.h>

#include <texteditor/refactoringchanges.h>

#include <optional>

namespace CppEditor {

class CppRefactoringChanges;
class CppRefactoringFile;
class CppRefactoringChangesData;
using CppRefactoringFilePtr = QSharedPointer<CppRefactoringFile>;
using CppRefactoringFileConstPtr = QSharedPointer<const CppRefactoringFile>;

class CPPEDITOR_EXPORT CppRefactoringFile: public TextEditor::RefactoringFile
{
public:
    CPlusPlus::Document::Ptr cppDocument() const;
    void setCppDocument(CPlusPlus::Document::Ptr document);

    CPlusPlus::Scope *scopeAt(unsigned index) const;

    bool isCursorOn(unsigned tokenIndex) const;
    bool isCursorOn(const CPlusPlus::AST *ast) const;

    Range range(int start, int end) const;
    Range range(unsigned tokenIndex) const;
    Range range(const CPlusPlus::AST *ast) const;

    const CPlusPlus::Token &tokenAt(unsigned index) const;

    int startOf(unsigned index) const;
    int startOf(const CPlusPlus::AST *ast) const;
    int endOf(unsigned index) const;
    int endOf(const CPlusPlus::AST *ast) const;

    void startAndEndOf(unsigned index, int *start, int *end) const;

    QList<CPlusPlus::Token> tokensForCursor() const;

    using TextEditor::RefactoringFile::textOf;
    QString textOf(const CPlusPlus::AST *ast) const;

private:
    CppRefactoringFile(const Utils::FilePath &filePath, const QSharedPointer<CppRefactoringChangesData> &data);
    CppRefactoringFile(QTextDocument *document, const Utils::FilePath &filePath);
    explicit CppRefactoringFile(TextEditor::TextEditorWidget *editor);

    void fileChanged() override;
    void indentSelection(const QTextCursor &selection,
                         const TextEditor::TextDocument *textDocument) const override;
    virtual void reindentSelection(const QTextCursor &selection,
                                   const TextEditor::TextDocument *textDocument) const override;


    int tokenIndexForPosition(const std::vector<CPlusPlus::Token> &tokens, int pos,
                              int startIndex) const;

    mutable CPlusPlus::Document::Ptr m_cppDocument;
    QSharedPointer<CppRefactoringChangesData> m_data;

    friend class CppRefactoringChanges; // for access to constructor
};

class CPPEDITOR_EXPORT CppRefactoringChangesData : public TextEditor::RefactoringChangesData
{
public:
    explicit CppRefactoringChangesData(const CPlusPlus::Snapshot &snapshot);

    CPlusPlus::Snapshot m_snapshot;
    WorkingCopy m_workingCopy;
};

class CPPEDITOR_EXPORT CppRefactoringChanges: public TextEditor::RefactoringChanges
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
    CppRefactoringChangesData *data() const;
};

} // namespace CppEditor
