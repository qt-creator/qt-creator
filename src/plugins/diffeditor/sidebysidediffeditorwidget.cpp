/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "sidebysidediffeditorwidget.h"
#include "selectabletexteditorwidget.h"
#include "diffeditorconstants.h"
#include "diffeditordocument.h"
#include "diffutils.h"

#include <QMenu>
#include <QPainter>
#include <QScrollBar>
#include <QTextBlock>
#include <QVBoxLayout>

#include <texteditor/textdocument.h>
#include <texteditor/textdocumentlayout.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/displaysettings.h>

#include <coreplugin/icore.h>
#include <coreplugin/find/highlightscrollbarcontroller.h>
#include <coreplugin/minisplitter.h>

#include <utils/tooltip/tooltip.h>

using namespace Core;
using namespace TextEditor;
using namespace Utils;

namespace DiffEditor {
namespace Internal {

class SideDiffEditorWidget : public SelectableTextEditorWidget
{
    Q_OBJECT
public:
    SideDiffEditorWidget(QWidget *parent = nullptr);

    // block number, file info
    QMap<int, DiffFileInfo> fileInfo() const { return m_fileInfo; }

    void setLineNumber(int blockNumber, int lineNumber);
    void setFileInfo(int blockNumber, const DiffFileInfo &fileInfo);
    void setSkippedLines(int blockNumber, int skippedLines, const QString &contextInfo = QString()) {
        m_skippedLines[blockNumber] = qMakePair(skippedLines, contextInfo);
        setSeparator(blockNumber, true);
    }
    void setChunkIndex(int startBlockNumber, int blockCount, int chunkIndex);
    void setSeparator(int blockNumber, bool separator) {
        m_separators[blockNumber] = separator;
    }
    bool isFileLine(int blockNumber) const {
        return m_fileInfo.contains(blockNumber);
    }
    int blockNumberForFileIndex(int fileIndex) const;
    int fileIndexForBlockNumber(int blockNumber) const;
    int chunkIndexForBlockNumber(int blockNumber) const;
    bool isChunkLine(int blockNumber) const {
        return m_skippedLines.contains(blockNumber);
    }
    void clearAll(const QString &message);
    void clearAllData();
    void saveState();
    void restoreState();

    void setFolded(int blockNumber, bool folded);

    void setDisplaySettings(const DisplaySettings &ds) override;

signals:
    void jumpToOriginalFileRequested(int diffFileIndex,
                                     int lineNumber,
                                     int columnNumber);
    void contextMenuRequested(QMenu *menu,
                              int diffFileIndex,
                              int chunkIndex);
    void foldChanged(int blockNumber, bool folded);
    void gotDisplaySettings();
    void gotFocus();

protected:
    int extraAreaWidth(int *markWidthPtr = nullptr) const override {
        return SelectableTextEditorWidget::extraAreaWidth(markWidthPtr);
    }
    void applyFontSettings() override;

