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

#ifndef TEXTEDITORACTIONHANDLER_H
#define TEXTEDITORACTIONHANDLER_H

#include "texteditor_global.h"

#include <coreplugin/icontext.h>
#include <coreplugin/id.h>

#include <QObject>
#include <QPointer>
#include <QList>

QT_FORWARD_DECLARE_CLASS(QAction)

namespace Core {
class ICore;
class IEditor;
}

namespace TextEditor {
class BaseTextEditorWidget;

// Redirects slots from global actions to the respective editor.

class TEXTEDITOR_EXPORT TextEditorActionHandler : public QObject
{
    Q_OBJECT

public:
    enum OptionalActionsMask {
        None = 0,
        Format = 1,
        UnCommentSelection = 2,
        UnCollapseAll = 4,
        FollowSymbolUnderCursor = 8,
        JumpToFileUnderCursor = 16
    };

    explicit TextEditorActionHandler(const char *context, uint optionalActions = None);
    ~TextEditorActionHandler();

    void setupActions(BaseTextEditorWidget *editor);

    void initializeActions();

public slots:
    void updateActions();
    void updateRedoAction();
    void updateUndoAction();
    void updateCopyAction();

protected:
    const QPointer<BaseTextEditorWidget> &currentEditor() const;
    QAction *registerNewAction(const Core::Id &id, bool scriptable = false, const QString &title = QString());
    QAction *registerNewAction(const Core::Id &id, QObject *receiver, const char *slot, bool scriptable = false,
                               const QString &title = QString());

    enum UpdateMode { ReadOnlyMode, WriteMode };
    UpdateMode updateMode() const;

    virtual void createActions();
    virtual bool supportsAction(const Core::Id &id) const;
    virtual void updateActions(UpdateMode um);

private slots:
    void undoAction();
    void redoAction();
    void copyAction();
    void cutAction();
    void pasteAction();
    void circularPasteAction();
    void switchUtf8bomAction();
    void selectAllAction();
    void gotoAction();
    void printAction();
    void formatAction();
    void rewrapParagraphAction();
    void setVisualizeWhitespace(bool);
    void cleanWhitespace();
    void setTextWrapping(bool);
    void unCommentSelection();
    void unfoldAll();
    void fold();
    void unfold();
    void cutLine();
    void copyLine();
    void deleteLine();
    void deleteEndOfWord();
    void deleteEndOfWordCamelCase();
    void deleteStartOfWord();
    void deleteStartOfWordCamelCase();
    void selectEncoding();
    void increaseFontSize();
    void decreaseFontSize();
    void resetFontSize();
    void gotoBlockStart();
    void gotoBlockEnd();
    void gotoBlockStartWithSelection();
    void gotoBlockEndWithSelection();
    void selectBlockUp();
    void selectBlockDown();
    void moveLineUp();
    void moveLineDown();
    void copyLineUp();
    void copyLineDown();
    void joinLines();
    void insertLineAbove();
    void insertLineBelow();
    void uppercaseSelection();
    void lowercaseSelection();
    void updateCurrentEditor(Core::IEditor *editor);
    void indent();
    void unindent();
    void openLinkUnderCursor();

    void gotoLineStart();
    void gotoLineStartWithSelection();
    void gotoLineEnd();
    void gotoLineEndWithSelection();
    void gotoNextLine();
    void gotoNextLineWithSelection();
    void gotoPreviousLine();
    void gotoPreviousLineWithSelection();
    void gotoPreviousCharacter();
    void gotoPreviousCharacterWithSelection();
    void gotoNextCharacter();
    void gotoNextCharacterWithSelection();
    void gotoPreviousWord();
    void gotoPreviousWordWithSelection();
    void gotoNextWord();
    void gotoNextWordWithSelection();
    void gotoPreviousWordCamelCase();
    void gotoPreviousWordCamelCaseWithSelection();
    void gotoNextWordCamelCase();
    void gotoNextWordCamelCaseWithSelection();


private:
    QAction *m_undoAction;
    QAction *m_redoAction;
    QAction *m_copyAction;
    QAction *m_cutAction;
    QAction *m_pasteAction;
    QAction *m_circularPasteAction;
    QAction *m_switchUtf8bomAction;
    QAction *m_selectAllAction;
    QAction *m_gotoAction;
    QAction *m_printAction;
    QAction *m_formatAction;
    QAction *m_rewrapParagraphAction;
    QAction *m_visualizeWhitespaceAction;
    QAction *m_cleanWhitespaceAction;
    QAction *m_textWrappingAction;
    QAction *m_unCommentSelectionAction;
    QAction *m_unfoldAllAction;
    QAction *m_foldAction;
    QAction *m_unfoldAction;
    QAction *m_cutLineAction;
    QAction *m_copyLineAction;
    QAction *m_deleteLineAction;
    QAction *m_deleteEndOfWordAction;
    QAction *m_deleteEndOfWordCamelCaseAction;
    QAction *m_deleteStartOfWordAction;
    QAction *m_deleteStartOfWordCamelCaseAction;
    QAction *m_selectEncodingAction;
    QAction *m_increaseFontSizeAction;
    QAction *m_decreaseFontSizeAction;
    QAction *m_resetFontSizeAction;
    QAction *m_gotoBlockStartAction;
    QAction *m_gotoBlockEndAction;
    QAction *m_gotoBlockStartWithSelectionAction;
    QAction *m_gotoBlockEndWithSelectionAction;
    QAction *m_selectBlockUpAction;
    QAction *m_selectBlockDownAction;
    QAction *m_moveLineUpAction;
    QAction *m_moveLineDownAction;
    QAction *m_copyLineUpAction;
    QAction *m_copyLineDownAction;
    QAction *m_joinLinesAction;
    QAction *m_insertLineAboveAction;
    QAction *m_insertLineBelowAction;
    QAction *m_upperCaseSelectionAction;
    QAction *m_lowerCaseSelectionAction;
    QAction *m_indentAction;
    QAction *m_unindentAction;
    QAction *m_followSymbolAction;
    QAction *m_jumpToFileAction;
    QList<QAction *> m_modifyingActions;

    uint m_optionalActions;
    QPointer<BaseTextEditorWidget> m_currentEditor;
    Core::Context m_contextId;
    bool m_initialized;
};

} // namespace TextEditor

#endif // TEXTEDITORACTIONHANDLER_H
