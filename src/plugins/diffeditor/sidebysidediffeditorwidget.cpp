/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "sidebysidediffeditorwidget.h"
#include "selectabletexteditorwidget.h"
#include "diffeditorguicontroller.h"
#include "diffutils.h"
#include "diffeditorconstants.h"

#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QPlainTextDocumentLayout>
#include <QTextBlock>
#include <QScrollBar>
#include <QPainter>
#include <QDir>
#include <QToolButton>
#include <QTextCodec>
#include <QMessageBox>

#include <texteditor/basetexteditor.h>
#include <texteditor/basetextdocumentlayout.h>
#include <texteditor/highlighterfactory.h>
#include <texteditor/syntaxhighlighter.h>
#include <texteditor/basetextdocument.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/displaysettings.h>
#include <texteditor/highlighterutils.h>

#include <coreplugin/icore.h>
#include <coreplugin/minisplitter.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/patchtool.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/tooltip/tipcontents.h>
#include <utils/tooltip/tooltip.h>

//static const int FILE_LEVEL = 1;
//static const int CHUNK_LEVEL = 2;

using namespace Core;
using namespace TextEditor;

namespace DiffEditor {

//////////////////////

class SideDiffEditor : public BaseTextEditor
{
    Q_OBJECT
public:
    SideDiffEditor(BaseTextEditorWidget *editorWidget)
        : BaseTextEditor(editorWidget)
    {
        document()->setId("DiffEditor.SideDiffEditor");
        connect(this, SIGNAL(tooltipRequested(TextEditor::BaseTextEditor*,QPoint,int)),
                this, SLOT(slotTooltipRequested(TextEditor::BaseTextEditor*,QPoint,int)));
    }

private slots:
    void slotTooltipRequested(TextEditor::BaseTextEditor *editor,
                              const QPoint &globalPoint,
                              int position);

};

////////////////////////
/*
class MultiHighlighter : public SyntaxHighlighter
{
    Q_OBJECT
public:
    MultiHighlighter(SideDiffEditorWidget *editor, QTextDocument *document = 0);
    ~MultiHighlighter();

    virtual void setFontSettings(const TextEditor::FontSettings &fontSettings);
    void setDocuments(const QList<QPair<DiffFileInfo, QString> > &documents);

protected:
    virtual void highlightBlock(const QString &text);

private:
    SideDiffEditorWidget *m_editor;
    QMap<QString, HighlighterFactory *> m_mimeTypeToHighlighterFactory;
    QList<SyntaxHighlighter *> m_highlighters;
    QList<QTextDocument *> m_documents;
};
*/
////////////////////////

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
        return BaseTextEditorWidget::firstVisibleBlock();
    }
//    void setDocuments(const QList<QPair<DiffFileInfo, QString> > &documents);

public slots:
    void setDisplaySettings(const DisplaySettings &ds);

signals:
    void jumpToOriginalFileRequested(int diffFileIndex,
                                     int lineNumber,
                                     int columnNumber);
    void contextMenuRequested(QMenu *menu,
                              int diffFileIndex,
                              int chunkIndex);

protected:
    virtual int extraAreaWidth(int *markWidthPtr = 0) const {
        return SelectableTextEditorWidget::extraAreaWidth(markWidthPtr);
    }
    void applyFontSettings();
    BaseTextEditor *createEditor() { return new SideDiffEditor(this); }
    virtual QString lineNumber(int blockNumber) const;
    virtual int lineNumberDigits() const;
    virtual bool selectionVisible(int blockNumber) const;
    virtual bool replacementVisible(int blockNumber) const;
    QColor replacementPenColor(int blockNumber) const;
    virtual QString plainTextFromSelection(const QTextCursor &cursor) const;
//    virtual void drawCollapsedBlockPopup(QPainter &painter,
//                                 const QTextBlock &block,
//                                 QPointF offset,
//                                 const QRect &clip);
    void mouseDoubleClickEvent(QMouseEvent *e);
    void contextMenuEvent(QContextMenuEvent *e);
    virtual void paintEvent(QPaintEvent *e);
    virtual void scrollContentsBy(int dx, int dy);

private:
//    void paintCollapsedBlockPopup(QPainter &painter, const QRect &clipRect);
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
//    MultiHighlighter *m_highlighter;
};

////////////////////////

