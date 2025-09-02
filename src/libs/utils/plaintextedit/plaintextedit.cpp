// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "plaintextedit.h"

#include "inputcontrol.h"
#include "widgettextcontrol.h"

#include <QAccessible>
#include <QApplication>
#include <QBasicTimer>
#include <QGestureEvent>
#include <QPainter>
#include <QPointer>
#include <QScrollBar>
#include <QStyle>
#include <QTextBlock>

#include <memory.h>

namespace Utils {

class PlainTextEditControl : public WidgetTextControl
{
    Q_OBJECT
public:
    PlainTextEditControl(PlainTextEdit *parent);

    QMimeData *createMimeDataFromSelection() const override;
    bool canInsertFromMimeData(const QMimeData *source) const override;
    void insertFromMimeData(const QMimeData *source) override;
    int hitTest(const QPointF &point, Qt::HitTestAccuracy = Qt::FuzzyHit) const override;
    QRectF blockBoundingRect(const QTextBlock &block) const override;
    inline QRectF cursorRect(const QTextCursor &cursor) const {
        QRectF r = WidgetTextControl::cursorRect(cursor);
        r.setLeft(qMax(r.left(), (qreal) 0.));
        return r;
    }
    inline QRectF cursorRect() { return cursorRect(textCursor()); }
    void ensureCursorVisible() override {
        textEdit->ensureCursorVisible();
        emit microFocusChanged();
    }

    PlainTextEdit *textEdit;
    int topBlock;
    QTextBlock firstVisibleBlock() const;

    QVariant loadResource(int type, const QUrl &name) override {
        return textEdit->loadResource(type, name);
    }
};


class PlainTextEditPrivate : public QObject
{
    Q_OBJECT
public:
    PlainTextEditPrivate(PlainTextEdit *qq);

    QWidget *viewport() const { return q->viewport(); }
    QScrollBar *hbar() { return q->horizontalScrollBar(); }
    const QScrollBar *hbar() const { return q->horizontalScrollBar(); }
    QScrollBar *vbar() { return q->verticalScrollBar(); }
    const QScrollBar *vbar() const { return q->verticalScrollBar(); }

    void init(const QString &txt = QString());
    void repaintContents(const QRectF &contentsRect);
    void updatePlaceholderVisibility();

    inline QPoint mapToContents(const QPoint &point) const
    { return QPoint(point.x() + horizontalOffset(), point.y() + verticalOffset()); }

    int vScrollbarValueForLine(int topLine);
    int topLineForVScrollbarValue(int scrollbarValue);
    void adjustScrollbars();
    void verticalScrollbarActionTriggered(int action);
    void ensureViewportLayouted();
    void relayoutDocument();

    void pageUpDown(QTextCursor::MoveOperation op, QTextCursor::MoveMode moveMode, bool moveCursor = true);

    inline int horizontalOffset() const
    { return (q->isRightToLeft() ? (hbar()->maximum() - hbar()->value()) : hbar()->value()); }
    qreal verticalOffset(int topBlock, int topLine) const;
    qreal verticalOffset() const;

    inline void sendControlEvent(QEvent *e)
    { control->processEvent(e, QPointF(horizontalOffset(), verticalOffset()), viewport()); }

    void updateDefaultTextOption();

    PlainTextEdit *q = nullptr;
    QBasicTimer autoScrollTimer;
#ifdef QT_KEYPAD_NAVIGATION
    QBasicTimer deleteAllTimer;
#endif
    QPoint autoScrollDragPos;
    QString placeholderText;

    PlainTextEditControl *control = nullptr;
    qreal topLineOffset = 0;
    qreal pageUpDownLastCursorY = 0;
    PlainTextEdit::LineWrapMode lineWrap = PlainTextEdit::WidgetWidth;
    QTextOption::WrapMode wordWrap = QTextOption::WrapAtWordBoundaryOrAnywhere;
    int originalOffsetY = 0;
    int topLine = 0;

    uint tabChangesFocus : 1;
    uint showCursorOnInitialShow : 1;
    uint backgroundVisible : 1;
    uint centerOnScroll : 1;
    uint inDrag : 1;
    uint clickCausedFocus : 1;
    uint pageUpDownLastCursorYIsValid : 1;
    uint placeholderTextShown : 1;

    void setTopLine(int visualTopLine, int dx = 0, int dy = 0);
    void setTopBlock(int newTopBlock, int newTopLine, int dx = 0, int dy = 0);

    void ensureVisible(int position, bool center, bool forceCenter = false);
    void ensureCursorVisible(bool center = false);
    void updateViewport();

    QPointer<PlainTextDocumentLayout> documentLayoutPtr;

    void append(const QString &text, Qt::TextFormat format = Qt::AutoText);

    void cursorPositionChanged();
    void modificationChanged(bool);
    inline bool placeHolderTextToBeShown() const
    {
        return q->document()->isEmpty() && !q->placeholderText().isEmpty();
    }
    inline void handleSoftwareInputPanel(Qt::MouseButton button, bool clickCausedFocus)
    {
        if (button == Qt::LeftButton)
            handleSoftwareInputPanel(clickCausedFocus);
    }

    inline void handleSoftwareInputPanel(bool clickCausedFocus = false)
    {
        if (qApp->autoSipEnabled()) {
            QStyle::RequestSoftwareInputPanel behavior = QStyle::RequestSoftwareInputPanel(
                q->style()->styleHint(QStyle::SH_RequestSoftwareInputPanel));
            if (!clickCausedFocus || behavior == QStyle::RSIP_OnMouseClick) {
                QGuiApplication::inputMethod()->show();
            }
        }
    }

};

static inline bool shouldEnableInputMethod(PlainTextEdit *control)
{
#if defined(Q_OS_ANDROID)
    Q_UNUSED(control)
    return !control->isReadOnly() || (control->textInteractionFlags() & Qt::TextSelectableByMouse);
#else
    return !control->isReadOnly();
#endif
}

class PlainTextDocumentLayoutPrivate
{
public:
    PlainTextDocumentLayoutPrivate(PlainTextDocumentLayout *qq) {
        q = qq;
        mainViewPrivate = nullptr;
        width = 0;
        maximumWidth = 0;
        maximumWidthBlockNumber = 0;
        blockCount = 1;
        blockUpdate = blockDocumentSizeChanged = false;
        cursorWidth = 1;
        textLayoutFlags = 0;
    }

    PlainTextDocumentLayout *q;

    qreal width;
    qreal maximumWidth;
    int maximumWidthBlockNumber;
    int blockCount;
    PlainTextEditPrivate *mainViewPrivate;
    bool blockUpdate;
    bool blockDocumentSizeChanged;
    int cursorWidth;
    int textLayoutFlags;

    void layoutBlock(const QTextBlock &block);
    qreal blockWidth(const QTextBlock &block);

