/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "sidebysidediffeditorwidget.h"
#include "selectabletexteditorwidget.h"
#include "diffeditordocument.h"
#include "diffutils.h"
#include "diffeditorconstants.h"

#include <coreplugin/editormanager/editormanager.h>

#include <QTextBlock>
#include <QTextCodec>
#include <QScrollBar>
#include <QPainter>
#include <QDir>
#include <QMessageBox>
#include <QMenu>
#include <QVBoxLayout>

#include <texteditor/texteditor.h>
#include <texteditor/textdocumentlayout.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/displaysettings.h>
#include <texteditor/highlighterutils.h>

#include <coreplugin/minisplitter.h>
#include <coreplugin/patchtool.h>

#include <cpaster/codepasterservice.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/qtcassert.h>
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
    SideDiffEditorWidget(QWidget *parent = 0);

    // block number, file info
    QMap<int, DiffFileInfo> fileInfo() const { return m_fileInfo; }

    void setLineNumber(int blockNumber, int lineNumber);
    void setFileInfo(int blockNumber, const DiffFileInfo &fileInfo);
    void setSkippedLines(int blockNumber, int skippedLines) {
        m_skippedLines[blockNumber] = skippedLines;
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
    QTextBlock firstVisibleBlock() const {
        return TextEditorWidget::firstVisibleBlock();
    }
    void saveState();
    void restoreState();

public slots:
    void setDisplaySettings(const DisplaySettings &ds) override;

signals:
    void jumpToOriginalFileRequested(int diffFileIndex,
                                     int lineNumber,
                                     int columnNumber);
    void contextMenuRequested(QMenu *menu,
                              int diffFileIndex,
                              int chunkIndex);

protected:
    int extraAreaWidth(int *markWidthPtr = 0) const override {
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
    void contextMenuEvent(QContextMenuEvent *e) override;
    void paintEvent(QPaintEvent *e) override;
    void scrollContentsBy(int dx, int dy) override;

private:
    void paintSeparator(QPainter &painter, QColor &color, const QString &text,
                        const QTextBlock &block, int top);
    void jumpToOriginalFile(const QTextCursor &cursor);

    // block number, visual line number.
    QMap<int, int> m_lineNumbers;
    int m_lineNumberDigits;
    // block number, fileInfo. Set for file lines only.
    QMap<int, DiffFileInfo> m_fileInfo;
    // block number, skipped lines. Set for chunk lines only.
    QMap<int, int> m_skippedLines;
    // start block number, block count of a chunk, chunk index inside a file.
    QMap<int, QPair<int, int> > m_chunkInfo;
    // block number, separator. Set for file, chunk or span line.
    QMap<int, bool> m_separators;
    bool m_inPaintEvent;
    QColor m_fileLineForeground;
    QColor m_chunkLineForeground;
    QColor m_textForeground;
    QByteArray m_state;
};

SideDiffEditorWidget::SideDiffEditorWidget(QWidget *parent)
    : SelectableTextEditorWidget("DiffEditor.SideDiffEditor", parent),
      m_lineNumberDigits(1),
      m_inPaintEvent(false)
{
    DisplaySettings settings = displaySettings();
    settings.m_textWrapping = false;
    settings.m_displayLineNumbers = true;
    settings.m_highlightCurrentLine = false;
    settings.m_displayFoldingMarkers = true;
    settings.m_markTextChanges = false;
    settings.m_highlightBlocks = false;
    SelectableTextEditorWidget::setDisplaySettings(settings);

    connect(this, &TextEditorWidget::tooltipRequested, [this](const QPoint &point, int position) {
        int block = document()->findBlock(position).blockNumber();
        auto it = m_fileInfo.constFind(block);
        if (it != m_fileInfo.constEnd())
            ToolTip::show(point, it.value().fileName, this);
        else
            ToolTip::hide();
    });
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

void SideDiffEditorWidget::setDisplaySettings(const DisplaySettings &ds)
{
    DisplaySettings settings = displaySettings();
    settings.m_visualizeWhitespace = ds.m_visualizeWhitespace;
    SelectableTextEditorWidget::setDisplaySettings(settings);
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
    if (m_lineNumbers.contains(blockNumber))
        return QString::number(m_lineNumbers.value(blockNumber));
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
           && TextDocumentLayout::isFolded(
                                            document()->findBlockByNumber(blockNumber)));
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

    QTextBlock startBlock = document()->findBlock(startPosition);
    QTextBlock endBlock = document()->findBlock(endPosition);
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
                    text += QLatin1Char('\n');
                if (block == endBlock)
                    text += block.text().left(endPosition - block.position());
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

    QMap<int, DiffFileInfo>::const_iterator it
            = m_fileInfo.constBegin();
    for (int i = 0; i < fileIndex; i++)
        ++it;

    return it.key();
}

int SideDiffEditorWidget::fileIndexForBlockNumber(int blockNumber) const
{
    QMap<int, DiffFileInfo>::const_iterator it = m_fileInfo.constBegin();
    QMap<int, DiffFileInfo>::const_iterator itEnd = m_fileInfo.constEnd();

    int i = -1;
    while (it != itEnd) {
        if (it.key() > blockNumber)
            break;
        ++it;
        ++i;
    }
    return i;
}

int SideDiffEditorWidget::chunkIndexForBlockNumber(int blockNumber) const
{
    if (m_chunkInfo.isEmpty())
        return -1;

    QMap<int, QPair<int, int> >::const_iterator it
            = m_chunkInfo.upperBound(blockNumber);
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
        foreground = palette().foreground().color();

    painter.setPen(foreground);

    const QString replacementText = QLatin1String(" {")
            + foldReplacementText(block)
            + QLatin1String("}; ");
    const int replacementTextWidth = fontMetrics().width(replacementText) + 24;
    int x = replacementTextWidth + offset.x();
    if (x < document()->documentMargin()
            || !TextDocumentLayout::isFolded(block)) {
        x = document()->documentMargin();
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
    const int columnNumber = cursor.positionInBlock();
    if (!m_lineNumbers.contains(blockNumber))
        return;

    const int lineNumber = m_lineNumbers.value(blockNumber);

    emit jumpToOriginalFileRequested(fileIndexForBlockNumber(blockNumber),
                                     lineNumber, columnNumber);
}

static QString skippedText(int skippedNumber)
{
    if (skippedNumber > 0)
        return SideBySideDiffEditorWidget::tr("Skipped %n lines...", 0, skippedNumber);
    if (skippedNumber == -2)
        return SideBySideDiffEditorWidget::tr("Binary files differ");
    return SideBySideDiffEditorWidget::tr("Skipped unknown number of lines...");
}

void SideDiffEditorWidget::paintEvent(QPaintEvent *e)
{
    m_inPaintEvent = true;
    SelectableTextEditorWidget::paintEvent(e);
    m_inPaintEvent = false;

    QPainter painter(viewport());
    QPointF offset = contentOffset();
    QTextBlock firstBlock = firstVisibleBlock();
    QTextBlock currentBlock = firstBlock;

    while (currentBlock.isValid()) {
        if (currentBlock.isVisible()) {
            qreal top = blockBoundingGeometry(currentBlock).translated(offset).top();
            qreal bottom = top + blockBoundingRect(currentBlock).height();

            if (top > e->rect().bottom())
                break;

            if (bottom >= e->rect().top()) {
                const int blockNumber = currentBlock.blockNumber();

                const int skippedBefore = m_skippedLines.value(blockNumber);
                if (skippedBefore) {
                    const QString skippedRowsText = skippedText(skippedBefore);
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
}

//////////////////

SideBySideDiffEditorWidget::SideBySideDiffEditorWidget(QWidget *parent)
    : QWidget(parent)
    , m_document(0)
    , m_ignoreCurrentIndexChange(false)
    , m_foldingBlocker(false)
    , m_horizontalSync(false)
    , m_contextMenuFileIndex(-1)
    , m_contextMenuChunkIndex(-1)
{
    m_leftEditor = new SideDiffEditorWidget(this);
    m_leftEditor->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_leftEditor->setReadOnly(true);
    connect(TextEditorSettings::instance(), &TextEditorSettings::displaySettingsChanged,
            m_leftEditor, &SideDiffEditorWidget::setDisplaySettings);
    m_leftEditor->setDisplaySettings(TextEditorSettings::displaySettings());
    m_leftEditor->setCodeStyle(TextEditorSettings::codeStyle());
    connect(m_leftEditor, &SideDiffEditorWidget::jumpToOriginalFileRequested,
            this, &SideBySideDiffEditorWidget::slotLeftJumpToOriginalFileRequested);
    connect(m_leftEditor, &SideDiffEditorWidget::contextMenuRequested,
            this, &SideBySideDiffEditorWidget::slotLeftContextMenuRequested,
            Qt::DirectConnection);

    m_rightEditor = new SideDiffEditorWidget(this);
    m_rightEditor->setReadOnly(true);
    connect(TextEditorSettings::instance(), &TextEditorSettings::displaySettingsChanged,
            m_rightEditor, &SideDiffEditorWidget::setDisplaySettings);
    m_rightEditor->setDisplaySettings(TextEditorSettings::displaySettings());
    m_rightEditor->setCodeStyle(TextEditorSettings::codeStyle());
    connect(m_rightEditor, &SideDiffEditorWidget::jumpToOriginalFileRequested,
            this, &SideBySideDiffEditorWidget::slotRightJumpToOriginalFileRequested);
    connect(m_rightEditor, &SideDiffEditorWidget::contextMenuRequested,
            this, &SideBySideDiffEditorWidget::slotRightContextMenuRequested,
            Qt::DirectConnection);

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
//    connect(m_leftEditor->document()->documentLayout(), SIGNAL(documentSizeChanged(QSizeF)),
//            this, SLOT(leftDocumentSizeChanged()));

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

    m_splitter = new MiniSplitter(this);
    m_splitter->addWidget(m_leftEditor);
    m_splitter->addWidget(m_rightEditor);
    QVBoxLayout *l = new QVBoxLayout(this);
    l->setMargin(0);
    l->addWidget(m_splitter);
    setFocusProxy(m_rightEditor);
}

void SideBySideDiffEditorWidget::setDocument(DiffEditorDocument *document)
{
    m_document = document;
}

void SideBySideDiffEditorWidget::clear(const QString &message)
{
    const bool oldIgnore = m_ignoreCurrentIndexChange;
    m_ignoreCurrentIndexChange = true;
    setDiff(QList<FileData>(), QString());
    m_leftEditor->clearAll(message);
    m_rightEditor->clearAll(message);
    m_ignoreCurrentIndexChange = oldIgnore;
}

void SideBySideDiffEditorWidget::setDiff(const QList<FileData> &diffFileList,
                                         const QString &workingDirectory)
{
    Q_UNUSED(workingDirectory)

    const bool oldIgnore = m_ignoreCurrentIndexChange;
    m_ignoreCurrentIndexChange = true;
    m_leftEditor->clear();
    m_rightEditor->clear();

    m_contextFileData = diffFileList;
    if (m_contextFileData.isEmpty()) {
        const QString msg = tr("No difference");
        m_leftEditor->setPlainText(msg);
        m_rightEditor->setPlainText(msg);
    } else {
        showDiff();
    }
    m_ignoreCurrentIndexChange = oldIgnore;
}

void SideBySideDiffEditorWidget::setCurrentDiffFileIndex(int diffFileIndex)
{
    if (m_ignoreCurrentIndexChange)
        return;

    const int blockNumber = m_leftEditor->blockNumberForFileIndex(diffFileIndex);

    const bool oldIgnore = m_ignoreCurrentIndexChange;
    m_ignoreCurrentIndexChange = true;
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

    m_ignoreCurrentIndexChange = oldIgnore;
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
    QChar separator = QLatin1Char('\n');
    for (int i = 0; i < m_contextFileData.count(); i++) {
        QString leftText, rightText;
        const FileData &contextFileData = m_contextFileData.at(i);

        leftFormats[blockNumber].append(DiffSelection(&m_fileLineFormat));
        rightFormats[blockNumber].append(DiffSelection(&m_fileLineFormat));
        m_leftEditor->setFileInfo(blockNumber, contextFileData.leftFileInfo);
        m_rightEditor->setFileInfo(blockNumber, contextFileData.rightFileInfo);
        leftText = separator;
        rightText = separator;
        blockNumber++;

        int lastLeftLineNumber = -1;

        if (contextFileData.binaryFiles) {
            leftFormats[blockNumber].append(DiffSelection(&m_chunkLineFormat));
            rightFormats[blockNumber].append(DiffSelection(&m_chunkLineFormat));
            m_leftEditor->setSkippedLines(blockNumber, -2);
            m_rightEditor->setSkippedLines(blockNumber, -2);
            leftText += separator;
            rightText += separator;
            blockNumber++;
        } else {
            for (int j = 0; j < contextFileData.chunks.count(); j++) {
                ChunkData chunkData = contextFileData.chunks.at(j);

                int leftLineNumber = chunkData.leftStartingLineNumber;
                int rightLineNumber = chunkData.rightStartingLineNumber;

                if (!chunkData.contextChunk) {
                    const int skippedLines = leftLineNumber - lastLeftLineNumber - 1;
                    if (skippedLines > 0) {
                        leftFormats[blockNumber].append(DiffSelection(&m_chunkLineFormat));
                        rightFormats[blockNumber].append(DiffSelection(&m_chunkLineFormat));
                        m_leftEditor->setSkippedLines(blockNumber, skippedLines);
                        m_rightEditor->setSkippedLines(blockNumber, skippedLines);
                        leftText += separator;
                        rightText += separator;
                        blockNumber++;
                    }

                    m_leftEditor->setChunkIndex(blockNumber, chunkData.rows.count(), j);
                    m_rightEditor->setChunkIndex(blockNumber, chunkData.rows.count(), j);

                    for (int k = 0; k < chunkData.rows.count(); k++) {
                        RowData rowData = chunkData.rows.at(k);
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
                                leftFormats[blockNumber].append(DiffSelection(&m_leftLineFormat));
                            else
                                leftFormats[blockNumber].append(DiffSelection(&m_spanLineFormat));
                            if (rowData.rightLine.textLineType == TextLineData::TextLine)
                                rightFormats[blockNumber].append(DiffSelection(&m_rightLineFormat));
                            else
                                rightFormats[blockNumber].append(DiffSelection(&m_spanLineFormat));
                        }

                        QMapIterator<int, int> itLeft(leftLineData.changedPositions);
                        while (itLeft.hasNext()) {
                            itLeft.next();
                            leftFormats[blockNumber].append(
                                        DiffSelection(itLeft.key(), itLeft.value(),
                                                      &m_leftCharFormat));
                        }

                        QMapIterator<int, int> itRight(rightLineData.changedPositions);
                        while (itRight.hasNext()) {
                            itRight.next();
                            rightFormats[blockNumber].append(
                                        DiffSelection(itRight.key(), itRight.value(),
                                                      &m_rightCharFormat));
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
                        leftFormats[blockNumber].append(DiffSelection(&m_chunkLineFormat));
                        rightFormats[blockNumber].append(DiffSelection(&m_chunkLineFormat));
                        m_leftEditor->setSkippedLines(blockNumber, skippedLines);
                        m_rightEditor->setSkippedLines(blockNumber, skippedLines);
                        leftText += separator;
                        rightText += separator;
                        blockNumber++;
                    } // otherwise nothing skipped
                }
            }
        }
        leftText.replace(QLatin1Char('\r'), QLatin1Char(' '));
        rightText.replace(QLatin1Char('\r'), QLatin1Char(' '));
        leftTexts += leftText;
        rightTexts += rightText;
    }

    if (leftTexts.isEmpty() && rightTexts.isEmpty())
        return;

    const bool oldIgnore = m_ignoreCurrentIndexChange;
    m_ignoreCurrentIndexChange = true;
    m_leftEditor->clear();
    m_leftEditor->setPlainText(leftTexts);
    m_rightEditor->clear();
    m_rightEditor->setPlainText(rightTexts);
    m_ignoreCurrentIndexChange = oldIgnore;

    m_leftEditor->setSelections(leftFormats);
    m_rightEditor->setSelections(rightFormats);
}

void SideBySideDiffEditorWidget::setFontSettings(
        const FontSettings &fontSettings)
{
    m_spanLineFormat  = fontSettings.toTextCharFormat(C_LINE_NUMBER);
    m_fileLineFormat  = fontSettings.toTextCharFormat(C_DIFF_FILE_LINE);
    m_chunkLineFormat = fontSettings.toTextCharFormat(C_DIFF_CONTEXT_LINE);
    m_leftLineFormat  = fontSettings.toTextCharFormat(C_DIFF_SOURCE_LINE);
    m_leftCharFormat  = fontSettings.toTextCharFormat(C_DIFF_SOURCE_CHAR);
    m_rightLineFormat = fontSettings.toTextCharFormat(C_DIFF_DEST_LINE);
    m_rightCharFormat = fontSettings.toTextCharFormat(C_DIFF_DEST_CHAR);
}

void SideBySideDiffEditorWidget::slotLeftJumpToOriginalFileRequested(
        int diffFileIndex,
        int lineNumber,
        int columnNumber)
{
    if (diffFileIndex < 0 || diffFileIndex >= m_contextFileData.count())
        return;

    const FileData fileData = m_contextFileData.at(diffFileIndex);
    const QString leftFileName = fileData.leftFileInfo.fileName;
    const QString rightFileName = fileData.rightFileInfo.fileName;
    if (leftFileName == rightFileName) {
        // The same file (e.g. in git diff), jump to the line number taken from the right editor.
        // Warning: git show SHA^ vs SHA or git diff HEAD vs Index
        // (when Working tree has changed in meantime) will not work properly.
        for (int i = 0; i < fileData.chunks.count(); i++) {
            const ChunkData chunkData = fileData.chunks.at(i);

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
                    jumpToOriginalFile(leftFileName, rightLineNumber, colNr);
                    return;
                }
            }
        }
    } else {
        // different file (e.g. in Tools | Diff...)
        jumpToOriginalFile(leftFileName, lineNumber, columnNumber);
    }
}

void SideBySideDiffEditorWidget::slotRightJumpToOriginalFileRequested(
        int diffFileIndex,
        int lineNumber,
        int columnNumber)
{
    if (diffFileIndex < 0 || diffFileIndex >= m_contextFileData.count())
        return;

    const FileData fileData = m_contextFileData.at(diffFileIndex);
    const QString fileName = fileData.rightFileInfo.fileName;
    jumpToOriginalFile(fileName, lineNumber, columnNumber);
}

void SideBySideDiffEditorWidget::jumpToOriginalFile(const QString &fileName,
                                                    int lineNumber, int columnNumber)
{
    if (!m_document)
        return;

    const QDir dir(m_document->baseDirectory());
    const QString absoluteFileName = dir.absoluteFilePath(fileName);
    QFileInfo fi(absoluteFileName);
    if (fi.exists() && !fi.isDir())
        EditorManager::openEditorAt(absoluteFileName, lineNumber, columnNumber);
}

void SideBySideDiffEditorWidget::slotLeftContextMenuRequested(QMenu *menu,
                                                              int diffFileIndex,
                                                              int chunkIndex)
{
    menu->addSeparator();
    if (ExtensionSystem::PluginManager::getObject<CodePaster::Service>()) {
        // optional code pasting service
        QAction *sendChunkToCodePasterAction =
                menu->addAction(tr("Send Chunk to CodePaster..."));
        connect(sendChunkToCodePasterAction, &QAction::triggered,
                this, &SideBySideDiffEditorWidget::slotSendChunkToCodePaster);
        menu->addSeparator();
    }
    QAction *applyAction = menu->addAction(tr("Apply Chunk..."));
    connect(applyAction, &QAction::triggered, this, &SideBySideDiffEditorWidget::slotApplyChunk);
    applyAction->setEnabled(false);

    m_contextMenuFileIndex = diffFileIndex;
    m_contextMenuChunkIndex = chunkIndex;

    if (m_contextMenuFileIndex < 0 || m_contextMenuChunkIndex < 0)
        return;

    if (m_contextMenuFileIndex >= m_contextFileData.count())
        return;

    const FileData fileData = m_contextFileData.at(m_contextMenuFileIndex);
    if (m_contextMenuChunkIndex >= fileData.chunks.count())
        return;

    m_document->chunkActionsRequested(menu, diffFileIndex, chunkIndex);

    if (fileData.leftFileInfo.fileName == fileData.rightFileInfo.fileName)
        return;

    applyAction->setEnabled(true);
}

void SideBySideDiffEditorWidget::slotRightContextMenuRequested(QMenu *menu,
                                                               int diffFileIndex,
                                                               int chunkIndex)
{
    menu->addSeparator();
    if (ExtensionSystem::PluginManager::getObject<CodePaster::Service>()) {
        // optional code pasting service
        QAction *sendChunkToCodePasterAction =
                menu->addAction(tr("Send Chunk to CodePaster..."));
        connect(sendChunkToCodePasterAction, &QAction::triggered,
                this, &SideBySideDiffEditorWidget::slotSendChunkToCodePaster);
        menu->addSeparator();
    }
    QAction *revertAction = menu->addAction(tr("Revert Chunk..."));
    connect(revertAction, &QAction::triggered, this, &SideBySideDiffEditorWidget::slotRevertChunk);
    revertAction->setEnabled(false);

    m_contextMenuFileIndex = diffFileIndex;
    m_contextMenuChunkIndex = chunkIndex;

    if (m_contextMenuFileIndex < 0 || m_contextMenuChunkIndex < 0)
        return;

    if (m_contextMenuFileIndex >= m_contextFileData.count())
        return;

    const FileData fileData = m_contextFileData.at(m_contextMenuFileIndex);
    if (m_contextMenuChunkIndex >= fileData.chunks.count())
        return;

    m_document->chunkActionsRequested(menu, diffFileIndex, chunkIndex);

    revertAction->setEnabled(true);
}

void SideBySideDiffEditorWidget::slotSendChunkToCodePaster()
{
    if (!m_document)
        return;

    // Retrieve service by soft dependency.
    auto pasteService = ExtensionSystem::PluginManager::getObject<CodePaster::Service>();
    QTC_ASSERT(pasteService, return);

    const QString patch = m_document->makePatch(m_contextMenuFileIndex, m_contextMenuChunkIndex, false);
    if (patch.isEmpty())
        return;

    pasteService->postText(patch, QLatin1String(Constants::DIFF_EDITOR_MIMETYPE));
}

void SideBySideDiffEditorWidget::slotApplyChunk()
{
    patch(false);
}

void SideBySideDiffEditorWidget::slotRevertChunk()
{
    patch(true);
}

void SideBySideDiffEditorWidget::patch(bool revert)
{
    if (!m_document)
        return;

    const FileData fileData = m_contextFileData.at(m_contextMenuFileIndex);

    const QString title = revert ? tr("Revert Chunk") : tr("Apply Chunk");
    const QString question = revert
            ? tr("Would you like to revert the chunk?")
            : tr("Would you like to apply the chunk?");
    if (QMessageBox::No == QMessageBox::question(this, title,
                                                 question,
                                                 QMessageBox::Yes
                                                 | QMessageBox::No)) {
        return;
    }

    const int strip = m_document->baseDirectory().isEmpty() ? -1 : 0;

    const QString fileName = revert
            ? fileData.rightFileInfo.fileName
            : fileData.leftFileInfo.fileName;

    const QString workingDirectory = m_document->baseDirectory().isEmpty()
            ? QFileInfo(fileName).absolutePath()
            : m_document->baseDirectory();

    const QString patch = m_document->makePatch(m_contextMenuFileIndex, m_contextMenuChunkIndex, revert);

    if (patch.isEmpty())
        return;

    if (PatchTool::runPatch(EditorManager::defaultTextCodec()->fromUnicode(patch),
                            workingDirectory, strip, revert))
        m_document->reload();
}

void SideBySideDiffEditorWidget::leftVSliderChanged()
{
    m_rightEditor->verticalScrollBar()->setValue(m_leftEditor->verticalScrollBar()->value());
}

void SideBySideDiffEditorWidget::rightVSliderChanged()
{
    m_leftEditor->verticalScrollBar()->setValue(m_rightEditor->verticalScrollBar()->value());
}

void SideBySideDiffEditorWidget::leftHSliderChanged()
{
    if (m_horizontalSync)
        m_rightEditor->horizontalScrollBar()->setValue(m_leftEditor->horizontalScrollBar()->value());
}

void SideBySideDiffEditorWidget::rightHSliderChanged()
{
    if (m_horizontalSync)
        m_leftEditor->horizontalScrollBar()->setValue(m_rightEditor->horizontalScrollBar()->value());
}

void SideBySideDiffEditorWidget::leftCursorPositionChanged()
{
    leftVSliderChanged();
    leftHSliderChanged();

    if (m_ignoreCurrentIndexChange)
        return;

    const bool oldIgnore = m_ignoreCurrentIndexChange;
    m_ignoreCurrentIndexChange = true;
    emit currentDiffFileIndexChanged(
                m_leftEditor->fileIndexForBlockNumber(m_leftEditor->textCursor().blockNumber()));
    m_ignoreCurrentIndexChange = oldIgnore;
}

void SideBySideDiffEditorWidget::rightCursorPositionChanged()
{
    rightVSliderChanged();
    rightHSliderChanged();

    if (m_ignoreCurrentIndexChange)
        return;

    const bool oldIgnore = m_ignoreCurrentIndexChange;
    m_ignoreCurrentIndexChange = true;
    emit currentDiffFileIndexChanged(
                m_rightEditor->fileIndexForBlockNumber(m_rightEditor->textCursor().blockNumber()));
    m_ignoreCurrentIndexChange = oldIgnore;
}

} // namespace Internal
} // namespace DiffEditor

#include "sidebysidediffeditorwidget.moc"
