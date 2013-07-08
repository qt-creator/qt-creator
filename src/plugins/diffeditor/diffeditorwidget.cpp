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

#include "diffeditorwidget.h"
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QPlainTextDocumentLayout>
#include <QTextBlock>
#include <QScrollBar>
#include <QPainter>
#include <QDir>
#include <QToolButton>

#include <texteditor/basetexteditor.h>
#include <texteditor/snippets/snippeteditor.h>
#include <texteditor/basetextdocumentlayout.h>
#include <texteditor/syntaxhighlighter.h>
#include <texteditor/basetextdocument.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/displaysettings.h>

#include <coreplugin/minisplitter.h>

#include <utils/tooltip/tipcontents.h>
#include <utils/tooltip/tooltip.h>

static const int BASE_LEVEL = 0;
static const int FILE_LEVEL = 1;
static const int CHUNK_LEVEL = 2;

using namespace TextEditor;

namespace DiffEditor {

struct TextLineData {
    enum TextLineType {
        TextLine,
        Separator,
        Invalid
    };
    TextLineData() : textLineType(Invalid) {}
    TextLineData(const QString &txt) : textLineType(TextLine), text(txt) {}
    TextLineData(TextLineType t) : textLineType(t) {}
    TextLineType textLineType;
    QString text;
};

struct RowData {
    RowData() : equal(true) {}
    RowData(const TextLineData &l)
        : leftLine(l), rightLine(l), equal(true) {}
    RowData(const TextLineData &l, const TextLineData &r, bool e = false)
        : leftLine(l), rightLine(r), equal(e) {}
    TextLineData leftLine;
    TextLineData rightLine;
    bool equal; // true if left and right lines are equal, taking whitespaces into account (or both invalid)
};

struct ChunkData {
    ChunkData() : contextChunk(false) {}
    QList<RowData> rows;
    bool contextChunk;
    QMap<int, int> changedLeftPositions; // counting from the beginning of the chunk
    QMap<int, int> changedRightPositions; // counting from the beginning of the chunk
};

struct FileData {
    FileData() {}
    FileData(const ChunkData &chunkData) { chunks.append(chunkData); }
    QList<ChunkData> chunks;
    DiffEditorWidget::DiffFileInfo leftFileInfo;
    DiffEditorWidget::DiffFileInfo rightFileInfo;
};

//////////////////////

class DiffViewEditorEditable : public BaseTextEditor
{
Q_OBJECT
public:
    DiffViewEditorEditable(BaseTextEditorWidget *editorWidget)
        : BaseTextEditor(editorWidget)
    {
        connect(this, SIGNAL(tooltipRequested(TextEditor::ITextEditor*,QPoint,int)),
                this, SLOT(slotTooltipRequested(TextEditor::ITextEditor*,QPoint, int)));
    }

    Core::Id id() const { return "DiffViewEditor"; }
    bool isTemporary() const { return false; }

private slots:
    void slotTooltipRequested(TextEditor::ITextEditor *editor, const QPoint &globalPoint, int position);

};


////////////////////////

class DiffViewEditorWidget : public SnippetEditorWidget
{
    Q_OBJECT
public:
    DiffViewEditorWidget(QWidget *parent = 0);

    void setSyntaxHighlighter(SyntaxHighlighter *sh) {
        baseTextDocument()->setSyntaxHighlighter(sh);
    }
    QTextCodec *codec() const {
        return const_cast<QTextCodec *>(baseTextDocument()->codec());
    }

    QMap<int, int> skippedLines() const { return m_skippedLines; }
    QMap<int, DiffEditorWidget::DiffFileInfo> fileInfo() const { return m_fileInfo; }

    void setLineNumber(int blockNumber, int lineNumber);
    void setFileInfo(int blockNumber, const DiffEditorWidget::DiffFileInfo &fileInfo) { m_fileInfo[blockNumber] = fileInfo; setSeparator(blockNumber, true); }
    void setSkippedLines(int blockNumber, int skippedLines) { m_skippedLines[blockNumber] = skippedLines; setSeparator(blockNumber, true); }
    void setSeparator(int blockNumber, bool separator) { m_separators[blockNumber] = separator; }
    bool isFileLine(int blockNumber) const { return m_fileInfo.contains(blockNumber); }
    int blockNumberForFileIndex(int fileIndex) const;
    int fileIndexForBlockNumber(int blockNumber) const;
    bool isChunkLine(int blockNumber) const { return m_skippedLines.contains(blockNumber); }
    void clearAll();
    void clearAll(const QString &message);
    void clearAllData();
    QTextBlock firstVisibleBlock() const { return SnippetEditorWidget::firstVisibleBlock(); }

public slots:
    void setDisplaySettings(const DisplaySettings &ds);
    void setFontSettings(const TextEditor::FontSettings &fs);

signals:
    void jumpToOriginalFileRequested(int diffFileIndex,
                                     int lineNumber,
                                     int columnNumber);

protected:
    virtual int extraAreaWidth(int *markWidthPtr = 0) const { return BaseTextEditorWidget::extraAreaWidth(markWidthPtr); }
    BaseTextEditor *createEditor() { return new DiffViewEditorEditable(this); }
    virtual QString lineNumber(int blockNumber) const;
    virtual int lineNumberDigits() const;
    virtual bool selectionVisible(int blockNumber) const;
    virtual bool replacementVisible(int blockNumber) const;
    QColor replacementPenColor(int blockNumber) const;
    virtual QString plainTextFromSelection(const QTextCursor &cursor) const;
    virtual void drawCollapsedBlockPopup(QPainter &painter,
                                 const QTextBlock &block,
                                 QPointF offset,
                                 const QRect &clip);
    void mouseDoubleClickEvent(QMouseEvent *e);
    virtual void paintEvent(QPaintEvent *e);
    virtual void scrollContentsBy(int dx, int dy);

private:
    void paintCollapsedBlockPopup(QPainter &painter, const QRect &clipRect);
    void paintSeparator(QPainter &painter, QColor &color, const QString &text,
                        const QTextBlock &block, int top);
    void jumpToOriginalFile(const QTextCursor &cursor);