    void relayout();
};



/*! \class Utils::PlainTextDocumentLayout
    \inmodule QtCreator

    \brief The PlainTextDocumentLayout class implements a plain text layout for QTextDocument.

   A PlainTextDocumentLayout is required for text documents that can
   be display or edited in a PlainTextEdit. See
   QTextDocument::setDocumentLayout().

   PlainTextDocumentLayout uses the QAbstractTextDocumentLayout API
   that QTextDocument requires, but redefines it partially in order to
   support plain text better. For instances, it does not operate on
   vertical pixels, but on paragraphs (called blocks) instead. The
   height of a document is identical to the number of paragraphs it
   contains. The layout also doesn't support tables or nested frames,
   or any sort of advanced text layout that goes beyond a list of
   paragraphs with syntax highlighting.

*/



/*!
  Constructs a plain text document layout for the text \a document.
 */
PlainTextDocumentLayout::PlainTextDocumentLayout(QTextDocument *document)
    : QAbstractTextDocumentLayout(document)
    , d(std::make_unique<PlainTextDocumentLayoutPrivate>(this))
{
}
/*!
  Destructs a plain text document layout.
 */
PlainTextDocumentLayout::~PlainTextDocumentLayout() {}


/*!
  \overload QAbstractTextDocumentLayout::draw()
 */
void PlainTextDocumentLayout::draw(QPainter *, const PaintContext &)
{
}

/*!
  \overload QAbstractTextDocumentLayout::hitTest()
 */
int PlainTextDocumentLayout::hitTest(const QPointF &, Qt::HitTestAccuracy ) const
{
    //     this function is used from
    //     QAbstractTextDocumentLayout::anchorAt(), but is not
    //     implementable in a plain text document layout, because the
    //     layout depends on the top block and top line which depends on
    //     the view
    return -1;
}

/*!
  \overload QAbstractTextDocumentLayout::pageCount()
 */
int PlainTextDocumentLayout::pageCount() const
{ return 1; }

/*!
  \overload QAbstractTextDocumentLayout::documentSize()
 */
QSizeF PlainTextDocumentLayout::documentSize() const
{
    return QSizeF(d->maximumWidth, document()->lineCount());
}

QRectF PlainTextDocumentLayout::frameBoundingRect(QTextFrame *) const
{
    return QRectF(0, 0, qMax(d->width, d->maximumWidth), qreal(INT_MAX));
}

QRectF PlainTextDocumentLayout::blockBoundingRect(const QTextBlock &block) const
{
    if (!block.isValid()) { return QRectF(); }
    QTextLayout *tl = block.layout();
    if (!tl->lineCount())
        const_cast<PlainTextDocumentLayout*>(this)->layoutBlock(block);
    QRectF br;
    if (block.isVisible()) {
        br = QRectF(QPointF(0, 0), tl->boundingRect().bottomRight());
        if (tl->lineCount() == 1)
            br.setWidth(qMax(br.width(), tl->lineAt(0).naturalTextWidth()));
        qreal margin = document()->documentMargin();
        br.adjust(0, 0, margin, 0);
        if (!block.next().isValid())
            br.adjust(0, 0, 0, margin);
    }
    return br;

}

/*!
  \internal

  Ensures that \a block has a valid layout
 */
void PlainTextDocumentLayout::ensureBlockLayout(const QTextBlock &block) const
{
    if (!block.isValid())
        return;
    QTextLayout *tl = block.layout();
    if (!tl->lineCount())
        const_cast<PlainTextDocumentLayout*>(this)->layoutBlock(block);
}


/*! \property Utils::PlainTextDocumentLayout::cursorWidth

    This property specifies the width of the cursor in pixels. The default value is 1.
*/
void PlainTextDocumentLayout::setCursorWidth(int width)
{
    d->cursorWidth = width;
}

int PlainTextDocumentLayout::cursorWidth() const
{
    return d->cursorWidth;
}

/*!

   Requests a complete update on all views.
 */
void PlainTextDocumentLayout::requestUpdate()
{
    emit update(QRectF(0., -document()->documentMargin(), 1000000000., 1000000000.));
}


void PlainTextDocumentLayout::setTextWidth(qreal newWidth)
{
    d->width = d->maximumWidth = newWidth;
    bool layoutChanged = false;
    for (QTextBlock block = document()->firstBlock(); block.isValid(); block = block.next()) {
        QTextLayout *tl = block.layout();
        if (tl->lineCount() == 0 || (tl->lineCount() == 1 && tl->lineAt(0).naturalTextWidth() < newWidth))
            continue;
        layoutChanged = true;
        tl->clearLayout();
        block.setLineCount(block.isVisible() ? 1 : 0);
    }
    if (layoutChanged)
        emit update();
}

qreal PlainTextDocumentLayout::textWidth() const
{
    return d->width;
}

void PlainTextDocumentLayoutPrivate::relayout()
{
    QTextBlock block = q->document()->firstBlock();
    while (block.isValid()) {
        block.layout()->clearLayout();
        block.setLineCount(block.isVisible() ? 1 : 0);
        block = block.next();
    }
    emit q->update();
}


/*!
  \overload QAbstractTextDocumentLayout::documentChanged()
 */
void PlainTextDocumentLayout::documentChanged(int from, int charsRemoved, int charsAdded)
{
    QTextDocument *doc = document();
    int newBlockCount = doc->blockCount();
    int charsChanged = charsRemoved + charsAdded;

    QTextBlock changeStartBlock = doc->findBlock(from);
    QTextBlock changeEndBlock = doc->findBlock(qMax(0, from + charsChanged - 1));
    bool blockVisibilityChanged = false;

    if (changeStartBlock == changeEndBlock && newBlockCount == d->blockCount) {
        QTextBlock block = changeStartBlock;
        if (block.isValid() && block.length()) {
            QRectF oldBr = blockBoundingRect(block);
            layoutBlock(block);
            QRectF newBr = blockBoundingRect(block);
            if (newBr.height() == oldBr.height()) {
                if (!d->blockUpdate)
                    emit updateBlock(block);
                return;
            }
        }
    } else {
        QTextBlock block = changeStartBlock;
        do {
            block.clearLayout();
            if (block.isVisible()
                    ? (block.lineCount() == 0)
                    : (block.lineCount() > 0)) {
                blockVisibilityChanged = true;
                block.setLineCount(block.isVisible() ? 1 : 0);
            }
            if (block == changeEndBlock)
                break;
            block = block.next();
        } while (block.isValid());
    }

    if (newBlockCount != d->blockCount || blockVisibilityChanged) {
        int changeEnd = changeEndBlock.blockNumber();
        int blockDiff = newBlockCount - d->blockCount;
        int oldChangeEnd = changeEnd - blockDiff;

        if (d->maximumWidthBlockNumber > oldChangeEnd)
            d->maximumWidthBlockNumber += blockDiff;

        d->blockCount = newBlockCount;
        if (d->blockCount == 1)
            d->maximumWidth = blockWidth(doc->firstBlock());

        if (!d->blockDocumentSizeChanged)
            emit documentSizeChanged(documentSize());

        if (blockDiff == 1 && changeEnd == newBlockCount -1 ) {
            if (!d->blockUpdate) {
                QTextBlock b = changeStartBlock;
                for (;;) {
                    emit updateBlock(b);
                    if (b == changeEndBlock)
                        break;
                    b = b.next();
                }
            }
            return;
        }
    }

    if (!d->blockUpdate)
        emit update(QRectF(0., -doc->documentMargin(), 1000000000., 1000000000.)); // optimization potential
}


void PlainTextDocumentLayout::layoutBlock(const QTextBlock &block)
{
    QTextDocument *doc = document();
    qreal margin = doc->documentMargin();
    qreal blockMaximumWidth = 0;

    qreal height = 0;
    QTextLayout *tl = block.layout();
    QTextOption option = doc->defaultTextOption();
    tl->setTextOption(option);

    int extraMargin = 0;
    if (option.flags() & QTextOption::AddSpaceForLineAndParagraphSeparators) {
        QFontMetrics fm(block.charFormat().font());
        extraMargin += fm.horizontalAdvance(QChar(0x21B5));
    }
    tl->beginLayout();
    qreal availableWidth = d->width;
    if (availableWidth <= 0) {
        availableWidth = qreal(INT_MAX); // similar to text edit with pageSize.width == 0
    }
    availableWidth -= 2*margin + extraMargin;
    while (1) {
        QTextLine line = tl->createLine();
        if (!line.isValid())
            break;
        line.setLeadingIncluded(true);
        line.setLineWidth(availableWidth);
        line.setPosition(QPointF(margin, height));
        height += line.height();
        if (line.leading() < 0)
            height += qCeil(line.leading());
        blockMaximumWidth = qMax(blockMaximumWidth, line.naturalTextWidth() + 2*margin);
    }
    tl->endLayout();

    int previousLineCount = doc->lineCount();
    const_cast<QTextBlock&>(block).setLineCount(block.isVisible() ? tl->lineCount() : 0);
    int lineCount = doc->lineCount();

    bool emitDocumentSizeChanged = previousLineCount != lineCount;
    if (blockMaximumWidth > d->maximumWidth) {
        // new longest line
        d->maximumWidth = blockMaximumWidth;
        d->maximumWidthBlockNumber = block.blockNumber();
        emitDocumentSizeChanged = true;
    } else if (block.blockNumber() == d->maximumWidthBlockNumber && blockMaximumWidth < d->maximumWidth) {
        // longest line shrinking
        QTextBlock b = doc->firstBlock();
        d->maximumWidth = 0;
        QTextBlock maximumBlock;
        while (b.isValid()) {
            qreal blockMaximumWidth = blockWidth(b);
            if (blockMaximumWidth > d->maximumWidth) {
                d->maximumWidth = blockMaximumWidth;
                maximumBlock = b;
            }
            b = b.next();
        }
        if (maximumBlock.isValid()) {
            d->maximumWidthBlockNumber = maximumBlock.blockNumber();
            emitDocumentSizeChanged = true;
        }
    }
    if (emitDocumentSizeChanged && !d->blockDocumentSizeChanged)
        emit documentSizeChanged(documentSize());
}

qreal PlainTextDocumentLayout::blockWidth(const QTextBlock &block)
{
    QTextLayout *layout = block.layout();
    if (!layout->lineCount())
        return 0; // only for layouted blocks
    qreal blockWidth = 0;
    for (int i = 0; i < layout->lineCount(); ++i) {
        QTextLine line = layout->lineAt(i);
        blockWidth = qMax(line.naturalTextWidth() + 8, blockWidth);
    }
    return blockWidth;
}


PlainTextEditControl::PlainTextEditControl(PlainTextEdit *parent)
    : WidgetTextControl(parent), textEdit(parent),
    topBlock(0)
{
    setAcceptRichText(false);
}

void PlainTextEditPrivate::cursorPositionChanged()
{
    pageUpDownLastCursorYIsValid = false;
#if QT_CONFIG(accessibility)
    QAccessibleTextCursorEvent ev(q, q->textCursor().position());
    QAccessible::updateAccessibility(&ev);
#endif
    emit q->cursorPositionChanged();
}

void PlainTextEditPrivate::verticalScrollbarActionTriggered(int action) {

    const auto a = static_cast<QAbstractSlider::SliderAction>(action);
    switch (a) {
    case QAbstractSlider::SliderPageStepAdd:
        pageUpDown(QTextCursor::Down, QTextCursor::MoveAnchor, false);
        break;
    case QAbstractSlider::SliderPageStepSub:
        pageUpDown(QTextCursor::Up, QTextCursor::MoveAnchor, false);
        break;
    default:
        break;
    }
}

QMimeData *PlainTextEditControl::createMimeDataFromSelection() const {
    PlainTextEdit *ed = qobject_cast<PlainTextEdit *>(parent());
    if (!ed)
        return WidgetTextControl::createMimeDataFromSelection();
    return ed->createMimeDataFromSelection();
}
bool PlainTextEditControl::canInsertFromMimeData(const QMimeData *source) const {
    PlainTextEdit *ed = qobject_cast<PlainTextEdit *>(parent());
    if (!ed)
        return WidgetTextControl::canInsertFromMimeData(source);
    return ed->canInsertFromMimeData(source);
}
void PlainTextEditControl::insertFromMimeData(const QMimeData *source) {
    PlainTextEdit *ed = qobject_cast<PlainTextEdit *>(parent());
    if (!ed)
        WidgetTextControl::insertFromMimeData(source);
    else
        ed->insertFromMimeData(source);
}

qreal PlainTextEditPrivate::verticalOffset(int topBlock, int topLine) const
{
    qreal offset = 0;
    QTextDocument *doc = control->document();

    if (topLine) {
        QTextBlock currentBlock = doc->findBlockByNumber(topBlock);
        PlainTextDocumentLayout *documentLayout = qobject_cast<PlainTextDocumentLayout*>(doc->documentLayout());
        Q_ASSERT(documentLayout);
        QRectF r = documentLayout->blockBoundingRect(currentBlock);
        Q_UNUSED(r)
        QTextLayout *layout = currentBlock.layout();
        if (layout && topLine <= layout->lineCount()) {
            QTextLine line = layout->lineAt(topLine - 1);
            const QRectF lr = line.naturalTextRect();
            offset = lr.bottom();
        }
    }
    if (topBlock == 0 && topLine == 0)
        offset -= doc->documentMargin(); // top margin
    return offset;
}


qreal PlainTextEditPrivate::verticalOffset() const {
    return verticalOffset(control->topBlock, topLine) + topLineOffset;
}


QTextBlock PlainTextEditControl::firstVisibleBlock() const
{
    return document()->findBlockByNumber(topBlock);
}



int PlainTextEditControl::hitTest(const QPointF &point, Qt::HitTestAccuracy ) const {
    int currentBlockNumber = topBlock;
    QTextBlock currentBlock = document()->findBlockByNumber(currentBlockNumber);
    if (!currentBlock.isValid())
        return -1;

    PlainTextDocumentLayout *documentLayout = qobject_cast<PlainTextDocumentLayout*>(document()->documentLayout());
    Q_ASSERT(documentLayout);

    QPointF offset;
    QRectF r = documentLayout->blockBoundingRect(currentBlock);
    while (currentBlock.next().isValid() && r.bottom() + offset.y() <= point.y()) {
        offset.ry() += r.height();
        currentBlock = currentBlock.next();
        ++currentBlockNumber;
        r = documentLayout->blockBoundingRect(currentBlock);
    }
    while (currentBlock.previous().isValid() && r.top() + offset.y() > point.y()) {
        offset.ry() -= r.height();
        currentBlock = currentBlock.previous();
        --currentBlockNumber;
        r = documentLayout->blockBoundingRect(currentBlock);
    }


    if (!currentBlock.isValid())
        return -1;
    QTextLayout *layout = currentBlock.layout();
    int off = 0;
    QPointF pos = point - offset;
    for (int i = 0; i < layout->lineCount(); ++i) {
        QTextLine line = layout->lineAt(i);
        const QRectF lr = line.naturalTextRect();
        if (lr.top() > pos.y()) {
            off = qMin(off, line.textStart());
        } else if (lr.bottom() <= pos.y()) {
            off = qMax(off, line.textStart() + line.textLength());
        } else {
            off = line.xToCursor(pos.x(), overwriteMode() ?
                                              QTextLine::CursorOnCharacter : QTextLine::CursorBetweenCharacters);
            break;
        }
    }

    return currentBlock.position() + off;
}

QRectF PlainTextEditControl::blockBoundingRect(const QTextBlock &block) const {
    int currentBlockNumber = topBlock;
    int blockNumber = block.blockNumber();
    QTextBlock currentBlock = document()->findBlockByNumber(currentBlockNumber);
    if (!currentBlock.isValid())
        return QRectF();
    Q_ASSERT(currentBlock.blockNumber() == currentBlockNumber);
    QTextDocument *doc = document();
    PlainTextDocumentLayout *documentLayout = qobject_cast<PlainTextDocumentLayout*>(doc->documentLayout());
    Q_ASSERT(documentLayout);

    QPointF offset;
    if (!block.isValid())
        return QRectF();
    QRectF r = documentLayout->blockBoundingRect(currentBlock);
    int maxVerticalOffset = r.height();
    while (currentBlockNumber < blockNumber && offset.y() - maxVerticalOffset <= 2* textEdit->viewport()->height()) {
        offset.ry() += r.height();
        currentBlock = currentBlock.next();
        ++currentBlockNumber;
        if (!currentBlock.isVisible()) {
            currentBlock = doc->findBlockByLineNumber(currentBlock.firstLineNumber());
            currentBlockNumber = currentBlock.blockNumber();
        }
        r = documentLayout->blockBoundingRect(currentBlock);
    }
    while (currentBlockNumber > blockNumber && offset.y() + maxVerticalOffset >= -textEdit->viewport()->height()) {
        currentBlock = currentBlock.previous();
        --currentBlockNumber;
        while (!currentBlock.isVisible()) {
            currentBlock = currentBlock.previous();
            --currentBlockNumber;
        }
        if (!currentBlock.isValid())
            break;

        r = documentLayout->blockBoundingRect(currentBlock);
        offset.ry() -= r.height();
    }

    if (currentBlockNumber != blockNumber) {
        // fallback for blocks out of reach. Give it some geometry at
        // least, and ensure the layout is up to date.
        r = documentLayout->blockBoundingRect(block);
        if (currentBlockNumber > blockNumber)
            offset.ry() -= r.height();
    }
    r.translate(offset);
    return r;
}

void PlainTextEditPrivate::setTopLine(int visualTopLine, int dx, int dy)
{
    QTextDocument *doc = control->document();
    QTextBlock block = doc->findBlockByLineNumber(visualTopLine);
    int blockNumber = block.blockNumber();
    int lineNumber = visualTopLine - block.firstLineNumber();
    setTopBlock(blockNumber, lineNumber, dx, dy);
}

void PlainTextEditPrivate::setTopBlock(int blockNumber, int lineNumber, int dx, int dy)
{
    blockNumber = qMax(0, blockNumber);
    lineNumber = qMax(0, lineNumber);
    QTextDocument *doc = control->document();
    QTextBlock block = doc->findBlockByNumber(blockNumber);

    int newTopLine = block.firstLineNumber() + lineNumber;
    int maxTopLine = vbar()->maximum(); // FIXME

    if (newTopLine > maxTopLine) {
        block = doc->findBlockByLineNumber(maxTopLine);
        blockNumber = block.blockNumber();
        lineNumber = maxTopLine - block.firstLineNumber();
    }

    vbar()->setValue(vScrollbarValueForLine(block.firstLineNumber() + lineNumber) + dy);

    if (!dx && dy == topLineOffset && blockNumber == control->topBlock && lineNumber == topLine)
        return;

    if (viewport()->updatesEnabled() && viewport()->isVisible()) {
        if (dy == 0 && doc->findBlockByNumber(control->topBlock).isValid()) {
            qreal realdy = -q->blockBoundingGeometry(block).y()
            + verticalOffset() - verticalOffset(blockNumber, lineNumber);
            topLineOffset = realdy - (int)realdy;
        } else {
            topLineOffset = dy;
        }
        control->topBlock = blockNumber;
        topLine = lineNumber;

        if (dx) {
            viewport()->scroll(q->isRightToLeft() ? -dx : dx, 0);
            QGuiApplication::inputMethod()->update(Qt::ImCursorRectangle | Qt::ImAnchorRectangle);
        } else {
            viewport()->update();
        }
        emit q->updateRequest(viewport()->rect(), topLineOffset);
    } else {
        control->topBlock = blockNumber;
        topLine = lineNumber;
        topLineOffset = dy;
    }
}

void PlainTextEditPrivate::ensureVisible(int position, bool center, bool forceCenter) {
    QRectF visible = QRectF(viewport()->rect()).translated(-q->contentOffset());
    QTextBlock block = control->document()->findBlock(position);
    if (!block.isValid())
        return;
    QRectF br = control->blockBoundingRect(block);
    if (!br.isValid())
        return;
    QTextLine line = block.layout()->lineForTextPosition(position - block.position());
    Q_ASSERT(line.isValid());
    QRectF lr = line.naturalTextRect().translated(br.topLeft());

    if (lr.bottom() >= visible.bottom() || (center && lr.top() < visible.top()) || forceCenter){

        qreal height = visible.height();
        if (center)
            height /= 2;

        qreal h = center ? line.naturalTextRect().center().y() : line.naturalTextRect().bottom();

        QTextBlock previousVisibleBlock = block;
        while (h < height && block.previous().isValid()) {
            previousVisibleBlock = block;
            do {
                block = block.previous();
            } while (!block.isVisible() && block.previous().isValid());
            h += q->blockBoundingRect(block).height();
        }

        int l = 0;
        int lineCount = block.layout()->lineCount();
        qreal voffset = verticalOffset(block.blockNumber(), 0);
        while (l < lineCount) {
            QRectF lineRect = block.layout()->lineAt(l).naturalTextRect();
            if (h - voffset - lineRect.top() <= height)
                break;
            ++l;
        }

        if (l >= lineCount) {
            block = previousVisibleBlock;
            l = 0;
        }
        setTopBlock(block.blockNumber(), l);
    } else if (lr.top() < visible.top()) {
        setTopBlock(block.blockNumber(), line.lineNumber());
    }

}


void PlainTextEditPrivate::updateViewport()
{
    viewport()->update();
    emit q->updateRequest(viewport()->rect(), 0);
}

PlainTextEditPrivate::PlainTextEditPrivate(PlainTextEdit *qq)
    : q(qq)
    , tabChangesFocus(false)
    , showCursorOnInitialShow(false)
    , backgroundVisible(false)
    , centerOnScroll(false)
    , inDrag(false)
    , clickCausedFocus(false)
    , pageUpDownLastCursorYIsValid(false)
    , placeholderTextShown(false)
{
}

void PlainTextEditPrivate::init(const QString &txt)
{
    control = new PlainTextEditControl(q);

    QTextDocument *doc = new QTextDocument(control);
    QAbstractTextDocumentLayout *layout = new PlainTextDocumentLayout(doc);
    doc->setDocumentLayout(layout);
    control->setDocument(doc);

    control->setPalette(q->palette());

    QObject::connect(vbar(), &QAbstractSlider::actionTriggered,
                     this, &PlainTextEditPrivate::verticalScrollbarActionTriggered);
    QObject::connect(control, &WidgetTextControl::microFocusChanged, q,
                     [this](){q->updateMicroFocus(); });
    QObject::connect(control, &WidgetTextControl::documentSizeChanged,
                     this, &PlainTextEditPrivate::adjustScrollbars);
    QObject::connect(control, &WidgetTextControl::blockCountChanged,
                     q, &PlainTextEdit::blockCountChanged);
    QObject::connect(control, &WidgetTextControl::updateRequest,
                     this, &PlainTextEditPrivate::repaintContents);
    QObject::connect(control, &WidgetTextControl::modificationChanged,
                     q, &PlainTextEdit::modificationChanged);
    QObject::connect(control, &WidgetTextControl::textChanged, q, &PlainTextEdit::textChanged);
    QObject::connect(control, &WidgetTextControl::undoAvailable, q, &PlainTextEdit::undoAvailable);
    QObject::connect(control, &WidgetTextControl::redoAvailable, q, &PlainTextEdit::redoAvailable);
    QObject::connect(control, &WidgetTextControl::copyAvailable, q, &PlainTextEdit::copyAvailable);
    QObject::connect(control, &WidgetTextControl::selectionChanged, q, &PlainTextEdit::selectionChanged);
    QObject::connect(control, &WidgetTextControl::cursorPositionChanged,
                     this, &PlainTextEditPrivate::cursorPositionChanged);
    QObject::connect(control, &WidgetTextControl::textChanged,
                     this, &PlainTextEditPrivate::updatePlaceholderVisibility);
    QObject::connect(control, &WidgetTextControl::textChanged, q, [this](){q->updateMicroFocus(); });

    // set a null page size initially to avoid any relayouting until the textedit
    // is shown. relayoutDocument() will take care of setting the page size to the
    // viewport dimensions later.
    doc->setTextWidth(-1);
    doc->documentLayout()->setPaintDevice(viewport());
    doc->setDefaultFont(q->font());

    if (!txt.isEmpty())
        control->setPlainText(txt);

    hbar()->setSingleStep(20);

    viewport()->setBackgroundRole(QPalette::Base);
    q->setAcceptDrops(true);
    q->setFocusPolicy(Qt::StrongFocus);
    q->setAttribute(Qt::WA_KeyCompression);
    q->setAttribute(Qt::WA_InputMethodEnabled);
    q->setInputMethodHints(Qt::ImhMultiLine);

#ifndef QT_NO_CURSOR
    viewport()->setCursor(Qt::IBeamCursor);
#endif
}

void PlainTextEditPrivate::updatePlaceholderVisibility()
{
    // We normally only repaint the part of view that contains text in the
    // document that has changed (in repaintContents). But the placeholder
    // text is not a part of the document, but is drawn on separately. So whenever
    // we either show or hide the placeholder text, we issue a full update.
    if (placeholderTextShown != placeHolderTextToBeShown()) {
        viewport()->update();
        placeholderTextShown = placeHolderTextToBeShown();
    }
}

void PlainTextEditPrivate::repaintContents(const QRectF &contentsRect)
{
    if (!contentsRect.isValid()) {
        updateViewport();
        return;
    }
    const int xOffset = horizontalOffset();
    const int yOffset = (int)verticalOffset();
    const QRect visibleRect(xOffset, yOffset, viewport()->width(), viewport()->height());

    QRect r = contentsRect.adjusted(-1, -1, 1, 1).intersected(visibleRect).toAlignedRect();
    if (r.isEmpty())
        return;

    r.translate(-xOffset, -yOffset);
    viewport()->update(r);
    emit q->updateRequest(r, 0);
}

void PlainTextEditPrivate::pageUpDown(QTextCursor::MoveOperation op, QTextCursor::MoveMode moveMode, bool moveCursor)
{


    QTextCursor cursor = control->textCursor();
    if (moveCursor) {
        ensureCursorVisible();
        if (!pageUpDownLastCursorYIsValid)
            pageUpDownLastCursorY = control->cursorRect(cursor).top() - verticalOffset();
    }

    qreal lastY = pageUpDownLastCursorY;


    if (op == QTextCursor::Down) {
        QRectF visible = QRectF(viewport()->rect()).translated(-q->contentOffset());
        QTextBlock firstVisibleBlock = q->firstVisibleBlock();
        QTextBlock block = firstVisibleBlock;
        QRectF br = q->blockBoundingRect(block);
        qreal h = 0;
        int atEnd = false;
        while (h + br.height() <= visible.bottom()) {
            if (!block.next().isValid()) {
                atEnd = true;
                lastY = visible.bottom(); // set cursor to last line
                break;
            }
            h += br.height();
            block = block.next();
            br = q->blockBoundingRect(block);
        }

        if (!atEnd) {
            int line = 0;
            qreal diff = visible.bottom() - h;
            int lineCount = block.layout()->lineCount();
            while (line < lineCount - 1) {
                if (block.layout()->lineAt(line).naturalTextRect().bottom() > diff) {
                    // the first line that did not completely fit the screen
                    break;
                }
                ++line;
            }
            setTopBlock(block.blockNumber(), line);
        }

        if (moveCursor) {
            // move using movePosition to keep the cursor's x
            lastY += verticalOffset();
            bool moved = false;
            do {
                moved = cursor.movePosition(op, moveMode);
            } while (moved && control->cursorRect(cursor).top() < lastY);
        }

    } else if (op == QTextCursor::Up) {

        QRectF visible = QRectF(viewport()->rect()).translated(-q->contentOffset());
        visible.translate(0, -visible.height()); // previous page
        QTextBlock block = q->firstVisibleBlock();
        qreal h = 0;
        while (h >= visible.top()) {
            if (!block.previous().isValid()) {
                if (control->topBlock == 0 && topLine == 0) {
                    lastY = 0; // set cursor to first line
                }
                break;
            }
            block = block.previous();
            QRectF br = q->blockBoundingRect(block);
            h -= br.height();
        }

        int line = 0;
        if (block.isValid()) {
            qreal diff = visible.top() - h;
            int lineCount = block.layout()->lineCount();
            while (line < lineCount) {
                if (block.layout()->lineAt(line).naturalTextRect().top() >= diff)
                    break;
                ++line;
            }
            if (line == lineCount) {
                if (block.next().isValid() && block.next() != q->firstVisibleBlock()) {
                    block = block.next();
                    line = 0;
                } else {
                    --line;
                }
            }
        }
        setTopBlock(block.blockNumber(), line);

        if (moveCursor) {
            cursor.setVisualNavigation(true);
            // move using movePosition to keep the cursor's x
            lastY += verticalOffset();
            bool moved = false;
            do {
                moved = cursor.movePosition(op, moveMode);
            } while (moved && control->cursorRect(cursor).top() > lastY);
        }
    }

    if (moveCursor) {
        control->setTextCursor(cursor, moveMode == QTextCursor::KeepAnchor);
        pageUpDownLastCursorYIsValid = true;
    }
}

// QFontMetrics::lineSpacing() returns a rounded value for fonts with a franctional line spacing,
// but in the layout process the height is rounded up since eef8a57daf0bf6e6142470db1a08970d5020e07d
// in qtbase.
static int expectedBlockBoundingRectHeight(const QFont &f)
{
    return qCeil(QFontMetricsF(f).lineSpacing());
}

#if QT_CONFIG(scrollbar)

int PlainTextEditPrivate::vScrollbarValueForLine(int topLineNumber)
{
    int value = 0;
    QTextDocument *doc = control->document();
    const int lineSpacing = expectedBlockBoundingRectHeight(doc->defaultFont());
    if (lineWrap == PlainTextEdit::NoWrap)
        return lineSpacing * topLineNumber;
    QTextBlock block = doc->firstBlock();
    auto *documentLayout = qobject_cast<PlainTextDocumentLayout*>(doc->documentLayout());

    int line = 0;

    while (block.isValid()) {
        if (!block.isVisible()) {
            block = block.next();
            continue;
        }

        if (line + block.lineCount() - 1 >= topLineNumber) {
            int end = topLineNumber - line;
            for (int lineNumber = 0; lineNumber < end; ++lineNumber)
                value += block.layout()->lineAt(lineNumber).naturalTextRect().height();
            return value;
        }
        line += block.lineCount();
        if (!block.layout()->lineCount())
            value += lineSpacing;
        else
            value += documentLayout->blockBoundingRect(block).height();
        block = block.next();
    }

    return value;
}

int PlainTextEditPrivate::topLineForVScrollbarValue(int scrollbarValue)
{
    QTextDocument *doc = control->document();
    const int lineSpacing = expectedBlockBoundingRectHeight(doc->defaultFont());
    if (lineWrap == PlainTextEdit::NoWrap)
        return scrollbarValue / lineSpacing;
    QTextBlock block = doc->firstBlock();
    auto *documentLayout = qobject_cast<PlainTextDocumentLayout*>(doc->documentLayout());

    int value = 0;
    while (block.isValid()) {
        if (!block.isVisible()) {
            block = block.next();
            continue;
        }

        int blockLayoutValid = block.layout()->lineCount();
        value += blockLayoutValid ? documentLayout->blockBoundingRect(block).height() : lineSpacing;
        if (value > scrollbarValue) {
            if (blockLayoutValid) {
                for (int line = block.layout()->lineCount() - 1; line >= 0; --line) {
                    value -= block.layout()->lineAt(line).naturalTextRect().height();
                    if (value < scrollbarValue)
                        return block.firstLineNumber() + line;
                }
            }
            return block.firstLineNumber();
        }
        block = block.next();
    }

    return doc->lastBlock().firstLineNumber() + doc->lastBlock().lineCount() - 1;
}

void PlainTextEditPrivate::adjustScrollbars()
{
    QTextDocument *doc = control->document();
    const int lineSpacing = expectedBlockBoundingRectHeight(doc->defaultFont());
    PlainTextDocumentLayout *documentLayout = qobject_cast<PlainTextDocumentLayout*>(doc->documentLayout());
    Q_ASSERT(documentLayout);
    bool documentSizeChangedBlocked = documentLayout->d->blockDocumentSizeChanged;
    documentLayout->d->blockDocumentSizeChanged = true;

    int vmax = 2 * doc->documentMargin();
    QTextBlock block = doc->firstBlock();
    while (block.isValid()) {
        if (!block.isVisible()) {
            block = block.next();
            continue;
        }
        if (!block.layout()->lineCount())
            vmax += lineSpacing;
        else
            vmax += documentLayout->blockBoundingRect(block).height();
        block = block.next();
    }
    if (!centerOnScroll)
        vmax -= qMax(0, viewport()->height());
    QSizeF documentSize = documentLayout->documentSize();
    vbar()->setRange(0, qMax(0, vmax));
    vbar()->setPageStep(viewport()->height());
    int visualTopLine = vmax;
    QTextBlock firstVisibleBlock = q->firstVisibleBlock();
    if (firstVisibleBlock.isValid())
        visualTopLine = firstVisibleBlock.firstLineNumber() + topLine;

    vbar()->setValue(vScrollbarValueForLine(visualTopLine));
    vbar()->setSingleStep(expectedBlockBoundingRectHeight(doc->defaultFont()));

    hbar()->setRange(0, (int)documentSize.width() - viewport()->width());
    hbar()->setPageStep(viewport()->width());
    documentLayout->d->blockDocumentSizeChanged = documentSizeChangedBlocked;
    setTopLine(visualTopLine);
}

#endif


void PlainTextEditPrivate::ensureViewportLayouted()
{
}

/*!
    \class Utils::PlainTextEdit
    \inmodule QtCreator

    \brief The PlainTextEdit class provides a widget that is used to edit and display
    plain text.

    PlainTextEdit is an advanced viewer/editor supporting plain
    text. It is optimized to handle large documents and to respond
    quickly to user input.

    QPlainText uses very much the same technology and concepts as
    QTextEdit, but is optimized for plain text handling.

    PlainTextEdit works on paragraphs and characters. A paragraph is
    a formatted string which is word-wrapped to fit into the width of
    the widget. By default when reading plain text, one newline
    signifies a paragraph. A document consists of zero or more
    paragraphs. Paragraphs are separated by hard line breaks. Each
    character within a paragraph has its own attributes, for example,
    font and color.

    The shape of the mouse cursor on a PlainTextEdit is
    Qt::IBeamCursor by default.  It can be changed through the
    \c viewport() cursor property.

    \section1 Using PlainTextEdit as a Display Widget

    The text is set or replaced using setPlainText() which deletes the
    existing text and replaces it with the text passed to setPlainText().

    Text can be inserted using the QTextCursor class or using the
    convenience functions insertPlainText(), appendPlainText() or
    paste().

    By default, the text edit wraps words at whitespace to fit within
    the text edit widget. The setLineWrapMode() function is used to
    specify the kind of line wrap you want, \l WidgetWidth or \l
    NoWrap if you don't want any wrapping.  If you use word wrap to
    the widget's width \l WidgetWidth, you can specify whether to
    break on whitespace or anywhere with setWordWrapMode().

    The \c find() function can be used to find and select a given string
    within the text.

    If you want to limit the total number of paragraphs in a
    PlainTextEdit, as it is for example useful in a log viewer, then
    you can use the maximumBlockCount property. The combination of
    setMaximumBlockCount() and appendPlainText() turns PlainTextEdit
    into an efficient viewer for log text. The scrolling can be
    reduced with the centerOnScroll() property, making the log viewer
    even faster. Text can be formatted in a limited way, either using
    a syntax highlighter (see below), or by appending html-formatted
    text with appendHtml(). While PlainTextEdit does not support
    complex rich text rendering with tables and floats, it does
    support limited paragraph-based formatting that you may need in a
    log viewer.

    \section2 Read-only Key Bindings

    When PlainTextEdit is used read-only the key bindings are limited to
    navigation, and text may only be selected with the mouse:
    \table
    \header \li Keypresses \li Action
    \row \li Qt::UpArrow        \li Moves one line up.
    \row \li Qt::DownArrow        \li Moves one line down.
    \row \li Qt::LeftArrow        \li Moves one character to the left.
    \row \li Qt::RightArrow        \li Moves one character to the right.
    \row \li PageUp        \li Moves one (viewport) page up.
    \row \li PageDown        \li Moves one (viewport) page down.
    \row \li Home        \li Moves to the beginning of the text.
    \row \li End                \li Moves to the end of the text.
    \row \li Alt+Wheel
         \li Scrolls the page horizontally (the Wheel is the mouse wheel).
    \row \li Ctrl+Wheel        \li Zooms the text.
    \row \li Ctrl+A            \li Selects all text.
    \endtable


    \section1 Using PlainTextEdit as an Editor

    All the information about using PlainTextEdit as a display widget also
    applies here.

    Selection of text is handled by the QTextCursor class, which provides
    functionality for creating selections, retrieving the text contents or
    deleting selections. You can retrieve the object that corresponds with
    the user-visible cursor using the textCursor() method. If you want to set
    a selection in PlainTextEdit just create one on a QTextCursor object and
    then make that cursor the visible cursor using \c setCursor(). The selection
    can be copied to the clipboard with copy(), or cut to the clipboard with
    cut(). The entire text can be selected using selectAll().

    PlainTextEdit holds a QTextDocument object which can be retrieved using the
    document() method. You can also set your own document object using setDocument().
    QTextDocument emits a textChanged() signal if the text changes and it also
    provides an \c isModified() function which will return \c true if the text has been
    modified since it was either loaded or since the last call to setModified
    with \c false as argument. In addition it provides methods for undo and redo.

    \section2 Syntax Highlighting

    Just like QTextEdit, PlainTextEdit works together with
    QSyntaxHighlighter.

    \section2 Editing Key Bindings

    The list of key bindings which are implemented for editing:
    \table
    \header \li Keypresses \li Action
    \row \li Backspace \li Deletes the character to the left of the cursor.
    \row \li Delete \li Deletes the character to the right of the cursor.
    \row \li Ctrl+C \li Copy the selected text to the clipboard.
    \row \li Ctrl+Insert \li Copy the selected text to the clipboard.
    \row \li Ctrl+K \li Deletes to the end of the line.
    \row \li Ctrl+V \li Pastes the clipboard text into text edit.
    \row \li Shift+Insert \li Pastes the clipboard text into text edit.
    \row \li Ctrl+X \li Deletes the selected text and copies it to the clipboard.
    \row \li Shift+Delete \li Deletes the selected text and copies it to the clipboard.
    \row \li Ctrl+Z \li Undoes the last operation.
    \row \li Ctrl+Y \li Redoes the last operation.
    \row \li LeftArrow \li Moves the cursor one character to the left.
    \row \li Ctrl+LeftArrow \li Moves the cursor one word to the left.
    \row \li RightArrow \li Moves the cursor one character to the right.
    \row \li Ctrl+RightArrow \li Moves the cursor one word to the right.
    \row \li UpArrow \li Moves the cursor one line up.
    \row \li Ctrl+UpArrow \li Moves the cursor one word up.
    \row \li DownArrow \li Moves the cursor one line down.
    \row \li Ctrl+Down Arrow \li Moves the cursor one word down.
    \row \li PageUp \li Moves the cursor one page up.
    \row \li PageDown \li Moves the cursor one page down.
    \row \li Home \li Moves the cursor to the beginning of the line.
    \row \li Ctrl+Home \li Moves the cursor to the beginning of the text.
    \row \li End \li Moves the cursor to the end of the line.
    \row \li Ctrl+End \li Moves the cursor to the end of the text.
    \row \li Alt+Wheel \li Scrolls the page horizontally (the Wheel is the mouse wheel).
    \row \li Ctrl+Wheel \li Zooms the text.
    \endtable

    To select (mark) text hold down the Shift key whilst pressing one
    of the movement keystrokes, for example, \e{Shift+Right Arrow}
    will select the character to the right, and \e{Shift+Ctrl+Right
    Arrow} will select the word to the right, etc.

   \section1 Differences to QTextEdit

   PlainTextEdit is a thin class, implemented by using most of the
   technology that is behind QTextEdit and QTextDocument. Its
   performance benefits over QTextEdit stem mostly from using a
   different and simplified text layout called
   PlainTextDocumentLayout on the text document (see
   QTextDocument::setDocumentLayout()). The plain text document layout
   does not support tables nor embedded frames, and \e{replaces a
   pixel-exact height calculation with a line-by-line respectively
   paragraph-by-paragraph scrolling approach}. This makes it possible
   to handle significantly larger documents, and still resize the
   editor with line wrap enabled in real time. It also makes for a
   fast log viewer (see setMaximumBlockCount()).

    \sa {QTextDocument}, {QTextCursor}, {Syntax Highlighter Example},
    {Rich Text Processing}

*/

/*!
    \property Utils::PlainTextEdit::plainText

    This property gets and sets the plain text editor's contents. The previous
    contents are removed and undo/redo history is reset when this property is set.
    currentCharFormat() is also reset, unless textCursor() is already at the
    beginning of the document.

    By default, for an editor with no contents, this property contains an empty string.
*/

/*!
    \property Utils::PlainTextEdit::undoRedoEnabled
    \brief whether undo and redo are enabled

    Users are only able to undo or redo actions if this property is
    \c true, and if there is an action that can be undone (or redone).

    By default, this property is \c true.
*/

/*!
    \enum Utils::PlainTextEdit::LineWrapMode

    Holds the line wrap mode.

    \value NoWrap
    \value WidgetWidth
*/


/*!
    Constructs an empty PlainTextEdit with parent \a
    parent.
*/
PlainTextEdit::PlainTextEdit(QWidget *parent)
    : QAbstractScrollArea(parent)
    , d(std::make_unique<PlainTextEditPrivate>(this))
{
    d->init();
}

/*!
    \internal
    // Note: This constructor is unused, as we cannot forward the private "dd" argument 
PlainTextEdit::PlainTextEdit(PlainTextEditPrivate &dd, QWidget *parent)
    : QAbstractScrollArea(parent)
    , d(std::make_unique<PlainTextEditPrivate>(this))
{
    d->init();
}
*/

/*!
    Constructs a PlainTextEdit with parent \a parent. The text edit will display
    the plain text \a text.
*/
PlainTextEdit::PlainTextEdit(const QString &text, QWidget *parent)
    : QAbstractScrollArea(parent)
    , d(std::make_unique<PlainTextEditPrivate>(this))
{
    d->init(text);
}


/*!
    Destructor.
*/
PlainTextEdit::~PlainTextEdit()
{
    if (d->documentLayoutPtr) {
        if (d->documentLayoutPtr->d->mainViewPrivate == d.get())
            d->documentLayoutPtr->d->mainViewPrivate = nullptr;
    }
}

/*!
    Makes \a document the new document of the text editor.

    The parent QObject of the provided document remains the owner
    of the object. If the current document is a child of the text
    editor, then it is deleted.

    The document must have a document layout that inherits
    PlainTextDocumentLayout (see QTextDocument::setDocumentLayout()).

    \sa document()
*/
void PlainTextEdit::setDocument(QTextDocument *document)
{
    PlainTextDocumentLayout *documentLayout = nullptr;

    if (!document) {
        document = new QTextDocument(d->control);
        documentLayout = new PlainTextDocumentLayout(document);
        document->setDocumentLayout(documentLayout);
    } else {
        documentLayout = qobject_cast<PlainTextDocumentLayout*>(document->documentLayout());
        if (Q_UNLIKELY(!documentLayout)) {
            qWarning("PlainTextEdit::setDocument: Document set does not support PlainTextDocumentLayout");
            return;
        }
    }
    d->control->setDocument(document);
    if (!documentLayout->d->mainViewPrivate)
        documentLayout->d->mainViewPrivate = d.get();
    d->documentLayoutPtr = documentLayout;
    d->updateDefaultTextOption();
    d->relayoutDocument();
    d->adjustScrollbars();
}

/*!
    Returns a pointer to the underlying document.

    \sa setDocument()
*/
QTextDocument *PlainTextEdit::document() const
{
    return d->control->document();
}

/*!
    \property Utils::PlainTextEdit::placeholderText
    \brief the editor placeholder text

    Setting this property makes the editor display a grayed-out
    placeholder text as long as the document() is empty.

    By default, this property contains an empty string.

    \sa document()
*/
void PlainTextEdit::setPlaceholderText(const QString &placeholderText)
{
    if (d->placeholderText != placeholderText) {
        d->placeholderText = placeholderText;
        d->updatePlaceholderVisibility();
    }
}

QString PlainTextEdit::placeholderText() const
{
    return d->placeholderText;
}

/*!
    Sets the visible \a cursor.
*/
void PlainTextEdit::setTextCursor(const QTextCursor &cursor)
{
    doSetTextCursor(cursor);
}

/*!
    \internal

     This provides a hook for subclasses to intercept cursor changes.
*/

void PlainTextEdit::doSetTextCursor(const QTextCursor &cursor)
{
    d->control->setTextCursor(cursor);
}

/*!
    Returns a copy of the QTextCursor that represents the currently visible cursor.
    Note that changes on the returned cursor do not affect PlainTextEdit's cursor; use
    setTextCursor() to update the visible cursor.
 */
QTextCursor PlainTextEdit::textCursor() const
{
    return d->control->textCursor();
}

QVariant PlainTextEdit::variantTextCursor() const // needed for testing with Squish
{
    return QVariant::fromValue(textCursor());
}

/*!
    Returns the reference of the anchor at position \a pos, or an
    empty string if no anchor exists at that point.
 */
QString PlainTextEdit::anchorAt(const QPoint &pos) const
{
    Q_UNUSED(pos);
    return QString();
}

/*!
    Undoes the last operation.

    If there is no operation to undo, i.e. there is no undo step in
    the undo/redo history, nothing happens.

    \sa redo()
*/
void PlainTextEdit::undo()
{
    d->control->undo();
}

void PlainTextEdit::redo()
{
    d->control->redo();
}

/*!
    \fn void PlainTextEdit::redo()

    Redoes the last operation.

    If there is no operation to redo, i.e. there is no redo step in
    the undo/redo history, nothing happens.

    \sa undo()
*/

#ifndef QT_NO_CLIPBOARD
/*!
    Copies the selected text to the clipboard and deletes it from
    the text edit.

    If there is no selected text nothing happens.

    \sa copy(), paste()
*/

void PlainTextEdit::cut()
{
    d->control->cut();
}

/*!
    Copies any selected text to the clipboard.

    \sa copyAvailable()
*/

void PlainTextEdit::copy()
{
    d->control->copy();
}

/*!
    Pastes the text from the clipboard into the text edit at the
    current cursor position.

    If there is no text in the clipboard nothing happens.

    To change the behavior of this function, i.e. to modify what
    PlainTextEdit can paste and how it is being pasted, reimplement the
    virtual canInsertFromMimeData() and insertFromMimeData()
    functions.

    \sa cut(), copy()
*/

void PlainTextEdit::paste()
{
    d->control->paste();
}
#endif

/*!
    Deletes all the text in the text edit.

    Notes:
    \list
    \li The undo/redo history is also cleared.
    \li currentCharFormat() is reset, unless textCursor()
    is already at the beginning of the document.
    \endlist

    \sa cut(), setPlainText()
*/
void PlainTextEdit::clear()
{
    // clears and sets empty content
    d->control->topBlock = d->topLine = d->topLineOffset = 0;
    d->control->clear();
}


/*!
    Selects all text.

    \sa copy(), cut(), textCursor()
 */
void PlainTextEdit::selectAll()
{
    d->control->selectAll();
}

/*! \internal
*/
bool PlainTextEdit::event(QEvent *e)
{

    switch (e->type()) {
#ifndef QT_NO_CONTEXTMENU
    case QEvent::ContextMenu:
        if (static_cast<QContextMenuEvent *>(e)->reason() == QContextMenuEvent::Keyboard) {
            ensureCursorVisible();
            const QPoint cursorPos = cursorRect().center();
            QContextMenuEvent ce(QContextMenuEvent::Keyboard, cursorPos, viewport()->mapToGlobal(cursorPos));
            ce.setAccepted(e->isAccepted());
            const bool result = QAbstractScrollArea::event(&ce);
            e->setAccepted(ce.isAccepted());
            return result;
        }
        break;
#endif // QT_NO_CONTEXTMENU
    case QEvent::ShortcutOverride:
    case QEvent::ToolTip:
        d->sendControlEvent(e);
        break;
#ifdef QT_KEYPAD_NAVIGATION
    case QEvent::EnterEditFocus:
    case QEvent::LeaveEditFocus:
        if (QApplicationPrivate::keypadNavigationEnabled())
            d->sendControlEvent(e);
        break;
#endif
#ifndef QT_NO_GESTURES
    case QEvent::Gesture:
        if (auto *g = static_cast<QGestureEvent *>(e)->gesture(Qt::PanGesture)) {
            QPanGesture *panGesture = static_cast<QPanGesture *>(g);
            QScrollBar *hBar = horizontalScrollBar();
            QScrollBar *vBar = verticalScrollBar();
            if (panGesture->state() == Qt::GestureStarted)
                d->originalOffsetY = vBar->value();
            QPointF offset = panGesture->offset();
            if (!offset.isNull()) {
                if (QGuiApplication::isRightToLeft())
                    offset.rx() *= -1;
                // PlainTextEdit scrolls by lines only in vertical direction
                QFontMetrics fm(document()->defaultFont());
                int lineHeight = fm.height();
                int newX = hBar->value() - panGesture->delta().x();
                int newY = d->originalOffsetY - offset.y()/lineHeight;
                hBar->setValue(newX);
                vBar->setValue(newY);
            }
        }
        return true;
#endif // QT_NO_GESTURES
    case QEvent::WindowActivate:
    case QEvent::WindowDeactivate:
        d->control->setPalette(palette());
        break;
    default:
        break;
    }
    return QAbstractScrollArea::event(e);
}

/*! \internal
*/

void PlainTextEdit::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == d->autoScrollTimer.timerId()) {
        QRect visible = viewport()->rect();
        QPoint pos;
        if (d->inDrag) {
            pos = d->autoScrollDragPos;
            visible.adjust(qMin(visible.width()/3,20), qMin(visible.height()/3,20),
                           -qMin(visible.width()/3,20), -qMin(visible.height()/3,20));
        } else {
            const QPoint globalPos = QCursor::pos();
            pos = viewport()->mapFromGlobal(globalPos);
            QMouseEvent ev(QEvent::MouseMove, pos, viewport()->mapTo(viewport()->topLevelWidget(), pos), globalPos,
                           Qt::LeftButton, Qt::LeftButton, QGuiApplication::keyboardModifiers());
            mouseMoveEvent(&ev);
        }
        int deltaY = qMax(pos.y() - visible.top(), visible.bottom() - pos.y()) - visible.height();
        int deltaX = qMax(pos.x() - visible.left(), visible.right() - pos.x()) - visible.width();
        int delta = qMax(deltaX, deltaY);
        if (delta >= 0) {
            if (delta < 7)
                delta = 7;
            int timeout = 4900 / (delta * delta);
            d->autoScrollTimer.start(timeout, this);

            if (deltaY > 0)
                d->vbar()->triggerAction(pos.y() < visible.center().y() ?
                                           QAbstractSlider::SliderSingleStepSub
                                                                      : QAbstractSlider::SliderSingleStepAdd);
            if (deltaX > 0)
                d->hbar()->triggerAction(pos.x() < visible.center().x() ?
                                           QAbstractSlider::SliderSingleStepSub
                                                                      : QAbstractSlider::SliderSingleStepAdd);
        }
    }
