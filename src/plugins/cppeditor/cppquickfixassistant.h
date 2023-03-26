// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppsemanticinfo.h"

#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/iassistprovider.h>
#include <texteditor/quickfix.h>

#include <cplusplus/LookupContext.h>

namespace CppEditor {
class CppEditorWidget;
class CppRefactoringFile;
using CppRefactoringFilePtr = QSharedPointer<CppRefactoringFile>;

namespace Internal {

class CppQuickFixInterface : public TextEditor::AssistInterface
{
public:
    CppQuickFixInterface(CppEditorWidget *editor, TextEditor::AssistReason reason);

    const QList<CPlusPlus::AST *> &path() const;
    CPlusPlus::Snapshot snapshot() const;
    SemanticInfo semanticInfo() const;
    const CPlusPlus::LookupContext &context() const;
    CppEditorWidget *editor() const;

    CppRefactoringFilePtr currentFile() const;

    bool isCursorOn(unsigned tokenIndex) const;
    bool isCursorOn(const CPlusPlus::AST *ast) const;
    bool isBaseObject() const override { return false; }

private:
    QTextCursor adjustedCursor();

    CppEditorWidget *m_editor;
    SemanticInfo m_semanticInfo;
    CPlusPlus::Snapshot m_snapshot;
    CppRefactoringFilePtr m_currentFile;
    CPlusPlus::LookupContext m_context;
    QList<CPlusPlus::AST *> m_path;
};

class CppQuickFixAssistProvider : public TextEditor::IAssistProvider
{
public:
    CppQuickFixAssistProvider() = default;
    TextEditor::IAssistProcessor *createProcessor(const TextEditor::AssistInterface *) const override;
};

TextEditor::QuickFixOperations quickFixOperations(const TextEditor::AssistInterface *interface);

} // Internal
} // CppEditor
