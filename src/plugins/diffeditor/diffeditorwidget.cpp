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
#include <QSplitter>
#include <QVBoxLayout>
#include <QPlainTextDocumentLayout>
#include <QTextBlock>
#include <QScrollBar>
#include <QPainter>
#include <QTime>

#include <QDebug>

#include <texteditor/basetexteditor.h>
#include <texteditor/snippets/snippeteditor.h>
#include <texteditor/basetextdocumentlayout.h>
#include <texteditor/syntaxhighlighter.h>
#include <texteditor/basetextdocument.h>
#include <texteditor/texteditorsettings.h>

using namespace TextEditor;

namespace DiffEditor {

//////////////////////

class DiffViewEditorEditable : public BaseTextEditor
{
Q_OBJECT
public:
    DiffViewEditorEditable(BaseTextEditorWidget *editorWidget) : BaseTextEditor(editorWidget) {}
    virtual Core::Id id() const { return "DiffViewEditor"; }
    virtual bool duplicateSupported() const { return false; }
    virtual IEditor *duplicate(QWidget *parent) { Q_UNUSED(parent) return 0; }
    virtual bool isTemporary() const { return false; }

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

    void setLineNumber(int blockNumber, const QString &lineNumber);
    void setSkippedLines(int blockNumber, int skippedLines) { m_skippedLines[blockNumber] = skippedLines; }
    void setSeparator(int blockNumber, bool separator) { m_separators[blockNumber] = separator; }
    void clearLineNumbers();
    void clearSkippedLines() { m_skippedLines.clear(); }
    void clearSeparators() { m_separators.clear(); }
    QTextBlock firstVisibleBlock() const { return SnippetEditorWidget::firstVisibleBlock(); }

protected:
    virtual int extraAreaWidth(int *markWidthPtr = 0) const { return BaseTextEditorWidget::extraAreaWidth(markWidthPtr); }
    BaseTextEditor *createEditor() { return new DiffViewEditorEditable(this); }
    virtual QString lineNumber(int blockNumber) const;
    virtual int lineNumberDigits() const;
    virtual bool selectionVisible(int blockNumber) const;
    virtual bool replacementVisible(int blockNumber) const;
    virtual void paintEvent(QPaintEvent *e);
    virtual void scrollContentsBy(int dx, int dy);

private:
    QMap<int, QString> m_lineNumbers;
    int m_lineNumberDigits;
    // block number, skipped lines
    QMap<int, int> m_skippedLines;
    // block number, separator. Separator used as lines alignment and inside skipped lines
    QMap<int, bool> m_separators;
};

DiffViewEditorWidget::DiffViewEditorWidget(QWidget *parent)
    : SnippetEditorWidget(parent), m_lineNumberDigits(1)
{
    setLineNumbersVisible(true);
    setCodeFoldingSupported(true);
    setFrameStyle(QFrame::NoFrame);
}

QString DiffViewEditorWidget::lineNumber(int blockNumber) const
{
    return m_lineNumbers.value(blockNumber);
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
    return m_skippedLines.value(blockNumber);
}

void DiffViewEditorWidget::setLineNumber(int blockNumber, const QString &lineNumber)
{
    m_lineNumbers.insert(blockNumber, lineNumber);
    m_lineNumberDigits = qMax(m_lineNumberDigits, lineNumber.count());
}

void DiffViewEditorWidget::clearLineNumbers()
{
    m_lineNumbers.clear();
    m_lineNumberDigits = 1;
}

void DiffViewEditorWidget::scrollContentsBy(int dx, int dy)
{
    SnippetEditorWidget::scrollContentsBy(dx, dy);
    // TODO: update only chunk lines
    viewport()->update();
}

void DiffViewEditorWidget::paintEvent(QPaintEvent *e)
{
    SnippetEditorWidget::paintEvent(e);
    QPainter painter(viewport());

    QPointF offset = contentOffset();
    QTextBlock firstBlock = firstVisibleBlock();
    QTextBlock currentBlock = firstBlock;

    QMap<int, int> skipped = skippedLines();

    while (currentBlock.isValid()) {
        if (currentBlock.isVisible()) {
            qreal top = blockBoundingGeometry(currentBlock).translated(offset).top();
            qreal bottom = top + blockBoundingRect(currentBlock).height();

            if (top > e->rect().bottom())
                break;

            if (bottom >= e->rect().top()) {
                QTextLayout *layout = currentBlock.layout();

                int skippedBefore = skipped.value(currentBlock.blockNumber());
                if (skippedBefore) {
                    painter.save();
                    painter.setPen(palette().foreground().color());
                    QTextLine textLine = layout->lineAt(0);
                    QRectF lineRect = textLine.naturalTextRect().translated(0, top);
                    QString skippedRowsText = tr("Skipped %n lines...", 0, skippedBefore);
                    QFontMetrics fm(font());
                    const int textWidth = fm.width(skippedRowsText);
                    painter.drawText(QPointF(lineRect.right()
                                             + (viewport()->width() - textWidth) / 2.0,
                                             lineRect.top() + textLine.ascent()),
                                     skippedRowsText);
                    painter.restore();
                }
            }
        }
        currentBlock = currentBlock.next();
    }
}

//////////////////

DiffEditorWidget::DiffEditorWidget(QWidget *parent)
    : QWidget(parent),
      m_contextLinesNumber(1),
      m_ignoreWhitespaces(true),
      m_foldingBlocker(false)
{
    TextEditor::TextEditorSettings *settings = TextEditorSettings::instance();

    m_leftEditor = new DiffViewEditorWidget(this);
    m_leftEditor->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_leftEditor->setReadOnly(true);
    m_leftEditor->setHighlightCurrentLine(false);
    m_leftEditor->setWordWrapMode(QTextOption::NoWrap);
    m_leftEditor->setFontSettings(settings->fontSettings());

    m_rightEditor = new DiffViewEditorWidget(this);
    m_rightEditor->setReadOnly(true);
    m_rightEditor->setHighlightCurrentLine(false);
    m_rightEditor->setWordWrapMode(QTextOption::NoWrap);
    m_rightEditor->setFontSettings(settings->fontSettings());

    connect(m_leftEditor->verticalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(leftSliderChanged()));
    connect(m_leftEditor->verticalScrollBar(), SIGNAL(actionTriggered(int)),
            this, SLOT(leftSliderChanged()));
    connect(m_leftEditor, SIGNAL(cursorPositionChanged()),
            this, SLOT(leftSliderChanged()));
    connect(m_leftEditor->document()->documentLayout(), SIGNAL(documentSizeChanged(QSizeF)),
            this, SLOT(leftDocumentSizeChanged()));
    connect(m_rightEditor->verticalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(rightSliderChanged()));
    connect(m_rightEditor->verticalScrollBar(), SIGNAL(actionTriggered(int)),
            this, SLOT(rightSliderChanged()));
    connect(m_rightEditor, SIGNAL(cursorPositionChanged()),
            this, SLOT(rightSliderChanged()));
    connect(m_rightEditor->document()->documentLayout(), SIGNAL(documentSizeChanged(QSizeF)),
            this, SLOT(rightDocumentSizeChanged()));

    m_splitter = new QSplitter(this);
    m_splitter->addWidget(m_leftEditor);
    m_splitter->addWidget(m_rightEditor);
    QVBoxLayout *l = new QVBoxLayout(this);
    l->addWidget(m_splitter);
}

DiffEditorWidget::~DiffEditorWidget()
{

}

void DiffEditorWidget::setDiff(const QString &leftText, const QString &rightText)
{
//    QTime time;
//    time.start();
    Differ differ;
    QList<Diff> list = differ.cleanupSemantics(differ.diff(leftText, rightText));
//    int ela = time.elapsed();
//    qDebug() << "Time spend in diff:" << ela;
    setDiff(list);
}

void DiffEditorWidget::setDiff(const QList<Diff> &diffList)
{
    m_diffList = diffList;

    QList<Diff> transformedDiffList = m_diffList;

    m_originalChunkData = calculateOriginalData(transformedDiffList);
    m_contextFileData = calculateContextData(m_originalChunkData);
    showDiff();
}

void DiffEditorWidget::setContextLinesNumber(int lines)
{
    if (m_contextLinesNumber == lines)
        return;

    m_contextLinesNumber = lines;
    m_contextFileData = calculateContextData(m_originalChunkData);
    showDiff();
}

void DiffEditorWidget::setIgnoreWhitespaces(bool ignore)
{
    if (m_ignoreWhitespaces == ignore)
        return;

    m_ignoreWhitespaces = ignore;
    setDiff(m_diffList);
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
            lastRightLineEqual = lastRightLineEqual ? equal : false;
        } else if (diff.command == Diff::Delete) {
            lastLeftLineEqual = lastLeftLineEqual ? equal : false;
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
                    lastLeftLineEqual = line.count() ? equal : true;
                }
                if (diff.command != Diff::Delete) {
                    currentRightLine++;
                    currentRightLineOffset++;
                    rightLines.append(QString());
                    currentRightPos++;
                    lastRightLineEqual = line.count() ? equal : true;
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
//                        currentLeftPos += spans;
                    } else if (currentRightLineOffset < currentLeftLineOffset) {
                        const int spans = currentLeftLineOffset - currentRightLineOffset;
                        rightLineSpans[currentRightLine] = spans;
//                        currentRightPos += spans;
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
                //                        currentLeftPos += spans;
            } else if (currentRightLineOffset < currentLeftLineOffset) {
                const int spans = currentLeftLineOffset - currentRightLineOffset;
                rightLineSpans[currentRightLine] = spans;
                //                        currentRightPos += spans;
            }
        }
        if (lastLeftLineEqual && lastRightLineEqual) {
            leftEqualLines.append(currentLeftLine);
            rightEqualLines.append(currentRightLine);
        }
    }

    QList<TextLineData> leftData;
    int spanOffset = 0;
    int pos = 0;
    QMap<int, int>::ConstIterator leftChangedIt = leftChangedPositions.constBegin();
    for (int i = 0; i < leftLines.count(); i++) {
        for (int j = 0; j < leftLineSpans.value(i); j++) {
            leftData.append(TextLineData(TextLineData::Separator));
            spanOffset++;
        }
        const int textLength = leftLines.at(i).count() + 1;
        pos += textLength;
        leftData.append(leftLines.at(i));
        while (leftChangedIt != leftChangedPositions.constEnd()) {
            if (leftChangedIt.key() >= pos)
                break;

            const int startPos = leftChangedIt.key() + spanOffset;
            const int endPos = leftChangedIt.value() + spanOffset;
            chunkData.changedLeftPositions.insert(startPos, endPos);
            leftChangedIt++;
        }
    }
    while (leftChangedIt != leftChangedPositions.constEnd()) {
        if (leftChangedIt.key() >= pos)
            break;

        const int startPos = leftChangedIt.key() + spanOffset;
        const int endPos = leftChangedIt.value() + spanOffset;
        chunkData.changedLeftPositions.insert(startPos, endPos);
        leftChangedIt++;
    }

    QList<TextLineData> rightData;
    spanOffset = 0;
    pos = 0;
    QMap<int, int>::ConstIterator rightChangedIt = rightChangedPositions.constBegin();
    for (int i = 0; i < rightLines.count(); i++) {
        for (int j = 0; j < rightLineSpans.value(i); j++) {
            rightData.append(TextLineData(TextLineData::Separator));
            spanOffset++;
        }
        const int textLength = rightLines.at(i).count() + 1;
        pos += textLength;
        rightData.append(rightLines.at(i));
        while (rightChangedIt != rightChangedPositions.constEnd()) {
            if (rightChangedIt.key() >= pos)
                break;

            const int startPos = rightChangedIt.key() + spanOffset;
            const int endPos = rightChangedIt.value() + spanOffset;
            chunkData.changedRightPositions.insert(startPos, endPos);
            rightChangedIt++;
        }
    }
    while (rightChangedIt != rightChangedPositions.constEnd()) {
        if (rightChangedIt.key() >= pos)
            break;

        const int startPos = rightChangedIt.key() + spanOffset;
        const int endPos = rightChangedIt.value() + spanOffset;
        chunkData.changedRightPositions.insert(startPos, endPos);
        rightChangedIt++;
    }

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
            chunkData.alwaysShown = true;
            while (i < originalData.rows.count()) {
                if (hiddenRows.contains(i))
                    break;
                RowData rowData = originalData.rows.at(i);
                chunkData.rows.append(rowData);

                leftCharCounter += rowData.leftLine.text.count() + 1; // +1 for separator or for '\n', each line has one of it
                rightCharCounter += rowData.rightLine.text.count() + 1; // +1 for separator or for '\n', each line has one of it
                i++;
            }
            while (leftChangedIt != originalData.changedLeftPositions.constEnd()) {
                if (leftChangedIt.key() < leftOffset
                        || leftChangedIt.key() > leftCharCounter)
                    break;

                const int startPos = leftChangedIt.key();
                const int endPos = leftChangedIt.value();
                chunkData.changedLeftPositions.insert(startPos, endPos);
                leftChangedIt++;
            }
            while (rightChangedIt != originalData.changedRightPositions.constEnd()) {
                if (rightChangedIt.key() < rightOffset
                        || rightChangedIt.key() > rightCharCounter)
                    break;

                const int startPos = rightChangedIt.key();
                const int endPos = rightChangedIt.value();
                chunkData.changedRightPositions.insert(startPos, endPos);
                rightChangedIt++;
            }
            fileData.chunks.append(chunkData);
        } else {
            ChunkData chunkData;
            chunkData.alwaysShown = false;
            while (i < originalData.rows.count()) {
                if (!hiddenRows.contains(i))
                    break;
                RowData rowData = originalData.rows.at(i);
                chunkData.rows.append(rowData);

                leftCharCounter += rowData.leftLine.text.count() + 1; // +1 for separator or for '\n', each line has one of it
                rightCharCounter += rowData.rightLine.text.count() + 1; // +1 for separator or for '\n', each line has one of it
                i++;
            }
            fileData.chunks.append(chunkData);
        }
    }
    return fileData;
}

