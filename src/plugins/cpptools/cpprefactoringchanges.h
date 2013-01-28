/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CPPREFACTORINGCHANGES_H
#define CPPREFACTORINGCHANGES_H

#include <ASTfwd.h>
#include "ModelManagerInterface.h"
#include <cplusplus/CppDocument.h>
#include <cplusplus/LookupContext.h>

#include <cpptools/cpptools_global.h>

#include <texteditor/refactoringchanges.h>

namespace CppTools {

class CppRefactoringChanges;
class CppRefactoringFile;
class CppRefactoringChangesData;
typedef QSharedPointer<CppRefactoringFile> CppRefactoringFilePtr;
typedef QSharedPointer<const CppRefactoringFile> CppRefactoringFileConstPtr;

class CPPTOOLS_EXPORT CppRefactoringFile: public TextEditor::RefactoringFile
{
public:
    CPlusPlus::Document::Ptr cppDocument() const;
    void setCppDocument(CPlusPlus::Document::Ptr document);

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

protected:
    CppRefactoringFile(const QString &fileName, const QSharedPointer<TextEditor::RefactoringChangesData> &data);
    CppRefactoringFile(QTextDocument *document, const QString &fileName);
    CppRefactoringFile(TextEditor::BaseTextEditorWidget *editor);

    CppRefactoringChangesData *data() const;
    virtual void fileChanged();

    mutable CPlusPlus::Document::Ptr m_cppDocument;

    friend class CppRefactoringChanges; // for access to constructor
};

class CPPTOOLS_EXPORT CppRefactoringChanges: public TextEditor::RefactoringChanges
{
public:
    CppRefactoringChanges(const CPlusPlus::Snapshot &snapshot);

    static CppRefactoringFilePtr file(TextEditor::BaseTextEditorWidget *editor,
                                      const CPlusPlus::Document::Ptr &document);
    CppRefactoringFilePtr file(const QString &fileName) const;
    // safe to use from non-gui threads
    CppRefactoringFileConstPtr fileNoEditor(const QString &fileName) const;

    const CPlusPlus::Snapshot &snapshot() const;

private:
    CppRefactoringChangesData *data() const;
};

} // namespace CppTools

#endif // CPPREFACTORINGCHANGES_H
