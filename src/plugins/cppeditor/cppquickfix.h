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

#ifndef CPPQUICKFIX_H
#define CPPQUICKFIX_H

#include <texteditor/icompletioncollector.h>
#include <texteditor/quickfix.h>

#include <cplusplus/CppDocument.h>
#include <ASTfwd.h>

#include <utils/changeset.h>

#include <QtCore/QSharedPointer>
#include <QtGui/QTextCursor>

#include "cpprefactoringchanges.h"

namespace CppTools {
    class CppModelManagerInterface;
} // end of namespace CppTools

namespace CppEditor {
namespace Internal {

class CppQuickFixOperation: public TextEditor::QuickFixOperation
{
    Q_DISABLE_COPY(CppQuickFixOperation)

public:
    CppQuickFixOperation(TextEditor::BaseTextEditor *editor);
    virtual ~CppQuickFixOperation();

    virtual int match(const QList<CPlusPlus::AST *> &path) = 0;

    CPlusPlus::Document::Ptr document() const;
    const CPlusPlus::Snapshot &snapshot() const;

    virtual int match(TextEditor::QuickFixState *state);

protected:
    QString fileName() const;

    virtual void apply();
    virtual CppRefactoringChanges *refactoringChanges() const;

    const CPlusPlus::Token &tokenAt(unsigned index) const;

    int startOf(unsigned index) const;
    int startOf(const CPlusPlus::AST *ast) const;
    int endOf(unsigned index) const;
    int endOf(const CPlusPlus::AST *ast) const;
    void startAndEndOf(unsigned index, int *start, int *end) const;

    bool isCursorOn(unsigned tokenIndex) const;
    bool isCursorOn(const CPlusPlus::AST *ast) const;

    using TextEditor::QuickFixOperation::textOf;
    using TextEditor::QuickFixOperation::charAt;

    void move(Utils::ChangeSet *changeSet, unsigned tokenIndex, int to);
    void move(Utils::ChangeSet *changeSet, const CPlusPlus::AST *ast, int to);
    void replace(Utils::ChangeSet *changeSet, unsigned tokenIndex,
                 const QString &replacement);
    void replace(Utils::ChangeSet *changeSet, const CPlusPlus::AST *ast,
                 const QString &replacement);
    void remove(Utils::ChangeSet *changeSet, unsigned tokenIndex);
    void remove(Utils::ChangeSet *changeSet, const CPlusPlus::AST *ast);
    void flip(Utils::ChangeSet *changeSet, const CPlusPlus::AST *ast1,
              const CPlusPlus::AST *ast2);
    void copy(Utils::ChangeSet *changeSet, unsigned tokenIndex, int to);
    void copy(Utils::ChangeSet *changeSet, const CPlusPlus::AST *ast, int to);

    QString textOf(const CPlusPlus::AST *ast) const;

private:
    CppRefactoringChanges *_refactoringChanges;
    CPlusPlus::Document::Ptr _document;
    CPlusPlus::AST *_topLevelNode;
};

class CppQuickFixCollector: public TextEditor::QuickFixCollector
{
    Q_OBJECT

public:
    CppQuickFixCollector();
    virtual ~CppQuickFixCollector();

    virtual bool supportsEditor(TextEditor::ITextEditable *editor);
    virtual TextEditor::QuickFixState *initializeCompletion(TextEditor::ITextEditable *editable);
    virtual QList<TextEditor::QuickFixOperation::Ptr> quickFixOperations(TextEditor::BaseTextEditor *editor) const;
};

} // end of namespace Internal
} // end of namespace CppEditor

#endif // CPPQUICKFIX_H
