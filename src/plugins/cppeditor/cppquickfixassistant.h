/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CPPQUICKFIXASSISTANT_H
#define CPPQUICKFIXASSISTANT_H

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
    IAssistProvider::RunType runType() const override;
    bool supportsEditor(Core::Id editorId) const override;
    TextEditor::IAssistProcessor *createProcessor() const override;

    QList<TextEditor::QuickFixFactory *> quickFixFactories() const override;
};

} // Internal
} // CppEditor

#endif // CPPQUICKFIXASSISTANT_H
