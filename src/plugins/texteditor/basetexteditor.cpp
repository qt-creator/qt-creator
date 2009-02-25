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

#include "texteditor_global.h"

#include "texteditorconstants.h"
#ifndef TEXTEDITOR_STANDALONE
#include "texteditorplugin.h"
#include "completionsupport.h"
#endif
#include "basetextdocument.h"
#include "basetexteditor_p.h"
#include "codecselector.h"

#ifndef TEXTEDITOR_STANDALONE
#include <coreplugin/manhattanstyle.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <extensionsystem/pluginmanager.h>
#include <find/basetextfind.h>
#include <texteditor/fontsettings.h>
#include <utils/reloadpromptutils.h>
#include <aggregation/aggregate.h>
#endif
#include <utils/linecolumnlabel.h>
#include <utils/qtcassert.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QTextCodec>
#include <QtCore/QFile>
#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtGui/QAbstractTextDocumentLayout>
#include <QtGui/QApplication>
#include <QtGui/QKeyEvent>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QPainter>
#include <QtGui/QPrinter>
#include <QtGui/QPrintDialog>
#include <QtGui/QPainter>
#include <QtGui/QScrollBar>
#include <QtGui/QShortcut>
#include <QtGui/QScrollBar>
#include <QtGui/QStyle>
#include <QtGui/QSyntaxHighlighter>
#include <QtGui/QTextCursor>
#include <QtGui/QTextBlock>
#include <QtGui/QTextLayout>
#include <QtGui/QToolBar>
#include <QtGui/QToolTip>
#include <QtGui/QInputDialog>
#include <QtGui/QMenu>

using namespace TextEditor;
using namespace TextEditor::Internal;


namespace TextEditor {
namespace Internal {

class TextEditExtraArea : public QWidget {
    BaseTextEditor *textEdit;
public:
    TextEditExtraArea(BaseTextEditor *edit):QWidget(edit) {
        textEdit = edit;
        setAutoFillBackground(true);
    }
public:

    QSize sizeHint() const {
        return QSize(textEdit->extraAreaWidth(), 0);
    }
protected:
    void paintEvent(QPaintEvent *event){
        textEdit->extraAreaPaintEvent(event);
    }
    void mousePressEvent(QMouseEvent *event){
        textEdit->extraAreaMouseEvent(event);
    }
    void mouseMoveEvent(QMouseEvent *event){
        textEdit->extraAreaMouseEvent(event);
    }
    void mouseReleaseEvent(QMouseEvent *event){
        textEdit->extraAreaMouseEvent(event);
    }
    void leaveEvent(QEvent *event){
        textEdit->extraAreaLeaveEvent(event);
    }

    void wheelEvent(QWheelEvent *event) {
        QCoreApplication::sendEvent(textEdit->viewport(), event);
    }
};

} // namespace Internal
} // namespace TextEditor

ITextEditor *BaseTextEditor::openEditorAt(const QString &fileName,
                                             int line,
                                             int column,
                                             const QString &editorKind)
{
    Core::EditorManager *editorManager = Core::EditorManager::instance();
    editorManager->addCurrentPositionToNavigationHistory(true);
    Core::IEditor *editor = editorManager->openEditor(fileName, editorKind, true);
    TextEditor::ITextEditor *texteditor = qobject_cast<TextEditor::ITextEditor *>(editor);
    if (texteditor) {
        texteditor->gotoLine(line, column);
        editorManager->addCurrentPositionToNavigationHistory();
        return texteditor;
    }
    return 0;
}

BaseTextEditor::BaseTextEditor(QWidget *parent)
    : QPlainTextEdit(parent)
{
    d = new BaseTextEditorPrivate();
    d->q = this;
    d->m_extraArea = new TextEditExtraArea(this);
    d->m_extraArea->setMouseTracking(true);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    d->setupDocumentSignals(d->m_document);
    d->setupDocumentSignals(d->m_document);

    d->m_lastScrollPos = -1;
    setCursorWidth(2);


    // from RESEARCH

    setLayoutDirection(Qt::LeftToRight);
    viewport()->setMouseTracking(true);
    d->extraAreaSelectionAnchorBlockNumber
        = d->extraAreaToggleMarkBlockNumber
        = d->extraAreaHighlightCollapseBlockNumber
        = d->extraAreaHighlightFadingBlockNumber
        = -1;
    d->extraAreaCollapseAlpha = 255;

    d->visibleCollapsedBlockNumber = d->suggestedVisibleCollapsedBlockNumber = -1;

    connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(slotUpdateExtraAreaWidth()));
    connect(this, SIGNAL(modificationChanged(bool)), this, SLOT(slotModificationChanged(bool)));
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(slotCursorPositionChanged()));
    connect(this, SIGNAL(updateRequest(QRect, int)), this, SLOT(slotUpdateRequest(QRect, int)));
    connect(this, SIGNAL(selectionChanged()), this, SLOT(slotSelectionChanged()));

//     (void) new QShortcut(tr("CTRL+L"), this, SLOT(centerCursor()), 0, Qt::WidgetShortcut);
//     (void) new QShortcut(tr("F9"), this, SLOT(slotToggleMark()), 0, Qt::WidgetShortcut);
//     (void) new QShortcut(tr("F11"), this, SLOT(slotToggleBlockVisible()));


    // parentheses matcher
    d->m_parenthesesMatchingEnabled = false;
    d->m_formatRange = true;
    d->m_matchFormat.setForeground(Qt::red);
    d->m_rangeFormat.setBackground(QColor(0xb4, 0xee, 0xb4));
    d->m_mismatchFormat.setBackground(Qt::magenta);
    d->m_parenthesesMatchingTimer = new QTimer(this);
    d->m_parenthesesMatchingTimer->setSingleShot(true);
    connect(d->m_parenthesesMatchingTimer, SIGNAL(timeout()), this, SLOT(_q_matchParentheses()));


    d->m_searchResultFormat.setBackground(QColor(0xffef0b));

    slotUpdateExtraAreaWidth();
    slotCursorPositionChanged();
    setFrameStyle(QFrame::NoFrame);


    d->extraAreaTimeLine = new QTimeLine(150, this);
    d->extraAreaTimeLine->setFrameRange(0, 255);
    connect(d->extraAreaTimeLine, SIGNAL(frameChanged(int)), this,
            SLOT(setCollapseIndicatorAlpha(int)));


    connect(Core::EditorManager::instance(), SIGNAL(currentEditorChanged(Core::IEditor*)),
            this, SLOT(currentEditorChanged(Core::IEditor*)));
}

BaseTextEditor::~BaseTextEditor()
{
    delete d;
    d = 0;
}

QString BaseTextEditor::mimeType() const
{
    return d->m_document->mimeType();
}

void BaseTextEditor::setMimeType(const QString &mt)
{
    d->m_document->setMimeType(mt);
}

void BaseTextEditor::print(QPrinter *printer)
{
    const bool oldFullPage =  printer->fullPage();
    printer->setFullPage(true);
    QPrintDialog *dlg = new QPrintDialog(printer, this);
    dlg->setWindowTitle(tr("Print Document"));
    if (dlg->exec() == QDialog::Accepted) {
        d->print(printer);
    }
    printer->setFullPage(oldFullPage);
    delete dlg;
}

static void printPage(int index, QPainter *painter, const QTextDocument *doc,
                      const QRectF &body, const QRectF &titleBox,
                      const QString &title)
{
    painter->save();

    painter->translate(body.left(), body.top() - (index - 1) * body.height());
    QRectF view(0, (index - 1) * body.height(), body.width(), body.height());

    QAbstractTextDocumentLayout *layout = doc->documentLayout();
    QAbstractTextDocumentLayout::PaintContext ctx;

    painter->setFont(QFont(doc->defaultFont()));
    QRectF box = titleBox.translated(0, view.top());
    int dpix = painter->device()->logicalDpiX();
    int dpiy = painter->device()->logicalDpiY();
    int mx = 5 * dpix / 72.0;
    int my = 2 * dpiy / 72.0;
    painter->fillRect(box.adjusted(-mx, -my, mx, my), QColor(210, 210, 210));
    if (!title.isEmpty())
        painter->drawText(box, Qt::AlignCenter, title);
    const QString pageString = QString::number(index);
    painter->drawText(box, Qt::AlignRight, pageString);

    painter->setClipRect(view);
    ctx.clip = view;
    // don't use the system palette text as default text color, on HP/UX
    // for example that's white, and white text on white paper doesn't
    // look that nice
    ctx.palette.setColor(QPalette::Text, Qt::black);

    layout->draw(painter, ctx);

    painter->restore();
}

void BaseTextEditorPrivate::print(QPrinter *printer)
{

    QTextDocument *doc = q->document();

    QString title = q->displayName();
    if (title.isEmpty())
        printer->setDocName(title);


    QPainter p(printer);

    // Check that there is a valid device to print to.
    if (!p.isActive())
        return;

    doc = doc->clone(doc);

    QTextOption opt = doc->defaultTextOption();
    opt.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    doc->setDefaultTextOption(opt);

    (void)doc->documentLayout(); // make sure that there is a layout


    QColor background = q->palette().color(QPalette::Base);
    bool backgroundIsDark = background.value() < 128;

    for (QTextBlock srcBlock = q->document()->firstBlock(), dstBlock = doc->firstBlock();
         srcBlock.isValid() && dstBlock.isValid();
         srcBlock = srcBlock.next(), dstBlock = dstBlock.next()) {


        QList<QTextLayout::FormatRange> formatList = srcBlock.layout()->additionalFormats();
        if (backgroundIsDark) {
            // adjust syntax highlighting colors for better contrast
            for (int i = formatList.count() - 1; i >=0; --i) {
                QTextCharFormat &format = formatList[i].format;
                if (format.background().color() == background) {
                    QBrush brush = format.foreground();
                    QColor color = brush.color();
                    int h,s,v,a;
                    color.getHsv(&h, &s, &v, &a);
                    color.setHsv(h, s, qMin(128, v), a);
                    brush.setColor(color);
                    format.setForeground(brush);
                }
                format.setBackground(Qt::white);
            }
        }

        dstBlock.layout()->setAdditionalFormats(formatList);
    }

    QAbstractTextDocumentLayout *layout = doc->documentLayout();
    layout->setPaintDevice(p.device());

    int dpiy = p.device()->logicalDpiY();
    int margin = (int) ((2/2.54)*dpiy); // 2 cm margins

    QTextFrameFormat fmt = doc->rootFrame()->frameFormat();
    fmt.setMargin(margin);
    doc->rootFrame()->setFrameFormat(fmt);

    QRectF pageRect(printer->pageRect());
    QRectF body = QRectF(0, 0, pageRect.width(), pageRect.height());
    QFontMetrics fontMetrics(doc->defaultFont(), p.device());

    QRectF titleBox(margin,
                    body.top() + margin
                    - fontMetrics.height()
                    - 6 * dpiy / 72.0,
                    body.width() - 2*margin,
                    fontMetrics.height());
    doc->setPageSize(body.size());

    int docCopies;
    int pageCopies;
    if (printer->collateCopies() == true){
        docCopies = 1;
        pageCopies = printer->numCopies();
    } else {
        docCopies = printer->numCopies();
        pageCopies = 1;
    }

    int fromPage = printer->fromPage();
    int toPage = printer->toPage();
    bool ascending = true;

    if (fromPage == 0 && toPage == 0) {
        fromPage = 1;
        toPage = doc->pageCount();
    }
    // paranoia check
    fromPage = qMax(1, fromPage);
    toPage = qMin(doc->pageCount(), toPage);

    if (printer->pageOrder() == QPrinter::LastPageFirst) {
        int tmp = fromPage;
        fromPage = toPage;
        toPage = tmp;
        ascending = false;
    }

    for (int i = 0; i < docCopies; ++i) {

        int page = fromPage;
        while (true) {
            for (int j = 0; j < pageCopies; ++j) {
                if (printer->printerState() == QPrinter::Aborted
                    || printer->printerState() == QPrinter::Error)
                    goto UserCanceled;
                printPage(page, &p, doc, body, titleBox, title);
                if (j < pageCopies - 1)
                    printer->newPage();
            }

            if (page == toPage)
                break;

            if (ascending)
                ++page;
            else
                --page;

            printer->newPage();
        }

        if ( i < docCopies - 1)
            printer->newPage();
    }

UserCanceled:
    delete doc;
}


bool DocumentMarker::addMark(TextEditor::ITextMark *mark, int line)
{
    QTC_ASSERT(line >= 1, return false);
    int blockNumber = line - 1;
    TextEditDocumentLayout *documentLayout = qobject_cast<TextEditDocumentLayout*>(document->documentLayout());
    QTC_ASSERT(documentLayout, return false);
    QTextBlock block = document->findBlockByNumber(blockNumber);

    if (block.isValid()) {
        TextBlockUserData *userData = TextEditDocumentLayout::userData(block);
        userData->addMark(mark);
        mark->updateLineNumber(blockNumber + 1);
        mark->updateBlock(block);
        documentLayout->hasMarks = true;
        documentLayout->requestUpdate();
        return true;
    }
    return false;
}


TextEditor::TextMarks DocumentMarker::marksAt(int line) const
{
    QTC_ASSERT(line >= 1, return TextMarks());
    int blockNumber = line - 1;
    QTextBlock block = document->findBlockByNumber(blockNumber);

    if (block.isValid()) {
        if (TextBlockUserData *userData = TextEditDocumentLayout::testUserData(block))
            return userData->marks();
    }
    return TextMarks();
}

void DocumentMarker::removeMark(TextEditor::ITextMark *mark)
{
    bool needUpdate = false;
    QTextBlock block = document->begin();
    while (block.isValid()) {
        if (TextBlockUserData *data = static_cast<TextBlockUserData *>(block.userData())) {
            needUpdate |= data->removeMark(mark);
        }
        block = block.next();
    }
    if (needUpdate)
        updateMark(0);
}


bool DocumentMarker::hasMark(TextEditor::ITextMark *mark) const
{
    QTextBlock block = document->begin();
    while (block.isValid()) {
        if (TextBlockUserData *data = static_cast<TextBlockUserData *>(block.userData())) {
            if (data->hasMark(mark))
                return true;
        }
        block = block.next();
    }
    return false;
}

ITextMarkable *BaseTextEditor::markableInterface() const
{
    return baseTextDocument()->documentMarker();
}

ITextEditable *BaseTextEditor::editableInterface() const
{
    if (!d->m_editable) {
        d->m_editable = const_cast<BaseTextEditor*>(this)->createEditableInterface();
        connect(this, SIGNAL(textChanged()),
                d->m_editable, SIGNAL(contentsChanged()));
        connect(this, SIGNAL(changed()),
                d->m_editable, SIGNAL(changed()));
    }
    return d->m_editable;
}


void BaseTextEditor::currentEditorChanged(Core::IEditor *editor)
{
    if (editor == d->m_editable) {
        if (d->m_document->hasDecodingError()) {
            Core::EditorManager::instance()->showEditorInfoBar(QLatin1String(Constants::SELECT_ENCODING),
                tr("<b>Error:</b> Could not decode \"%1\" with \"%2\"-encoding. Editing not possible.")
                    .arg(displayName()).arg(QString::fromLatin1(d->m_document->codec()->name())),
                tr("Select Encoding"),
                this, SLOT(selectEncoding()));
        }
    }
}

