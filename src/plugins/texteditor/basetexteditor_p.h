/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef BASETEXTEDITOR_P_H
#define BASETEXTEDITOR_P_H

#include "basetexteditor.h"
#include "behaviorsettings.h"
#include "displaysettings.h"
#include "fontsettings.h"
#include "refactoroverlay.h"

#include <utils/changeset.h>

#include <QBasicTimer>
#include <QSharedData>
#include <QPointer>
#include <QScopedPointer>

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

//========== Pointers with reference count ==========

template <class T> class QRefCountData : public QSharedData
{
public:
    QRefCountData(T *data) { m_data = data; }

    ~QRefCountData() { delete m_data; }

    T *m_data;
};

/* MOSTLY COPIED FROM QSHAREDDATA(-POINTER) */
template <class T> class QRefCountPointer
{
public:
    inline T &operator*() { return d ? *(d->m_data) : 0; }
    inline const T &operator*() const { return d ? *(d->m_data) : 0; }
    inline T *operator->() { return d ? d->m_data : 0; }
    inline const T *operator->() const { return d ? d->m_data : 0; }
    inline operator T *() { return d ? d->m_data : 0; }
    inline operator const T *() const { return d ? d->m_data : 0; }

    inline bool operator==(const QRefCountPointer<T> &other) const { return d == other.d; }
    inline bool operator!=(const QRefCountPointer<T> &other) const { return d != other.d; }

    inline QRefCountPointer() { d = 0; }
    inline ~QRefCountPointer() { if (d && !d->ref.deref()) delete d; }

    explicit QRefCountPointer(T *data) {
        if (data) {
            d = new QRefCountData<T>(data);
            d->ref.ref();
        }
        else {
            d = 0;
        }
    }
    inline QRefCountPointer(const QRefCountPointer<T> &o) : d(o.d) { if (d) d->ref.ref(); }
    inline QRefCountPointer<T> & operator=(const QRefCountPointer<T> &o) {
        if (o.d != d) {
            if (d && !d->ref.deref())
                delete d;
            //todo: atomic assign of pointers
            d = o.d;
            if (d)
                d->ref.ref();
        }
        return *this;
    }
    inline QRefCountPointer &operator=(T *o) {
        if (d == 0 || d->m_data != o) {
            if (d && !d->ref.deref())
                delete d;
            d = new QRefCountData<T>(o);
            if (d)
                d->ref.ref();
        }
        return *this;
    }

    inline bool operator!() const { return !d; }

private:
    QRefCountData<T> *d;
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
    void setupDocumentSignals(BaseTextDocument *document);
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

    QRefCountPointer<BaseTextDocument> m_document;
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

    QString m_tabSettingsId;
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

    QScopedPointer<AutoCompleter> m_autoCompleter;
    QScopedPointer<Indenter> m_indenter;

    QScopedPointer<Internal::ClipboardAssistProvider> m_clipboardAssistProvider;
};

} // namespace Internal
} // namespace TextEditor

#endif // BASETEXTEDITOR_P_H