#ifdef QT_KEYPAD_NAVIGATION
    else if (e->timerId() == d->deleteAllTimer.timerId()) {
        d->deleteAllTimer.stop();
        clear();
    }
#endif
}

/*!
    Changes the text of the text edit to the string \a text.
    Any previous text is removed.

    \a text is interpreted as plain text.

    Notes:
    \list
    \li The undo/redo history is also cleared.
    \li currentCharFormat() is reset, unless textCursor()
    is already at the beginning of the document.
    \endlist

    \sa toPlainText()
*/

void PlainTextEdit::setPlainText(const QString &text)
{
    d->control->setPlainText(text);
}

/*!
    \fn QString PlainTextEdit::toPlainText() const

    Returns the text of the text edit as plain text.

    \sa PlainTextEdit::setPlainText()
 */

/*!
    \overload QTextEdit::keyPressEvent()
*/
void PlainTextEdit::keyPressEvent(QKeyEvent *e)
{

#ifdef QT_KEYPAD_NAVIGATION
    switch (e->key()) {
    case Qt::Key_Select:
        if (QApplicationPrivate::keypadNavigationEnabled()) {
            if (!(d->control->textInteractionFlags() & Qt::LinksAccessibleByKeyboard))
                setEditFocus(!hasEditFocus());
            else {
                if (!hasEditFocus())
                    setEditFocus(true);
                else {
                    QTextCursor cursor = d->control->textCursor();
                    QTextCharFormat charFmt = cursor.charFormat();
                    if (!cursor.hasSelection() || charFmt.anchorHref().isEmpty()) {
                        setEditFocus(false);
                    }
                }
            }
        }
        break;
    case Qt::Key_Back:
    case Qt::Key_No:
        if (!QApplicationPrivate::keypadNavigationEnabled() || !hasEditFocus()) {
            e->ignore();
            return;
        }
        break;
    default:
        if (QApplicationPrivate::keypadNavigationEnabled()) {
            if (!hasEditFocus() && !(e->modifiers() & Qt::ControlModifier)) {
                if (e->text()[0].isPrint()) {
                    setEditFocus(true);
                    clear();
                } else {
                    e->ignore();
                    return;
                }
            }
        }
        break;
    }
#endif

#ifndef QT_NO_SHORTCUT

    Qt::TextInteractionFlags tif = d->control->textInteractionFlags();

    if (tif & Qt::TextSelectableByKeyboard){
        if (e == QKeySequence::SelectPreviousPage) {
            e->accept();
            d->pageUpDown(QTextCursor::Up, QTextCursor::KeepAnchor);
            return;
        } else if (e ==QKeySequence::SelectNextPage) {
            e->accept();
            d->pageUpDown(QTextCursor::Down, QTextCursor::KeepAnchor);
            return;
        }
    }
    if (tif & (Qt::TextSelectableByKeyboard | Qt::TextEditable)) {
        if (e == QKeySequence::MoveToPreviousPage) {
            e->accept();
            d->pageUpDown(QTextCursor::Up, QTextCursor::MoveAnchor);
            return;
        } else if (e == QKeySequence::MoveToNextPage) {
            e->accept();
            d->pageUpDown(QTextCursor::Down, QTextCursor::MoveAnchor);
            return;
        }
    }

    if (!(tif & Qt::TextEditable)) {
        switch (e->key()) {
        case Qt::Key_Space:
            e->accept();
            if (e->modifiers() & Qt::ShiftModifier)
                d->vbar()->triggerAction(QAbstractSlider::SliderPageStepSub);
            else
                d->vbar()->triggerAction(QAbstractSlider::SliderPageStepAdd);
            break;
        default:
            d->sendControlEvent(e);
            if (!e->isAccepted() && e->modifiers() == Qt::NoModifier) {
                if (e->key() == Qt::Key_Home) {
                    d->vbar()->triggerAction(QAbstractSlider::SliderToMinimum);
                    e->accept();
                } else if (e->key() == Qt::Key_End) {
                    d->vbar()->triggerAction(QAbstractSlider::SliderToMaximum);
                    e->accept();
                }
            }
            if (!e->isAccepted()) {
                QAbstractScrollArea::keyPressEvent(e);
            }
        }
        return;
    }
#endif // QT_NO_SHORTCUT

    d->sendControlEvent(e);
#ifdef QT_KEYPAD_NAVIGATION
    if (!e->isAccepted()) {
        switch (e->key()) {
        case Qt::Key_Up:
        case Qt::Key_Down:
            if (QApplicationPrivate::keypadNavigationEnabled()) {
                // Cursor position didn't change, so we want to leave
                // these keys to change focus.
                e->ignore();
                return;
            }
            break;
        case Qt::Key_Left:
        case Qt::Key_Right:
            if (QApplicationPrivate::keypadNavigationEnabled()
                && QApplication::navigationMode() == Qt::NavigationModeKeypadDirectional) {
                // Same as for Key_Up and Key_Down.
                e->ignore();
                return;
            }
            break;
        case Qt::Key_Back:
            if (!e->isAutoRepeat()) {
                if (QApplicationPrivate::keypadNavigationEnabled()) {
                    if (document()->isEmpty()) {
                        setEditFocus(false);
                        e->accept();
                    } else if (!d->deleteAllTimer.isActive()) {
                        e->accept();
                        d->deleteAllTimer.start(750, this);
                    }
                } else {
                    e->ignore();
                    return;
                }
            }
            break;
        default: break;
        }
    }
#endif
}