void SideDiffEditor::slotTooltipRequested(TextEditor::BaseTextEditor *editor,
                                          const QPoint &globalPoint,
                                          int position)
{
    SideDiffEditorWidget *ew = qobject_cast<SideDiffEditorWidget *>(editorWidget());
    if (!ew)
        return;

    QMap<int, DiffFileInfo> fi = ew->fileInfo();
    QMap<int, DiffFileInfo>::const_iterator it
            = fi.constFind(ew->document()->findBlock(position).blockNumber());
    if (it != fi.constEnd()) {
        Utils::ToolTip::show(globalPoint, Utils::TextContent(it.value().fileName),
                                         editor->widget());
    } else {
        Utils::ToolTip::hide();
    }
}

////////////////////////
/*
MultiHighlighter::MultiHighlighter(SideDiffEditorWidget *editor, QTextDocument *document)
    : SyntaxHighlighter(document),
      m_editor(editor)
{
    const QList<HighlighterFactory *> &factories =
        ExtensionSystem::PluginManager::getObjects<TextEditor::HighlighterFactory>();
    foreach (HighlighterFactory *factory, factories) {
        QStringList mimeTypes = factory->mimeTypes();
        foreach (const QString &mimeType, mimeTypes)
            m_mimeTypeToHighlighterFactory.insert(mimeType, factory);
    }
}

MultiHighlighter::~MultiHighlighter()
{
    setDocuments(QList<QPair<DiffFileInfo, QString> >());
}

void MultiHighlighter::setFontSettings(const TextEditor::FontSettings &fontSettings)
{
    foreach (SyntaxHighlighter *highlighter, m_highlighters) {
        if (highlighter) {
            highlighter->setFontSettings(fontSettings);
            highlighter->rehighlight();
        }
    }
}

void MultiHighlighter::setDocuments(const QList<QPair<DiffFileInfo, QString> > &documents)
{
    // clear old documents
    qDeleteAll(m_documents);
    m_documents.clear();
    qDeleteAll(m_highlighters);
    m_highlighters.clear();

    // create new documents
    for (int i = 0; i < documents.count(); i++) {
        DiffFileInfo fileInfo = documents.at(i).first;
        const QString contents = documents.at(i).second;
        QTextDocument *document = new QTextDocument(contents);
        const MimeType mimeType = MimeDatabase::findByFile(QFileInfo(fileInfo.fileName));
        SyntaxHighlighter *highlighter = 0;
        if (const HighlighterFactory *factory = m_mimeTypeToHighlighterFactory.value(mimeType.type())) {
            highlighter = factory->createHighlighter();
            if (highlighter)
                highlighter->setDocument(document);
        }
        if (!highlighter) {
            highlighter = createGenericSyntaxHighlighter(mimeType);
            highlighter->setDocument(document);
        }
        m_documents.append(document);
        m_highlighters.append(highlighter);
    }
}

void MultiHighlighter::highlightBlock(const QString &text)
{
    Q_UNUSED(text)

    QTextBlock block = currentBlock();
    const int fileIndex = m_editor->fileIndexForBlockNumber(block.blockNumber());
    if (fileIndex < 0)
        return;

    SyntaxHighlighter *currentHighlighter = m_highlighters.at(fileIndex);
    if (!currentHighlighter)
        return;

    // find block in document
    QTextDocument *currentDocument = m_documents.at(fileIndex);
    if (!currentDocument)
        return;

    QTextBlock documentBlock = currentDocument->findBlockByNumber(
                block.blockNumber() - m_editor->blockNumberForFileIndex(fileIndex));

    if (!documentBlock.isValid())
        return;

    QList<QTextLayout::FormatRange> formats = documentBlock.layout()->additionalFormats();
    setExtraAdditionalFormats(block, formats);
}
*/
////////////////////////

SideDiffEditorWidget::SideDiffEditorWidget(QWidget *parent)
    : SelectableTextEditorWidget(parent),
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

//    setCodeFoldingSupported(true);

