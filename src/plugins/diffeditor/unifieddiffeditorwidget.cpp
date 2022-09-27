// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "unifieddiffeditorwidget.h"

#include "diffeditorconstants.h"
#include "diffeditordocument.h"
#include "diffeditorplugin.h"
#include "diffutils.h"

#include <QHash>
#include <QMenu>
#include <QPainter>
#include <QScrollBar>
#include <QTextBlock>

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <texteditor/textdocument.h>
#include <texteditor/textdocumentlayout.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/displaysettings.h>

#include <utils/qtcassert.h>
#include <utils/runextensions.h>
#include <utils/tooltip/tooltip.h>

using namespace Core;
using namespace TextEditor;
using namespace Utils;

namespace DiffEditor {
namespace Internal {

UnifiedDiffEditorWidget::UnifiedDiffEditorWidget(QWidget *parent)
    : SelectableTextEditorWidget("DiffEditor.UnifiedDiffEditor", parent)
    , m_controller(this)
{
    setReadOnly(true);

    DisplaySettings settings = displaySettings();
    settings.m_textWrapping = false;
    settings.m_displayLineNumbers = true;
    settings.m_markTextChanges = false;
    settings.m_highlightBlocks = false;
    SelectableTextEditorWidget::setDisplaySettings(settings);
    connect(TextEditorSettings::instance(), &TextEditorSettings::displaySettingsChanged,
            this, &UnifiedDiffEditorWidget::setDisplaySettings);
    setDisplaySettings(TextEditorSettings::displaySettings());

    setCodeStyle(TextEditorSettings::codeStyle());

    connect(TextEditorSettings::instance(), &TextEditorSettings::fontSettingsChanged,
            this, &UnifiedDiffEditorWidget::setFontSettings);
    setFontSettings(TextEditorSettings::fontSettings());

    clear(tr("No document"));

    connect(this, &QPlainTextEdit::cursorPositionChanged,
            this, &UnifiedDiffEditorWidget::slotCursorPositionChangedInEditor);

    auto context = new Core::IContext(this);
    context->setWidget(this);
    context->setContext(Core::Context(Constants::UNIFIED_VIEW_ID));
    Core::ICore::addContextObject(context);
    setCodeFoldingSupported(true);
}

UnifiedDiffEditorWidget::~UnifiedDiffEditorWidget()
{
    if (m_watcher) {
        m_watcher->cancel();
        DiffEditorPlugin::addFuture(m_watcher->future());
    }
}

void UnifiedDiffEditorWidget::setDocument(DiffEditorDocument *document)
{
    m_controller.setBusyShowing(true);
    m_controller.setDocument(document);
    clear();
    setDiff(document ? document->diffFiles() : QList<FileData>());
}

DiffEditorDocument *UnifiedDiffEditorWidget::diffDocument() const
{
    return m_controller.document();
}

void UnifiedDiffEditorWidget::saveState()
{
    if (!m_state.isNull())
        return;

    m_state = SelectableTextEditorWidget::saveState();
}

void UnifiedDiffEditorWidget::restoreState()
{
    if (m_state.isNull())
        return;

    const GuardLocker locker(m_controller.m_ignoreChanges);
    SelectableTextEditorWidget::restoreState(m_state);
    m_state.clear();
}

void UnifiedDiffEditorWidget::setDisplaySettings(const DisplaySettings &ds)
{
    DisplaySettings settings = displaySettings();
    settings.m_visualizeWhitespace = ds.m_visualizeWhitespace;
    settings.m_displayFoldingMarkers = ds.m_displayFoldingMarkers;
    settings.m_scrollBarHighlights = ds.m_scrollBarHighlights;
    settings.m_highlightCurrentLine = ds.m_highlightCurrentLine;
    SelectableTextEditorWidget::setDisplaySettings(settings);
}

void UnifiedDiffEditorWidget::setFontSettings(const FontSettings &fontSettings)
{
    m_controller.setFontSettings(fontSettings);
}

void UnifiedDiffEditorWidget::slotCursorPositionChangedInEditor()
{
    const int fileIndex = fileIndexForBlockNumber(textCursor().blockNumber());
    if (fileIndex < 0)
        return;

    if (m_controller.m_ignoreChanges.isLocked())
        return;

    const GuardLocker locker(m_controller.m_ignoreChanges);
    emit currentDiffFileIndexChanged(fileIndex);
}

void UnifiedDiffEditorWidget::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton && !(e->modifiers() & Qt::ShiftModifier)) {
        QTextCursor cursor = cursorForPosition(e->pos());
        jumpToOriginalFile(cursor);
        e->accept();
        return;
    }
    SelectableTextEditorWidget::mouseDoubleClickEvent(e);
}

