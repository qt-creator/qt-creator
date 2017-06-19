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

#include <cpptools/cppsemanticinfo.h>
#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/quickfixassistprovider.h>
#include <texteditor/codeassist/quickfixassistprocessor.h>

#include <cplusplus/LookupContext.h>


namespace CppTools {
class CppRefactoringFile;
typedef QSharedPointer<CppRefactoringFile> CppRefactoringFilePtr;
}

namespace CppEditor {
namespace Internal {

class CppEditorWidget;

class CppQuickFixInterface : public TextEditor::AssistInterface
{
public:
    CppQuickFixInterface(CppEditorWidget *editor, TextEditor::AssistReason reason);

    const QList<CPlusPlus::AST *> &path() const;
    CPlusPlus::Snapshot snapshot() const;
    CppTools::SemanticInfo semanticInfo() const;
    const CPlusPlus::LookupContext &context() const;
    CppEditorWidget *editor() const;

    CppTools::CppRefactoringFilePtr currentFile() const;

    bool isCursorOn(unsigned tokenIndex) const;
    bool isCursorOn(const CPlusPlus::AST *ast) const;

private:
    CppEditorWidget *m_editor;
    CppTools::SemanticInfo m_semanticInfo;
    CPlusPlus::Snapshot m_snapshot;
    CppTools::CppRefactoringFilePtr m_currentFile;
    CPlusPlus::LookupContext m_context;
    QList<CPlusPlus::AST *> m_path;
};

class CppQuickFixAssistProvider : public TextEditor::QuickFixAssistProvider
{
public:
    CppQuickFixAssistProvider(QObject *parent = 0) : TextEditor::QuickFixAssistProvider(parent) {}
    IAssistProvider::RunType runType() const override;
    TextEditor::IAssistProcessor *createProcessor() const override;

    QList<TextEditor::QuickFixFactory *> quickFixFactories() const override;
};

} // Internal
} // CppEditor
