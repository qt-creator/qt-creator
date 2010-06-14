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

#ifndef QMLJSQUICKFIX_H
#define QMLJSQUICKFIX_H

#include "qmljseditor.h"

#include <texteditor/quickfix.h>
#include <qmljs/parser/qmljsastfwd_p.h>
#include <qmljs/qmljsdocument.h>

namespace QmlJS {
    class ModelManagerInterface;
}

namespace QmlJSEditor {
class QmlJSRefactoringChanges;

namespace Internal {

class QmlJSQuickFixOperation: public TextEditor::QuickFixOperation
{
    Q_DISABLE_COPY(QmlJSQuickFixOperation)

public:
    QmlJSQuickFixOperation(TextEditor::BaseTextEditor *editor);
    virtual ~QmlJSQuickFixOperation();

    QmlJS::Document::Ptr document() const;
    const QmlJS::Snapshot &snapshot() const;
    const SemanticInfo &semanticInfo() const;

    virtual int check() = 0;
    virtual int match(TextEditor::QuickFixState *state);

protected:
    using TextEditor::QuickFixOperation::move;
    using TextEditor::QuickFixOperation::replace;
    using TextEditor::QuickFixOperation::insert;
    using TextEditor::QuickFixOperation::remove;
    using TextEditor::QuickFixOperation::flip;
    using TextEditor::QuickFixOperation::copy;
    using TextEditor::QuickFixOperation::textOf;
    using TextEditor::QuickFixOperation::charAt;
    using TextEditor::QuickFixOperation::position;

    virtual void apply();
    QmlJSRefactoringChanges *qmljsRefactoringChanges() const;
    virtual TextEditor::RefactoringChanges *refactoringChanges() const;

    unsigned position(const QmlJS::AST::SourceLocation &loc) const;

    // token based operations
    void move(const QmlJS::AST::SourceLocation &loc, int to);
    void replace(const QmlJS::AST::SourceLocation &loc, const QString &replacement);
    void remove(const QmlJS::AST::SourceLocation &loc);
    void copy(const QmlJS::AST::SourceLocation &loc, int to);

private:
    SemanticInfo _semanticInfo;
    QmlJSRefactoringChanges *_refactoringChanges;
};

class QmlJSQuickFixCollector: public TextEditor::QuickFixCollector
{
    Q_OBJECT

public:
    QmlJSQuickFixCollector();
    virtual ~QmlJSQuickFixCollector();

    virtual bool supportsEditor(TextEditor::ITextEditable *editor);
    virtual TextEditor::QuickFixState *initializeCompletion(TextEditor::ITextEditable *editable);
    virtual QList<TextEditor::QuickFixOperation::Ptr> quickFixOperations(TextEditor::BaseTextEditor *editor) const;
};

} // end of namespace Internal
} // end of namespace QmlJSEditor

#endif // QMLJSQUICKFIX_H