//    m_highlighter = new MultiHighlighter(this, baseTextDocument()->document());
//    baseTextDocument()->setSyntaxHighlighter(m_highlighter);
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
    const TextEditor::FontSettings &fs = textDocument()->fontSettings();
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
           && TextEditor::BaseTextDocumentLayout::isFolded(
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
    QMap<int, DiffFileInfo>::const_iterator it
            = m_fileInfo.constBegin();
    QMap<int, DiffFileInfo>::const_iterator itEnd
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
    setExtraSelections(BaseTextEditorWidget::OtherSelection,
                       QList<QTextEdit::ExtraSelection>());
    setPlainText(message);
//    m_highlighter->setDocuments(QList<QPair<DiffFileInfo, QString> >());
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
/*
void  SideDiffEditorWidget::setDocuments(const QList<QPair<DiffFileInfo, QString> > &documents)
{
    m_highlighter->setDocuments(documents);
}
*/
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
            || !TextEditor::BaseTextDocumentLayout::isFolded(block)) {
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

    connect(this, SIGNAL(destroyed()), menu, SLOT(deleteLater()));
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
//    paintCollapsedBlockPopup(painter, e->rect());
}
/*
void SideDiffEditorWidget::paintCollapsedBlockPopup(QPainter &painter, const QRect &clipRect)
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

void SideDiffEditorWidget::drawCollapsedBlockPopup(QPainter &painter,
                                             const QTextBlock &block,
                                             QPointF offset,
                                             const QRect &clip)
{
    // We ignore the call coming from the SelectableTextEditorWidget::paintEvent()
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
*/

//////////////////

SideBySideDiffEditorWidget::SideBySideDiffEditorWidget(QWidget *parent)
    : QWidget(parent)
    , m_guiController(0)
    , m_controller(0)
    , m_ignoreCurrentIndexChange(false)
    , m_foldingBlocker(false)
    , m_contextMenuFileIndex(-1)
    , m_contextMenuChunkIndex(-1)
{
    m_leftEditor = new SideDiffEditorWidget(this);
    m_leftEditor->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_leftEditor->setReadOnly(true);
    connect(TextEditorSettings::instance(),
            SIGNAL(displaySettingsChanged(TextEditor::DisplaySettings)),
            m_leftEditor, SLOT(setDisplaySettings(TextEditor::DisplaySettings)));
    m_leftEditor->setDisplaySettings(TextEditorSettings::displaySettings());
    m_leftEditor->setCodeStyle(TextEditorSettings::codeStyle());
    connect(m_leftEditor, SIGNAL(jumpToOriginalFileRequested(int,int,int)),
            this, SLOT(slotLeftJumpToOriginalFileRequested(int,int,int)));
    connect(m_leftEditor, SIGNAL(contextMenuRequested(QMenu*,int,int)),
            this, SLOT(slotLeftContextMenuRequested(QMenu*,int,int)),
            Qt::DirectConnection);

    m_rightEditor = new SideDiffEditorWidget(this);
    m_rightEditor->setReadOnly(true);
    connect(TextEditorSettings::instance(),
            SIGNAL(displaySettingsChanged(TextEditor::DisplaySettings)),
            m_rightEditor, SLOT(setDisplaySettings(TextEditor::DisplaySettings)));
    m_rightEditor->setDisplaySettings(TextEditorSettings::displaySettings());
    m_rightEditor->setCodeStyle(TextEditorSettings::codeStyle());
    connect(m_rightEditor, SIGNAL(jumpToOriginalFileRequested(int,int,int)),
            this, SLOT(slotRightJumpToOriginalFileRequested(int,int,int)));
    connect(m_rightEditor, SIGNAL(contextMenuRequested(QMenu*,int,int)),
            this, SLOT(slotRightContextMenuRequested(QMenu*,int,int)),
            Qt::DirectConnection);

    connect(TextEditorSettings::instance(),
            SIGNAL(fontSettingsChanged(TextEditor::FontSettings)),
            this, SLOT(setFontSettings(TextEditor::FontSettings)));
    setFontSettings(TextEditorSettings::fontSettings());

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
//    connect(m_leftEditor->document()->documentLayout(), SIGNAL(documentSizeChanged(QSizeF)),
//            this, SLOT(leftDocumentSizeChanged()));

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
//    connect(m_rightEditor->document()->documentLayout(), SIGNAL(documentSizeChanged(QSizeF)),
//            this, SLOT(rightDocumentSizeChanged()));

    m_splitter = new Core::MiniSplitter(this);
    m_splitter->addWidget(m_leftEditor);
    m_splitter->addWidget(m_rightEditor);
    QVBoxLayout *l = new QVBoxLayout(this);
    l->setMargin(0);
    l->addWidget(m_splitter);

    clear(tr("No controller"));
}

SideBySideDiffEditorWidget::~SideBySideDiffEditorWidget()
{

}

