// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef CPPLOCALRENAMING
#define CPPLOCALRENAMING

#include <texteditor/texteditorconstants.h>

#include <QTextEdit>

namespace TextEditor { class TextEditorWidget; }

namespace CppEditor {
namespace Internal {

class CppLocalRenaming : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(CppLocalRenaming)

public:
    explicit CppLocalRenaming(TextEditor::TextEditorWidget *editorWidget);

    bool start();
    bool isActive() const;
    void stop();

    // Delegates for the editor widget
    bool handlePaste();
    bool handleCut();
    bool handleSelectAll();

    // E.g. limit navigation keys to selection, stop() on Esc/Return or delegate
    // to BaseTextEditorWidget::keyPressEvent()
    bool handleKeyPressEvent(QKeyEvent *e);

    bool encourageApply();
    void onContentsChangeOfEditorWidgetDocument(int position, int charsRemoved, int charsAdded);

    void updateSelectionsForVariableUnderCursor(const QList<QTextEdit::ExtraSelection> &selections);
    bool isSameSelection(int cursorPosition) const;

signals:
    void finished();
    void processKeyPressNormally(QKeyEvent *e);

private:
    CppLocalRenaming();

    // The "rename selection" is the local use selection on which the user started the renaming
    bool findRenameSelection(int cursorPosition);
    void forgetRenamingSelection();
    static bool isWithinSelection(const QTextEdit::ExtraSelection &selection, int position);
    bool isWithinRenameSelection(int position);

    QTextEdit::ExtraSelection &renameSelection();
    int renameSelectionBegin() { return renameSelection().cursor.selectionStart(); }
    int renameSelectionEnd() { return renameSelection().cursor.selectionEnd(); }

    void updateRenamingSelectionCursor(const QTextCursor &cursor);
    void updateRenamingSelectionFormat(const QTextCharFormat &format);

    void changeOtherSelectionsText(const QString &text);

    void startRenameChange();
    void finishRenameChange();

    void updateEditorWidgetWithSelections();

    QTextCharFormat textCharFormat(TextEditor::TextStyle category) const;

private:
    TextEditor::TextEditorWidget *m_editorWidget;

    QList<QTextEdit::ExtraSelection> m_selections;
    int m_renameSelectionIndex;
    bool m_modifyingSelections;
    bool m_renameSelectionChanged;
    bool m_firstRenameChangeExpected;
};

} // namespace Internal
} // namespace CppEditor

#endif // CPPLOCALRENAMING