    QString lineNumber(int blockNumber) const override;
    int lineNumberDigits() const override;
    bool selectionVisible(int blockNumber) const override;
    bool replacementVisible(int blockNumber) const override;
    QColor replacementPenColor(int blockNumber) const override;
    QString plainTextFromSelection(const QTextCursor &cursor) const override;
    void mouseDoubleClickEvent(QMouseEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
    void contextMenuEvent(QContextMenuEvent *e) override;
    void paintEvent(QPaintEvent *e) override;
    void scrollContentsBy(int dx, int dy) override;
    void customDrawCollapsedBlockPopup(QPainter &painter,
                                       const QTextBlock &block,
                                       QPointF offset,
                                       const QRect &clip);
    void drawCollapsedBlockPopup(QPainter &painter,
                                 const QTextBlock &block,
                                 QPointF offset,
                                 const QRect &clip) override;
    void focusInEvent(QFocusEvent *e) override;

private:
    void paintSeparator(QPainter &painter, QColor &color, const QString &text,
                        const QTextBlock &block, int top);
    void jumpToOriginalFile(const QTextCursor &cursor);

    // block number, visual line number.
    QMap<int, int> m_lineNumbers;
    // block number, fileInfo. Set for file lines only.
    QMap<int, DiffFileInfo> m_fileInfo;
    // block number, skipped lines and context info. Set for chunk lines only.
    QMap<int, QPair<int, QString> > m_skippedLines;
    // start block number, block count of a chunk, chunk index inside a file.
    QMap<int, QPair<int, int> > m_chunkInfo;
    // block number, separator. Set for file, chunk or span line.
    QMap<int, bool> m_separators;
    QColor m_fileLineForeground;
    QColor m_chunkLineForeground;
    QColor m_textForeground;
    QByteArray m_state;

    QTextBlock m_drawCollapsedBlock;
    QPointF m_drawCollapsedOffset;
    QRect m_drawCollapsedClip;
    int m_lineNumberDigits = 1;
};

SideDiffEditorWidget::SideDiffEditorWidget(QWidget *parent)
    : SelectableTextEditorWidget("DiffEditor.SideDiffEditor", parent)
{
    DisplaySettings settings = displaySettings();
    settings.m_textWrapping = false;
    settings.m_displayLineNumbers = true;
    settings.m_markTextChanges = false;
    settings.m_highlightBlocks = false;
    SelectableTextEditorWidget::setDisplaySettings(settings);

    connect(this, &TextEditorWidget::tooltipRequested, this, [this](const QPoint &point, int position) {
        const int block = document()->findBlock(position).blockNumber();
        const auto it = m_fileInfo.constFind(block);
        if (it != m_fileInfo.constEnd())
            ToolTip::show(point, it.value().fileName, this);
        else
            ToolTip::hide();
    });

    auto documentLayout = qobject_cast<TextDocumentLayout*>(document()->documentLayout());
    if (documentLayout)
        connect(documentLayout, &TextDocumentLayout::foldChanged,
                this, &SideDiffEditorWidget::foldChanged);
    setCodeFoldingSupported(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
}

void SideDiffEditorWidget::saveState()
{
    if (!m_state.isNull())
        return;

    m_state = SelectableTextEditorWidget::saveState();
}

void SideDiffEditorWidget::restoreState()
{
    if (m_state.isNull())
        return;

    SelectableTextEditorWidget::restoreState(m_state);
    m_state.clear();
}

void SideDiffEditorWidget::setFolded(int blockNumber, bool folded)
{
    QTextBlock block = document()->findBlockByNumber(blockNumber);
    if (!block.isValid())
        return;

    if (TextDocumentLayout::isFolded(block) == folded)
        return;

    TextDocumentLayout::doFoldOrUnfold(block, !folded);

    auto documentLayout = qobject_cast<TextDocumentLayout*>(document()->documentLayout());
    documentLayout->requestUpdate();
    documentLayout->emitDocumentSizeChanged();
}

void SideDiffEditorWidget::setDisplaySettings(const DisplaySettings &ds)
{
    DisplaySettings settings = displaySettings();
    settings.m_visualizeWhitespace = ds.m_visualizeWhitespace;
    settings.m_displayFoldingMarkers = ds.m_displayFoldingMarkers;
    settings.m_scrollBarHighlights = ds.m_scrollBarHighlights;
    settings.m_highlightCurrentLine = ds.m_highlightCurrentLine;
    SelectableTextEditorWidget::setDisplaySettings(settings);
    emit gotDisplaySettings();
}

void SideDiffEditorWidget::applyFontSettings()
{
    SelectableTextEditorWidget::applyFontSettings();
    const FontSettings &fs = textDocument()->fontSettings();
    m_fileLineForeground = fs.formatFor(C_DIFF_FILE_LINE).foreground();
    m_chunkLineForeground = fs.formatFor(C_DIFF_CONTEXT_LINE).foreground();
    m_textForeground = fs.toTextCharFormat(C_TEXT).foreground().color();
    update();
}

QString SideDiffEditorWidget::lineNumber(int blockNumber) const
{
    const auto it = m_lineNumbers.constFind(blockNumber);
    if (it != m_lineNumbers.constEnd())
        return QString::number(it.value());
    return QString();
}

int SideDiffEditorWidget::lineNumberDigits() const
{
    return m_lineNumberDigits;
}

bool SideDiffEditorWidget::selectionVisible(int blockNumber) const
{
    return !m_separators.value(blockNumber, false);
}

bool SideDiffEditorWidget::replacementVisible(int blockNumber) const
{
    return isChunkLine(blockNumber) || (isFileLine(blockNumber)
           && TextDocumentLayout::isFolded(document()->findBlockByNumber(blockNumber)));
}

QColor SideDiffEditorWidget::replacementPenColor(int blockNumber) const
{
    Q_UNUSED(blockNumber)
    return m_chunkLineForeground;
}

QString SideDiffEditorWidget::plainTextFromSelection(const QTextCursor &cursor) const
{
    const int startPosition = cursor.selectionStart();
    const int endPosition = cursor.selectionEnd();
    if (startPosition == endPosition)
        return QString(); // no selection

    const QTextBlock startBlock = document()->findBlock(startPosition);
    const QTextBlock endBlock = document()->findBlock(endPosition);
    QTextBlock block = startBlock;
    QString text;
    bool textInserted = false;
    while (block.isValid() && block.blockNumber() <= endBlock.blockNumber()) {
        if (selectionVisible(block.blockNumber())) {
            if (block == startBlock) {
                if (block == endBlock)
                    text = cursor.selectedText(); // just one line text
                else
                    text = block.text().mid(startPosition - block.position());
            } else {
                if (textInserted)
                    text += '\n';
                if (block == endBlock)
                    text += block.text().leftRef(endPosition - block.position());
                else
                    text += block.text();
            }
            textInserted = true;
        }
        block = block.next();
    }

    return convertToPlainText(text);
}

void SideDiffEditorWidget::setLineNumber(int blockNumber, int lineNumber)
{
    const QString lineNumberString = QString::number(lineNumber);
    m_lineNumbers.insert(blockNumber, lineNumber);
    m_lineNumberDigits = qMax(m_lineNumberDigits, lineNumberString.count());
}

void SideDiffEditorWidget::setFileInfo(int blockNumber, const DiffFileInfo &fileInfo)
{
    m_fileInfo[blockNumber] = fileInfo;
    setSeparator(blockNumber, true);
}

void SideDiffEditorWidget::setChunkIndex(int startBlockNumber, int blockCount, int chunkIndex)
{
    m_chunkInfo.insert(startBlockNumber, qMakePair(blockCount, chunkIndex));
}

int SideDiffEditorWidget::blockNumberForFileIndex(int fileIndex) const
{
    if (fileIndex < 0 || fileIndex >= m_fileInfo.count())
        return -1;

    return (m_fileInfo.constBegin() + fileIndex).key();
}

int SideDiffEditorWidget::fileIndexForBlockNumber(int blockNumber) const
{
    int i = -1;
    for (auto it = m_fileInfo.cbegin(), end = m_fileInfo.cend(); it != end; ++it, ++i) {
        if (it.key() > blockNumber)
            break;
    }

    return i;
}

int SideDiffEditorWidget::chunkIndexForBlockNumber(int blockNumber) const
{
    if (m_chunkInfo.isEmpty())
        return -1;

    auto it = m_chunkInfo.upperBound(blockNumber);
    if (it == m_chunkInfo.constBegin())
        return -1;

    --it;

    if (blockNumber < it.key() + it.value().first)
        return it.value().second;

    return -1;
}

void SideDiffEditorWidget::clearAll(const QString &message)
{
    setBlockSelection(false);
    clear();
    clearAllData();
    setExtraSelections(TextEditorWidget::OtherSelection,
                       QList<QTextEdit::ExtraSelection>());
    setPlainText(message);
}

void SideDiffEditorWidget::clearAllData()
{
    m_lineNumberDigits = 1;
    m_lineNumbers.clear();
    m_fileInfo.clear();
    m_skippedLines.clear();
    m_chunkInfo.clear();
    m_separators.clear();
    setSelections(QMap<int, QList<DiffSelection> >());
}

void SideDiffEditorWidget::scrollContentsBy(int dx, int dy)
{
    SelectableTextEditorWidget::scrollContentsBy(dx, dy);
    // TODO: update only chunk lines
    viewport()->update();
}

void SideDiffEditorWidget::paintSeparator(QPainter &painter,
                                          QColor &color,
                                          const QString &text,
                                          const QTextBlock &block,
                                          int top)
{
    QPointF offset = contentOffset();
    painter.save();

    QColor foreground = color;
    if (!foreground.isValid())
        foreground = m_textForeground;
    if (!foreground.isValid())
        foreground = palette().windowText().color();

    painter.setPen(foreground);

    const QString replacementText = " {" + foldReplacementText(block) + "}; ";
    const int replacementTextWidth = fontMetrics().width(replacementText) + 24;
    int x = replacementTextWidth + int(offset.x());
    if (x < document()->documentMargin()
            || !TextDocumentLayout::isFolded(block)) {
        x = int(document()->documentMargin());
    }
    const QString elidedText = fontMetrics().elidedText(text,
                                                        Qt::ElideRight,
                                                        viewport()->width() - x);
    QTextLayout *layout = block.layout();
    QTextLine textLine = layout->lineAt(0);
    QRectF lineRect = textLine.naturalTextRect().translated(offset.x(), top);
    QRect clipRect = contentsRect();
    clipRect.setLeft(x);
    painter.setClipRect(clipRect);
    painter.drawText(QPointF(x, lineRect.top() + textLine.ascent()),
                     elidedText);
    painter.restore();
}

void SideDiffEditorWidget::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton && !(e->modifiers() & Qt::ShiftModifier)) {
        QTextCursor cursor = cursorForPosition(e->pos());
        jumpToOriginalFile(cursor);
        e->accept();
        return;
    }
    SelectableTextEditorWidget::mouseDoubleClickEvent(e);
}

void SideDiffEditorWidget::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return) {
        jumpToOriginalFile(textCursor());
        e->accept();
        return;
    }
    SelectableTextEditorWidget::keyPressEvent(e);
}

