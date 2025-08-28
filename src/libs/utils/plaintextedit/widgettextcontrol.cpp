// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "widgettextcontrol.h"

#include "../utilstr.h"

#include <QAccessible>
#include <QApplication>
#include <QBasicTimer>
#include <QBuffer>
#include <QDesktopServices>
#include <QDrag>
#include <QGraphicsSceneEvent>
#include <QLineEdit>
#include <QMenu>
#include <QMetaMethod>
#include <QMimeData>
#include <QPainter>
#include <QPointer>
#include <QPrinter>
#include <QStyleHintReturnVariant>
#include <QStyleHints>
#include <QTextBlock>
#include <QTextDocumentFragment>
#include <QTextDocumentWriter>
#include <QTextList>
#include <QTextTable>
#include <QToolTip>

namespace Utils {

static QTextLine currentTextLine(const QTextCursor &cursor)
{
    const QTextBlock block = cursor.block();
    if (!block.isValid())
        return QTextLine();

    const QTextLayout *layout = block.layout();
    if (!layout)
        return QTextLine();

    const int relativePos = cursor.position() - block.position();
    return layout->lineForTextPosition(relativePos);
}

class UnicodeControlCharacterMenu : public QMenu
{
    Q_OBJECT
public:
    UnicodeControlCharacterMenu(QObject *editWidget, QWidget *parent);

private Q_SLOTS:
    void menuActionTriggered();

private:
    QObject *editWidget;
};

class TextEditMimeData : public QMimeData
{
public:
    inline TextEditMimeData(const QTextDocumentFragment &aFragment) : fragment(aFragment) {}

    virtual QStringList formats() const override;
    bool hasFormat(const QString &format) const override;

protected:
    virtual QVariant retrieveData(const QString &mimeType, QMetaType type) const override;
private:
    void setup() const;

    mutable QTextDocumentFragment fragment;
};

class WidgetTextControlPrivate : public QObject
{
public:
    WidgetTextControlPrivate(WidgetTextControl *q);

    bool cursorMoveKeyEvent(QKeyEvent *e);

    void updateCurrentCharFormat();

    void indent();
    void outdent();

    void gotoNextTableCell();
    void gotoPreviousTableCell();

    void createAutoBulletList();

    void init(Qt::TextFormat format = Qt::RichText, const QString &text = QString(),
              QTextDocument *document = nullptr);
    void setContent(Qt::TextFormat format = Qt::RichText, const QString &text = QString(),
                    QTextDocument *document = nullptr);
    void startDrag();

    void paste(const QMimeData *source);

    void setCursorPosition(const QPointF &pos);
    void setCursorPosition(int pos, QTextCursor::MoveMode mode = QTextCursor::MoveAnchor);

    void repaintCursor();
    inline void repaintSelection()
    { repaintOldAndNewSelection(QTextCursor()); }
    void repaintOldAndNewSelection(const QTextCursor &oldSelection);

    void selectionChanged(bool forceEmitSelectionChanged = false);

    void _q_updateCurrentCharFormatAndSelection();

#ifndef QT_NO_CLIPBOARD
    void setClipboardSelection();
#endif

    void _q_emitCursorPosChanged(const QTextCursor &someCursor);
    void _q_contentsChanged(int from, int charsRemoved, int charsAdded);

    void setCursorVisible(bool visible);
    void setBlinkingCursorEnabled(bool enable);
    void updateCursorBlinking();

    void extendWordwiseSelection(int suggestedNewPosition, qreal mouseXPosition);
    void extendBlockwiseSelection(int suggestedNewPosition);

    void _q_deleteSelected();

    void _q_setCursorAfterUndoRedo(int undoPosition, int charsAdded, int charsRemoved);

    QRectF cursorRectPlusUnicodeDirectionMarkers(const QTextCursor &cursor) const;
    QRectF rectForPosition(int position) const;
    QRectF selectionRect(const QTextCursor &cursor) const;
    inline QRectF selectionRect() const
    { return selectionRect(this->cursor); }

    QString anchorForCursor(const QTextCursor &anchor) const;

    void keyPressEvent(QKeyEvent *e);
    void mousePressEvent(QEvent *e, Qt::MouseButton button, const QPointF &pos,
                         Qt::KeyboardModifiers modifiers,
                         Qt::MouseButtons buttons,
                         const QPoint &globalPos);
    void mouseMoveEvent(QEvent *e, Qt::MouseButton button, const QPointF &pos,
                        Qt::KeyboardModifiers modifiers,
                        Qt::MouseButtons buttons,
                        const QPoint &globalPos);
    void mouseReleaseEvent(QEvent *e, Qt::MouseButton button, const QPointF &pos,
                           Qt::KeyboardModifiers modifiers,
                           Qt::MouseButtons buttons,
                           const QPoint &globalPos);
    void mouseDoubleClickEvent(QEvent *e, Qt::MouseButton button, const QPointF &pos,
                               Qt::KeyboardModifiers modifiers,
                               Qt::MouseButtons buttons,
                               const QPoint &globalPos);
    bool sendMouseEventToInputContext(QEvent *e,  QEvent::Type eventType, Qt::MouseButton button,
                                      const QPointF &pos,
                                      Qt::KeyboardModifiers modifiers,
                                      Qt::MouseButtons buttons,
                                      const QPoint &globalPos);
    void contextMenuEvent(const QPoint &screenPos, const QPointF &docPos, QWidget *contextWidget);
    void focusEvent(QFocusEvent *e);
#ifdef QT_KEYPAD_NAVIGATION
    void editFocusEvent(QEvent *e);
#endif
    bool dragEnterEvent(QEvent *e, const QMimeData *mimeData);
    void dragLeaveEvent();
    bool dragMoveEvent(QEvent *e, const QMimeData *mimeData, const QPointF &pos);
    bool dropEvent(const QMimeData *mimeData, const QPointF &pos, Qt::DropAction dropAction, QObject *source);

    void inputMethodEvent(QInputMethodEvent *);

    void activateLinkUnderCursor(QString href = QString());

#if QT_CONFIG(tooltip)
    void showToolTip(const QPoint &globalPos, const QPointF &pos, QWidget *contextWidget);
#endif

    bool isPreediting() const;
    void commitPreedit();

    void insertParagraphSeparator();
    void append(const QString &text, Qt::TextFormat format = Qt::AutoText);

    QTextDocument *doc;
    bool cursorOn;
    bool cursorVisible;
    QTextCursor cursor;
    bool cursorIsFocusIndicator;
    QTextCharFormat lastCharFormat;

    QTextCursor dndFeedbackCursor;

    Qt::TextInteractionFlags interactionFlags;

    QBasicTimer cursorBlinkTimer;
    QBasicTimer trippleClickTimer;
    QPointF trippleClickPoint;

    bool dragEnabled;

    bool mousePressed;

    bool mightStartDrag;
    QPoint mousePressPos;
    QPointer<QWidget> contextWidget;

    int lastSelectionPosition;
    int lastSelectionAnchor;

    bool ignoreAutomaticScrollbarAdjustement;

    QTextCursor selectedWordOnDoubleClick;
    QTextCursor selectedBlockOnTrippleClick;

    bool overwriteMode;
    bool acceptRichText;

    int preeditCursor;
    bool hideCursor; // used to hide the cursor in the preedit area

    QList<QAbstractTextDocumentLayout::Selection> extraSelections;

    QPalette palette;
    bool hasFocus;
#ifdef QT_KEYPAD_NAVIGATION
    bool hasEditFocus;
#endif
    bool isEnabled;

    QString highlightedAnchor; // Anchor below cursor
    QString anchorOnMousePress;
    QTextBlock blockWithMarkerUnderMouse;
    bool hadSelectionOnMousePress;

    bool ignoreUnusedNavigationEvents;
    bool openExternalLinks;

    bool wordSelectionEnabled;

    QString linkToCopy;
    void _q_copyLink();
    void _q_updateBlock(const QTextBlock &);
    void _q_documentLayoutChanged();