void BaseTextEditor::selectEncoding()
{
    BaseTextDocument *doc = d->m_document;
    CodecSelector codecSelector(this, doc);

    switch (codecSelector.exec()) {
    case CodecSelector::Reload:
        doc->reload(codecSelector.selectedCodec());
        setReadOnly(d->m_document->hasDecodingError());
        if (doc->hasDecodingError())
            currentEditorChanged(Core::EditorManager::instance()->currentEditor());
        else
            Core::EditorManager::instance()->hideEditorInfoBar(QLatin1String(Constants::SELECT_ENCODING));
        break;
    case CodecSelector::Save:
        doc->setCodec(codecSelector.selectedCodec());
        Core::EditorManager::instance()->saveEditor(editableInterface());
        break;
    case CodecSelector::Cancel:
        break;
    }
}

void DocumentMarker::updateMark(ITextMark *mark)
{
    Q_UNUSED(mark);
    TextEditDocumentLayout *documentLayout = qobject_cast<TextEditDocumentLayout*>(document->documentLayout());
    QTC_ASSERT(documentLayout, return);
    documentLayout->requestUpdate();
}


void BaseTextEditor::triggerCompletions()
{
    emit requestAutoCompletion(editableInterface(), true);
}

bool BaseTextEditor::createNew(const QString &contents)
{
    setPlainText(contents);
    document()->setModified(false);
    return true;
}

bool BaseTextEditor::open(const QString &fileName)
{
    if (d->m_document->open(fileName)) {
        moveCursor(QTextCursor::Start);
        setReadOnly(d->m_document->hasDecodingError());
        return true;
    }
    return false;
}

Core::IFile *BaseTextEditor::file()
{
    return d->m_document;
}

void BaseTextEditor::editorContentsChange(int position, int charsRemoved, int charsAdded)
{
    d->m_contentsChanged = true;

    // Keep the line numbers and the block information for the text marks updated
    if (charsRemoved != 0) {
        d->updateMarksLineNumber();
        d->updateMarksBlock(document()->findBlock(position));
    } else {
        const QTextBlock posBlock = document()->findBlock(position);
        const QTextBlock nextBlock = document()->findBlock(position + charsAdded);
        if (posBlock != nextBlock) {
            d->updateMarksLineNumber();
            d->updateMarksBlock(posBlock);
            d->updateMarksBlock(nextBlock);
        } else {
            d->updateMarksBlock(posBlock);
        }
    }
}


void BaseTextEditor::slotSelectionChanged()
{
    bool changed = (d->m_inBlockSelectionMode != d->m_lastEventWasBlockSelectionEvent);
    d->m_inBlockSelectionMode = d->m_lastEventWasBlockSelectionEvent;
    if (changed || d->m_inBlockSelectionMode)
        viewport()->update();
    if (!d->m_inBlockSelectionMode)
        d->m_blockSelectionExtraX = 0;
    if (!d->m_selectBlockAnchor.isNull() && !textCursor().hasSelection())
        d->m_selectBlockAnchor = QTextCursor();
}

void BaseTextEditor::gotoBlockStart()
{
    QTextCursor cursor = textCursor();
    if (TextBlockUserData::findPreviousOpenParenthesis(&cursor, false)) {
        setTextCursor(cursor);
        _q_matchParentheses();
    }
}

void BaseTextEditor::gotoBlockEnd()
{
    QTextCursor cursor = textCursor();
    if (TextBlockUserData::findNextClosingParenthesis(&cursor, false)) {
        setTextCursor(cursor);
        _q_matchParentheses();
    }
}

void BaseTextEditor::gotoBlockStartWithSelection()
{
    QTextCursor cursor = textCursor();
    if (TextBlockUserData::findPreviousOpenParenthesis(&cursor, true)) {
        setTextCursor(cursor);
        _q_matchParentheses();
    }
}

void BaseTextEditor::gotoBlockEndWithSelection()
{
    QTextCursor cursor = textCursor();
    if (TextBlockUserData::findNextClosingParenthesis(&cursor, true)) {
        setTextCursor(cursor);
        _q_matchParentheses();
    }
}

static QTextCursor flippedCursor(const QTextCursor &cursor) {
    QTextCursor flipped = cursor;
    flipped.clearSelection();
    flipped.setPosition(cursor.anchor(), QTextCursor::KeepAnchor);
    return flipped;
}

void BaseTextEditor::selectBlockUp()
{
    QTextCursor cursor = textCursor();
    if (!cursor.hasSelection())
        d->m_selectBlockAnchor = cursor;
    else
        cursor.setPosition(cursor.selectionStart());


    if (!TextBlockUserData::findPreviousOpenParenthesis(&cursor, false))
        return;
    if (!TextBlockUserData::findNextClosingParenthesis(&cursor, true))
        return;
    setTextCursor(flippedCursor(cursor));
    _q_matchParentheses();
}

void BaseTextEditor::selectBlockDown()
{
    QTextCursor tc = textCursor();
    QTextCursor cursor = d->m_selectBlockAnchor;

    if (!tc.hasSelection() || cursor.isNull())
        return;
    tc.setPosition(tc.selectionStart());

    forever {
        QTextCursor ahead = cursor;
        if (!TextBlockUserData::findPreviousOpenParenthesis(&ahead, false))
            break;
        if (ahead.position() <= tc.position())
            break;
        cursor = ahead;
    }
    if ( cursor != d->m_selectBlockAnchor)
        TextBlockUserData::findNextClosingParenthesis(&cursor, true);

    setTextCursor(flippedCursor(cursor));
    _q_matchParentheses();
}

void BaseTextEditor::moveLineUp()
{
    moveLineUpDown(true);
}

void BaseTextEditor::moveLineDown()
{
    moveLineUpDown(false);
}

void BaseTextEditor::moveLineUpDown(bool up)
{
    QTextCursor cursor = textCursor();
    QTextCursor move = cursor;
    move.beginEditBlock();

    bool hasSelection = cursor.hasSelection();

    if (cursor.hasSelection()) {
        move.setPosition(cursor.selectionStart());
        move.movePosition(QTextCursor::StartOfBlock);
        move.setPosition(cursor.selectionEnd(), QTextCursor::KeepAnchor);
        move.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    } else {
        move.movePosition(QTextCursor::StartOfBlock);
        move.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    }
    QString text = move.selectedText();
    move.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
    move.removeSelectedText();

    if (up) {
        move.movePosition(QTextCursor::PreviousBlock);
        move.insertBlock();
        move.movePosition(QTextCursor::Left);
    } else {
        move.movePosition(QTextCursor::EndOfBlock);
        if (move.atBlockStart()) { // empty block
            move.movePosition(QTextCursor::NextBlock);
            move.insertBlock();
            move.movePosition(QTextCursor::Left);
        } else {
            move.insertBlock();
        }
    }

    int start = move.position();
    move.clearSelection();
    move.insertText(text);
    int end = move.position();

    if (hasSelection) {
        move.setPosition(start);
        move.setPosition(end, QTextCursor::KeepAnchor);
    }

    indent(document(), move, QChar::Null);
    move.endEditBlock();

    setTextCursor(move);
}

void BaseTextEditor::cleanWhitespace()
{
    d->m_document->cleanWhitespace();
}

void BaseTextEditor::keyPressEvent(QKeyEvent *e)
{

    d->clearVisibleCollapsedBlock();

    QKeyEvent *original_e = e;
    d->m_lastEventWasBlockSelectionEvent = false;

    if (e->key() == Qt::Key_Escape) {
        e->accept();
        QTextCursor cursor = textCursor();
        cursor.clearSelection();
        setTextCursor(cursor);
        return;
    }

    d->m_contentsChanged = false;

    bool ro = isReadOnly();

    if (d->m_inBlockSelectionMode) {
        if (e == QKeySequence::Cut) {
            if (!ro) {
                cut();
                e->accept();
                return;
            }
        } else if (e == QKeySequence::Delete) {
            if (!ro) {
                d->removeBlockSelection();
                e->accept();
                return;
            }
        } else if (e == QKeySequence::Paste) {
            if (!ro) {
                d->removeBlockSelection();
                // continue
            }
        }
    }


    if (!ro
        && (e == QKeySequence::InsertParagraphSeparator
            || (!d->m_lineSeparatorsAllowed && e == QKeySequence::InsertLineSeparator))
        ) {

        QTextCursor cursor = textCursor();
        if (d->m_inBlockSelectionMode)
            cursor.clearSelection();
        if (d->m_document->tabSettings().m_autoIndent) {
            cursor.beginEditBlock();
            cursor.insertBlock();
            indent(document(), cursor, QChar::Null);
            cursor.endEditBlock();
        } else {
            cursor.insertBlock();
        }
        e->accept();
        setTextCursor(cursor);
        return;
    } else switch (e->key()) {


#if 0
    case Qt::Key_sterling: {

        static bool toggle = false;
        if ((toggle = !toggle)) {
            QList<BaseTextEditor::BlockRange> rangeList;
            rangeList += BaseTextEditor::BlockRange(4, 12);
            rangeList += BaseTextEditor::BlockRange(15, 19);
            setIfdefedOutBlocks(rangeList);
        } else {
            setIfdefedOutBlocks(QList<BaseTextEditor::BlockRange>());
        }
        e->accept();
        return;

    } break;
#endif
    case Qt::Key_Tab:
    case Qt::Key_Backtab:
        if (ro) break;
        indentOrUnindent(e->key() == Qt::Key_Tab);
        e->accept();
        return;
    case Qt::Key_Backspace:
        if (ro) break;
        if (d->m_document->tabSettings().m_smartBackspace
            && (e->modifiers() & (Qt::ControlModifier
                               | Qt::ShiftModifier
                               | Qt::AltModifier
                               | Qt::MetaModifier)) == Qt::NoModifier
            && !textCursor().hasSelection()) {
            handleBackspaceKey();
            e->accept();
            return;
        }
        break;
    case Qt::Key_Home:
        if (!(e == QKeySequence::MoveToStartOfDocument) && !(e == QKeySequence::SelectStartOfDocument)) {
            if ((e->modifiers() & (Qt::AltModifier | Qt::ShiftModifier)) == (Qt::AltModifier | Qt::ShiftModifier))
                d->m_lastEventWasBlockSelectionEvent = true;
            handleHomeKey(e->modifiers() & Qt::ShiftModifier);
            e->accept();
            return;
        }
        break;
    case Qt::Key_Up:
    case Qt::Key_Down:
        if (e->modifiers() & Qt::ControlModifier) {
            verticalScrollBar()->triggerAction(
                    e->key() == Qt::Key_Up ? QAbstractSlider::SliderSingleStepSub :
                                             QAbstractSlider::SliderSingleStepAdd);
            e->accept();
            return;
        }
        // fall through
    case Qt::Key_End:
    case Qt::Key_Right:
    case Qt::Key_Left:
#ifndef Q_OS_MAC
        if ((e->modifiers() & (Qt::AltModifier | Qt::ShiftModifier)) == (Qt::AltModifier | Qt::ShiftModifier)) {

            d->m_lastEventWasBlockSelectionEvent = true;

            if (d->m_inBlockSelectionMode) {
                if (e->key() == Qt::Key_Right && textCursor().atBlockEnd()) {
                    d->m_blockSelectionExtraX++;
                    viewport()->update();
                    e->accept();
                    return;
                } else if (e->key() == Qt::Key_Left && d->m_blockSelectionExtraX > 0) {
                    d->m_blockSelectionExtraX--;
                    e->accept();
                    viewport()->update();
                    return;
                }
            }

            e = new QKeyEvent(
                e->type(),
                e->key(),
                e->modifiers() & ~Qt::AltModifier,
                e->text(),
                e->isAutoRepeat(),
                e->count()
                );
        }
#endif
        break;
    case Qt::Key_PageUp:
    case Qt::Key_PageDown:
        if (e->modifiers() == Qt::ControlModifier) {
            verticalScrollBar()->triggerAction(
                    e->key() == Qt::Key_PageUp ? QAbstractSlider::SliderPageStepSub :
                                                 QAbstractSlider::SliderPageStepAdd);
            e->accept();
            return;
        }
        break;

    default:
        if (! ro && d->m_document->tabSettings().m_autoIndent
            && ! e->text().isEmpty() && isElectricCharacter(e->text().at(0))) {
            QTextCursor cursor = textCursor();
            const QString text = e->text();
            cursor.insertText(text);
            const QString leftText = cursor.block().text().left(cursor.position() - 1 - cursor.block().position());
            if (leftText.simplified().isEmpty()) {
                const QChar typedChar = e->text().at(0);
                indent(document(), cursor, typedChar);
            }
#if 0
            TextEditDocumentLayout *documentLayout = qobject_cast<TextEditDocumentLayout*>(document()->documentLayout());
            QTC_ASSERT(documentLayout, return);
            documentLayout->requestUpdate(); // a bit drastic
            e->accept();
#endif
            setTextCursor(cursor);
            return;
        }
        break;
    }

    if (d->m_inBlockSelectionMode) {
        QString text = e->text();
        if (!text.isEmpty() && (text.at(0).isPrint() || text.at(0) == QLatin1Char('\t'))) {
            d->removeBlockSelection(text);
            goto skip_event;
        }
    }

    QPlainTextEdit::keyPressEvent(e);

skip_event:
    if (!ro && e->key() == Qt::Key_Delete && d->m_parenthesesMatchingEnabled)
        slotCursorPositionChanged(); // parentheses matching

    if (!ro && d->m_contentsChanged && !e->text().isEmpty() && e->text().at(0).isPrint())
        emit requestAutoCompletion(editableInterface(), false);

    if (e != original_e)
        delete e;
}

void BaseTextEditor::setTextCursor(const QTextCursor &cursor)
{
    // workaround for QTextControl bug
    bool selectionChange = cursor.hasSelection() || textCursor().hasSelection();
    QPlainTextEdit::setTextCursor(cursor);
    if (selectionChange)
        slotSelectionChanged();
}

void BaseTextEditor::gotoLine(int line, int column)
{
    const int blockNumber = line - 1;
    const QTextBlock &block = document()->findBlockByNumber(blockNumber);
    if (block.isValid()) {
        QTextCursor cursor(block);
        if (column > 0) {
            cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, column);
        } else {
            int pos = cursor.position();
            while (characterAt(pos).category() == QChar::Separator_Space) {
                ++pos;
            }
            cursor.setPosition(pos);
        }
        setTextCursor(cursor);
        centerCursor();
    }
}

int BaseTextEditor::position(ITextEditor::PositionOperation posOp, int at) const
{
    QTextCursor tc = textCursor();

    if (at != -1)
        tc.setPosition(at);

    if (posOp == ITextEditor::Current)
        return tc.position();

    switch (posOp) {
    case ITextEditor::EndOfLine:
        tc.movePosition(QTextCursor::EndOfLine);
        return tc.position();
    case ITextEditor::StartOfLine:
        tc.movePosition(QTextCursor::StartOfLine);
        return tc.position();
    case ITextEditor::Anchor:
        if (tc.hasSelection())
            return tc.anchor();
        break;
    case ITextEditor::EndOfDoc:
        tc.movePosition(QTextCursor::End);
        return tc.position();
    default:
        break;
    }

    return -1;
}