void SideDiffEditorWidget::contextMenuEvent(QContextMenuEvent *e)
{
    QPointer<QMenu> menu = createStandardContextMenu();

    QTextCursor cursor = cursorForPosition(e->pos());
    const int blockNumber = cursor.blockNumber();

    emit contextMenuRequested(menu, fileIndexForBlockNumber(blockNumber),
                              chunkIndexForBlockNumber(blockNumber));

    connect(this, &SideDiffEditorWidget::destroyed, menu.data(), &QMenu::deleteLater);
    menu->exec(e->globalPos());
    delete menu;
}

void SideDiffEditorWidget::jumpToOriginalFile(const QTextCursor &cursor)
{
    if (m_fileInfo.isEmpty())
        return;

    const int blockNumber = cursor.blockNumber();
    if (!m_lineNumbers.contains(blockNumber))
        return;

    const int lineNumber = m_lineNumbers.value(blockNumber);
    const int columnNumber = cursor.positionInBlock();

    emit jumpToOriginalFileRequested(fileIndexForBlockNumber(blockNumber),
                                     lineNumber, columnNumber);
}

static QString skippedText(int skippedNumber)
{
    if (skippedNumber > 0)
        return SideBySideDiffEditorWidget::tr("Skipped %n lines...", nullptr, skippedNumber);
    if (skippedNumber == -2)
        return SideBySideDiffEditorWidget::tr("Binary files differ");
    return SideBySideDiffEditorWidget::tr("Skipped unknown number of lines...");
}

void SideDiffEditorWidget::paintEvent(QPaintEvent *e)
{
    SelectableTextEditorWidget::paintEvent(e);

    QPainter painter(viewport());
    QPointF offset = contentOffset();
    QTextBlock currentBlock = firstVisibleBlock();

    while (currentBlock.isValid()) {
        if (currentBlock.isVisible()) {
            qreal top = blockBoundingGeometry(currentBlock).translated(offset).top();
            qreal bottom = top + blockBoundingRect(currentBlock).height();

            if (top > e->rect().bottom())
                break;

            if (bottom >= e->rect().top()) {
                const int blockNumber = currentBlock.blockNumber();

                auto it = m_skippedLines.constFind(blockNumber);
                if (it != m_skippedLines.constEnd()) {
                    QString skippedRowsText = '[' + skippedText(it->first) + ']';
                    if (!it->second.isEmpty())
                        skippedRowsText += ' ' + it->second;
                    paintSeparator(painter, m_chunkLineForeground,
                                   skippedRowsText, currentBlock, top);
                }

                const DiffFileInfo fileInfo = m_fileInfo.value(blockNumber);
                if (!fileInfo.fileName.isEmpty()) {
                    const QString fileNameText = fileInfo.typeInfo.isEmpty()
                            ? fileInfo.fileName
                            : tr("[%1] %2").arg(fileInfo.typeInfo)
                              .arg(fileInfo.fileName);
                    paintSeparator(painter, m_fileLineForeground,
                                   fileNameText, currentBlock, top);
                }
            }
        }
        currentBlock = currentBlock.next();
    }

    if (m_drawCollapsedBlock.isValid()) {
        // draw it now
        customDrawCollapsedBlockPopup(painter,
                                      m_drawCollapsedBlock,
                                      m_drawCollapsedOffset,
                                      m_drawCollapsedClip);
        // reset the data for the drawing
        m_drawCollapsedBlock = QTextBlock();
    }
}