/*!
    \overload QTextEdit::keyReleaseEvent()
*/
void PlainTextEdit::keyReleaseEvent(QKeyEvent *e)
{
    if (!isReadOnly())
        d->handleSoftwareInputPanel();

#ifdef QT_KEYPAD_NAVIGATION
    if (QApplicationPrivate::keypadNavigationEnabled()) {
        if (!e->isAutoRepeat() && e->key() == Qt::Key_Back
            && d->deleteAllTimer.isActive()) {
            d->deleteAllTimer.stop();
            QTextCursor cursor = d->control->textCursor();
            QTextBlockFormat blockFmt = cursor.blockFormat();

            QTextList *list = cursor.currentList();
            if (list && cursor.atBlockStart()) {
                list->remove(cursor.block());
            } else if (cursor.atBlockStart() && blockFmt.indent() > 0) {
                blockFmt.setIndent(blockFmt.indent() - 1);
                cursor.setBlockFormat(blockFmt);
            } else {
                cursor.deletePreviousChar();
            }
            setTextCursor(cursor);
        }
    }
#else
    QWidget::keyReleaseEvent(e);
#endif
}

/*!
    Loads the resource specified by the given \a type and \a name.

    This function is an extension of QTextDocument::loadResource().

    \sa QTextDocument::loadResource()
*/
QVariant PlainTextEdit::loadResource(int type, const QUrl &name)
{
    Q_UNUSED(type)
    Q_UNUSED(name)
    return QVariant();
}

