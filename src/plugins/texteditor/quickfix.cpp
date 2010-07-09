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

#include "quickfix.h"
#include "basetexteditor.h"

#include <coreplugin/ifile.h>
#include <extensionsystem/pluginmanager.h>
#include <QtGui/QApplication>
#include <QtGui/QTextBlock>

#include <QtCore/QDebug>

using TextEditor::RefactoringChanges;
using TextEditor::QuickFixOperation;
using TextEditor::QuickFixCollector;
using TextEditor::IQuickFixFactory;

QuickFixOperation::QuickFixOperation(TextEditor::BaseTextEditor *editor)
    : _editor(editor)
{
}

QuickFixOperation::~QuickFixOperation()
{
}

TextEditor::BaseTextEditor *QuickFixOperation::editor() const
{
    return _editor;
}

QTextCursor QuickFixOperation::textCursor() const
{
    return _textCursor;
}

void QuickFixOperation::setTextCursor(const QTextCursor &cursor)
{
    _textCursor = cursor;
}

int QuickFixOperation::selectionStart() const
{
    return _textCursor.selectionStart();
}

int QuickFixOperation::selectionEnd() const
{
    return _textCursor.selectionEnd();
}

int QuickFixOperation::position(int line, int column) const
{
    QTextDocument *doc = editor()->document();
    return doc->findBlockByNumber(line - 1).position() + column - 1;
}

QChar QuickFixOperation::charAt(int offset) const
{
    QTextDocument *doc = _textCursor.document();
    return doc->characterAt(offset);
}

QString QuickFixOperation::textOf(int start, int end) const
{
    QTextCursor tc = _textCursor;
    tc.setPosition(start);
    tc.setPosition(end, QTextCursor::KeepAnchor);
    return tc.selectedText();
}

TextEditor::RefactoringChanges::Range QuickFixOperation::range(int start, int end)
{
    return TextEditor::RefactoringChanges::Range(start, end);
}

void QuickFixOperation::perform()
{
    createChanges();
    apply();
}

QuickFixCollector::QuickFixCollector()
    : _editable(0)
{ }

QuickFixCollector::~QuickFixCollector()
{ }

TextEditor::ITextEditable *QuickFixCollector::editor() const
{ return _editable; }

int QuickFixCollector::startPosition() const
{ return _editable->position(); }

bool QuickFixCollector::triggersCompletion(TextEditor::ITextEditable *)
{ return false; }

int QuickFixCollector::startCompletion(TextEditor::ITextEditable *editable)
{
    Q_ASSERT(editable != 0);

    _editable = editable;

    if (TextEditor::QuickFixState *state = initializeCompletion(editable)) {
        TextEditor::BaseTextEditor *editor = qobject_cast<TextEditor::BaseTextEditor *>(editable->widget());
        Q_ASSERT(editor != 0);

        const QList<TextEditor::QuickFixOperation::Ptr> quickFixOperations = this->quickFixOperations(editor);
        QMap<int, QList<TextEditor::QuickFixOperation::Ptr> > matchedOps;

        foreach (TextEditor::QuickFixOperation::Ptr op, quickFixOperations) {
            op->setTextCursor(editor->textCursor());
            int priority = op->match(state);
            if (priority != -1)
                matchedOps[priority].append(op);
        }

        QMapIterator<int, QList<TextEditor::QuickFixOperation::Ptr> > it(matchedOps);
        it.toBack();
        if (it.hasPrevious()) {
            it.previous();
            _quickFixes = it.value();
        }

        delete state;

        if (! _quickFixes.isEmpty())
            return editable->position();
    }

    return -1;
}

void QuickFixCollector::completions(QList<TextEditor::CompletionItem> *quickFixItems)
{
    for (int i = 0; i < _quickFixes.size(); ++i) {
        TextEditor::QuickFixOperation::Ptr op = _quickFixes.at(i);

        TextEditor::CompletionItem item(this);
        item.text = op->description();
        item.data = QVariant::fromValue(i);
        quickFixItems->append(item);
    }
}

void QuickFixCollector::fix(const TextEditor::CompletionItem &item)
{
    const int index = item.data.toInt();

    if (index < _quickFixes.size()) {
        TextEditor::QuickFixOperation::Ptr quickFix = _quickFixes.at(index);
        quickFix->perform();
    }
}

void QuickFixCollector::cleanup()
{
    _quickFixes.clear();
}

QList<TextEditor::QuickFixOperation::Ptr> QuickFixCollector::quickFixOperations(TextEditor::BaseTextEditor *editor) const
{
    QList<TextEditor::IQuickFixFactory *> factories =
            ExtensionSystem::PluginManager::instance()->getObjects<TextEditor::IQuickFixFactory>();

    QList<TextEditor::QuickFixOperation::Ptr> quickFixOperations;

    foreach (TextEditor::IQuickFixFactory *factory, factories)
        quickFixOperations += factory->quickFixOperations(editor);

    return quickFixOperations;
}

IQuickFixFactory::IQuickFixFactory(QObject *parent)
    : QObject(parent)
{
}

IQuickFixFactory::~IQuickFixFactory()
{

}