void SideBySideDiffEditorWidget::setDiffEditorGuiController(
        DiffEditorGuiController *controller)
{
    if (m_guiController == controller)
        return;

    if (m_guiController) {
        disconnect(m_controller, SIGNAL(cleared(QString)),
                   this, SLOT(clearAll(QString)));
        disconnect(m_controller, SIGNAL(diffFilesChanged(QList<FileData>,QString)),
                   this, SLOT(setDiff(QList<FileData>,QString)));

        disconnect(m_guiController, SIGNAL(currentDiffFileIndexChanged(int)),
                   this, SLOT(setCurrentDiffFileIndex(int)));

        clearAll(tr("No controller"));
    }
    m_guiController = controller;
    m_controller = 0;
    if (m_guiController) {
        m_controller = m_guiController->controller();

        connect(m_controller, SIGNAL(cleared(QString)),
                this, SLOT(clearAll(QString)));
        connect(m_controller, SIGNAL(diffFilesChanged(QList<FileData>,QString)),
                this, SLOT(setDiff(QList<FileData>,QString)));

        connect(m_guiController, SIGNAL(currentDiffFileIndexChanged(int)),
                this, SLOT(setCurrentDiffFileIndex(int)));

        setDiff(m_controller->diffFiles(), m_controller->workingDirectory());
        setCurrentDiffFileIndex(m_guiController->currentDiffFileIndex());
    }
}


DiffEditorGuiController *SideBySideDiffEditorWidget::diffEditorGuiController() const
{
    return m_guiController;
}

void SideBySideDiffEditorWidget::clear(const QString &message)
{
    const bool oldIgnore = m_ignoreCurrentIndexChange;
    m_ignoreCurrentIndexChange = true;
    m_leftEditor->clearAll(message);
    m_rightEditor->clearAll(message);
    m_ignoreCurrentIndexChange = oldIgnore;
}

void SideBySideDiffEditorWidget::clearAll(const QString &message)
{
    setDiff(QList<FileData>(), QString());
    clear(message);
}

void SideBySideDiffEditorWidget::setDiff(const QList<FileData> &diffFileList,
                                         const QString &workingDirectory)
{
    Q_UNUSED(workingDirectory)

    m_contextFileData = diffFileList;
    showDiff();
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

    QTextBlock rightBlock = m_rightEditor->document()->findBlockByNumber(blockNumber);
    QTextCursor rightCursor = m_rightEditor->textCursor();
    rightCursor.setPosition(rightBlock.position());
    m_rightEditor->setTextCursor(rightCursor);

    m_leftEditor->centerCursor();
    m_rightEditor->centerCursor();
    m_ignoreCurrentIndexChange = oldIgnore;
}

void SideBySideDiffEditorWidget::showDiff()
{
    // TODO: remember the line number of the line in the middle
    const int verticalValue = m_leftEditor->verticalScrollBar()->value();
    const int leftHorizontalValue = m_leftEditor->horizontalScrollBar()->value();
    const int rightHorizontalValue = m_rightEditor->horizontalScrollBar()->value();

    clear(tr("No difference"));

    QMap<int, QList<DiffSelection> > leftFormats;
    QMap<int, QList<DiffSelection> > rightFormats;

//    QList<QPair<DiffFileInfo, QString> > leftDocs, rightDocs;
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
//        leftDocs.append(qMakePair(contextFileData.leftFileInfo, leftText));
//        rightDocs.append(qMakePair(contextFileData.rightFileInfo, rightText));
    }

    if (leftTexts.isEmpty() && rightTexts.isEmpty())
        return;

//    m_leftEditor->setDocuments(leftDocs);
//    m_rightEditor->setDocuments(rightDocs);

    const bool oldIgnore = m_ignoreCurrentIndexChange;
    m_ignoreCurrentIndexChange = true;
    m_leftEditor->clear();
    m_leftEditor->setPlainText(leftTexts);
    m_rightEditor->clear();
    m_rightEditor->setPlainText(rightTexts);
    m_ignoreCurrentIndexChange = oldIgnore;

    m_leftEditor->setSelections(leftFormats);
    m_rightEditor->setSelections(rightFormats);


/*
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
*/
    m_leftEditor->verticalScrollBar()->setValue(verticalValue);
    m_rightEditor->verticalScrollBar()->setValue(verticalValue);
    m_leftEditor->horizontalScrollBar()->setValue(leftHorizontalValue);
    m_rightEditor->horizontalScrollBar()->setValue(rightHorizontalValue);

//    m_leftEditor->updateFoldingHighlight(QPoint(-1, -1));
//    m_rightEditor->updateFoldingHighlight(QPoint(-1, -1));
}

