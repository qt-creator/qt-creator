// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <texteditor/texteditor.h>
#include <utils/uniqueobjectptr.h>

#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <memory>

namespace Core {
class FindToolBarPlaceHolder;
}
namespace QmlDesigner {

class TextEditorView;
class TextEditorStatusBar;

class TextEditorWidget : public QWidget {

    Q_OBJECT

public:
    TextEditorWidget(TextEditorView *textEditorView);

    void setTextEditor(Utils::UniqueObjectLatePtr<TextEditor::BaseTextEditor> textEditor);

    TextEditor::BaseTextEditor *textEditor() const
    {
        return m_textEditor.get();
    }

    void contextHelp(const Core::IContext::HelpCallback &callback) const;
    void jumpTextCursorToSelectedModelNode();
    void gotoCursorPosition(int line, int column);

    void setStatusText(const QString &text);
    void clearStatusBar();

    int currentLine() const;

    void setBlockCursorSelectionSynchronisation(bool b);

protected:
    bool eventFilter(QObject *object, QEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *dragEnterEvent) override;
    void dropEvent(QDropEvent *dropEvent) override;

private:
    void updateSelectionByCursorPosition();

    Utils::UniqueObjectLatePtr<TextEditor::BaseTextEditor> m_textEditor;
    QPointer<TextEditorView> m_textEditorView;
    QTimer m_updateSelectionTimer;
    TextEditorStatusBar *m_statusBar = nullptr;
    Core::FindToolBarPlaceHolder *m_findToolBar = nullptr;
    QVBoxLayout *m_layout = nullptr;
    bool m_blockCursorSelectionSynchronisation = false;
    bool m_blockRoundTrip = false;
};

} // namespace QmlDesigner
