/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef CPPREFACTORINGCHANGES_H
#define CPPREFACTORINGCHANGES_H

#include <ASTfwd.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/LookupContext.h>

#include <cpptools/cppmodelmanagerinterface.h>
#include <cppeditor/cppeditor_global.h>

#include <texteditor/refactoringchanges.h>

namespace CppEditor {

class CppRefactoringChanges;

class CPPEDITOR_EXPORT CppRefactoringFile: public TextEditor::RefactoringFile
{
public:
    CppRefactoringFile();
    CppRefactoringFile(const QString &fileName, CppRefactoringChanges *refactoringChanges);
    CppRefactoringFile(TextEditor::BaseTextEditor *editor, CPlusPlus::Document::Ptr document);

    CPlusPlus::Document::Ptr cppDocument() const;

    CPlusPlus::Scope *scopeAt(unsigned index) const;

    bool isCursorOn(unsigned tokenIndex) const;
    bool isCursorOn(const CPlusPlus::AST *ast) const;

    Range range(int start, int end) const;
    Range range(unsigned tokenIndex) const;
    Range range(CPlusPlus::AST *ast) const;

    const CPlusPlus::Token &tokenAt(unsigned index) const;

    int startOf(unsigned index) const;
    int startOf(const CPlusPlus::AST *ast) const;
    int endOf(unsigned index) const;
    int endOf(const CPlusPlus::AST *ast) const;

    void startAndEndOf(unsigned index, int *start, int *end) const;

    using TextEditor::RefactoringFile::textOf;
    QString textOf(const CPlusPlus::AST *ast) const;

private:
    CppRefactoringChanges *refactoringChanges() const;

    mutable CPlusPlus::Document::Ptr m_cppDocument;
};

class CPPEDITOR_EXPORT CppRefactoringChanges: public TextEditor::RefactoringChanges
{
public:
    CppRefactoringChanges(const CPlusPlus::Snapshot &snapshot);

    const CPlusPlus::Snapshot &snapshot() const;
    CppRefactoringFile file(const QString &fileName);

private:
    virtual void indentSelection(const QTextCursor &selection) const;
    virtual void fileChanged(const QString &fileName);

private:
    CPlusPlus::Document::Ptr m_thisDocument;
    CPlusPlus::Snapshot m_snapshot;
    CPlusPlus::LookupContext m_context;
    CppTools::CppModelManagerInterface *m_modelManager;
    CppTools::CppModelManagerInterface::WorkingCopy m_workingCopy;
};

} // namespace CppEditor

#endif // CPPREFACTORINGCHANGES_H
