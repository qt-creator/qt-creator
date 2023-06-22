// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "unifieddiffeditorwidget.h"

#include "diffeditorconstants.h"
#include "diffeditordocument.h"
#include "diffeditortr.h"

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <extensionsystem/pluginmanager.h>

#include <texteditor/fontsettings.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditorsettings.h>

#include <utils/async.h>
#include <utils/mathutils.h>
#include <utils/qtcassert.h>

#include <QMenu>
#include <QScrollBar>
#include <QTextBlock>

using namespace Core;
using namespace TextEditor;
using namespace Utils;

namespace DiffEditor::Internal {

UnifiedDiffEditorWidget::UnifiedDiffEditorWidget(QWidget *parent)
    : SelectableTextEditorWidget("DiffEditor.UnifiedDiffEditor", parent)
    , m_controller(this)
{
    setVisualIndentOffset(1);

    connect(TextEditorSettings::instance(), &TextEditorSettings::fontSettingsChanged,
            this, &UnifiedDiffEditorWidget::setFontSettings);
    setFontSettings(TextEditorSettings::fontSettings());

    clear(Tr::tr("No document"));

    connect(this, &QPlainTextEdit::cursorPositionChanged,
            this, &UnifiedDiffEditorWidget::slotCursorPositionChangedInEditor);

    auto context = new IContext(this);
    context->setWidget(this);
    context->setContext(Context(Constants::UNIFIED_VIEW_ID));
    ICore::addContextObject(context);
}

UnifiedDiffEditorWidget::~UnifiedDiffEditorWidget() = default;

void UnifiedDiffEditorWidget::setDocument(DiffEditorDocument *document)
{
    m_controller.setDocument(document);
    clear();
    setDiff(document ? document->diffFiles() : QList<FileData>());
}

DiffEditorDocument *UnifiedDiffEditorWidget::diffDocument() const
{
    return m_controller.document();
}

void UnifiedDiffEditorWidget::setDiff(const QList<FileData> &diffFileList)
{
    const GuardLocker locker(m_controller.m_ignoreChanges);
    clear(Tr::tr("Waiting for data..."));
    m_controller.m_contextFileData = diffFileList;
    showDiff();
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

void UnifiedDiffEditorWidget::setFontSettings(const FontSettings &fontSettings)
{
    m_controller.setFontSettings(fontSettings);
}

void UnifiedDiffEditorWidget::slotCursorPositionChangedInEditor()
{
    if (m_controller.m_ignoreChanges.isLocked())
        return;

    const int fileIndex = m_data.fileIndexForBlockNumber(textCursor().blockNumber());
    if (fileIndex < 0)
        return;

    const GuardLocker locker(m_controller.m_ignoreChanges);
    m_controller.setCurrentDiffFileIndex(fileIndex);
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

    const int fileIndex = m_data.fileIndexForBlockNumber(blockNumber);
    const int chunkIndex = m_data.m_chunkInfo.chunkIndexForBlockNumber(blockNumber);

    const ChunkData chunkData = m_controller.chunkData(fileIndex, chunkIndex);

    QList<int> leftSelection, rightSelection;

    for (int i = startBlockNumber; i <= endBlockNumber; ++i) {
        const int currentFileIndex = m_data.fileIndexForBlockNumber(i);
        if (currentFileIndex < fileIndex)
            continue;

        if (currentFileIndex > fileIndex)
            break;

        const int currentChunkIndex = m_data.m_chunkInfo.chunkIndexForBlockNumber(i);
        if (currentChunkIndex < chunkIndex)
            continue;

        if (currentChunkIndex > chunkIndex)
            break;

        const int leftRow = m_data.m_lineNumbers[LeftSide].value(i, {-1, -1}).second;
        const int rightRow = m_data.m_lineNumbers[RightSide].value(i, {-1, -1}).second;

        if (leftRow >= 0)
            leftSelection.append(leftRow);
        if (rightRow >= 0)
            rightSelection.append(rightRow);
    }

    const ChunkSelection selection(leftSelection, rightSelection);

    addContextMenuActions(menu, m_data.fileIndexForBlockNumber(blockNumber),
                          m_data.m_chunkInfo.chunkIndexForBlockNumber(blockNumber), selection);

    connect(this, &UnifiedDiffEditorWidget::destroyed, menu.data(), &QMenu::deleteLater);
    menu->exec(e->globalPos());
    delete menu;
}

void UnifiedDiffEditorWidget::addContextMenuActions(QMenu *menu, int fileIndex, int chunkIndex,
                                                    const ChunkSelection &selection)
{
    menu->addSeparator();

    m_controller.addCodePasterAction(menu, fileIndex, chunkIndex);
    m_controller.addPatchAction(menu, fileIndex, chunkIndex, PatchAction::Apply);
    m_controller.addPatchAction(menu, fileIndex, chunkIndex, PatchAction::Revert);
    m_controller.addExtraActions(menu, fileIndex, chunkIndex, selection);
}

void UnifiedDiffEditorWidget::clear(const QString &message)
{
    m_data = {};
    setSelections({});
    if (m_asyncTask) {
        m_asyncTask.reset();
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
        auto addSideNumber = [&](DiffSide side, bool lineExists) {
            const QString line = lineExists
                    ? QString::number(m_data.m_lineNumbers[side].value(blockNumber).first)
                    : QString();
            lineNumberString += QString(m_data.m_lineNumberDigits[side] - line.size(), ' ') + line;
        };
        addSideNumber(LeftSide, leftLineExists);
        lineNumberString += '|';
        addSideNumber(RightSide, rightLineExists);
    }
    return lineNumberString;
}

int UnifiedDiffEditorWidget::lineNumberDigits() const
{
    return m_data.m_lineNumberDigits[LeftSide] + m_data.m_lineNumberDigits[RightSide] + 1;
}

int UnifiedDiffData::blockNumberForFileIndex(int fileIndex) const
{
    if (fileIndex < 0 || fileIndex >= m_fileInfo.count())
        return -1;

    return std::next(m_fileInfo.constBegin(), fileIndex).key();
}

int UnifiedDiffData::fileIndexForBlockNumber(int blockNumber) const
{
    int i = -1;
    for (auto it = m_fileInfo.cbegin(), end = m_fileInfo.cend(); it != end; ++it, ++i) {
        if (it.key() > blockNumber)
            break;
    }
    return i;
}

void UnifiedDiffData::setLineNumber(DiffSide side, int blockNumber, int lineNumber, int rowNumberInChunk)
{
    QTC_ASSERT(side < SideCount, return);
    const QString lineNumberString = QString::number(lineNumber);
    m_lineNumbers[side].insert(blockNumber, {lineNumber, rowNumberInChunk});
    m_lineNumberDigits[side] = qMax(m_lineNumberDigits[side], lineNumberString.size());
}

QString UnifiedDiffData::setChunk(const DiffEditorInput &input, const ChunkData &chunkData,
                                  bool lastChunk, int *blockNumber, DiffSelections *selections)
{
    if (chunkData.contextChunk)
        return {};

    QString diffText;
    int blockCount = 0;
    std::array<int, SideCount> lineCount{};
    std::array<QList<TextLineData>, SideCount> buffer{};
    std::array<QList<int>, SideCount> rowsBuffer{};

    (*selections)[*blockNumber].append({input.m_chunkLineFormat});

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

    auto processSideChunk = [&](DiffSide side, int chunkIndex) {
        if (buffer[side].isEmpty())
            return;

        for (int j = 0; j < buffer[side].count(); j++) {
            const TextLineData &lineData = buffer[side].at(j);
            const QString line = DiffUtils::makePatchLine(
                        side == LeftSide ? '-' : '+',
                        lineData.text,
                        lastChunk,
                        chunkIndex == chunkData.rows.count() && j == buffer[side].count() - 1);

            const int blockDelta = line.count('\n'); // no new line
            // could have been added
            for (int k = 0; k < blockDelta; k++)
                (*selections)[*blockNumber + blockCount + 1 + k].append({input.m_lineFormat[side]});

            for (auto it = lineData.changedPositions.cbegin(),
                 end = lineData.changedPositions.cend(); it != end; ++it) {
                const int startPos = it.key() < 0 ? 1 : it.key() + 1;
                const int endPos = it.value() < 0 ? it.value() : it.value() + 1;
                (*selections)[*blockNumber + blockCount + 1].append(
                            {input.m_charFormat[side], startPos, endPos});
            }

            if (!line.isEmpty()) {
                setLineNumber(side,
                              *blockNumber + blockCount + 1,
                              chunkData.startingLineNumber[side] + lineCount[side] + 1,
                              rowsBuffer[side].at(j));
                blockCount += blockDelta;
                ++lineCount[side];
            }
            diffText += line;
        }
        buffer[side].clear();
        rowsBuffer[side].clear();
    };

    auto processSideChunkEmpty = [&](DiffSide side, int chunkIndex) {
        setLineNumber(side, *blockNumber + blockCount + 1,
                      chunkData.startingLineNumber[side] + lineCount[side] + 1, chunkIndex);
        ++lineCount[side];
    };

    auto processSideChunkDifferent = [&](DiffSide side, int chunkIndex, const RowData &rowData) {
        if (rowData.line[side].textLineType == TextLineData::TextLine) {
            buffer[side].append(rowData.line[side]);
            rowsBuffer[side].append(chunkIndex);
        }
    };

    for (int i = 0; i <= chunkData.rows.count(); i++) {
        const RowData &rowData = i < chunkData.rows.count()
                ? chunkData.rows.at(i)
                  // dummy, ensure we process buffers to the end. rowData will be equal
                : RowData(TextLineData(TextLineData::Separator));


        if (rowData.equal && i != lastEqualRow) {
            processSideChunk(LeftSide, i);
            processSideChunk(RightSide, i);
            if (i < chunkData.rows.count()) {
                const QString line = DiffUtils::makePatchLine(' ',
                                          rowData.line[RightSide].text,
                                          lastChunk,
                                          i == chunkData.rows.count() - 1);
                if (!line.isEmpty()) {
                    processSideChunkEmpty(LeftSide, i);
                    processSideChunkEmpty(RightSide, i);
                    blockCount += line.count('\n');
                }
                diffText += line;
            }
        } else {
            processSideChunkDifferent(LeftSide, i, rowData);
            processSideChunkDifferent(RightSide, i, rowData);
        }
    }

    const QString chunkLine = "@@ -"
            + QString::number(chunkData.startingLineNumber[LeftSide] + 1)
            + ','
            + QString::number(lineCount[LeftSide])
            + " +"
            + QString::number(chunkData.startingLineNumber[RightSide]+ 1)
            + ','
            + QString::number(lineCount[RightSide])
            + " @@"
            + chunkData.contextInfo
            + '\n';

    diffText.prepend(chunkLine);

    *blockNumber += blockCount + 1; // +1 for chunk line
    return diffText;
}

static UnifiedDiffOutput diffOutput(QPromise<UnifiedShowResult> &promise, int progressMin,
                                    int progressMax, const DiffEditorInput &input)
{
    UnifiedDiffOutput output;

    int blockNumber = 0;
    int i = 0;
    const int count = input.m_contextFileData.size();

    for (const FileData &fileData : std::as_const(input.m_contextFileData)) {
        const QString leftFileInfo = "--- " + fileData.fileInfo[LeftSide].fileName + '\n';
        const QString rightFileInfo = "+++ " + fileData.fileInfo[RightSide].fileName + '\n';
        output.diffData.m_fileInfo[blockNumber] = fileData.fileInfo;
        output.foldingIndent.insert(blockNumber, 1);
        output.selections[blockNumber].append({input.m_fileLineFormat});
        blockNumber++;
        output.foldingIndent.insert(blockNumber, 1);
        output.selections[blockNumber].append({input.m_fileLineFormat});
        blockNumber++;

        output.diffText += leftFileInfo;
        output.diffText += rightFileInfo;

        if (fileData.binaryFiles) {
            output.foldingIndent.insert(blockNumber, 2);
            output.selections[blockNumber].append({input.m_chunkLineFormat});
            blockNumber++;
            const QString binaryLine = "Binary files "
                    + fileData.fileInfo[LeftSide].fileName
                    + " and "
                    + fileData.fileInfo[RightSide].fileName
                    + " differ\n";
            output.diffText += binaryLine;
        } else {
            for (int j = 0; j < fileData.chunks.count(); j++) {
                const int oldBlock = blockNumber;
                output.foldingIndent.insert(blockNumber, 2);
                output.diffText += output.diffData.setChunk(input, fileData.chunks.at(j),
                                                            (j == fileData.chunks.count() - 1)
                                                            && fileData.lastChunkAtTheEndOfFile,
                                                            &blockNumber,
                                                            &output.selections);
                if (!fileData.chunks.at(j).contextChunk)
                    output.diffData.m_chunkInfo.setChunkIndex(oldBlock, blockNumber - oldBlock, j);
            }
        }
        promise.setProgressValue(MathUtils::interpolateLinear(++i, 0, count, progressMin, progressMax));
        if (promise.isCanceled())
            return {};
    }

    output.diffText.replace('\r', ' ');
    output.selections = SelectableTextEditorWidget::polishedSelections(output.selections);
    return output;
}

void UnifiedDiffEditorWidget::showDiff()
{
    if (m_controller.m_contextFileData.isEmpty()) {
        setPlainText(Tr::tr("No difference."));
        return;
    }

    m_asyncTask.reset(new Async<UnifiedShowResult>());
    m_asyncTask->setFutureSynchronizer(ExtensionSystem::PluginManager::futureSynchronizer());
    m_controller.setBusyShowing(true);
    connect(m_asyncTask.get(), &AsyncBase::done, this, [this] {
        if (m_asyncTask->isCanceled() || !m_asyncTask->isResultAvailable()) {
            setPlainText(Tr::tr("Retrieving data failed."));
        } else {
            const UnifiedShowResult result = m_asyncTask->result();
            m_data = result.diffData;
            TextDocumentPtr doc(result.textDocument);
            {
                const GuardLocker locker(m_controller.m_ignoreChanges);
                // TextDocument was living in no thread, so it's safe to pull it
                doc->moveToThread(thread());
                setTextDocument(doc);

                setReadOnly(true);
            }
            setSelections(result.selections);
            setCurrentDiffFileIndex(m_controller.currentDiffFileIndex());
        }
        m_asyncTask.release()->deleteLater();
        m_controller.setBusyShowing(false);
    });

    const DiffEditorInput input(&m_controller);

    auto getDocument = [input](QPromise<UnifiedShowResult> &promise) {
        const int progressMax = 100;
        const int firstPartMax = 20; // diffOutput is about 4 times quicker than filling document
        promise.setProgressRange(0, progressMax);
        promise.setProgressValue(0);
        const UnifiedDiffOutput output = diffOutput(promise, 0, firstPartMax, input);
        if (promise.isCanceled())
            return;

        const UnifiedShowResult result = {TextDocumentPtr(new TextDocument("DiffEditor.UnifiedDiffEditor")),
                                   output.diffData, output.selections};
        // No need to store the change history
        result.textDocument->document()->setUndoRedoEnabled(false);

        // We could do just:
        //   result.textDocument->setPlainText(output.diffText);
        // but this would freeze the thread for couple of seconds without progress reporting
        // and without checking for canceled.
        const int diffSize = output.diffText.size();
        const int packageSize = 10000;
        int currentPos = 0;
        QTextCursor cursor(result.textDocument->document());
        while (currentPos < diffSize) {
            const QString package = output.diffText.mid(currentPos, packageSize);
            cursor.insertText(package);
            currentPos += package.size();
            promise.setProgressValue(MathUtils::interpolateLinear(currentPos, 0, diffSize,
                                                                  firstPartMax, progressMax));
            if (promise.isCanceled())
                return;
        }

        QTextBlock block = result.textDocument->document()->firstBlock();
        for (int b = 0; block.isValid(); block = block.next(), ++b)
            setFoldingIndent(block, output.foldingIndent.value(b, 3));

        // If future was canceled, the destructor runs in this thread, so we can't move it
        // to caller's thread. We push it to no thread (make object to have no thread affinity),
        // and later, in the caller's thread, we pull it back to the caller's thread.
        result.textDocument->moveToThread(nullptr);
        promise.addResult(result);
    };

    m_asyncTask->setConcurrentCallData(getDocument);
    m_asyncTask->start();
    ProgressManager::addTask(m_asyncTask->future(), Tr::tr("Rendering diff"), "DiffEditor");
}

void UnifiedDiffEditorWidget::jumpToOriginalFile(const QTextCursor &cursor)
{
    if (m_data.m_fileInfo.isEmpty())
        return;

    const int blockNumber = cursor.blockNumber();
    const int fileIndex = m_data.fileIndexForBlockNumber(blockNumber);
    if (fileIndex < 0)
        return;

    const FileData fileData = m_controller.m_contextFileData.at(fileIndex);
    const QString leftFileName = fileData.fileInfo[LeftSide].fileName;
    const QString rightFileName = fileData.fileInfo[RightSide].fileName;

    const int columnNumber = cursor.positionInBlock() - 1; // -1 for the first character in line

    const int rightLineNumber = m_data.m_lineNumbers[RightSide].value(blockNumber, {-1, 0}).first;
    if (rightLineNumber >= 0) {
        m_controller.jumpToOriginalFile(rightFileName, rightLineNumber, columnNumber);
        return;
    }

    const int leftLineNumber = m_data.m_lineNumbers[LeftSide].value(blockNumber, {-1, 0}).first;
    if (leftLineNumber < 0)
        return;
    if (leftFileName != rightFileName) {
        m_controller.jumpToOriginalFile(leftFileName, leftLineNumber, columnNumber);
        return;
    }

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
}

void UnifiedDiffEditorWidget::setCurrentDiffFileIndex(int diffFileIndex)
{
    if (m_controller.m_ignoreChanges.isLocked())
        return;

    const GuardLocker locker(m_controller.m_ignoreChanges);
    m_controller.setCurrentDiffFileIndex(diffFileIndex);
    const int blockNumber = m_data.blockNumberForFileIndex(diffFileIndex);

    QTextBlock block = document()->findBlockByNumber(blockNumber);
    QTextCursor cursor = textCursor();
    cursor.setPosition(block.position());
    setTextCursor(cursor);
    verticalScrollBar()->setValue(blockNumber);
}

} // namespace DiffEditor::Internal
