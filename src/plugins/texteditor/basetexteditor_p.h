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

#ifndef BASETEXTEDITOR_P_H
#define BASETEXTEDITOR_P_H

#include "basetexteditor.h"
#include "behaviorsettings.h"
#include "displaysettings.h"
#include "fontsettings.h"
#include "refactoroverlay.h"

#include <coreplugin/id.h>
#include <utils/changeset.h>

#include <QBasicTimer>
#include <QSharedPointer>
#include <QPointer>
#include <QScopedPointer>
#include <QTextBlock>

namespace TextEditor {

class BaseTextDocument;
class TextEditorActionHandler;
class CodeAssistant;

namespace Internal {
class TextEditorOverlay;
class ClipboardAssistProvider;

class TEXTEDITOR_EXPORT BaseTextBlockSelection
{
public:

    bool isValid() const{ return !firstBlock.isNull() && !lastBlock.isNull(); }
    void clear() { firstBlock = lastBlock = QTextCursor(); }

    QTextCursor firstBlock; // defines the first block
    QTextCursor lastBlock; // defines the last block
    int firstVisualColumn; // defines the first visual column of the selection
    int lastVisualColumn; // defines the last visual column of the selection
    enum Anchor {TopLeft = 0, TopRight, BottomLeft, BottomRight} anchor;
    BaseTextBlockSelection():firstVisualColumn(0), lastVisualColumn(0), anchor(BottomRight){}
    void moveAnchor(int blockNumber, int visualColumn);
    inline int anchorColumnNumber() const { return (anchor % 2) ? lastVisualColumn : firstVisualColumn; }
    inline int anchorBlockNumber() const {
        return (anchor <= TopRight ? firstBlock.blockNumber() : lastBlock.blockNumber()); }
    QTextCursor selection(const TabSettings &ts) const;
    void fromSelection(const TabSettings &ts, const QTextCursor &selection);
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

    void setupBasicEditActions(TextEditorActionHandler *actionHandler);
    void setupDocumentSignals(const QSharedPointer<BaseTextDocument> &document);
    void updateLineSelectionColor();

    void print(QPrinter *printer);

    QTextBlock m_firstVisible;
    int m_lastScrollPos;
    int m_lineNumber;

    BaseTextEditorWidget *q;
    bool m_contentsChanged;
    bool m_lastCursorChangeWasInteresting;

    QList<QTextEdit::ExtraSelection> m_syntaxHighlighterSelections;
    QTextEdit::ExtraSelection m_lineSelection;

    QSharedPointer<BaseTextDocument> m_document;
    QByteArray m_tempState;
    QByteArray m_tempNavigationState;

    QString m_displayName;
    bool m_parenthesesMatchingEnabled;
    QTimer *m_updateTimer;

    Utils::ChangeSet m_changeSet;

    // parentheses matcher
    bool m_formatRange;
    QTextCharFormat m_matchFormat;
    QTextCharFormat m_mismatchFormat;
    QTimer *m_parenthesesMatchingTimer;
    // end parentheses matcher

    QWidget *m_extraArea;

    Core::Id m_tabSettingsId;
    ICodeStylePreferences *m_codeStylePreferences;
    DisplaySettings m_displaySettings;
    FontSettings m_fontSettings;
    BehaviorSettings m_behaviorSettings;

    int extraAreaSelectionAnchorBlockNumber;
    int extraAreaToggleMarkBlockNumber;
    int extraAreaHighlightFoldedBlockNumber;

    TextEditorOverlay *m_overlay;
    TextEditorOverlay *m_snippetOverlay;
    TextEditorOverlay *m_searchResultOverlay;
    bool snippetCheckCursor(const QTextCursor &cursor);
    void snippetTabOrBacktab(bool forward);
    QTextCharFormat m_occurrencesFormat;
    QTextCharFormat m_occurrenceRenameFormat;

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

    QTextCharFormat m_linkFormat;
    BaseTextEditorWidget::Link m_currentLink;
    bool m_linkPressed;

    QTextCharFormat m_ifdefedOutFormat;

    QRegExp m_searchExpr;
    Find::FindFlags m_findFlags;
    QTextCharFormat m_searchResultFormat;
    QTextCharFormat m_searchScopeFormat;
    QTextCharFormat m_currentLineFormat;
    QTextCharFormat m_currentLineNumberFormat;
    void highlightSearchResults(const QTextBlock &block, TextEditorOverlay *overlay);
    QTimer *m_delayedUpdateTimer;

    BaseTextEditor *m_editor;

    QObject *m_actionHack;

    QList<QTextEdit::ExtraSelection> m_extraSelections[BaseTextEditorWidget::NExtraSelectionKinds];

    // block selection mode
    bool m_inBlockSelectionMode;
    void clearBlockSelection();
    QString copyBlockSelection();
    void removeBlockSelection(const QString &text = QString());
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
    QTimer *m_highlightBlocksTimer;

    QScopedPointer<CodeAssistant> m_codeAssistant;
    bool m_assistRelevantContentAdded;

    QPointer<BaseTextEditorAnimator> m_animator;
    int m_cursorBlockNumber;

    QPoint m_markDragStart;
    bool m_markDragging;

    QScopedPointer<AutoCompleter> m_autoCompleter;
    QScopedPointer<Indenter> m_indenter;

    QScopedPointer<Internal::ClipboardAssistProvider> m_clipboardAssistProvider;
};

} // namespace Internal
} // namespace TextEditor

#endif // BASETEXTEDITOR_P_H
