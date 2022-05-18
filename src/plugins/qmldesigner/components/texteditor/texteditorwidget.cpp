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
#include <coreplugin/findplaceholder.h>
#include <rewriterview.h>

#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>

#include <theme.h>

#include <utils/fileutils.h>

#include <texteditor/textdocument.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QEvent>
#include <QScrollBar>
#include <QVBoxLayout>

#include <vector>
#include <algorithm>

namespace QmlDesigner {

TextEditorWidget::TextEditorWidget(TextEditorView *textEditorView)
    : QWidget()
    , m_textEditorView(textEditorView)
    , m_statusBar(new TextEditorStatusBar(this))
    , m_findToolBar(new Core::FindToolBarPlaceHolder(this))
    , m_layout(new QVBoxLayout(this))
{
    setAcceptDrops(true);

    m_statusBar->hide();

    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);
    m_layout->addWidget(m_statusBar);
    m_layout->addWidget(m_findToolBar);

    m_updateSelectionTimer.setSingleShot(true);
    m_updateSelectionTimer.setInterval(200);

    connect(&m_updateSelectionTimer, &QTimer::timeout, this, &TextEditorWidget::updateSelectionByCursorPosition);
}

void TextEditorWidget::setTextEditor(TextEditor::BaseTextEditor *textEditor)
{
    TextEditor::BaseTextEditor *oldEditor = m_textEditor.release();
    m_textEditor.reset(textEditor);

    if (textEditor) {
        m_layout->insertWidget(0, textEditor->editorWidget());

        setFocusProxy(textEditor->editorWidget());

        QmlDesignerPlugin::instance()->emitCurrentTextEditorChanged(textEditor);

        connect(textEditor->editorWidget(), &QPlainTextEdit::cursorPositionChanged, this, [this] {
            // Cursor position is changed by rewriter
            if (!m_blockCursorSelectionSynchronisation)
                m_updateSelectionTimer.start();
        });

        textEditor->editorWidget()->installEventFilter(this);

        static QString styleSheet = Theme::replaceCssColors(
            QString::fromUtf8(Utils::FileReader::fetchQrc(":/qmldesigner/scrollbar.css")));
        textEditor->editorWidget()->verticalScrollBar()->setStyleSheet(styleSheet);
        textEditor->editorWidget()->horizontalScrollBar()->setStyleSheet(styleSheet);
    }

    if (oldEditor)
        oldEditor->deleteLater();
}

void TextEditorWidget::contextHelp(const Core::IContext::HelpCallback &callback) const
{
    m_textEditorView->contextHelp(callback);
}

void TextEditorWidget::updateSelectionByCursorPosition()
{
    if (!m_textEditorView->model())
        return;

    const int cursorPosition = m_textEditor->editorWidget()->textCursor().position();
    RewriterView *rewriterView = m_textEditorView->model()->rewriterView();

    m_blockRoundTrip = true;
    if (rewriterView) {
        ModelNode modelNode = rewriterView->nodeAtTextCursorPosition(cursorPosition);
        if (modelNode.isValid() && !m_textEditorView->isSelectedModelNode(modelNode))
            m_textEditorView->setSelectedModelNode(modelNode);
    }
    m_blockRoundTrip = false;
}

void TextEditorWidget::jumpTextCursorToSelectedModelNode()
{
    if (m_blockRoundTrip)
        return;

    ModelNode selectedNode;

    if (hasFocus())
        return;

    if (m_textEditor && m_textEditor->editorWidget()->hasFocus())
        return;

    if (!m_textEditorView->selectedModelNodes().isEmpty())
        selectedNode = m_textEditorView->selectedModelNodes().constFirst();

    if (selectedNode.isValid()) {
        RewriterView *rewriterView = m_textEditorView->model()->rewriterView();

        const int nodeOffset = rewriterView->nodeOffset(selectedNode);
        if (nodeOffset > 0) {
            int line, column;
            m_textEditor->editorWidget()->convertPosition(nodeOffset, &line, &column);
            // line has to be 1 based, column 0 based!
            m_textEditor->editorWidget()->gotoLine(line, column - 1);
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
    m_statusBar->setVisible(!text.isEmpty());
}

void TextEditorWidget::clearStatusBar()
{
    m_statusBar->clearText();
    m_statusBar->hide();
}

int TextEditorWidget::currentLine() const
{
    if (m_textEditor)
        return m_textEditor->currentLine();
    return -1;
}

void TextEditorWidget::setBlockCursorSelectionSynchronisation(bool b)
{
    m_blockCursorSelectionSynchronisation = b;
}

bool TextEditorWidget::eventFilter(QObject *, QEvent *event)
{
    static std::vector<int> overrideKeys = { Qt::Key_Delete, Qt::Key_Backspace, Qt::Key_Insert,
                                             Qt::Key_Escape };

    static std::vector<QKeySequence> overrideSequences = { QKeySequence::SelectAll, QKeySequence::Cut,
                                                           QKeySequence::Copy, QKeySequence::Delete,
                                                           QKeySequence::Paste, QKeySequence::Undo,
                                                           QKeySequence::Redo, QKeySequence(Qt::CTRL | Qt::ALT),
                                                           QKeySequence(Qt::Key_Left | Qt::CTRL),
                                                           QKeySequence(Qt::Key_Right | Qt::CTRL),
                                                           QKeySequence(Qt::Key_Up | Qt::CTRL),
                                                           QKeySequence(Qt::Key_Down | Qt::CTRL)
                                                         };
    if (event->type() == QEvent::ShortcutOverride) {
        auto keyEvent = static_cast<QKeyEvent *>(event);

        if (std::find(overrideKeys.begin(), overrideKeys.end(), keyEvent->key()) != overrideKeys.end()) {
            if (keyEvent->key() == Qt::Key_Escape)
                m_findToolBar->hide();

            keyEvent->accept();
            return true;
        }

        static const Qt::KeyboardModifiers relevantModifiers = Qt::ShiftModifier
                                                             | Qt::ControlModifier
                                                             | Qt::AltModifier
                                                             | Qt::MetaModifier;

        const QKeySequence keySqeuence(keyEvent->key() | (keyEvent->modifiers() & relevantModifiers));
        for (const QKeySequence &overrideSequence : overrideSequences) {
            if (keySqeuence.matches(overrideSequence)) {
                keyEvent->accept();
                return true;
            }
        }
    }
    return false;
}

void TextEditorWidget::dragEnterEvent(QDragEnterEvent *dragEnterEvent)
{
    const DesignerActionManager &actionManager = QmlDesignerPlugin::instance()
                                                     ->viewManager().designerActionManager();
    if (actionManager.externalDragHasSupportedAssets(dragEnterEvent->mimeData()))
        dragEnterEvent->acceptProposedAction();
}

void TextEditorWidget::dropEvent(QDropEvent *dropEvent)
{
    const DesignerActionManager &actionManager = QmlDesignerPlugin::instance()
                                                     ->viewManager().designerActionManager();
    actionManager.handleExternalAssetsDrop(dropEvent->mimeData());
}

void TextEditorWidget::focusOutEvent(QFocusEvent *focusEvent)
{
    QmlDesignerPlugin::emitUsageStatisticsTime(Constants::EVENT_TEXTEDITOR_TIME,
                                               m_usageTimer.elapsed());
    QWidget::focusOutEvent(focusEvent);
}

void TextEditorWidget::focusInEvent(QFocusEvent *focusEvent)
{
    m_usageTimer.restart();
    QWidget::focusInEvent(focusEvent);
}

} // namespace QmlDesigner