void UnifiedDiffEditorWidget::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return) {
        jumpToOriginalFile(textCursor());
        e->accept();
        return;
    }
    SelectableTextEditorWidget::keyPressEvent(e);
}

void UnifiedDiffEditorWidget::contextMenuEvent(QContextMenuEvent *e)
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

    const int fileIndex = fileIndexForBlockNumber(blockNumber);
    const int chunkIndex = chunkIndexForBlockNumber(blockNumber);

    const ChunkData chunkData = m_controller.chunkData(fileIndex, chunkIndex);

    QList<int> leftSelection, rightSelection;

    for (int i = startBlockNumber; i <= endBlockNumber; ++i) {
        const int currentFileIndex = fileIndexForBlockNumber(i);
        if (currentFileIndex < fileIndex)
            continue;

        if (currentFileIndex > fileIndex)
            break;

        const int currentChunkIndex = chunkIndexForBlockNumber(i);
        if (currentChunkIndex < chunkIndex)
            continue;

        if (currentChunkIndex > chunkIndex)
            break;

        const int leftRow = m_data.m_lineNumbers[LeftSide].value(i, qMakePair(-1, -1)).second;
        const int rightRow = m_data.m_lineNumbers[RightSide].value(i, qMakePair(-1, -1)).second;

        if (leftRow >= 0)
            leftSelection.append(leftRow);
        if (rightRow >= 0)
            rightSelection.append(rightRow);
    }

    const ChunkSelection selection(leftSelection, rightSelection);

    addContextMenuActions(menu, fileIndexForBlockNumber(blockNumber),
                          chunkIndexForBlockNumber(blockNumber), selection);

    connect(this, &UnifiedDiffEditorWidget::destroyed, menu.data(), &QMenu::deleteLater);
    menu->exec(e->globalPos());
    delete menu;
}

void UnifiedDiffEditorWidget::addContextMenuActions(QMenu *menu,
                                                    int fileIndex,
                                                    int chunkIndex,
                                                    const ChunkSelection &selection)
{
    menu->addSeparator();

    m_controller.addCodePasterAction(menu, fileIndex, chunkIndex);
    m_controller.addApplyAction(menu, fileIndex, chunkIndex);
    m_controller.addRevertAction(menu, fileIndex, chunkIndex);
    m_controller.addExtraActions(menu, fileIndex, chunkIndex, selection);
}

void UnifiedDiffEditorWidget::clear(const QString &message)
{
    m_data = {};
    setSelections({});
    if (m_watcher) {
        m_watcher->cancel();
        DiffEditorPlugin::addFuture(m_watcher->future());
        m_watcher.reset();
        m_controller.setBusyShowing(false);
    }

    const GuardLocker locker(m_controller.m_ignoreChanges);
    SelectableTextEditorWidget::clear();
    m_controller.m_contextFileData.clear();
    setPlainText(message);
}

QString UnifiedDiffEditorWidget::lineNumber(int blockNumber) const
{
    QString lineNumberString;

    const bool leftLineExists = m_data.m_lineNumbers[LeftSide].contains(blockNumber);
    const bool rightLineExists = m_data.m_lineNumbers[RightSide].contains(blockNumber);

    if (leftLineExists || rightLineExists) {
        const QString leftLine = leftLineExists
                ? QString::number(m_data.m_lineNumbers[LeftSide].value(blockNumber).first)
                : QString();
        lineNumberString += QString(m_data.m_lineNumberDigits[LeftSide] - leftLine.count(),
                                    ' ') + leftLine;

        lineNumberString += '|';

        const QString rightLine = rightLineExists
                ? QString::number(m_data.m_lineNumbers[RightSide].value(blockNumber).first)
                : QString();
        lineNumberString += QString(m_data.m_lineNumberDigits[RightSide] - rightLine.count(),
                                    ' ') + rightLine;
    }
    return lineNumberString;
}

