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

#include "cppsemanticinfo.h"

#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/iassistprovider.h>

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

private:
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
    IAssistProvider::RunType runType() const override;
    TextEditor::IAssistProcessor *createProcessor(const TextEditor::AssistInterface *) const override;
};

} // Internal
} // CppEditor