void SideDiffEditorWidget::customDrawCollapsedBlockPopup(QPainter &painter,
                                                   const QTextBlock &block,
                                                   QPointF offset,
                                                   const QRect &clip)
{
    int margin = block.document()->documentMargin();
    qreal maxWidth = 0;
    qreal blockHeight = 0;
    QTextBlock b = block;

    while (!b.isVisible()) {
        const int blockNumber = b.blockNumber();
        if (!m_skippedLines.contains(blockNumber) && !m_separators.contains(blockNumber)) {
            b.setVisible(true); // make sure block bounding rect works
            QRectF r = blockBoundingRect(b).translated(offset);

            QTextLayout *layout = b.layout();
            for (int i = layout->lineCount()-1; i >= 0; --i)
                maxWidth = qMax(maxWidth, layout->lineAt(i).naturalTextWidth() + 2*margin);

            blockHeight += r.height();

            b.setVisible(false); // restore previous state
            b.setLineCount(0); // restore 0 line count for invisible block
        }
        b = b.next();
    }

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.translate(.5, .5);
    QBrush brush = palette().base();
    const QTextCharFormat &ifdefedOutFormat
            = textDocument()->fontSettings().toTextCharFormat(C_DISABLED_CODE);
    if (ifdefedOutFormat.hasProperty(QTextFormat::BackgroundBrush))
        brush = ifdefedOutFormat.background();
    painter.setBrush(brush);
    painter.drawRoundedRect(QRectF(offset.x(),
                                   offset.y(),
                                   maxWidth, blockHeight).adjusted(0, 0, 0, 0), 3, 3);
    painter.restore();

    QTextBlock end = b;
    b = block;
    while (b != end) {
        const int blockNumber = b.blockNumber();
        if (!m_skippedLines.contains(blockNumber) && !m_separators.contains(blockNumber)) {
            b.setVisible(true); // make sure block bounding rect works
            QRectF r = blockBoundingRect(b).translated(offset);
            QTextLayout *layout = b.layout();
            QVector<QTextLayout::FormatRange> selections;
            layout->draw(&painter, offset, selections, clip);

            b.setVisible(false); // restore previous state
            b.setLineCount(0); // restore 0 line count for invisible block
            offset.ry() += r.height();
        }
        b = b.next();
    }
}

void SideDiffEditorWidget::drawCollapsedBlockPopup(QPainter &painter,
                                                   const QTextBlock &block,
                                                   QPointF offset,
                                                   const QRect &clip)
{
    Q_UNUSED(painter)
    // Called from inside SelectableTextEditorWidget::paintEvent().
    // Postpone the drawing for now, do it after our paintEvent's
    // custom painting. Store the data for the future redraw.
    m_drawCollapsedBlock = block;
    m_drawCollapsedOffset = offset;
    m_drawCollapsedClip = clip;
}

void SideDiffEditorWidget::focusInEvent(QFocusEvent *e)
{
    SelectableTextEditorWidget::focusInEvent(e);
    emit gotFocus();
}

//////////////////