void PlainTextEdit::resizeEvent(QResizeEvent *e)
{
    if (e->oldSize().width() != e->size().width())
        d->relayoutDocument();
    d->adjustScrollbars();
}

void PlainTextEditPrivate::relayoutDocument()
{
    if (!documentLayoutPtr)
        return;

    int width = viewport()->width();

    if (documentLayoutPtr->d->mainViewPrivate == nullptr
        || documentLayoutPtr->d->mainViewPrivate == this
        || width > documentLayoutPtr->textWidth()) {
        documentLayoutPtr->d->mainViewPrivate = this;
        documentLayoutPtr->setTextWidth(width);
    }
}

static void fillBackground(QPainter *p, const QRectF &rect, QBrush brush, const QRectF &gradientRect = QRectF())
{
    p->save();
    if (brush.style() >= Qt::LinearGradientPattern && brush.style() <= Qt::ConicalGradientPattern) {
        if (!gradientRect.isNull()) {
            QTransform m = QTransform::fromTranslate(gradientRect.left(), gradientRect.top());
            m.scale(gradientRect.width(), gradientRect.height());
            brush.setTransform(m);
            const_cast<QGradient *>(brush.gradient())->setCoordinateMode(QGradient::LogicalMode);
        }
    } else {
        p->setBrushOrigin(rect.topLeft());
    }
    p->fillRect(rect, brush);
    p->restore();
}



