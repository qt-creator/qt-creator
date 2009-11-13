/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "cppquickfix.h"
#include "cppeditor.h"
#include <TranslationUnit.h>
#include <Token.h>

#include <cpptools/cppmodelmanagerinterface.h>
#include <QtDebug>

using namespace CppEditor::Internal;
using namespace CPlusPlus;

QuickFixOperation::QuickFixOperation(CPlusPlus::Document::Ptr doc, const CPlusPlus::Snapshot &snapshot)
    : _doc(doc), _snapshot(snapshot)
{ }

QuickFixOperation::~QuickFixOperation()
{ }

QTextCursor QuickFixOperation::textCursor() const
{ return _textCursor; }

void QuickFixOperation::setTextCursor(const QTextCursor &tc)
{ _textCursor = tc; }

const CPlusPlus::Token &QuickFixOperation::tokenAt(unsigned index) const
{ return _doc->translationUnit()->tokenAt(index); }

void QuickFixOperation::getTokenStartPosition(unsigned index, unsigned *line, unsigned *column) const
{ _doc->translationUnit()->getPosition(tokenAt(index).begin(), line, column); }

void QuickFixOperation::getTokenEndPosition(unsigned index, unsigned *line, unsigned *column) const
{ _doc->translationUnit()->getPosition(tokenAt(index).end(), line, column); }

QTextCursor QuickFixOperation::cursor(unsigned index) const
{
    const Token &tk = tokenAt(index);

    unsigned line, col;
    getTokenStartPosition(index, &line, &col);
    QTextCursor tc = _textCursor;
    tc.setPosition(tc.document()->findBlockByNumber(line - 1).position() + col - 1);
    tc.setPosition(tc.position() + tk.f.length, QTextCursor::KeepAnchor);
    return tc;
}

QTextCursor QuickFixOperation::moveAtStartOfToken(unsigned index) const
{
    unsigned line, col;
    getTokenStartPosition(index, &line, &col);
    QTextCursor tc = _textCursor;
    tc.setPosition(tc.document()->findBlockByNumber(line - 1).position() + col - 1);
    return tc;
}

QTextCursor QuickFixOperation::moveAtEndOfToken(unsigned index) const
{
    const Token &tk = tokenAt(index);

    unsigned line, col;
    getTokenStartPosition(index, &line, &col);
    QTextCursor tc = _textCursor;
    tc.setPosition(tc.document()->findBlockByNumber(line - 1).position() + col + tk.f.length - 1);
    return tc;
}

CPPQuickFixCollector::CPPQuickFixCollector()
    : _modelManager(CppTools::CppModelManagerInterface::instance()), _editor(0)
{ }

CPPQuickFixCollector::~CPPQuickFixCollector()
{ }

bool CPPQuickFixCollector::supportsEditor(TextEditor::ITextEditable *editor)
{ return qobject_cast<CPPEditorEditable *>(editor) != 0; }

bool CPPQuickFixCollector::triggersCompletion(TextEditor::ITextEditable *)
{
    qDebug() << Q_FUNC_INFO;
    return false;
}

int CPPQuickFixCollector::startCompletion(TextEditor::ITextEditable *editable)
{
    qDebug() << Q_FUNC_INFO;
    Q_ASSERT(editable != 0);
    _editor = qobject_cast<CPPEditor *>(editable->widget());
    return -1;
}

void CPPQuickFixCollector::completions(QList<TextEditor::CompletionItem> *quicFixItems)
{
    quicFixItems->append(_quickFixItems);
}

void CPPQuickFixCollector::complete(const TextEditor::CompletionItem &item)
{
    qDebug() << Q_FUNC_INFO;

    const int index = item.data.toInt();

    QList<QuickFixOperationPtr> quickFixes; // ### get get the quick fixes.

    if (index < quickFixes.size()) {
        QuickFixOperationPtr quickFix = quickFixes.at(index);
        quickFix->apply(_editor->textCursor());
    }
}

void CPPQuickFixCollector::cleanup()
{
    _quickFixItems.clear();
}