void DiffEditorWidget::showDiff()
{
//    QTime time;
//    time.start();

    // TODO: remember the line number of the line in the middle
    const int verticalValue = m_leftEditor->verticalScrollBar()->value();
    const int leftHorizontalValue = m_leftEditor->horizontalScrollBar()->value();
    const int rightHorizontalValue = m_rightEditor->horizontalScrollBar()->value();

    m_leftEditor->clear();
    m_rightEditor->clear();
    m_leftEditor->clearLineNumbers();
    m_rightEditor->clearLineNumbers();
    m_leftEditor->clearSkippedLines();
    m_rightEditor->clearSkippedLines();
    m_leftEditor->clearSeparators();
    m_rightEditor->clearSeparators();
//    int ela1 = time.elapsed();

    QString leftText, rightText;
    int leftLineNumber = 0;
    int rightLineNumber = 0;
    int blockNumber = 0;
    QChar separator = QLatin1Char('\n');
    for (int i = 0; i < m_contextFileData.chunks.count(); i++) {
        ChunkData chunkData = m_contextFileData.chunks.at(i);
        if (!chunkData.alwaysShown) {
            const int skippedLines = chunkData.rows.count();
//            leftLineNumber += skippedLines;
//            rightLineNumber += skippedLines;
            m_leftEditor->setSkippedLines(blockNumber, skippedLines);
            m_rightEditor->setSkippedLines(blockNumber, skippedLines);
            m_leftEditor->setSeparator(blockNumber, true);
            m_rightEditor->setSeparator(blockNumber, true);
            leftText += separator;
            rightText += separator;
            blockNumber++;
        }

        for (int j = 0; j < chunkData.rows.count(); j++) {
            RowData rowData = chunkData.rows.at(j);
            TextLineData leftLineData = rowData.leftLine;
            TextLineData rightLineData = rowData.rightLine;
            if (leftLineData.textLineType == TextLineData::TextLine) {
                leftText += leftLineData.text;
                leftLineNumber++;
                m_leftEditor->setLineNumber(blockNumber, QString::number(leftLineNumber));
            } else if (leftLineData.textLineType == TextLineData::Separator) {
                m_leftEditor->setSeparator(blockNumber, true);
            }

            if (rightLineData.textLineType == TextLineData::TextLine) {
                rightText += rightLineData.text;
                rightLineNumber++;
                m_rightEditor->setLineNumber(blockNumber, QString::number(rightLineNumber));
            } else if (rightLineData.textLineType == TextLineData::Separator) {
                m_rightEditor->setSeparator(blockNumber, true);
            }
            leftText += separator;
            rightText += separator;
            blockNumber++;
        }
    }
//    int ela2 = time.elapsed();

    m_leftEditor->setPlainText(leftText);
    m_rightEditor->setPlainText(rightText);
//    int ela3 = time.elapsed();

//    int ela4 = time.elapsed();

    colorDiff(m_contextFileData);

    blockNumber = 0;
    for (int i = 0; i < m_contextFileData.chunks.count(); i++) {
        ChunkData chunkData = m_contextFileData.chunks.at(i);
        if (!chunkData.alwaysShown) {
            blockNumber++;
            QTextBlock leftBlock = m_leftEditor->document()->findBlockByNumber(blockNumber);
            for (int j = 0; j < chunkData.rows.count(); j++) {
                TextEditor::BaseTextDocumentLayout::setFoldingIndent(leftBlock, 1);
                leftBlock = leftBlock.next();
            }
            QTextBlock rightBlock = m_rightEditor->document()->findBlockByNumber(blockNumber);
            for (int j = 0; j < chunkData.rows.count(); j++) {
                TextEditor::BaseTextDocumentLayout::setFoldingIndent(rightBlock, 1);
                rightBlock = rightBlock.next();
            }
        }
        blockNumber += chunkData.rows.count();
    }
    blockNumber = 0;
    for (int i = 0; i < m_contextFileData.chunks.count(); i++) {
        ChunkData chunkData = m_contextFileData.chunks.at(i);
        if (!chunkData.alwaysShown) {
            QTextBlock leftBlock = m_leftEditor->document()->findBlockByNumber(blockNumber);
            TextEditor::BaseTextDocumentLayout::doFoldOrUnfold(leftBlock, false);
            QTextBlock rightBlock = m_rightEditor->document()->findBlockByNumber(blockNumber);
            TextEditor::BaseTextDocumentLayout::doFoldOrUnfold(rightBlock, false);
            blockNumber++;
        }
        blockNumber += chunkData.rows.count();
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

//    int ela5 = time.elapsed();

    m_leftEditor->verticalScrollBar()->setValue(verticalValue);
    m_rightEditor->verticalScrollBar()->setValue(verticalValue);
    m_leftEditor->horizontalScrollBar()->setValue(leftHorizontalValue);
    m_rightEditor->horizontalScrollBar()->setValue(rightHorizontalValue);
//    int ela6 = time.elapsed();
//    qDebug() << ela1 << ela2 << ela3 << ela4 << ela5 << ela6;
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

void DiffEditorWidget::colorDiff(const FileData &fileData)
{
    QTextCharFormat leftLineFormat;
    leftLineFormat.setBackground(QColor(255, 223, 223));
    leftLineFormat.setProperty(QTextFormat::FullWidthSelection, true);

    QTextCharFormat leftCharFormat;
    leftCharFormat.setBackground(QColor(255, 175, 175));
    leftCharFormat.setProperty(QTextFormat::FullWidthSelection, true);

    QTextCharFormat rightLineFormat;
    rightLineFormat.setBackground(QColor(223, 255, 223));
    rightLineFormat.setProperty(QTextFormat::FullWidthSelection, true);

    QTextCharFormat rightCharFormat;
    rightCharFormat.setBackground(QColor(175, 255, 175));
    rightCharFormat.setProperty(QTextFormat::FullWidthSelection, true);

    QPalette pal = m_leftEditor->extraArea()->palette();
    pal.setCurrentColorGroup(QPalette::Active);
    QTextCharFormat spanLineFormat;
    spanLineFormat.setBackground(pal.color(QPalette::Background));
    spanLineFormat.setProperty(QTextFormat::FullWidthSelection, true);

    QTextCharFormat chunkLineFormat;
    chunkLineFormat.setBackground(QColor(175, 215, 231));
    chunkLineFormat.setProperty(QTextFormat::FullWidthSelection, true);

    int leftPos = 0;
    int rightPos = 0;
    // startPos, endPos
    QMap<int, int> leftLinePos;
    QMap<int, int> rightLinePos;
    QMap<int, int> leftCharPos;
    QMap<int, int> rightCharPos;
    QMap<int, int> leftSkippedPos;
    QMap<int, int> rightSkippedPos;
    QMap<int, int> leftChunkPos;
    QMap<int, int> rightChunkPos;
    int leftLastDiffBlockStartPos = 0;
    int rightLastDiffBlockStartPos = 0;
    int leftLastSkippedBlockStartPos = 0;
    int rightLastSkippedBlockStartPos = 0;
    int chunkOffset = 0;

    for (int i = 0; i < fileData.chunks.count(); i++) {
        ChunkData chunkData = fileData.chunks.at(i);
        if (!chunkData.alwaysShown) {
            leftChunkPos[leftPos] = leftPos + 1;
            rightChunkPos[rightPos] = rightPos + 1;
            leftPos++; // for chunk line
            rightPos++; // for chunk line
            chunkOffset++;
        }
        leftLastDiffBlockStartPos = leftPos;
        rightLastDiffBlockStartPos = rightPos;
        leftLastSkippedBlockStartPos = leftPos;
        rightLastSkippedBlockStartPos = rightPos;

        QMapIterator<int, int> itLeft(chunkData.changedLeftPositions);
        while (itLeft.hasNext()) {
            itLeft.next();

            leftCharPos[itLeft.key() + chunkOffset] = itLeft.value() + chunkOffset;
        }

        QMapIterator<int, int> itRight(chunkData.changedRightPositions);
        while (itRight.hasNext()) {
            itRight.next();

            rightCharPos[itRight.key() + chunkOffset] = itRight.value() + chunkOffset;
        }

        for (int j = 0; j < chunkData.rows.count(); j++) {
            RowData rowData = chunkData.rows.at(j);

            leftPos += rowData.leftLine.text.count() + 1; // +1 for separator or for '\n', each line has one of it
            rightPos += rowData.rightLine.text.count() + 1; // +1 for separator or for '\n', each line has one of it

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

    QTextCursor leftCursor = m_leftEditor->textCursor();
    QTextCursor rightCursor = m_rightEditor->textCursor();

    QList<QTextEdit::ExtraSelection> leftSelections
            = colorPositions(leftLineFormat, leftCursor, leftLinePos);
    leftSelections
            += colorPositions(spanLineFormat, leftCursor, leftSkippedPos);
    leftSelections
            += colorPositions(chunkLineFormat, leftCursor, leftChunkPos);
    leftSelections
            += colorPositions(leftCharFormat, leftCursor, leftCharPos);

    QList<QTextEdit::ExtraSelection> rightSelections
            = colorPositions(rightLineFormat, rightCursor, rightLinePos);
    rightSelections
            += colorPositions(spanLineFormat, rightCursor, rightSkippedPos);
    rightSelections
            += colorPositions(chunkLineFormat, rightCursor, rightChunkPos);
    rightSelections
            += colorPositions(rightCharFormat, rightCursor, rightCharPos);

    m_leftEditor->setExtraSelections(BaseTextEditorWidget::OtherSelection,
                                      m_leftEditor->extraSelections(BaseTextEditorWidget::OtherSelection)
                                      + leftSelections);
    m_rightEditor->setExtraSelections(BaseTextEditorWidget::OtherSelection,
                                      m_rightEditor->extraSelections(BaseTextEditorWidget::OtherSelection)
                                      + rightSelections);
}

void DiffEditorWidget::leftSliderChanged()
{
    m_rightEditor->verticalScrollBar()->setValue(m_leftEditor->verticalScrollBar()->value());
}

void DiffEditorWidget::rightSliderChanged()
{
    m_leftEditor->verticalScrollBar()->setValue(m_rightEditor->verticalScrollBar()->value());
}

void DiffEditorWidget::leftDocumentSizeChanged()
{
    synchronizeFoldings(m_leftEditor, m_rightEditor);
}

void DiffEditorWidget::rightDocumentSizeChanged()
{
    synchronizeFoldings(m_rightEditor, m_leftEditor);
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
                if (isSourceFolded) { // we fold the destination
                    QTextBlock previousSource = sourceBlock.previous(); // skippedLines
                    QTextBlock previousDestination = destinationBlock.previous(); // skippedLines
                    QTextBlock firstVisibleDestinationBlock = destination->firstVisibleBlock();
                    QTextBlock firstDestinationBlock = destination->document()->firstBlock();
                    TextEditor::BaseTextDocumentLayout::doFoldOrUnfold(destinationBlock, !isSourceFolded);
                    TextEditor::BaseTextDocumentLayout::setFoldingIndent(sourceBlock, 1);
                    TextEditor::BaseTextDocumentLayout::setFoldingIndent(destinationBlock, 1);
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
                } else { // we unfold the destination
                    QTextBlock nextSource = sourceBlock.next();
                    QTextBlock nextDestination = destinationBlock.next();
                    TextEditor::BaseTextDocumentLayout::doFoldOrUnfold(destinationBlock, !isSourceFolded);
                    TextEditor::BaseTextDocumentLayout::setFoldingIndent(nextSource, 0);
                    TextEditor::BaseTextDocumentLayout::setFoldingIndent(nextDestination, 0);
                    sourceBlock.setVisible(false);
                    sourceBlock.setLineCount(0);
                    destinationBlock.setVisible(false);
                    destinationBlock.setLineCount(0);
                    TextEditor::BaseTextDocumentLayout::setFolded(nextSource, false);
                    TextEditor::BaseTextDocumentLayout::setFolded(nextDestination, false);
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
    source->updateFoldingHighlight(source->mapFromGlobal(QCursor::pos()));

    BaseTextDocumentLayout *destinationLayout = qobject_cast<BaseTextDocumentLayout *>(destination->document()->documentLayout());
    if (destinationLayout) {
        destinationLayout->requestUpdate();
        destinationLayout->emitDocumentSizeChanged();
    }
    m_foldingBlocker = false;
}


} // namespace DiffEditor

#include "diffeditorwidget.moc"