/*!
    \overload QTextEdit::paintEvent()
*/
void PlainTextEdit::paintEvent(QPaintEvent *e)
{
    QPainter painter(viewport());
    Q_ASSERT(qobject_cast<PlainTextDocumentLayout*>(document()->documentLayout()));

    QPointF offset(contentOffset());

    QRect er = e->rect();
    QRect viewportRect = viewport()->rect();

    bool editable = !isReadOnly();

    QTextBlock block = firstVisibleBlock();
    qreal maximumWidth = document()->documentLayout()->documentSize().width();

    // Set a brush origin so that the WaveUnderline knows where the wave started
    painter.setBrushOrigin(offset);

    // keep right margin clean from full-width selection
    int maxX = offset.x() + qMax((qreal)viewportRect.width(), maximumWidth)
               - document()->documentMargin() + cursorWidth();
    er.setRight(qMin(er.right(), maxX));
    painter.setClipRect(er);

    if (d->placeHolderTextToBeShown()) {
        const QColor col = d->control->palette().placeholderText().color();
        painter.setPen(col);
        painter.setClipRect(e->rect());
        const int margin = int(document()->documentMargin());
        QRectF textRect = viewportRect.adjusted(margin, margin, 0, 0);
        painter.drawText(textRect, Qt::AlignTop | Qt::TextWordWrap, placeholderText());
    }

    QAbstractTextDocumentLayout::PaintContext context = getPaintContext();
    painter.setPen(context.palette.text().color());

    while (block.isValid()) {

        QRectF r = blockBoundingRect(block).translated(offset);
        QTextLayout *layout = block.layout();

        if (!block.isVisible()) {
            offset.ry() += r.height();
            block = block.next();
            continue;
        }

        if (r.bottom() >= er.top() && r.top() <= er.bottom()) {

            QTextBlockFormat blockFormat = block.blockFormat();

            QBrush bg = blockFormat.background();
            if (bg != Qt::NoBrush) {
                QRectF contentsRect = r;
                contentsRect.setWidth(qMax(r.width(), maximumWidth));
                fillBackground(&painter, contentsRect, bg);
            }

            QList<QTextLayout::FormatRange> selections;
            int blpos = block.position();
            int bllen = block.length();
            for (int i = 0; i < context.selections.size(); ++i) {
                const QAbstractTextDocumentLayout::Selection &range = context.selections.at(i);
                const int selStart = range.cursor.selectionStart() - blpos;
                const int selEnd = range.cursor.selectionEnd() - blpos;
                if (selStart < bllen && selEnd > 0
                    && selEnd > selStart) {
                    QTextLayout::FormatRange o;
                    o.start = selStart;
                    o.length = selEnd - selStart;
                    o.format = range.format;
                    selections.append(o);
                } else if (!range.cursor.hasSelection() && range.format.hasProperty(QTextFormat::FullWidthSelection)
                           && block.contains(range.cursor.position())) {
                    // for full width selections we don't require an actual selection, just
                    // a position to specify the line. that's more convenience in usage.
                    QTextLayout::FormatRange o;
                    QTextLine l = layout->lineForTextPosition(range.cursor.position() - blpos);
                    o.start = l.textStart();
                    o.length = l.textLength();
                    if (o.start + o.length == bllen - 1)
                        ++o.length; // include newline
                    o.format = range.format;
                    selections.append(o);
                }
            }

            bool drawCursor = ((editable || (textInteractionFlags() & Qt::TextSelectableByKeyboard))
                               && context.cursorPosition >= blpos
                               && context.cursorPosition < blpos + bllen);

            bool drawCursorAsBlock = drawCursor && overwriteMode() ;

            if (drawCursorAsBlock) {
                if (context.cursorPosition == blpos + bllen - 1) {
                    drawCursorAsBlock = false;
                } else {
                    QTextLayout::FormatRange o;
                    o.start = context.cursorPosition - blpos;
                    o.length = 1;
                    o.format.setForeground(palette().base());
                    o.format.setBackground(palette().text());
                    selections.append(o);
                }
            }

            layout->draw(&painter, offset, selections, er);

            if ((drawCursor && !drawCursorAsBlock)
                || (editable && context.cursorPosition < -1
                    && !layout->preeditAreaText().isEmpty())) {
                int cpos = context.cursorPosition;
                if (cpos < -1)
                    cpos = layout->preeditAreaPosition() - (cpos + 2);
                else
                    cpos -= blpos;
                layout->drawCursor(&painter, offset, cpos, cursorWidth());
            }
        }

        offset.ry() += r.height();
        if (offset.y() > viewportRect.height())
            break;
        block = block.next();
    }

    if (backgroundVisible() && !block.isValid() && offset.y() <= er.bottom()
        && (centerOnScroll() || verticalScrollBar()->maximum() == verticalScrollBar()->minimum())) {
        painter.fillRect(QRect(QPoint((int)er.left(), (int)offset.y()), er.bottomRight()), palette().window());
    }
}


void PlainTextEditPrivate::updateDefaultTextOption()
{
    QTextDocument *doc = control->document();

    QTextOption opt = doc->defaultTextOption();
    QTextOption::WrapMode oldWrapMode = opt.wrapMode();

    if (lineWrap == PlainTextEdit::NoWrap)
        opt.setWrapMode(QTextOption::NoWrap);
    else
        opt.setWrapMode(wordWrap);

    if (opt.wrapMode() != oldWrapMode)
        doc->setDefaultTextOption(opt);
}


/*!
    \overload QTextEdit::mousePressEvent()
*/
void PlainTextEdit::mousePressEvent(QMouseEvent *e)
{
#ifdef QT_KEYPAD_NAVIGATION
    if (QApplicationPrivate::keypadNavigationEnabled() && !hasEditFocus())
        setEditFocus(true);
#endif
    d->sendControlEvent(e);
}

/*!
    \overload QTextEdit::mouseMoveEvent()
*/
void PlainTextEdit::mouseMoveEvent(QMouseEvent *e)
{
    d->inDrag = false; // paranoia
    const QPoint pos = e->position().toPoint();
    d->sendControlEvent(e);
    if (!(e->buttons() & Qt::LeftButton))
        return;
    if (e->source() == Qt::MouseEventNotSynthesized) {
        const QRect visible = viewport()->rect();
        if (visible.contains(pos))
            d->autoScrollTimer.stop();
        else if (!d->autoScrollTimer.isActive())
            d->autoScrollTimer.start(100, this);
    }
}

/*!
    \overload QTextEdit::mouseReleaseEvent()
*/
void PlainTextEdit::mouseReleaseEvent(QMouseEvent *e)
{
    d->sendControlEvent(e);
    if (e->source() == Qt::MouseEventNotSynthesized && d->autoScrollTimer.isActive()) {
        d->autoScrollTimer.stop();
        d->ensureCursorVisible();
    }

    if (!isReadOnly() && rect().contains(e->position().toPoint()))
        d->handleSoftwareInputPanel(e->button(), d->clickCausedFocus);
    d->clickCausedFocus = 0;
}

/*!
    \overload QTextEdit::mouseDoubleClickEvent()
*/
void PlainTextEdit::mouseDoubleClickEvent(QMouseEvent *e)
{
    d->sendControlEvent(e);
}

/*!
    \overload QWidget::focusNextPrevChild()
*/
bool PlainTextEdit::focusNextPrevChild(bool next)
{
    if (!d->tabChangesFocus && d->control->textInteractionFlags() & Qt::TextEditable)
        return false;
    return QAbstractScrollArea::focusNextPrevChild(next);
}

#ifndef QT_NO_CONTEXTMENU
/*!
  \internal

  Shows the standard context menu created with createStandardContextMenu().

  If you do not want the text edit to have a context menu, you can set
  its \l contextMenuPolicy to Qt::NoContextMenu. If you want to
  customize the context menu, reimplement this function. If you want
  to extend the standard context menu, reimplement this function, call
  createStandardContextMenu() and extend the menu returned.

  Information about the event is passed in the \a event object.

  \code
  void MyQPlainTextEdit::contextMenuEvent(QContextMenuEvent *event)
  {
      QMenu *menu = createStandardContextMenu();
      menu->addAction(tr("My Menu Item"));
      //...
      menu->exec(event->globalPos());
      delete menu;
  }
  \endcode
*/
void PlainTextEdit::contextMenuEvent(QContextMenuEvent *e)
{
    d->sendControlEvent(e);
}
#endif // QT_NO_CONTEXTMENU

#if QT_CONFIG(draganddrop)
void PlainTextEdit::dragEnterEvent(QDragEnterEvent *e)
{
    d->inDrag = true;
    d->sendControlEvent(e);
}

void PlainTextEdit::dragLeaveEvent(QDragLeaveEvent *e)
{
    d->inDrag = false;
    d->autoScrollTimer.stop();
    d->sendControlEvent(e);
}

void PlainTextEdit::dragMoveEvent(QDragMoveEvent *e)
{
    d->autoScrollDragPos = e->position().toPoint();
    if (!d->autoScrollTimer.isActive())
        d->autoScrollTimer.start(100, this);
    d->sendControlEvent(e);
}

/*!
    \overload QTextEdit::dropEvent()
*/
void PlainTextEdit::dropEvent(QDropEvent *e)
{
    d->inDrag = false;
    d->autoScrollTimer.stop();
    d->sendControlEvent(e);
}

#endif // QT_CONFIG(draganddrop)

void PlainTextEdit::inputMethodEvent(QInputMethodEvent *e)
{
#ifdef QT_KEYPAD_NAVIGATION
    if (d->control->textInteractionFlags() & Qt::TextEditable
        && QApplicationPrivate::keypadNavigationEnabled()
        && !hasEditFocus()) {
        setEditFocus(true);
        selectAll();    // so text is replaced rather than appended to
    }
#endif
    d->sendControlEvent(e);
    const bool emptyEvent = e->preeditString().isEmpty() && e->commitString().isEmpty()
                            && e->attributes().isEmpty();
    if (emptyEvent)
        return;
    ensureCursorVisible();
}

void PlainTextEdit::scrollContentsBy(int dx, int /*dy*/)
{
    int line = d->topLineForVScrollbarValue(d->vbar()->value());
    int scrollbarLineValue = d->vScrollbarValueForLine(line);
    d->setTopLine(line, dx, d->vbar()->value() - scrollbarLineValue);
}

QVariant PlainTextEdit::inputMethodQuery(Qt::InputMethodQuery property) const
{
    return inputMethodQuery(property, QVariant());
}

/*!\internal
 */