    WidgetTextControl *q = nullptr;
};

WidgetTextControlPrivate::WidgetTextControlPrivate(WidgetTextControl *q)
    : doc(nullptr), cursorOn(false), cursorVisible(false), cursorIsFocusIndicator(false),
#ifndef Q_OS_ANDROID
    interactionFlags(Qt::TextEditorInteraction),
#else
    interactionFlags(Qt::TextEditable | Qt::TextSelectableByKeyboard),
#endif
    dragEnabled(true),
#if QT_CONFIG(draganddrop)
    mousePressed(false), mightStartDrag(false),
#endif
    lastSelectionPosition(0), lastSelectionAnchor(0),
    ignoreAutomaticScrollbarAdjustement(false),
    overwriteMode(false),
    acceptRichText(true),
    preeditCursor(0), hideCursor(false),
    hasFocus(false),
#ifdef QT_KEYPAD_NAVIGATION
    hasEditFocus(false),
#endif
    isEnabled(true),
    hadSelectionOnMousePress(false),
    ignoreUnusedNavigationEvents(false),
    openExternalLinks(false),
    wordSelectionEnabled(false),
    q(q)
{}

bool WidgetTextControlPrivate::cursorMoveKeyEvent(QKeyEvent *e)
{
#ifdef QT_NO_SHORTCUT
    Q_UNUSED(e)
#endif

    if (cursor.isNull())
        return false;

    const QTextCursor oldSelection = cursor;
    const int oldCursorPos = cursor.position();

    QTextCursor::MoveMode mode = QTextCursor::MoveAnchor;
    QTextCursor::MoveOperation op = QTextCursor::NoMove;

    if (false) {
    }
#ifndef QT_NO_SHORTCUT
    if (e == QKeySequence::MoveToNextChar) {
        op = QTextCursor::Right;
    }
    else if (e == QKeySequence::MoveToPreviousChar) {
        op = QTextCursor::Left;
    }
    else if (e == QKeySequence::SelectNextChar) {
        op = QTextCursor::Right;
        mode = QTextCursor::KeepAnchor;
    }
    else if (e == QKeySequence::SelectPreviousChar) {
        op = QTextCursor::Left;
        mode = QTextCursor::KeepAnchor;
    }
    else if (e == QKeySequence::SelectNextWord) {
        op = QTextCursor::WordRight;
        mode = QTextCursor::KeepAnchor;
    }
    else if (e == QKeySequence::SelectPreviousWord) {
        op = QTextCursor::WordLeft;
        mode = QTextCursor::KeepAnchor;
    }
    else if (e == QKeySequence::SelectStartOfLine) {
        op = QTextCursor::StartOfLine;
        mode = QTextCursor::KeepAnchor;
    }
    else if (e == QKeySequence::SelectEndOfLine) {
        op = QTextCursor::EndOfLine;
        mode = QTextCursor::KeepAnchor;
    }
    else if (e == QKeySequence::SelectStartOfBlock) {
        op = QTextCursor::StartOfBlock;
        mode = QTextCursor::KeepAnchor;
    }
    else if (e == QKeySequence::SelectEndOfBlock) {
        op = QTextCursor::EndOfBlock;
        mode = QTextCursor::KeepAnchor;
    }
    else if (e == QKeySequence::SelectStartOfDocument) {
        op = QTextCursor::Start;
        mode = QTextCursor::KeepAnchor;
    }
    else if (e == QKeySequence::SelectEndOfDocument) {
        op = QTextCursor::End;
        mode = QTextCursor::KeepAnchor;
    }
    else if (e == QKeySequence::SelectPreviousLine) {
        op = QTextCursor::Up;
        mode = QTextCursor::KeepAnchor;
        {
            QTextBlock block = cursor.block();
            QTextLine line = currentTextLine(cursor);
            if (!block.previous().isValid()
                && line.isValid()
                && line.lineNumber() == 0)
                op = QTextCursor::Start;
        }
    }
    else if (e == QKeySequence::SelectNextLine) {
        op = QTextCursor::Down;
        mode = QTextCursor::KeepAnchor;
        {
            QTextBlock block = cursor.block();
            QTextLine line = currentTextLine(cursor);
            if (!block.next().isValid()
                && line.isValid()
                && line.lineNumber() == block.layout()->lineCount() - 1)
                op = QTextCursor::End;
        }
    }
    else if (e == QKeySequence::MoveToNextWord) {
        op = QTextCursor::WordRight;
    }
    else if (e == QKeySequence::MoveToPreviousWord) {
        op = QTextCursor::WordLeft;
    }
    else if (e == QKeySequence::MoveToEndOfBlock) {
        op = QTextCursor::EndOfBlock;
    }
    else if (e == QKeySequence::MoveToStartOfBlock) {
        op = QTextCursor::StartOfBlock;
    }
    else if (e == QKeySequence::MoveToNextLine) {
        op = QTextCursor::Down;
    }
    else if (e == QKeySequence::MoveToPreviousLine) {
        op = QTextCursor::Up;
    }
    else if (e == QKeySequence::MoveToStartOfLine) {
        op = QTextCursor::StartOfLine;
    }
    else if (e == QKeySequence::MoveToEndOfLine) {
        op = QTextCursor::EndOfLine;
    }
    else if (e == QKeySequence::MoveToStartOfDocument) {
        op = QTextCursor::Start;
    }
    else if (e == QKeySequence::MoveToEndOfDocument) {
        op = QTextCursor::End;
    }
#endif // QT_NO_SHORTCUT
    else {
        return false;
    }

    // Except for pageup and pagedown, macOSN has very different behavior, we don't do it all, but
    // here's the breakdown:
    // Shift still works as an anchor, but only one of the other keys can be down Ctrl (Command),
    // Alt (Option), or Meta (Control).
    // Command/Control + Left/Right -- Move to left or right of the line
    //                 + Up/Down -- Move to top bottom of the file. (Control doesn't move the cursor)
    // Option + Left/Right -- Move one word Left/right.
    //        + Up/Down  -- Begin/End of Paragraph.
    // Home/End Top/Bottom of file. (usually don't move the cursor, but will select)

    bool visualNavigation = cursor.visualNavigation();
    cursor.setVisualNavigation(true);
    const bool moved = cursor.movePosition(op, mode);
    cursor.setVisualNavigation(visualNavigation);
    q->ensureCursorVisible();

    bool ignoreNavigationEvents = ignoreUnusedNavigationEvents;
    bool isNavigationEvent = e->key() == Qt::Key_Up || e->key() == Qt::Key_Down;

#ifdef QT_KEYPAD_NAVIGATION
    ignoreNavigationEvents = ignoreNavigationEvents || QApplicationPrivate::keypadNavigationEnabled();
    isNavigationEvent = isNavigationEvent ||
                        (QApplication::navigationMode() == Qt::NavigationModeKeypadDirectional
                         && (e->key() == Qt::Key_Left || e->key() == Qt::Key_Right));
#else
    isNavigationEvent = isNavigationEvent || e->key() == Qt::Key_Left || e->key() == Qt::Key_Right;
#endif

    if (moved) {
        if (cursor.position() != oldCursorPos)
            emit q->cursorPositionChanged();
        emit q->microFocusChanged();
    } else if (ignoreNavigationEvents && isNavigationEvent && oldSelection.anchor() == cursor.anchor()) {
        return false;
    }

    selectionChanged(/*forceEmitSelectionChanged =*/(mode == QTextCursor::KeepAnchor));

    repaintOldAndNewSelection(oldSelection);

    return true;
}

void WidgetTextControlPrivate::updateCurrentCharFormat()
{

    QTextCharFormat fmt = cursor.charFormat();
    if (fmt == lastCharFormat)
        return;
    lastCharFormat = fmt;

    emit q->currentCharFormatChanged(fmt);
    emit q->microFocusChanged();
}

void WidgetTextControlPrivate::indent()
{
    QTextBlockFormat blockFmt = cursor.blockFormat();

    QTextList *list = cursor.currentList();
    if (!list) {
        QTextBlockFormat modifier;
        modifier.setIndent(blockFmt.indent() + 1);
        cursor.mergeBlockFormat(modifier);
    } else {
        QTextListFormat format = list->format();
        format.setIndent(format.indent() + 1);

        if (list->itemNumber(cursor.block()) == 1)
            list->setFormat(format);
        else
            cursor.createList(format);
    }
}

void WidgetTextControlPrivate::outdent()
{
    QTextBlockFormat blockFmt = cursor.blockFormat();

    QTextList *list = cursor.currentList();

    if (!list) {
        QTextBlockFormat modifier;
        modifier.setIndent(blockFmt.indent() - 1);
        cursor.mergeBlockFormat(modifier);
    } else {
        QTextListFormat listFmt = list->format();
        listFmt.setIndent(listFmt.indent() - 1);
        list->setFormat(listFmt);
    }
}

void WidgetTextControlPrivate::gotoNextTableCell()
{
    QTextTable *table = cursor.currentTable();
    QTextTableCell cell = table->cellAt(cursor);

    int newColumn = cell.column() + cell.columnSpan();
    int newRow = cell.row();

    if (newColumn >= table->columns()) {
        newColumn = 0;
        ++newRow;
        if (newRow >= table->rows())
            table->insertRows(table->rows(), 1);
    }

    cell = table->cellAt(newRow, newColumn);
    cursor = cell.firstCursorPosition();
}

void WidgetTextControlPrivate::gotoPreviousTableCell()
{
    QTextTable *table = cursor.currentTable();
    QTextTableCell cell = table->cellAt(cursor);

    int newColumn = cell.column() - 1;
    int newRow = cell.row();

    if (newColumn < 0) {
        newColumn = table->columns() - 1;
        --newRow;
        if (newRow < 0)
            return;
    }

    cell = table->cellAt(newRow, newColumn);
    cursor = cell.firstCursorPosition();
}

void WidgetTextControlPrivate::createAutoBulletList()
{
    cursor.beginEditBlock();

    QTextBlockFormat blockFmt = cursor.blockFormat();

    QTextListFormat listFmt;
    listFmt.setStyle(QTextListFormat::ListDisc);
    listFmt.setIndent(blockFmt.indent() + 1);

    blockFmt.setIndent(0);
    cursor.setBlockFormat(blockFmt);

    cursor.createList(listFmt);

    cursor.endEditBlock();
}

void WidgetTextControlPrivate::init(Qt::TextFormat format, const QString &text, QTextDocument *document)
{
    setContent(format, text, document);

    doc->setUndoRedoEnabled(interactionFlags & Qt::TextEditable);
    q->setCursorWidth(-1);
}

void WidgetTextControlPrivate::setContent(Qt::TextFormat format, const QString &text, QTextDocument *document)
{

    // for use when called from setPlainText. we may want to re-use the currently
    // set char format then.
    const QTextCharFormat charFormatForInsertion = cursor.charFormat();

    bool clearDocument = true;
    if (!doc) {
        if (document) {
            doc = document;
        } else {
            palette = QApplication::palette("WidgetTextControl");
            doc = new QTextDocument(q);
        }
        clearDocument = false;
        _q_documentLayoutChanged();
        cursor = QTextCursor(doc);

        // ####        doc->documentLayout()->setPaintDevice(viewport);

        QObject::connect(doc, &QTextDocument::contentsChanged, this,
                         &WidgetTextControlPrivate::_q_updateCurrentCharFormatAndSelection);
        QObject::connect(doc, &QTextDocument::cursorPositionChanged, this,
                         &WidgetTextControlPrivate::_q_emitCursorPosChanged);
        QObject::connect(doc, &QTextDocument::documentLayoutChanged, this,
                         &WidgetTextControlPrivate::_q_documentLayoutChanged);

        // convenience signal forwards
        QObject::connect(doc, &QTextDocument::undoAvailable, q, &WidgetTextControl::undoAvailable);
        QObject::connect(doc, &QTextDocument::redoAvailable, q, &WidgetTextControl::redoAvailable);
        QObject::connect(doc, &QTextDocument::modificationChanged, q,
                         &WidgetTextControl::modificationChanged);
        QObject::connect(doc, &QTextDocument::blockCountChanged, q,
                         &WidgetTextControl::blockCountChanged);
    }

    bool previousUndoRedoState = doc->isUndoRedoEnabled();
    if (!document)
        doc->setUndoRedoEnabled(false);

    //Saving the index save some time.
    static int contentsChangedIndex = QMetaMethod::fromSignal(&QTextDocument::contentsChanged).methodIndex();
    static int textChangedIndex = QMetaMethod::fromSignal(&WidgetTextControl::textChanged).methodIndex();
    // avoid multiple textChanged() signals being emitted
    QMetaObject::disconnect(doc, contentsChangedIndex, q, textChangedIndex);

    if (!text.isEmpty()) {
        // clear 'our' cursor for insertion to prevent
        // the emission of the cursorPositionChanged() signal.
        // instead we emit it only once at the end instead of
        // at the end of the document after loading and when
        // positioning the cursor again to the start of the
        // document.
        cursor = QTextCursor();
        if (format == Qt::PlainText) {
            QTextCursor formatCursor(doc);
            // put the setPlainText and the setCharFormat into one edit block,
            // so that the syntax highlight triggers only /once/ for the entire
            // document, not twice.
            formatCursor.beginEditBlock();
            doc->setPlainText(text);
            doc->setUndoRedoEnabled(false);
            formatCursor.select(QTextCursor::Document);
            formatCursor.setCharFormat(charFormatForInsertion);
            formatCursor.endEditBlock();
#if QT_CONFIG(textmarkdownreader)
        } else if (format == Qt::MarkdownText) {
            doc->setMarkdown(text);
            doc->setUndoRedoEnabled(false);
#endif
        } else {
#ifndef QT_NO_TEXTHTMLPARSER
            doc->setHtml(text);
#else
            doc->setPlainText(text);
#endif
            doc->setUndoRedoEnabled(false);
        }
        cursor = QTextCursor(doc);
    } else if (clearDocument) {
        doc->clear();
    }
    cursor.setCharFormat(charFormatForInsertion);

    QMetaObject::connect(doc, contentsChangedIndex, q, textChangedIndex);
    emit q->textChanged();
    if (!document)
        doc->setUndoRedoEnabled(previousUndoRedoState);
    _q_updateCurrentCharFormatAndSelection();
    if (!document)
        doc->setModified(false);

    q->ensureCursorVisible();
    emit q->cursorPositionChanged();

    QObject::connect(doc, &QTextDocument::contentsChange, this,
                     &WidgetTextControlPrivate::_q_contentsChanged, Qt::UniqueConnection);
}

void WidgetTextControlPrivate::startDrag()
{
#if QT_CONFIG(draganddrop)
    mousePressed = false;
    if (!contextWidget)
        return;
    QMimeData *data = q->createMimeDataFromSelection();

    QDrag *drag = new QDrag(contextWidget);
    drag->setMimeData(data);

    Qt::DropActions actions = Qt::CopyAction;
    Qt::DropAction action;
    if (interactionFlags & Qt::TextEditable) {
        actions |= Qt::MoveAction;
        action = drag->exec(actions, Qt::MoveAction);
    } else {
        action = drag->exec(actions, Qt::CopyAction);
    }

    if (action == Qt::MoveAction && drag->target() != contextWidget)
        cursor.removeSelectedText();
#endif
}

void WidgetTextControlPrivate::setCursorPosition(const QPointF &pos)
{
    const int cursorPos = q->hitTest(pos, Qt::FuzzyHit);
    if (cursorPos == -1)
        return;
    cursor.setPosition(cursorPos);
}

void WidgetTextControlPrivate::setCursorPosition(int pos, QTextCursor::MoveMode mode)
{
    cursor.setPosition(pos, mode);

    if (mode != QTextCursor::KeepAnchor) {
        selectedWordOnDoubleClick = QTextCursor();
        selectedBlockOnTrippleClick = QTextCursor();
    }
}

void WidgetTextControlPrivate::repaintCursor()
{
    emit q->updateRequest(cursorRectPlusUnicodeDirectionMarkers(cursor));
}

void WidgetTextControlPrivate::repaintOldAndNewSelection(const QTextCursor &oldSelection)
{
    if (cursor.hasSelection()
        && oldSelection.hasSelection()
        && cursor.currentFrame() == oldSelection.currentFrame()
        && !cursor.hasComplexSelection()
        && !oldSelection.hasComplexSelection()
        && cursor.anchor() == oldSelection.anchor()
        ) {
        QTextCursor differenceSelection(doc);
        differenceSelection.setPosition(oldSelection.position());
        differenceSelection.setPosition(cursor.position(), QTextCursor::KeepAnchor);
        emit q->updateRequest(q->selectionRect(differenceSelection));
    } else {
        if (!oldSelection.isNull())
            emit q->updateRequest(q->selectionRect(oldSelection) | cursorRectPlusUnicodeDirectionMarkers(oldSelection));
        emit q->updateRequest(q->selectionRect() | cursorRectPlusUnicodeDirectionMarkers(cursor));
    }
}

void WidgetTextControlPrivate::selectionChanged(bool forceEmitSelectionChanged /*=false*/)
{
    if (forceEmitSelectionChanged) {
        emit q->selectionChanged();
#if QT_CONFIG(accessibility)
        if (q->parent() && q->parent()->isWidgetType()) {
            QAccessibleTextSelectionEvent ev(q->parent(), cursor.anchor(), cursor.position());
            QAccessible::updateAccessibility(&ev);
        }
#endif
    }

    if (cursor.position() == lastSelectionPosition
        && cursor.anchor() == lastSelectionAnchor)
        return;

    bool selectionStateChange = (cursor.hasSelection()
                                 != (lastSelectionPosition != lastSelectionAnchor));
    if (selectionStateChange)
        emit q->copyAvailable(cursor.hasSelection());

    if (!forceEmitSelectionChanged
        && (selectionStateChange
            || (cursor.hasSelection()
                && (cursor.position() != lastSelectionPosition
                    || cursor.anchor() != lastSelectionAnchor)))) {
        emit q->selectionChanged();
#if QT_CONFIG(accessibility)
        if (q->parent() && q->parent()->isWidgetType()) {
            QAccessibleTextSelectionEvent ev(q->parent(), cursor.anchor(), cursor.position());
            QAccessible::updateAccessibility(&ev);
        }
#endif
    }
    emit q->microFocusChanged();
    lastSelectionPosition = cursor.position();
    lastSelectionAnchor = cursor.anchor();
}

void WidgetTextControlPrivate::_q_updateCurrentCharFormatAndSelection()
{
    updateCurrentCharFormat();
    selectionChanged();
}

#ifndef QT_NO_CLIPBOARD
void WidgetTextControlPrivate::setClipboardSelection()
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    if (!cursor.hasSelection() || !clipboard->supportsSelection())
        return;
    QMimeData *data = q->createMimeDataFromSelection();
    clipboard->setMimeData(data, QClipboard::Selection);
}
#endif

void WidgetTextControlPrivate::_q_emitCursorPosChanged(const QTextCursor &someCursor)
{
    if (someCursor.isCopyOf(cursor)) {
        emit q->cursorPositionChanged();
        emit q->microFocusChanged();
    }
}

void WidgetTextControlPrivate::_q_contentsChanged(int from, int charsRemoved, int charsAdded)
{
#if QT_CONFIG(accessibility)

    if (QAccessible::isActive() && q->parent() && q->parent()->isWidgetType()) {
        QTextCursor tmp(doc);
        tmp.setPosition(from);
        // when setting a new text document the length is off
        // QTBUG-32583 - characterCount is off by 1 requires the -1
        tmp.setPosition(qMin(doc->characterCount() - 1, from + charsAdded), QTextCursor::KeepAnchor);
        QString newText = tmp.selectedText();

        // always report the right number of removed chars, but in lack of the real string use spaces
        QString oldText = QString(charsRemoved, u' ');

        QAccessibleEvent *ev = nullptr;
        if (charsRemoved == 0) {
            ev = new QAccessibleTextInsertEvent(q->parent(), from, newText);
        } else if (charsAdded == 0) {
            ev = new QAccessibleTextRemoveEvent(q->parent(), from, oldText);
        } else {
            ev = new QAccessibleTextUpdateEvent(q->parent(), from, oldText, newText);
        }
        QAccessible::updateAccessibility(ev);
        delete ev;
    }
#else
    Q_UNUSED(from)
    Q_UNUSED(charsRemoved)
    Q_UNUSED(charsAdded)
#endif
}

void WidgetTextControlPrivate::_q_documentLayoutChanged()
{
    QAbstractTextDocumentLayout *layout = doc->documentLayout();
    QObject::connect(layout, &QAbstractTextDocumentLayout::update, q,
                     &WidgetTextControl::updateRequest);
    QObject::connect(layout, &QAbstractTextDocumentLayout::updateBlock, this,
                     &WidgetTextControlPrivate::_q_updateBlock);
    QObject::connect(layout, &QAbstractTextDocumentLayout::documentSizeChanged, q,
                     &WidgetTextControl::documentSizeChanged);
}

void WidgetTextControlPrivate::setCursorVisible(bool visible)
{
    if (cursorVisible == visible)
        return;

    cursorVisible = visible;
    updateCursorBlinking();

    if (cursorVisible)
        connect(QGuiApplication::styleHints(), &QStyleHints::cursorFlashTimeChanged, this, &WidgetTextControlPrivate::updateCursorBlinking);
    else
        disconnect(QGuiApplication::styleHints(), &QStyleHints::cursorFlashTimeChanged, this, &WidgetTextControlPrivate::updateCursorBlinking);
}

void WidgetTextControlPrivate::updateCursorBlinking()
{
    cursorBlinkTimer.stop();
    if (cursorVisible) {
        int flashTime = QGuiApplication::styleHints()->cursorFlashTime();
        if (flashTime >= 2)
            cursorBlinkTimer.start(flashTime / 2, q);
    }

    cursorOn = cursorVisible;
    repaintCursor();
}

