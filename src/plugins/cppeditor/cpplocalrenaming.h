/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

public slots:
    void updateSelectionsForVariableUnderCursor(const QList<QTextEdit::ExtraSelection> &selections);

signals:
    void finished();
    void processKeyPressNormally(QKeyEvent *e);

private slots:
    void onContentsChangeOfEditorWidgetDocument(int position, int charsRemoved, int charsAdded);

private:
    CppLocalRenaming();

    // The "rename selection" is the local use selection on which the user started the renaming
    bool findRenameSelection(int cursorPosition);
    void forgetRenamingSelection();
    bool isWithinRenameSelection(int position);

    QTextEdit::ExtraSelection &renameSelection();
    int renameSelectionBegin() { return renameSelection().cursor.position(); }
    int renameSelectionEnd() { return renameSelection().cursor.anchor(); }

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