SideBySideDiffEditorWidget::SideBySideDiffEditorWidget(QWidget *parent)
    : QWidget(parent)
    , m_controller(this)
{
    m_leftEditor = new SideDiffEditorWidget(this);
    m_leftEditor->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_leftEditor->setReadOnly(true);
    m_leftEditor->setCodeStyle(TextEditorSettings::codeStyle());
    connect(m_leftEditor, &SideDiffEditorWidget::jumpToOriginalFileRequested,
            this, &SideBySideDiffEditorWidget::slotLeftJumpToOriginalFileRequested);
    connect(m_leftEditor, &SideDiffEditorWidget::contextMenuRequested,
            this, &SideBySideDiffEditorWidget::slotLeftContextMenuRequested,
            Qt::DirectConnection);

    m_rightEditor = new SideDiffEditorWidget(this);
    m_rightEditor->setReadOnly(true);
    m_rightEditor->setCodeStyle(TextEditorSettings::codeStyle());
    connect(m_rightEditor, &SideDiffEditorWidget::jumpToOriginalFileRequested,
            this, &SideBySideDiffEditorWidget::slotRightJumpToOriginalFileRequested);
    connect(m_rightEditor, &SideDiffEditorWidget::contextMenuRequested,
            this, &SideBySideDiffEditorWidget::slotRightContextMenuRequested,
            Qt::DirectConnection);

    auto setupHighlightController = [this]() {
        HighlightScrollBarController *highlightController = m_leftEditor->highlightScrollBarController();
        if (highlightController)
            highlightController->setScrollArea(m_rightEditor);
    };

    setupHighlightController();
    connect(m_leftEditor, &SideDiffEditorWidget::gotDisplaySettings, this, setupHighlightController);

    m_rightEditor->verticalScrollBar()->setFocusProxy(m_leftEditor);
    connect(m_leftEditor, &SideDiffEditorWidget::gotFocus, this, [this]() {
        if (m_rightEditor->verticalScrollBar()->focusProxy() == m_leftEditor)
            return; // We already did it before.

        // Hack #1. If the left editor got a focus last time
        // we don't want to focus right editor when clicking the right
        // scrollbar.
        m_rightEditor->verticalScrollBar()->setFocusProxy(m_leftEditor);

        // Hack #2. If the focus is currently not on the scrollbar's proxy
        // and we click on the scrollbar, the focus will go to the parent
        // of the scrollbar. In order to give the focus to the proxy
        // we need to set a click focus policy on the scrollbar.
        // See QApplicationPrivate::giveFocusAccordingToFocusPolicy().
        m_rightEditor->verticalScrollBar()->setFocusPolicy(Qt::ClickFocus);

        // Hack #3. Setting the focus policy is not orthogonal to setting
        // the focus proxy and unfortuantely it changes the policy of the proxy
        // too. We bring back the original policy to keep tab focus working.
        m_leftEditor->setFocusPolicy(Qt::StrongFocus);
    });
    connect(m_rightEditor, &SideDiffEditorWidget::gotFocus, this, [this]() {
        // Unhack #1.
        m_rightEditor->verticalScrollBar()->setFocusProxy(nullptr);
        // Unhack #2.
        m_rightEditor->verticalScrollBar()->setFocusPolicy(Qt::NoFocus);
    });

    connect(TextEditorSettings::instance(),
            &TextEditorSettings::fontSettingsChanged,
            this, &SideBySideDiffEditorWidget::setFontSettings);
    setFontSettings(TextEditorSettings::fontSettings());

    connect(m_leftEditor->verticalScrollBar(), &QAbstractSlider::valueChanged,
            this, &SideBySideDiffEditorWidget::leftVSliderChanged);
    connect(m_leftEditor->verticalScrollBar(), &QAbstractSlider::actionTriggered,
            this, &SideBySideDiffEditorWidget::leftVSliderChanged);

    connect(m_leftEditor->horizontalScrollBar(), &QAbstractSlider::valueChanged,
            this, &SideBySideDiffEditorWidget::leftHSliderChanged);
    connect(m_leftEditor->horizontalScrollBar(), &QAbstractSlider::actionTriggered,
            this, &SideBySideDiffEditorWidget::leftHSliderChanged);

    connect(m_leftEditor, &QPlainTextEdit::cursorPositionChanged,
            this, &SideBySideDiffEditorWidget::leftCursorPositionChanged);

    connect(m_rightEditor->verticalScrollBar(), &QAbstractSlider::valueChanged,
            this, &SideBySideDiffEditorWidget::rightVSliderChanged);
    connect(m_rightEditor->verticalScrollBar(), &QAbstractSlider::actionTriggered,
            this, &SideBySideDiffEditorWidget::rightVSliderChanged);

    connect(m_rightEditor->horizontalScrollBar(), &QAbstractSlider::valueChanged,
            this, &SideBySideDiffEditorWidget::rightHSliderChanged);
    connect(m_rightEditor->horizontalScrollBar(), &QAbstractSlider::actionTriggered,
            this, &SideBySideDiffEditorWidget::rightHSliderChanged);

    connect(m_rightEditor, &QPlainTextEdit::cursorPositionChanged,
            this, &SideBySideDiffEditorWidget::rightCursorPositionChanged);

    connect(m_leftEditor, &SideDiffEditorWidget::foldChanged,
            m_rightEditor, &SideDiffEditorWidget::setFolded);
    connect(m_rightEditor, &SideDiffEditorWidget::foldChanged,
            m_leftEditor, &SideDiffEditorWidget::setFolded);

    connect(m_leftEditor->horizontalScrollBar(), &QAbstractSlider::rangeChanged,
            this, &SideBySideDiffEditorWidget::syncHorizontalScrollBarPolicy);

    connect(m_rightEditor->horizontalScrollBar(), &QAbstractSlider::rangeChanged,
            this, &SideBySideDiffEditorWidget::syncHorizontalScrollBarPolicy);

    syncHorizontalScrollBarPolicy();

    m_splitter = new MiniSplitter(this);
    m_splitter->addWidget(m_leftEditor);
    m_splitter->addWidget(m_rightEditor);
    QVBoxLayout *l = new QVBoxLayout(this);
    l->setMargin(0);
    l->addWidget(m_splitter);
    setFocusProxy(m_leftEditor);

    m_leftContext = new IContext(this);
    m_leftContext->setWidget(m_leftEditor);
    m_leftContext->setContext(Core::Context(Core::Id(Constants::SIDE_BY_SIDE_VIEW_ID).withSuffix(1)));
    Core::ICore::addContextObject(m_leftContext);
    m_rightContext = new IContext(this);
    m_rightContext->setWidget(m_rightEditor);
    m_rightContext->setContext(Core::Context(Core::Id(Constants::SIDE_BY_SIDE_VIEW_ID).withSuffix(2)));
    Core::ICore::addContextObject(m_rightContext);
}

SideBySideDiffEditorWidget::~SideBySideDiffEditorWidget()
{
    Core::ICore::removeContextObject(m_leftContext);
    Core::ICore::removeContextObject(m_rightContext);
}

TextEditorWidget *SideBySideDiffEditorWidget::leftEditorWidget() const
{
    return m_leftEditor;
}

TextEditorWidget *SideBySideDiffEditorWidget::rightEditorWidget() const
{
    return m_rightEditor;
}

void SideBySideDiffEditorWidget::setDocument(DiffEditorDocument *document)
{
    m_controller.setDocument(document);
    clear();
    QList<FileData> diffFileList;
    QString workingDirectory;
    if (document) {
        diffFileList = document->diffFiles();
        workingDirectory = document->baseDirectory();
    }
    setDiff(diffFileList, workingDirectory);
}

DiffEditorDocument *SideBySideDiffEditorWidget::diffDocument() const
{
    return m_controller.document();
}

