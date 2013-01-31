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

#include "qmlconsoleedit.h"
#include "qmlconsoleitemmodel.h"
#include "qmlconsolemodel.h"
#include "qmlconsolemanager.h"

#include <utils/qtcassert.h>

#include <QUrl>
#include <QMenu>
#include <QKeyEvent>

using namespace QmlJS;

namespace QmlJSTools {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// QmlConsoleEdit
//
///////////////////////////////////////////////////////////////////////

QmlConsoleEdit::QmlConsoleEdit(const QModelIndex &index, QWidget *parent) :
    QTextEdit(parent),
    m_historyIndex(index),
    m_prompt(QLatin1String(":/qmljstools/images/prompt.png")),
    m_startOfEditableArea(0)
{
    setFrameStyle(QFrame::NoFrame);
    setUndoRedoEnabled(false);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    document()->addResource(QTextDocument::ImageResource, QUrl(QLatin1String("prompt")), m_prompt);
    QTextImageFormat format;
    format.setName(QLatin1String("prompt"));
    format.setHeight(9);
    format.setWidth(9);
    textCursor().insertText(QLatin1String(" "));
    textCursor().insertImage(format);
    textCursor().insertText(QLatin1String("  "));
    m_startOfEditableArea = textCursor().position();

    ensureCursorVisible();
    setTextInteractionFlags(Qt::TextEditorInteraction);
}

void QmlConsoleEdit::keyPressEvent(QKeyEvent *e)
{
    bool keyConsumed = false;

    switch (e->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter: {
        m_interpreter.clearText();
        QString currentScript = getCurrentScript();
        m_interpreter.appendText(currentScript);
        if (currentScript.isEmpty()) {
            emit editingFinished();
        } else if (m_interpreter.canEvaluate()) {
            QmlConsoleModel::evaluate(currentScript);
            emit editingFinished();
        }
        break;
    }

    case Qt::Key_Backspace:
        if (textCursor().selectionStart() <= m_startOfEditableArea)
            keyConsumed = true;
        break;

    case Qt::Key_Delete:
        if (textCursor().selectionStart() < m_startOfEditableArea)
            keyConsumed = true;
        break;

    case Qt::Key_Home: {
        QTextCursor c(textCursor());
        c.setPosition(m_startOfEditableArea);
        setTextCursor(c);
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

        //  Ctrl+Left: Moves the cursor one word to the left.
        //  Left: Moves the cursor one character to the left.
    case Qt::Key_Left:
        if (textCursor().position() <= m_startOfEditableArea
                || e->modifiers() & Qt::ControlModifier) {
            QTextCursor c(textCursor());
            c.setPosition(m_startOfEditableArea);
            setTextCursor(c);
            keyConsumed = true;
        }
        break;

        //  Ctrl+Right: Moves the cursor one word to the right.
        //  Right: Moves the cursor one character to the right.
    case Qt::Key_Right:
        if ( !(e->modifiers() & Qt::ControlModifier)
             && textCursor().position() <= m_startOfEditableArea) {
            QTextCursor c(textCursor());
            c.setPosition(m_startOfEditableArea);
            setTextCursor(c);
            keyConsumed = true;
        }
        break;

        //  Ctrl+C, Ctrl+Insert: Allow to Copy the selected text to the clipboard.
    case Qt::Key_C:
    case Qt::Key_Insert:
        if (textCursor().selectionStart() < m_startOfEditableArea &&
                !(e->modifiers() & Qt::ControlModifier))
            keyConsumed = true;
        break;

    default:
        //      Disallow any other keys in the prompt area
        if (textCursor().selectionStart() < m_startOfEditableArea)
            keyConsumed = true;
        break;
    }

    if (!keyConsumed)
        QTextEdit::keyPressEvent(e);
}

void QmlConsoleEdit::contextMenuEvent(QContextMenuEvent *event)
{
    // TODO:: on right click the editor closes
    return QTextEdit::contextMenuEvent(event);

    QTextCursor cursor = textCursor();
    bool editable = cursor.position() > m_startOfEditableArea;
    QMenu *menu = new QMenu();
    QAction *a;

    a = menu->addAction(tr("Cu&t"), this, SLOT(cut()));
    a->setEnabled(cursor.hasSelection() && editable);

    a = menu->addAction(tr("&Copy"), this, SLOT(copy()));
    a->setEnabled(cursor.hasSelection());

    a = menu->addAction(tr("&Paste"), this, SLOT(paste()));
    a->setEnabled(canPaste() && editable);

    menu->addSeparator();
    a = menu->addAction(tr("Select &All"), this, SLOT(selectAll()));
    a->setEnabled(!document()->isEmpty());

    menu->addSeparator();
    menu->addAction(tr("C&lear"), this, SLOT(clear()));

    menu->exec(event->globalPos());

    delete menu;
}

void QmlConsoleEdit::focusOutEvent(QFocusEvent * /*e*/)
{
    emit editingFinished();
}

void QmlConsoleEdit::handleUpKey()
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
                        index, QmlConsoleItemModel::TypeRole).toInt()) {
                m_historyIndex = index;
                replaceCurrentScript(model->data(index, Qt::DisplayRole).toString());
                break;
            }
        }
    }
}

void QmlConsoleEdit::handleDownKey()
{
    QTC_ASSERT(m_historyIndex.isValid(), return);
    int currentRow = m_historyIndex.row();
    const QAbstractItemModel *model = m_historyIndex.model();
    while (currentRow < model->rowCount() - 1) {
        currentRow++;
        if (model->hasIndex(currentRow, 0)) {
            QModelIndex index = model->index(currentRow, 0);
            if (ConsoleItem::InputType == (ConsoleItem::ItemType)model->data(
                        index, QmlConsoleItemModel::TypeRole).toInt()) {
                m_historyIndex = index;
                if (currentRow == model->rowCount() - 1)
                    replaceCurrentScript(m_cachedScript);
                else
                    replaceCurrentScript(model->data(index, Qt::DisplayRole).toString());
                break;
            }
        }
    }
}

QString QmlConsoleEdit::getCurrentScript() const
{
    QTextCursor cursor = textCursor();
    cursor.setPosition(m_startOfEditableArea);
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    QString script = cursor.selectedText();
    return script.trimmed();
}

void QmlConsoleEdit::replaceCurrentScript(const QString &script)
{
    QTextCursor cursor = textCursor();
    cursor.setPosition(m_startOfEditableArea);
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();
    cursor.insertText(script);
    setTextCursor(cursor);
}

} // Internal
} // QmlJSTools
