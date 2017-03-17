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

#include "texteditorwidget.h"

#include <texteditorstatusbar.h>
#include <texteditorview.h>

#include <rewriterview.h>

#include <qmldesignerplugin.h>

#include <theme.h>

#include <utils/fileutils.h>

#include <texteditor/textdocument.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QEvent>
#include <QVBoxLayout>

#include <vector>
#include <algorithm>

namespace QmlDesigner {

TextEditorWidget::TextEditorWidget(TextEditorView *textEditorView)
    : QWidget()
    , m_textEditorView(textEditorView)
    , m_statusBar(new TextEditorStatusBar(this))
{
    QBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_statusBar);

    m_updateSelectionTimer.setSingleShot(true);
    m_updateSelectionTimer.setInterval(200);

    connect(&m_updateSelectionTimer, &QTimer::timeout, this, &TextEditorWidget::updateSelectionByCursorPosition);
    setStyleSheet(Theme::replaceCssColors(QString::fromUtf8(Utils::FileReader::fetchQrc(QLatin1String(":/qmldesigner/scrollbar.css")))));
}

void TextEditorWidget::setTextEditor(TextEditor::BaseTextEditor *textEditor)
{
    TextEditor::BaseTextEditor *oldEditor = m_textEditor.release();
    m_textEditor.reset(textEditor);

    if (textEditor) {
        layout()->removeWidget(m_statusBar);
        layout()->addWidget(textEditor->editorWidget());
        layout()->addWidget(m_statusBar);
        setFocusProxy(textEditor->editorWidget());

        QmlDesignerPlugin::instance()->emitCurrentTextEditorChanged(textEditor);

        connect(textEditor->editorWidget(), &QPlainTextEdit::cursorPositionChanged,
                this, [this]() {
            /* Cursor position is changed by rewriter */
            if (!m_blockCurserSelectionSyncronisation)
                m_updateSelectionTimer.start();
        });

        textEditor->editorWidget()->installEventFilter(this);
    }

    if (oldEditor)
        oldEditor->deleteLater();
}

QString TextEditorWidget::contextHelpId() const
{
    return m_textEditorView->contextHelpId();
}

void TextEditorWidget::updateSelectionByCursorPosition()
{
    /* Because of the timer we have to be careful. */
    if (!m_textEditorView->model())
        return;

    const int cursorPosition = m_textEditor->editorWidget()->textCursor().position();
    RewriterView *rewriterView = m_textEditorView->model()->rewriterView();

    if (rewriterView) {
        ModelNode modelNode = rewriterView->nodeAtTextCursorPosition(cursorPosition);
        if (modelNode.isValid() && !m_textEditorView->isSelectedModelNode(modelNode))
            m_textEditorView->setSelectedModelNode(modelNode);
    }
}

void TextEditorWidget::jumpTextCursorToSelectedModelNode()
{
    ModelNode selectedNode;

    if (hasFocus())
        return;

    if (m_textEditor && m_textEditor->editorWidget()->hasFocus())
        return;

    if (!m_textEditorView->selectedModelNodes().isEmpty())
        selectedNode = m_textEditorView->selectedModelNodes().first();

    if (selectedNode.isValid()) {
        RewriterView *rewriterView = m_textEditorView->model()->rewriterView();

        const int nodeOffset = rewriterView->nodeOffset(selectedNode);
        if (nodeOffset > 0) {
            if (!rewriterView->nodeContainsCursor(selectedNode, m_textEditor->editorWidget()->textCursor().position())) {
                int line, column;
                m_textEditor->editorWidget()->convertPosition(nodeOffset, &line, &column);
                m_textEditor->editorWidget()->gotoLine(line, column);
            }
        }
    }
    m_updateSelectionTimer.stop();
}

void TextEditorWidget::gotoCursorPosition(int line, int column)
{
    if (m_textEditor) {
        m_textEditor->editorWidget()->gotoLine(line, column);
        m_textEditor->editorWidget()->setFocus();
    }
}

void TextEditorWidget::setStatusText(const QString &text)
{
    m_statusBar->setText(text);
}

void TextEditorWidget::clearStatusBar()
{
    m_statusBar->clearText();
}

int TextEditorWidget::currentLine() const
{
    if (m_textEditor)
        return m_textEditor->currentLine();
    return -1;
}

void TextEditorWidget::setBlockCurserSelectionSyncronisation(bool b)
{
    m_blockCurserSelectionSyncronisation = b;
}

bool TextEditorWidget::eventFilter( QObject *, QEvent *event)
{
    static std::vector<int> overrideKeys = { Qt::Key_Delete, Qt::Key_Backspace, Qt::Key_Left,
                                             Qt::Key_Right, Qt::Key_Up, Qt::Key_Down, Qt::Key_Insert,
                                             Qt::Key_Escape, Qt::Key_Home, Qt::Key_End };

    static std::vector<QKeySequence> overrideSequences = { QKeySequence::SelectAll, QKeySequence::Cut,
                                                          QKeySequence::Copy, QKeySequence::Delete,
                                                          QKeySequence::Paste, QKeySequence::Undo,
                                                          QKeySequence::Redo, QKeySequence(Qt::CTRL + Qt::ALT) };
    if (event->type() == QEvent::ShortcutOverride) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

        if (std::find(overrideKeys.begin(), overrideKeys.end(), keyEvent->key()) != overrideKeys.end()) {
            keyEvent->accept();
            return true;
        }

        QKeySequence keySqeuence(keyEvent->key() | keyEvent->modifiers());
        for (QKeySequence overrideSequence : overrideSequences)
            if (keySqeuence.matches(overrideSequence)) {
                keyEvent->accept();
                return true;
            }
    }
    return false;
}


} // namespace QmlDesigner
