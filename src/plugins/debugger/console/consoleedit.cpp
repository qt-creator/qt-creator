// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "consoleedit.h"
#include "console.h"

#include <utils/qtcassert.h>

#include <QUrl>
#include <QMenu>
#include <QKeyEvent>

namespace Debugger::Internal {

ConsoleEdit::ConsoleEdit(const QModelIndex &index, QWidget *parent) :
    QTextEdit(parent),
    m_historyIndex(index)
{
    m_historyContext = index.data(ConsoleItem::HistoryContextRole).toInt();
    setFrameStyle(QFrame::NoFrame);
    setUndoRedoEnabled(false);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ensureCursorVisible();
    setTextInteractionFlags(Qt::TextEditorInteraction);
}

void ConsoleEdit::keyPressEvent(QKeyEvent *e)
{
    bool keyConsumed = false;

    switch (e->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter: {
        QString currentScript = getCurrentScript();
        debuggerConsole()->evaluate(currentScript);
        emit editingFinished();
        keyConsumed = true;
        break;
    }

    case Qt::Key_Up:
        handleUpKey();
        keyConsumed = true;
        break;

    case Qt::Key_Down:
        handleDownKey();
        keyConsumed = true;
        break;

    default:
        break;
    }

    if (!keyConsumed)
        QTextEdit::keyPressEvent(e);
}

void ConsoleEdit::focusOutEvent(QFocusEvent * /*e*/)
{
    emit editingFinished();
}

void ConsoleEdit::handleUpKey()
{
    updateHistoryContext();
    QTC_ASSERT(m_historyIndex.isValid(), return);
    int currentRow = m_historyIndex.row();
    const QAbstractItemModel *model = m_historyIndex.model();
    if (currentRow == model->rowCount() - 1)
        m_cachedScript = getCurrentScript();

    while (currentRow) {
        currentRow--;
        if (model->hasIndex(currentRow, 0)) {
            QModelIndex index = model->index(currentRow, 0);
            if (model->data(index, ConsoleItem::TypeRole).toInt() == ConsoleItem::InputType
                && model->data(index, ConsoleItem::HistoryContextRole).toInt() == m_historyContext) {
                m_historyIndex = index;
                replaceCurrentScript(
                            model->data(index, ConsoleItem::ExpressionRole).toString());
                break;
            }
        }
    }
}

void ConsoleEdit::handleDownKey()
{
    updateHistoryContext();
    QTC_ASSERT(m_historyIndex.isValid(), return);
    int currentRow = m_historyIndex.row();
    const QAbstractItemModel *model = m_historyIndex.model();
    while (currentRow < model->rowCount() - 1) {
        currentRow++;
        if (model->hasIndex(currentRow, 0)) {
            QModelIndex index = model->index(currentRow, 0);
            if (model->data(index, ConsoleItem::TypeRole).toInt() == ConsoleItem::InputType
                && model->data(index, ConsoleItem::HistoryContextRole).toInt() == m_historyContext) {
                m_historyIndex = index;
                if (currentRow == model->rowCount() - 1) {
                    replaceCurrentScript(m_cachedScript);
                } else {
                    replaceCurrentScript(
                                model->data(index, ConsoleItem::ExpressionRole).toString());
                }
                break;
            }
        }
    }
}

void ConsoleEdit::updateHistoryContext()
{
    const int historyContext = int(debuggerConsole()->mode());
    if (m_historyContext == historyContext)
        return;

    QTC_ASSERT(m_historyIndex.isValid(), return);
    const QAbstractItemModel *model = m_historyIndex.model();
    QTC_ASSERT(model->rowCount() > 0, return);
    m_historyContext = historyContext;
    m_historyIndex = model->index(model->rowCount() - 1, 0);
    m_cachedScript = getCurrentScript();
}

QString ConsoleEdit::getCurrentScript() const
{
    QTextCursor cursor = textCursor();
    cursor.setPosition(0);
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    return cursor.selectedText();
}

void ConsoleEdit::replaceCurrentScript(const QString &script)
{
    QTextCursor cursor = textCursor();
    cursor.setPosition(0);
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();
    cursor.insertText(script);
    setTextCursor(cursor);
}

} // Debugger::Internal
