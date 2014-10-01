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
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef BASETEXTEDITOR_P_H
#define BASETEXTEDITOR_P_H

#include "basetexteditor.h"
#include "behaviorsettings.h"
#include "displaysettings.h"
#include "marginsettings.h"
#include "fontsettings.h"
#include "refactoroverlay.h"

#include <coreplugin/id.h>
#include <utils/changeset.h>

#include <QBasicTimer>
#include <QSharedPointer>
#include <QPointer>
#include <QScopedPointer>
#include <QTextBlock>
#include <QTimer>

namespace TextEditor {

class BaseTextDocument;
class CodeAssistant;

namespace Internal {
class TextEditorOverlay;
class ClipboardAssistProvider;

class TEXTEDITOR_EXPORT BaseTextBlockSelection
{
public:
    BaseTextBlockSelection()
        : positionBlock(0), positionColumn(0)
        , anchorBlock(0) , anchorColumn(0){}
    BaseTextBlockSelection(const BaseTextBlockSelection &other);

    void clear();
    QTextCursor selection(const BaseTextDocument *baseTextDocument) const;
    QTextCursor cursor(const BaseTextDocument *baseTextDocument) const;
    void fromPostition(int positionBlock, int positionColumn, int anchorBlock, int anchorColumn);
    bool hasSelection() { return !(positionBlock == anchorBlock && positionColumn == anchorColumn); }

    // defines the first block
    inline int firstBlockNumber() const { return qMin(positionBlock, anchorBlock); }
     // defines the last block
    inline int lastBlockNumber() const { return qMax(positionBlock, anchorBlock); }
    // defines the first visual column of the selection
    inline int firstVisualColumn() const { return qMin(positionColumn, anchorColumn); }
    // defines the last visual column of the selection
    inline int lastVisualColumn() const { return qMax(positionColumn, anchorColumn); }

public:
    int positionBlock;
    int positionColumn;
    int anchorBlock;
    int anchorColumn;

private:
    QTextCursor cursor(const BaseTextDocument *baseTextDocument, bool fullSelection) const;
};

//================BaseTextEditorPrivate==============

struct BaseTextEditorPrivateHighlightBlocks
{
    QList<int> open;
    QList<int> close;
    QList<int> visualIndent;
    inline int count() const { return visualIndent.size(); }
    inline bool isEmpty() const { return open.isEmpty() || close.isEmpty() || visualIndent.isEmpty(); }
    inline bool operator==(const BaseTextEditorPrivateHighlightBlocks &o) const {
        return (open == o.open && close == o.close && visualIndent == o.visualIndent);
    }
    inline bool operator!=(const BaseTextEditorPrivateHighlightBlocks &o) const { return !(*this == o); }
};


class BaseTextEditorWidgetPrivate
{
    BaseTextEditorWidgetPrivate(const BaseTextEditorWidgetPrivate &);
    BaseTextEditorWidgetPrivate &operator=(const BaseTextEditorWidgetPrivate &);

public:
    BaseTextEditorWidgetPrivate();
    ~BaseTextEditorWidgetPrivate();

    void setupDocumentSignals();
    void updateLineSelectionColor();

    void print(QPrinter *printer);

    BaseTextEditorWidget *q;
    bool m_contentsChanged;
    bool m_lastCursorChangeWasInteresting;

    QSharedPointer<BaseTextDocument> m_document;
    QByteArray m_tempState;
    QByteArray m_tempNavigationState;

    bool m_parenthesesMatchingEnabled;

    // parentheses matcher
    bool m_formatRange;
    QTextCharFormat m_mismatchFormat;
    QTimer m_parenthesesMatchingTimer;
    // end parentheses matcher

    QWidget *m_extraArea;

    Core::Id m_tabSettingsId;
    ICodeStylePreferences *m_codeStylePreferences;
    DisplaySettings m_displaySettings;
    MarginSettings m_marginSettings;
    bool m_fontSettingsNeedsApply;
    BehaviorSettings m_behaviorSettings;