    QMap<int, int> m_lineNumbers;
    int m_lineNumberDigits;
    // block number, fileInfo
    QMap<int, DiffEditorWidget::DiffFileInfo> m_fileInfo;
    // block number, skipped lines
    QMap<int, int> m_skippedLines;
    // block number, separator. Separator used as lines alignment and inside skipped lines
    QMap<int, bool> m_separators;
    bool m_inPaintEvent;
    QColor m_fileLineForeground;
    QColor m_chunkLineForeground;
    QColor m_textForeground;
};

void DiffViewEditorEditable::slotTooltipRequested(TextEditor::ITextEditor *editor, const QPoint &globalPoint, int position)
{
    DiffViewEditorWidget *ew = qobject_cast<DiffViewEditorWidget *>(editorWidget());
    if (!ew)
        return;

    QMap<int, DiffEditorWidget::DiffFileInfo> fi = ew->fileInfo();
    QMap<int, DiffEditorWidget::DiffFileInfo>::const_iterator it
            = fi.constFind(ew->document()->findBlock(position).blockNumber());
    if (it != fi.constEnd()) {
        Utils::ToolTip::instance()->show(globalPoint,
                                         Utils::TextContent(it.value().fileName),
                                         editor->widget());
    } else {
        Utils::ToolTip::instance()->hide();
    }
}

DiffViewEditorWidget::DiffViewEditorWidget(QWidget *parent)
    : SnippetEditorWidget(parent), m_lineNumberDigits(1), m_inPaintEvent(false)
{
    DisplaySettings settings = displaySettings();
    settings.m_textWrapping = false;
    settings.m_displayLineNumbers = true;
    settings.m_highlightCurrentLine = false;
    settings.m_displayFoldingMarkers = true;
    settings.m_markTextChanges = false;
    settings.m_highlightBlocks = false;
    SnippetEditorWidget::setDisplaySettings(settings);

    setCodeFoldingSupported(true);
    setFrameStyle(QFrame::NoFrame);
}

void DiffViewEditorWidget::setDisplaySettings(const DisplaySettings &ds)
{
    DisplaySettings settings = displaySettings();
    settings.m_visualizeWhitespace = ds.m_visualizeWhitespace;
    SnippetEditorWidget::setDisplaySettings(settings);
}

void DiffViewEditorWidget::setFontSettings(const TextEditor::FontSettings &fs)
{
    SnippetEditorWidget::setFontSettings(fs);
    m_fileLineForeground = fs.formatFor(C_DIFF_FILE_LINE).foreground();
    m_chunkLineForeground = fs.formatFor(C_DIFF_CONTEXT_LINE).foreground();
    m_textForeground = fs.toTextCharFormat(C_TEXT).foreground().color();
    update();
}

QString DiffViewEditorWidget::lineNumber(int blockNumber) const
{
    if (m_lineNumbers.contains(blockNumber))
        return QString::number(m_lineNumbers.value(blockNumber));
    return QString();
}

int DiffViewEditorWidget::lineNumberDigits() const
{
    return m_lineNumberDigits;
}

bool DiffViewEditorWidget::selectionVisible(int blockNumber) const
{
    return !m_separators.value(blockNumber, false);
}

bool DiffViewEditorWidget::replacementVisible(int blockNumber) const
{
    return isChunkLine(blockNumber) || (isFileLine(blockNumber)
           && TextEditor::BaseTextDocumentLayout::isFolded(document()->findBlockByNumber(blockNumber)));
}

QColor DiffViewEditorWidget::replacementPenColor(int blockNumber) const
{
    Q_UNUSED(blockNumber)
    return m_chunkLineForeground;
}

QString DiffViewEditorWidget::plainTextFromSelection(const QTextCursor &cursor) const
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

void DiffViewEditorWidget::setLineNumber(int blockNumber, int lineNumber)
{
    const QString lineNumberString = QString::number(lineNumber);
    m_lineNumbers.insert(blockNumber, lineNumber);
    m_lineNumberDigits = qMax(m_lineNumberDigits, lineNumberString.count());
}

int DiffViewEditorWidget::blockNumberForFileIndex(int fileIndex) const
{
    if (fileIndex < 0 || fileIndex >= m_fileInfo.count())
        return -1;

    QMap<int, DiffEditorWidget::DiffFileInfo>::const_iterator it
            = m_fileInfo.constBegin();
    for (int i = 0; i < fileIndex; i++)
        ++it;

    return it.key();
}

int DiffViewEditorWidget::fileIndexForBlockNumber(int blockNumber) const
{
    QMap<int, DiffEditorWidget::DiffFileInfo>::const_iterator it
            = m_fileInfo.constBegin();
    QMap<int, DiffEditorWidget::DiffFileInfo>::const_iterator itEnd
            = m_fileInfo.constEnd();

    int i = -1;
    while (it != itEnd) {
        if (it.key() > blockNumber)
            break;
        ++it;
        ++i;
    }
    return i;
}

void DiffViewEditorWidget::clearAll()
{
    clearAll(tr("No difference"));
}

void DiffViewEditorWidget::clearAll(const QString &message)
{
    setBlockSelection(false);
    clear();
    clearAllData();
    setPlainText(message);
}

void DiffViewEditorWidget::clearAllData()
{
    m_lineNumberDigits = 1;
    m_lineNumbers.clear();
    m_fileInfo.clear();
    m_skippedLines.clear();
    m_separators.clear();
}

void DiffViewEditorWidget::scrollContentsBy(int dx, int dy)
{
    SnippetEditorWidget::scrollContentsBy(dx, dy);
    // TODO: update only chunk lines
    viewport()->update();
}

void DiffViewEditorWidget::paintSeparator(QPainter &painter,
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
    if (x < document()->documentMargin() || !TextEditor::BaseTextDocumentLayout::isFolded(block))
        x = document()->documentMargin();
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

void DiffViewEditorWidget::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton && !(e->modifiers() & Qt::ShiftModifier)) {
        QTextCursor cursor = cursorForPosition(e->pos());
        jumpToOriginalFile(cursor);
        e->accept();
        return;
    }
    SnippetEditorWidget::mouseDoubleClickEvent(e);
}

void DiffViewEditorWidget::jumpToOriginalFile(const QTextCursor &cursor)
{
    if (m_fileInfo.isEmpty())
        return;

    const int blockNumber = cursor.blockNumber();
    const int columnNumber = cursor.positionInBlock();
    if (!m_lineNumbers.contains(blockNumber))
        return;

    const int lineNumber = m_lineNumbers.value(blockNumber);

    emit jumpToOriginalFileRequested(fileIndexForBlockNumber(blockNumber), lineNumber, columnNumber);
}

void DiffViewEditorWidget::paintEvent(QPaintEvent *e)
{
    m_inPaintEvent = true;
    SnippetEditorWidget::paintEvent(e);
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
                    const QString skippedRowsText = tr("Skipped %n lines...", 0, skippedBefore);
                    paintSeparator(painter, m_chunkLineForeground,
                                   skippedRowsText, currentBlock, top);
                }

                const DiffEditorWidget::DiffFileInfo fileInfo = m_fileInfo.value(blockNumber);
                if (!fileInfo.fileName.isEmpty()) {
                    const QString fileNameText = fileInfo.typeInfo.isEmpty()
                            ? fileInfo.fileName
                            : tr("[%1] %2").arg(fileInfo.typeInfo).arg(fileInfo.fileName);
                    paintSeparator(painter, m_fileLineForeground,
                                   fileNameText, currentBlock, top);
                }
            }
        }
        currentBlock = currentBlock.next();
    }
    paintCollapsedBlockPopup(painter, e->rect());
}

void DiffViewEditorWidget::paintCollapsedBlockPopup(QPainter &painter, const QRect &clipRect)
{
    QPointF offset(contentOffset());
    QRect viewportRect = viewport()->rect();
    QTextBlock block = firstVisibleBlock();
    QTextBlock visibleCollapsedBlock;
    QPointF visibleCollapsedBlockOffset;

    while (block.isValid()) {

        QRectF r = blockBoundingRect(block).translated(offset);

        offset.ry() += r.height();

        if (offset.y() > viewportRect.height())
            break;

        block = block.next();

        if (!block.isVisible()) {
            if (block.blockNumber() == visibleFoldedBlockNumber()) {
                visibleCollapsedBlock = block;
                visibleCollapsedBlockOffset = offset + QPointF(0,1);
                break;
            }

            // invisible blocks do have zero line count
            block = document()->findBlockByLineNumber(block.firstLineNumber());
        }
    }
    if (visibleCollapsedBlock.isValid()) {
        drawCollapsedBlockPopup(painter,
                                visibleCollapsedBlock,
                                visibleCollapsedBlockOffset,
                                clipRect);
    }
}