void SideBySideDiffEditorWidget::clear(const QString &message)
{
    const bool oldIgnore = m_controller.m_ignoreCurrentIndexChange;
    m_controller.m_ignoreCurrentIndexChange = true;
    setDiff(QList<FileData>(), QString());
    m_leftEditor->clearAll(message);
    m_rightEditor->clearAll(message);
    m_controller.m_ignoreCurrentIndexChange = oldIgnore;
}

void SideBySideDiffEditorWidget::setDiff(const QList<FileData> &diffFileList,
                                         const QString &workingDirectory)
{
    Q_UNUSED(workingDirectory)

    const bool oldIgnore = m_controller.m_ignoreCurrentIndexChange;
    m_controller.m_ignoreCurrentIndexChange = true;
    m_leftEditor->clear();
    m_rightEditor->clear();

    m_controller.m_contextFileData = diffFileList;
    if (m_controller.m_contextFileData.isEmpty()) {
        const QString msg = tr("No difference.");
        m_leftEditor->setPlainText(msg);
        m_rightEditor->setPlainText(msg);
    } else {
        showDiff();
    }
    m_controller.m_ignoreCurrentIndexChange = oldIgnore;
}

void SideBySideDiffEditorWidget::setCurrentDiffFileIndex(int diffFileIndex)
{
    if (m_controller.m_ignoreCurrentIndexChange)
        return;

    const int blockNumber = m_leftEditor->blockNumberForFileIndex(diffFileIndex);

    const bool oldIgnore = m_controller.m_ignoreCurrentIndexChange;
    m_controller.m_ignoreCurrentIndexChange = true;
    QTextBlock leftBlock = m_leftEditor->document()->findBlockByNumber(blockNumber);
    QTextCursor leftCursor = m_leftEditor->textCursor();
    leftCursor.setPosition(leftBlock.position());
    m_leftEditor->setTextCursor(leftCursor);
    m_leftEditor->verticalScrollBar()->setValue(blockNumber);

    QTextBlock rightBlock = m_rightEditor->document()->findBlockByNumber(blockNumber);
    QTextCursor rightCursor = m_rightEditor->textCursor();
    rightCursor.setPosition(rightBlock.position());
    m_rightEditor->setTextCursor(rightCursor);
    m_rightEditor->verticalScrollBar()->setValue(blockNumber);

    m_controller.m_ignoreCurrentIndexChange = oldIgnore;
}

void SideBySideDiffEditorWidget::setHorizontalSync(bool sync)
{
    m_horizontalSync = sync;
    rightHSliderChanged();
}

void SideBySideDiffEditorWidget::saveState()
{
    m_leftEditor->saveState();
    m_rightEditor->saveState();
}

void SideBySideDiffEditorWidget::restoreState()
{
    m_leftEditor->restoreState();
    m_rightEditor->restoreState();
}

