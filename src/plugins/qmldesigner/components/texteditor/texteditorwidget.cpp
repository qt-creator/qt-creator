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

#include <texteditorview.h>
#include <rewriterview.h>

#include <theming.h>

#include <utils/fileutils.h>

#include <QVBoxLayout>

namespace QmlDesigner {

TextEditorWidget::TextEditorWidget(TextEditorView *textEditorView) : QWidget()
  , m_textEditorView(textEditorView)

{
    QBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);

    m_updateSelectionTimer.setSingleShot(true);
    m_updateSelectionTimer.setInterval(200);

    connect(&m_updateSelectionTimer, &QTimer::timeout, this, &TextEditorWidget::updateSelectionByCursorPosition);
    setStyleSheet(Theming::replaceCssColors(QString::fromUtf8(Utils::FileReader::fetchQrc(QLatin1String(":/qmldesigner/scrollbar.css")))));
}

void TextEditorWidget::setTextEditor(TextEditor::BaseTextEditor *textEditor) {
    m_textEditor.reset(textEditor);
    layout()->addWidget(textEditor->editorWidget());


    connect(textEditor->editorWidget(), &QPlainTextEdit::cursorPositionChanged,
            &m_updateSelectionTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
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
        if (modelNode.isValid())
            m_textEditorView->setSelectedModelNode(modelNode);
    }
}

void TextEditorWidget::jumpTextCursorToSelectedModelNode()
{
    ModelNode selectedNode;

    if (!m_textEditorView->selectedModelNodes().isEmpty())
        selectedNode = m_textEditorView->selectedModelNodes().first();

    if (selectedNode.isValid()) {
        RewriterView *rewriterView = m_textEditorView->model()->rewriterView();

        const int nodeOffset = rewriterView->nodeOffset(selectedNode);
        if (nodeOffset > 0) {
            const ModelNode currentSelectedNode = rewriterView->
                nodeAtTextCursorPosition(m_textEditor->editorWidget()->textCursor().position());

            if (currentSelectedNode != selectedNode) {
                int line, column;
                m_textEditor->editorWidget()->convertPosition(nodeOffset, &line, &column);
                m_textEditor->editorWidget()->gotoLine(line, column);
            }
        }
    }
}

} // namespace QmlDesigner