void WidgetTextControlPrivate::extendWordwiseSelection(int suggestedNewPosition, qreal mouseXPosition)
{

    // if inside the initial selected word keep that
    if (suggestedNewPosition >= selectedWordOnDoubleClick.selectionStart()
        && suggestedNewPosition <= selectedWordOnDoubleClick.selectionEnd()) {
        q->setTextCursor(selectedWordOnDoubleClick);
        return;
    }

    QTextCursor curs = selectedWordOnDoubleClick;
    curs.setPosition(suggestedNewPosition, QTextCursor::KeepAnchor);

    if (!curs.movePosition(QTextCursor::StartOfWord))
        return;
    const int wordStartPos = curs.position();

    const int blockPos = curs.block().position();
    const QPointF blockCoordinates = q->blockBoundingRect(curs.block()).topLeft();

    QTextLine line = currentTextLine(curs);
    if (!line.isValid())
        return;

    const qreal wordStartX = line.cursorToX(curs.position() - blockPos) + blockCoordinates.x();

    if (!curs.movePosition(QTextCursor::EndOfWord))
        return;
    const int wordEndPos = curs.position();

    const QTextLine otherLine = currentTextLine(curs);
    if (otherLine.textStart() != line.textStart()
        || wordEndPos == wordStartPos)
        return;

    const qreal wordEndX = line.cursorToX(curs.position() - blockPos) + blockCoordinates.x();

    if (!wordSelectionEnabled && (mouseXPosition < wordStartX || mouseXPosition > wordEndX))
        return;

    if (wordSelectionEnabled) {
        if (suggestedNewPosition < selectedWordOnDoubleClick.position()) {
            cursor.setPosition(selectedWordOnDoubleClick.selectionEnd());
            setCursorPosition(wordStartPos, QTextCursor::KeepAnchor);
        } else {
            cursor.setPosition(selectedWordOnDoubleClick.selectionStart());
            setCursorPosition(wordEndPos, QTextCursor::KeepAnchor);
        }
    } else {
        // keep the already selected word even when moving to the left
        // (#39164)
        if (suggestedNewPosition < selectedWordOnDoubleClick.position())
            cursor.setPosition(selectedWordOnDoubleClick.selectionEnd());
        else
            cursor.setPosition(selectedWordOnDoubleClick.selectionStart());

        const qreal differenceToStart = mouseXPosition - wordStartX;
        const qreal differenceToEnd = wordEndX - mouseXPosition;

        if (differenceToStart < differenceToEnd)
            setCursorPosition(wordStartPos, QTextCursor::KeepAnchor);
        else
            setCursorPosition(wordEndPos, QTextCursor::KeepAnchor);
    }

    if (interactionFlags & Qt::TextSelectableByMouse) {
#ifndef QT_NO_CLIPBOARD
        setClipboardSelection();
#endif
        selectionChanged(true);
    }
}

void WidgetTextControlPrivate::extendBlockwiseSelection(int suggestedNewPosition)
{

    // if inside the initial selected line keep that
    if (suggestedNewPosition >= selectedBlockOnTrippleClick.selectionStart()
        && suggestedNewPosition <= selectedBlockOnTrippleClick.selectionEnd()) {
        q->setTextCursor(selectedBlockOnTrippleClick);
        return;
    }

    if (suggestedNewPosition < selectedBlockOnTrippleClick.position()) {
        cursor.setPosition(selectedBlockOnTrippleClick.selectionEnd());
        cursor.setPosition(suggestedNewPosition, QTextCursor::KeepAnchor);
        cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
    } else {
        cursor.setPosition(selectedBlockOnTrippleClick.selectionStart());
        cursor.setPosition(suggestedNewPosition, QTextCursor::KeepAnchor);
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
    }

    if (interactionFlags & Qt::TextSelectableByMouse) {
#ifndef QT_NO_CLIPBOARD
        setClipboardSelection();
#endif
        selectionChanged(true);
    }
}

void WidgetTextControlPrivate::_q_deleteSelected()
{
    if (!(interactionFlags & Qt::TextEditable) || !cursor.hasSelection())
        return;
    cursor.removeSelectedText();
}

void WidgetTextControl::undo()
{
    d->repaintSelection();
    const int oldCursorPos = d->cursor.position();
    d->doc->undo(&d->cursor);
    if (d->cursor.position() != oldCursorPos)
        emit cursorPositionChanged();
    emit microFocusChanged();
    ensureCursorVisible();
}

void WidgetTextControl::redo()
{
    d->repaintSelection();
    const int oldCursorPos = d->cursor.position();
    d->doc->redo(&d->cursor);
    if (d->cursor.position() != oldCursorPos)
        emit cursorPositionChanged();
    emit microFocusChanged();
    ensureCursorVisible();
}

WidgetTextControl::WidgetTextControl(QObject *parent)
    : InputControl(InputControl::TextEdit, parent)
    , d(new WidgetTextControlPrivate(this))
{
    d->init();
}

WidgetTextControl::WidgetTextControl(const QString &text, QObject *parent)
    : InputControl(InputControl::TextEdit, parent)
    , d(new WidgetTextControlPrivate(this))
{
    d->init(Qt::RichText, text);
}

WidgetTextControl::WidgetTextControl(QTextDocument *doc, QObject *parent)
    : InputControl(InputControl::TextEdit, parent)
    , d(new WidgetTextControlPrivate(this))
{
    d->init(Qt::RichText, QString(), doc);
}

WidgetTextControl::~WidgetTextControl()
{
}

void WidgetTextControl::setDocument(QTextDocument *document)
{
    if (d->doc == document)
        return;

    d->doc->disconnect(this);
    d->doc->documentLayout()->disconnect(this);
    d->doc->documentLayout()->setPaintDevice(nullptr);

    if (d->doc->parent() == this)
        delete d->doc;

    d->doc = nullptr;
    d->setContent(Qt::RichText, QString(), document);
}

QTextDocument *WidgetTextControl::document() const
{
    return d->doc;
}

void WidgetTextControl::setTextCursor(const QTextCursor &cursor, bool selectionClipboard)
{
    d->cursorIsFocusIndicator = false;
    const bool posChanged = cursor.position() != d->cursor.position();
    const QTextCursor oldSelection = d->cursor;
    d->cursor = cursor;
    d->cursorOn = d->hasFocus
                  && (d->interactionFlags & (Qt::TextSelectableByKeyboard | Qt::TextEditable));
    d->_q_updateCurrentCharFormatAndSelection();
    ensureCursorVisible();
    d->repaintOldAndNewSelection(oldSelection);
    if (posChanged)
        emit cursorPositionChanged();

#ifndef QT_NO_CLIPBOARD
    if (selectionClipboard)
        d->setClipboardSelection();
#else
    Q_UNUSED(selectionClipboard)
#endif
}

QTextCursor WidgetTextControl::textCursor() const
{
    return d->cursor;
}

#ifndef QT_NO_CLIPBOARD

void WidgetTextControl::cut()
{
    if (!(d->interactionFlags & Qt::TextEditable) || !d->cursor.hasSelection())
        return;
    copy();
    d->cursor.removeSelectedText();
}

void WidgetTextControl::copy()
{
    if (!d->cursor.hasSelection())
        return;
    QMimeData *data = createMimeDataFromSelection();
    QGuiApplication::clipboard()->setMimeData(data);
}

void WidgetTextControl::paste(QClipboard::Mode mode)
{
    const QMimeData *md = QGuiApplication::clipboard()->mimeData(mode);
    if (md)
        insertFromMimeData(md);
}
#endif

void WidgetTextControl::clear()
{
    // clears and sets empty content
    d->extraSelections.clear();
    d->setContent();
}


void WidgetTextControl::selectAll()
{
    const int selectionLength = qAbs(d->cursor.position() - d->cursor.anchor());
    const int oldCursorPos = d->cursor.position();
    d->cursor.select(QTextCursor::Document);
    d->selectionChanged(selectionLength != qAbs(d->cursor.position() - d->cursor.anchor()));
    d->cursorIsFocusIndicator = false;
    if (d->cursor.position() != oldCursorPos)
        emit cursorPositionChanged();
    emit updateRequest();
}

void WidgetTextControl::processEvent(QEvent *e, const QPointF &coordinateOffset, QWidget *contextWidget)
{
    QTransform t;
    t.translate(coordinateOffset.x(), coordinateOffset.y());
    processEvent(e, t, contextWidget);
}

void WidgetTextControl::processEvent(QEvent *e, const QTransform &transform, QWidget *contextWidget)
{
    if (d->interactionFlags == Qt::NoTextInteraction) {
        e->ignore();
        return;
    }

    d->contextWidget = contextWidget;

    if (!d->contextWidget) {
        switch (e->type()) {
#if QT_CONFIG(graphicsview)
        case QEvent::GraphicsSceneMouseMove:
        case QEvent::GraphicsSceneMousePress:
        case QEvent::GraphicsSceneMouseRelease:
        case QEvent::GraphicsSceneMouseDoubleClick:
        case QEvent::GraphicsSceneContextMenu:
        case QEvent::GraphicsSceneHoverEnter:
        case QEvent::GraphicsSceneHoverMove:
        case QEvent::GraphicsSceneHoverLeave:
        case QEvent::GraphicsSceneHelp:
        case QEvent::GraphicsSceneDragEnter:
        case QEvent::GraphicsSceneDragMove:
        case QEvent::GraphicsSceneDragLeave:
        case QEvent::GraphicsSceneDrop: {
            QGraphicsSceneEvent *ev = static_cast<QGraphicsSceneEvent *>(e);
            d->contextWidget = ev->widget();
            break;
        }
#endif // QT_CONFIG(graphicsview)
        default: break;
        };
    }

    switch (e->type()) {
    case QEvent::KeyPress:
        d->keyPressEvent(static_cast<QKeyEvent *>(e));
        break;
    case QEvent::MouseButtonPress: {
        QMouseEvent *ev = static_cast<QMouseEvent *>(e);
        d->mousePressEvent(ev, ev->button(), transform.map(ev->position().toPoint()), ev->modifiers(),
                           ev->buttons(), ev->globalPosition().toPoint());
        break; }
    case QEvent::MouseMove: {
        QMouseEvent *ev = static_cast<QMouseEvent *>(e);
        d->mouseMoveEvent(ev, ev->button(), transform.map(ev->position().toPoint()), ev->modifiers(),
                          ev->buttons(), ev->globalPosition().toPoint());
        break; }
    case QEvent::MouseButtonRelease: {
        QMouseEvent *ev = static_cast<QMouseEvent *>(e);
        d->mouseReleaseEvent(ev, ev->button(), transform.map(ev->position().toPoint()), ev->modifiers(),
                             ev->buttons(), ev->globalPosition().toPoint());
        break; }
    case QEvent::MouseButtonDblClick: {
        QMouseEvent *ev = static_cast<QMouseEvent *>(e);
        d->mouseDoubleClickEvent(ev, ev->button(), transform.map(ev->position().toPoint()), ev->modifiers(),
                                 ev->buttons(), ev->globalPosition().toPoint());
        break; }
    case QEvent::InputMethod:
        d->inputMethodEvent(static_cast<QInputMethodEvent *>(e));
        break;
#ifndef QT_NO_CONTEXTMENU
    case QEvent::ContextMenu: {
        QContextMenuEvent *ev = static_cast<QContextMenuEvent *>(e);
        d->contextMenuEvent(ev->globalPos(), transform.map(ev->pos()), contextWidget);
        break; }
#endif // QT_NO_CONTEXTMENU
    case QEvent::FocusIn:
    case QEvent::FocusOut:
        d->focusEvent(static_cast<QFocusEvent *>(e));
        break;

    case QEvent::EnabledChange:
        d->isEnabled = e->isAccepted();
        break;

#if QT_CONFIG(tooltip)
    case QEvent::ToolTip: {
        QHelpEvent *ev = static_cast<QHelpEvent *>(e);
        d->showToolTip(ev->globalPos(), transform.map(ev->pos()), contextWidget);
        break;
    }
#endif // QT_CONFIG(tooltip)

#if QT_CONFIG(draganddrop)
    case QEvent::DragEnter: {
        QDragEnterEvent *ev = static_cast<QDragEnterEvent *>(e);
        if (d->dragEnterEvent(e, ev->mimeData()))
            ev->acceptProposedAction();
        break;
    }
    case QEvent::DragLeave:
        d->dragLeaveEvent();
        break;
    case QEvent::DragMove: {
        QDragMoveEvent *ev = static_cast<QDragMoveEvent *>(e);
        if (d->dragMoveEvent(e, ev->mimeData(), transform.map(ev->position().toPoint())))
            ev->acceptProposedAction();
        break;
    }
    case QEvent::Drop: {
        QDropEvent *ev = static_cast<QDropEvent *>(e);
        if (d->dropEvent(ev->mimeData(), transform.map(ev->position().toPoint()), ev->dropAction(), ev->source()))
            ev->acceptProposedAction();
        break;
    }
#endif

#if QT_CONFIG(graphicsview)
    case QEvent::GraphicsSceneMousePress: {
        QGraphicsSceneMouseEvent *ev = static_cast<QGraphicsSceneMouseEvent *>(e);
        d->mousePressEvent(ev, ev->button(), transform.map(ev->pos()), ev->modifiers(), ev->buttons(),
                           ev->screenPos());
        break; }
    case QEvent::GraphicsSceneMouseMove: {
        QGraphicsSceneMouseEvent *ev = static_cast<QGraphicsSceneMouseEvent *>(e);
        d->mouseMoveEvent(ev, ev->button(), transform.map(ev->pos()), ev->modifiers(), ev->buttons(),
                          ev->screenPos());
        break; }
    case QEvent::GraphicsSceneMouseRelease: {
        QGraphicsSceneMouseEvent *ev = static_cast<QGraphicsSceneMouseEvent *>(e);
        d->mouseReleaseEvent(ev, ev->button(), transform.map(ev->pos()), ev->modifiers(), ev->buttons(),
                             ev->screenPos());
        break; }
    case QEvent::GraphicsSceneMouseDoubleClick: {
        QGraphicsSceneMouseEvent *ev = static_cast<QGraphicsSceneMouseEvent *>(e);
        d->mouseDoubleClickEvent(ev, ev->button(), transform.map(ev->pos()), ev->modifiers(), ev->buttons(),
                                 ev->screenPos());
        break; }
    case QEvent::GraphicsSceneContextMenu: {
        QGraphicsSceneContextMenuEvent *ev = static_cast<QGraphicsSceneContextMenuEvent *>(e);
        d->contextMenuEvent(ev->screenPos(), transform.map(ev->pos()), contextWidget);
        break; }

    case QEvent::GraphicsSceneHoverMove: {
        QGraphicsSceneHoverEvent *ev = static_cast<QGraphicsSceneHoverEvent *>(e);
        d->mouseMoveEvent(ev, Qt::NoButton, transform.map(ev->pos()), ev->modifiers(),Qt::NoButton,
                          ev->screenPos());
        break; }

    case QEvent::GraphicsSceneDragEnter: {
        QGraphicsSceneDragDropEvent *ev = static_cast<QGraphicsSceneDragDropEvent *>(e);
        if (d->dragEnterEvent(e, ev->mimeData()))
            ev->acceptProposedAction();
        break; }
    case QEvent::GraphicsSceneDragLeave:
        d->dragLeaveEvent();
        break;
    case QEvent::GraphicsSceneDragMove: {
        QGraphicsSceneDragDropEvent *ev = static_cast<QGraphicsSceneDragDropEvent *>(e);
        if (d->dragMoveEvent(e, ev->mimeData(), transform.map(ev->pos())))
            ev->acceptProposedAction();
        break; }
    case QEvent::GraphicsSceneDrop: {
        QGraphicsSceneDragDropEvent *ev = static_cast<QGraphicsSceneDragDropEvent *>(e);
        if (d->dropEvent(ev->mimeData(), transform.map(ev->pos()), ev->dropAction(), ev->source()))
            ev->accept();
        break; }
#endif // QT_CONFIG(graphicsview)
#ifdef QT_KEYPAD_NAVIGATION
    case QEvent::EnterEditFocus:
    case QEvent::LeaveEditFocus:
        if (QApplicationPrivate::keypadNavigationEnabled())
            d->editFocusEvent(e);
        break;
#endif
    case QEvent::ShortcutOverride:
        if (d->interactionFlags & Qt::TextEditable) {
            QKeyEvent* ke = static_cast<QKeyEvent *>(e);
            if (isCommonTextEditShortcut(ke))
                ke->accept();
        }
        break;
    default:
        break;
    }
}