void SideBySideDiffEditorWidget::showDiff()
{
    QMap<int, QList<DiffSelection> > leftFormats;
    QMap<int, QList<DiffSelection> > rightFormats;

    QString leftTexts, rightTexts;
    int blockNumber = 0;
    QChar separator = '\n';
    QHash<int, int> foldingIndent;
    for (const FileData &contextFileData : m_controller.m_contextFileData) {
        QString leftText, rightText;

        foldingIndent.insert(blockNumber, 1);
        leftFormats[blockNumber].append(DiffSelection(&m_controller.m_fileLineFormat));
        rightFormats[blockNumber].append(DiffSelection(&m_controller.m_fileLineFormat));
        m_leftEditor->setFileInfo(blockNumber, contextFileData.leftFileInfo);
        m_rightEditor->setFileInfo(blockNumber, contextFileData.rightFileInfo);
        leftText = separator;
        rightText = separator;
        blockNumber++;

        int lastLeftLineNumber = -1;

        if (contextFileData.binaryFiles) {
            foldingIndent.insert(blockNumber, 2);
            leftFormats[blockNumber].append(DiffSelection(&m_controller.m_chunkLineFormat));
            rightFormats[blockNumber].append(DiffSelection(&m_controller.m_chunkLineFormat));
            m_leftEditor->setSkippedLines(blockNumber, -2);
            m_rightEditor->setSkippedLines(blockNumber, -2);
            leftText += separator;
            rightText += separator;
            blockNumber++;
        } else {
            for (int j = 0; j < contextFileData.chunks.count(); j++) {
                const ChunkData &chunkData = contextFileData.chunks.at(j);

                int leftLineNumber = chunkData.leftStartingLineNumber;
                int rightLineNumber = chunkData.rightStartingLineNumber;

                if (!chunkData.contextChunk) {
                    const int skippedLines = leftLineNumber - lastLeftLineNumber - 1;
                    if (skippedLines > 0) {
                        foldingIndent.insert(blockNumber, 2);
                        leftFormats[blockNumber].append(DiffSelection(&m_controller.m_chunkLineFormat));
                        rightFormats[blockNumber].append(DiffSelection(&m_controller.m_chunkLineFormat));
                        m_leftEditor->setSkippedLines(blockNumber, skippedLines, chunkData.contextInfo);
                        m_rightEditor->setSkippedLines(blockNumber, skippedLines, chunkData.contextInfo);
                        leftText += separator;
                        rightText += separator;
                        blockNumber++;
                    }

                    m_leftEditor->setChunkIndex(blockNumber, chunkData.rows.count(), j);
                    m_rightEditor->setChunkIndex(blockNumber, chunkData.rows.count(), j);

                    for (const RowData &rowData : chunkData.rows) {
                        TextLineData leftLineData = rowData.leftLine;
                        TextLineData rightLineData = rowData.rightLine;
                        if (leftLineData.textLineType == TextLineData::TextLine) {
                            leftText += leftLineData.text;
                            lastLeftLineNumber = leftLineNumber;
                            leftLineNumber++;
                            m_leftEditor->setLineNumber(blockNumber, leftLineNumber);
                        } else if (leftLineData.textLineType == TextLineData::Separator) {
                            m_leftEditor->setSeparator(blockNumber, true);
                        }

                        if (rightLineData.textLineType == TextLineData::TextLine) {
                            rightText += rightLineData.text;
                            rightLineNumber++;
                            m_rightEditor->setLineNumber(blockNumber, rightLineNumber);
                        } else if (rightLineData.textLineType == TextLineData::Separator) {
                            m_rightEditor->setSeparator(blockNumber, true);
                        }

                        if (!rowData.equal) {
                            if (rowData.leftLine.textLineType == TextLineData::TextLine)
                                leftFormats[blockNumber].append(DiffSelection(&m_controller.m_leftLineFormat));
                            else
                                leftFormats[blockNumber].append(DiffSelection(&m_spanLineFormat));
                            if (rowData.rightLine.textLineType == TextLineData::TextLine)
                                rightFormats[blockNumber].append(DiffSelection(&m_controller.m_rightLineFormat));
                            else
                                rightFormats[blockNumber].append(DiffSelection(&m_spanLineFormat));
                        }

                        for (auto it = leftLineData.changedPositions.cbegin(),
                                  end = leftLineData.changedPositions.cend(); it != end; ++it) {
                            leftFormats[blockNumber].append(
                                        DiffSelection(it.key(), it.value(),
                                                      &m_controller.m_leftCharFormat));
                        }

                        for (auto it = rightLineData.changedPositions.cbegin(),
                                  end = rightLineData.changedPositions.cend(); it != end; ++it) {
                            rightFormats[blockNumber].append(
                                        DiffSelection(it.key(), it.value(),
                                                      &m_controller.m_rightCharFormat));
                        }

                        leftText += separator;
                        rightText += separator;
                        blockNumber++;
                    }
                }

                if (j == contextFileData.chunks.count() - 1) { // the last chunk
                    int skippedLines = -2;
                    if (chunkData.contextChunk) {
                        // if it's context chunk
                        skippedLines = chunkData.rows.count();
                    } else if (!contextFileData.lastChunkAtTheEndOfFile
                               && !contextFileData.contextChunksIncluded) {
                        // if not a context chunk and not a chunk at the end of file
                        // and context lines not included
                        skippedLines = -1; // unknown count skipped by the end of file
                    }

                    if (skippedLines >= -1) {
                        leftFormats[blockNumber].append(DiffSelection(&m_controller.m_chunkLineFormat));
                        rightFormats[blockNumber].append(DiffSelection(&m_controller.m_chunkLineFormat));
                        m_leftEditor->setSkippedLines(blockNumber, skippedLines);
                        m_rightEditor->setSkippedLines(blockNumber, skippedLines);
                        leftText += separator;
                        rightText += separator;
                        blockNumber++;
                    } // otherwise nothing skipped
                }
            }
        }
        leftText.replace('\r', ' ');
        rightText.replace('\r', ' ');
        leftTexts += leftText;
        rightTexts += rightText;
    }

    if (leftTexts.isEmpty() && rightTexts.isEmpty())
        return;

    const bool oldIgnore = m_controller.m_ignoreCurrentIndexChange;
    m_controller.m_ignoreCurrentIndexChange = true;
    m_leftEditor->clear();
    m_leftEditor->setPlainText(leftTexts);
    m_rightEditor->clear();
    m_rightEditor->setPlainText(rightTexts);
    m_controller.m_ignoreCurrentIndexChange = oldIgnore;

    QTextBlock block = m_leftEditor->document()->firstBlock();
    for (int b = 0; block.isValid(); block = block.next(), ++b)
        SelectableTextEditorWidget::setFoldingIndent(block, foldingIndent.value(b, 3));
    block = m_rightEditor->document()->firstBlock();
    for (int b = 0; block.isValid(); block = block.next(), ++b)
        SelectableTextEditorWidget::setFoldingIndent(block, foldingIndent.value(b, 3));

    m_leftEditor->setSelections(leftFormats);
    m_rightEditor->setSelections(rightFormats);
}

void SideBySideDiffEditorWidget::setFontSettings(
        const FontSettings &fontSettings)
{
    m_spanLineFormat  = fontSettings.toTextCharFormat(C_LINE_NUMBER);
    m_controller.setFontSettings(fontSettings);
}

void SideBySideDiffEditorWidget::slotLeftJumpToOriginalFileRequested(
        int diffFileIndex,
        int lineNumber,
        int columnNumber)
{
    if (diffFileIndex < 0 || diffFileIndex >= m_controller.m_contextFileData.count())
        return;

    const FileData fileData = m_controller.m_contextFileData.at(diffFileIndex);
    const QString leftFileName = fileData.leftFileInfo.fileName;
    const QString rightFileName = fileData.rightFileInfo.fileName;
    if (leftFileName == rightFileName) {
        // The same file (e.g. in git diff), jump to the line number taken from the right editor.
        // Warning: git show SHA^ vs SHA or git diff HEAD vs Index
        // (when Working tree has changed in meantime) will not work properly.
        for (const ChunkData &chunkData : fileData.chunks) {

            int leftLineNumber = chunkData.leftStartingLineNumber;
            int rightLineNumber = chunkData.rightStartingLineNumber;

            for (int j = 0; j < chunkData.rows.count(); j++) {
                const RowData rowData = chunkData.rows.at(j);
                if (rowData.leftLine.textLineType == TextLineData::TextLine)
                    leftLineNumber++;
                if (rowData.rightLine.textLineType == TextLineData::TextLine)
                    rightLineNumber++;
                if (leftLineNumber == lineNumber) {
                    int colNr = rowData.equal ? columnNumber : 0;
                    m_controller.jumpToOriginalFile(leftFileName, rightLineNumber, colNr);
                    return;
                }
            }
        }
    } else {
        // different file (e.g. in Tools | Diff...)
        m_controller.jumpToOriginalFile(leftFileName, lineNumber, columnNumber);
    }
}