int UnifiedDiffEditorWidget::lineNumberDigits() const
{
    return m_data.m_lineNumberDigits[LeftSide] + m_data.m_lineNumberDigits[RightSide] + 1;
}

void UnifiedDiffData::setLineNumber(DiffSide side, int blockNumber, int lineNumber, int rowNumberInChunk)
{
    QTC_ASSERT(side < SideCount, return);
    const QString lineNumberString = QString::number(lineNumber);
    m_lineNumbers[side].insert(blockNumber, qMakePair(lineNumber, rowNumberInChunk));
    m_lineNumberDigits[side] = qMax(m_lineNumberDigits[side], lineNumberString.count());
}

void UnifiedDiffData::setFileInfo(int blockNumber, const DiffFileInfo &leftInfo,
                                                   const DiffFileInfo &rightInfo)
{
    m_fileInfo[blockNumber] = qMakePair(leftInfo, rightInfo);
}

void UnifiedDiffData::setChunkIndex(int startBlockNumber, int blockCount, int chunkIndex)
{
    m_chunkInfo.insert(startBlockNumber, qMakePair(blockCount, chunkIndex));
}

void UnifiedDiffEditorWidget::setDiff(const QList<FileData> &diffFileList)
{
    const GuardLocker locker(m_controller.m_ignoreChanges);
    clear(tr("Waiting for data..."));
    m_controller.m_contextFileData = diffFileList;
    showDiff();
}

