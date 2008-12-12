/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef TEXTEDITORACTIONHANDLER_H
#define TEXTEDITORACTIONHANDLER_H

#include "texteditor_global.h"
#include "basetexteditor.h"

#include "coreplugin/icontext.h"
#include "coreplugin/icore.h"

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QList>

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

    TextEditorActionHandler(Core::ICore *core,
                            const QString &context,
                            uint optionalActions = None);
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
    Core::ICore *core() const;

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
    Core::ICore *m_core;
    QList<int> m_contextId;
    bool m_initialized;
};

} // namespace TextEditor

#endif // TEXTEDITORACTIONHANDLER_H