void BaseTextEditor::convertPosition(int pos, int *line, int *column) const
{
    QTextBlock block = document()->findBlock(pos);
    if (!block.isValid()) {
        (*line) = -1;
        (*column) = -1;
    } else {
        (*line) = block.blockNumber() + 1;
        (*column) = pos - block.position();
    }
}

QChar BaseTextEditor::characterAt(int pos) const
{
    return document()->characterAt(pos);
}

bool BaseTextEditor::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::ShortcutOverride:
        e->ignore(); // we are a really nice citizen
        return true;
    default:
        break;
    }

    return QPlainTextEdit::event(e);
}

void BaseTextEditor::duplicateFrom(BaseTextEditor *editor)
{
    if (this == editor)
        return;
    setDisplayName(editor->displayName());
    d->m_revisionsVisible = editor->d->m_revisionsVisible;
    if (d->m_document == editor->d->m_document)
        return;
    d->setupDocumentSignals(editor->d->m_document);
    d->m_document = editor->d->m_document;
}

QString BaseTextEditor::displayName() const
{
    return d->m_displayName;
}

void BaseTextEditor::setDisplayName(const QString &title)
{
    d->m_displayName = title;
}

BaseTextDocument *BaseTextEditor::baseTextDocument() const
{
    return d->m_document;
}

void BaseTextEditor::setBaseTextDocument(BaseTextDocument *doc)
{
    if (doc) {
        d->setupDocumentSignals(doc);
        d->m_document = doc;
    }
}

void BaseTextEditor::memorizeCursorPosition()
{
    d->m_tempState = saveState();
}

void BaseTextEditor::restoreCursorPosition()
{
    restoreState(d->m_tempState);
}

QByteArray BaseTextEditor::saveState() const
{
    QByteArray state;
    QDataStream stream(&state, QIODevice::WriteOnly);
    stream << 0; // version number
    stream << verticalScrollBar()->value();
    stream << horizontalScrollBar()->value();
    int line, column;
    convertPosition(textCursor().position(), &line, &column);
    stream << line;
    stream << column;
    return state;
}

bool BaseTextEditor::restoreState(const QByteArray &state)
{
    int version;
    int vval;
    int hval;
    int lval;
    int cval;
    QDataStream stream(state);
    stream >> version;
    stream >> vval;
    stream >> hval;
    stream >> lval;
    stream >> cval;
    gotoLine(lval, cval);
    verticalScrollBar()->setValue(vval);
    horizontalScrollBar()->setValue(hval);
    return true;
}

void BaseTextEditor::setDefaultPath(const QString &defaultPath)
{
    baseTextDocument()->setDefaultPath(defaultPath);
}

void BaseTextEditor::setSuggestedFileName(const QString &suggestedFileName)
{
    baseTextDocument()->setSuggestedFileName(suggestedFileName);
}

void BaseTextEditor::setParenthesesMatchingEnabled(bool b)
{
    d->m_parenthesesMatchingEnabled = b;
}

bool BaseTextEditor::isParenthesesMatchingEnabled() const
{
    return d->m_parenthesesMatchingEnabled;
}

void BaseTextEditor::setHighlightCurrentLine(bool b)
{
    d->m_highlightCurrentLine = b;
    slotCursorPositionChanged();
}

bool BaseTextEditor::highlightCurrentLine() const
{
    return d->m_highlightCurrentLine;
}

void BaseTextEditor::setLineNumbersVisible(bool b)
{
    d->m_lineNumbersVisible = b;
    slotUpdateExtraAreaWidth();
}

bool BaseTextEditor::lineNumbersVisible() const
{
    return d->m_lineNumbersVisible;
}

void BaseTextEditor::setMarksVisible(bool b)
{
    d->m_marksVisible = b;
    slotUpdateExtraAreaWidth();
}

bool BaseTextEditor::marksVisible() const
{
    return d->m_marksVisible;
}

void BaseTextEditor::setRequestMarkEnabled(bool b)
{
    d->m_requestMarkEnabled = b;
}

bool BaseTextEditor::requestMarkEnabled() const
{
    return d->m_requestMarkEnabled;
}

void BaseTextEditor::setLineSeparatorsAllowed(bool b)
{
    d->m_lineSeparatorsAllowed = b;
}

bool BaseTextEditor::lineSeparatorsAllowed() const
{
    return d->m_lineSeparatorsAllowed;
}


void BaseTextEditor::setCodeFoldingVisible(bool b)
{
    d->m_codeFoldingVisible = b;
    slotUpdateExtraAreaWidth();
}

bool BaseTextEditor::codeFoldingVisible() const
{
    return d->m_codeFoldingVisible;
}

void BaseTextEditor::setRevisionsVisible(bool b)
{
    d->m_revisionsVisible = b;
    slotUpdateExtraAreaWidth();
}

bool BaseTextEditor::revisionsVisible() const
{
    return d->m_revisionsVisible;
}

void BaseTextEditor::setVisibleWrapColumn(int column)
{
    d->m_visibleWrapColumn = column;
    viewport()->update();
}

int BaseTextEditor::visibleWrapColumn() const
{
    return d->m_visibleWrapColumn;
}

void BaseTextEditor::setFontSettings(const TextEditor::FontSettings &fs)
{
    const QTextCharFormat textFormat = fs.toTextCharFormat(QLatin1String(Constants::C_TEXT));
    const QTextCharFormat selectionFormat = fs.toTextCharFormat(QLatin1String(Constants::C_SELECTION));
    const QTextCharFormat lineNumberFormat = fs.toTextCharFormat(QLatin1String(Constants::C_LINE_NUMBER));
    const QTextCharFormat searchResultFormat = fs.toTextCharFormat(QLatin1String(Constants::C_SEARCH_RESULT));
    const QTextCharFormat searchScopeFormat = fs.toTextCharFormat(QLatin1String(Constants::C_SEARCH_SCOPE));
    const QTextCharFormat parenthesesFormat = fs.toTextCharFormat(QLatin1String(Constants::C_PARENTHESES));
    const QTextCharFormat currentLineFormat = fs.toTextCharFormat(QLatin1String(Constants::C_CURRENT_LINE));
    const QTextCharFormat ifdefedOutFormat = fs.toTextCharFormat(QLatin1String(Constants::C_DISABLED_CODE));
    QFont font(textFormat.font());

    const QColor foreground = textFormat.foreground().color();
    const QColor background = textFormat.background().color();
    QPalette p = palette();
    p.setColor(QPalette::Text, foreground);
    p.setColor(QPalette::Foreground, foreground);
    p.setColor(QPalette::Base, background);
    p.setColor(QPalette::Highlight, (selectionFormat.background().style() != Qt::NoBrush) ?
               selectionFormat.background().color() :
               QApplication::palette().color(QPalette::Highlight));
    p.setColor(QPalette::HighlightedText, selectionFormat.foreground().color());
    p.setBrush(QPalette::Inactive, QPalette::Highlight, p.highlight());
    p.setBrush(QPalette::Inactive, QPalette::HighlightedText, p.highlightedText());
    setPalette(p);
    setFont(font);
    setTabSettings(d->m_document->tabSettings()); // update tabs, they depend on the font

    // Line numbers
    QPalette ep = d->m_extraArea->palette();
    ep.setColor(QPalette::Dark, lineNumberFormat.foreground().color());
    ep.setColor(QPalette::Background, lineNumberFormat.background().style() != Qt::NoBrush ?
                lineNumberFormat.background().color() : background);
    d->m_extraArea->setPalette(ep);

    // Search results
    d->m_searchResultFormat.setBackground(searchResultFormat.background());
    d->m_searchScopeFormat.setBackground(searchScopeFormat.background());
    d->m_currentLineFormat.setBackground(currentLineFormat.background());

    // Matching braces
    d->m_matchFormat.setForeground(parenthesesFormat.foreground());
    d->m_rangeFormat.setBackground(parenthesesFormat.background());

    // Disabled code
    d->m_ifdefedOutFormat.setForeground(ifdefedOutFormat.foreground());

    slotUpdateExtraAreaWidth();
}

void BaseTextEditor::setStorageSettings(const StorageSettings &storageSettings)
{
    d->m_document->setStorageSettings(storageSettings);
}

//--------- BaseTextEditorPrivate -----------

BaseTextEditorPrivate::BaseTextEditorPrivate()
    :
    m_contentsChanged(false),
    m_document(new BaseTextDocument()),
    m_parenthesesMatchingEnabled(false),
    m_extraArea(0),
    m_marksVisible(false),
    m_codeFoldingVisible(false),
    m_revisionsVisible(false),
    m_lineNumbersVisible(true),
    m_highlightCurrentLine(true),
    m_requestMarkEnabled(true),
    m_lineSeparatorsAllowed(false),
    m_visibleWrapColumn(0),
    m_editable(0),
    m_actionHack(0),
    m_inBlockSelectionMode(false),
    m_lastEventWasBlockSelectionEvent(false),
    m_blockSelectionExtraX(0)
{
}

BaseTextEditorPrivate::~BaseTextEditorPrivate()
{
}

void BaseTextEditorPrivate::setupDocumentSignals(BaseTextDocument *document)
{
    BaseTextDocument *oldDocument = q->baseTextDocument();
    if (oldDocument) {
        q->disconnect(oldDocument->document(), 0, q, 0);
        q->disconnect(oldDocument, 0, q, 0);
    }

    QTextDocument *doc = document->document();
    TextEditDocumentLayout *documentLayout = qobject_cast<TextEditDocumentLayout*>(doc->documentLayout());
    if (!documentLayout) {
        QTextOption opt = doc->defaultTextOption();
        opt.setTextDirection(Qt::LeftToRight);
        opt.setFlags(opt.flags() | QTextOption::IncludeTrailingSpaces
                | QTextOption::AddSpaceForLineAndParagraphSeparators
                );
        doc->setDefaultTextOption(opt);
        documentLayout = new TextEditDocumentLayout(doc);
        doc->setDocumentLayout(documentLayout);
    }


    q->setDocument(doc);
    QObject::connect(documentLayout, SIGNAL(updateBlock(QTextBlock)), q, SLOT(slotUpdateBlockNotify(QTextBlock)));
    QObject::connect(q, SIGNAL(requestBlockUpdate(QTextBlock)), documentLayout, SIGNAL(updateBlock(QTextBlock)));
    QObject::connect(doc, SIGNAL(modificationChanged(bool)), q, SIGNAL(changed()));
    QObject::connect(doc, SIGNAL(contentsChange(int,int,int)), q,
        SLOT(editorContentsChange(int,int,int)), Qt::DirectConnection);
    QObject::connect(document, SIGNAL(changed()), q, SIGNAL(changed()));
    QObject::connect(document, SIGNAL(titleChanged(QString)), q, SLOT(setDisplayName(const QString &)));
    QObject::connect(document, SIGNAL(aboutToReload()), q, SLOT(memorizeCursorPosition()));
    QObject::connect(document, SIGNAL(reloaded()), q, SLOT(restoreCursorPosition()));
    q->slotUpdateExtraAreaWidth();
}

#ifndef TEXTEDITOR_STANDALONE
bool BaseTextEditorPrivate::needMakeWritableCheck() const
{
    return !m_document->isModified()
            && !m_document->fileName().isEmpty()
            && m_document->isReadOnly();
}
#endif

bool Parenthesis::hasClosingCollapse(const Parentheses &parentheses)
{
    return closeCollapseAtPos(parentheses) >= 0;
}


int Parenthesis::closeCollapseAtPos(const Parentheses &parentheses)
{
    int depth = 0;
    for (int i = 0; i < parentheses.size(); ++i) {
        const Parenthesis &p = parentheses.at(i);
        if (p.chr == QLatin1Char('{') || p.chr == QLatin1Char('+')) {
            ++depth;
        } else if (p.chr == QLatin1Char('}') || p.chr == QLatin1Char('-')) {
            if (--depth < 0)
                return p.pos;
        }
    }
    return -1;
}

int Parenthesis::collapseAtPos(const Parentheses &parentheses, QChar *character)
{
    int result = -1;
    QChar c;

    int depth = 0;
    for (int i = 0; i < parentheses.size(); ++i) {
        const Parenthesis &p = parentheses.at(i);
        if (p.chr == QLatin1Char('{') || p.chr == QLatin1Char('+')) {
            if (depth == 0) {
                result = p.pos;
                c = p.chr;
            }
            ++depth;
        } else if (p.chr == QLatin1Char('}') || p.chr == QLatin1Char('-')) {
            if (--depth < 0)
                depth = 0;
            result = -1;
        }
    }
    if (result >= 0 && character)
        *character = c;
    return result;
}


int TextBlockUserData::collapseAtPos() const
{
    return Parenthesis::collapseAtPos(m_parentheses);
}


void TextEditDocumentLayout::setParentheses(const QTextBlock &block, const Parentheses &parentheses)
{
    if (parentheses.isEmpty()) {
        if (TextBlockUserData *userData = testUserData(block))
            userData->clearParentheses();
    } else {
        userData(block)->setParentheses(parentheses);
    }
}

Parentheses TextEditDocumentLayout::parentheses(const QTextBlock &block)
{
    if (TextBlockUserData *userData = testUserData(block))
        return userData->parentheses();
    return Parentheses();
}

bool TextEditDocumentLayout::hasParentheses(const QTextBlock &block)
{
    if (TextBlockUserData *userData = testUserData(block))
        return userData->hasParentheses();
    return false;
}


bool TextEditDocumentLayout::setIfdefedOut(const QTextBlock &block)
{
    return userData(block)->setIfdefedOut();
}

bool TextEditDocumentLayout::clearIfdefedOut(const QTextBlock &block)
{
    if (TextBlockUserData *userData = testUserData(block))
        return userData->clearIfdefedOut();
    return false;
}

bool TextEditDocumentLayout::ifdefedOut(const QTextBlock &block)
{
    if (TextBlockUserData *userData = testUserData(block))
        return userData->ifdefedOut();
    return false;
}


TextEditDocumentLayout::TextEditDocumentLayout(QTextDocument *doc)
    :QPlainTextDocumentLayout(doc) {
    lastSaveRevision = 0;
    hasMarks = 0;
}

TextEditDocumentLayout::~TextEditDocumentLayout()
{
}

QRectF TextEditDocumentLayout::blockBoundingRect(const QTextBlock &block) const
{
    QRectF r = QPlainTextDocumentLayout::blockBoundingRect(block);
    return r;
}


bool BaseTextEditor::viewportEvent(QEvent *event)
{
    if (event->type() == QEvent::ContextMenu) {
        const QContextMenuEvent *ce = static_cast<QContextMenuEvent*>(event);
        if (ce->reason() == QContextMenuEvent::Mouse && !textCursor().hasSelection())
            setTextCursor(cursorForPosition(ce->pos()));
    } else if (event->type() == QEvent::ToolTip) {
        const QHelpEvent *he = static_cast<QHelpEvent*>(event);
        const QPoint &pos = he->pos();

        // Allow plugins to show tooltips
        const QTextCursor &c = cursorForPosition(pos);
        QPoint cursorPos = mapToGlobal(cursorRect(c).bottomRight() + QPoint(1,1));
        cursorPos.setX(cursorPos.x() + d->m_extraArea->width());

        editableInterface(); // create if necessary

        emit d->m_editable->tooltipRequested(editableInterface(), cursorPos, c.position());
        return true;
    }
    return QPlainTextEdit::viewportEvent(event);
}


void BaseTextEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);
    QRect cr = viewport()->rect();
    d->m_extraArea->setGeometry(
        QStyle::visualRect(layoutDirection(), cr,
                           QRect(cr.left(), cr.top(), extraAreaWidth(), cr.height())));
}

QRect BaseTextEditor::collapseBox(const QTextBlock &block)
{
    QRectF br = blockBoundingGeometry(block).translated(contentOffset());
    int collapseBoxWidth = fontMetrics().lineSpacing() + 1;
    return QRect(d->m_extraArea->width() - collapseBoxWidth + collapseBoxWidth/4,
                 int(br.top()) + collapseBoxWidth/4,
                  2 * (collapseBoxWidth/4) + 1, 2 * (collapseBoxWidth/4) + 1);

}

QTextBlock BaseTextEditor::collapsedBlockAt(const QPoint &pos, QRect *box) const {
    QPointF offset(contentOffset());
    QTextBlock block = firstVisibleBlock();
    int top = (int)blockBoundingGeometry(block).translated(offset).top();
    int bottom = top + (int)blockBoundingRect(block).height();

    int viewportHeight = viewport()->height();

    while (block.isValid() && top <= viewportHeight) {
        QTextBlock nextBlock = block.next();
        if (block.isVisible() && bottom >= 0) {
            if (nextBlock.isValid() && !nextBlock.isVisible()) {
                QTextLayout *layout = block.layout();
                QTextLine line = layout->lineAt(layout->lineCount()-1);
                QRectF lineRect = line.naturalTextRect().translated(offset.x(), top);
                lineRect.adjust(0, 0, -1, -1);

                QRectF collapseRect(lineRect.right() + 12,
                                    lineRect.top(),
                                    fontMetrics().width(QLatin1String(" {...}; ")),
                                    lineRect.height());
                if (collapseRect.contains(pos)) {
                    QTextBlock result = block;
                    if (box)
                        *box = collapseRect.toAlignedRect();
                    return result;
                } else {
                    block = nextBlock;
                    while (nextBlock.isValid() && !nextBlock.isVisible()) {
                        block = nextBlock;
                        nextBlock = block.next();
                    }
                }
            }
        }

        block = nextBlock;
        top = bottom;
        bottom = top + (int)blockBoundingRect(block).height();
    }
    return QTextBlock();
}

void BaseTextEditorPrivate::highlightSearchResults(const QTextBlock &block,
                                                   QVector<QTextLayout::FormatRange> *selections)
{
    if (m_searchExpr.isEmpty())
        return;

    QString text = block.text();
    text.replace(QChar::Nbsp, QLatin1Char(' '));
    int idx = -1;
    while (idx < text.length()) {
        idx = m_searchExpr.indexIn(text, idx + 1);
        if (idx < 0)
            break;
        int l = m_searchExpr.matchedLength();
        if ((m_findFlags & QTextDocument::FindWholeWords)
            && ((idx && text.at(idx-1).isLetterOrNumber())
                || (idx + l < text.length() && text.at(idx + l).isLetterOrNumber())))
            continue;

        if (m_findScope.isNull()
            || (block.position() + idx >= m_findScope.selectionStart()
                && block.position() + idx + l <= m_findScope.selectionEnd())) {
            QTextLayout::FormatRange selection;
            selection.start = idx;
            selection.length = l;
            selection.format = m_searchResultFormat;
            selections->append(selection);
        }
    }
}


namespace TextEditor {
    namespace Internal {
        struct BlockSelectionData {
            int selectionIndex;
            int selectionStart;
            int selectionEnd;
            int firstColumn;
            int lastColumn;
        };
    }
}

void BaseTextEditorPrivate::clearBlockSelection()
{
    if (m_inBlockSelectionMode) {
        m_inBlockSelectionMode = false;
        QTextCursor cursor = q->textCursor();
        cursor.clearSelection();
        q->setTextCursor(cursor);
    }
}

QString BaseTextEditorPrivate::copyBlockSelection()
{
    QString text;

    QTextCursor cursor = q->textCursor();
    if (!cursor.hasSelection())
        return text;

    QTextDocument *doc = q->document();
    int start = cursor.selectionStart();
    int end = cursor.selectionEnd();
    QTextBlock startBlock = doc->findBlock(start);
    int columnA = start - startBlock.position();
    QTextBlock endBlock = doc->findBlock(end);
    int columnB = end - endBlock.position();
    int firstColumn = qMin(columnA, columnB);
    int lastColumn = qMax(columnA, columnB) + m_blockSelectionExtraX;

    QTextBlock block = startBlock;
    for (;;) {

        cursor.setPosition(block.position() + qMin(block.length()-1, firstColumn));
        cursor.setPosition(block.position() + qMin(block.length()-1, lastColumn), QTextCursor::KeepAnchor);
        text += cursor.selectedText();
        if (block == endBlock)
            break;
        text += QLatin1Char('\n');
        block = block.next();
    }

    return text;
}

