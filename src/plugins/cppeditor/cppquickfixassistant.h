/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef CPPQUICKFIXASSISTANT_H
#define CPPQUICKFIXASSISTANT_H

#include "cppsemanticinfo.h"

#include <ASTfwd.h>
#include <cplusplus/CppDocument.h>

#include <texteditor/codeassist/defaultassistinterface.h>
#include <texteditor/codeassist/quickfixassistprovider.h>
#include <texteditor/codeassist/quickfixassistprocessor.h>

namespace CppTools {
class CppRefactoringFile;
typedef QSharedPointer<CppRefactoringFile> CppRefactoringFilePtr;
}

namespace CppEditor {
namespace Internal {

class CPPEditorWidget;

class CppQuickFixAssistInterface : public TextEditor::DefaultAssistInterface
{
public:
    CppQuickFixAssistInterface(CPPEditorWidget *editor, TextEditor::AssistReason reason);

    const QList<CPlusPlus::AST *> &path() const;
    CPlusPlus::Snapshot snapshot() const;
    CppEditor::Internal::SemanticInfo semanticInfo() const;
    const CPlusPlus::LookupContext &context() const;
    CPPEditorWidget *editor() const;

    CppTools::CppRefactoringFilePtr currentFile() const;

    bool isCursorOn(unsigned tokenIndex) const;
    bool isCursorOn(const CPlusPlus::AST *ast) const;

private:
    CPPEditorWidget *m_editor;
    CppEditor::Internal::SemanticInfo m_semanticInfo;
    CPlusPlus::Snapshot m_snapshot;
    CppTools::CppRefactoringFilePtr m_currentFile;
    CPlusPlus::LookupContext m_context;
    QList<CPlusPlus::AST *> m_path;
};

class CppQuickFixAssistProcessor : public TextEditor::QuickFixAssistProcessor
{
public:
    CppQuickFixAssistProcessor(const TextEditor::IAssistProvider *provider);

    virtual const TextEditor::IAssistProvider *provider() const;

private:
    const TextEditor::IAssistProvider *m_provider;
};

class CppQuickFixAssistProvider : public TextEditor::QuickFixAssistProvider
{
public:
    virtual bool supportsEditor(const QString &editorId) const;
    virtual TextEditor::IAssistProcessor *createProcessor() const;

    virtual QList<TextEditor::QuickFixFactory *> quickFixFactories() const;
};

} // Internal
} // CppEditor

#endif // CPPQUICKFIXASSISTANT_H