QString UnifiedDiffData::setChunk(const DiffEditorInput &input, const ChunkData &chunkData,
                                  bool lastChunk, int *blockNumber, int *charNumber,
                                  DiffSelections *selections)
{
    if (chunkData.contextChunk)
        return {};

    QString diffText;
    int leftLineCount = 0;
    int rightLineCount = 0;
    int blockCount = 0;
    int charCount = 0;
    QList<TextLineData> leftBuffer, rightBuffer;
    QList<int> leftRowsBuffer, rightRowsBuffer;

    (*selections)[*blockNumber].append(DiffSelection(input.m_chunkLineFormat));

    int lastEqualRow = -1;
    if (lastChunk) {
        for (int i = chunkData.rows.count(); i > 0; i--) {
            if (chunkData.rows.at(i - 1).equal) {
                if (i != chunkData.rows.count())
                    lastEqualRow = i - 1;
                break;
            }
        }
    }

    for (int i = 0; i <= chunkData.rows.count(); i++) {
        const RowData &rowData = i < chunkData.rows.count()
                ? chunkData.rows.at(i)
                : RowData(TextLineData(TextLineData::Separator)); // dummy,
                                       // ensure we process buffers to the end.
                                       // rowData will be equal
        if (rowData.equal && i != lastEqualRow) {
            if (!leftBuffer.isEmpty()) {
                for (int j = 0; j < leftBuffer.count(); j++) {
                    const TextLineData &lineData = leftBuffer.at(j);
                    const QString line = DiffUtils::makePatchLine(
                                '-',
                                lineData.text,
                                lastChunk,
                                i == chunkData.rows.count() && j == leftBuffer.count() - 1);

                    const int blockDelta = line.count('\n'); // no new line
                                                     // could have been added
                    for (int k = 0; k < blockDelta; k++)
                        (*selections)[*blockNumber + blockCount + 1 + k].append(input.m_leftLineFormat);

                    for (auto it = lineData.changedPositions.cbegin(),
                              end = lineData.changedPositions.cend(); it != end; ++it) {
                        const int startPos = it.key() < 0 ? 1 : it.key() + 1;
                        const int endPos = it.value() < 0 ? it.value() : it.value() + 1;
                        (*selections)[*blockNumber + blockCount + 1].append(
                                    DiffSelection(startPos, endPos, input.m_leftCharFormat));
                    }

                    if (!line.isEmpty()) {
                        setLineNumber(LeftSide,
                                      *blockNumber + blockCount + 1,
                                      chunkData.startingLineNumber[LeftSide] + leftLineCount + 1,
                                      leftRowsBuffer.at(j));
                        blockCount += blockDelta;
                        ++leftLineCount;
                    }

                    diffText += line;

                    charCount += line.count();
                }
                leftBuffer.clear();
                leftRowsBuffer.clear();
            }
            if (!rightBuffer.isEmpty()) {
                for (int j = 0; j < rightBuffer.count(); j++) {
                    const TextLineData &lineData = rightBuffer.at(j);
                    const QString line = DiffUtils::makePatchLine(
                                '+',
                                lineData.text,
                                lastChunk,
                                i == chunkData.rows.count() && j == rightBuffer.count() - 1);

                    const int blockDelta = line.count('\n'); // no new line
                                                     // could have been added

                    for (int k = 0; k < blockDelta; k++)
                        (*selections)[*blockNumber + blockCount + 1 + k].append(input.m_rightLineFormat);

                    for (auto it = lineData.changedPositions.cbegin(),
                              end = lineData.changedPositions.cend(); it != end; ++it) {
                        const int startPos = it.key() < 0 ? 1 : it.key() + 1;
                        const int endPos = it.value() < 0 ? it.value() : it.value() + 1;
                        (*selections)[*blockNumber + blockCount + 1].append
                                (DiffSelection(startPos, endPos, input.m_rightCharFormat));
                    }

                    if (!line.isEmpty()) {
                        setLineNumber(RightSide,
                                      *blockNumber + blockCount + 1,
                                      chunkData.startingLineNumber[RightSide] + rightLineCount + 1,
                                      rightRowsBuffer.at(j));
                        blockCount += blockDelta;
                        ++rightLineCount;
                    }

                    diffText += line;

                    charCount += line.count();
                }
                rightBuffer.clear();
                rightRowsBuffer.clear();
            }
            if (i < chunkData.rows.count()) {
                const QString line = DiffUtils::makePatchLine(' ',
                                          rowData.line[RightSide].text,
                                          lastChunk,
                                          i == chunkData.rows.count() - 1);

                if (!line.isEmpty()) {
                    setLineNumber(LeftSide, *blockNumber + blockCount + 1,
                                  chunkData.startingLineNumber[LeftSide] + leftLineCount + 1, i);
                    setLineNumber(RightSide, *blockNumber + blockCount + 1,
                                  chunkData.startingLineNumber[RightSide] + rightLineCount + 1, i);
                    blockCount += line.count('\n');
                    ++leftLineCount;
                    ++rightLineCount;
                }

                diffText += line;

                charCount += line.count();
            }
        } else {
            if (rowData.line[LeftSide].textLineType == TextLineData::TextLine) {
                leftBuffer.append(rowData.line[LeftSide]);
                leftRowsBuffer.append(i);
            }
            if (rowData.line[RightSide].textLineType == TextLineData::TextLine) {
                rightBuffer.append(rowData.line[RightSide]);
                rightRowsBuffer.append(i);
            }
        }
    }

    const QString chunkLine = "@@ -"
            + QString::number(chunkData.startingLineNumber[LeftSide] + 1)
            + ','
            + QString::number(leftLineCount)
            + " +"
            + QString::number(chunkData.startingLineNumber[RightSide]+ 1)
            + ','
            + QString::number(rightLineCount)
            + " @@"
            + chunkData.contextInfo
            + '\n';

    diffText.prepend(chunkLine);

    *blockNumber += blockCount + 1; // +1 for chunk line
    *charNumber += charCount + chunkLine.count();
    return diffText;
}

static int interpolate(int x, int x1, int x2, int y1, int y2)
{
    if (x1 == x2)
        return x1;
    if (x == x1)
        return y1;
    if (x == x2)
        return y2;
    const int numerator = (y2 - y1) * x + x2 * y1 - x1 * y2;
    const int denominator = x2 - x1;
    return qRound((double)numerator / denominator);
}