void BaseTextEditorPrivate::removeBlockSelection(const QString &text)
{
    QTextCursor cursor = q->textCursor();
    if (!cursor.hasSelection())
        return;

    QTextDocument *doc = q->document();
    int start = cursor.selectionStart();
    int end = cursor.selectionEnd();
    QTextBlock startBlock = doc->findBlock(start);
    int columnA = start - startBlock.position();
    QTextBlock endBlock = doc->findBlock(end);
    int columnB = end - endBlock.position();
    int firstColumn = qMin(columnA, columnB);
    int lastColumn = qMax(columnA, columnB) + m_blockSelectionExtraX;

    cursor.clearSelection();
    cursor.beginEditBlock();

    QTextBlock block = startBlock;
    for (;;) {

        cursor.setPosition(block.position() + qMin(block.length()-1, firstColumn));
        cursor.setPosition(block.position() + qMin(block.length()-1, lastColumn), QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
        if (block == endBlock)
            break;
        block = block.next();
    }

    cursor.setPosition(start);
    if (!text.isEmpty())
        cursor.insertText(text);
    cursor.endEditBlock();
    q->setTextCursor(cursor);
}

void BaseTextEditor::paintEvent(QPaintEvent *e)
{
    /*
      Here comes an almost verbatim copy of
      QPlainTextEdit::paintEvent() so we can adjust the extra
      selections dynamically to indicate all search results.
    */
    //begin QPlainTextEdit::paintEvent()

    QPainter painter(viewport());
    QTextDocument *doc = document();
    TextEditDocumentLayout *documentLayout = qobject_cast<TextEditDocumentLayout*>(doc->documentLayout());
    QTC_ASSERT(documentLayout, return);

    QPointF offset(contentOffset());

    QRect er = e->rect();
    QRect viewportRect = viewport()->rect();

    const QColor baseColor = palette().base().color();
    const int blendBase = (baseColor.value() > 128) ? 0 : 255;
    // Darker backgrounds may need a bit more contrast
    // (this calculation is temporary solution until we have a setting for this color)
    const int blendFactor = (baseColor.value() > 128) ? 8 : 16;
    const QColor blendColor(
        (blendBase * blendFactor + baseColor.blue() * (256 - blendFactor)) / 256,
        (blendBase * blendFactor + baseColor.green() * (256 - blendFactor)) / 256,
        (blendBase * blendFactor + baseColor.blue() * (256 - blendFactor)) / 256);
    if (d->m_visibleWrapColumn > 0) {
        qreal lineX = fontMetrics().averageCharWidth() * d->m_visibleWrapColumn + offset.x() + 4;
        painter.fillRect(QRectF(lineX, 0, viewportRect.width() - lineX, viewportRect.height()), blendColor);
    }

    // keep right margin clean from full-width selection
    int maxX = offset.x() + qMax((qreal)viewportRect.width(), documentLayout->documentSize().width())
               - doc->documentMargin();
    er.setRight(qMin(er.right(), maxX));
    painter.setClipRect(er);

    bool editable = !isReadOnly();

    QTextBlock block = firstVisibleBlock();

    QAbstractTextDocumentLayout::PaintContext context = getPaintContext();

    if (!d->m_findScope.isNull()) {
        QAbstractTextDocumentLayout::Selection selection;
        selection.format.setBackground(d->m_searchScopeFormat.background());
        selection.cursor = d->m_findScope;
        context.selections.prepend(selection);
    }

    BlockSelectionData *blockSelection = 0;

    if (d->m_inBlockSelectionMode
        && context.selections.count() && context.selections.last().cursor == textCursor()) {
        blockSelection = new BlockSelectionData;
        blockSelection->selectionIndex = context.selections.size()-1;
        const QAbstractTextDocumentLayout::Selection &selection = context.selections[blockSelection->selectionIndex];
        int start = blockSelection->selectionStart = selection.cursor.selectionStart();
        int end = blockSelection->selectionEnd = selection.cursor.selectionEnd();
        QTextBlock block = doc->findBlock(start);
        int columnA = start - block.position();
        block = doc->findBlock(end);
        int columnB = end - block.position();
        blockSelection->firstColumn = qMin(columnA, columnB);
        blockSelection->lastColumn = qMax(columnA, columnB) + d->m_blockSelectionExtraX;
    }

    QTextBlock visibleCollapsedBlock;
    QPointF visibleCollapsedBlockOffset;

    while (block.isValid()) {

        if (!block.isVisible()) {
            block = block.next();
            continue;
        }

        QRectF r = blockBoundingRect(block).translated(offset);
        QTextLayout *layout = block.layout();

        QTextOption option = layout->textOption();
        if (TextEditDocumentLayout::ifdefedOut(block)) {
            option.setFlags(option.flags() | QTextOption::SuppressColors);
            painter.setPen(d->m_ifdefedOutFormat.foreground().color());
        } else {
            option.setFlags(option.flags() & ~QTextOption::SuppressColors);
            painter.setPen(context.palette.text().color());
        }
        layout->setTextOption(option);

        if (r.bottom() >= er.top() && r.top() <= er.bottom()) {

            int blpos = block.position();
            int bllen = block.length();

            QVector<QTextLayout::FormatRange> selections;
            QVector<QTextLayout::FormatRange> selectionsWithText;

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
                    if (blockSelection && blockSelection->selectionIndex == i) {
                        o.start = qMin(blockSelection->firstColumn, bllen-1);
                        o.length = qMin(blockSelection->lastColumn, bllen-1) - o.start;
                    }
                    if (o.format.foreground().style() != Qt::NoBrush)
                        selectionsWithText.append(o);
                    else
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
                    if (o.format.foreground().style() != Qt::NoBrush)
                        selectionsWithText.append(o);
                    else
                        selections.append(o);
                }
            }
            d->highlightSearchResults(block, &selections);
            selections += selectionsWithText;

            bool drawCursor = ((editable || true) // we want the cursor in read-only mode
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
        if (!block.isVisible()) {
            if (block.blockNumber() == d->visibleCollapsedBlockNumber) {
                visibleCollapsedBlock = block;
                visibleCollapsedBlockOffset = offset;
            }

            // invisible blocks do have zero line count
            block = doc->findBlockByLineNumber(block.firstLineNumber());
        }

    }

    if (backgroundVisible() && !block.isValid() && offset.y() <= er.bottom()
        && (centerOnScroll() || verticalScrollBar()->maximum() == verticalScrollBar()->minimum())) {
        painter.fillRect(QRect(QPoint((int)er.left(), (int)offset.y()), er.bottomRight()), palette().background());
    }

    //end QPlainTextEdit::paintEvent()

    delete blockSelection;

    offset = contentOffset();
    block = firstVisibleBlock();

    int top = (int)blockBoundingGeometry(block).translated(offset).top();
    int bottom = top + (int)blockBoundingRect(block).height();

    QTextCursor cursor = textCursor();
    bool hasSelection = cursor.hasSelection();
    int selectionStart = cursor.selectionStart();
    int selectionEnd = cursor.selectionEnd();


    while (block.isValid() && top <= e->rect().bottom()) {
        QTextBlock nextBlock = block.next();
        QTextBlock nextVisibleBlock = nextBlock;

        if (!nextVisibleBlock.isVisible())
            // invisible blocks do have zero line count
            nextVisibleBlock = doc->findBlockByLineNumber(nextVisibleBlock.firstLineNumber());
        if (block.isVisible() && bottom >= e->rect().top()) {
            if (d->m_displaySettings.m_visualizeWhitespace) {
                QTextLayout *layout = block.layout();
                int lineCount = layout->lineCount();
                if (lineCount >= 2 || !nextBlock.isValid()) {
                    painter.save();
                    painter.setPen(Qt::lightGray);
                    for (int i = 0; i < lineCount-1; ++i) { // paint line wrap indicator
                        QTextLine line = layout->lineAt(i);
                        QRectF lineRect = line.naturalTextRect().translated(offset.x(), top);
                        QChar visualArrow((ushort)0x21b5);
                        painter.drawText(static_cast<int>(lineRect.right()),
                                         static_cast<int>(lineRect.top() + line.ascent()), visualArrow);
                    }
                    if (!nextBlock.isValid()) { // paint EOF symbol
                        QTextLine line = layout->lineAt(lineCount-1);
                        QRectF lineRect = line.naturalTextRect().translated(offset.x(), top);
                        int h = 4;
                        lineRect.adjust(0, 0, -1, -1);
                        QPainterPath path;
                        QPointF pos(lineRect.topRight() + QPointF(h+4, line.ascent()));
                        path.moveTo(pos);
                        path.lineTo(pos + QPointF(-h, -h));
                        path.lineTo(pos + QPointF(0, -2*h));
                        path.lineTo(pos + QPointF(h, -h));
                        path.closeSubpath();
                        painter.setBrush(painter.pen().color());
                        painter.drawPath(path);
                    }
                    painter.restore();
                }
            }

            if (nextBlock.isValid() && !nextBlock.isVisible()) {

                bool selectThis = (hasSelection
                                   && nextBlock.position() >= selectionStart
                                   && nextBlock.position() < selectionEnd);
                if (selectThis) {
                    painter.save();
                    painter.setBrush(palette().highlight());
                }

                QTextLayout *layout = block.layout();
                QTextLine line = layout->lineAt(layout->lineCount()-1);
                QRectF lineRect = line.naturalTextRect().translated(offset.x(), top);
                lineRect.adjust(0, 0, -1, -1);

                QRectF collapseRect(lineRect.right() + 12,
                                    lineRect.top(),
                                    fontMetrics().width(QLatin1String(" {...}; ")),
                                    lineRect.height());
                painter.setRenderHint(QPainter::Antialiasing, true);
                painter.translate(.5, .5);
                painter.drawRoundedRect(collapseRect.adjusted(0, 0, 0, -1), 3, 3);
                painter.setRenderHint(QPainter::Antialiasing, false);
                painter.translate(-.5, -.5);

                QString replacement = QLatin1String("...");

                QTextBlock info = block;
                if (block.userData()
                    && static_cast<TextBlockUserData*>(block.userData())->collapseMode() == TextBlockUserData::CollapseAfter)
                    ;
                else if (block.next().userData()
                         && static_cast<TextBlockUserData*>(block.next().userData())->collapseMode()
                         == TextBlockUserData::CollapseThis) {
                    replacement.prepend(nextBlock.text().trimmed().left(1));
                    info = nextBlock;
                }


                block = nextVisibleBlock.previous();
                if (!block.isValid())
                    block = doc->lastBlock();

                if (info.userData()
                    && static_cast<TextBlockUserData*>(info.userData())->collapseIncludesClosure()) {
                    QString right = block.text().trimmed();
                    if (right.endsWith(QLatin1Char(';'))) {
                        right.chop(1);
                        right = right.trimmed();
                        replacement.append(right.right(right.endsWith(QLatin1Char('/')) ? 2 : 1));
                        replacement.append(QLatin1Char(';'));
                    } else {
                        replacement.append(right.right(right.endsWith(QLatin1Char('/')) ? 2 : 1));
                    }
                }
                if (selectThis)
                    painter.setPen(palette().highlightedText().color());
                painter.drawText(collapseRect, Qt::AlignCenter, replacement);
                if (selectThis)
                    painter.restore();
            }
        }

        block = nextVisibleBlock;
        top = bottom;
        bottom = top + (int)blockBoundingRect(block).height();
    }

    if (visibleCollapsedBlock.isValid() ) {
        int margin = doc->documentMargin();
        qreal maxWidth = 0;
        qreal blockHeight = 0;
        QTextBlock b = visibleCollapsedBlock;

        while (!b.isVisible() && visibleCollapsedBlockOffset.y() + blockHeight <= e->rect().bottom()) {
            b.setVisible(true); // make sure block bounding rect works
            QRectF r = blockBoundingRect(b).translated(visibleCollapsedBlockOffset);

            QTextLayout *layout = b.layout();
            for (int i = layout->lineCount()-1; i >= 0; --i)
                maxWidth = qMax(maxWidth, layout->lineAt(i).naturalTextWidth() + margin);

            blockHeight += r.height();

            b.setVisible(false); // restore previous state
            b = b.next();
        }

        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.translate(.5, .5);
        QColor color = blendColor;
//        color.setAlpha(240); // someone thinks alpha blending looks messy
        painter.setBrush(color);
        painter.drawRoundedRect(QRectF(visibleCollapsedBlockOffset.x(),
                                       visibleCollapsedBlockOffset.y(),
                                       maxWidth, blockHeight).adjusted(0, 0, 1, 1), 3, 3);
        painter.restore();

        QTextBlock end = b;
        b = visibleCollapsedBlock;
        while (b != end) {
            b.setVisible(true); // make sure block bounding rect works
            QRectF r = blockBoundingRect(b).translated(visibleCollapsedBlockOffset);
            QTextLayout *layout = b.layout();
            QVector<QTextLayout::FormatRange> selections;
            d->highlightSearchResults(b, &selections);
            layout->draw(&painter, visibleCollapsedBlockOffset, selections, er);

            b.setVisible(false); // restore previous state
            visibleCollapsedBlockOffset.ry() += r.height();
            b = b.next();
        }
    }


    if (d->m_visibleWrapColumn > 0) {
        qreal lineX = fontMetrics().width('x') * d->m_visibleWrapColumn + offset.x() + 4;
        const QColor bg = palette().base().color();
        QColor col = (bg.value() > 128) ? Qt::black : Qt::white;
        col.setAlpha(32);
        painter.setPen(QPen(col, 0));
        painter.drawLine(QPointF(lineX, 0), QPointF(lineX, viewport()->height()));
    }
}

void BaseTextEditor::slotUpdateExtraAreaWidth()
{
    if (isLeftToRight())
        setViewportMargins(extraAreaWidth(), 0, 0, 0);
    else
        setViewportMargins(0, 0, extraAreaWidth(), 0);
}


QWidget *BaseTextEditor::extraArea() const
{
    return d->m_extraArea;
}

int BaseTextEditor::extraAreaWidth(int *markWidthPtr) const
{
    TextEditDocumentLayout *documentLayout = qobject_cast<TextEditDocumentLayout*>(document()->documentLayout());
    if (!documentLayout)
        return 0;

    if (!d->m_marksVisible && documentLayout->hasMarks)
        d->m_marksVisible = true;

    int space = 0;
    const QFontMetrics fm(d->m_extraArea->fontMetrics());

    if (d->m_lineNumbersVisible) {
        int digits = 2;
        int max = qMax(1, blockCount());
        while (max >= 100) {
            max /= 10;
            ++digits;
        }
        space += fm.width(QLatin1Char('9')) * digits;
    }
    int markWidth = 0;

    if (d->m_marksVisible) {
        markWidth += fm.lineSpacing();
//     if (documentLayout->doubleMarkCount)
//         markWidth += fm.lineSpacing() / 3;
        space += markWidth;
    } else {
        space += 2;
    }

    if (markWidthPtr)
        *markWidthPtr = markWidth;

    space += 4;

    if (d->m_codeFoldingVisible)
        space += fm.lineSpacing();
    return space;
}

void BaseTextEditor::slotModificationChanged(bool m)
{
    if (m)
        return;

    QTextDocument *doc = document();
    TextEditDocumentLayout *documentLayout = qobject_cast<TextEditDocumentLayout*>(doc->documentLayout());
    QTC_ASSERT(documentLayout, return);
    int oldLastSaveRevision = documentLayout->lastSaveRevision;
    documentLayout->lastSaveRevision = doc->revision();

    if (oldLastSaveRevision != documentLayout->lastSaveRevision) {
        QTextBlock block = doc->begin();
        while (block.isValid()) {
            if (block.revision() < 0 || block.revision() != oldLastSaveRevision) {
                block.setRevision(-documentLayout->lastSaveRevision - 1);
            } else {
                block.setRevision(documentLayout->lastSaveRevision);
            }
            block = block.next();
        }
    }
    d->m_extraArea->update();
}

void BaseTextEditor::slotUpdateBlockNotify(const QTextBlock &block)
{
    static bool blockRecursion = false;
    if (blockRecursion)
        return;
    if (block.previous().isValid() && block.userState() != block.previous().userState()) {
        /* The syntax highlighting state changes. This opens up for
           the possibility that the paragraph has braces that support
           code folding. In this case, do the save thing and also
           update the previous block, which might contain a collapse
           box which now is invalid.*/
        blockRecursion = true;
        emit requestBlockUpdate(block.previous());
        blockRecursion = false;
    }
}

void BaseTextEditor::slotUpdateRequest(const QRect &r, int dy)
{
    if (dy)
        d->m_extraArea->scroll(0, dy);
    else if (r.width() > 4) { // wider than cursor width, not just cursor blinking
        d->m_extraArea->update(0, r.y(), d->m_extraArea->width(), r.height());
    }

    if (r.contains(viewport()->rect()))
        slotUpdateExtraAreaWidth();
}


void BaseTextEditor::setCollapseIndicatorAlpha(int alpha)
{
    d->extraAreaCollapseAlpha = alpha;
    d->m_extraArea->update();
}

void BaseTextEditor::extraAreaPaintEvent(QPaintEvent *e)
{
    QTextDocument *doc = document();
    TextEditDocumentLayout *documentLayout = qobject_cast<TextEditDocumentLayout*>(doc->documentLayout());
    QTC_ASSERT(documentLayout, return);

    QPalette pal = d->m_extraArea->palette();
    pal.setCurrentColorGroup(QPalette::Active);
    QPainter painter(d->m_extraArea);
    QFontMetrics fm(painter.fontMetrics());
    int fmLineSpacing = fm.lineSpacing();

    int markWidth = 0;
    if (d->m_marksVisible)
        markWidth += fm.lineSpacing();
//     if (documentLayout->doubleMarkCount)
//         markWidth += fm.lineSpacing() / 3;

    const int collapseBoxWidth = d->m_codeFoldingVisible ? fmLineSpacing + 1: 0;
    const int extraAreaWidth = d->m_extraArea->width() - collapseBoxWidth;

    painter.fillRect(e->rect(), pal.color(QPalette::Base));
    painter.fillRect(e->rect().intersected(QRect(0, 0, extraAreaWidth, INT_MAX)),
                     pal.color(QPalette::Background));


    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int)blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top;

    int extraAreaHighlightCollapseEndBlockNumber = -1;

    int extraAreaHighlightCollapseBlockNumber = d->extraAreaHighlightCollapseBlockNumber;
    if (extraAreaHighlightCollapseBlockNumber < 0) {
        extraAreaHighlightCollapseBlockNumber = d->extraAreaHighlightFadingBlockNumber;
    }

    if (extraAreaHighlightCollapseBlockNumber >= 0 ) {
        QTextBlock highlightBlock = doc->findBlockByNumber(extraAreaHighlightCollapseBlockNumber);
        if (highlightBlock.isValid() && highlightBlock.next().isValid() && highlightBlock.next().isVisible())
            extraAreaHighlightCollapseEndBlockNumber = TextBlockUserData::testCollapse(highlightBlock).blockNumber();
        else
            extraAreaHighlightCollapseEndBlockNumber = extraAreaHighlightCollapseBlockNumber;
    }


    while (block.isValid() && top <= e->rect().bottom()) {

        bool collapseThis = false;
        bool collapseAfter = false;
        bool hasClosingCollapse = false;


        top = bottom;
        bottom = top + (int)blockBoundingRect(block).height();
        QTextBlock nextBlock = block.next();

        QTextBlock nextVisibleBlock = nextBlock;
        int nextVisibleBlockNumber = blockNumber + 1;

        if (!nextVisibleBlock.isVisible()) {
            // invisible blocks do have zero line count
            nextVisibleBlock = doc->findBlockByLineNumber(nextVisibleBlock.firstLineNumber());
            nextVisibleBlockNumber = nextVisibleBlock.blockNumber();
        }

        painter.setPen(pal.color(QPalette::Dark));

        if (d->m_codeFoldingVisible || d->m_marksVisible) {
            painter.save();
            painter.setRenderHint(QPainter::Antialiasing, false);

            int previousBraceDepth = block.previous().userState();
            if (previousBraceDepth >= 0)
                previousBraceDepth >>= 8;
            else
                previousBraceDepth = 0;

            int braceDepth = block.userState();
            if (!nextBlock.isVisible()) {
                QTextBlock lastInvisibleBlock = nextVisibleBlock.previous();
                if (!lastInvisibleBlock.isValid())
                    lastInvisibleBlock = doc->lastBlock();
                braceDepth = lastInvisibleBlock.userState();
            }
            if (braceDepth >= 0)
                braceDepth >>= 8;
            else
                braceDepth = 0;

            if (TextBlockUserData *userData = static_cast<TextBlockUserData*>(block.userData())) {
                if (d->m_marksVisible) {
                    int xoffset = 0;
                    foreach (ITextMark *mrk, userData->marks()) {
                        int x = 0;
                        int radius = fmLineSpacing - 1;
                        QRect r(x + xoffset, top, radius, radius);
                        mrk->icon().paint(&painter, r, Qt::AlignCenter);
                        xoffset += 2;
                    }
                }

                if (!userData->ifdefedOut()) {
                    collapseAfter = (userData->collapseMode() == TextBlockUserData::CollapseAfter);
                    collapseThis = (userData->collapseMode() == TextBlockUserData::CollapseThis);
                    hasClosingCollapse = userData->hasClosingCollapse() && (previousBraceDepth > 0);
                }
            }

            if (d->m_codeFoldingVisible) {
                const QRect box(extraAreaWidth + collapseBoxWidth/4, top + collapseBoxWidth/4,
                                2 * (collapseBoxWidth/4) + 1, 2 * (collapseBoxWidth/4) + 1);
                const QPoint boxCenter = box.center();

                QColor textColorAlpha = pal.text().color();
                textColorAlpha.setAlpha(d->extraAreaCollapseAlpha);
                QColor textColorInactive = pal.text().color();
                textColorInactive.setAlpha(100);
                QColor textColor = pal.text().color();
                textColor.setAlpha(qMax(textColorInactive.alpha(), d->extraAreaCollapseAlpha));

                const QPen pen( (blockNumber >= extraAreaHighlightCollapseBlockNumber
                                 && blockNumber <= extraAreaHighlightCollapseEndBlockNumber) ?
                                textColorAlpha : pal.base().color());
                const QPen boxPen((blockNumber == extraAreaHighlightCollapseBlockNumber) ?
                                  textColor : textColorInactive);
                const QPen endPen((blockNumber == extraAreaHighlightCollapseEndBlockNumber) ?
                                  textColorAlpha : pal.base().color());
                const QPen previousPen((blockNumber-1 >= extraAreaHighlightCollapseBlockNumber
                                        && blockNumber-1 < extraAreaHighlightCollapseEndBlockNumber) ?
                                       textColorAlpha : pal.base().color());
                const QPen nextPen((blockNumber+1 > extraAreaHighlightCollapseBlockNumber
                                    && blockNumber+1 <= extraAreaHighlightCollapseEndBlockNumber) ?
                                   textColorAlpha : pal.base().color());

                TextBlockUserData *nextBlockUserData = TextEditDocumentLayout::testUserData(nextBlock);

                bool collapseNext = nextBlockUserData
                                    && nextBlockUserData->collapseMode()
                                    == TextBlockUserData::CollapseThis
                                    && !nextBlockUserData->ifdefedOut();

                bool nextHasClosingCollapse = nextBlockUserData
                                              && nextBlockUserData->hasClosingCollapseInside()
                                              && nextBlockUserData->ifdefedOut();

                bool drawBox = ((collapseAfter || collapseNext) && !nextHasClosingCollapse);

                if (braceDepth || (collapseNext && nextBlock.isVisible())) {
                    painter.setPen((hasClosingCollapse || !nextBlock.isVisible())? nextPen : pen);
                    painter.drawLine(boxCenter.x(), boxCenter.y(), boxCenter.x(), bottom - 1);
                }

                if (previousBraceDepth || collapseThis) {
                    painter.setPen((collapseAfter || collapseNext) ? previousPen : pen);
                    painter.drawLine(boxCenter.x(), top, boxCenter.x(), boxCenter.y());
                }

                if (drawBox) {
                    painter.setPen(boxPen);
                    painter.setBrush(pal.base());
                    painter.drawRect(box.adjusted(0, 0, -1, -1));
                    if (!nextBlock.isVisible())
                        painter.drawLine(boxCenter.x(), box.top() + 2, boxCenter.x(), box.bottom() - 2);
                    painter.drawLine(box.left() + 2, boxCenter.y(), box.right() - 2, boxCenter.y());
                } else if (hasClosingCollapse || collapseAfter || collapseNext) {
                    painter.setPen(endPen);
                    painter.drawLine(boxCenter.x() + 1, boxCenter.y(), box.right() - 1, boxCenter.y());
                }

            }

            painter.restore();
        }


        if (d->m_revisionsVisible && block.revision() != documentLayout->lastSaveRevision) {
            painter.save();
            painter.setRenderHint(QPainter::Antialiasing, false);
            if (block.revision() < 0)
                painter.setPen(QPen(Qt::darkGreen, 2));
            else
                painter.setPen(QPen(Qt::red, 2));
            painter.drawLine(extraAreaWidth-1, top, extraAreaWidth-1, bottom-1);
            painter.restore();
        }

        if (d->m_lineNumbersVisible) {
            const QString &number = QString::number(blockNumber + 1);
            painter.drawText(markWidth, top, extraAreaWidth - markWidth - 4, fm.height(), Qt::AlignRight, number);
        }

        block = nextVisibleBlock;
        blockNumber = nextVisibleBlockNumber;
    }
}