    int extraAreaSelectionAnchorBlockNumber;
    int extraAreaToggleMarkBlockNumber;
    int extraAreaHighlightFoldedBlockNumber;

    TextEditorOverlay *m_overlay;
    TextEditorOverlay *m_snippetOverlay;
    TextEditorOverlay *m_searchResultOverlay;
    bool snippetCheckCursor(const QTextCursor &cursor);
    void snippetTabOrBacktab(bool forward);

    RefactorOverlay *m_refactorOverlay;

    QBasicTimer foldedBlockTimer;
    int visibleFoldedBlockNumber;
    int suggestedVisibleFoldedBlockNumber;
    void clearVisibleFoldedBlock();
    bool m_mouseOnFoldedMarker;
    void foldLicenseHeader();

    QBasicTimer autoScrollTimer;
    uint m_marksVisible : 1;
    uint m_codeFoldingVisible : 1;
    uint m_codeFoldingSupported : 1;
    uint m_revisionsVisible : 1;
    uint m_lineNumbersVisible : 1;
    uint m_highlightCurrentLine : 1;
    uint m_requestMarkEnabled : 1;
    uint m_lineSeparatorsAllowed : 1;
    uint autoParenthesisOverwriteBackup : 1;
    uint surroundWithEnabledOverwriteBackup : 1;
    uint m_maybeFakeTooltipEvent : 1;
    int m_visibleWrapColumn;

    BaseTextEditorWidget::Link m_currentLink;
    bool m_linkPressed;

    QRegExp m_searchExpr;
    Core::FindFlags m_findFlags;
    void highlightSearchResults(const QTextBlock &block, TextEditorOverlay *overlay);
    QTimer m_delayedUpdateTimer;

    BaseTextEditor *m_editor;

    QList<QTextEdit::ExtraSelection> m_extraSelections[BaseTextEditorWidget::NExtraSelectionKinds];

    // block selection mode
    bool m_inBlockSelectionMode;
    QString copyBlockSelection();
    void insertIntoBlockSelection(const QString &text = QString());
    void setCursorToColumn(QTextCursor &cursor, int column,
                          QTextCursor::MoveMode moveMode = QTextCursor::MoveAnchor);
    void removeBlockSelection();
    void enableBlockSelection(const QTextCursor &cursor);
    void enableBlockSelection(int positionBlock, int positionColumn,
                              int anchorBlock, int anchorColumn);
    void disableBlockSelection(bool keepSelection = true);
    void resetCursorFlashTimer();
    QBasicTimer m_cursorFlashTimer;
    bool m_cursorVisible;
    bool m_moveLineUndoHack;

    QTextCursor m_findScopeStart;
    QTextCursor m_findScopeEnd;
    int m_findScopeVerticalBlockSelectionFirstColumn;
    int m_findScopeVerticalBlockSelectionLastColumn;

    QTextCursor m_selectBlockAnchor;

    Internal::BaseTextBlockSelection m_blockSelection;

    void moveCursorVisible(bool ensureVisible = true);

    int visualIndent(const QTextBlock &block) const;
    BaseTextEditorPrivateHighlightBlocks m_highlightBlocksInfo;
    QTimer m_highlightBlocksTimer;

    QScopedPointer<CodeAssistant> m_codeAssistant;
    bool m_assistRelevantContentAdded;

    QPointer<BaseTextEditorAnimator> m_animator;
    int m_cursorBlockNumber;
    int m_blockCount;

    QPoint m_markDragStart;
    bool m_markDragging;

    QScopedPointer<AutoCompleter> m_autoCompleter;

    QScopedPointer<Internal::ClipboardAssistProvider> m_clipboardAssistProvider;
};

} // namespace Internal
} // namespace TextEditor

#endif // BASETEXTEDITOR_P_H
