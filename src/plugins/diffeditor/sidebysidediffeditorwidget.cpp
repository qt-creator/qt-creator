// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sidebysidediffeditorwidget.h"

#include "diffeditorconstants.h"
#include "diffeditordocument.h"
#include "diffeditortr.h"

#include <coreplugin/find/highlightscrollbarcontroller.h>
#include <coreplugin/icore.h>
#include <coreplugin/minisplitter.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <extensionsystem/pluginmanager.h>

#include <texteditor/displaysettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/textdocument.h>
#include <texteditor/textdocumentlayout.h>
#include <texteditor/texteditorsettings.h>

#include <utils/async.h>
#include <utils/mathutils.h>
#include <utils/tooltip/tooltip.h>

#include <QMenu>
#include <QPainter>
#include <QScrollBar>
#include <QVBoxLayout>

using namespace Core;
using namespace TextEditor;
using namespace Utils;

using namespace std::placeholders;

namespace DiffEditor::Internal {

static DiffSide oppositeSide(DiffSide side)
{
    return side == LeftSide ? RightSide : LeftSide;
}

class SideDiffEditorWidget : public SelectableTextEditorWidget
{
    Q_OBJECT
public:
    SideDiffEditorWidget(QWidget *parent = nullptr);

    void clearAll(const QString &message);
    void saveState();
    using TextEditor::TextEditorWidget::restoreState;
    void restoreState();

    void setFolded(int blockNumber, bool folded);

    void setDisplaySettings(const DisplaySettings &displaySettings) override;

    SideDiffData diffData() const { return m_data; }
    void setDiffData(const SideDiffData &data) { m_data = data; }

signals:
    void jumpToOriginalFileRequested(int diffFileIndex,
                                     int lineNumber,
                                     int columnNumber);
    void contextMenuRequested(QMenu *menu,
                              int diffFileIndex,
                              int chunkIndex,
                              const ChunkSelection &selection);
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

    SideDiffData m_data;

    QColor m_fileLineForeground;
    QColor m_chunkLineForeground;
    QColor m_textForeground;
    QByteArray m_state;

    QTextBlock m_drawCollapsedBlock;
    QPointF m_drawCollapsedOffset;
    QRect m_drawCollapsedClip;
};

SideDiffEditorWidget::SideDiffEditorWidget(QWidget *parent)
    : SelectableTextEditorWidget("DiffEditor.SideDiffEditor", parent)
{
    connect(this, &TextEditorWidget::tooltipRequested, this, [this](const QPoint &point, int position) {
        const int block = document()->findBlock(position).blockNumber();
        const auto it = m_data.m_fileInfo.constFind(block);
        if (it != m_data.m_fileInfo.constEnd())
            ToolTip::show(point, it.value().fileName, this);
        else
            ToolTip::hide();
    });

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

void SideDiffEditorWidget::setDisplaySettings(const DisplaySettings &displaySettings)
{
    SelectableTextEditorWidget::setDisplaySettings(displaySettings);
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
    const auto it = m_data.m_lineNumbers.constFind(blockNumber);
    if (it != m_data.m_lineNumbers.constEnd())
        return QString::number(it.value());
    return {};
}

int SideDiffEditorWidget::lineNumberDigits() const
{
    return m_data.m_lineNumberDigits;
}

bool SideDiffEditorWidget::selectionVisible(int blockNumber) const
{
    return !m_data.m_separators.value(blockNumber, false);
}

bool SideDiffEditorWidget::replacementVisible(int blockNumber) const
{
    return m_data.isChunkLine(blockNumber) || (m_data.isFileLine(blockNumber)
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
        return {}; // no selection

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
                    text += QStringView(block.text()).left(endPosition - block.position());
                else
                    text += block.text();
            }
            textInserted = true;
        }
        block = block.next();
    }

    return TextDocument::convertToPlainText(text);
}