void BaseTextEditor::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == d->autoScrollTimer.timerId()) {
        const QPoint globalPos = QCursor::pos();
        const QPoint pos = d->m_extraArea->mapFromGlobal(globalPos);
        QRect visible = d->m_extraArea->rect();
        verticalScrollBar()->triggerAction( pos.y() < visible.center().y() ?
                                            QAbstractSlider::SliderSingleStepSub
                                            : QAbstractSlider::SliderSingleStepAdd);
        QMouseEvent ev(QEvent::MouseMove, pos, globalPos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        extraAreaMouseEvent(&ev);
        int delta = qMax(pos.y() - visible.top(), visible.bottom() - pos.y()) - visible.height();
        if (delta < 7)
            delta = 7;
        int timeout = 4900 / (delta * delta);
        d->autoScrollTimer.start(timeout, this);

    } else if (e->timerId() == d->collapsedBlockTimer.timerId()) {
        d->visibleCollapsedBlockNumber = d->suggestedVisibleCollapsedBlockNumber;
        d->suggestedVisibleCollapsedBlockNumber = -1;
        d->collapsedBlockTimer.stop();
        viewport()->update();
    }
    QPlainTextEdit::timerEvent(e);
}


void BaseTextEditorPrivate::clearVisibleCollapsedBlock()
{
    if (suggestedVisibleCollapsedBlockNumber) {
        suggestedVisibleCollapsedBlockNumber = -1;
        collapsedBlockTimer.stop();
    }
    if (visibleCollapsedBlockNumber >= 0) {
        visibleCollapsedBlockNumber = -1;
        q->viewport()->update();
    }
}


void BaseTextEditor::mouseMoveEvent(QMouseEvent *e)
{
    d->m_lastEventWasBlockSelectionEvent = (e->modifiers() & Qt::AltModifier);
    if (e->buttons() == 0) {
        QTextBlock collapsedBlock = collapsedBlockAt(e->pos());
        int blockNumber = collapsedBlock.next().blockNumber();
        if (blockNumber < 0) {
            d->clearVisibleCollapsedBlock();
        } else if (blockNumber != d->visibleCollapsedBlockNumber) {
            d->suggestedVisibleCollapsedBlockNumber = blockNumber;
            d->collapsedBlockTimer.start(40, this);
        }
        viewport()->setCursor(collapsedBlock.isValid() ? Qt::PointingHandCursor : Qt::IBeamCursor);
    } else {
        QPlainTextEdit::mouseMoveEvent(e);
    }
    if (d->m_lastEventWasBlockSelectionEvent && d->m_inBlockSelectionMode) {
        if (textCursor().atBlockEnd()) {
            d->m_blockSelectionExtraX = qMax(0, e->pos().x() - cursorRect().center().x()) / fontMetrics().width(QLatin1Char('x'));
        } else {
            d->m_blockSelectionExtraX = 0;
        }
    }
}

void BaseTextEditor::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        d->clearBlockSelection(); // just in case, otherwise we might get strange drag and drop

        QTextBlock collapsedBlock = collapsedBlockAt(e->pos());
        if (collapsedBlock.isValid()) {
            toggleBlockVisible(collapsedBlock);
            viewport()->setCursor(Qt::IBeamCursor);
        }
    }
    QPlainTextEdit::mousePressEvent(e);
}

void BaseTextEditor::extraAreaLeaveEvent(QEvent *)
{
    if (d->extraAreaHighlightCollapseBlockNumber >= 0) {
        d->extraAreaHighlightFadingBlockNumber = d->extraAreaHighlightCollapseBlockNumber;
        d->extraAreaHighlightCollapseBlockNumber = -1; // missing mouse move event from Qt
        d->extraAreaTimeLine->setDirection(QTimeLine::Backward);
        if (d->extraAreaTimeLine->state() != QTimeLine::Running)
            d->extraAreaTimeLine->start();
    }
}

void BaseTextEditor::extraAreaMouseEvent(QMouseEvent *e)
{
    QTextCursor cursor = cursorForPosition(QPoint(0, e->pos().y()));
    cursor.setPosition(cursor.block().position());

    int markWidth;
    extraAreaWidth(&markWidth);

    if (e->type() == QEvent::MouseMove && e->buttons() == 0) { // mouse tracking
        // Update which folder marker is highlighted
        const int highlightBlockNumber = d->extraAreaHighlightCollapseBlockNumber;
        d->extraAreaHighlightCollapseBlockNumber = -1;

        if (d->m_codeFoldingVisible
            && TextBlockUserData::canCollapse(cursor.block())
            && !TextBlockUserData::hasClosingCollapseInside(cursor.block().next())
            && collapseBox(cursor.block()).contains(e->pos()))
            d->extraAreaHighlightCollapseBlockNumber = cursor.blockNumber();

        // Set whether the mouse cursor is a hand or normal arrow
        bool hand = (e->pos().x() <= markWidth || d->extraAreaHighlightCollapseBlockNumber >= 0);
        if (hand != (d->m_extraArea->cursor().shape() == Qt::PointingHandCursor))
            d->m_extraArea->setCursor(hand ? Qt::PointingHandCursor : Qt::ArrowCursor);

        // Start fading in or out the highlighted folding marker
        if (highlightBlockNumber != d->extraAreaHighlightCollapseBlockNumber) {
            d->extraAreaTimeLine->stop();
            d->extraAreaTimeLine->setDirection(d->extraAreaHighlightCollapseBlockNumber >= 0?
                                               QTimeLine::Forward : QTimeLine::Backward);
            if (d->extraAreaTimeLine->direction() == QTimeLine::Backward)
                d->extraAreaHighlightFadingBlockNumber = highlightBlockNumber;
            else
                d->extraAreaHighlightFadingBlockNumber = -1;
            if (d->extraAreaTimeLine->state() != QTimeLine::Running)
                d->extraAreaTimeLine->start();
        }
    }

    if (e->type() == QEvent::MouseButtonPress || e->type() == QEvent::MouseButtonDblClick) {
        if (e->button() == Qt::LeftButton) {
            if (d->m_codeFoldingVisible && TextBlockUserData::canCollapse(cursor.block())
                && !TextBlockUserData::hasClosingCollapseInside(cursor.block().next())
                && collapseBox(cursor.block()).contains(e->pos())) {
                toggleBlockVisible(cursor.block());
                d->moveCursorVisible(false);
            } else if (d->m_marksVisible && e->pos().x() > markWidth) {
                QTextCursor selection = cursor;
                selection.setVisualNavigation(true);
                d->extraAreaSelectionAnchorBlockNumber = selection.blockNumber();
                selection.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                selection.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
                setTextCursor(selection);
            } else {
                d->extraAreaToggleMarkBlockNumber = cursor.blockNumber();
            }
        } else if (d->m_marksVisible && e->button() == Qt::RightButton) {
            QMenu * contextMenu = new QMenu(this);
            emit d->m_editable->markContextMenuRequested(editableInterface(), cursor.blockNumber() + 1, contextMenu);
            if (!contextMenu->isEmpty())
                contextMenu->exec(e->globalPos());
            delete contextMenu;
        }
    } else if (d->extraAreaSelectionAnchorBlockNumber >= 0) {
        QTextCursor selection = cursor;
        selection.setVisualNavigation(true);
        if (e->type() == QEvent::MouseMove) {
            QTextBlock anchorBlock = document()->findBlockByNumber(d->extraAreaSelectionAnchorBlockNumber);
            selection.setPosition(anchorBlock.position());
            if (cursor.blockNumber() < d->extraAreaSelectionAnchorBlockNumber) {
                selection.movePosition(QTextCursor::EndOfBlock);
                selection.movePosition(QTextCursor::Right);
            }
            selection.setPosition(cursor.block().position(), QTextCursor::KeepAnchor);
            if (cursor.blockNumber() >= d->extraAreaSelectionAnchorBlockNumber) {
                selection.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                selection.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
            }

            if (e->pos().y() >= 0 && e->pos().y() <= d->m_extraArea->height())
                d->autoScrollTimer.stop();
            else if (!d->autoScrollTimer.isActive())
                d->autoScrollTimer.start(100, this);

        } else {
            d->autoScrollTimer.stop();
            d->extraAreaSelectionAnchorBlockNumber = -1;
            return;
        }
        setTextCursor(selection);
    } else if (d->extraAreaToggleMarkBlockNumber >= 0 && d->m_marksVisible && d->m_requestMarkEnabled) {
        if (e->type() == QEvent::MouseButtonRelease && e->button() == Qt::LeftButton) {
            int n = d->extraAreaToggleMarkBlockNumber;
            d->extraAreaToggleMarkBlockNumber = -1;
            if (cursor.blockNumber() == n) {
                int line = n + 1;
                emit d->m_editable->markRequested(editableInterface(), line);
            }
        }
    }
}

void BaseTextEditor::slotCursorPositionChanged()
{
    QList<QTextEdit::ExtraSelection> extraSelections;
    setExtraSelections(ParenthesesMatchingSelection, extraSelections); // clear
    if (d->m_parenthesesMatchingEnabled)
        d->m_parenthesesMatchingTimer->start(50);

    if (d->m_highlightCurrentLine) {
        QTextEdit::ExtraSelection sel;
        sel.format.setBackground(d->m_currentLineFormat.background());
        sel.format.setProperty(QTextFormat::FullWidthSelection, true);
        sel.cursor = textCursor();
        sel.cursor.clearSelection();
        extraSelections.append(sel);
    }

    setExtraSelections(CurrentLineSelection, extraSelections);
}

QTextBlock TextBlockUserData::testCollapse(const QTextBlock& block)
{
    QTextBlock info = block;
    if (block.userData() && static_cast<TextBlockUserData*>(block.userData())->collapseMode() == CollapseAfter)
        ;
    else if (block.next().userData()
             && static_cast<TextBlockUserData*>(block.next().userData())->collapseMode()
             == TextBlockUserData::CollapseThis)
        info = block.next();
    else
        return QTextBlock();
    int pos = static_cast<TextBlockUserData*>(info.userData())->collapseAtPos();
    if (pos < 0)
        return QTextBlock();
    QTextCursor cursor(info);
    cursor.setPosition(cursor.position() + pos);
    matchCursorForward(&cursor);
    return cursor.block();
}

void TextBlockUserData::doCollapse(const QTextBlock& block, bool visible)
{
    QTextBlock info = block;
    if (block.userData() && static_cast<TextBlockUserData*>(block.userData())->collapseMode() == CollapseAfter)
        ;
    else if (block.next().userData()
             && static_cast<TextBlockUserData*>(block.next().userData())->collapseMode()
             == TextBlockUserData::CollapseThis)
        info = block.next();
    else {
        if (visible && !block.next().isVisible()) {
            // no match, at least unfold!
            QTextBlock b = block.next();
            while (b.isValid() && !b.isVisible()) {
                b.setVisible(true);
                b.setLineCount(visible ? qMax(1, b.layout()->lineCount()) : 0);
                b = b.next();
            }
        }
        return;
    }
    int pos = static_cast<TextBlockUserData*>(info.userData())->collapseAtPos();
    if (pos < 0)
        return;
    QTextCursor cursor(info);
    cursor.setPosition(cursor.position() + pos);
    if (matchCursorForward(&cursor) != Match) {
        if (visible) {
            // no match, at least unfold!
            QTextBlock b = block.next();
            while (b.isValid() && !b.isVisible()) {
                b.setVisible(true);
                b.setLineCount(visible ? qMax(1, b.layout()->lineCount()) : 0);
                b = b.next();
            }
        }
        return;
    }

    QTextBlock b = block.next();
    while (b < cursor.block()) {
        b.setVisible(visible);
        b.setLineCount(visible ? qMax(1, b.layout()->lineCount()) : 0);
        if (visible) {
            TextBlockUserData *data = canCollapse(b);
            if (data && data->collapsed()) {
                QTextBlock end =  testCollapse(b);
                if (data->collapseIncludesClosure())
                    end = end.next();
                if (end.isValid()) {
                    b = end;
                    continue;
                }
            }
        }
        b = b.next();
    }

    bool collapseIncludesClosure = hasClosingCollapseAtEnd(b);
    if (collapseIncludesClosure) {
        b.setVisible(visible);
        b.setLineCount(visible ? qMax(1, b.layout()->lineCount()) : 0);
    }
    static_cast<TextBlockUserData*>(info.userData())->setCollapseIncludesClosure(collapseIncludesClosure);
    static_cast<TextBlockUserData*>(info.userData())->setCollapsed(!block.next().isVisible());

}


void BaseTextEditor::ensureCursorVisible()
{
    QTextBlock block = textCursor().block();
    if (!block.isVisible()) {
        while (!block.isVisible() && block.previous().isValid())
            block = block.previous();
        toggleBlockVisible(block);
    }
    QPlainTextEdit::ensureCursorVisible();
}

void BaseTextEditor::toggleBlockVisible(const QTextBlock &block)
{
    TextEditDocumentLayout *documentLayout = qobject_cast<TextEditDocumentLayout*>(document()->documentLayout());
    QTC_ASSERT(documentLayout, return);

    bool visible = block.next().isVisible();
    TextBlockUserData::doCollapse(block, !visible);
    documentLayout->requestUpdate();
    documentLayout->emitDocumentSizeChanged();
}


const TabSettings &BaseTextEditor::tabSettings() const
{
    return d->m_document->tabSettings();
}

const DisplaySettings &BaseTextEditor::displaySettings() const
{
    return d->m_displaySettings;
}

const InteractionSettings &BaseTextEditor::interactionSettings() const
{
    return d->m_interactionSettings;
}