void SideBySideDiffEditorWidget::setFontSettings(
        const TextEditor::FontSettings &fontSettings)
{
    m_leftEditor->textDocument()->setFontSettings(fontSettings);
    m_rightEditor->textDocument()->setFontSettings(fontSettings);

    m_spanLineFormat  = fontSettings.toTextCharFormat(C_LINE_NUMBER);
    m_fileLineFormat  = fontSettings.toTextCharFormat(C_DIFF_FILE_LINE);
    m_chunkLineFormat = fontSettings.toTextCharFormat(C_DIFF_CONTEXT_LINE);
    m_leftLineFormat  = fontSettings.toTextCharFormat(C_DIFF_SOURCE_LINE);
    m_leftCharFormat  = fontSettings.toTextCharFormat(C_DIFF_SOURCE_CHAR);
    m_rightLineFormat = fontSettings.toTextCharFormat(C_DIFF_DEST_LINE);
    m_rightCharFormat = fontSettings.toTextCharFormat(C_DIFF_DEST_CHAR);

    m_leftEditor->update();
    m_rightEditor->update();
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
    if (!m_controller)
        return;

    const QDir dir(m_controller->workingDirectory());
    const QString absoluteFileName = dir.absoluteFilePath(fileName);
    QFileInfo fi(absoluteFileName);
    if (fi.exists() && !fi.isDir())
        Core::EditorManager::openEditorAt(absoluteFileName, lineNumber, columnNumber);
}

void SideBySideDiffEditorWidget::slotLeftContextMenuRequested(QMenu *menu,
                                                              int diffFileIndex,
                                                              int chunkIndex)
{
    menu->addSeparator();
    QAction *sendChunkToCodePasterAction =
            menu->addAction(tr("Send Chunk to CodePaster..."));
    connect(sendChunkToCodePasterAction, SIGNAL(triggered()),
            this, SLOT(slotSendChunkToCodePaster()));
    menu->addSeparator();
    QAction *applyAction = menu->addAction(tr("Apply Chunk..."));
    connect(applyAction, SIGNAL(triggered()), this, SLOT(slotApplyChunk()));
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

    m_controller->requestChunkActions(menu, diffFileIndex, chunkIndex);

    if (fileData.leftFileInfo.fileName == fileData.rightFileInfo.fileName)
        return;

    applyAction->setEnabled(true);
}

void SideBySideDiffEditorWidget::slotRightContextMenuRequested(QMenu *menu,
                                                               int diffFileIndex,
                                                               int chunkIndex)
{
    menu->addSeparator();
    QAction *sendChunkToCodePasterAction =
            menu->addAction(tr("Send Chunk to CodePaster..."));
    connect(sendChunkToCodePasterAction, SIGNAL(triggered()),
            this, SLOT(slotSendChunkToCodePaster()));
    menu->addSeparator();
    QAction *revertAction = menu->addAction(tr("Revert Chunk..."));
    connect(revertAction, SIGNAL(triggered()), this, SLOT(slotRevertChunk()));
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

    m_controller->requestChunkActions(menu, diffFileIndex, chunkIndex);

    revertAction->setEnabled(true);
}

void SideBySideDiffEditorWidget::slotSendChunkToCodePaster()
{
    if (!m_controller)
        return;

    if (m_contextMenuFileIndex < 0 || m_contextMenuChunkIndex < 0)
        return;

    if (m_contextMenuFileIndex >= m_contextFileData.count())
        return;

    const FileData fileData = m_contextFileData.at(m_contextMenuFileIndex);
    if (m_contextMenuChunkIndex >= fileData.chunks.count())
        return;

    const QString patch = m_controller->makePatch(m_contextMenuFileIndex,
                                                  m_contextMenuChunkIndex,
                                                  false);
    if (patch.isEmpty())
        return;

    // Retrieve service by soft dependency.
    QObject *pasteService =
            ExtensionSystem::PluginManager::getObjectByClassName(
                QLatin1String("CodePaster::CodePasterService"));
    if (pasteService) {
        QMetaObject::invokeMethod(pasteService, "postText",
                                  Q_ARG(QString, patch),
                                  Q_ARG(QString, QLatin1String(DiffEditor::Constants::DIFF_EDITOR_MIMETYPE)));
    } else {
        QMessageBox::information(this, tr("Unable to Paste"),
                                 tr("Code pasting services are not available."));
    }
}

void SideBySideDiffEditorWidget::slotApplyChunk()
{
    patch(m_contextMenuFileIndex, m_contextMenuChunkIndex, false);
}

