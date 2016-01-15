/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "consoleedit.h"
#include "consoleitemmodel.h"
#include "console.h"
#include <utils/qtcassert.h>

#include <QUrl>
#include <QMenu>
#include <QKeyEvent>

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// ConsoleEdit
//
///////////////////////////////////////////////////////////////////////

ConsoleEdit::ConsoleEdit(const QModelIndex &index, QWidget *parent) :
    QTextEdit(parent),
    m_historyIndex(index)
{
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
    QTC_ASSERT(m_historyIndex.isValid(), return);
    int currentRow = m_historyIndex.row();
    const QAbstractItemModel *model = m_historyIndex.model();
    if (currentRow == model->rowCount() - 1)
        m_cachedScript = getCurrentScript();

    while (currentRow) {
        currentRow--;
        if (model->hasIndex(currentRow, 0)) {
            QModelIndex index = model->index(currentRow, 0);
            if (ConsoleItem::InputType == (ConsoleItem::ItemType)model->data(
                        index, ConsoleItem::TypeRole).toInt()) {
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
    QTC_ASSERT(m_historyIndex.isValid(), return);
    int currentRow = m_historyIndex.row();
    const QAbstractItemModel *model = m_historyIndex.model();
    while (currentRow < model->rowCount() - 1) {
        currentRow++;
        if (model->hasIndex(currentRow, 0)) {
            QModelIndex index = model->index(currentRow, 0);
            if (ConsoleItem::InputType == (ConsoleItem::ItemType)model->data(
                        index, ConsoleItem::TypeRole).toInt()) {
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

} // Internal
} // Debugger
