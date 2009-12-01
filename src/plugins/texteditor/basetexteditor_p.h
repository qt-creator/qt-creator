/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef BASETEXTEDITOR_P_H
#define BASETEXTEDITOR_P_H

#include "basetexteditor.h"
#include "texteditoroverlay.h"
#include <texteditor/fontsettings.h>
#include <utils/changeset.h>

#include <QtCore/QBasicTimer>
#include <QtCore/QSharedData>
#include <QtCore/QPointer>

#include <QtGui/QTextEdit>
#include <QtGui/QPixmap>

namespace TextEditor {

class BaseTextDocument;

namespace Internal {

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


class BaseTextEditorPrivate
{
    BaseTextEditorPrivate(const BaseTextEditorPrivate &);
    BaseTextEditorPrivate &operator=(const BaseTextEditorPrivate &);

public:
    BaseTextEditorPrivate();
    ~BaseTextEditorPrivate();

#ifndef TEXTEDITOR_STANDALONE
    void setupBasicEditActions(TextEditorActionHandler *actionHandler);
#endif
    void setupDocumentSignals(BaseTextDocument *document);
    void updateLineSelectionColor();

    void print(QPrinter *printer);

    QTextBlock m_firstVisible;
    int m_lastScrollPos;
    int m_lineNumber;

    BaseTextEditor *q;
    bool m_contentsChanged;
    bool m_lastCursorChangeWasInteresting;

    QList<QTextEdit::ExtraSelection> m_syntaxHighlighterSelections;
    QTextEdit::ExtraSelection m_lineSelection;

    QRefCountPointer<BaseTextDocument> m_document;
    QByteArray m_tempState;
    QByteArray m_tempNavigationState;

    QString m_displayName;
    bool m_parenthesesMatchingEnabled;
    bool m_autoParenthesesEnabled;
    QTimer *m_updateTimer;

    Utils::ChangeSet m_changeSet;

    // parentheses matcher
    bool m_formatRange;
    QTextCharFormat m_matchFormat;
    QTextCharFormat m_mismatchFormat;
    QTextCharFormat m_rangeFormat;
    QTimer *m_parenthesesMatchingTimer;
    // end parentheses matcher

    QWidget *m_extraArea;
    DisplaySettings m_displaySettings;
    TextEditor::FontSettings m_fontSettings;

    int extraAreaSelectionAnchorBlockNumber;
    int extraAreaToggleMarkBlockNumber;
    int extraAreaHighlightCollapseBlockNumber;
    int extraAreaHighlightCollapseColumn;

    TextEditorOverlay *m_overlay;
    TextEditorOverlay *m_searchResultOverlay;
    TextEditorOverlay *m_searchResultUnderlay;

    QBasicTimer collapsedBlockTimer;
    int visibleCollapsedBlockNumber;
    int suggestedVisibleCollapsedBlockNumber;
    void clearVisibleCollapsedBlock();
    bool m_mouseOnCollapsedMarker;

    QBasicTimer autoScrollTimer;
    void updateMarksLineNumber();
    void updateMarksBlock(const QTextBlock &block);
    uint m_marksVisible : 1;
    uint m_codeFoldingVisible : 1;
    uint m_codeFoldingSupported : 1;
    uint m_mouseNavigationEnabled : 1;
    uint m_revisionsVisible : 1;
    uint m_lineNumbersVisible : 1;
    uint m_highlightCurrentLine : 1;
    uint m_requestMarkEnabled : 1;
    uint m_lineSeparatorsAllowed : 1;
    int m_visibleWrapColumn;

    QTextCharFormat m_linkFormat;
    BaseTextEditor::Link m_currentLink;
    bool m_linkPressed;

    QTextCharFormat m_ifdefedOutFormat;

    QRegExp m_searchExpr;
    Find::IFindSupport::FindFlags m_findFlags;
    QTextCharFormat m_searchResultFormat;
    QTextCharFormat m_searchScopeFormat;
    QTextCharFormat m_currentLineFormat;
    QTextCharFormat m_currentLineNumberFormat;
    void highlightSearchResults(const QTextBlock &block,
                                TextEditorOverlay *overlay,
                                QVector<QTextLayout::FormatRange> *selections = 0);
    QTimer *m_delayedUpdateTimer;

    BaseTextEditorEditable *m_editable;

    QObject *m_actionHack;

    QList<QTextEdit::ExtraSelection> m_extraSelections[BaseTextEditor::NExtraSelectionKinds];

    // block selection mode
    bool m_inBlockSelectionMode;
    bool m_lastEventWasBlockSelectionEvent;
    int m_blockSelectionExtraX;
    void clearBlockSelection();
    QString copyBlockSelection();
    void removeBlockSelection(const QString &text = QString());
    bool m_moveLineUndoHack;

    QTextCursor m_findScope;
    QTextCursor m_selectBlockAnchor;

    void moveCursorVisible(bool ensureVisible = true);

    int visualIndent(const QTextBlock &block) const;
    BaseTextEditorPrivateHighlightBlocks m_highlightBlocksInfo;
    QTimer *m_highlightBlocksTimer;

    QPointer<BaseTextEditorAnimator> m_animator;
    int m_cursorBlockNumber;

};

} // namespace Internal
} // namespace TextEditor

#endif // BASETEXTEDITOR_P_H