void SideBySideDiffEditorWidget::slotRevertChunk()
{
    patch(m_contextMenuFileIndex, m_contextMenuChunkIndex, true);
}

void SideBySideDiffEditorWidget::patch(int diffFileIndex, int chunkIndex, bool revert)
{
    if (!m_controller)
        return;

    if (diffFileIndex < 0 || chunkIndex < 0)
        return;

    if (diffFileIndex >= m_contextFileData.count())
        return;

    const FileData fileData = m_contextFileData.at(diffFileIndex);
    if (chunkIndex >= fileData.chunks.count())
        return;

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

    const int strip = m_controller->workingDirectory().isEmpty() ? -1 : 0;

    const QString fileName = revert
            ? fileData.rightFileInfo.fileName
            : fileData.leftFileInfo.fileName;

    const QString workingDirectory = m_controller->workingDirectory().isEmpty()
            ? QFileInfo(fileName).absolutePath()
            : m_controller->workingDirectory();

    const QString patch = m_controller->makePatch(diffFileIndex, chunkIndex, revert);

    if (patch.isEmpty())
        return;

    if (PatchTool::runPatch(Core::EditorManager::defaultTextCodec()->fromUnicode(patch),
                        workingDirectory, strip, revert))
        m_controller->requestReload();
}

void SideBySideDiffEditorWidget::leftVSliderChanged()
{
    m_rightEditor->verticalScrollBar()->setValue(
                m_leftEditor->verticalScrollBar()->value());
}

void SideBySideDiffEditorWidget::rightVSliderChanged()
{
    m_leftEditor->verticalScrollBar()->setValue(
                m_rightEditor->verticalScrollBar()->value());
}

void SideBySideDiffEditorWidget::leftHSliderChanged()
{
    if (!m_guiController || m_guiController->horizontalScrollBarSynchronization())
        m_rightEditor->horizontalScrollBar()->setValue(
                    m_leftEditor->horizontalScrollBar()->value());
}

void SideBySideDiffEditorWidget::rightHSliderChanged()
{
    if (!m_guiController || m_guiController->horizontalScrollBarSynchronization())
        m_leftEditor->horizontalScrollBar()->setValue(
                    m_rightEditor->horizontalScrollBar()->value());
}

void SideBySideDiffEditorWidget::leftCursorPositionChanged()
{
    leftVSliderChanged();
    leftHSliderChanged();

    if (!m_guiController)
        return;

    if (m_ignoreCurrentIndexChange)
        return;

    const bool oldIgnore = m_ignoreCurrentIndexChange;
    m_ignoreCurrentIndexChange = true;
    m_guiController->setCurrentDiffFileIndex(
                m_leftEditor->fileIndexForBlockNumber(
                    m_leftEditor->textCursor().blockNumber()));
    m_ignoreCurrentIndexChange = oldIgnore;
}

void SideBySideDiffEditorWidget::rightCursorPositionChanged()
{
    rightVSliderChanged();
    rightHSliderChanged();

    if (!m_guiController)
        return;

    if (m_ignoreCurrentIndexChange)
        return;

    const bool oldIgnore = m_ignoreCurrentIndexChange;
    m_ignoreCurrentIndexChange = true;
    m_guiController->setCurrentDiffFileIndex(
                m_rightEditor->fileIndexForBlockNumber(
                    m_rightEditor->textCursor().blockNumber()));
    m_ignoreCurrentIndexChange = oldIgnore;
}

#if 0
void SideBySideDiffEditorWidget::leftDocumentSizeChanged()
{
    synchronizeFoldings(m_leftEditor, m_rightEditor);
}

void SideBySideDiffEditorWidget::rightDocumentSizeChanged()
{
    synchronizeFoldings(m_rightEditor, m_leftEditor);
}

/* Special version of that method (original: TextEditor::BaseTextDocumentLayout::doFoldOrUnfold())
   The hack lies in fact, that when unfolding all direct sub-blocks are made visible,
   while some of them need to stay invisible (i.e. unfolded chunk lines)
*/
static void doFoldOrUnfold(SideDiffEditorWidget *editor, const QTextBlock &block, bool unfold)
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

void SideBySideDiffEditorWidget::synchronizeFoldings(SideDiffEditorWidget *source, SideDiffEditorWidget *destination)
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
#endif

} // namespace DiffEditor

#include "sidebysidediffeditorwidget.moc"