bool WidgetTextControl::event(QEvent *e)
{
    return QObject::event(e);
}

void WidgetTextControl::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == d->cursorBlinkTimer.timerId()) {
        d->cursorOn = !d->cursorOn;

        if (d->cursor.hasSelection())
            d->cursorOn &= (QApplication::style()->styleHint(QStyle::SH_BlinkCursorWhenTextSelected)
                            != 0);

        d->repaintCursor();
    } else if (e->timerId() == d->trippleClickTimer.timerId()) {
        d->trippleClickTimer.stop();
    }
}

void WidgetTextControl::setPlainText(const QString &text)
{
    d->setContent(Qt::PlainText, text);
}

#if QT_CONFIG(textmarkdownreader)
void WidgetTextControl::setMarkdown(const QString &text)
{
    d->setContent(Qt::MarkdownText, text);
}
#endif

void WidgetTextControl::setHtml(const QString &text)
{
    d->setContent(Qt::RichText, text);
}

void WidgetTextControlPrivate::keyPressEvent(QKeyEvent *e)
{
#ifndef QT_NO_SHORTCUT
    if (e == QKeySequence::SelectAll) {
        e->accept();
        q->selectAll();
#ifndef QT_NO_CLIPBOARD
        setClipboardSelection();
#endif
        return;
    }
#ifndef QT_NO_CLIPBOARD
    else if (e == QKeySequence::Copy) {
        e->accept();
        q->copy();
        return;
    }
#endif
#endif // QT_NO_SHORTCUT

    if (interactionFlags & Qt::TextSelectableByKeyboard
        && cursorMoveKeyEvent(e))
        goto accept;

    if (interactionFlags & Qt::LinksAccessibleByKeyboard) {
        if ((e->key() == Qt::Key_Return
             || e->key() == Qt::Key_Enter
#ifdef QT_KEYPAD_NAVIGATION
             || e->key() == Qt::Key_Select
#endif
             )
            && cursor.hasSelection()) {

            e->accept();
            activateLinkUnderCursor();
            return;
        }
    }

    if (!(interactionFlags & Qt::TextEditable)) {
        e->ignore();
        return;
    }

    if (e->key() == Qt::Key_Direction_L || e->key() == Qt::Key_Direction_R) {
        QTextBlockFormat fmt;
        fmt.setLayoutDirection((e->key() == Qt::Key_Direction_L) ? Qt::LeftToRight : Qt::RightToLeft);
        cursor.mergeBlockFormat(fmt);
        goto accept;
    }

    // schedule a repaint of the region of the cursor, as when we move it we
    // want to make sure the old cursor disappears (not noticeable when moving
    // only a few pixels but noticeable when jumping between cells in tables for
    // example)
    repaintSelection();

    if (e->key() == Qt::Key_Backspace && !(e->modifiers() & ~(Qt::ShiftModifier | Qt::GroupSwitchModifier))) {
        QTextBlockFormat blockFmt = cursor.blockFormat();
        QTextList *list = cursor.currentList();
        if (list && cursor.atBlockStart() && !cursor.hasSelection()) {
            list->remove(cursor.block());
        } else if (cursor.atBlockStart() && blockFmt.indent() > 0) {
            blockFmt.setIndent(blockFmt.indent() - 1);
            cursor.setBlockFormat(blockFmt);
        } else {
            cursor.deletePreviousChar();
            // QTextCursor localCursor = cursor;
            // localCursor.deletePreviousChar();
            // if (cursor.d)
            //     cursor.d->setX();
        }
        goto accept;
    }
#ifndef QT_NO_SHORTCUT
    else if (e == QKeySequence::InsertParagraphSeparator) {
        insertParagraphSeparator();
        e->accept();
        goto accept;
    } else if (e == QKeySequence::InsertLineSeparator) {
        cursor.insertText(QString(QChar::LineSeparator));
        e->accept();
        goto accept;
    }
#endif
    if (false) {
    }
#ifndef QT_NO_SHORTCUT
    else if (e == QKeySequence::Undo) {
        q->undo();
    }
    else if (e == QKeySequence::Redo) {
        q->redo();
    }
#ifndef QT_NO_CLIPBOARD
    else if (e == QKeySequence::Cut) {
        q->cut();
    }
    else if (e == QKeySequence::Paste) {
        QClipboard::Mode mode = QClipboard::Clipboard;
        if (QGuiApplication::clipboard()->supportsSelection()) {
            if (e->modifiers() == (Qt::CTRL | Qt::SHIFT) && e->key() == Qt::Key_Insert)
                mode = QClipboard::Selection;
        }
        q->paste(mode);
    }
#endif
    else if (e == QKeySequence::Delete) {
        cursor.deleteChar();
        // QTextCursor localCursor = cursor;
        // localCursor.deleteChar();
        // if (cursor.d)
        //     cursor.d->setX();
    } else if (e == QKeySequence::Backspace) {
        cursor.deletePreviousChar();
        // QTextCursor localCursor = cursor;
        // localCursor.deletePreviousChar();
        // if (cursor.d)
        //     cursor.d->setX();
    }else if (e == QKeySequence::DeleteEndOfWord) {
        if (!cursor.hasSelection())
            cursor.movePosition(QTextCursor::NextWord, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
    }
    else if (e == QKeySequence::DeleteStartOfWord) {
        if (!cursor.hasSelection())
            cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
    }
    else if (e == QKeySequence::DeleteEndOfLine) {
        QTextBlock block = cursor.block();
        if (cursor.position() == block.position() + block.length() - 2)
            cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
        else
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
    }
#endif // QT_NO_SHORTCUT
    else {
        goto process;
    }
    goto accept;

process:
{
    if (q->isAcceptableInput(e)) {
        if (overwriteMode
            // no need to call deleteChar() if we have a selection, insertText
            // does it already
            && !cursor.hasSelection()
            && !cursor.atBlockEnd())
            cursor.deleteChar();

        cursor.insertText(e->text());
        selectionChanged();
    } else {
        e->ignore();
        return;
    }
}

accept:

#ifndef QT_NO_CLIPBOARD
    setClipboardSelection();
#endif

    e->accept();
    cursorOn = true;

    q->ensureCursorVisible();

    updateCurrentCharFormat();
}

QVariant WidgetTextControl::loadResource(int type, const QUrl &name)
{
    Q_UNUSED(type)
    Q_UNUSED(name)
    return QVariant();
}

void WidgetTextControlPrivate::_q_updateBlock(const QTextBlock &block)
{
    QRectF br = q->blockBoundingRect(block);
    br.setRight(qreal(INT_MAX)); // the block might have shrunk
    emit q->updateRequest(br);
}

QRectF WidgetTextControlPrivate::rectForPosition(int position) const
{
    const QTextBlock block = doc->findBlock(position);
    if (!block.isValid())
        return QRectF();
    const QAbstractTextDocumentLayout *docLayout = doc->documentLayout();
    const QTextLayout *layout = block.layout();
    const QPointF layoutPos = q->blockBoundingRect(block).topLeft();
    int relativePos = position - block.position();
    if (preeditCursor != 0) {
        int preeditPos = layout->preeditAreaPosition();
        if (relativePos == preeditPos)
            relativePos += preeditCursor;
        else if (relativePos > preeditPos)
            relativePos += layout->preeditAreaText().size();
    }
    QTextLine line = layout->lineForTextPosition(relativePos);

    int cursorWidth;
    {
        bool ok = false;
        cursorWidth = docLayout->property("cursorWidth").toInt(&ok);
        if (!ok)
            cursorWidth = 1;
    }

    QRectF r;

    if (line.isValid()) {
        qreal x = line.cursorToX(relativePos);
        qreal w = 0;
        if (overwriteMode) {
            if (relativePos < line.textLength() - line.textStart())
                w = line.cursorToX(relativePos + 1) - x;
            else
                w = QFontMetrics(block.layout()->font()).horizontalAdvance(u' '); // in sync with QTextLine::draw()
        }
        r = QRectF(layoutPos.x() + x, layoutPos.y() + line.y(),
                   cursorWidth + w, line.height());
    } else {
        r = QRectF(layoutPos.x(), layoutPos.y(), cursorWidth, 10); // #### correct height
    }

    return r;
}

namespace {
struct QTextFrameComparator {
    bool operator()(QTextFrame *frame, int position) { return frame->firstPosition() < position; }
    bool operator()(int position, QTextFrame *frame) { return position < frame->firstPosition(); }
};
}

static QRectF boundingRectOfFloatsInSelection(const QTextCursor &cursor)
{
    QRectF r;
    QTextFrame *frame = cursor.currentFrame();
    const QList<QTextFrame *> children = frame->childFrames();

    const QList<QTextFrame *>::ConstIterator firstFrame = std::lower_bound(children.constBegin(), children.constEnd(),
                                                                           cursor.selectionStart(), QTextFrameComparator());
    const QList<QTextFrame *>::ConstIterator lastFrame = std::upper_bound(children.constBegin(), children.constEnd(),
                                                                          cursor.selectionEnd(), QTextFrameComparator());
    for (QList<QTextFrame *>::ConstIterator it = firstFrame; it != lastFrame; ++it) {
        if ((*it)->frameFormat().position() != QTextFrameFormat::InFlow)
            r |= frame->document()->documentLayout()->frameBoundingRect(*it);
    }
    return r;
}

QRectF WidgetTextControl::selectionRect(const QTextCursor &cursor) const
{

    QRectF r = d->rectForPosition(cursor.selectionStart());

    if (cursor.hasComplexSelection() && cursor.currentTable()) {
        QTextTable *table = cursor.currentTable();

        r = d->doc->documentLayout()->frameBoundingRect(table);
        /*
        int firstRow, numRows, firstColumn, numColumns;
        cursor.selectedTableCells(&firstRow, &numRows, &firstColumn, &numColumns);

        const QTextTableCell firstCell = table->cellAt(firstRow, firstColumn);
        const QTextTableCell lastCell = table->cellAt(firstRow + numRows - 1, firstColumn + numColumns - 1);

        const QAbstractTextDocumentLayout * const layout = doc->documentLayout();

        QRectF tableSelRect = layout->blockBoundingRect(firstCell.firstCursorPosition().block());

        for (int col = firstColumn; col < firstColumn + numColumns; ++col) {
            const QTextTableCell cell = table->cellAt(firstRow, col);
            const qreal y = layout->blockBoundingRect(cell.firstCursorPosition().block()).top();

            tableSelRect.setTop(qMin(tableSelRect.top(), y));
        }

        for (int row = firstRow; row < firstRow + numRows; ++row) {
            const QTextTableCell cell = table->cellAt(row, firstColumn);
            const qreal x = layout->blockBoundingRect(cell.firstCursorPosition().block()).left();

            tableSelRect.setLeft(qMin(tableSelRect.left(), x));
        }

        for (int col = firstColumn; col < firstColumn + numColumns; ++col) {
            const QTextTableCell cell = table->cellAt(firstRow + numRows - 1, col);
            const qreal y = layout->blockBoundingRect(cell.lastCursorPosition().block()).bottom();

            tableSelRect.setBottom(qMax(tableSelRect.bottom(), y));
        }

        for (int row = firstRow; row < firstRow + numRows; ++row) {
            const QTextTableCell cell = table->cellAt(row, firstColumn + numColumns - 1);
            const qreal x = layout->blockBoundingRect(cell.lastCursorPosition().block()).right();

            tableSelRect.setRight(qMax(tableSelRect.right(), x));
        }

        r = tableSelRect.toRect();
        */
    } else if (cursor.hasSelection()) {
        const int position = cursor.selectionStart();
        const int anchor = cursor.selectionEnd();
        const QTextBlock posBlock = d->doc->findBlock(position);
        const QTextBlock anchorBlock = d->doc->findBlock(anchor);
        if (posBlock == anchorBlock && posBlock.isValid() && posBlock.layout()->lineCount()) {
            const QTextLine posLine = posBlock.layout()->lineForTextPosition(position - posBlock.position());
            const QTextLine anchorLine = anchorBlock.layout()->lineForTextPosition(anchor - anchorBlock.position());

            const int firstLine = qMin(posLine.lineNumber(), anchorLine.lineNumber());
            const int lastLine = qMax(posLine.lineNumber(), anchorLine.lineNumber());
            const QTextLayout *layout = posBlock.layout();
            r = QRectF();
            for (int i = firstLine; i <= lastLine; ++i) {
                r |= layout->lineAt(i).rect();
                r |= layout->lineAt(i).naturalTextRect(); // might be bigger in the case of wrap not enabled
            }
            r.translate(blockBoundingRect(posBlock).topLeft());
        } else {
            QRectF anchorRect = d->rectForPosition(cursor.selectionEnd());
            r |= anchorRect;
            r |= boundingRectOfFloatsInSelection(cursor);
            QRectF frameRect(d->doc->documentLayout()->frameBoundingRect(cursor.currentFrame()));
            r.setLeft(frameRect.left());
            r.setRight(frameRect.right());
        }
        if (r.isValid())
            r.adjust(-1, -1, 1, 1);
    }

    return r;
}

QRectF WidgetTextControl::selectionRect() const
{
    return selectionRect(d->cursor);
}

void WidgetTextControlPrivate::mousePressEvent(QEvent *e, Qt::MouseButton button, const QPointF &pos, Qt::KeyboardModifiers modifiers,
                                               Qt::MouseButtons buttons, const QPoint &globalPos)
{

    mousePressPos = pos.toPoint();

#if QT_CONFIG(draganddrop)
    mightStartDrag = false;
#endif

    if (sendMouseEventToInputContext(
            e, QEvent::MouseButtonPress, button, pos, modifiers, buttons, globalPos)) {
        return;
    }

    if (interactionFlags & Qt::LinksAccessibleByMouse) {
        anchorOnMousePress = q->anchorAt(pos);

        if (cursorIsFocusIndicator) {
            cursorIsFocusIndicator = false;
            repaintSelection();
            cursor.clearSelection();
        }
    }
    if (!(button & Qt::LeftButton) ||
        !((interactionFlags & Qt::TextSelectableByMouse) || (interactionFlags & Qt::TextEditable))) {
        e->ignore();
        return;
    }
    bool wasValid = blockWithMarkerUnderMouse.isValid();
    blockWithMarkerUnderMouse = q->blockWithMarkerAt(pos);
    if (wasValid != blockWithMarkerUnderMouse.isValid())
        emit q->blockMarkerHovered(blockWithMarkerUnderMouse);


    cursorIsFocusIndicator = false;
    const QTextCursor oldSelection = cursor;
    const int oldCursorPos = cursor.position();

    mousePressed = (interactionFlags & Qt::TextSelectableByMouse);

    commitPreedit();

    if (trippleClickTimer.isActive()
        && ((pos - trippleClickPoint).toPoint().manhattanLength() < QApplication::startDragDistance())) {

        cursor.movePosition(QTextCursor::StartOfBlock);
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        selectedBlockOnTrippleClick = cursor;

        anchorOnMousePress = QString();
        blockWithMarkerUnderMouse = QTextBlock();
        emit q->blockMarkerHovered(blockWithMarkerUnderMouse);

        trippleClickTimer.stop();
    } else {
        int cursorPos = q->hitTest(pos, Qt::FuzzyHit);
        if (cursorPos == -1) {
            e->ignore();
            return;
        }

        if (modifiers == Qt::ShiftModifier && (interactionFlags & Qt::TextSelectableByMouse)) {
            if (wordSelectionEnabled && !selectedWordOnDoubleClick.hasSelection()) {
                selectedWordOnDoubleClick = cursor;
                selectedWordOnDoubleClick.select(QTextCursor::WordUnderCursor);
            }

            if (selectedBlockOnTrippleClick.hasSelection())
                extendBlockwiseSelection(cursorPos);
            else if (selectedWordOnDoubleClick.hasSelection())
                extendWordwiseSelection(cursorPos, pos.x());
            else if (!wordSelectionEnabled)
                setCursorPosition(cursorPos, QTextCursor::KeepAnchor);
        } else {

            if (dragEnabled
                && cursor.hasSelection()
                && !cursorIsFocusIndicator
                && cursorPos >= cursor.selectionStart()
                && cursorPos <= cursor.selectionEnd()
                && q->hitTest(pos, Qt::ExactHit) != -1) {
#if QT_CONFIG(draganddrop)
                mightStartDrag = true;
#endif
                return;
            }

            setCursorPosition(cursorPos);
        }
    }

    if (interactionFlags & Qt::TextEditable) {
        q->ensureCursorVisible();
        if (cursor.position() != oldCursorPos)
            emit q->cursorPositionChanged();
        _q_updateCurrentCharFormatAndSelection();
    } else {
        if (cursor.position() != oldCursorPos) {
            emit q->cursorPositionChanged();
            emit q->microFocusChanged();
        }
        selectionChanged();
    }
    repaintOldAndNewSelection(oldSelection);
    hadSelectionOnMousePress = cursor.hasSelection();
}

void WidgetTextControlPrivate::mouseMoveEvent(QEvent *e, Qt::MouseButton button, const QPointF &mousePos, Qt::KeyboardModifiers modifiers,
                                              Qt::MouseButtons buttons, const QPoint &globalPos)
{

    if (interactionFlags & Qt::LinksAccessibleByMouse) {
        QString anchor = q->anchorAt(mousePos);
        if (anchor != highlightedAnchor) {
            highlightedAnchor = anchor;
            emit q->linkHovered(anchor);
        }
    }

    if (buttons & Qt::LeftButton) {
        const bool editable = interactionFlags & Qt::TextEditable;

        if (!(mousePressed
              || editable
              || mightStartDrag
              || selectedWordOnDoubleClick.hasSelection()
              || selectedBlockOnTrippleClick.hasSelection()))
            return;

        const QTextCursor oldSelection = cursor;
        const int oldCursorPos = cursor.position();

        if (mightStartDrag) {
            if ((mousePos.toPoint() - mousePressPos).manhattanLength() > QApplication::startDragDistance())
                startDrag();
            return;
        }

        const qreal mouseX = qreal(mousePos.x());

        int newCursorPos = q->hitTest(mousePos, Qt::FuzzyHit);

        if (isPreediting()) {
            // note: oldCursorPos not including preedit
            int selectionStartPos = q->hitTest(mousePressPos, Qt::FuzzyHit);

            if (newCursorPos != selectionStartPos) {
                commitPreedit();
                // commit invalidates positions
                newCursorPos = q->hitTest(mousePos, Qt::FuzzyHit);
                selectionStartPos = q->hitTest(mousePressPos, Qt::FuzzyHit);
                setCursorPosition(selectionStartPos);
            }
        }

        if (newCursorPos == -1)
            return;

        if (mousePressed && wordSelectionEnabled && !selectedWordOnDoubleClick.hasSelection()) {
            selectedWordOnDoubleClick = cursor;
            selectedWordOnDoubleClick.select(QTextCursor::WordUnderCursor);
        }

        if (selectedBlockOnTrippleClick.hasSelection())
            extendBlockwiseSelection(newCursorPos);
        else if (selectedWordOnDoubleClick.hasSelection())
            extendWordwiseSelection(newCursorPos, mouseX);
        else if (mousePressed && !isPreediting())
            setCursorPosition(newCursorPos, QTextCursor::KeepAnchor);

        if (interactionFlags & Qt::TextEditable) {
            // don't call ensureVisible for the visible cursor to avoid jumping
            // scrollbars. the autoscrolling ensures smooth scrolling if necessary.
            //q->ensureCursorVisible();
            if (cursor.position() != oldCursorPos)
                emit q->cursorPositionChanged();
            _q_updateCurrentCharFormatAndSelection();
#ifndef QT_NO_IM
            if (contextWidget)
                QGuiApplication::inputMethod()->update(Qt::ImQueryInput);
#endif //QT_NO_IM
        } else {
            //emit q->visibilityRequest(QRectF(mousePos, QSizeF(1, 1)));
            if (cursor.position() != oldCursorPos) {
                emit q->cursorPositionChanged();
                emit q->microFocusChanged();
            }
        }
        selectionChanged(true);
        repaintOldAndNewSelection(oldSelection);
    } else {
        bool wasValid = blockWithMarkerUnderMouse.isValid();
        blockWithMarkerUnderMouse = q->blockWithMarkerAt(mousePos);
        if (wasValid != blockWithMarkerUnderMouse.isValid())
            emit q->blockMarkerHovered(blockWithMarkerUnderMouse);
    }

    sendMouseEventToInputContext(e, QEvent::MouseMove, button, mousePos, modifiers, buttons, globalPos);
}

void WidgetTextControlPrivate::mouseReleaseEvent(QEvent *e, Qt::MouseButton button, const QPointF &pos, Qt::KeyboardModifiers modifiers,
                                                 Qt::MouseButtons buttons, const QPoint &globalPos)
{

    const QTextCursor oldSelection = cursor;
    if (sendMouseEventToInputContext(
            e, QEvent::MouseButtonRelease, button, pos, modifiers, buttons, globalPos)) {
        repaintOldAndNewSelection(oldSelection);
        return;
    }

    const int oldCursorPos = cursor.position();

#if QT_CONFIG(draganddrop)
    if (mightStartDrag && (button & Qt::LeftButton)) {
        mousePressed = false;
        setCursorPosition(pos);
        cursor.clearSelection();
        selectionChanged();
    }
#endif
    if (mousePressed) {
        mousePressed = false;
#ifndef QT_NO_CLIPBOARD
        setClipboardSelection();
        selectionChanged(true);
    } else if (button == Qt::MiddleButton
               && (interactionFlags & Qt::TextEditable)
               && QGuiApplication::clipboard()->supportsSelection()) {
        setCursorPosition(pos);
        const QMimeData *md = QGuiApplication::clipboard()->mimeData(QClipboard::Selection);
        if (md)
            q->insertFromMimeData(md);
#endif
    }

    repaintOldAndNewSelection(oldSelection);

    if (cursor.position() != oldCursorPos) {
        emit q->cursorPositionChanged();
        emit q->microFocusChanged();
    }

    // toggle any checkbox that the user clicks
    if ((interactionFlags & Qt::TextEditable) && (button & Qt::LeftButton) &&
        (blockWithMarkerUnderMouse.isValid()) && !cursor.hasSelection()) {
        QTextBlock markerBlock = q->blockWithMarkerAt(pos);
        if (markerBlock == blockWithMarkerUnderMouse) {
            auto fmt = blockWithMarkerUnderMouse.blockFormat();
            switch (fmt.marker()) {
            case QTextBlockFormat::MarkerType::Unchecked :
                fmt.setMarker(QTextBlockFormat::MarkerType::Checked);
                break;
            case QTextBlockFormat::MarkerType::Checked:
                fmt.setMarker(QTextBlockFormat::MarkerType::Unchecked);
                break;
            default:
                break;
            }
            cursor.setBlockFormat(fmt);
        }
    }

    if (interactionFlags & Qt::LinksAccessibleByMouse) {

        // Ignore event unless left button has been pressed
        if (!(button & Qt::LeftButton)) {
            e->ignore();
            return;
        }

        const QString anchor = q->anchorAt(pos);

        // Ignore event without selection anchor
        if (anchor.isEmpty()) {
            e->ignore();
            return;
        }

        if (!cursor.hasSelection()
            || (anchor == anchorOnMousePress && hadSelectionOnMousePress)) {

            const int anchorPos = q->hitTest(pos, Qt::ExactHit);

            // Ignore event without valid anchor position
            if (anchorPos < 0) {
                e->ignore();
                return;
            }

            cursor.setPosition(anchorPos);
            QString anchor = anchorOnMousePress;
            anchorOnMousePress = QString();
            activateLinkUnderCursor(anchor);
        }
    }
}

void WidgetTextControlPrivate::mouseDoubleClickEvent(QEvent *e, Qt::MouseButton button, const QPointF &pos,
                                                     Qt::KeyboardModifiers modifiers, Qt::MouseButtons buttons,
                                                     const QPoint &globalPos)
{

    if (button == Qt::LeftButton
        && (interactionFlags & Qt::TextSelectableByMouse)) {

#if QT_CONFIG(draganddrop)
        mightStartDrag = false;
#endif
        commitPreedit();

        const QTextCursor oldSelection = cursor;
        setCursorPosition(pos);
        QTextLine line = currentTextLine(cursor);
        bool doEmit = false;
        if (line.isValid() && line.textLength()) {
            cursor.select(QTextCursor::WordUnderCursor);
            doEmit = true;
        }
        repaintOldAndNewSelection(oldSelection);

        cursorIsFocusIndicator = false;
        selectedWordOnDoubleClick = cursor;

        trippleClickPoint = pos;
        trippleClickTimer.start(QApplication::doubleClickInterval(), q);
        if (doEmit) {
            selectionChanged();
#ifndef QT_NO_CLIPBOARD
            setClipboardSelection();
#endif
            emit q->cursorPositionChanged();
        }
    } else if (!sendMouseEventToInputContext(e, QEvent::MouseButtonDblClick, button, pos,
                                             modifiers, buttons, globalPos)) {
        e->ignore();
    }
}

bool WidgetTextControlPrivate::sendMouseEventToInputContext(
    QEvent *e, QEvent::Type eventType, Qt::MouseButton button, const QPointF &pos,
    Qt::KeyboardModifiers modifiers, Qt::MouseButtons buttons, const QPoint &globalPos)
{
    Q_UNUSED(eventType)
    Q_UNUSED(button)
    Q_UNUSED(pos)
    Q_UNUSED(modifiers)
    Q_UNUSED(buttons)
    Q_UNUSED(globalPos)
#if !defined(QT_NO_IM)

    if (isPreediting()) {
        QTextLayout *layout = cursor.block().layout();
        int cursorPos = q->hitTest(pos, Qt::FuzzyHit) - cursor.position();

        if (cursorPos < 0 || cursorPos > layout->preeditAreaText().size())
            cursorPos = -1;

        if (cursorPos >= 0) {
            if (eventType == QEvent::MouseButtonRelease)
                QGuiApplication::inputMethod()->invokeAction(QInputMethod::Click, cursorPos);

            e->setAccepted(true);
            return true;
        }
    }
#else
    Q_UNUSED(e)
#endif
    return false;
}

void WidgetTextControlPrivate::contextMenuEvent(const QPoint &screenPos, const QPointF &docPos, QWidget *contextWidget)
{
#ifdef QT_NO_CONTEXTMENU
    Q_UNUSED(screenPos)
    Q_UNUSED(docPos)
    Q_UNUSED(contextWidget)
#else
    QMenu *menu = q->createStandardContextMenu(docPos, contextWidget);
    if (!menu)
        return;
    menu->setAttribute(Qt::WA_DeleteOnClose);

/*
    if (auto *widget = qobject_cast<QWidget *>(parent())) {
        if (auto *window = widget->window()->windowHandle())
            ;// QMenuPrivate::get(menu)->topData()->initialScreen = window->screen();
    }
 */

    menu->popup(screenPos);
#endif
}

bool WidgetTextControlPrivate::dragEnterEvent(QEvent *e, const QMimeData *mimeData)
{
    if (!(interactionFlags & Qt::TextEditable) || !q->canInsertFromMimeData(mimeData)) {
        e->ignore();
        return false;
    }

    dndFeedbackCursor = QTextCursor();

    return true; // accept proposed action
}

void WidgetTextControlPrivate::dragLeaveEvent()
{

    const QRectF crect = q->cursorRect(dndFeedbackCursor);
    dndFeedbackCursor = QTextCursor();

    if (crect.isValid())
        emit q->updateRequest(crect);
}

bool WidgetTextControlPrivate::dragMoveEvent(QEvent *e, const QMimeData *mimeData, const QPointF &pos)
{
    if (!(interactionFlags & Qt::TextEditable) || !q->canInsertFromMimeData(mimeData)) {
        e->ignore();
        return false;
    }

    const int cursorPos = q->hitTest(pos, Qt::FuzzyHit);
    if (cursorPos != -1) {
        QRectF crect = q->cursorRect(dndFeedbackCursor);
        if (crect.isValid())
            emit q->updateRequest(crect);

        dndFeedbackCursor = cursor;
        dndFeedbackCursor.setPosition(cursorPos);

        crect = q->cursorRect(dndFeedbackCursor);
        emit q->updateRequest(crect);
    }

    return true; // accept proposed action
}

bool WidgetTextControlPrivate::dropEvent(const QMimeData *mimeData, const QPointF &pos, Qt::DropAction dropAction, QObject *source)
{
    dndFeedbackCursor = QTextCursor();

    if (!(interactionFlags & Qt::TextEditable) || !q->canInsertFromMimeData(mimeData))
        return false;

    repaintSelection();

    QTextCursor insertionCursor = q->cursorForPosition(pos);
    insertionCursor.beginEditBlock();

    if (dropAction == Qt::MoveAction && source == contextWidget)
        cursor.removeSelectedText();

    cursor = insertionCursor;
    q->insertFromMimeData(mimeData);
    insertionCursor.endEditBlock();
    q->ensureCursorVisible();
    return true; // accept proposed action
}

void WidgetTextControlPrivate::inputMethodEvent(QInputMethodEvent *e)
{
    if (!(interactionFlags & (Qt::TextEditable | Qt::TextSelectableByMouse)) || cursor.isNull()) {
        e->ignore();
        return;
    }
    bool isGettingInput = !e->commitString().isEmpty()
                          || e->preeditString() != cursor.block().layout()->preeditAreaText()
                          || e->replacementLength() > 0;

    if (!isGettingInput && e->attributes().isEmpty()) {
        e->ignore();
        return;
    }

    int oldCursorPos = cursor.position();

    cursor.beginEditBlock();
    if (isGettingInput) {
        cursor.removeSelectedText();
    }

    QTextBlock block;

    // insert commit string
    if (!e->commitString().isEmpty() || e->replacementLength()) {
        if (e->commitString().endsWith(QChar::LineFeed))
            block = cursor.block(); // Remember the block where the preedit text is
        QTextCursor c = cursor;
        c.setPosition(c.position() + e->replacementStart());
        c.setPosition(c.position() + e->replacementLength(), QTextCursor::KeepAnchor);
        c.insertText(e->commitString());
    }

    for (int i = 0; i < e->attributes().size(); ++i) {
        const QInputMethodEvent::Attribute &a = e->attributes().at(i);
        if (a.type == QInputMethodEvent::Selection) {
            QTextCursor oldCursor = cursor;
            int blockStart = a.start + cursor.block().position();
            cursor.setPosition(blockStart, QTextCursor::MoveAnchor);
            cursor.setPosition(blockStart + a.length, QTextCursor::KeepAnchor);
            q->ensureCursorVisible();
            repaintOldAndNewSelection(oldCursor);
        }
    }

    if (!block.isValid())
        block = cursor.block();
    QTextLayout *layout = block.layout();
    if (isGettingInput)
        layout->setPreeditArea(cursor.position() - block.position(), e->preeditString());
    QList<QTextLayout::FormatRange> overrides;
    overrides.reserve(e->attributes().size());
    const int oldPreeditCursor = preeditCursor;
    preeditCursor = e->preeditString().size();
    hideCursor = false;
    for (int i = 0; i < e->attributes().size(); ++i) {
        const QInputMethodEvent::Attribute &a = e->attributes().at(i);
        if (a.type == QInputMethodEvent::Cursor) {
            preeditCursor = a.start;
            hideCursor = !a.length;
        } else if (a.type == QInputMethodEvent::TextFormat) {
            QTextCharFormat f = cursor.charFormat();
            f.merge(qvariant_cast<QTextFormat>(a.value).toCharFormat());
            if (f.isValid()) {
                QTextLayout::FormatRange o;
                o.start = a.start + cursor.position() - block.position();
                o.length = a.length;
                o.format = f;

                // Make sure list is sorted by start index
                QList<QTextLayout::FormatRange>::iterator it = overrides.end();
                while (it != overrides.begin()) {
                    QList<QTextLayout::FormatRange>::iterator previous = it - 1;
                    if (o.start >= previous->start) {
                        overrides.insert(it, o);
                        break;
                    }
                    it = previous;
                }

                if (it == overrides.begin())
                    overrides.prepend(o);
            }
        }
    }

    if (cursor.charFormat().isValid()) {
        int start = cursor.position() - block.position();
        int end = start + e->preeditString().size();

        QList<QTextLayout::FormatRange>::iterator it = overrides.begin();
        while (it != overrides.end()) {
            QTextLayout::FormatRange range = *it;
            int rangeStart = range.start;
            if (rangeStart > start) {
                QTextLayout::FormatRange o;
                o.start = start;
                o.length = rangeStart - start;
                o.format = cursor.charFormat();
                it = overrides.insert(it, o) + 1;
            }

            ++it;
            start = range.start + range.length;
        }

        if (start < end) {
            QTextLayout::FormatRange o;
            o.start = start;
            o.length = end - start;
            o.format = cursor.charFormat();
            overrides.append(o);
        }
    }
    layout->setFormats(overrides);

    cursor.endEditBlock();

    // if (cursor.d)
    //     cursor.d->setX();
    if (oldCursorPos != cursor.position())
        emit q->cursorPositionChanged();
    if (oldPreeditCursor != preeditCursor)
        emit q->microFocusChanged();
}

QVariant WidgetTextControl::inputMethodQuery(Qt::InputMethodQuery property, QVariant argument) const
{
    QTextBlock block = d->cursor.block();
    switch (property) {
    case Qt::ImCursorRectangle:
        return cursorRect();
    case Qt::ImAnchorRectangle:
        return d->rectForPosition(d->cursor.anchor());
    case Qt::ImFont:
        return QVariant(d->cursor.charFormat().font());
    case Qt::ImCursorPosition: {
        const QPointF pt = argument.toPointF();
        if (!pt.isNull())
            return QVariant(cursorForPosition(pt).position() - block.position());
        return QVariant(d->cursor.position() - block.position()); }
    case Qt::ImSurroundingText:
        return QVariant(block.text());
    case Qt::ImCurrentSelection:
        return QVariant(d->cursor.selectedText());
    case Qt::ImMaximumTextLength:
        return QVariant(); // No limit.
    case Qt::ImAnchorPosition:
        return QVariant(d->cursor.anchor() - block.position());
    case Qt::ImAbsolutePosition: {
        const QPointF pt = argument.toPointF();
        if (!pt.isNull())
            return QVariant(cursorForPosition(pt).position());
        return QVariant(d->cursor.position()); }
    case Qt::ImTextAfterCursor:
    {
        int maxLength = argument.isValid() ? argument.toInt() : 1024;
        QTextCursor tmpCursor = d->cursor;
        int localPos = d->cursor.position() - block.position();
        QString result = block.text().mid(localPos);
        while (result.size() < maxLength) {
            int currentBlock = tmpCursor.blockNumber();
            tmpCursor.movePosition(QTextCursor::NextBlock);
            if (tmpCursor.blockNumber() == currentBlock)
                break;
            result += u'\n' + tmpCursor.block().text();
        }
        return QVariant(result);
    }
    case Qt::ImTextBeforeCursor:
    {
        int maxLength = argument.isValid() ? argument.toInt() : 1024;
        QTextCursor tmpCursor = d->cursor;
        int localPos = d->cursor.position() - block.position();
        int numBlocks = 0;
        int resultLen = localPos;
        while (resultLen < maxLength) {
            int currentBlock = tmpCursor.blockNumber();
            tmpCursor.movePosition(QTextCursor::PreviousBlock);
            if (tmpCursor.blockNumber() == currentBlock)
                break;
            numBlocks++;
            resultLen += tmpCursor.block().length();
        }
        QString result;
        while (numBlocks) {
            result += tmpCursor.block().text() + u'\n';
            tmpCursor.movePosition(QTextCursor::NextBlock);
            --numBlocks;
        }
        result += QStringView{block.text()}.mid(0, localPos);
        return QVariant(result);
    }
    default:
        return QVariant();
    }
}

void WidgetTextControl::setFocus(bool focus, Qt::FocusReason reason)
{
    QFocusEvent ev(focus ? QEvent::FocusIn : QEvent::FocusOut,
                   reason);
    processEvent(&ev);
}

void WidgetTextControlPrivate::focusEvent(QFocusEvent *e)
{
    emit q->updateRequest(q->selectionRect());
    if (e->gotFocus()) {
#ifdef QT_KEYPAD_NAVIGATION
        if (!QApplicationPrivate::keypadNavigationEnabled() || (hasEditFocus && (e->reason() == Qt::PopupFocusReason))) {
#endif
            cursorOn = (interactionFlags & (Qt::TextSelectableByKeyboard | Qt::TextEditable));
            if (interactionFlags & Qt::TextEditable) {
                setCursorVisible(true);
            }
#ifdef QT_KEYPAD_NAVIGATION
        }
#endif
    } else {
        setCursorVisible(false);
        cursorOn = false;

        if (cursorIsFocusIndicator
            && e->reason() != Qt::ActiveWindowFocusReason
            && e->reason() != Qt::PopupFocusReason
            && cursor.hasSelection()) {
            cursor.clearSelection();
        }
    }
    hasFocus = e->gotFocus();
}

QString WidgetTextControlPrivate::anchorForCursor(const QTextCursor &anchorCursor) const
{
    if (anchorCursor.hasSelection()) {
        QTextCursor cursor = anchorCursor;
        if (cursor.selectionStart() != cursor.position())
            cursor.setPosition(cursor.selectionStart());
        cursor.movePosition(QTextCursor::NextCharacter);
        QTextCharFormat fmt = cursor.charFormat();
        if (fmt.isAnchor() && fmt.hasProperty(QTextFormat::AnchorHref))
            return fmt.stringProperty(QTextFormat::AnchorHref);
    }
    return QString();
}

#ifdef QT_KEYPAD_NAVIGATION
void WidgetTextControlPrivate::editFocusEvent(QEvent *e)
{

    if (QApplicationPrivate::keypadNavigationEnabled()) {
        if (e->type() == QEvent::EnterEditFocus && interactionFlags & Qt::TextEditable) {
            const QTextCursor oldSelection = cursor;
            const int oldCursorPos = cursor.position();
            const bool moved = cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
            q->ensureCursorVisible();
            if (moved) {
                if (cursor.position() != oldCursorPos)
                    emit q->cursorPositionChanged();
                emit q->microFocusChanged();
            }
            selectionChanged();
            repaintOldAndNewSelection(oldSelection);

            setBlinkingCursorEnabled(true);
        } else
            setBlinkingCursorEnabled(false);
    }

    hasEditFocus = e->type() == QEvent::EnterEditFocus;
}
#endif

#ifndef QT_NO_CONTEXTMENU
void setActionIcon(QAction *action, const QString &name)
{
    const QIcon icon = QIcon::fromTheme(name);
    if (!icon.isNull())
        action->setIcon(icon);
}

static QString ACCEL_KEY(QKeySequence::StandardKey k)
{
    auto result = QKeySequence(k).toString(QKeySequence::NativeText);
    if (!result.isEmpty())
        result.prepend('\t');
    return result;
}

QMenu *WidgetTextControl::createStandardContextMenu(const QPointF &pos, QWidget *parent)
{

    const bool showTextSelectionActions = d->interactionFlags & (Qt::TextEditable | Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);

    d->linkToCopy = QString();
    if (!pos.isNull())
        d->linkToCopy = anchorAt(pos);

    if (d->linkToCopy.isEmpty() && !showTextSelectionActions)
        return nullptr;

    QMenu *menu = new QMenu(parent);
    QAction *a;

    if (d->interactionFlags & Qt::TextEditable) {
        a = menu->addAction(Tr::tr("&Undo") + ACCEL_KEY(QKeySequence::Undo), this, SLOT(undo()));
        a->setEnabled(d->doc->isUndoAvailable());
        a->setObjectName(QStringLiteral("edit-undo"));
        setActionIcon(a, QStringLiteral("edit-undo"));
        a = menu->addAction(Tr::tr("&Redo") + ACCEL_KEY(QKeySequence::Redo), this, SLOT(redo()));
        a->setEnabled(d->doc->isRedoAvailable());
        a->setObjectName(QStringLiteral("edit-redo"));
        setActionIcon(a, QStringLiteral("edit-redo"));
        menu->addSeparator();

#ifndef QT_NO_CLIPBOARD
        a = menu->addAction(Tr::tr("Cu&t") + ACCEL_KEY(QKeySequence::Cut), this, SLOT(cut()));
        a->setEnabled(d->cursor.hasSelection());
        a->setObjectName(QStringLiteral("edit-cut"));
        setActionIcon(a, QStringLiteral("edit-cut"));
#endif
    }

#ifndef QT_NO_CLIPBOARD
    if (showTextSelectionActions) {
        a = menu->addAction(Tr::tr("&Copy") + ACCEL_KEY(QKeySequence::Copy), this, SLOT(copy()));
        a->setEnabled(d->cursor.hasSelection());
        a->setObjectName(QStringLiteral("edit-copy"));
        setActionIcon(a, QStringLiteral("edit-copy"));
    }

    if ((d->interactionFlags & Qt::LinksAccessibleByKeyboard)
        || (d->interactionFlags & Qt::LinksAccessibleByMouse)) {
        a = menu->addAction(Tr::tr("Copy &Link Location"), this, SLOT(_q_copyLink()));
        a->setEnabled(!d->linkToCopy.isEmpty());
        a->setObjectName(QStringLiteral("link-copy"));
    }
#endif // QT_NO_CLIPBOARD

    if (d->interactionFlags & Qt::TextEditable) {
#ifndef QT_NO_CLIPBOARD
        a = menu->addAction(Tr::tr("&Paste") + ACCEL_KEY(QKeySequence::Paste), this, SLOT(paste()));
        a->setEnabled(canPaste());
        a->setObjectName(QStringLiteral("edit-paste"));
        setActionIcon(a, QStringLiteral("edit-paste"));
#endif
        a = menu->addAction(Tr::tr("Delete"), this, SLOT(_q_deleteSelected()));
        a->setEnabled(d->cursor.hasSelection());
        a->setObjectName(QStringLiteral("edit-delete"));
        setActionIcon(a, QStringLiteral("edit-delete"));
    }


    if (showTextSelectionActions) {
        menu->addSeparator();
        a = menu->addAction(
            Tr::tr("Select All") + ACCEL_KEY(QKeySequence::SelectAll), this, SLOT(selectAll()));
        a->setEnabled(!d->doc->isEmpty());
        a->setObjectName(QStringLiteral("select-all"));
        setActionIcon(a, QStringLiteral("edit-select-all"));
    }

    if ((d->interactionFlags & Qt::TextEditable) && QGuiApplication::styleHints()->useRtlExtensions()) {
        menu->addSeparator();
        UnicodeControlCharacterMenu *ctrlCharacterMenu = new UnicodeControlCharacterMenu(this, menu);
        menu->addMenu(ctrlCharacterMenu);
    }

    return menu;
}
#endif // QT_NO_CONTEXTMENU

QTextCursor WidgetTextControl::cursorForPosition(const QPointF &pos) const
{
    int cursorPos = hitTest(pos, Qt::FuzzyHit);
    if (cursorPos == -1)
        cursorPos = 0;
    QTextCursor c(d->doc);
    c.setPosition(cursorPos);
    return c;
}

QRectF WidgetTextControl::cursorRect(const QTextCursor &cursor) const
{
    if (cursor.isNull())
        return QRectF();

    return d->rectForPosition(cursor.position());
}

QRectF WidgetTextControl::cursorRect() const
{
    return cursorRect(d->cursor);
}

QRectF WidgetTextControlPrivate::cursorRectPlusUnicodeDirectionMarkers(const QTextCursor &cursor) const
{
    if (cursor.isNull())
        return QRectF();

    return rectForPosition(cursor.position()).adjusted(-4, 0, 4, 0);
}

QString WidgetTextControl::anchorAt(const QPointF &pos) const
{
    return d->doc->documentLayout()->anchorAt(pos);
}

QString WidgetTextControl::anchorAtCursor() const
{

    return d->anchorForCursor(d->cursor);
}

QTextBlock WidgetTextControl::blockWithMarkerAt(const QPointF &pos) const
{
    return d->doc->documentLayout()->blockWithMarkerAt(pos);
}

bool WidgetTextControl::overwriteMode() const
{
    return d->overwriteMode;
}

void WidgetTextControl::setOverwriteMode(bool overwrite)
{
    d->overwriteMode = overwrite;
}

int WidgetTextControl::cursorWidth() const
{
    return d->doc->documentLayout()->property("cursorWidth").toInt();
}

void WidgetTextControl::setCursorWidth(int width)
{
    if (width == -1)
        width = QApplication::style()->pixelMetric(QStyle::PM_TextCursorWidth, nullptr, qobject_cast<QWidget *>(parent()));
    d->doc->documentLayout()->setProperty("cursorWidth", width);
    d->repaintCursor();
}

bool WidgetTextControl::acceptRichText() const
{
    return d->acceptRichText;
}

void WidgetTextControl::setAcceptRichText(bool accept)
{
    d->acceptRichText = accept;
}

#if QT_CONFIG(textedit)

void WidgetTextControl::setExtraSelections(const QList<QTextEdit::ExtraSelection> &selections)
{

    QMultiHash<int, int> hash;
    for (int i = 0; i < d->extraSelections.size(); ++i) {
        const QAbstractTextDocumentLayout::Selection &esel = d->extraSelections.at(i);
        hash.insert(esel.cursor.anchor(), i);
    }

    for (int i = 0; i < selections.size(); ++i) {
        const QTextEdit::ExtraSelection &sel = selections.at(i);
        const auto it = hash.constFind(sel.cursor.anchor());
        if (it != hash.cend()) {
            const QAbstractTextDocumentLayout::Selection &esel = d->extraSelections.at(it.value());
            if (esel.cursor.position() == sel.cursor.position()
                && esel.format == sel.format) {
                hash.erase(it);
                continue;
            }
        }
        QRectF r = selectionRect(sel.cursor);
        if (sel.format.boolProperty(QTextFormat::FullWidthSelection)) {
            r.setLeft(0);
            r.setWidth(qreal(INT_MAX));
        }
        emit updateRequest(r);
    }

    for (auto it = hash.cbegin(); it != hash.cend(); ++it) {
        const QAbstractTextDocumentLayout::Selection &esel = d->extraSelections.at(it.value());
        QRectF r = selectionRect(esel.cursor);
        if (esel.format.boolProperty(QTextFormat::FullWidthSelection)) {
            r.setLeft(0);
            r.setWidth(qreal(INT_MAX));
        }
        emit updateRequest(r);
    }

    d->extraSelections.resize(selections.size());
    for (int i = 0; i < selections.size(); ++i) {
        d->extraSelections[i].cursor = selections.at(i).cursor;
        d->extraSelections[i].format = selections.at(i).format;
    }
}

QList<QTextEdit::ExtraSelection> WidgetTextControl::extraSelections() const
{
    QList<QTextEdit::ExtraSelection> selections;
    const int numExtraSelections = d->extraSelections.size();
    selections.reserve(numExtraSelections);
    for (int i = 0; i < numExtraSelections; ++i) {
        QTextEdit::ExtraSelection sel;
        const QAbstractTextDocumentLayout::Selection &sel2 = d->extraSelections.at(i);
        sel.cursor = sel2.cursor;
        sel.format = sel2.format;
        selections.append(sel);
    }
    return selections;
}

#endif // QT_CONFIG(textedit)

void WidgetTextControl::setTextWidth(qreal width)
{
    d->doc->setTextWidth(width);
}

qreal WidgetTextControl::textWidth() const
{
    return d->doc->textWidth();
}

QSizeF WidgetTextControl::size() const
{
    return d->doc->size();
}

void WidgetTextControl::setOpenExternalLinks(bool open)
{
    d->openExternalLinks = open;
}

bool WidgetTextControl::openExternalLinks() const
{
    return d->openExternalLinks;
}

bool WidgetTextControl::ignoreUnusedNavigationEvents() const
{
    return d->ignoreUnusedNavigationEvents;
}

void WidgetTextControl::setIgnoreUnusedNavigationEvents(bool ignore)
{
    d->ignoreUnusedNavigationEvents = ignore;
}

void WidgetTextControl::moveCursor(QTextCursor::MoveOperation op, QTextCursor::MoveMode mode)
{
    const QTextCursor oldSelection = d->cursor;
    const bool moved = d->cursor.movePosition(op, mode);
    d->_q_updateCurrentCharFormatAndSelection();
    ensureCursorVisible();
    d->repaintOldAndNewSelection(oldSelection);
    if (moved)
        emit cursorPositionChanged();
}

bool WidgetTextControl::canPaste() const
{
#ifndef QT_NO_CLIPBOARD
    if (d->interactionFlags & Qt::TextEditable) {
        const QMimeData *md = QGuiApplication::clipboard()->mimeData();
        return md && canInsertFromMimeData(md);
    }
#endif
    return false;
}

void WidgetTextControl::setCursorIsFocusIndicator(bool b)
{
    d->cursorIsFocusIndicator = b;
    d->repaintCursor();
}

bool WidgetTextControl::cursorIsFocusIndicator() const
{
    return d->cursorIsFocusIndicator;
}


void WidgetTextControl::setDragEnabled(bool enabled)
{
    d->dragEnabled = enabled;
}

bool WidgetTextControl::isDragEnabled() const
{
    return d->dragEnabled;
}

void WidgetTextControl::setWordSelectionEnabled(bool enabled)
{
    d->wordSelectionEnabled = enabled;
}

bool WidgetTextControl::isWordSelectionEnabled() const
{
    return d->wordSelectionEnabled;
}

bool WidgetTextControl::isPreediting()
{
    return d->isPreediting();
}

#ifndef QT_NO_PRINTER
void WidgetTextControl::print(QPagedPaintDevice *printer) const
{
    if (!printer)
        return;
    QTextDocument *tempDoc = nullptr;
    const QTextDocument *doc = d->doc;
    if (auto qprinter = dynamic_cast<QPrinter *>(printer); qprinter && qprinter->printRange() == QPrinter::Selection) {
        if (!d->cursor.hasSelection())
            return;
        tempDoc = new QTextDocument(const_cast<QTextDocument *>(doc));
        tempDoc->setResourceProvider(doc->resourceProvider());
        tempDoc->setMetaInformation(QTextDocument::DocumentTitle, doc->metaInformation(QTextDocument::DocumentTitle));
        tempDoc->setPageSize(doc->pageSize());
        tempDoc->setDefaultFont(doc->defaultFont());
        tempDoc->setUseDesignMetrics(doc->useDesignMetrics());
        QTextCursor(tempDoc).insertFragment(d->cursor.selection());
        doc = tempDoc;

        // copy the custom object handlers
        // doc->documentLayout()->d_func()->handlers = d->doc->documentLayout()->d_func()->handlers;
    }
    doc->print(printer);
    delete tempDoc;
}
#endif

QMimeData *WidgetTextControl::createMimeDataFromSelection() const
{
    const QTextDocumentFragment fragment(d->cursor);
    return new TextEditMimeData(fragment);
}

bool WidgetTextControl::canInsertFromMimeData(const QMimeData *source) const
{
    if (d->acceptRichText)
        return (source->hasText() && !source->text().isEmpty())
               || source->hasHtml()
               || source->hasFormat("application/x-qrichtext")
               || source->hasFormat("application/x-qt-richtext");
    else
        return source->hasText() && !source->text().isEmpty();
}

void WidgetTextControl::insertFromMimeData(const QMimeData *source)
{
    if (!(d->interactionFlags & Qt::TextEditable) || !source)
        return;

    bool hasData = false;
    QTextDocumentFragment fragment;
#if QT_CONFIG(textmarkdownreader)
    const auto formats = source->formats();
    if (formats.size() && formats.first() == "text/markdown") {
        auto s = QString::fromUtf8(source->data("text/markdown"));
        fragment = QTextDocumentFragment::fromMarkdown(s);
        hasData = true;
    } else
#endif
#ifndef QT_NO_TEXTHTMLPARSER
        if (source->hasFormat("application/x-qrichtext") && d->acceptRichText) {
            // x-qrichtext is always UTF-8 (taken from Qt3 since we don't use it anymore).
            const QString richtext = "<meta name=\"qrichtext\" content=\"1\" />"
                                     + QString::fromUtf8(source->data("application/x-qrichtext"));
            fragment = QTextDocumentFragment::fromHtml(richtext, d->doc);
            hasData = true;
        } else if (source->hasHtml() && d->acceptRichText) {
            fragment = QTextDocumentFragment::fromHtml(source->html(), d->doc);
            hasData = true;
        }
#endif // QT_NO_TEXTHTMLPARSER
    if (!hasData) {
        const QString text = source->text();
        if (!text.isNull()) {
            fragment = QTextDocumentFragment::fromPlainText(text);
            hasData = true;
        }
    }

    if (hasData)
        d->cursor.insertFragment(fragment);
    ensureCursorVisible();
}

bool WidgetTextControl::findNextPrevAnchor(const QTextCursor &startCursor, bool next, QTextCursor &newAnchor)
{

    int anchorStart = -1;
    QString anchorHref;
    int anchorEnd = -1;

    if (next) {
        const int startPos = startCursor.selectionEnd();

        QTextBlock block = d->doc->findBlock(startPos);
        QTextBlock::Iterator it = block.begin();

        while (!it.atEnd() && it.fragment().position() < startPos)
            ++it;

        while (block.isValid()) {
            anchorStart = -1;

            // find next anchor
            for (; !it.atEnd(); ++it) {
                const QTextFragment fragment = it.fragment();
                const QTextCharFormat fmt = fragment.charFormat();

                if (fmt.isAnchor() && fmt.hasProperty(QTextFormat::AnchorHref)) {
                    anchorStart = fragment.position();
                    anchorHref = fmt.anchorHref();
                    break;
                }
            }

            if (anchorStart != -1) {
                anchorEnd = -1;

                // find next non-anchor fragment
                for (; !it.atEnd(); ++it) {
                    const QTextFragment fragment = it.fragment();
                    const QTextCharFormat fmt = fragment.charFormat();

                    if (!fmt.isAnchor() || fmt.anchorHref() != anchorHref) {
                        anchorEnd = fragment.position();
                        break;
                    }
                }

                if (anchorEnd == -1)
                    anchorEnd = block.position() + block.length() - 1;

                // make found selection
                break;
            }

            block = block.next();
            it = block.begin();
        }
    } else {
        int startPos = startCursor.selectionStart();
        if (startPos > 0)
            --startPos;

        QTextBlock block = d->doc->findBlock(startPos);
        QTextBlock::Iterator blockStart = block.begin();
        QTextBlock::Iterator it = block.end();

        if (startPos == block.position()) {
            it = block.begin();
        } else {
            do {
                if (it == blockStart) {
                    it = QTextBlock::Iterator();
                    block = QTextBlock();
                } else {
                    --it;
                }
            } while (!it.atEnd() && it.fragment().position() + it.fragment().length() - 1 > startPos);
        }

        while (block.isValid()) {
            anchorStart = -1;

            if (!it.atEnd()) {
                do {
                    const QTextFragment fragment = it.fragment();
                    const QTextCharFormat fmt = fragment.charFormat();

                    if (fmt.isAnchor() && fmt.hasProperty(QTextFormat::AnchorHref)) {
                        anchorStart = fragment.position() + fragment.length();
                        anchorHref = fmt.anchorHref();
                        break;
                    }

                    if (it == blockStart)
                        it = QTextBlock::Iterator();
                    else
                        --it;
                } while (!it.atEnd());
            }

            if (anchorStart != -1 && !it.atEnd()) {
                anchorEnd = -1;

                do {
                    const QTextFragment fragment = it.fragment();
                    const QTextCharFormat fmt = fragment.charFormat();

                    if (!fmt.isAnchor() || fmt.anchorHref() != anchorHref) {
                        anchorEnd = fragment.position() + fragment.length();
                        break;
                    }

                    if (it == blockStart)
                        it = QTextBlock::Iterator();
                    else
                        --it;
                } while (!it.atEnd());

                if (anchorEnd == -1)
                    anchorEnd = qMax(0, block.position());

                break;
            }

            block = block.previous();
            it = block.end();
            if (it != block.begin())
                --it;
            blockStart = block.begin();
        }

    }

    if (anchorStart != -1 && anchorEnd != -1) {
        newAnchor = d->cursor;
        newAnchor.setPosition(anchorStart);
        newAnchor.setPosition(anchorEnd, QTextCursor::KeepAnchor);
        return true;
    }

    return false;
}

void WidgetTextControlPrivate::activateLinkUnderCursor(QString href)
{
    QTextCursor oldCursor = cursor;

    if (href.isEmpty()) {
        QTextCursor tmp = cursor;
        if (tmp.selectionStart() != tmp.position())
            tmp.setPosition(tmp.selectionStart());
        tmp.movePosition(QTextCursor::NextCharacter);
        href = tmp.charFormat().anchorHref();
    }
    if (href.isEmpty())
        return;

    if (!cursor.hasSelection()) {
        QTextBlock block = cursor.block();
        const int cursorPos = cursor.position();

        QTextBlock::Iterator it = block.begin();
        QTextBlock::Iterator linkFragment;

        for (; !it.atEnd(); ++it) {
            QTextFragment fragment = it.fragment();
            const int fragmentPos = fragment.position();
            if (fragmentPos <= cursorPos &&
                fragmentPos + fragment.length() > cursorPos) {
                linkFragment = it;
                break;
            }
        }

        if (!linkFragment.atEnd()) {
            it = linkFragment;
            cursor.setPosition(it.fragment().position());
            if (it != block.begin()) {
                do {
                    --it;
                    QTextFragment fragment = it.fragment();
                    if (fragment.charFormat().anchorHref() != href)
                        break;
                    cursor.setPosition(fragment.position());
                } while (it != block.begin());
            }

            for (it = linkFragment; !it.atEnd(); ++it) {
                QTextFragment fragment = it.fragment();
                if (fragment.charFormat().anchorHref() != href)
                    break;
                cursor.setPosition(fragment.position() + fragment.length(), QTextCursor::KeepAnchor);
            }
        }
    }

    if (hasFocus) {
        cursorIsFocusIndicator = true;
    } else {
        cursorIsFocusIndicator = false;
        cursor.clearSelection();
    }
    repaintOldAndNewSelection(oldCursor);

#ifndef QT_NO_DESKTOPSERVICES
    if (openExternalLinks)
        QDesktopServices::openUrl(href);
    else
#endif
        emit q->linkActivated(href);
}

#if QT_CONFIG(tooltip)
void WidgetTextControlPrivate::showToolTip(const QPoint &globalPos, const QPointF &pos, QWidget *contextWidget)
{
    const QString toolTip = q->cursorForPosition(pos).charFormat().toolTip();
    if (toolTip.isEmpty())
        return;
    QToolTip::showText(globalPos, toolTip, contextWidget);
}
#endif // QT_CONFIG(tooltip)

bool WidgetTextControlPrivate::isPreediting() const
{
    QTextLayout *layout = cursor.block().layout();
    if (layout && !layout->preeditAreaText().isEmpty())
        return true;

    return false;
}

void WidgetTextControlPrivate::commitPreedit()
{
    if (!isPreediting())
        return;

    QGuiApplication::inputMethod()->commit();

    if (!isPreediting())
        return;

    cursor.beginEditBlock();
    preeditCursor = 0;
    QTextBlock block = cursor.block();
    QTextLayout *layout = block.layout();
    layout->setPreeditArea(-1, QString());
    layout->clearFormats();
    cursor.endEditBlock();
}

bool WidgetTextControl::setFocusToNextOrPreviousAnchor(bool next)
{

    if (!(d->interactionFlags & Qt::LinksAccessibleByKeyboard))
        return false;

    QRectF crect = selectionRect();
    emit updateRequest(crect);

    // If we don't have a current anchor, we start from the start/end
    if (!d->cursor.hasSelection()) {
        d->cursor = QTextCursor(d->doc);
        if (next)
            d->cursor.movePosition(QTextCursor::Start);
        else
            d->cursor.movePosition(QTextCursor::End);
    }

    QTextCursor newAnchor;
    if (findNextPrevAnchor(d->cursor, next, newAnchor)) {
        d->cursor = newAnchor;
        d->cursorIsFocusIndicator = true;
    } else {
        d->cursor.clearSelection();
    }

    if (d->cursor.hasSelection()) {
        crect = selectionRect();
        emit updateRequest(crect);
        emit visibilityRequest(crect);
        return true;
    } else {
        return false;
    }
}

bool WidgetTextControl::setFocusToAnchor(const QTextCursor &newCursor)
{

    if (!(d->interactionFlags & Qt::LinksAccessibleByKeyboard))
        return false;

    // Verify that this is an anchor.
    const QString anchorHref = d->anchorForCursor(newCursor);
    if (anchorHref.isEmpty())
        return false;

    // and process it
    QRectF crect = selectionRect();
    emit updateRequest(crect);

    d->cursor.setPosition(newCursor.selectionStart());
    d->cursor.setPosition(newCursor.selectionEnd(), QTextCursor::KeepAnchor);
    d->cursorIsFocusIndicator = true;

    crect = selectionRect();
    emit updateRequest(crect);
    emit visibilityRequest(crect);
    return true;
}

void WidgetTextControl::setTextInteractionFlags(Qt::TextInteractionFlags flags)
{
    if (flags == d->interactionFlags)
        return;
    d->interactionFlags = flags;

    if (d->hasFocus)
        d->setCursorVisible(flags & Qt::TextEditable);
}

Qt::TextInteractionFlags WidgetTextControl::textInteractionFlags() const
{
    return d->interactionFlags;
}

void WidgetTextControl::mergeCurrentCharFormat(const QTextCharFormat &modifier)
{
    d->cursor.mergeCharFormat(modifier);
    d->updateCurrentCharFormat();
}

void WidgetTextControl::setCurrentCharFormat(const QTextCharFormat &format)
{
    d->cursor.setCharFormat(format);
    d->updateCurrentCharFormat();
}

QTextCharFormat WidgetTextControl::currentCharFormat() const
{
    return d->cursor.charFormat();
}

void WidgetTextControl::insertPlainText(const QString &text)
{
    d->cursor.insertText(text);
}

#ifndef QT_NO_TEXTHTMLPARSER
void WidgetTextControl::insertHtml(const QString &text)
{
    d->cursor.insertHtml(text);
}
#endif // QT_NO_TEXTHTMLPARSER

QPointF WidgetTextControl::anchorPosition(const QString &name) const
{
    if (name.isEmpty())
        return QPointF();

    QRectF r;
    for (QTextBlock block = d->doc->begin(); block.isValid(); block = block.next()) {
        QTextCharFormat format = block.charFormat();
        if (format.isAnchor() && format.anchorNames().contains(name)) {
            r = d->rectForPosition(block.position());
            break;
        }

        for (QTextBlock::Iterator it = block.begin(); !it.atEnd(); ++it) {
            QTextFragment fragment = it.fragment();
            format = fragment.charFormat();
            if (format.isAnchor() && format.anchorNames().contains(name)) {
                r = d->rectForPosition(fragment.position());
                block = QTextBlock();
                break;
            }
        }
    }
    if (!r.isValid())
        return QPointF();
    return QPointF(0, r.top());
}

void WidgetTextControl::adjustSize()
{
    d->doc->adjustSize();
}

bool WidgetTextControl::find(const QString &exp, QTextDocument::FindFlags options)
{
    QTextCursor search = d->doc->find(exp, d->cursor, options);
    if (search.isNull())
        return false;

    setTextCursor(search);
    return true;
}

#if QT_CONFIG(regularexpression)
bool WidgetTextControl::find(const QRegularExpression &exp, QTextDocument::FindFlags options)
{
    QTextCursor search = d->doc->find(exp, d->cursor, options);
    if (search.isNull())
        return false;

    setTextCursor(search);
    return true;
}
#endif

QString WidgetTextControl::toPlainText() const
{
    return document()->toPlainText();
}

#ifndef QT_NO_TEXTHTMLPARSER
QString WidgetTextControl::toHtml() const
{
    return document()->toHtml();
}
#endif

#if QT_CONFIG(textmarkdownwriter)
QString WidgetTextControl::toMarkdown(QTextDocument::MarkdownFeatures features) const
{
    return document()->toMarkdown(features);
}
#endif

void WidgetTextControlPrivate::insertParagraphSeparator()
{
    // clear blockFormat properties that the user is unlikely to want duplicated:
    // - don't insert <hr/> automatically
    // - the next paragraph after a heading should be a normal paragraph
    // - remove the bottom margin from the last list item before appending
    // - the next checklist item after a checked item should be unchecked
    auto blockFmt = cursor.blockFormat();
    auto charFmt = cursor.charFormat();
    blockFmt.clearProperty(QTextFormat::BlockTrailingHorizontalRulerWidth);
    if (blockFmt.hasProperty(QTextFormat::HeadingLevel)) {
        blockFmt.clearProperty(QTextFormat::HeadingLevel);
        charFmt = QTextCharFormat();
    }
    if (cursor.currentList()) {
        auto existingFmt = cursor.blockFormat();
        existingFmt.clearProperty(QTextBlockFormat::BlockBottomMargin);
        cursor.setBlockFormat(existingFmt);
        if (blockFmt.marker() == QTextBlockFormat::MarkerType::Checked)
            blockFmt.setMarker(QTextBlockFormat::MarkerType::Unchecked);
    }

    // After a blank line, reset block and char formats. I.e. you can end a list,
    // block quote, etc. by hitting enter twice, and get back to normal paragraph style.
    if (cursor.block().text().isEmpty() &&
        !cursor.blockFormat().hasProperty(QTextFormat::BlockTrailingHorizontalRulerWidth) &&
        !cursor.blockFormat().hasProperty(QTextFormat::BlockCodeLanguage)) {
        blockFmt = QTextBlockFormat();
        const bool blockFmtChanged = (cursor.blockFormat() != blockFmt);
        charFmt = QTextCharFormat();
        cursor.setBlockFormat(blockFmt);
        cursor.setCharFormat(charFmt);
        // If the user hit enter twice just to get back to default format,
        // don't actually insert a new block. But if the user then hits enter
        // yet again, the block format will not change, so we will insert a block.
        // This is what many word processors do.
        if (blockFmtChanged)
            return;
    }

    cursor.insertBlock(blockFmt, charFmt);
}

void WidgetTextControlPrivate::append(const QString &text, Qt::TextFormat format)
{
    QTextCursor tmp(doc);
    tmp.beginEditBlock();
    tmp.movePosition(QTextCursor::End);

    if (!doc->isEmpty())
        tmp.insertBlock(cursor.blockFormat(), cursor.charFormat());
    else
        tmp.setCharFormat(cursor.charFormat());

    // preserve the char format
    QTextCharFormat oldCharFormat = cursor.charFormat();

#ifndef QT_NO_TEXTHTMLPARSER
    if (format == Qt::RichText || (format == Qt::AutoText && Qt::mightBeRichText(text))) {
        tmp.insertHtml(text);
    } else {
        tmp.insertText(text);
    }
#else
    Q_UNUSED(format)
    tmp.insertText(text);
#endif // QT_NO_TEXTHTMLPARSER
    if (!cursor.hasSelection())
        cursor.setCharFormat(oldCharFormat);

    tmp.endEditBlock();
}

void WidgetTextControl::append(const QString &text)
{
    d->append(text, Qt::AutoText);
}

void WidgetTextControl::appendHtml(const QString &html)
{
    d->append(html, Qt::RichText);
}

void WidgetTextControl::appendPlainText(const QString &text)
{
    d->append(text, Qt::PlainText);
}


void WidgetTextControl::ensureCursorVisible()
{
    QRectF crect = d->rectForPosition(d->cursor.position()).adjusted(-5, 0, 5, 0);
    emit visibilityRequest(crect);
    emit microFocusChanged();
}

QPalette WidgetTextControl::palette() const
{
    return d->palette;
}

void WidgetTextControl::setPalette(const QPalette &pal)
{
    d->palette = pal;
}

QAbstractTextDocumentLayout::PaintContext WidgetTextControl::getPaintContext(QWidget *widget) const
{

    QAbstractTextDocumentLayout::PaintContext ctx;

    ctx.selections = d->extraSelections;
    ctx.palette = d->palette;
    // #if QT_CONFIG(style_stylesheet)
    //     if (widget) {
    //         if (auto cssStyle = qt_styleSheet(widget->style())) {
    //             QStyleOption option;
    //             option.initFrom(widget);
    //             cssStyle->styleSheetPalette(widget, &option, &ctx.palette);
    //         }
    //     }
    // #endif // style_stylesheet
    if (d->cursorOn && d->isEnabled) {
        if (d->hideCursor)
            ctx.cursorPosition = -1;
        else if (d->preeditCursor != 0)
            ctx.cursorPosition = - (d->preeditCursor + 2);
        else
            ctx.cursorPosition = d->cursor.position();
    }

    if (!d->dndFeedbackCursor.isNull())
        ctx.cursorPosition = d->dndFeedbackCursor.position();
#ifdef QT_KEYPAD_NAVIGATION
    if (!QApplicationPrivate::keypadNavigationEnabled() || d->hasEditFocus)
#endif
        if (d->cursor.hasSelection()) {
            QAbstractTextDocumentLayout::Selection selection;
            selection.cursor = d->cursor;
            if (d->cursorIsFocusIndicator) {
                QStyleOption opt;
                opt.palette = ctx.palette;
                QStyleHintReturnVariant ret;
                QStyle *style = QApplication::style();
                if (widget)
                    style = widget->style();
                style->styleHint(QStyle::SH_TextControl_FocusIndicatorTextCharFormat, &opt, widget, &ret);
                selection.format = qvariant_cast<QTextFormat>(ret.variant).toCharFormat();
            } else {
                QPalette::ColorGroup cg = d->hasFocus ? QPalette::Active : QPalette::Inactive;
                selection.format.setBackground(ctx.palette.brush(cg, QPalette::Highlight));
                selection.format.setForeground(ctx.palette.brush(cg, QPalette::HighlightedText));
                QStyleOption opt;
                QStyle *style = QApplication::style();
                if (widget) {
                    opt.initFrom(widget);
                    style = widget->style();
                }
                if (style->styleHint(QStyle::SH_RichText_FullWidthSelection, &opt, widget))
                    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
            }
            ctx.selections.append(selection);
        }

    return ctx;
}

void WidgetTextControl::drawContents(QPainter *p, const QRectF &rect, QWidget *widget)
{
    p->save();
    QAbstractTextDocumentLayout::PaintContext ctx = getPaintContext(widget);
    if (rect.isValid())
        p->setClipRect(rect, Qt::IntersectClip);
    ctx.clip = rect;

    d->doc->documentLayout()->draw(p, ctx);
    p->restore();
}

void WidgetTextControlPrivate::_q_copyLink()
{
#ifndef QT_NO_CLIPBOARD
    QMimeData *md = new QMimeData;
    md->setText(linkToCopy);
    QGuiApplication::clipboard()->setMimeData(md);
#endif
}

int WidgetTextControl::hitTest(const QPointF &point, Qt::HitTestAccuracy accuracy) const
{
    return d->doc->documentLayout()->hitTest(point, accuracy);
}

QRectF WidgetTextControl::blockBoundingRect(const QTextBlock &block) const
{
    return d->doc->documentLayout()->blockBoundingRect(block);
}

#ifndef QT_NO_CONTEXTMENU
#define NUM_CONTROL_CHARACTERS 14
const struct QUnicodeControlCharacter {
    const char *text;
    ushort character;
} qt_controlCharacters[NUM_CONTROL_CHARACTERS] = {
    { QT_TRANSLATE_NOOP("QUnicodeControlCharacterMenu", "LRM Left-to-right mark"), 0x200e },
    { QT_TRANSLATE_NOOP("QUnicodeControlCharacterMenu", "RLM Right-to-left mark"), 0x200f },
    { QT_TRANSLATE_NOOP("QUnicodeControlCharacterMenu", "ZWJ Zero width joiner"), 0x200d },
    { QT_TRANSLATE_NOOP("QUnicodeControlCharacterMenu", "ZWNJ Zero width non-joiner"), 0x200c },
    { QT_TRANSLATE_NOOP("QUnicodeControlCharacterMenu", "ZWSP Zero width space"), 0x200b },
    { QT_TRANSLATE_NOOP("QUnicodeControlCharacterMenu", "LRE Start of left-to-right embedding"), 0x202a },
    { QT_TRANSLATE_NOOP("QUnicodeControlCharacterMenu", "RLE Start of right-to-left embedding"), 0x202b },
    { QT_TRANSLATE_NOOP("QUnicodeControlCharacterMenu", "LRO Start of left-to-right override"), 0x202d },
    { QT_TRANSLATE_NOOP("QUnicodeControlCharacterMenu", "RLO Start of right-to-left override"), 0x202e },
    { QT_TRANSLATE_NOOP("QUnicodeControlCharacterMenu", "PDF Pop directional formatting"), 0x202c },
    { QT_TRANSLATE_NOOP("QUnicodeControlCharacterMenu", "LRI Left-to-right isolate"), 0x2066 },
    { QT_TRANSLATE_NOOP("QUnicodeControlCharacterMenu", "RLI Right-to-left isolate"), 0x2067 },
    { QT_TRANSLATE_NOOP("QUnicodeControlCharacterMenu", "FSI First strong isolate"), 0x2068 },
    { QT_TRANSLATE_NOOP("QUnicodeControlCharacterMenu", "PDI Pop directional isolate"), 0x2069 }
};

UnicodeControlCharacterMenu::UnicodeControlCharacterMenu(QObject *_editWidget, QWidget *parent)
    : QMenu(parent), editWidget(_editWidget)
{
    setTitle(Tr::tr("Insert Unicode Control Character"));
    for (int i = 0; i < NUM_CONTROL_CHARACTERS; ++i) {
        addAction(Tr::tr(qt_controlCharacters[i].text), this, SLOT(menuActionTriggered()));
    }
}

void UnicodeControlCharacterMenu::menuActionTriggered()
{
    QAction *a = qobject_cast<QAction *>(sender());
    int idx = actions().indexOf(a);
    if (idx < 0 || idx >= NUM_CONTROL_CHARACTERS)
        return;
    QChar c(qt_controlCharacters[idx].character);
    QString str(c);

#if QT_CONFIG(textedit)
    if (QTextEdit *edit = qobject_cast<QTextEdit *>(editWidget)) {
        edit->insertPlainText(str);
        return;
    }
#endif
    if (WidgetTextControl *control = qobject_cast<WidgetTextControl *>(editWidget)) {
        control->insertPlainText(str);
    }
#if QT_CONFIG(lineedit)
    if (QLineEdit *edit = qobject_cast<QLineEdit *>(editWidget)) {
        edit->insert(str);
        return;
    }
#endif
}
#endif // QT_NO_CONTEXTMENU

static QStringList supportedMimeTypes()
{
    static const QStringList mimeTypes{
        "text/plain",
        "text/html"
#if QT_CONFIG(textmarkdownwriter)
        , "text/markdown"
#endif
#if QT_CONFIG(textodfwriter)
        , "application/vnd.oasis.opendocument.text"
#endif
    };
    return mimeTypes;
}

/*! \internal
    \reimp
*/
QStringList TextEditMimeData::formats() const
{
    if (!fragment.isEmpty())
        return supportedMimeTypes();

    return QMimeData::formats();
}

/*! \internal
    \reimp
*/
bool TextEditMimeData::hasFormat(const QString &format) const
{
    if (!fragment.isEmpty())
        return supportedMimeTypes().contains(format);

    return QMimeData::hasFormat(format);
}

QVariant TextEditMimeData::retrieveData(const QString &mimeType, QMetaType type) const
{
    if (!fragment.isEmpty())
        setup();
    return QMimeData::retrieveData(mimeType, type);
}

void TextEditMimeData::setup() const
{
    TextEditMimeData *that = const_cast<TextEditMimeData *>(this);
#ifndef QT_NO_TEXTHTMLPARSER
    that->setData("text/html", fragment.toHtml().toUtf8());
#endif
#if QT_CONFIG(textmarkdownwriter)
    that->setData("text/markdown", fragment.toMarkdown().toUtf8());
#endif
#ifndef QT_NO_TEXTODFWRITER
    {
        QBuffer buffer;
        QTextDocumentWriter writer(&buffer, "ODF");
        writer.write(fragment);
        buffer.close();
        that->setData("application/vnd.oasis.opendocument.text", buffer.data());
    }
#endif
    that->setText(fragment.toPlainText());
    fragment = QTextDocumentFragment();
}

} // namespace Utils

#include "widgettextcontrol.moc"