static SideBySideDiffOutput diffOutput(QPromise<SideBySideShowResults> &promise, int progressMin,
                                       int progressMax, const DiffEditorInput &input)
{
    SideBySideDiffOutput output;

    const QChar separator = '\n';
    int blockNumber = 0;
    int i = 0;
    const int count = input.m_contextFileData.size();
    std::array<QString, SideCount> diffText{};

    auto addFileLine = [&](DiffSide side, const FileData &fileData) {
        output.side[side].selections[blockNumber].append({input.m_fileLineFormat});
        output.side[side].diffData.setFileInfo(blockNumber, fileData.fileInfo[side]);
        diffText[side] += separator;
    };

    auto addChunkLine = [&](DiffSide side, int skippedLines, const QString &contextInfo = {}) {
        output.side[side].selections[blockNumber].append({input.m_chunkLineFormat});
        output.side[side].diffData.setSkippedLines(blockNumber, skippedLines, contextInfo);
        diffText[side] += separator;
    };

    auto addRowLine = [&](DiffSide side, const RowData &rowData,
                          int *lineNumber, int *lastLineNumber = nullptr) {
        if (rowData.line[side].textLineType == TextLineData::TextLine) {
            diffText[side] += rowData.line[side].text;
            if (lastLineNumber)
                *lastLineNumber = *lineNumber;
            ++(*lineNumber);
            output.side[side].diffData.setLineNumber(blockNumber, *lineNumber);
        } else if (rowData.line[side].textLineType == TextLineData::Separator) {
            output.side[side].diffData.setSeparator(blockNumber, true);
        }

        if (!rowData.equal) {
            if (rowData.line[side].textLineType == TextLineData::TextLine)
                output.side[side].selections[blockNumber].append({input.m_lineFormat[side]});
            else
                output.side[side].selections[blockNumber].append({input.m_spanLineFormat});
        }
        for (auto it = rowData.line[side].changedPositions.cbegin(),
                  end = rowData.line[side].changedPositions.cend(); it != end; ++it) {
            output.side[side].selections[blockNumber].append(
                        {input.m_charFormat[side], it.key(), it.value()});
        }
        diffText[side] += separator;
    };

    auto addSkippedLine = [&](DiffSide side, int skippedLines) {
        output.side[side].selections[blockNumber].append({input.m_chunkLineFormat});
        output.side[side].diffData.setSkippedLines(blockNumber, skippedLines);
        diffText[side] += separator;
    };

    for (const FileData &contextFileData : input.m_contextFileData) {
        diffText = {};
        output.foldingIndent.insert(blockNumber, 1);

        addFileLine(LeftSide, contextFileData);
        addFileLine(RightSide, contextFileData);
        blockNumber++;

        int lastLeftLineNumber = -1;

        if (contextFileData.binaryFiles) {
            output.foldingIndent.insert(blockNumber, 2);
            addChunkLine(LeftSide, -2);
            addChunkLine(RightSide, -2);
            blockNumber++;
        } else {
            for (int j = 0; j < contextFileData.chunks.size(); j++) {
                const ChunkData &chunkData = contextFileData.chunks.at(j);

                int leftLineNumber = chunkData.startingLineNumber[LeftSide];
                int rightLineNumber = chunkData.startingLineNumber[RightSide];

                if (!chunkData.contextChunk) {
                    const int skippedLines = leftLineNumber - lastLeftLineNumber - 1;
                    if (skippedLines > 0) {
                        output.foldingIndent.insert(blockNumber, 2);
                        addChunkLine(LeftSide, skippedLines, chunkData.contextInfo);
                        addChunkLine(RightSide, skippedLines, chunkData.contextInfo);
                        blockNumber++;
                    }

                    const int rows = chunkData.rows.size();
                    output.side[LeftSide].diffData.m_chunkInfo.setChunkIndex(blockNumber, rows, j);
                    output.side[RightSide].diffData.m_chunkInfo.setChunkIndex(blockNumber, rows, j);

                    for (const RowData &rowData : chunkData.rows) {
                        addRowLine(LeftSide, rowData, &leftLineNumber, &lastLeftLineNumber);
                        addRowLine(RightSide, rowData, &rightLineNumber);
                        blockNumber++;
                    }
                }

                if (j == contextFileData.chunks.size() - 1) { // the last chunk
                    int skippedLines = -2;
                    if (chunkData.contextChunk) {
                        // if it's context chunk
                        skippedLines = chunkData.rows.size();
                    } else if (!contextFileData.lastChunkAtTheEndOfFile
                               && !contextFileData.contextChunksIncluded) {
                        // if not a context chunk and not a chunk at the end of file
                        // and context lines not included
                        skippedLines = -1; // unknown count skipped by the end of file
                    }

                    if (skippedLines >= -1) {
                        addSkippedLine(LeftSide, skippedLines);
                        addSkippedLine(RightSide, skippedLines);
                        blockNumber++;
                    } // otherwise nothing skipped
                }
            }
        }
        diffText[LeftSide].replace('\r', ' ');
        diffText[RightSide].replace('\r', ' ');
        output.side[LeftSide].diffText += diffText[LeftSide];
        output.side[RightSide].diffText += diffText[RightSide];
        promise.setProgressValue(MathUtils::interpolateLinear(++i, 0, count, progressMin, progressMax));
        if (promise.isCanceled())
            return {};
    }
    output.side[LeftSide].selections = SelectableTextEditorWidget::polishedSelections(
                output.side[LeftSide].selections);
    output.side[RightSide].selections = SelectableTextEditorWidget::polishedSelections(
                output.side[RightSide].selections);
    return output;
}

