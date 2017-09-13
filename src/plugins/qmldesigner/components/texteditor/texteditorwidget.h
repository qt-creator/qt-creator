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
#pragma once

#include <texteditor/texteditor.h>

#include <QWidget>
#include <QTimer>

#include <memory>

namespace QmlDesigner {

class TextEditorView;
class TextEditorStatusBar;

class TextEditorWidget : public QWidget {

    Q_OBJECT

public:
    TextEditorWidget(TextEditorView *textEditorView);

    void setTextEditor(TextEditor::BaseTextEditor *textEditor);

    TextEditor::BaseTextEditor *textEditor() const
    {
        return m_textEditor.get();
    }

    QString contextHelpId() const;
    void jumpTextCursorToSelectedModelNode();
    void gotoCursorPosition(int line, int column);

    void setStatusText(const QString &text);
    void clearStatusBar();

    int currentLine() const;

    void setBlockCursorSelectionSynchronisation(bool b);

protected:
    bool eventFilter(QObject *object, QEvent *event) override;

private:
    void updateSelectionByCursorPosition();

    std::unique_ptr<TextEditor::BaseTextEditor> m_textEditor;
    QPointer<TextEditorView> m_textEditorView;
    QTimer m_updateSelectionTimer;
    TextEditorStatusBar *m_statusBar;
    bool m_blockCursorSelectionSynchronisation = false;
};

} // namespace QmlDesigner