QVariant PlainTextEdit::inputMethodQuery(Qt::InputMethodQuery query, QVariant argument) const
{
    switch (query) {
    case Qt::ImEnabled:
        return isEnabled() && !isReadOnly();
    case Qt::ImHints:
    case Qt::ImInputItemClipRectangle:
        return QWidget::inputMethodQuery(query);
    case Qt::ImReadOnly:
        return isReadOnly();
    default:
        break;
    }

    const QPointF offset = contentOffset();
    switch (argument.userType()) {
    case QMetaType::QRectF:
        argument = argument.toRectF().translated(-offset);
        break;
    case QMetaType::QPointF:
        argument = argument.toPointF() - offset;
        break;
    case QMetaType::QRect:
        argument = argument.toRect().translated(-offset.toPoint());
        break;
    case QMetaType::QPoint:
        argument = argument.toPoint() - offset;
        break;
    default:
        break;
    }

    const QVariant v = d->control->inputMethodQuery(query, argument);
    switch (v.userType()) {
    case QMetaType::QRectF:
        return v.toRectF().translated(offset);
    case QMetaType::QPointF:
        return v.toPointF() + offset;
    case QMetaType::QRect:
        return v.toRect().translated(offset.toPoint());
    case QMetaType::QPoint:
        return v.toPoint() + offset.toPoint();
    default:
        break;
    }
    return v;
}

void PlainTextEdit::focusInEvent(QFocusEvent *e)
{
    if (e->reason() == Qt::MouseFocusReason) {
        d->clickCausedFocus = 1;
    }
    QAbstractScrollArea::focusInEvent(e);
    d->sendControlEvent(e);
}

void PlainTextEdit::focusOutEvent(QFocusEvent *e)
{
    QAbstractScrollArea::focusOutEvent(e);
    d->sendControlEvent(e);
}

void PlainTextEdit::showEvent(QShowEvent *)
{
    if (d->showCursorOnInitialShow) {
        d->showCursorOnInitialShow = false;
        ensureCursorVisible();
    }
    d->adjustScrollbars();
}

void PlainTextEdit::changeEvent(QEvent *e)
{
    QAbstractScrollArea::changeEvent(e);

    switch (e->type()) {
    case QEvent::ApplicationFontChange:
    case QEvent::FontChange:
        d->control->document()->setDefaultFont(font());
        break;
    case QEvent::ActivationChange:
        if (!isActiveWindow())
            d->autoScrollTimer.stop();
        break;
    case QEvent::EnabledChange:
        e->setAccepted(isEnabled());
        d->control->setPalette(palette());
        d->sendControlEvent(e);
        break;
    case QEvent::PaletteChange:
        d->control->setPalette(palette());
        break;
    case QEvent::LayoutDirectionChange:
        d->sendControlEvent(e);
        break;
    default:
        break;
    }
}

#if QT_CONFIG(wheelevent)
void PlainTextEdit::wheelEvent(QWheelEvent *e)
{
    if (!(d->control->textInteractionFlags() & Qt::TextEditable)) {
        if (e->modifiers() & Qt::ControlModifier) {
            float delta = e->angleDelta().y() / 120.f;
            zoomInF(delta);
            return;
        }
    }
    QAbstractScrollArea::wheelEvent(e);
    updateMicroFocus();
}
#endif

/*!
    Zooms in on the text by making the base font size \a range
    points larger and recalculating all font sizes to be the new size.
    This does not change the size of any images.

    \sa zoomOut()
*/
void PlainTextEdit::zoomIn(int range)
{
    zoomInF(range);
}

/*!
    Zooms out on the text by making the base font size \a range points
    smaller and recalculating all font sizes to be the new size. This
    does not change the size of any images.

    \sa zoomIn()
*/
void PlainTextEdit::zoomOut(int range)
{
    zoomInF(-range);
}

/*!
    \internal
*/
void PlainTextEdit::zoomInF(float range)
{
    if (range == 0.f)
        return;
    QFont f = font();
    const float newSize = f.pointSizeF() + range;
    if (newSize <= 0)
        return;
    f.setPointSizeF(newSize);
    setFont(f);
}

#ifndef QT_NO_CONTEXTMENU
/*!  This function creates the standard context menu which is shown
  when the user clicks on the text edit with the right mouse
  button. It is called from the default \c contextMenuEvent() handler.
  The popup menu's ownership is transferred to the caller.

  We recommend that you use the \c createStandardContextMenu(QPoint) version instead
  which will enable the actions that are sensitive to where the user clicked.
*/

QMenu *PlainTextEdit::createStandardContextMenu()
{
    return d->control->createStandardContextMenu(QPointF(), this);
}

/*!
  Creates the standard context menu, which is shown
  when the user clicks on the text edit with the right mouse
  button. It is called from the default \c contextMenuEvent() handler
  and it takes the \a position in document coordinates where the mouse click was.
  This can enable actions that are sensitive to the position where the user clicked.
  The popup menu's ownership is transferred to the caller.
*/

QMenu *PlainTextEdit::createStandardContextMenu(const QPoint &position)
{
    return d->control->createStandardContextMenu(position, this);
}
#endif // QT_NO_CONTEXTMENU

/*!
  Returns a QTextCursor at position \a pos (in viewport coordinates).
*/
QTextCursor PlainTextEdit::cursorForPosition(const QPoint &pos) const
{
    return d->control->cursorForPosition(d->mapToContents(pos));
}

/*!
  Returns a rectangle (in viewport coordinates) that includes the
  \a cursor.
 */
QRect PlainTextEdit::cursorRect(const QTextCursor &cursor) const
{
    if (cursor.isNull())
        return QRect();

    QRect r = d->control->cursorRect(cursor).toRect();
    r.translate(-d->horizontalOffset(),-(int)d->verticalOffset());
    return r;
}

/*!
  Returns a rectangle (in viewport coordinates) that includes the
  cursor of the text edit.
 */
QRect PlainTextEdit::cursorRect() const
{
    QRect r = d->control->cursorRect().toRect();
    r.translate(-d->horizontalOffset(),-(int)d->verticalOffset());
    return r;
}


/*!
   \property Utils::PlainTextEdit::overwriteMode
   \brief whether text entered by the user will overwrite existing text

   As with many text editors, the plain text editor widget can be configured
   to insert or overwrite existing text with new text entered by the user.

   If this property is \c true, existing text is overwritten, character-for-character
   by new text; otherwise, text is inserted at the cursor position, displacing
   existing text.

   By default, this property is \c false (new text does not overwrite existing text).
*/

bool PlainTextEdit::overwriteMode() const
{
    return d->control->overwriteMode();
}

void PlainTextEdit::setOverwriteMode(bool overwrite)
{
    d->control->setOverwriteMode(overwrite);
}

/*!
    \property Utils::PlainTextEdit::tabStopDistance
    \brief the tab stop distance in pixels

    By default, this property contains a value of 80 pixels.

    Do not set a value less than the \l {QFontMetrics::}{horizontalAdvance()}
    of the QChar::VisualTabCharacter character, otherwise the tab-character
    will be drawn incompletely.

    \sa QTextOption::ShowTabsAndSpaces, QTextDocument::defaultTextOption
*/

qreal PlainTextEdit::tabStopDistance() const
{
    return d->control->document()->defaultTextOption().tabStopDistance();
}

void PlainTextEdit::setTabStopDistance(qreal distance)
{
    QTextOption opt = d->control->document()->defaultTextOption();
    if (opt.tabStopDistance() == distance || distance < 0)
        return;
    opt.setTabStopDistance(distance);
    d->control->document()->setDefaultTextOption(opt);
}


/*!
    \property Utils::PlainTextEdit::cursorWidth

    This property specifies the width of the cursor in pixels. The default value is 1.
*/
int PlainTextEdit::cursorWidth() const
{
    return d->control->cursorWidth();
}

void PlainTextEdit::setCursorWidth(int width)
{
    d->control->setCursorWidth(width);
}



/*!
    This function allows temporarily marking certain regions in the document
    with a given color, specified as \a selections. This can be useful for
    example in a programming editor to mark a whole line of text with a given
    background color to indicate the existence of a breakpoint.

    \sa QTextEdit::ExtraSelection, extraSelections()
*/
void PlainTextEdit::setExtraSelections(const QList<QTextEdit::ExtraSelection> &selections)
{
    d->control->setExtraSelections(selections);
}

/*!
    Returns previously set extra selections.

    \sa setExtraSelections()
*/
QList<QTextEdit::ExtraSelection> PlainTextEdit::extraSelections() const
{
    return d->control->extraSelections();
}

/*!
    This function returns a new MIME data object to represent the contents
    of the text edit's current selection. It is called when the selection needs
    to be encapsulated into a new QMimeData object; for example, when a drag
    and drop operation is started, or when data is copied to the clipboard.

    If you reimplement this function, note that the ownership of the returned
    QMimeData object is passed to the caller. The selection can be retrieved
    by using the textCursor() function.
*/
QMimeData *PlainTextEdit::createMimeDataFromSelection() const
{
    return d->control->WidgetTextControl::createMimeDataFromSelection();
}

/*!
    This function returns \c true if the contents of the MIME data object, specified
    by \a source, can be decoded and inserted into the document. It is called
    for example when during a drag operation the mouse enters this widget and it
    is necessary to determine whether it is possible to accept the drag.
 */
bool PlainTextEdit::canInsertFromMimeData(const QMimeData *source) const
{
    return d->control->WidgetTextControl::canInsertFromMimeData(source);
}

/*!
    This function inserts the contents of the MIME data object, specified
    by \a source, into the text edit at the current cursor position. It is
    called whenever text is inserted as the result of a clipboard paste
    operation, or when the text edit accepts data from a drag and drop
    operation.
*/
void PlainTextEdit::insertFromMimeData(const QMimeData *source)
{
    d->control->WidgetTextControl::insertFromMimeData(source);
}

/*!
    \property Utils::PlainTextEdit::readOnly
    \brief whether the text edit is read-only

    In a read-only text edit the user can only navigate through the
    text and select text; modifying the text is not possible.

    This property's default is \c false.
*/

bool PlainTextEdit::isReadOnly() const
{
    return !d->control || !(d->control->textInteractionFlags() & Qt::TextEditable);
}

void PlainTextEdit::setReadOnly(bool ro)
{
    Qt::TextInteractionFlags flags = Qt::NoTextInteraction;
    if (ro) {
        flags = Qt::TextSelectableByMouse;
    } else {
        flags = Qt::TextEditorInteraction;
    }
    d->control->setTextInteractionFlags(flags);
    setAttribute(Qt::WA_InputMethodEnabled, shouldEnableInputMethod(this));
    QEvent event(QEvent::ReadOnlyChange);
    QCoreApplication::sendEvent(this, &event);
}

/*!
    \property Utils::PlainTextEdit::textInteractionFlags

    Specifies how the label should interact with user input if it displays text.

    If the flags contain either Qt::LinksAccessibleByKeyboard or Qt::TextSelectableByKeyboard
    then the focus policy is also automatically set to Qt::ClickFocus.

    The default value depends on whether the PlainTextEdit is read-only
    or editable.
*/

void PlainTextEdit::setTextInteractionFlags(Qt::TextInteractionFlags flags)
{
    d->control->setTextInteractionFlags(flags);
}

Qt::TextInteractionFlags PlainTextEdit::textInteractionFlags() const
{
    return d->control->textInteractionFlags();
}

/*!
    Merges the properties specified in \a modifier into the current character
    format by calling QTextCursor::mergeCharFormat on the editor's cursor.
    If the editor has a selection then the properties of \a modifier are
    directly applied to the selection.

    \sa QTextCursor::mergeCharFormat()
 */
void PlainTextEdit::mergeCurrentCharFormat(const QTextCharFormat &modifier)
{
    d->control->mergeCurrentCharFormat(modifier);
}

/*!
    Sets the char format that is be used when inserting new text to \a
    format by calling QTextCursor::setCharFormat() on the editor's
    cursor.  If the editor has a selection then the char format is
    directly applied to the selection.
 */
void PlainTextEdit::setCurrentCharFormat(const QTextCharFormat &format)
{
    d->control->setCurrentCharFormat(format);
}

/*!
    Returns the char format that is used when inserting new text.
 */
QTextCharFormat PlainTextEdit::currentCharFormat() const
{
    return d->control->currentCharFormat();
}



/*!
    Convenience slot that inserts \a text at the current
    cursor position.

    It is equivalent to:

    \code
    edit->textCursor().insertText(text);
    \endcode
 */
void PlainTextEdit::insertPlainText(const QString &text)
{
    d->control->insertPlainText(text);
}


/*!
    \internal

    Moves the cursor by performing the given \a operation.

    If \a mode is QTextCursor::KeepAnchor, the cursor selects the text it moves over.
    This is the same effect that the user achieves when they hold down the Shift key
    and move the cursor with the cursor keys.

    \sa QTextCursor::movePosition()
*/
void PlainTextEdit::moveCursor(QTextCursor::MoveOperation operation, QTextCursor::MoveMode mode)
{
    d->control->moveCursor(operation, mode);
}

/*!
    Returns whether text can be pasted from the clipboard into the text edit.
*/
bool PlainTextEdit::canPaste() const
{
    return d->control->canPaste();
}

/*!
    \internal

    Convenience function to print the text edit's document to the given \a printer. This
    is equivalent to calling the print method on the document directly except that this
    function also supports QPrinter::Selection as print range.

    \sa QTextDocument::print()
*/
#ifndef QT_NO_PRINTER
void PlainTextEdit::print(QPagedPaintDevice *printer) const
{
    d->control->print(printer);
}
#endif