void SideDiffData::setLineNumber(int blockNumber, int lineNumber)
{
    const QString lineNumberString = QString::number(lineNumber);
    m_lineNumbers.insert(blockNumber, lineNumber);
    m_lineNumberDigits = qMax(m_lineNumberDigits, lineNumberString.size());
}

void SideDiffData::setFileInfo(int blockNumber, const DiffFileInfo &fileInfo)
{
    m_fileInfo[blockNumber] = fileInfo;
    setSeparator(blockNumber, true);
}

int SideDiffData::blockNumberForFileIndex(int fileIndex) const
{
    if (fileIndex < 0 || fileIndex >= m_fileInfo.count())
        return -1;

    return std::next(m_fileInfo.constBegin(), fileIndex).key();
}

int SideDiffData::fileIndexForBlockNumber(int blockNumber) const
{
    int i = -1;
    for (auto it = m_fileInfo.cbegin(), end = m_fileInfo.cend(); it != end; ++it, ++i) {
        if (it.key() > blockNumber)
            break;
    }

    return i;
}

void SideDiffEditorWidget::clearAll(const QString &message)
{
    clear();
    m_data = {};
    setSelections({});
    setExtraSelections(TextEditorWidget::OtherSelection, {});
    setPlainText(message);
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
    const int replacementTextWidth = fontMetrics().horizontalAdvance(replacementText) + 24;
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

    const QTextCursor tc = textCursor();
    QTextCursor start = tc;
    start.setPosition(tc.selectionStart());
    QTextCursor end = tc;
    end.setPosition(tc.selectionEnd());
    const int startBlockNumber = start.blockNumber();
    const int endBlockNumber = end.blockNumber();

    QTextCursor cursor = cursorForPosition(e->pos());
    const int blockNumber = cursor.blockNumber();

    const int fileIndex = m_data.fileIndexForBlockNumber(blockNumber);
    const int chunkIndex = m_data.m_chunkInfo.chunkIndexForBlockNumber(blockNumber);

    const int selectionStartFileIndex = m_data.fileIndexForBlockNumber(startBlockNumber);
    const int selectionStartChunkIndex = m_data.m_chunkInfo.chunkIndexForBlockNumber(startBlockNumber);
    const int selectionEndFileIndex = m_data.fileIndexForBlockNumber(endBlockNumber);
    const int selectionEndChunkIndex = m_data.m_chunkInfo.chunkIndexForBlockNumber(endBlockNumber);

    const int selectionStart = selectionStartFileIndex == fileIndex
            && selectionStartChunkIndex == chunkIndex
            ? m_data.m_chunkInfo.chunkRowForBlockNumber(startBlockNumber)
            : 0;

    const int selectionEnd = selectionEndFileIndex == fileIndex
            && selectionEndChunkIndex == chunkIndex
            ? m_data.m_chunkInfo.chunkRowForBlockNumber(endBlockNumber)
            : m_data.m_chunkInfo.chunkRowsCountForBlockNumber(blockNumber);

    QList<int> rows;
    for (int i = selectionStart; i <= selectionEnd; ++i)
        rows.append(i);

    const ChunkSelection selection(rows, rows);

    emit contextMenuRequested(menu, m_data.fileIndexForBlockNumber(blockNumber),
                              m_data.m_chunkInfo.chunkIndexForBlockNumber(blockNumber),
                              selection);

    connect(this, &SideDiffEditorWidget::destroyed, menu.data(), &QMenu::deleteLater);
    menu->exec(e->globalPos());
    delete menu;
}