void DiffViewEditorWidget::drawCollapsedBlockPopup(QPainter &painter,
                                             const QTextBlock &block,
                                             QPointF offset,
                                             const QRect &clip)
{
    // We ignore the call coming from the BaseTextEditorWidget::paintEvent()
    // since we will draw it later, after custom drawings of this paintEvent.
    // We need to draw it after our custom drawings, otherwise custom
    // drawings will appear in front of block popup.
    if (m_inPaintEvent)
        return;

    int margin = block.document()->documentMargin();
    qreal maxWidth = 0;
    qreal blockHeight = 0;
    QTextBlock b = block;

    while (!b.isVisible()) {
        if (!m_separators.contains(b.blockNumber())) {
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
    painter.setBrush(brush);
    painter.drawRoundedRect(QRectF(offset.x(),
                                   offset.y(),
                                   maxWidth, blockHeight).adjusted(0, 0, 0, 0), 3, 3);
    painter.restore();

    QTextBlock end = b;
    b = block;
    while (b != end) {
        if (!m_separators.contains(b.blockNumber())) {
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


//////////////////

DiffEditorWidget::DiffEditorWidget(QWidget *parent)
    : QWidget(parent),
      m_contextLinesNumber(3),
      m_ignoreWhitespaces(true),
      m_syncScrollBars(true),
      m_foldingBlocker(false)
{
    TextEditor::TextEditorSettings *settings = TextEditorSettings::instance();

    m_leftEditor = new DiffViewEditorWidget(this);
    m_leftEditor->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_leftEditor->setReadOnly(true);
    connect(settings, SIGNAL(displaySettingsChanged(TextEditor::DisplaySettings)),
            m_leftEditor, SLOT(setDisplaySettings(TextEditor::DisplaySettings)));
    m_leftEditor->setDisplaySettings(settings->displaySettings());
    m_leftEditor->setCodeStyle(settings->codeStyle());
    connect(m_leftEditor, SIGNAL(jumpToOriginalFileRequested(int,int,int)),
            this, SLOT(slotLeftJumpToOriginalFileRequested(int,int,int)));

    m_rightEditor = new DiffViewEditorWidget(this);
    m_rightEditor->setReadOnly(true);
    connect(settings, SIGNAL(displaySettingsChanged(TextEditor::DisplaySettings)),
            m_rightEditor, SLOT(setDisplaySettings(TextEditor::DisplaySettings)));
    m_rightEditor->setDisplaySettings(settings->displaySettings());
    m_rightEditor->setCodeStyle(settings->codeStyle());
    connect(m_rightEditor, SIGNAL(jumpToOriginalFileRequested(int,int,int)),
            this, SLOT(slotRightJumpToOriginalFileRequested(int,int,int)));

    connect(settings, SIGNAL(fontSettingsChanged(TextEditor::FontSettings)),
            this, SLOT(setFontSettings(TextEditor::FontSettings)));
    setFontSettings(settings->fontSettings());

    connect(m_leftEditor->verticalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(leftVSliderChanged()));
    connect(m_leftEditor->verticalScrollBar(), SIGNAL(actionTriggered(int)),
            this, SLOT(leftVSliderChanged()));

    connect(m_leftEditor->horizontalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(leftHSliderChanged()));
    connect(m_leftEditor->horizontalScrollBar(), SIGNAL(actionTriggered(int)),
            this, SLOT(leftHSliderChanged()));

    connect(m_leftEditor, SIGNAL(cursorPositionChanged()),
            this, SLOT(leftCursorPositionChanged()));
    connect(m_leftEditor->document()->documentLayout(), SIGNAL(documentSizeChanged(QSizeF)),
            this, SLOT(leftDocumentSizeChanged()));

    connect(m_rightEditor->verticalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(rightVSliderChanged()));
    connect(m_rightEditor->verticalScrollBar(), SIGNAL(actionTriggered(int)),
            this, SLOT(rightVSliderChanged()));

    connect(m_rightEditor->horizontalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(rightHSliderChanged()));
    connect(m_rightEditor->horizontalScrollBar(), SIGNAL(actionTriggered(int)),
            this, SLOT(rightHSliderChanged()));

    connect(m_rightEditor, SIGNAL(cursorPositionChanged()),
            this, SLOT(rightCursorPositionChanged()));
    connect(m_rightEditor->document()->documentLayout(), SIGNAL(documentSizeChanged(QSizeF)),
            this, SLOT(rightDocumentSizeChanged()));

    m_splitter = new Core::MiniSplitter(this);
    m_splitter->addWidget(m_leftEditor);
    m_splitter->addWidget(m_rightEditor);
    QVBoxLayout *l = new QVBoxLayout(this);
    l->setMargin(0);
    l->addWidget(m_splitter);

    clear();
}

DiffEditorWidget::~DiffEditorWidget()
{

}

void DiffEditorWidget::clear()
{
    m_leftEditor->clearAll();
    m_rightEditor->clearAll();
}

void DiffEditorWidget::clear(const QString &message)
{
    m_leftEditor->clearAll(message);
    m_rightEditor->clearAll(message);
}

void DiffEditorWidget::setDiff(const QList<DiffFilesContents> &diffFileList, const QString &workingDirectory)
{
    m_workingDirectory = workingDirectory;
    Differ differ;
    QList<DiffList> diffList;
    for (int i = 0; i < diffFileList.count(); i++) {
        DiffFilesContents dfc = diffFileList.at(i);
        DiffList dl;
        dl.leftFileInfo = dfc.leftFileInfo;
        dl.rightFileInfo = dfc.rightFileInfo;
        dl.diffList = differ.cleanupSemantics(differ.diff(dfc.leftText, dfc.rightText));
        diffList.append(dl);
    }
    setDiff(diffList);
}

void DiffEditorWidget::setDiff(const QList<DiffList> &diffList)
{
    m_diffList = diffList;
    m_originalChunkData.clear();
    m_contextFileData.clear();

    for (int i = 0; i < m_diffList.count(); i++) {
        const DiffList &dl = m_diffList.at(i);
        ChunkData chunkData = calculateOriginalData(dl.diffList);
        m_originalChunkData.append(chunkData);
        FileData fileData = calculateContextData(chunkData);
        fileData.leftFileInfo = dl.leftFileInfo;
        fileData.rightFileInfo = dl.rightFileInfo;
        m_contextFileData.append(fileData);
    }
    showDiff();
}

void DiffEditorWidget::setContextLinesNumber(int lines)
{
    if (m_contextLinesNumber == lines)
        return;

    m_contextLinesNumber = lines;
    for (int i = 0; i < m_diffList.count(); i++) {
        const FileData oldFileData = m_contextFileData.at(i);
        FileData newFileData = calculateContextData(m_originalChunkData.at(i));
        newFileData.leftFileInfo = oldFileData.leftFileInfo;
        newFileData.rightFileInfo = oldFileData.rightFileInfo;
        m_contextFileData[i] = newFileData;
    }

    showDiff();
}

void DiffEditorWidget::setIgnoreWhitespaces(bool ignore)
{
    if (m_ignoreWhitespaces == ignore)
        return;

    m_ignoreWhitespaces = ignore;
    setDiff(m_diffList);
}

void DiffEditorWidget::navigateToDiffFile(int diffFileIndex)
{
    const int blockNumber = m_leftEditor->blockNumberForFileIndex(diffFileIndex);

    QTextBlock leftBlock = m_leftEditor->document()->findBlockByNumber(blockNumber);
    QTextCursor leftCursor = m_leftEditor->textCursor();
    leftCursor.setPosition(leftBlock.position());
    m_leftEditor->setTextCursor(leftCursor);

    QTextBlock rightBlock = m_rightEditor->document()->findBlockByNumber(blockNumber);
    QTextCursor rightCursor = m_rightEditor->textCursor();
    rightCursor.setPosition(rightBlock.position());
    m_rightEditor->setTextCursor(rightCursor);

    m_leftEditor->centerCursor();
    m_rightEditor->centerCursor();
}

QTextCodec *DiffEditorWidget::codec() const
{
    return const_cast<QTextCodec *>(m_leftEditor->codec());
}

SnippetEditorWidget *DiffEditorWidget::leftEditor() const
{
    return m_leftEditor;
}

SnippetEditorWidget *DiffEditorWidget::rightEditor() const
{
    return m_rightEditor;
}

bool DiffEditorWidget::isWhitespace(const QChar &c) const
{
    if (c == QLatin1Char(' ') || c == QLatin1Char('\t'))
        return true;
    return false;
}

bool DiffEditorWidget::isWhitespace(const Diff &diff) const
{
    for (int i = 0; i < diff.text.count(); i++) {
        if (!isWhitespace(diff.text.at(i)))
            return false;
    }
    return true;
}

bool DiffEditorWidget::isEqual(const QList<Diff> &diffList, int diffNumber) const
{
    const Diff &diff = diffList.at(diffNumber);
    if (diff.command == Diff::Equal)
        return true;

    if (diff.text.count() == 0)
        return true;

    if (!m_ignoreWhitespaces)
        return false;

    if (isWhitespace(diff) == false)
        return false;

    if (diffNumber == 0 || diffNumber == diffList.count() - 1)
        return false; // it's a Diff start or end

    // Examine previous diff
    if (diffNumber > 0) {
        const Diff &previousDiff = diffList.at(diffNumber - 1);
        if (previousDiff.command == Diff::Equal) {
            const int previousDiffCount = previousDiff.text.count();
            if (previousDiffCount && isWhitespace(previousDiff.text.at(previousDiffCount - 1)))
                return true;
        } else if (diff.command != previousDiff.command
                   && isWhitespace(previousDiff)) {
            return true;
        }
    }

    // Examine next diff
    if (diffNumber < diffList.count() - 1) {
        const Diff &nextDiff = diffList.at(diffNumber + 1);
        if (nextDiff.command == Diff::Equal) {
            const int nextDiffCount = nextDiff.text.count();
            if (nextDiffCount && isWhitespace(nextDiff.text.at(0)))
                return true;
        } else if (diff.command != nextDiff.command
                   && isWhitespace(nextDiff)) {
            return true;
        }
    }

    return false;
}

QList<TextLineData> DiffEditorWidget::assemblyRows(const QStringList &lines,
                                                   const QMap<int, int> &lineSpans,
                                                   const QMap<int, int> &changedPositions,
                                                   QMap<int, int> *outputChangedPositions) const
{
    QList<TextLineData> data;

    int previousSpanOffset = 0;
    int spanOffset = 0;
    int pos = 0;
    bool usePreviousSpanOffsetForStartPosition = false;
    QMap<int, int>::ConstIterator changedIt = changedPositions.constBegin();
    QMap<int, int>::ConstIterator changedEnd = changedPositions.constEnd();
    const int lineCount = lines.count();
    for (int i = 0; i <= lineCount; i++) {
        for (int j = 0; j < lineSpans.value(i); j++) {
            data.append(TextLineData(TextLineData::Separator));
            spanOffset++;
        }
        if (i < lineCount) {
            const int textLength = lines.at(i).count() + 1;
            pos += textLength;
            data.append(lines.at(i));
        }
        while (changedIt != changedEnd) {
            if (changedIt.key() >= pos)
                break;

            if (changedIt.value() >= pos) {
                usePreviousSpanOffsetForStartPosition = true;
                previousSpanOffset = spanOffset;
                break;
            }

            const int startSpanOffset = usePreviousSpanOffsetForStartPosition
                    ? previousSpanOffset : spanOffset;
            usePreviousSpanOffsetForStartPosition = false;

            const int startPos = changedIt.key() + startSpanOffset;
            const int endPos = changedIt.value() + spanOffset;
            if (outputChangedPositions)
                outputChangedPositions->insert(startPos, endPos);
            ++changedIt;
        }
    }
    return data;
}

ChunkData DiffEditorWidget::calculateOriginalData(const QList<Diff> &diffList) const
{
    ChunkData chunkData;

    QStringList leftLines;
    QStringList rightLines;
    leftLines.append(QString());
    rightLines.append(QString());
    QMap<int, int> leftLineSpans;
    QMap<int, int> rightLineSpans;
    QMap<int, int> leftChangedPositions;
    QMap<int, int> rightChangedPositions;
    QList<int> leftEqualLines;
    QList<int> rightEqualLines;

    int currentLeftLine = 0;
    int currentLeftPos = 0;
    int currentLeftLineOffset = 0;
    int currentRightLine = 0;
    int currentRightPos = 0;
    int currentRightLineOffset = 0;
    int lastAlignedLeftLine = -1;
    int lastAlignedRightLine = -1;
    bool lastLeftLineEqual = true;
    bool lastRightLineEqual = true;

    for (int i = 0; i < diffList.count(); i++) {
        Diff diff = diffList.at(i);

        const QStringList lines = diff.text.split(QLatin1Char('\n'));

        const bool equal = isEqual(diffList, i);
        if (diff.command == Diff::Insert) {
            lastRightLineEqual = lastRightLineEqual && equal;
        } else if (diff.command == Diff::Delete) {
            lastLeftLineEqual = lastLeftLineEqual && equal;
        }

        const int lastLeftPos = currentLeftPos;
        const int lastRightPos = currentRightPos;

        for (int j = 0; j < lines.count(); j++) {
            const QString line = lines.at(j);

            if (j > 0) {
                if (diff.command == Diff::Equal) {
                    if (lastLeftLineEqual && lastRightLineEqual) {
                        leftEqualLines.append(currentLeftLine);
                        rightEqualLines.append(currentRightLine);
                    }
                }

                if (diff.command != Diff::Insert) {
                    currentLeftLine++;
                    currentLeftLineOffset++;
                    leftLines.append(QString());
                    currentLeftPos++;
                    lastLeftLineEqual = !line.count() || equal;
                }
                if (diff.command != Diff::Delete) {
                    currentRightLine++;
                    currentRightLineOffset++;
                    rightLines.append(QString());
                    currentRightPos++;
                    lastRightLineEqual = !line.count() || equal;
                }
            }

            if (diff.command == Diff::Delete) {
                leftLines.last() += line;
                currentLeftPos += line.count();
            }
            else if (diff.command == Diff::Insert) {
                rightLines.last() += line;
                currentRightPos += line.count();
            }
            else if (diff.command == Diff::Equal) {
                if ((line.count() || (j && j < lines.count() - 1)) && // don't treat empty ending line as a line to be aligned unless a line is a one char '/n' only.
                        currentLeftLine != lastAlignedLeftLine &&
                        currentRightLine != lastAlignedRightLine) {
                    // apply line spans before the current lines
                    if (currentLeftLineOffset < currentRightLineOffset) {
                        const int spans = currentRightLineOffset - currentLeftLineOffset;
                        leftLineSpans[currentLeftLine] = spans;
                    } else if (currentRightLineOffset < currentLeftLineOffset) {
                        const int spans = currentLeftLineOffset - currentRightLineOffset;
                        rightLineSpans[currentRightLine] = spans;
                    }
                    currentLeftLineOffset = 0;
                    currentRightLineOffset = 0;
                    lastAlignedLeftLine = currentLeftLine;
                    lastAlignedRightLine = currentRightLine;
                }

                leftLines.last() += line;
                rightLines.last() += line;
                currentLeftPos += line.count();
                currentRightPos += line.count();
            }
        }

        if (!equal) {
            if (diff.command == Diff::Delete && lastLeftPos != currentLeftPos)
                leftChangedPositions.insert(lastLeftPos, currentLeftPos);
            else if (diff.command == Diff::Insert && lastRightPos != currentRightPos)
                rightChangedPositions.insert(lastRightPos, currentRightPos);
        }
    }

    if (diffList.count() && diffList.last().command == Diff::Equal) {
        if (currentLeftLine != lastAlignedLeftLine &&
                currentRightLine != lastAlignedRightLine) {
            // apply line spans before the current lines
            if (currentLeftLineOffset < currentRightLineOffset) {
                const int spans = currentRightLineOffset - currentLeftLineOffset;
                leftLineSpans[currentLeftLine] = spans;
            } else if (currentRightLineOffset < currentLeftLineOffset) {
                const int spans = currentLeftLineOffset - currentRightLineOffset;
                rightLineSpans[currentRightLine] = spans;
            }
        }
        if (lastLeftLineEqual && lastRightLineEqual) {
            leftEqualLines.append(currentLeftLine);
            rightEqualLines.append(currentRightLine);
        }
    }

    QList<TextLineData> leftData = assemblyRows(leftLines,
                                                leftLineSpans,
                                                leftChangedPositions,
                                                &chunkData.changedLeftPositions);
    QList<TextLineData> rightData = assemblyRows(rightLines,
                                                 rightLineSpans,
                                                 rightChangedPositions,
                                                 &chunkData.changedRightPositions);

    // fill ending separators
    for (int i = leftData.count(); i < rightData.count(); i++)
        leftData.append(TextLineData(TextLineData::Separator));
    for (int i = rightData.count(); i < leftData.count(); i++)
        rightData.append(TextLineData(TextLineData::Separator));

    const int visualLineCount = leftData.count();
    int l = 0;
    int r = 0;
    for (int i = 0; i < visualLineCount; i++) {
        RowData row(leftData.at(i), rightData.at(i));
        if (row.leftLine.textLineType == TextLineData::Separator
                && row.rightLine.textLineType == TextLineData::Separator)
            row.equal = true;
        if (row.leftLine.textLineType == TextLineData::TextLine
                && row.rightLine.textLineType == TextLineData::TextLine
                && leftEqualLines.contains(l)
                && rightEqualLines.contains(r))
            row.equal = true;
        chunkData.rows.append(row);
        if (leftData.at(i).textLineType == TextLineData::TextLine)
            l++;
        if (rightData.at(i).textLineType == TextLineData::TextLine)
            r++;
    }
    return chunkData;
}

FileData DiffEditorWidget::calculateContextData(const ChunkData &originalData) const
{
    if (m_contextLinesNumber < 0)
        return FileData(originalData);

    const int joinChunkThreshold = 1;

    FileData fileData;
    QMap<int, bool> hiddenRows;
    int i = 0;
    while (i < originalData.rows.count()) {
        const RowData &row = originalData.rows[i];
        if (row.equal) {
            // count how many equal
            int equalRowStart = i;
            i++;
            while (i < originalData.rows.count()) {
                if (!originalData.rows.at(i).equal)
                    break;
                i++;
            }
            const bool first = equalRowStart == 0; // includes first line?
            const bool last = i == originalData.rows.count(); // includes last line?

            const int firstLine = first ? 0 : equalRowStart + m_contextLinesNumber;
            const int lastLine = last ? originalData.rows.count() : i - m_contextLinesNumber;

            if (firstLine < lastLine - joinChunkThreshold) {
                for (int j = firstLine; j < lastLine; j++) {
                    hiddenRows.insert(j, true);
                }
            }
        } else {
            // iterate to the next row
            i++;
        }
    }
    i = 0;
    int leftCharCounter = 0;
    int rightCharCounter = 0;
    QMap<int, int>::ConstIterator leftChangedIt = originalData.changedLeftPositions.constBegin();
    QMap<int, int>::ConstIterator rightChangedIt = originalData.changedRightPositions.constBegin();
    while (i < originalData.rows.count()) {
        if (!hiddenRows.contains(i)) {
            ChunkData chunkData;
            int leftOffset = leftCharCounter;
            int rightOffset = rightCharCounter;
            chunkData.contextChunk = false;
            while (i < originalData.rows.count()) {
                if (hiddenRows.contains(i))
                    break;
                RowData rowData = originalData.rows.at(i);
                chunkData.rows.append(rowData);

                leftCharCounter += rowData.leftLine.text.count() + 1; // +1 for '\n'
                rightCharCounter += rowData.rightLine.text.count() + 1; // +1 for '\n'
                i++;
            }
            while (leftChangedIt != originalData.changedLeftPositions.constEnd()) {
                if (leftChangedIt.key() < leftOffset
                        || leftChangedIt.key() > leftCharCounter)
                    break;

                const int startPos = leftChangedIt.key();
                const int endPos = leftChangedIt.value();
                chunkData.changedLeftPositions.insert(startPos - leftOffset, endPos - leftOffset);
                leftChangedIt++;
            }
            while (rightChangedIt != originalData.changedRightPositions.constEnd()) {
                if (rightChangedIt.key() < rightOffset
                        || rightChangedIt.key() > rightCharCounter)
                    break;

                const int startPos = rightChangedIt.key();
                const int endPos = rightChangedIt.value();
                chunkData.changedRightPositions.insert(startPos - rightOffset, endPos - rightOffset);
                rightChangedIt++;
            }
            fileData.chunks.append(chunkData);
        } else {
            ChunkData chunkData;
            chunkData.contextChunk = true;
            while (i < originalData.rows.count()) {
                if (!hiddenRows.contains(i))
                    break;
                RowData rowData = originalData.rows.at(i);
                chunkData.rows.append(rowData);

                leftCharCounter += rowData.leftLine.text.count() + 1; // +1 for '\n'
                rightCharCounter += rowData.rightLine.text.count() + 1; // +1 for '\n'
                i++;
            }
            fileData.chunks.append(chunkData);
        }
    }
    return fileData;
}

void DiffEditorWidget::showDiff()
{
    // TODO: remember the line number of the line in the middle
    const int verticalValue = m_leftEditor->verticalScrollBar()->value();
    const int leftHorizontalValue = m_leftEditor->horizontalScrollBar()->value();
    const int rightHorizontalValue = m_rightEditor->horizontalScrollBar()->value();

    clear();

    QString leftText, rightText;
    int blockNumber = 0;
    QChar separator = QLatin1Char('\n');
    for (int i = 0; i < m_contextFileData.count(); i++) {
        const FileData &contextFileData = m_contextFileData.at(i);

        int leftLineNumber = 0;
        int rightLineNumber = 0;
        m_leftEditor->setFileInfo(blockNumber, contextFileData.leftFileInfo);
        m_rightEditor->setFileInfo(blockNumber, contextFileData.rightFileInfo);
        leftText += separator;
        rightText += separator;
        blockNumber++;

        for (int j = 0; j < contextFileData.chunks.count(); j++) {
            ChunkData chunkData = contextFileData.chunks.at(j);
            if (chunkData.contextChunk) {
                const int skippedLines = chunkData.rows.count();
                m_leftEditor->setSkippedLines(blockNumber, skippedLines);
                m_rightEditor->setSkippedLines(blockNumber, skippedLines);
                leftText += separator;
                rightText += separator;
                blockNumber++;
            }

            for (int k = 0; k < chunkData.rows.count(); k++) {
                RowData rowData = chunkData.rows.at(k);
                TextLineData leftLineData = rowData.leftLine;
                TextLineData rightLineData = rowData.rightLine;
                if (leftLineData.textLineType == TextLineData::TextLine) {
                    leftText += leftLineData.text;
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
                leftText += separator;
                rightText += separator;
                blockNumber++;
            }
        }
    }

    if (leftText.isEmpty() && rightText.isEmpty())
        return;

    m_leftEditor->setPlainText(leftText);
    m_rightEditor->setPlainText(rightText);

    colorDiff(m_contextFileData);

    QTextBlock leftBlock = m_leftEditor->document()->firstBlock();
    QTextBlock rightBlock = m_rightEditor->document()->firstBlock();
    for (int i = 0; i < m_contextFileData.count(); i++) {
        const FileData &contextFileData = m_contextFileData.at(i);
        leftBlock = leftBlock.next();
        rightBlock = rightBlock.next();
        for (int j = 0; j < contextFileData.chunks.count(); j++) {
            ChunkData chunkData = contextFileData.chunks.at(j);
            if (chunkData.contextChunk) {
                TextEditor::BaseTextDocumentLayout::setFoldingIndent(leftBlock, FILE_LEVEL);
                TextEditor::BaseTextDocumentLayout::setFoldingIndent(rightBlock, FILE_LEVEL);
                leftBlock = leftBlock.next();
                rightBlock = rightBlock.next();
            }
            const int indent = chunkData.contextChunk ? CHUNK_LEVEL : FILE_LEVEL;
            for (int k = 0; k < chunkData.rows.count(); k++) {
                TextEditor::BaseTextDocumentLayout::setFoldingIndent(leftBlock, indent);
                TextEditor::BaseTextDocumentLayout::setFoldingIndent(rightBlock, indent);
                leftBlock = leftBlock.next();
                rightBlock = rightBlock.next();
            }
        }
    }
    blockNumber = 0;
    for (int i = 0; i < m_contextFileData.count(); i++) {
        const FileData &contextFileData = m_contextFileData.at(i);
        blockNumber++;
        for (int j = 0; j < contextFileData.chunks.count(); j++) {
            ChunkData chunkData = contextFileData.chunks.at(j);
            if (chunkData.contextChunk) {
                QTextBlock leftBlock = m_leftEditor->document()->findBlockByNumber(blockNumber);
                TextEditor::BaseTextDocumentLayout::doFoldOrUnfold(leftBlock, false);
                QTextBlock rightBlock = m_rightEditor->document()->findBlockByNumber(blockNumber);
                TextEditor::BaseTextDocumentLayout::doFoldOrUnfold(rightBlock, false);
                blockNumber++;
            }
            blockNumber += chunkData.rows.count();
        }
    }
    m_foldingBlocker = true;
    BaseTextDocumentLayout *leftLayout = qobject_cast<BaseTextDocumentLayout *>(m_leftEditor->document()->documentLayout());
    if (leftLayout) {
        leftLayout->requestUpdate();
        leftLayout->emitDocumentSizeChanged();
    }
    BaseTextDocumentLayout *rightLayout = qobject_cast<BaseTextDocumentLayout *>(m_rightEditor->document()->documentLayout());
    if (rightLayout) {
        rightLayout->requestUpdate();
        rightLayout->emitDocumentSizeChanged();
    }
    m_foldingBlocker = false;

    m_leftEditor->verticalScrollBar()->setValue(verticalValue);
    m_rightEditor->verticalScrollBar()->setValue(verticalValue);
    m_leftEditor->horizontalScrollBar()->setValue(leftHorizontalValue);
    m_rightEditor->horizontalScrollBar()->setValue(rightHorizontalValue);
    m_leftEditor->updateFoldingHighlight(QPoint(-1, -1));
    m_rightEditor->updateFoldingHighlight(QPoint(-1, -1));
}

QList<QTextEdit::ExtraSelection> DiffEditorWidget::colorPositions(
        const QTextCharFormat &format,
        QTextCursor &cursor,
        const QMap<int, int> &positions) const
{
    QList<QTextEdit::ExtraSelection> lineSelections;

    cursor.setPosition(0);
    QMapIterator<int, int> itPositions(positions);
    while (itPositions.hasNext()) {
        itPositions.next();

        cursor.setPosition(itPositions.key());
        cursor.setPosition(itPositions.value(), QTextCursor::KeepAnchor);

        QTextEdit::ExtraSelection selection;
        selection.cursor = cursor;
        selection.format = format;
        lineSelections.append(selection);
    }
    return lineSelections;
}

void DiffEditorWidget::colorDiff(const QList<FileData> &fileDataList)
{
    QPalette pal = m_leftEditor->extraArea()->palette();
    pal.setCurrentColorGroup(QPalette::Active);
    QTextCharFormat spanLineFormat;
    spanLineFormat.setBackground(pal.color(QPalette::Background));
    spanLineFormat.setProperty(QTextFormat::FullWidthSelection, true);

    int leftPos = 0;
    int rightPos = 0;
    QMap<int, int> leftLinePos;
    QMap<int, int> rightLinePos;
    QMap<int, int> leftCharPos;
    QMap<int, int> rightCharPos;
    QMap<int, int> leftSkippedPos;
    QMap<int, int> rightSkippedPos;
    QMap<int, int> leftChunkPos;
    QMap<int, int> rightChunkPos;
    QMap<int, int> leftFilePos;
    QMap<int, int> rightFilePos;
    int leftLastDiffBlockStartPos = 0;
    int rightLastDiffBlockStartPos = 0;
    int leftLastSkippedBlockStartPos = 0;
    int rightLastSkippedBlockStartPos = 0;

    for (int i = 0; i < fileDataList.count(); i++) {
        leftFilePos[leftPos] = leftPos + 1;
        rightFilePos[rightPos] = rightPos + 1;
        leftPos++; // for file line
        rightPos++; // for file line
        const FileData &fileData = fileDataList.at(i);

        for (int j = 0; j < fileData.chunks.count(); j++) {
            ChunkData chunkData = fileData.chunks.at(j);
            if (chunkData.contextChunk) {
                leftChunkPos[leftPos] = leftPos + 1;
                rightChunkPos[rightPos] = rightPos + 1;
                leftPos++; // for chunk line
                rightPos++; // for chunk line
            }
            const int leftFileOffset = leftPos;
            const int rightFileOffset = rightPos;
            leftLastDiffBlockStartPos = leftPos;
            rightLastDiffBlockStartPos = rightPos;
            leftLastSkippedBlockStartPos = leftPos;
            rightLastSkippedBlockStartPos = rightPos;

            QMapIterator<int, int> itLeft(chunkData.changedLeftPositions);
            while (itLeft.hasNext()) {
                itLeft.next();

                leftCharPos[itLeft.key() + leftFileOffset] = itLeft.value() + leftFileOffset;
            }

            QMapIterator<int, int> itRight(chunkData.changedRightPositions);
            while (itRight.hasNext()) {
                itRight.next();

                rightCharPos[itRight.key() + rightFileOffset] = itRight.value() + rightFileOffset;
            }

            for (int k = 0; k < chunkData.rows.count(); k++) {
                RowData rowData = chunkData.rows.at(k);

                leftPos += rowData.leftLine.text.count() + 1; // +1 for '\n'
                rightPos += rowData.rightLine.text.count() + 1; // +1 for '\n'

                if (!rowData.equal) {
                    if (rowData.leftLine.textLineType == TextLineData::TextLine) {
                        leftLinePos[leftLastDiffBlockStartPos] = leftPos;
                        leftLastSkippedBlockStartPos = leftPos;
                    } else {
                        leftSkippedPos[leftLastSkippedBlockStartPos] = leftPos;
                        leftLastDiffBlockStartPos = leftPos;
                    }
                    if (rowData.rightLine.textLineType == TextLineData::TextLine) {
                        rightLinePos[rightLastDiffBlockStartPos] = rightPos;
                        rightLastSkippedBlockStartPos = rightPos;
                    } else {
                        rightSkippedPos[rightLastSkippedBlockStartPos] = rightPos;
                        rightLastDiffBlockStartPos = rightPos;
                    }
                } else {
                    leftLastDiffBlockStartPos = leftPos;
                    rightLastDiffBlockStartPos = rightPos;
                    leftLastSkippedBlockStartPos = leftPos;
                    rightLastSkippedBlockStartPos = rightPos;
                }
            }
        }
    }

    QTextCursor leftCursor = m_leftEditor->textCursor();
    QTextCursor rightCursor = m_rightEditor->textCursor();

    QList<QTextEdit::ExtraSelection> leftSelections
            = colorPositions(m_leftLineFormat, leftCursor, leftLinePos);
    leftSelections
            += colorPositions(spanLineFormat, leftCursor, leftSkippedPos);
    leftSelections
            += colorPositions(m_chunkLineFormat, leftCursor, leftChunkPos);
    leftSelections
            += colorPositions(m_fileLineFormat, leftCursor, leftFilePos);
    leftSelections
            += colorPositions(m_leftCharFormat, leftCursor, leftCharPos);

    QList<QTextEdit::ExtraSelection> rightSelections
            = colorPositions(m_rightLineFormat, rightCursor, rightLinePos);
    rightSelections
            += colorPositions(spanLineFormat, rightCursor, rightSkippedPos);
    rightSelections
            += colorPositions(m_chunkLineFormat, rightCursor, rightChunkPos);
    rightSelections
            += colorPositions(m_fileLineFormat, rightCursor, rightFilePos);
    rightSelections
            += colorPositions(m_rightCharFormat, rightCursor, rightCharPos);

    m_leftEditor->setExtraSelections(BaseTextEditorWidget::OtherSelection, leftSelections);
    m_rightEditor->setExtraSelections(BaseTextEditorWidget::OtherSelection, rightSelections);
}

static QTextCharFormat fullWidthFormatForTextStyle(const TextEditor::FontSettings &fontSettings,
                                          TextEditor::TextStyle textStyle)
{
    QTextCharFormat format = fontSettings.toTextCharFormat(textStyle);
    format.setProperty(QTextFormat::FullWidthSelection, true);
    return format;
}

void DiffEditorWidget::setFontSettings(const TextEditor::FontSettings &fontSettings)
{
    m_leftEditor->setFontSettings(fontSettings);
    m_rightEditor->setFontSettings(fontSettings);

    m_fileLineFormat = fullWidthFormatForTextStyle(fontSettings, C_DIFF_FILE_LINE);
    m_chunkLineFormat = fullWidthFormatForTextStyle(fontSettings, C_DIFF_CONTEXT_LINE);
    m_leftLineFormat = fullWidthFormatForTextStyle(fontSettings, C_DIFF_SOURCE_LINE);
    m_leftCharFormat = fullWidthFormatForTextStyle(fontSettings, C_DIFF_SOURCE_CHAR);
    m_rightLineFormat = fullWidthFormatForTextStyle(fontSettings, C_DIFF_DEST_LINE);
    m_rightCharFormat = fullWidthFormatForTextStyle(fontSettings, C_DIFF_DEST_CHAR);

    colorDiff(m_contextFileData);
}

void DiffEditorWidget::slotLeftJumpToOriginalFileRequested(int diffFileIndex,
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
        int leftLineNumber = 0;
        int rightLineNumber = 0;

        for (int i = 0; i < fileData.chunks.count(); i++) {
            const ChunkData chunkData = fileData.chunks.at(i);
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

void DiffEditorWidget::slotRightJumpToOriginalFileRequested(int diffFileIndex,
                                                            int lineNumber, int columnNumber)
{
    if (diffFileIndex < 0 || diffFileIndex >= m_contextFileData.count())
        return;

    const FileData fileData = m_contextFileData.at(diffFileIndex);
    const QString fileName = fileData.rightFileInfo.fileName;
    jumpToOriginalFile(fileName, lineNumber, columnNumber);
}

void DiffEditorWidget::jumpToOriginalFile(const QString &fileName,
                                          int lineNumber, int columnNumber)
{
    const QDir dir(m_workingDirectory);
    const QString absoluteFileName = dir.absoluteFilePath(fileName);
    Core::EditorManager::openEditorAt(absoluteFileName, lineNumber, columnNumber);
}

void DiffEditorWidget::leftVSliderChanged()
{
    m_rightEditor->verticalScrollBar()->setValue(m_leftEditor->verticalScrollBar()->value());
}

void DiffEditorWidget::rightVSliderChanged()
{
    m_leftEditor->verticalScrollBar()->setValue(m_rightEditor->verticalScrollBar()->value());
}

void DiffEditorWidget::leftHSliderChanged()
{
    if (m_syncScrollBars)
        m_rightEditor->horizontalScrollBar()->setValue(m_leftEditor->horizontalScrollBar()->value());
}

void DiffEditorWidget::rightHSliderChanged()
{
    if (m_syncScrollBars)
        m_leftEditor->horizontalScrollBar()->setValue(m_rightEditor->horizontalScrollBar()->value());
}

void DiffEditorWidget::leftCursorPositionChanged()
{
    leftVSliderChanged();
    leftHSliderChanged();
    emit navigatedToDiffFile(m_leftEditor->fileIndexForBlockNumber(m_leftEditor->textCursor().blockNumber()));
}

void DiffEditorWidget::rightCursorPositionChanged()
{
    rightVSliderChanged();
    rightHSliderChanged();
    emit navigatedToDiffFile(m_rightEditor->fileIndexForBlockNumber(m_rightEditor->textCursor().blockNumber()));
}

void DiffEditorWidget::leftDocumentSizeChanged()
{
    synchronizeFoldings(m_leftEditor, m_rightEditor);
}

void DiffEditorWidget::rightDocumentSizeChanged()
{
    synchronizeFoldings(m_rightEditor, m_leftEditor);
}

void DiffEditorWidget::setHorizontalScrollBarSynchronization(bool on)
{
    m_syncScrollBars = on;
}

/* Special version of that method (original: TextEditor::BaseTextDocumentLayout::doFoldOrUnfold())
   The hack lies in fact, that when unfolding all direct sub-blocks are made visible,
   while some of them need to stay invisible (i.e. unfolded chunk lines)
*/
static void doFoldOrUnfold(DiffViewEditorWidget *editor, const QTextBlock &block, bool unfold)
{
    if (!TextEditor::BaseTextDocumentLayout::canFold(block))
        return;
    QTextBlock b = block.next();

    int indent = TextEditor::BaseTextDocumentLayout::foldingIndent(block);
    while (b.isValid() && TextEditor::BaseTextDocumentLayout::foldingIndent(b) > indent && (unfold || b.next().isValid())) {
        if (unfold && editor->isChunkLine(b.blockNumber()) && !TextEditor::BaseTextDocumentLayout::isFolded(b)) {
            b.setVisible(false);
            b.setLineCount(0);
        } else {
            b.setVisible(unfold);
            b.setLineCount(unfold ? qMax(1, b.layout()->lineCount()) : 0);
        }
        if (unfold) { // do not unfold folded sub-blocks
            if (TextEditor::BaseTextDocumentLayout::isFolded(b) && b.next().isValid()) {
                int jndent = TextEditor::BaseTextDocumentLayout::foldingIndent(b);
                b = b.next();
                while (b.isValid() && TextEditor::BaseTextDocumentLayout::foldingIndent(b) > jndent)
                    b = b.next();
                continue;
            }
        }
        b = b.next();
    }
    TextEditor::BaseTextDocumentLayout::setFolded(block, !unfold);
}

void DiffEditorWidget::synchronizeFoldings(DiffViewEditorWidget *source, DiffViewEditorWidget *destination)
{
    if (m_foldingBlocker)
        return;

    m_foldingBlocker = true;
    QTextBlock sourceBlock = source->document()->firstBlock();
    QTextBlock destinationBlock = destination->document()->firstBlock();
    while (sourceBlock.isValid() && destinationBlock.isValid()) {
        if (TextEditor::BaseTextDocumentLayout::canFold(sourceBlock)) {
            const bool isSourceFolded = TextEditor::BaseTextDocumentLayout::isFolded(sourceBlock);
            const bool isDestinationFolded = TextEditor::BaseTextDocumentLayout::isFolded(destinationBlock);
            if (isSourceFolded != isDestinationFolded) {
                if (source->isFileLine(sourceBlock.blockNumber())) {
                    doFoldOrUnfold(source, sourceBlock, !isSourceFolded);
                    doFoldOrUnfold(destination, destinationBlock, !isSourceFolded);
                } else {
                    if (isSourceFolded) { // we fold the destination (shrinking)
                        QTextBlock previousSource = sourceBlock.previous(); // skippedLines
                        QTextBlock previousDestination = destinationBlock.previous(); // skippedLines
                        if (source->isChunkLine(previousSource.blockNumber())) {
                            QTextBlock firstVisibleDestinationBlock = destination->firstVisibleBlock();
                            QTextBlock firstDestinationBlock = destination->document()->firstBlock();
                            TextEditor::BaseTextDocumentLayout::doFoldOrUnfold(destinationBlock, !isSourceFolded);
                            TextEditor::BaseTextDocumentLayout::setFoldingIndent(sourceBlock, CHUNK_LEVEL);
                            TextEditor::BaseTextDocumentLayout::setFoldingIndent(destinationBlock, CHUNK_LEVEL);
                            previousSource.setVisible(true);
                            previousSource.setLineCount(1);
                            previousDestination.setVisible(true);
                            previousDestination.setLineCount(1);
                            sourceBlock.setVisible(false);
                            sourceBlock.setLineCount(0);
                            destinationBlock.setVisible(false);
                            destinationBlock.setLineCount(0);
                            TextEditor::BaseTextDocumentLayout::setFolded(previousSource, true);
                            TextEditor::BaseTextDocumentLayout::setFolded(previousDestination, true);

                            if (firstVisibleDestinationBlock == destinationBlock) {
                                /*
                                The following hack is completely crazy. That's the only way to scroll 1 line up
                                in case destinationBlock was the top visible block.
                                There is no need to scroll the source since this is in sync anyway
                                (leftSliderChanged(), rightSliderChanged())
                                */
                                destination->verticalScrollBar()->setValue(destination->verticalScrollBar()->value() - 1);
                                destination->verticalScrollBar()->setValue(destination->verticalScrollBar()->value() + 1);
                                if (firstVisibleDestinationBlock.previous() == firstDestinationBlock) {
                                    /*
                                    Even more crazy case: the destinationBlock was the first top visible block.
                                    */
                                    destination->verticalScrollBar()->setValue(0);
                                }
                            }
                        }
                    } else { // we unfold the destination (expanding)
                        if (source->isChunkLine(sourceBlock.blockNumber())) {
                            QTextBlock nextSource = sourceBlock.next();
                            QTextBlock nextDestination = destinationBlock.next();
                            TextEditor::BaseTextDocumentLayout::doFoldOrUnfold(destinationBlock, !isSourceFolded);
                            TextEditor::BaseTextDocumentLayout::setFoldingIndent(nextSource, FILE_LEVEL);
                            TextEditor::BaseTextDocumentLayout::setFoldingIndent(nextDestination, FILE_LEVEL);
                            sourceBlock.setVisible(false);
                            sourceBlock.setLineCount(0);
                            destinationBlock.setVisible(false);
                            destinationBlock.setLineCount(0);
                            TextEditor::BaseTextDocumentLayout::setFolded(nextSource, false);
                            TextEditor::BaseTextDocumentLayout::setFolded(nextDestination, false);
                        }
                    }
                }
                break; // only one should be synchronized
            }
        }

        sourceBlock = sourceBlock.next();
        destinationBlock = destinationBlock.next();
    }

    BaseTextDocumentLayout *sourceLayout = qobject_cast<BaseTextDocumentLayout *>(source->document()->documentLayout());
    if (sourceLayout) {
        sourceLayout->requestUpdate();
        sourceLayout->emitDocumentSizeChanged();
    }

    QWidget *ea = source->extraArea();
    if (ea->contentsRect().contains(ea->mapFromGlobal(QCursor::pos())))
        source->updateFoldingHighlight(source->mapFromGlobal(QCursor::pos()));

    BaseTextDocumentLayout *destinationLayout = qobject_cast<BaseTextDocumentLayout *>(destination->document()->documentLayout());
    if (destinationLayout) {
        destinationLayout->requestUpdate();
        destinationLayout->emitDocumentSizeChanged();
    }
    m_foldingBlocker = false;
}


} // namespace DiffEditor

#ifdef WITH_TESTS
#include <QTest>

void DiffEditor::DiffEditorWidget::testAssemblyRows()
{
    QStringList lines;
    lines << QLatin1String("abcd efgh"); // line 0
    lines << QLatin1String("ijkl mnop"); // line 1

    QMap<int, int> lineSpans;
    lineSpans[1] = 6; // before line 1 insert 6 span lines

    QMap<int, int> changedPositions;
    changedPositions[5] = 14; // changed text from position 5 to position 14, occupy 9 characters: "efgh\nijkl"

    QMap<int, int> expectedChangedPositions;
    expectedChangedPositions[5] = 20; // "efgh\n[\n\n\n\n\n\n]ijkl" - [\n] means inserted span

    QMap<int, int> outputChangedPositions;

    assemblyRows(lines, lineSpans, changedPositions, &outputChangedPositions);
    QVERIFY(outputChangedPositions == expectedChangedPositions);
}

#endif // WITH_TESTS

#include "diffeditorwidget.moc"