void SideBySideDiffEditorWidget::slotRightJumpToOriginalFileRequested(
        int diffFileIndex,
        int lineNumber,
        int columnNumber)
{
    if (diffFileIndex < 0 || diffFileIndex >= m_controller.m_contextFileData.count())
        return;

    const FileData fileData = m_controller.m_contextFileData.at(diffFileIndex);
    const QString fileName = fileData.rightFileInfo.fileName;
    m_controller.jumpToOriginalFile(fileName, lineNumber, columnNumber);
}

void SideBySideDiffEditorWidget::slotLeftContextMenuRequested(QMenu *menu,
                                                              int fileIndex,
                                                              int chunkIndex)
{
    menu->addSeparator();

    m_controller.addCodePasterAction(menu, fileIndex, chunkIndex);
    m_controller.addApplyAction(menu, fileIndex, chunkIndex);
    m_controller.addExtraActions(menu, fileIndex, chunkIndex);
}

void SideBySideDiffEditorWidget::slotRightContextMenuRequested(QMenu *menu,
                                                               int fileIndex,
                                                               int chunkIndex)
{
    menu->addSeparator();

    m_controller.addCodePasterAction(menu, fileIndex, chunkIndex);
    m_controller.addRevertAction(menu, fileIndex, chunkIndex);
    m_controller.addExtraActions(menu, fileIndex, chunkIndex);
}

void SideBySideDiffEditorWidget::leftVSliderChanged()
{
    if (m_controller.m_ignoreCurrentIndexChange)
        return;

    m_rightEditor->verticalScrollBar()->setValue(m_leftEditor->verticalScrollBar()->value());
}

void SideBySideDiffEditorWidget::rightVSliderChanged()
{
    if (m_controller.m_ignoreCurrentIndexChange)
        return;

    m_leftEditor->verticalScrollBar()->setValue(m_rightEditor->verticalScrollBar()->value());
}

void SideBySideDiffEditorWidget::leftHSliderChanged()
{
    if (m_controller.m_ignoreCurrentIndexChange)
        return;

    if (m_horizontalSync)
        m_rightEditor->horizontalScrollBar()->setValue(m_leftEditor->horizontalScrollBar()->value());
}

void SideBySideDiffEditorWidget::rightHSliderChanged()
{
    if (m_controller.m_ignoreCurrentIndexChange)
        return;

    if (m_horizontalSync)
        m_leftEditor->horizontalScrollBar()->setValue(m_rightEditor->horizontalScrollBar()->value());
}

void SideBySideDiffEditorWidget::leftCursorPositionChanged()
{
    if (m_controller.m_ignoreCurrentIndexChange)
        return;

    handlePositionChange(m_leftEditor, m_rightEditor);
    leftVSliderChanged();
    leftHSliderChanged();
}

void SideBySideDiffEditorWidget::rightCursorPositionChanged()
{
    if (m_controller.m_ignoreCurrentIndexChange)
        return;

    handlePositionChange(m_rightEditor, m_leftEditor);
    rightVSliderChanged();
    rightHSliderChanged();
}

void SideBySideDiffEditorWidget::syncHorizontalScrollBarPolicy()
{
    const bool alwaysOn = m_leftEditor->horizontalScrollBar()->maximum()
            || m_rightEditor->horizontalScrollBar()->maximum();
    const Qt::ScrollBarPolicy newPolicy = alwaysOn
            ? Qt::ScrollBarAlwaysOn : Qt::ScrollBarAsNeeded;
    if (m_leftEditor->horizontalScrollBarPolicy() != newPolicy)
        m_leftEditor->setHorizontalScrollBarPolicy(newPolicy);
    if (m_rightEditor->horizontalScrollBarPolicy() != newPolicy)
        m_rightEditor->setHorizontalScrollBarPolicy(newPolicy);
}

void SideBySideDiffEditorWidget::handlePositionChange(SideDiffEditorWidget *source, SideDiffEditorWidget *dest)
{
    if (m_controller.m_ignoreCurrentIndexChange)
        return;

    const bool oldIgnore = m_controller.m_ignoreCurrentIndexChange;
    m_controller.m_ignoreCurrentIndexChange = true;
    syncCursor(source, dest);
    emit currentDiffFileIndexChanged(
                source->fileIndexForBlockNumber(source->textCursor().blockNumber()));
    m_controller.m_ignoreCurrentIndexChange = oldIgnore;
}

void SideBySideDiffEditorWidget::syncCursor(SideDiffEditorWidget *source, SideDiffEditorWidget *dest)
{
    const int oldHSliderPos = dest->horizontalScrollBar()->value();

    const QTextCursor sourceCursor = source->textCursor();
    const int sourceLine = sourceCursor.blockNumber();
    const int sourceColumn = sourceCursor.positionInBlock();
    QTextCursor destCursor = dest->textCursor();
    const QTextBlock destBlock = dest->document()->findBlockByNumber(sourceLine);
    const int destColumn = qMin(sourceColumn, destBlock.length());
    const int destPosition = destBlock.position() + destColumn;
    destCursor.setPosition(destPosition);
    dest->setTextCursor(destCursor);

    dest->horizontalScrollBar()->setValue(oldHSliderPos);
}

} // namespace Internal
} // namespace DiffEditor

#include "sidebysidediffeditorwidget.moc"
