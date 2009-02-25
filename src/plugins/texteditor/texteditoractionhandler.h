/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef TEXTEDITORACTIONHANDLER_H
#define TEXTEDITORACTIONHANDLER_H

#include "texteditor_global.h"
#include "basetexteditor.h"

#include "coreplugin/icontext.h"

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPointer>

namespace TextEditor {

class BaseTextEditor;

// Redirects slots from global actions to the respective editor.

class TEXTEDITOR_EXPORT TextEditorActionHandler : public QObject
{
    Q_OBJECT

public:
    enum OptionalActionsMask {
        None = 0,
        Format = 1,
        UnCommentSelection = 2,
        UnCollapseAll = 4
    };

    TextEditorActionHandler(const QString &context, uint optionalActions = None);
    void setupActions(BaseTextEditor *editor);

    void initializeActions();

public slots:
    void updateActions();
    void updateRedoAction();
    void updateUndoAction();
    void updateCopyAction();

protected:
    const QPointer<BaseTextEditor> &currentEditor() const;
    QAction *registerNewAction(const QString &id, const QString &title = QString());
    QAction *registerNewAction(const QString &id, QObject *receiver, const char *slot,
                               const QString &title = QString());

    enum UpdateMode { NoEditor , ReadOnlyMode, WriteMode };
    UpdateMode updateMode() const;

    virtual void createActions();
    virtual bool supportsAction(const QString &id) const;
    virtual void updateActions(UpdateMode um);

private slots:
    void undoAction();
    void redoAction();
    void copyAction();
    void cutAction();
    void pasteAction();
    void selectAllAction();
    void gotoAction();
    void printAction();
    void formatAction();
    void setVisualizeWhitespace(bool);
    void cleanWhitespace();
    void setTextWrapping(bool);
    void unCommentSelection();
    void unCollapseAll();
    void collapse();
    void expand();
    void deleteLine();
    void selectEncoding();
    void increaseFontSize();
    void decreaseFontSize();
    void gotoBlockStart();
    void gotoBlockEnd();
    void gotoBlockStartWithSelection();
    void gotoBlockEndWithSelection();
    void selectBlockUp();
    void selectBlockDown();
    void moveLineUp();
    void moveLineDown();
    void updateCurrentEditor(Core::IContext *object);

private:
    QAction *m_undoAction;
    QAction *m_redoAction;
    QAction *m_copyAction;
    QAction *m_cutAction;
    QAction *m_pasteAction;
    QAction *m_selectAllAction;
    QAction *m_gotoAction;
    QAction *m_printAction;
    QAction *m_formatAction;
    QAction *m_visualizeWhitespaceAction;
    QAction *m_cleanWhitespaceAction;
    QAction *m_textWrappingAction;
    QAction *m_unCommentSelectionAction;
    QAction *m_unCollapseAllAction;
    QAction *m_collapseAction;
    QAction *m_expandAction;
    QAction *m_deleteLineAction;
    QAction *m_selectEncodingAction;
    QAction *m_increaseFontSizeAction;
    QAction *m_decreaseFontSizeAction;
    QAction *m_gotoBlockStartAction;
    QAction *m_gotoBlockEndAction;
    QAction *m_gotoBlockStartWithSelectionAction;
    QAction *m_gotoBlockEndWithSelectionAction;
    QAction *m_selectBlockUpAction;
    QAction *m_selectBlockDownAction;
    QAction *m_moveLineUpAction;
    QAction *m_moveLineDownAction;

    uint m_optionalActions;
    QPointer<BaseTextEditor> m_currentEditor;
    QList<int> m_contextId;
    bool m_initialized;
};

} // namespace TextEditor

#endif // TEXTEDITORACTIONHANDLER_H