void SideDiffEditorWidget::jumpToOriginalFile(const QTextCursor &cursor)
{
    if (m_data.m_fileInfo.isEmpty())
        return;

    const int blockNumber = cursor.blockNumber();
    if (!m_data.m_lineNumbers.contains(blockNumber))
        return;

    const int lineNumber = m_data.m_lineNumbers.value(blockNumber);
    const int columnNumber = cursor.positionInBlock();

    emit jumpToOriginalFileRequested(m_data.fileIndexForBlockNumber(blockNumber),
                                     lineNumber, columnNumber);
}

static QString skippedText(int skippedNumber)
{
    if (skippedNumber > 0)
        return Tr::tr("Skipped %n lines...", nullptr, skippedNumber);
    if (skippedNumber == -2)
        return Tr::tr("Binary files differ");
    return Tr::tr("Skipped unknown number of lines...");
}

void SideDiffEditorWidget::paintEvent(QPaintEvent *e)
{
    SelectableTextEditorWidget::paintEvent(e);

    QPainter painter(viewport());
    const QPointF offset = contentOffset();
    QTextBlock currentBlock = firstVisibleBlock();

    while (currentBlock.isValid()) {
        if (currentBlock.isVisible()) {
            qreal top = blockBoundingGeometry(currentBlock).translated(offset).top();
            qreal bottom = top + blockBoundingRect(currentBlock).height();

            if (top > e->rect().bottom())
                break;

            if (bottom >= e->rect().top()) {
                const int blockNumber = currentBlock.blockNumber();

                auto it = m_data.m_skippedLines.constFind(blockNumber);
                if (it != m_data.m_skippedLines.constEnd()) {
                    QString skippedRowsText = '[' + skippedText(it->first) + ']';
                    if (!it->second.isEmpty())
                        skippedRowsText += ' ' + it->second;
                    paintSeparator(painter, m_chunkLineForeground,
                                   skippedRowsText, currentBlock, top);
                }

                const DiffFileInfo fileInfo = m_data.m_fileInfo.value(blockNumber);
                if (!fileInfo.fileName.isEmpty()) {
                    const QString fileNameText = fileInfo.typeInfo.isEmpty()
                            ? fileInfo.fileName
                            : Tr::tr("[%1] %2").arg(fileInfo.typeInfo, fileInfo.fileName);
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
        m_drawCollapsedBlock = {};
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
        if (!m_data.m_skippedLines.contains(blockNumber) && !m_data.m_separators.contains(blockNumber)) {
            b.setVisible(true); // make sure block bounding rect works
            QRectF r = blockBoundingRect(b).translated(offset);

            QTextLayout *layout = b.layout();
            for (int i = layout->lineCount() - 1; i >= 0; --i)
                maxWidth = qMax(maxWidth, layout->lineAt(i).naturalTextWidth() + 2 * margin);

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
    painter.drawRoundedRect(QRectF(offset.x(), offset.y(), maxWidth, blockHeight)
                            .adjusted(0, 0, 0, 0), 3, 3);
    painter.restore();

    QTextBlock end = b;
    b = block;
    while (b != end) {
        const int blockNumber = b.blockNumber();
        if (!m_data.m_skippedLines.contains(blockNumber) && !m_data.m_separators.contains(blockNumber)) {
            b.setVisible(true); // make sure block bounding rect works
            const QRectF r = blockBoundingRect(b).translated(offset);
            QTextLayout *layout = b.layout();
            layout->draw(&painter, offset, {}, clip);

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
    auto setupEditor = [this](DiffSide side) {
        m_editor[side] = new SideDiffEditorWidget(this);

        connect(m_editor[side], &SideDiffEditorWidget::jumpToOriginalFileRequested,
                this, std::bind(&SideBySideDiffEditorWidget::jumpToOriginalFileRequested, this,
                                side, _1, _2, _3));
        connect(m_editor[side], &SideDiffEditorWidget::contextMenuRequested,
                this, std::bind(&SideBySideDiffEditorWidget::contextMenuRequested, this,
                                side, _1, _2, _3, _4));

        connect(m_editor[side]->verticalScrollBar(), &QAbstractSlider::valueChanged,
                this, std::bind(&SideBySideDiffEditorWidget::verticalSliderChanged, this, side));
        connect(m_editor[side]->verticalScrollBar(), &QAbstractSlider::actionTriggered,
                this, std::bind(&SideBySideDiffEditorWidget::verticalSliderChanged, this, side));

        connect(m_editor[side]->horizontalScrollBar(), &QAbstractSlider::valueChanged,
                this, std::bind(&SideBySideDiffEditorWidget::horizontalSliderChanged, this, side));
        connect(m_editor[side]->horizontalScrollBar(), &QAbstractSlider::actionTriggered,
                this, std::bind(&SideBySideDiffEditorWidget::horizontalSliderChanged, this, side));

        connect(m_editor[side], &QPlainTextEdit::cursorPositionChanged,
                this, std::bind(&SideBySideDiffEditorWidget::cursorPositionChanged, this, side));

        connect(m_editor[side]->horizontalScrollBar(), &QAbstractSlider::rangeChanged,
                this, &SideBySideDiffEditorWidget::syncHorizontalScrollBarPolicy);

        auto context = new IContext(this);
        context->setWidget(m_editor[side]);
        context->setContext(Context(Id(Constants::SIDE_BY_SIDE_VIEW_ID).withSuffix(side + 1)));
        ICore::addContextObject(context);
    };
    setupEditor(LeftSide);
    setupEditor(RightSide);
    m_editor[LeftSide]->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto setupHighlight = [this] {
        HighlightScrollBarController *ctrl = m_editor[LeftSide]->highlightScrollBarController();
        if (ctrl)
            ctrl->setScrollArea(m_editor[RightSide]);
    };
    setupHighlight();
    connect(m_editor[LeftSide], &SideDiffEditorWidget::gotDisplaySettings, this, setupHighlight);

    m_editor[RightSide]->verticalScrollBar()->setFocusProxy(m_editor[LeftSide]);
    connect(m_editor[LeftSide], &SideDiffEditorWidget::gotFocus, this, [this] {
        if (m_editor[RightSide]->verticalScrollBar()->focusProxy() == m_editor[LeftSide])
            return; // We already did it before.

        // Hack #1. If the left editor got a focus last time we don't want to focus right editor
        // when clicking the right scrollbar.
        m_editor[RightSide]->verticalScrollBar()->setFocusProxy(m_editor[LeftSide]);

        // Hack #2. If the focus is currently not on the scrollbar's proxy and we click on
        // the scrollbar, the focus will go to the parent of the scrollbar. In order to give
        // the focus to the proxy we need to set a click focus policy on the scrollbar.
        // See QApplicationPrivate::giveFocusAccordingToFocusPolicy().
        m_editor[RightSide]->verticalScrollBar()->setFocusPolicy(Qt::ClickFocus);

        // Hack #3. Setting the focus policy is not orthogonal to setting the focus proxy and
        // unfortuantely it changes the policy of the proxy too. We bring back the original policy
        // to keep tab focus working.
        m_editor[LeftSide]->setFocusPolicy(Qt::StrongFocus);
    });
    connect(m_editor[RightSide], &SideDiffEditorWidget::gotFocus, this, [this] {
        // Unhack #1.
        m_editor[RightSide]->verticalScrollBar()->setFocusProxy(nullptr);
        // Unhack #2.
        m_editor[RightSide]->verticalScrollBar()->setFocusPolicy(Qt::NoFocus);
    });

    connect(TextEditorSettings::instance(),
            &TextEditorSettings::fontSettingsChanged,
            this, &SideBySideDiffEditorWidget::setFontSettings);
    setFontSettings(TextEditorSettings::fontSettings());

    syncHorizontalScrollBarPolicy();

    m_splitter = new MiniSplitter(this);
    m_splitter->addWidget(m_editor[LeftSide]);
    m_splitter->addWidget(m_editor[RightSide]);
    QVBoxLayout *l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    l->addWidget(m_splitter);
    setFocusProxy(m_editor[LeftSide]);
}

SideBySideDiffEditorWidget::~SideBySideDiffEditorWidget() = default;

TextEditorWidget *SideBySideDiffEditorWidget::sideEditorWidget(DiffSide side) const
{
    return m_editor[side];
}

void SideBySideDiffEditorWidget::setDocument(DiffEditorDocument *document)
{
    m_controller.setDocument(document);
    clear();
    QList<FileData> diffFileList;
    if (document)
        diffFileList = document->diffFiles();
    setDiff(diffFileList);
}

DiffEditorDocument *SideBySideDiffEditorWidget::diffDocument() const
{
    return m_controller.document();
}

void SideBySideDiffEditorWidget::clear(const QString &message)
{
    const GuardLocker locker(m_controller.m_ignoreChanges);
    setDiff({});
    for (SideDiffEditorWidget *editor : m_editor)
        editor->clearAll(message);
    if (m_asyncTask) {
        m_asyncTask.reset();
        m_controller.setBusyShowing(false);
    }
}

void SideBySideDiffEditorWidget::setDiff(const QList<FileData> &diffFileList)
{
    const GuardLocker locker(m_controller.m_ignoreChanges);
    for (SideDiffEditorWidget *editor : m_editor)
        editor->clearAll(Tr::tr("Waiting for data..."));

    m_controller.m_contextFileData = diffFileList;
    if (m_controller.m_contextFileData.isEmpty()) {
        const QString msg = Tr::tr("No difference.");
        for (SideDiffEditorWidget *editor : m_editor)
            editor->setPlainText(msg);
    } else {
        showDiff();
    }
}

void SideBySideDiffEditorWidget::setCurrentDiffFileIndex(int diffFileIndex)
{
    if (m_controller.m_ignoreChanges.isLocked())
        return;

    const int blockNumber = m_editor[LeftSide]->diffData().blockNumberForFileIndex(diffFileIndex);

    const GuardLocker locker(m_controller.m_ignoreChanges);
    m_controller.setCurrentDiffFileIndex(diffFileIndex);

    for (SideDiffEditorWidget *editor : m_editor) {
        QTextBlock block = editor->document()->findBlockByNumber(blockNumber);
        QTextCursor cursor = editor->textCursor();
        cursor.setPosition(block.position());
        editor->setTextCursor(cursor);
        editor->verticalScrollBar()->setValue(blockNumber);
    }
}

void SideBySideDiffEditorWidget::setHorizontalSync(bool sync)
{
    m_horizontalSync = sync;
    horizontalSliderChanged(RightSide);
}

void SideBySideDiffEditorWidget::saveState()
{
    for (SideDiffEditorWidget *editor : m_editor)
        editor->saveState();
}

void SideBySideDiffEditorWidget::restoreState()
{
    for (SideDiffEditorWidget *editor : m_editor)
        editor->restoreState();
}

void SideBySideDiffEditorWidget::showDiff()
{
    m_asyncTask.reset(new Async<SideBySideShowResults>());
    m_asyncTask->setFutureSynchronizer(ExtensionSystem::PluginManager::futureSynchronizer());
    m_controller.setBusyShowing(true);

    connect(m_asyncTask.get(), &AsyncBase::done, this, [this] {
        if (m_asyncTask->isCanceled() || !m_asyncTask->isResultAvailable()) {
            for (SideDiffEditorWidget *editor : m_editor)
                editor->clearAll(Tr::tr("Retrieving data failed."));
        } else {
            const SideBySideShowResults results = m_asyncTask->result();
            m_editor[LeftSide]->setDiffData(results[LeftSide].diffData);
            m_editor[RightSide]->setDiffData(results[RightSide].diffData);
            TextDocumentPtr leftDoc(results[LeftSide].textDocument);
            TextDocumentPtr rightDoc(results[RightSide].textDocument);
            {
                const GuardLocker locker(m_controller.m_ignoreChanges);
                // TextDocument was living in no thread, so it's safe to pull it
                leftDoc->moveToThread(thread());
                rightDoc->moveToThread(thread());
                m_editor[LeftSide]->setTextDocument(leftDoc);
                m_editor[RightSide]->setTextDocument(rightDoc);

                m_editor[LeftSide]->setReadOnly(true);
                m_editor[RightSide]->setReadOnly(true);
            }
            auto leftDocumentLayout = qobject_cast<TextDocumentLayout*>(
                        m_editor[LeftSide]->document()->documentLayout());
            auto rightDocumentLayout = qobject_cast<TextDocumentLayout*>(
                        m_editor[RightSide]->document()->documentLayout());
            if (leftDocumentLayout && rightDocumentLayout) {
                connect(leftDocumentLayout, &TextDocumentLayout::foldChanged,
                        m_editor[RightSide], &SideDiffEditorWidget::setFolded);
                connect(rightDocumentLayout, &TextDocumentLayout::foldChanged,
                        m_editor[LeftSide], &SideDiffEditorWidget::setFolded);
            }
            m_editor[LeftSide]->setSelections(results[LeftSide].selections);
            m_editor[RightSide]->setSelections(results[RightSide].selections);
            setCurrentDiffFileIndex(m_controller.currentDiffFileIndex());
        }
        m_asyncTask.release()->deleteLater();
        m_controller.setBusyShowing(false);
    });

    const DiffEditorInput input(&m_controller);

    auto getDocument = [input](QPromise<SideBySideShowResults> &promise) {
        const int firstPartMax = 20; // diffOutput is about 4 times quicker than filling document
        const int leftPartMax = 60;
        const int rightPartMax = 100;
        promise.setProgressRange(0, rightPartMax);
        promise.setProgressValue(0);
        const SideBySideDiffOutput output = diffOutput(promise, 0, firstPartMax, input);
        if (promise.isCanceled())
            return;

        const SideBySideShowResult leftResult{TextDocumentPtr(new TextDocument("DiffEditor.SideDiffEditor")),
                    output.side[LeftSide].diffData, output.side[LeftSide].selections};
        const SideBySideShowResult rightResult{TextDocumentPtr(new TextDocument("DiffEditor.SideDiffEditor")),
                    output.side[RightSide].diffData, output.side[RightSide].selections};
        const SideBySideShowResults result{leftResult, rightResult};

        auto propagateDocument = [&output, &promise](DiffSide side, const SideBySideShowResult &result,
                                                int progressMin, int progressMax) {
            // No need to store the change history
            result.textDocument->document()->setUndoRedoEnabled(false);

            // We could do just:
            //   result.textDocument->setPlainText(output.diffText);
            // but this would freeze the thread for couple of seconds without progress reporting
            // and without checking for canceled.
            const int diffSize = output.side[side].diffText.size();
            const int packageSize = 10000;
            int currentPos = 0;
            QTextCursor cursor(result.textDocument->document());
            while (currentPos < diffSize) {
                const QString package = output.side[side].diffText.mid(currentPos, packageSize);
                cursor.insertText(package);
                currentPos += package.size();
                promise.setProgressValue(MathUtils::interpolateLinear(currentPos, 0, diffSize,
                                                                      progressMin, progressMax));
                if (promise.isCanceled())
                    return;
            }

            QTextBlock block = result.textDocument->document()->firstBlock();
            for (int b = 0; block.isValid(); block = block.next(), ++b)
                SelectableTextEditorWidget::setFoldingIndent(block, output.foldingIndent.value(b, 3));

            // If future was canceled, the destructor runs in this thread, so we can't move it
            // to caller's thread. We push it to no thread (make object to have no thread affinity),
            // and later, in the caller's thread, we pull it back to the caller's thread.
            result.textDocument->moveToThread(nullptr);
        };

        propagateDocument(LeftSide, leftResult, firstPartMax, leftPartMax);
        if (promise.isCanceled())
            return;
        propagateDocument(RightSide, rightResult, leftPartMax, rightPartMax);
        if (promise.isCanceled())
            return;

        promise.addResult(result);
    };

    m_asyncTask->setConcurrentCallData(getDocument);
    m_asyncTask->start();
    ProgressManager::addTask(m_asyncTask->future(), Tr::tr("Rendering diff"), "DiffEditor");
}

void SideBySideDiffEditorWidget::setFontSettings(const FontSettings &fontSettings)
{
    m_controller.setFontSettings(fontSettings);
}

void SideBySideDiffEditorWidget::jumpToOriginalFileRequested(DiffSide side, int diffFileIndex,
                                                             int lineNumber, int columnNumber)
{
    if (diffFileIndex < 0 || diffFileIndex >= m_controller.m_contextFileData.size())
        return;

    const FileData fileData = m_controller.m_contextFileData.at(diffFileIndex);
    const QString fileName = fileData.fileInfo[side].fileName;
    const DiffSide otherSide = oppositeSide(side);
    const QString otherFileName = fileData.fileInfo[otherSide].fileName;
    if (side == RightSide || fileName != otherFileName) {
        // different file (e.g. in Tools | Diff...)
        m_controller.jumpToOriginalFile(fileName, lineNumber, columnNumber);
        return;
    }

    // The same file (e.g. in git diff), jump to the line number taken from the right editor.
    // Warning: git show SHA^ vs SHA or git diff HEAD vs Index
    // (when Working tree has changed in meantime) will not work properly.
    for (const ChunkData &chunkData : fileData.chunks) {

        int thisLineNumber = chunkData.startingLineNumber[side];
        int otherLineNumber = chunkData.startingLineNumber[otherSide];

        for (int j = 0; j < chunkData.rows.size(); j++) {
            const RowData rowData = chunkData.rows.at(j);
            if (rowData.line[side].textLineType == TextLineData::TextLine)
                thisLineNumber++;
            if (rowData.line[otherSide].textLineType == TextLineData::TextLine)
                otherLineNumber++;
            if (thisLineNumber == lineNumber) {
                int colNr = rowData.equal ? columnNumber : 0;
                m_controller.jumpToOriginalFile(fileName, otherLineNumber, colNr);
                return;
            }
        }
    }
}

void SideBySideDiffEditorWidget::contextMenuRequested(DiffSide side, QMenu *menu, int fileIndex,
                                 int chunkIndex, const ChunkSelection &selection)
{
    menu->addSeparator();

    const PatchAction patchAction = side == LeftSide ? PatchAction::Apply : PatchAction::Revert;
    m_controller.addCodePasterAction(menu, fileIndex, chunkIndex);
    m_controller.addPatchAction(menu, fileIndex, chunkIndex, patchAction);
    m_controller.addExtraActions(menu, fileIndex, chunkIndex, selection);
}

void SideBySideDiffEditorWidget::verticalSliderChanged(DiffSide side)
{
    if (m_controller.m_ignoreChanges.isLocked())
        return;

    m_editor[oppositeSide(side)]->verticalScrollBar()->setValue(
                  m_editor[side]->verticalScrollBar()->value());
}

void SideBySideDiffEditorWidget::horizontalSliderChanged(DiffSide side)
{
    if (m_controller.m_ignoreChanges.isLocked() || !m_horizontalSync)
        return;

    m_editor[oppositeSide(side)]->horizontalScrollBar()->setValue(
                  m_editor[side]->horizontalScrollBar()->value());
}

void SideBySideDiffEditorWidget::cursorPositionChanged(DiffSide side)
{
    if (m_controller.m_ignoreChanges.isLocked())
        return;

    handlePositionChange(m_editor[side], m_editor[oppositeSide(side)]);
    verticalSliderChanged(side);
    horizontalSliderChanged(side);
}

void SideBySideDiffEditorWidget::syncHorizontalScrollBarPolicy()
{
    const bool alwaysOn = m_editor[LeftSide]->horizontalScrollBar()->maximum()
            || m_editor[RightSide]->horizontalScrollBar()->maximum();
    const Qt::ScrollBarPolicy newPolicy = alwaysOn ? Qt::ScrollBarAlwaysOn : Qt::ScrollBarAsNeeded;
    for (SideDiffEditorWidget *editor : m_editor) {
        if (editor->horizontalScrollBarPolicy() != newPolicy)
            editor->setHorizontalScrollBarPolicy(newPolicy);
    }
}

void SideBySideDiffEditorWidget::handlePositionChange(SideDiffEditorWidget *source, SideDiffEditorWidget *dest)
{
    if (m_controller.m_ignoreChanges.isLocked())
        return;

    const int fileIndex = source->diffData().fileIndexForBlockNumber(source->textCursor().blockNumber());
    if (fileIndex < 0)
        return;

    const GuardLocker locker(m_controller.m_ignoreChanges);
    syncCursor(source, dest);
    m_controller.setCurrentDiffFileIndex(fileIndex);
    emit currentDiffFileIndexChanged(fileIndex);
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

} // namespace DiffEditor::Internal

#include "sidebysidediffeditorwidget.moc"