UnifiedDiffOutput UnifiedDiffData::setDiff(QFutureInterface<void> &fi, int progressMin,
                                           int progressMax, const DiffEditorInput &input)
{
    UnifiedDiffOutput output;

    int blockNumber = 0;
    int charNumber = 0;
    int i = 0;
    const int count = input.m_contextFileData.size();

    for (const FileData &fileData : qAsConst(input.m_contextFileData)) {
        const QString leftFileInfo = "--- " + fileData.fileInfo[LeftSide].fileName + '\n';
        const QString rightFileInfo = "+++ " + fileData.fileInfo[RightSide].fileName + '\n';
        setFileInfo(blockNumber, fileData.fileInfo[LeftSide], fileData.fileInfo[RightSide]);
        output.foldingIndent.insert(blockNumber, 1);
        output.selections[blockNumber].append(DiffSelection(input.m_fileLineFormat));
        blockNumber++;
        output.foldingIndent.insert(blockNumber, 1);
        output.selections[blockNumber].append(DiffSelection(input.m_fileLineFormat));
        blockNumber++;

        output.diffText += leftFileInfo;
        output.diffText += rightFileInfo;
        charNumber += leftFileInfo.count() + rightFileInfo.count();

        if (fileData.binaryFiles) {
            output.foldingIndent.insert(blockNumber, 2);
            output.selections[blockNumber].append(DiffSelection(input.m_chunkLineFormat));
            blockNumber++;
            const QString binaryLine = "Binary files "
                    + fileData.fileInfo[LeftSide].fileName
                    + " and "
                    + fileData.fileInfo[RightSide].fileName
                    + " differ\n";
            output.diffText += binaryLine;
            charNumber += binaryLine.count();
        } else {
            for (int j = 0; j < fileData.chunks.count(); j++) {
                const int oldBlockNumber = blockNumber;
                output.foldingIndent.insert(blockNumber, 2);
                output.diffText += setChunk(input, fileData.chunks.at(j),
                                            (j == fileData.chunks.count() - 1)
                                            && fileData.lastChunkAtTheEndOfFile,
                                            &blockNumber,
                                            &charNumber,
                                            &output.selections);
                if (!fileData.chunks.at(j).contextChunk)
                    setChunkIndex(oldBlockNumber, blockNumber - oldBlockNumber, j);
            }
        }
        fi.setProgressValue(interpolate(++i, 0, count, progressMin, progressMax));
        if (fi.isCanceled())
            return {};
    }

    output.diffText.replace('\r', ' ');
    output.selections = SelectableTextEditorWidget::polishedSelections(output.selections);
    return output;
}

void UnifiedDiffEditorWidget::showDiff()
{
    if (m_controller.m_contextFileData.isEmpty()) {
        setPlainText(tr("No difference."));
        return;
    }

    m_watcher.reset(new QFutureWatcher<ShowResult>());
    m_controller.setBusyShowing(true);
    connect(m_watcher.get(), &QFutureWatcherBase::finished, this, [this] {
        if (m_watcher->isCanceled()) {
            setPlainText(tr("Retrieving data failed."));
        } else {
            const ShowResult result = m_watcher->result();
            m_data = result.diffData;
            TextDocumentPtr doc(result.textDocument);
            {
                const GuardLocker locker(m_controller.m_ignoreChanges);
                // TextDocument was living in no thread, so it's safe to pull it
                doc->moveToThread(thread());
                setTextDocument(doc);

                setReadOnly(true);

                QTextBlock block = document()->firstBlock();
                for (int b = 0; block.isValid(); block = block.next(), ++b)
                    setFoldingIndent(block, result.foldingIndent.value(b, 3));
            }
            setSelections(result.selections);
        }
        m_watcher.release()->deleteLater();
        m_controller.setBusyShowing(false);
    });

    const DiffEditorInput input(&m_controller);

    auto getDocument = [=](QFutureInterface<ShowResult> &futureInterface) {
        auto cleanup = qScopeGuard([&futureInterface] {
            if (futureInterface.isCanceled())
                futureInterface.reportCanceled();
        });
        const int progressMax = 100;
        const int firstPartMax = 20; // showDiff is about 4 times quicker than filling document
        futureInterface.setProgressRange(0, progressMax);
        futureInterface.setProgressValue(0);
        QFutureInterface<void> fi = futureInterface;
        UnifiedDiffData diffData;
        const UnifiedDiffOutput output = diffData.setDiff(fi, 0, firstPartMax, input);
        if (futureInterface.isCanceled())
            return;

        const ShowResult result = {TextDocumentPtr(new TextDocument("DiffEditor.UnifiedDiffEditor")),
                                   diffData, output.foldingIndent, output.selections};
        // No need to store the change history
        result.textDocument->document()->setUndoRedoEnabled(false);

        // We could do just:
        //   result.textDocument->setPlainText(output.diffText);
        // but this would freeze the thread for couple of seconds without progress reporting
        // and without checking for canceled.
        const int diffSize = output.diffText.size();
        const int packageSize = 100000;
        int currentPos = 0;
        QTextCursor cursor(result.textDocument->document());
        while (currentPos < diffSize) {
            const QString package = output.diffText.mid(currentPos, packageSize);
            cursor.insertText(package);
            currentPos += package.size();
            fi.setProgressValue(interpolate(currentPos, 0, diffSize, firstPartMax, progressMax));
            if (futureInterface.isCanceled())
                return;
        }

        // If future was canceled, the destructor runs in this thread, so we can't move it
        // to caller's thread. We push it to no thread (make object to have no thread affinity),
        // and later, in the caller's thread, we pull it back to the caller's thread.
        result.textDocument->moveToThread(nullptr);
        futureInterface.reportResult(result);
    };

    m_watcher->setFuture(runAsync(getDocument));
    ProgressManager::addTask(m_watcher->future(), tr("Rendering diff"), "DiffEditor");
}