/*! \property Utils::PlainTextEdit::tabChangesFocus
  \brief whether \uicontrol Tab changes focus or is accepted as input

  In some occasions text edits should not allow the user to input
  tabulators or change indentation using the \uicontrol Tab key, as this breaks
  the focus chain. The default is \c false.

*/

bool PlainTextEdit::tabChangesFocus() const
{
    return d->tabChangesFocus;
}

void PlainTextEdit::setTabChangesFocus(bool b)
{
    d->tabChangesFocus = b;
}

/*!
    \property Utils::PlainTextEdit::documentTitle
    \brief the title of the document parsed from the text.

    By default, this property contains an empty string.
*/

/*!
    \property Utils::PlainTextEdit::lineWrapMode
    \brief the line wrap mode

    The default mode is WidgetWidth which causes words to be
    wrapped at the right edge of the text edit. Wrapping occurs at
    whitespace, keeping whole words intact. If you want wrapping to
    occur within words use setWordWrapMode().
*/

PlainTextEdit::LineWrapMode PlainTextEdit::lineWrapMode() const
{
    return d->lineWrap;
}

void PlainTextEdit::setLineWrapMode(LineWrapMode wrap)
{
    if (d->lineWrap == wrap)
        return;
    d->lineWrap = wrap;
    d->updateDefaultTextOption();
    d->relayoutDocument();
    d->adjustScrollbars();
    ensureCursorVisible();
}

/*!
    \property Utils::PlainTextEdit::wordWrapMode
    \brief the mode PlainTextEdit will use when wrapping text by words

    By default, this property is set to QTextOption::WrapAtWordBoundaryOrAnywhere.

    \sa QTextOption::WrapMode
*/

QTextOption::WrapMode PlainTextEdit::wordWrapMode() const
{
    return d->wordWrap;
}

void PlainTextEdit::setWordWrapMode(QTextOption::WrapMode mode)
{
    if (mode == d->wordWrap)
        return;
    d->wordWrap = mode;
    d->updateDefaultTextOption();
}

/*!
    \property Utils::PlainTextEdit::backgroundVisible
    \brief Whether the palette background is visible outside the document area.

    If set to \c true, the plain text edit paints the palette background
    on the viewport area not covered by the text document. Otherwise,
    if set to \c false, it won't. The feature makes it possible for
    the user to visually distinguish between the area of the document,
    painted with the base color of the palette, and the empty
    area not covered by any document.

    The default is \c false.
*/

bool PlainTextEdit::backgroundVisible() const
{
    return d->backgroundVisible;
}

void PlainTextEdit::setBackgroundVisible(bool visible)
{
    if (visible == d->backgroundVisible)
        return;
    d->backgroundVisible = visible;
    d->updateViewport();
}

/*!
    \property Utils::PlainTextEdit::centerOnScroll
    \brief Whether the cursor should be centered on screen.

    If set to \c true, the plain text edit scrolls the document
    vertically to make the cursor visible at the center of the
    viewport. This also allows the text edit to scroll below the end
    of the document. Otherwise, if set to \c false, the plain text edit
    scrolls the smallest amount possible to ensure the cursor is
    visible.  The same algorithm is applied to any new line appended
    through appendPlainText().

    The default is \c false.

    \sa centerCursor(), ensureCursorVisible()
*/

bool PlainTextEdit::centerOnScroll() const
{
    return d->centerOnScroll;
}

void PlainTextEdit::setCenterOnScroll(bool enabled)
{
    if (enabled == d->centerOnScroll)
        return;
    d->centerOnScroll = enabled;
    d->adjustScrollbars();
}



/*!
    \internal

    Finds the next occurrence of the string, \a exp, using the given
    \a options. Returns \c true if \a exp was found and changes the
    cursor to select the match; otherwise returns \c false.
*/
bool PlainTextEdit::find(const QString &exp, QTextDocument::FindFlags options)
{
    return d->control->find(exp, options);
}

/*!
    \internal
    \overload

    Finds the next occurrence, matching the regular expression, \a exp, using the given
    \a options.

    Returns \c true if a match was found and changes the cursor to select the match;
    otherwise returns \c false.

    \warning For historical reasons, the case sensitivity option set on
    \a exp is ignored. Instead, the \a options are used to determine
    if the search is case sensitive or not.
*/
#if QT_CONFIG(regularexpression)
bool PlainTextEdit::find(const QRegularExpression &exp, QTextDocument::FindFlags options)
{
    return d->control->find(exp, options);
}
#endif

void PlainTextEdit::setTopBlock(const QTextBlock &block)
{
    d->setTopBlock(block.firstLineNumber(), 0, 0);
}

/*!
    \fn void PlainTextEdit::copyAvailable(bool yes)

    This signal is emitted when text is selected or de-selected in the
    text edit.

    When text is selected this signal will be emitted with \a yes set
    to \c true. If no text has been selected or if the selected text is
    de-selected this signal is emitted with \a yes set to \c false.

    If \a yes is \c true then copy() can be used to copy the selection to
    the clipboard. If \a yes is \c false then copy() does nothing.

    \sa selectionChanged()
*/


/*!
    \fn void PlainTextEdit::selectionChanged()

    This signal is emitted whenever the selection changes.

    \sa copyAvailable()
*/

/*!
    \fn void PlainTextEdit::cursorPositionChanged()

    This signal is emitted whenever the position of the
    cursor changed.
*/



/*!
    \fn void PlainTextEdit::updateRequest(const QRect &rect, int dy)

    This signal is emitted when the text document needs an update of
    the specified \a rect. If the text is scrolled, \a rect will cover
    the entire viewport area. If the text is scrolled vertically, \a
    dy carries the amount of pixels the viewport was scrolled.

    The purpose of the signal is to support extra widgets in plain
    text edit subclasses that e.g. show line numbers, breakpoints, or
    other extra information.
*/

/*!  \fn void PlainTextEdit::blockCountChanged(int newBlockCount);

    This signal is emitted whenever the block count changes. The new
    block count is passed in \a newBlockCount.
*/

/*!  \fn void PlainTextEdit::modificationChanged(bool changed);

    This signal is emitted whenever the content of the document
    changes in a way that affects the modification state. If \a
    changed is \c true, the document has been modified; otherwise it is
    \c false.

    For example, calling \c setModified(false) on a document and then
    inserting text causes the signal to get emitted. If you undo that
    operation, causing the document to return to its original
    unmodified state, the signal will get emitted again.
*/




void PlainTextEditPrivate::append(const QString &text, Qt::TextFormat format)
{

    QTextDocument *document = control->document();
    PlainTextDocumentLayout *documentLayout = qobject_cast<PlainTextDocumentLayout*>(document->documentLayout());
    Q_ASSERT(documentLayout);

    int maximumBlockCount = document->maximumBlockCount();
    if (maximumBlockCount)
        document->setMaximumBlockCount(0);

    const bool atBottom =  q->isVisible()
                          && (control->blockBoundingRect(document->lastBlock()).bottom() - verticalOffset()
                              <= viewport()->rect().bottom());

    if (!q->isVisible())
        showCursorOnInitialShow = true;

    bool documentSizeChangedBlocked = documentLayout->d->blockDocumentSizeChanged;
    documentLayout->d->blockDocumentSizeChanged = true;

    switch (format) {
    case Qt::RichText:
        control->appendHtml(text);
        break;
    case Qt::PlainText:
        control->appendPlainText(text);
        break;
    default:
        control->append(text);
        break;
    }

    if (maximumBlockCount > 0) {
        if (document->blockCount() > maximumBlockCount) {
            bool blockUpdate = false;
            if (control->topBlock) {
                control->topBlock--;
                blockUpdate = true;
                emit q->updateRequest(viewport()->rect(), 0);
            }

            bool updatesBlocked = documentLayout->d->blockUpdate;
            documentLayout->d->blockUpdate = blockUpdate;
            QTextCursor cursor(document);
            cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
            cursor.removeSelectedText();
            documentLayout->d->blockUpdate = updatesBlocked;
        }
        document->setMaximumBlockCount(maximumBlockCount);
    }

    documentLayout->d->blockDocumentSizeChanged = documentSizeChangedBlocked;
    adjustScrollbars();


    if (atBottom) {
        const bool needScroll =  !centerOnScroll
                                || control->blockBoundingRect(document->lastBlock()).bottom() - verticalOffset()
                                       > viewport()->rect().bottom();
        if (needScroll)
            vbar()->setValue(vbar()->maximum());
    }
}


/*!
    Appends a new paragraph with \a text to the end of the text edit.

    \sa appendHtml()
*/

void PlainTextEdit::appendPlainText(const QString &text)
{
    d->append(text, Qt::PlainText);
}

/*!
    Appends a new paragraph with \a html to the end of the text edit.

    appendPlainText()
*/

void PlainTextEdit::appendHtml(const QString &html)
{
    d->append(html, Qt::RichText);
}

void PlainTextEditPrivate::ensureCursorVisible(bool center)
{
    QRect visible = viewport()->rect();
    QRect cr = q->cursorRect();
    if (cr.top() < visible.top() || cr.bottom() > visible.bottom()) {
        ensureVisible(control->textCursor().position(), center);
    }

    const bool rtl = q->isRightToLeft();
    if (cr.left() < visible.left() || cr.right() > visible.right()) {
        int x = cr.center().x() + horizontalOffset() - visible.width()/2;
        hbar()->setValue(rtl ? hbar()->maximum() - x : x);
    }
}

/*!
    Ensures that the cursor is visible by scrolling the text edit if
    necessary.

    \sa centerCursor(), centerOnScroll
*/
void PlainTextEdit::ensureCursorVisible()
{
    d->ensureCursorVisible(d->centerOnScroll);
}


/*!  Scrolls the document in order to center the cursor vertically.

\sa ensureCursorVisible(), centerOnScroll
 */
void PlainTextEdit::centerCursor()
{
    d->ensureVisible(textCursor().position(), true, true);
}

/*!
  Returns the first visible block.
 */
QTextBlock PlainTextEdit::firstVisibleBlock() const
{
    return d->control->firstVisibleBlock();
}

/*!  Returns the content's origin in viewport coordinates.

     The origin of the content of a plain text edit is always the top
     left corner of the first visible text block. The content offset
     is different from (0,0) when the text has been scrolled
     horizontally, or when the first visible block has been scrolled
     partially off the screen, i.e. the visible text does not start
     with the first line of the first visible block, or when the first
     visible block is the very first block and the editor displays a
     margin.

     \sa firstVisibleBlock(), QAbstractScrollArea::horizontalScrollBar(),
     QAbstractScrollArea::verticalScrollBar()
 */
QPointF PlainTextEdit::contentOffset() const
{
    return QPointF(-d->horizontalOffset(), -d->verticalOffset());
}


/*!
  \internal

  Returns the bounding rectangle of the text \a block in content
  coordinates. Translate the rectangle with the contentOffset() to get
  visual coordinates on the viewport.

  \sa {firstVisibleBlock()}, {blockBoundingRect()}
 */
QRectF PlainTextEdit::blockBoundingGeometry(const QTextBlock &block) const
{
    return d->control->blockBoundingRect(block);
}

/*!
  \internal

  Returns the bounding rectangle of the text \a block in the block's own coordinates.

  \sa blockBoundingGeometry()
 */
QRectF PlainTextEdit::blockBoundingRect(const QTextBlock &block) const
{
    PlainTextDocumentLayout *documentLayout = qobject_cast<PlainTextDocumentLayout*>(document()->documentLayout());
    Q_ASSERT(documentLayout);
    return documentLayout->blockBoundingRect(block);
}

/*!
    \property Utils::PlainTextEdit::blockCount
    \brief the number of text blocks in the document.

    By default, in an empty document, this property contains a value of 1.
*/
int PlainTextEdit::blockCount() const
{
    return document()->blockCount();
}

/*!  Returns the paint context for the \c viewport(), useful only when
  reimplementing paintEvent().
 */
QAbstractTextDocumentLayout::PaintContext PlainTextEdit::getPaintContext() const
{
    return d->control->getPaintContext(viewport());
}

/*!
    \property Utils::PlainTextEdit::maximumBlockCount
    \brief the limit for blocks in the document.

    Specifies the maximum number of blocks the document may have. If there are
    more blocks in the document that specified with this property blocks are removed
    from the beginning of the document.

    A negative or zero value specifies that the document may contain an unlimited
    amount of blocks.

    The default value is 0.

    Note that setting this property will apply the limit immediately to the document
    contents. Setting this property also disables the undo redo history.

*/


/*!
    \fn void PlainTextEdit::textChanged()

    This signal is emitted whenever the document's content changes; for
    example, when text is inserted or deleted, or when formatting is applied.
*/

/*!
    \fn void PlainTextEdit::undoAvailable(bool available)

    This signal is emitted whenever undo operations become available
    (\a available is \c true) or unavailable (\a available is \c false).
*/

/*!
    \fn void PlainTextEdit::redoAvailable(bool available)

    This signal is emitted whenever redo operations become available
    (\a available is \c true) or unavailable (\a available is \c false).
*/

} // namespace Utils

#include "plaintextedit.moc"
