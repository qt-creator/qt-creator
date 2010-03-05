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

#include <cplusplus/CppDocument.h>
#include <cplusplus/LookupContext.h>
#include <ASTfwd.h>

#include <utils/changeset.h>

#include <QtCore/QSharedPointer>
#include <QtGui/QTextCursor>

namespace CppTools {
    class CppModelManagerInterface;
} // end of namespace CppTools

namespace CppEditor {
namespace Internal {

class CPPEditor;
class QuickFixOperation;
typedef QSharedPointer<QuickFixOperation> QuickFixOperationPtr;

class QuickFixOperation
{
    Q_DISABLE_COPY(QuickFixOperation)

public:
    QuickFixOperation();
    virtual ~QuickFixOperation();

    virtual QString description() const = 0;
    virtual int match(const QList<CPlusPlus::AST *> &path) = 0;
    virtual void createChangeSet() = 0;

    void apply();

    CPlusPlus::Document::Ptr document() const;
    void setDocument(CPlusPlus::Document::Ptr document);

    CPlusPlus::Snapshot snapshot() const;
    void setSnapshot(const CPlusPlus::Snapshot &snapshot);

    CPPEditor *editor() const;
    void setEditor(CPPEditor *editor);

    QTextCursor textCursor() const;
    void setTextCursor(const QTextCursor &cursor);

    int selectionStart() const;
    int selectionEnd() const;

    const Utils::ChangeSet &changeSet() const;

protected:
    CPlusPlus::AST *topLevelNode() const;
    void setTopLevelNode(CPlusPlus::AST *topLevelNode);

    const CPlusPlus::Token &tokenAt(unsigned index) const;

    int startOf(unsigned index) const;
    int startOf(const CPlusPlus::AST *ast) const;
    int endOf(unsigned index) const;
    int endOf(const CPlusPlus::AST *ast) const;
    void startAndEndOf(unsigned index, int *start, int *end) const;

    bool isCursorOn(unsigned tokenIndex) const;
    bool isCursorOn(const CPlusPlus::AST *ast) const;

    void move(int start, int end, int to);
    void move(unsigned tokenIndex, int to);
    void move(const CPlusPlus::AST *ast, int to);
    void replace(int start, int end, const QString &replacement);
    void replace(unsigned tokenIndex, const QString &replacement);
    void replace(const CPlusPlus::AST *ast, const QString &replacement);
    void insert(int at, const QString &text);
    void remove(int start, int end);
    void remove(unsigned tokenIndex);
    void remove(const CPlusPlus::AST *ast);
    void flip(int start1, int end1, int start2, int end2);
    void flip(const CPlusPlus::AST *ast1, const CPlusPlus::AST *ast2);
    void copy(int start, int end, int to);
    void copy(unsigned tokenIndex, int to);
    void copy(const CPlusPlus::AST *ast, int to);

    QString textOf(int firstOffset, int lastOffset) const;
    QString textOf(const CPlusPlus::AST *ast) const;
    QChar charAt(int offset) const;

    struct Range {
        Range() {}
        Range(const QTextCursor &tc): begin(tc), end(tc) {}

        QTextCursor begin;
        QTextCursor end;
    };

    Range createRange(CPlusPlus::AST *ast) const; // ### rename me
    void reindent(const Range &range);

    const QList<CPlusPlus::LookupItem> typeOf(CPlusPlus::ExpressionAST *ast);

private:
    CPlusPlus::Document::Ptr _document;
    CPlusPlus::Snapshot _snapshot;
    QTextCursor _textCursor;
    Utils::ChangeSet _changeSet;
    CPPEditor *_editor;
    CPlusPlus::AST *_topLevelNode;
    CPlusPlus::LookupContext _lookupContext;
};

class CPPQuickFixCollector: public TextEditor::IQuickFixCollector
{
    Q_OBJECT

public:
    CPPQuickFixCollector();
    virtual ~CPPQuickFixCollector();

    QList<QuickFixOperationPtr> quickFixes() const { return _quickFixes; }

    virtual TextEditor::ITextEditable *editor() const;
    virtual int startPosition() const;
    virtual bool supportsEditor(TextEditor::ITextEditable *editor);
    virtual bool triggersCompletion(TextEditor::ITextEditable *editor);
    virtual int startCompletion(TextEditor::ITextEditable *editor);
    virtual void completions(QList<TextEditor::CompletionItem> *completions);
    virtual void complete(const TextEditor::CompletionItem &item);
    virtual void cleanup();

public Q_SLOTS:
    void perform(QuickFixOperationPtr op);

private:
    CppTools::CppModelManagerInterface *_modelManager;
    TextEditor::ITextEditable *_editable;
    CPPEditor *_editor;
    QList<QuickFixOperationPtr> _quickFixes;
};

} // end of namespace Internal
} // end of namespace CppEditor

#endif // CPPQUICKFIX_H