void BaseTextEditor::indentOrUnindent(bool doIndent)
{
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();

    int pos = cursor.position();
    const TextEditor::TabSettings &tabSettings = d->m_document->tabSettings();


    QTextDocument *doc = document();
    if (!cursor.hasSelection()
        || (doc->findBlock(cursor.selectionStart()) == doc->findBlock(cursor.selectionEnd()) )) {
        cursor.removeSelectedText();
        QTextBlock block = cursor.block();
        QString text = block.text();
        int indentPosition = (cursor.position() - block.position());;
        int spaces = tabSettings.spacesLeftFromPosition(text, indentPosition);
        int startColumn = tabSettings.columnAt(text, indentPosition - spaces);
        int targetColumn = tabSettings.indentedColumn(tabSettings.columnAt(text, indentPosition), doIndent);

        cursor.setPosition(block.position() + indentPosition);
        cursor.setPosition(block.position() + indentPosition - spaces, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
        cursor.insertText(tabSettings.indentationString(startColumn, targetColumn));
    } else {
        int anchor = cursor.anchor();
        int start = qMin(anchor, pos);
        int end = qMax(anchor, pos);

        QTextBlock startBlock = doc->findBlock(start);
        QTextBlock endBlock = doc->findBlock(end-1).next();

        for (QTextBlock block = startBlock; block != endBlock; block = block.next()) {
            QString text = block.text();
            int indentPosition = tabSettings.lineIndentPosition(text);
            if (!doIndent && !indentPosition)
                indentPosition = tabSettings.firstNonSpace(text);
            int targetColumn = tabSettings.indentedColumn(tabSettings.columnAt(text, indentPosition), doIndent);
            cursor.setPosition(block.position() + indentPosition);
            cursor.insertText(tabSettings.indentationString(0, targetColumn));
            cursor.setPosition(block.position());
            cursor.setPosition(block.position() + indentPosition, QTextCursor::KeepAnchor);
            cursor.removeSelectedText();
        }
    }

    cursor.endEditBlock();
}

void BaseTextEditor::handleHomeKey(bool anchor)
{
    QTextCursor cursor = textCursor();
    QTextCursor::MoveMode mode = QTextCursor::MoveAnchor;

    if (anchor)
        mode = QTextCursor::KeepAnchor;

    const int initpos = cursor.position();
    int pos = cursor.block().position();
    QChar character = characterAt(pos);
    const QLatin1Char tab = QLatin1Char('\t');

    while (character == tab || character.category() == QChar::Separator_Space) {
        ++pos;
        if (pos == initpos)
            break;
        character = characterAt(pos);
    }

    // Go to the start of the block when we're already at the start of the text
    if (pos == initpos)
        pos = cursor.block().position();

    cursor.setPosition(pos, mode);
    setTextCursor(cursor);
}

void BaseTextEditor::handleBackspaceKey()
{
    QTextCursor cursor = textCursor();
    QTC_ASSERT(!cursor.hasSelection(), return);

    const TextEditor::TabSettings &tabSettings = d->m_document->tabSettings();
    QTextBlock currentBlock = cursor.block();
    int positionInBlock = cursor.position() - currentBlock.position();
    const QString blockText = currentBlock.text();
    if (cursor.atBlockStart() || tabSettings.firstNonSpace(blockText) < positionInBlock) {
        cursor.deletePreviousChar();
        return;
    }

    int previousIndent = 0;
    const int indent = tabSettings.columnAt(blockText, positionInBlock);

    for (QTextBlock previousNonEmptyBlock = currentBlock.previous();
         previousNonEmptyBlock.isValid();
         previousNonEmptyBlock = previousNonEmptyBlock.previous()) {
        QString previousNonEmptyBlockText = previousNonEmptyBlock.text();
        if (previousNonEmptyBlockText.trimmed().isEmpty())
            continue;
        previousIndent = tabSettings.columnAt(previousNonEmptyBlockText,
                                              tabSettings.firstNonSpace(previousNonEmptyBlockText));
        if (previousIndent < indent)
            break;
    }

    if (previousIndent >= indent)
        previousIndent = 0;

    cursor.beginEditBlock();
    cursor.setPosition(currentBlock.position(), QTextCursor::KeepAnchor);
    cursor.insertText(tabSettings.indentationString(0, previousIndent));
    cursor.endEditBlock();
}


void BaseTextEditor::format()
{
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();
    indent(document(), cursor, QChar::Null);
    cursor.endEditBlock();
}

void BaseTextEditor::unCommentSelection()
{
}

void BaseTextEditor::setTabSettings(const TabSettings &ts)
{
    d->m_document->setTabSettings(ts);
    int charWidth = QFontMetrics(font()).width(QChar(' '));
    setTabStopWidth(charWidth * ts.m_tabSize);
}

void BaseTextEditor::setDisplaySettings(const DisplaySettings &ds)
{
    setLineWrapMode(ds.m_textWrapping ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
    setLineNumbersVisible(ds.m_displayLineNumbers);
    setVisibleWrapColumn(ds.m_showWrapColumn ? ds.m_wrapColumn : 0);
    setCodeFoldingVisible(ds.m_displayFoldingMarkers);
    setHighlightCurrentLine(ds.m_highlightCurrentLine);

    if (d->m_displaySettings.m_visualizeWhitespace != ds.m_visualizeWhitespace) {
        if (QSyntaxHighlighter *highlighter = baseTextDocument()->syntaxHighlighter())
            highlighter->rehighlight();
        QTextOption option =  document()->defaultTextOption();
        if (ds.m_visualizeWhitespace)
            option.setFlags(option.flags() | QTextOption::ShowTabsAndSpaces);
        else
            option.setFlags(option.flags() & ~QTextOption::ShowTabsAndSpaces);
        option.setFlags(option.flags() | QTextOption::AddSpaceForLineAndParagraphSeparators);
        document()->setDefaultTextOption(option);
    }

    d->m_displaySettings = ds;
}


void BaseTextEditor::wheelEvent(QWheelEvent *e)
{
  d->clearVisibleCollapsedBlock();
    if (e->modifiers() & Qt::ControlModifier) {
        const int delta = e->delta();
        if (delta < 0)
            zoomOut();
        else if (delta > 0)
            zoomIn();
        return;
    }
    QPlainTextEdit::wheelEvent(e);
}

void BaseTextEditor::zoomIn(int range)
{
  d->clearVisibleCollapsedBlock();
    QFont f = font();
    const int newSize = f.pointSize() + range;
    if (newSize <= 0)
        return;
    f.setPointSize(newSize);
    setFont(f);
}

void BaseTextEditor::zoomOut(int range)
{
    zoomIn(-range);
}

bool BaseTextEditor::isElectricCharacter(const QChar &) const
{
    return false;
}

void BaseTextEditor::indentBlock(QTextDocument *, QTextBlock, QChar)
{
}

void BaseTextEditor::indent(QTextDocument *doc, const QTextCursor &cursor, QChar typedChar)
{
    if (cursor.hasSelection()) {
        QTextBlock block = doc->findBlock(qMin(cursor.selectionStart(), cursor.selectionEnd()));
        const QTextBlock end = doc->findBlock(qMax(cursor.selectionStart(), cursor.selectionEnd())).next();
        do {
            indentBlock(doc, block, typedChar);
            block = block.next();
        } while (block.isValid() && block != end);
    } else {
        indentBlock(doc, cursor.block(), typedChar);
    }
}

void BaseTextEditorPrivate::updateMarksBlock(const QTextBlock &block)
{
    if (const TextBlockUserData *userData = TextEditDocumentLayout::testUserData(block))
       foreach (ITextMark *mrk, userData->marks()) {
            mrk->updateBlock(block);
        }
}

void BaseTextEditorPrivate::updateMarksLineNumber()
{
    QTextDocument *doc = q->document();
    QTextBlock block = doc->begin();
    int blockNumber = 0;
    while (block.isValid()) {
        if (const TextBlockUserData *userData = TextEditDocumentLayout::testUserData(block))
            foreach (ITextMark *mrk, userData->marks()) {
                mrk->updateLineNumber(blockNumber + 1);
            }
        block = block.next();
        ++blockNumber;
    }
}

void BaseTextEditor::markBlocksAsChanged(QList<int> blockNumbers) {
    QTextBlock block = document()->begin();
    while (block.isValid()) {
        if (block.revision() < 0)
            block.setRevision(-block.revision() - 1);
        block = block.next();
    }
    foreach (const int blockNumber, blockNumbers) {
        QTextBlock block = document()->findBlockByNumber(blockNumber);
        if (block.isValid())
            block.setRevision(-block.revision() - 1);
    }
}



TextBlockUserData::MatchType TextBlockUserData::checkOpenParenthesis(QTextCursor *cursor, QChar c)
{
    QTextBlock block = cursor->block();
    if (!TextEditDocumentLayout::hasParentheses(block) || TextEditDocumentLayout::ifdefedOut(block))
        return NoMatch;

    Parentheses parenList = TextEditDocumentLayout::parentheses(block);
    Parenthesis openParen, closedParen;
    QTextBlock closedParenParag = block;

    const int cursorPos = cursor->position() - closedParenParag.position();
    int i = 0;
    int ignore = 0;
    bool foundOpen = false;
    for (;;) {
        if (!foundOpen) {
            if (i >= parenList.count())
                return NoMatch;
            openParen = parenList.at(i);
            if (openParen.pos != cursorPos) {
                ++i;
                continue;
            } else {
                foundOpen = true;
                ++i;
            }
        }

        if (i >= parenList.count()) {
            for (;;) {
                closedParenParag = closedParenParag.next();
                if (!closedParenParag.isValid())
                    return NoMatch;
                if (TextEditDocumentLayout::hasParentheses(closedParenParag)
                    && !TextEditDocumentLayout::ifdefedOut(closedParenParag)) {
                    parenList = TextEditDocumentLayout::parentheses(closedParenParag);
                    break;
                }
            }
            i = 0;
        }

        closedParen = parenList.at(i);
        if (closedParen.type == Parenthesis::Opened) {
            ignore++;
            ++i;
            continue;
        } else {
            if (ignore > 0) {
                ignore--;
                ++i;
                continue;
            }

            cursor->clearSelection();
            cursor->setPosition(closedParenParag.position() + closedParen.pos + 1, QTextCursor::KeepAnchor);

            if ((c == QLatin1Char('{') && closedParen.chr != QLatin1Char('}'))
                || (c == QLatin1Char('(') && closedParen.chr != QLatin1Char(')'))
                || (c == QLatin1Char('[') && closedParen.chr != QLatin1Char(']'))
                || (c == QLatin1Char('+') && closedParen.chr != QLatin1Char('-'))
               )
                return Mismatch;

            return Match;
        }
    }
}

TextBlockUserData::MatchType TextBlockUserData::checkClosedParenthesis(QTextCursor *cursor, QChar c)
{
    QTextBlock block = cursor->block();
    if (!TextEditDocumentLayout::hasParentheses(block) || TextEditDocumentLayout::ifdefedOut(block))
        return NoMatch;

    Parentheses parenList = TextEditDocumentLayout::parentheses(block);
    Parenthesis openParen, closedParen;
    QTextBlock openParenParag = block;

    const int cursorPos = cursor->position() - openParenParag.position();
    int i = parenList.count() - 1;
    int ignore = 0;
    bool foundClosed = false;
    for (;;) {
        if (!foundClosed) {
            if (i < 0)
                return NoMatch;
            closedParen = parenList.at(i);
            if (closedParen.pos != cursorPos - 1) {
                --i;
                continue;
            } else {
                foundClosed = true;
                --i;
            }
        }

        if (i < 0) {
            for (;;) {
                openParenParag = openParenParag.previous();
                if (!openParenParag.isValid())
                    return NoMatch;

                if (TextEditDocumentLayout::hasParentheses(openParenParag)
                    && !TextEditDocumentLayout::ifdefedOut(openParenParag)) {
                    parenList = TextEditDocumentLayout::parentheses(openParenParag);
                    break;
                }
            }
            i = parenList.count() - 1;
        }

        openParen = parenList.at(i);
        if (openParen.type == Parenthesis::Closed) {
            ignore++;
            --i;
            continue;
        } else {
            if (ignore > 0) {
                ignore--;
                --i;
                continue;
            }

            cursor->clearSelection();
            cursor->setPosition(openParenParag.position() + openParen.pos, QTextCursor::KeepAnchor);

            if ((c == '}' && openParen.chr != '{')    ||
                 (c == ')' && openParen.chr != '(')   ||
                 (c == ']' && openParen.chr != '[')   ||
                 (c == '-' && openParen.chr != '+'))
                return Mismatch;

            return Match;
        }
    }
}


bool TextBlockUserData::findPreviousOpenParenthesis(QTextCursor *cursor, bool select)
{
    QTextBlock block = cursor->block();
    int position = cursor->position();
    int ignore = 0;
    while (block.isValid()) {
        Parentheses parenList = TextEditDocumentLayout::parentheses(block);
        if (!parenList.isEmpty() && !TextEditDocumentLayout::ifdefedOut(block)) {
            for (int i = parenList.count()-1; i >= 0; --i) {
                Parenthesis paren = parenList.at(i);
                if (block == cursor->block() &&
                    (position - block.position() <= paren.pos + (paren.type == Parenthesis::Closed ? 1 : 0)))
                        continue;
                if (paren.type == Parenthesis::Closed) {
                    ++ignore;
                } else if (ignore > 0) {
                    --ignore;
                } else {
                    cursor->setPosition(block.position() + paren.pos, select ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor);
                    return true;
                }
            }
        }
        block = block.previous();
    }
    return false;
}

bool TextBlockUserData::findNextClosingParenthesis(QTextCursor *cursor, bool select)
{
    QTextBlock block = cursor->block();
    int position = cursor->position();
    int ignore = 0;
    while (block.isValid()) {
        Parentheses parenList = TextEditDocumentLayout::parentheses(block);
        if (!parenList.isEmpty() && !TextEditDocumentLayout::ifdefedOut(block)) {
            for (int i = 0; i < parenList.count(); ++i) {
                Parenthesis paren = parenList.at(i);
                if (block == cursor->block() && position - block.position() >= paren.pos)
                    continue;
                if (paren.type == Parenthesis::Opened) {
                    ++ignore;
                } else if (ignore > 0) {
                    --ignore;
                } else {
                    cursor->setPosition(block.position() + paren.pos+1, select ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor);
                    return true;
                }
            }
        }
        block = block.next();
    }
    return false;
}

TextBlockUserData::MatchType TextBlockUserData::matchCursorBackward(QTextCursor *cursor)
{
    cursor->clearSelection();
    const QTextBlock block = cursor->block();

    if (!TextEditDocumentLayout::hasParentheses(block) || TextEditDocumentLayout::ifdefedOut(block))
        return NoMatch;

    const int relPos = cursor->position() - block.position();

    Parentheses parentheses = TextEditDocumentLayout::parentheses(block);
    const Parentheses::const_iterator cend = parentheses.constEnd();
    for (Parentheses::const_iterator it = parentheses.constBegin();it != cend; ++it) {
        const Parenthesis &paren = *it;
        if (paren.pos == relPos - 1
            && paren.type == Parenthesis::Closed) {
            return checkClosedParenthesis(cursor, paren.chr);
        }
    }
    return NoMatch;
}

TextBlockUserData::MatchType TextBlockUserData::matchCursorForward(QTextCursor *cursor)
{
    cursor->clearSelection();
    const QTextBlock block = cursor->block();

    if (!TextEditDocumentLayout::hasParentheses(block) || TextEditDocumentLayout::ifdefedOut(block))
        return NoMatch;

    const int relPos = cursor->position() - block.position();

    Parentheses parentheses = TextEditDocumentLayout::parentheses(block);
    const Parentheses::const_iterator cend = parentheses.constEnd();
    for (Parentheses::const_iterator it = parentheses.constBegin();it != cend; ++it) {
        const Parenthesis &paren = *it;
        if (paren.pos == relPos
            && paren.type == Parenthesis::Opened) {
            return checkOpenParenthesis(cursor, paren.chr);
        }
    }
    return NoMatch;
}


void BaseTextEditor::highlightSearchResults(const QString &txt, QTextDocument::FindFlags findFlags)
{
    if (d->m_searchExpr.pattern() == txt)
        return;
    d->m_searchExpr.setPattern(txt);
    d->m_searchExpr.setPatternSyntax(QRegExp::FixedString);
    d->m_searchExpr.setCaseSensitivity((findFlags & QTextDocument::FindCaseSensitively) ?
                                       Qt::CaseSensitive : Qt::CaseInsensitive);
    d->m_findFlags = findFlags;
    viewport()->update();
}


void BaseTextEditor::setFindScope(const QTextCursor &scope)
{
    if (scope.isNull() != d->m_findScope.isNull()) {
        d->m_findScope = scope;
        viewport()->update();
    }
}

void BaseTextEditor::_q_matchParentheses()
{
    if (isReadOnly())
        return;

    QTextCursor backwardMatch = textCursor();
    QTextCursor forwardMatch = textCursor();
    const TextBlockUserData::MatchType backwardMatchType = TextBlockUserData::matchCursorBackward(&backwardMatch);
    const TextBlockUserData::MatchType forwardMatchType = TextBlockUserData::matchCursorForward(&forwardMatch);

    if (backwardMatchType == TextBlockUserData::NoMatch && forwardMatchType == TextBlockUserData::NoMatch)
        return;

    QList<QTextEdit::ExtraSelection> extraSelections;

    if (backwardMatch.hasSelection()) {
        QTextEdit::ExtraSelection sel;
        if (backwardMatchType == TextBlockUserData::Mismatch) {
            sel.cursor = backwardMatch;
            sel.format = d->m_mismatchFormat;
        } else {

            if (d->m_formatRange) {
                sel.cursor = backwardMatch;
                sel.format = d->m_rangeFormat;
                extraSelections.append(sel);
            }

            sel.cursor = backwardMatch;
            sel.format = d->m_matchFormat;

            sel.cursor.setPosition(backwardMatch.selectionStart());
            sel.cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
            extraSelections.append(sel);

            sel.cursor.setPosition(backwardMatch.selectionEnd());
            sel.cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
        }
        extraSelections.append(sel);
    }

    if (forwardMatch.hasSelection()) {
        QTextEdit::ExtraSelection sel;
        if (forwardMatchType == TextBlockUserData::Mismatch) {
            sel.cursor = forwardMatch;
            sel.format = d->m_mismatchFormat;
        } else {

            if (d->m_formatRange) {
                sel.cursor = forwardMatch;
                sel.format = d->m_rangeFormat;
                extraSelections.append(sel);
            }

            sel.cursor = forwardMatch;
            sel.format = d->m_matchFormat;

            sel.cursor.setPosition(forwardMatch.selectionStart());
            sel.cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
            extraSelections.append(sel);

            sel.cursor.setPosition(forwardMatch.selectionEnd());
            sel.cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
        }
        extraSelections.append(sel);
    }
    setExtraSelections(ParenthesesMatchingSelection, extraSelections);
}

void BaseTextEditor::setActionHack(QObject *hack)
{
    d->m_actionHack = hack;
}

QObject *BaseTextEditor::actionHack() const
{
    return d->m_actionHack;
}

void BaseTextEditor::changeEvent(QEvent *e)
{
    QPlainTextEdit::changeEvent(e);
    if (e->type() == QEvent::ApplicationFontChange
        || e->type() == QEvent::FontChange) {
        if (d->m_extraArea) {
            QFont f = d->m_extraArea->font();
            f.setPointSize(font().pointSize());
            d->m_extraArea->setFont(f);
            slotUpdateExtraAreaWidth();
            d->m_extraArea->update();
        }
    }
}

// shift+del
void BaseTextEditor::deleteLine()
{
    QTextCursor cursor = textCursor();
    if (!cursor.hasSelection()) {
        const QTextBlock &block = cursor.block();
        if (block.next().isValid()) {
            cursor.setPosition(block.position());
            cursor.setPosition(block.next().position(), QTextCursor::KeepAnchor);
        } else {
            cursor.movePosition(QTextCursor::EndOfBlock);
            cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
            cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
        }
        setTextCursor(cursor);
    }
    cut();
}

void BaseTextEditor::setExtraSelections(ExtraSelectionKind kind, const QList<QTextEdit::ExtraSelection> &selections)
{
    if (selections.isEmpty() && d->m_extraSelections[kind].isEmpty())
        return;
    d->m_extraSelections[kind] = selections;

    QList<QTextEdit::ExtraSelection> all;
    for (int i = 0; i < NExtraSelectionKinds; ++i)
        all += d->m_extraSelections[i];
    QPlainTextEdit::setExtraSelections(all);
}

QList<QTextEdit::ExtraSelection> BaseTextEditor::extraSelections(ExtraSelectionKind kind) const
{
    return d->m_extraSelections[kind];
}


// the blocks list must be sorted
void BaseTextEditor::setIfdefedOutBlocks(const QList<BaseTextEditor::BlockRange> &blocks)
{
    QTextDocument *doc = document();
    TextEditDocumentLayout *documentLayout = qobject_cast<TextEditDocumentLayout*>(doc->documentLayout());
    QTC_ASSERT(documentLayout, return);

    bool needUpdate = false;

    QTextBlock block = doc->firstBlock();

    int rangeNumber = 0;
    while (block.isValid()) {
        if (rangeNumber < blocks.size()) {
            const BlockRange &range = blocks.at(rangeNumber);

            if (block.position() >= range.first && (block.position() <= range.last || !range.last)) {
                needUpdate |= TextEditDocumentLayout::setIfdefedOut(block);
            } else {
                needUpdate |= TextEditDocumentLayout::clearIfdefedOut(block);
            }
            if (block.contains(range.last))
                ++rangeNumber;
        } else {
            needUpdate |= TextEditDocumentLayout::clearIfdefedOut(block);
        }

        block = block.next();
    }

    if (needUpdate)
        documentLayout->requestUpdate();
}


void BaseTextEditorPrivate::moveCursorVisible(bool ensureVisible)
{
    QTextCursor cursor = q->textCursor();
    if (!cursor.block().isVisible()) {
        cursor.setVisualNavigation(true);
        cursor.movePosition(QTextCursor::Up);
        q->setTextCursor(cursor);
    }
    if (ensureVisible)
        q->ensureCursorVisible();
}

void BaseTextEditor::collapse()
{
    QTextDocument *doc = document();
    TextEditDocumentLayout *documentLayout = qobject_cast<TextEditDocumentLayout*>(doc->documentLayout());
    QTC_ASSERT(documentLayout, return);
    QTextBlock block = textCursor().block();
    QTextBlock curBlock = block;
    while (block.isValid()) {
        if (TextBlockUserData::canCollapse(block) && block.next().isVisible()) {
            if (block == curBlock || block.next() == curBlock)
                break;
            if ((block.next().userState()) >> 8 <= (curBlock.previous().userState() >> 8))
                break;
        }
        block = block.previous();
    }
    if (block.isValid()) {
        TextBlockUserData::doCollapse(block, false);
        d->moveCursorVisible();
        documentLayout->requestUpdate();
        documentLayout->emitDocumentSizeChanged();
    }
}

void BaseTextEditor::expand()
{
    QTextDocument *doc = document();
    TextEditDocumentLayout *documentLayout = qobject_cast<TextEditDocumentLayout*>(doc->documentLayout());
    QTC_ASSERT(documentLayout, return);
    QTextBlock block = textCursor().block();
    while (block.isValid() && !block.isVisible())
        block = block.previous();
    TextBlockUserData::doCollapse(block, true);
    d->moveCursorVisible();
    documentLayout->requestUpdate();
    documentLayout->emitDocumentSizeChanged();
}

void BaseTextEditor::unCollapseAll()
{
    QTextDocument *doc = document();
    TextEditDocumentLayout *documentLayout = qobject_cast<TextEditDocumentLayout*>(doc->documentLayout());
    QTC_ASSERT(documentLayout, return);

    QTextBlock block = doc->firstBlock();
    bool makeVisible = true;
    while (block.isValid()) {
        if (block.isVisible() && TextBlockUserData::canCollapse(block) && block.next().isVisible()) {
            makeVisible = false;
            break;
        }
        block = block.next();
    }

    block = doc->firstBlock();

    while (block.isValid()) {
        if (TextBlockUserData::canCollapse(block))
            TextBlockUserData::doCollapse(block, makeVisible);
        block = block.next();
    }

    d->moveCursorVisible();
    documentLayout->requestUpdate();
    documentLayout->emitDocumentSizeChanged();
    centerCursor();
}

void BaseTextEditor::setTextCodec(QTextCodec *codec)
{
    baseTextDocument()->setCodec(codec);
}

QTextCodec *BaseTextEditor::textCodec() const
{
    return baseTextDocument()->codec();
}

void BaseTextEditor::setReadOnly(bool b)
{
    QPlainTextEdit::setReadOnly(b);
    if (b)
        setTextInteractionFlags(textInteractionFlags() | Qt::TextSelectableByKeyboard);
}

void BaseTextEditor::cut()
{
    if (d->m_inBlockSelectionMode) {
        copy();
        d->removeBlockSelection();
        return;
    }
    QPlainTextEdit::cut();
}

void BaseTextEditor::paste()
{
    if (d->m_inBlockSelectionMode) {
        d->removeBlockSelection();
    }
    QPlainTextEdit::paste();
}

QMimeData *BaseTextEditor::createMimeDataFromSelection() const
{
    if (d->m_inBlockSelectionMode) {
        QMimeData *mimeData = new QMimeData;
        QString text = d->copyBlockSelection();
        mimeData->setData(QLatin1String("application/vnd.nokia.qtcreator.blocktext"), text.toUtf8());
        mimeData->setText(text); // for exchangeability
        return mimeData;
    }
    return QPlainTextEdit::createMimeDataFromSelection();
}

bool BaseTextEditor::canInsertFromMimeData(const QMimeData *source) const
{
    return QPlainTextEdit::canInsertFromMimeData(source);
}

void BaseTextEditor::insertFromMimeData(const QMimeData *source)
{
    if (!isReadOnly() && source->hasFormat(QLatin1String("application/vnd.nokia.qtcreator.blocktext"))) {
        QString text = QString::fromUtf8(source->data(QLatin1String("application/vnd.nokia.qtcreator.blocktext")));
        if (text.isEmpty())
            return;
        QStringList lines = text.split(QLatin1Char('\n'));
        QTextCursor cursor = textCursor();
        cursor.beginEditBlock();
        int initialCursorPosition = cursor.position();
        int column = cursor.position() - cursor.block().position();
        cursor.insertText(lines.first());
        for (int i = 1; i < lines.count(); ++i) {
            QTextBlock next = cursor.block().next();
            if (next.isValid()) {
                cursor.setPosition(next.position() + qMin(column, next.length()-1));
            } else {
                cursor.movePosition(QTextCursor::EndOfBlock);
                cursor.insertBlock();
            }

            int actualColumn = cursor.position() - cursor.block().position();
            if (actualColumn < column)
                cursor.insertText(QString(column - actualColumn, QLatin1Char(' ')));
            cursor.insertText(lines.at(i));
        }
        cursor.setPosition(initialCursorPosition);
        cursor.endEditBlock();
        setTextCursor(cursor);
        ensureCursorVisible();
        return;
    }
    QPlainTextEdit::insertFromMimeData(source);
}

BaseTextEditorEditable::BaseTextEditorEditable(BaseTextEditor *editor)
  : e(editor)
{
#ifndef TEXTEDITOR_STANDALONE
    using namespace Find;
    Aggregation::Aggregate *aggregate = new Aggregation::Aggregate;
    BaseTextFind *baseTextFind = new BaseTextFind(editor);
    connect(baseTextFind, SIGNAL(highlightAll(QString, QTextDocument::FindFlags)),
            editor, SLOT(highlightSearchResults(QString, QTextDocument::FindFlags)));
    connect(baseTextFind, SIGNAL(findScopeChanged(QTextCursor)), editor, SLOT(setFindScope(QTextCursor)));
    aggregate->add(baseTextFind);
    aggregate->add(editor);
#endif

    m_cursorPositionLabel = new Core::Utils::LineColumnLabel;

    QHBoxLayout *l = new QHBoxLayout;
    QWidget *w = new QWidget;
    l->setMargin(0);
    l->setContentsMargins(0, 0, 5, 0);
    l->addStretch(1);
    l->addWidget(m_cursorPositionLabel);
    w->setLayout(l);

    m_toolBar = new QToolBar;
    m_toolBar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    m_toolBar->addWidget(w);

    connect(editor, SIGNAL(cursorPositionChanged()), this, SLOT(updateCursorPosition()));
}

BaseTextEditorEditable::~BaseTextEditorEditable()
{
    delete m_toolBar;
    delete e;
}

QToolBar *BaseTextEditorEditable::toolBar()
{
    return m_toolBar;
}

int BaseTextEditorEditable::find(const QString &) const
{
    return 0;
}

int BaseTextEditorEditable::currentLine() const
{
    return e->textCursor().blockNumber() + 1;
}

int BaseTextEditorEditable::currentColumn() const
{
    QTextCursor cursor = e->textCursor();
    return cursor.position() - cursor.block().position() + 1;
}

QRect BaseTextEditorEditable::cursorRect(int pos) const
{
    QTextCursor tc = e->textCursor();
    if (pos >= 0)
        tc.setPosition(pos);
    QRect result = e->cursorRect(tc);
    result.moveTo(e->viewport()->mapToGlobal(result.topLeft()));
    return result;
}

QString BaseTextEditorEditable::contents() const
{
    return e->toPlainText();
}

QString BaseTextEditorEditable::selectedText() const
{
    if (e->textCursor().hasSelection())
        return e->textCursor().selectedText();
    return QString();
}

QString BaseTextEditorEditable::textAt(int pos, int length) const
{
    QTextCursor c = e->textCursor();

    if (pos < 0)
        pos = 0;
    c.movePosition(QTextCursor::End);
    if (pos + length > c.position())
        length = c.position() - pos;

    c.setPosition(pos);
    c.setPosition(pos + length, QTextCursor::KeepAnchor);

    return c.selectedText();
}

void BaseTextEditorEditable::remove(int length)
{
    QTextCursor tc = e->textCursor();
    tc.setPosition(tc.position() + length, QTextCursor::KeepAnchor);
    tc.removeSelectedText();
}

void BaseTextEditorEditable::insert(const QString &string)
{
    QTextCursor tc = e->textCursor();
    tc.insertText(string);
}

void BaseTextEditorEditable::replace(int length, const QString &string)
{
    QTextCursor tc = e->textCursor();
    tc.setPosition(tc.position() + length, QTextCursor::KeepAnchor);
    tc.insertText(string);
}

void BaseTextEditorEditable::setCurPos(int pos)
{
    QTextCursor tc = e->textCursor();
    tc.setPosition(pos);
    e->setTextCursor(tc);
}

void BaseTextEditorEditable::select(int toPos)
{
    QTextCursor tc = e->textCursor();
    tc.setPosition(toPos, QTextCursor::KeepAnchor);
    e->setTextCursor(tc);
}

void BaseTextEditorEditable::updateCursorPosition()
{
    const QTextCursor cursor = e->textCursor();
    const QTextBlock block = cursor.block();
    const int line = block.blockNumber() + 1;
    const int column = cursor.position() - block.position() + 1;
    m_cursorPositionLabel->setText(QString("Line: %1, Col: %2").arg(line).arg(column),
                                   QString("Line: %1, Col: 999").arg(e->blockCount()));
    m_contextHelpId.clear();

    if (!block.isVisible())
        e->ensureCursorVisible();

}

QString BaseTextEditorEditable::contextHelpId() const
{
    if (m_contextHelpId.isEmpty())
        emit const_cast<BaseTextEditorEditable*>(this)->contextHelpIdRequested(e->editableInterface(),
                                                                               e->textCursor().position());
    return m_contextHelpId;
}


TextBlockUserData::~TextBlockUserData()
{
    TextMarks marks = m_marks;
    m_marks.clear();
    foreach (ITextMark *mrk, marks) {
        mrk->removedFromEditor();
    }
}