int UnifiedDiffEditorWidget::blockNumberForFileIndex(int fileIndex) const
{
    if (fileIndex < 0 || fileIndex >= m_data.m_fileInfo.count())
        return -1;

    return std::next(m_data.m_fileInfo.constBegin(), fileIndex).key();
}

int UnifiedDiffEditorWidget::fileIndexForBlockNumber(int blockNumber) const
{
    int i = -1;
    for (auto it = m_data.m_fileInfo.cbegin(), end = m_data.m_fileInfo.cend(); it != end; ++it, ++i) {
        if (it.key() > blockNumber)
            break;
    }

    return i;
}

int UnifiedDiffEditorWidget::chunkIndexForBlockNumber(int blockNumber) const
{
    if (m_data.m_chunkInfo.isEmpty())
        return -1;

    auto it = m_data.m_chunkInfo.upperBound(blockNumber);
    if (it == m_data.m_chunkInfo.constBegin())
        return -1;

    --it;

    if (blockNumber < it.key() + it.value().first)
        return it.value().second;

    return -1;
}

void UnifiedDiffEditorWidget::jumpToOriginalFile(const QTextCursor &cursor)
{
    if (m_data.m_fileInfo.isEmpty())
        return;

    const int blockNumber = cursor.blockNumber();
    const int fileIndex = fileIndexForBlockNumber(blockNumber);
    if (fileIndex < 0)
        return;

    const FileData fileData = m_controller.m_contextFileData.at(fileIndex);
    const QString leftFileName = fileData.fileInfo[LeftSide].fileName;
    const QString rightFileName = fileData.fileInfo[RightSide].fileName;

    const int columnNumber = cursor.positionInBlock() - 1; // -1 for the first character in line

    const int rightLineNumber = m_data.m_lineNumbers[RightSide].value(blockNumber, qMakePair(-1, 0)).first;
    if (rightLineNumber >= 0) {
        m_controller.jumpToOriginalFile(rightFileName, rightLineNumber, columnNumber);
        return;
    }

    const int leftLineNumber = m_data.m_lineNumbers[LeftSide].value(blockNumber, qMakePair(-1, 0)).first;
    if (leftLineNumber >= 0) {
        if (leftFileName == rightFileName) {
            for (const ChunkData &chunkData : fileData.chunks) {

                int newLeftLineNumber = chunkData.startingLineNumber[LeftSide];
                int newRightLineNumber = chunkData.startingLineNumber[RightSide];

                for (const RowData &rowData : chunkData.rows) {
                    if (rowData.line[LeftSide].textLineType == TextLineData::TextLine)
                        newLeftLineNumber++;
                    if (rowData.line[RightSide].textLineType == TextLineData::TextLine)
                        newRightLineNumber++;
                    if (newLeftLineNumber == leftLineNumber) {
                        m_controller.jumpToOriginalFile(leftFileName, newRightLineNumber, 0);
                        return;
                    }
                }
            }
        } else {
            m_controller.jumpToOriginalFile(leftFileName, leftLineNumber, columnNumber);
        }
        return;
    }
}

void UnifiedDiffEditorWidget::setCurrentDiffFileIndex(int diffFileIndex)
{
    if (m_controller.m_ignoreChanges.isLocked())
        return;

    const GuardLocker locker(m_controller.m_ignoreChanges);
    const int blockNumber = blockNumberForFileIndex(diffFileIndex);

    QTextBlock block = document()->findBlockByNumber(blockNumber);
    QTextCursor cursor = textCursor();
    cursor.setPosition(block.position());
    setTextCursor(cursor);
    verticalScrollBar()->setValue(blockNumber);
}

} // namespace Internal
} // namespace DiffEditor
