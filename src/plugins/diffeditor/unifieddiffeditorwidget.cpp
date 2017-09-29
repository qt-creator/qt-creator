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

#include "unifieddiffeditorwidget.h"

#include "diffeditorconstants.h"
#include "diffeditordocument.h"
#include "diffutils.h"

#include <QHash>
#include <QMenu>
#include <QPainter>
#include <QScrollBar>
#include <QTextBlock>

#include <coreplugin/icore.h>

#include <texteditor/textdocument.h>
#include <texteditor/textdocumentlayout.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/displaysettings.h>

#include <utils/tooltip/tooltip.h>

using namespace Core;
using namespace TextEditor;

namespace DiffEditor {
namespace Internal {

UnifiedDiffEditorWidget::UnifiedDiffEditorWidget(QWidget *parent)
    : SelectableTextEditorWidget("DiffEditor.UnifiedDiffEditor", parent)
    , m_controller(this)
{
    DisplaySettings settings = displaySettings();
    settings.m_textWrapping = false;
    settings.m_displayLineNumbers = true;
    settings.m_highlightCurrentLine = false;
    settings.m_markTextChanges = false;
    settings.m_highlightBlocks = false;
    SelectableTextEditorWidget::setDisplaySettings(settings);

    setReadOnly(true);
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

    m_context = new Core::IContext(this);
    m_context->setWidget(this);
    m_context->setContext(Core::Context(Constants::UNIFIED_VIEW_ID));
    Core::ICore::addContextObject(m_context);
    setCodeFoldingSupported(true);
}

UnifiedDiffEditorWidget::~UnifiedDiffEditorWidget()
{
    Core::ICore::removeContextObject(m_context);
}

void UnifiedDiffEditorWidget::setDocument(DiffEditorDocument *document)
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

    const bool oldIgnore = m_controller.m_ignoreCurrentIndexChange;
    m_controller.m_ignoreCurrentIndexChange = true;
    SelectableTextEditorWidget::restoreState(m_state);
    m_state.clear();
    m_controller.m_ignoreCurrentIndexChange = oldIgnore;
}

void UnifiedDiffEditorWidget::setDisplaySettings(const DisplaySettings &ds)
{
    DisplaySettings settings = displaySettings();
    settings.m_visualizeWhitespace = ds.m_visualizeWhitespace;
    settings.m_displayFoldingMarkers = ds.m_displayFoldingMarkers;
    SelectableTextEditorWidget::setDisplaySettings(settings);
}

void UnifiedDiffEditorWidget::setFontSettings(const FontSettings &fontSettings)
{
    m_controller.setFontSettings(fontSettings);
}

void UnifiedDiffEditorWidget::slotCursorPositionChangedInEditor()
{
    if (m_controller.m_ignoreCurrentIndexChange)
        return;

    const bool oldIgnore = m_controller.m_ignoreCurrentIndexChange;
    m_controller.m_ignoreCurrentIndexChange = true;
    emit currentDiffFileIndexChanged(fileIndexForBlockNumber(textCursor().blockNumber()));
    m_controller.m_ignoreCurrentIndexChange = oldIgnore;
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

    QTextCursor cursor = cursorForPosition(e->pos());
    const int blockNumber = cursor.blockNumber();

    addContextMenuActions(menu, fileIndexForBlockNumber(blockNumber),
                          chunkIndexForBlockNumber(blockNumber));

    connect(this, &UnifiedDiffEditorWidget::destroyed, menu.data(), &QMenu::deleteLater);
    menu->exec(e->globalPos());
    delete menu;
}

void UnifiedDiffEditorWidget::addContextMenuActions(QMenu *menu,
                                                    int diffFileIndex,
                                                    int chunkIndex)
{
    menu->addSeparator();

    m_controller.addCodePasterAction(menu);
    m_controller.addApplyAction(menu, diffFileIndex, chunkIndex);
    m_controller.addRevertAction(menu, diffFileIndex, chunkIndex);
}

void UnifiedDiffEditorWidget::clear(const QString &message)
{
    m_leftLineNumberDigits = 1;
    m_rightLineNumberDigits = 1;
    m_leftLineNumbers.clear();
    m_rightLineNumbers.clear();
    m_fileInfo.clear();
    m_chunkInfo.clear();
    setSelections(QMap<int, QList<DiffSelection> >());

    const bool oldIgnore = m_controller.m_ignoreCurrentIndexChange;
    m_controller.m_ignoreCurrentIndexChange = true;
    SelectableTextEditorWidget::clear();
    m_controller.m_contextFileData.clear();
    setPlainText(message);
    m_controller.m_ignoreCurrentIndexChange = oldIgnore;
}

QString UnifiedDiffEditorWidget::lineNumber(int blockNumber) const
{
    QString lineNumberString;

    const bool leftLineExists = m_leftLineNumbers.contains(blockNumber);
    const bool rightLineExists = m_rightLineNumbers.contains(blockNumber);

    if (leftLineExists || rightLineExists) {
        const QString leftLine = leftLineExists
                ? QString::number(m_leftLineNumbers.value(blockNumber))
                : QString();
        lineNumberString += QString(m_leftLineNumberDigits - leftLine.count(),
                                    QLatin1Char(' ')) + leftLine;

        lineNumberString += QLatin1Char('|');

        const QString rightLine = rightLineExists
                ? QString::number(m_rightLineNumbers.value(blockNumber))
                : QString();
        lineNumberString += QString(m_rightLineNumberDigits - rightLine.count(),
                                    QLatin1Char(' ')) + rightLine;
    }
    return lineNumberString;
}

int UnifiedDiffEditorWidget::lineNumberDigits() const
{
    return m_leftLineNumberDigits + m_rightLineNumberDigits + 1;
}

void UnifiedDiffEditorWidget::setLeftLineNumber(int blockNumber, int lineNumber)
{
    const QString lineNumberString = QString::number(lineNumber);
    m_leftLineNumbers.insert(blockNumber, lineNumber);
    m_leftLineNumberDigits = qMax(m_leftLineNumberDigits,
                                  lineNumberString.count());
}

void UnifiedDiffEditorWidget::setRightLineNumber(int blockNumber, int lineNumber)
{
    const QString lineNumberString = QString::number(lineNumber);
    m_rightLineNumbers.insert(blockNumber, lineNumber);
    m_rightLineNumberDigits = qMax(m_rightLineNumberDigits,
                                   lineNumberString.count());
}

void UnifiedDiffEditorWidget::setFileInfo(int blockNumber,
                                          const DiffFileInfo &leftFileInfo,
                                          const DiffFileInfo &rightFileInfo)
{
    m_fileInfo[blockNumber] = qMakePair(leftFileInfo, rightFileInfo);
}

void UnifiedDiffEditorWidget::setChunkIndex(int startBlockNumber,
                                            int blockCount,
                                            int chunkIndex)
{
    m_chunkInfo.insert(startBlockNumber, qMakePair(blockCount, chunkIndex));
}

void UnifiedDiffEditorWidget::setDiff(const QList<FileData> &diffFileList,
                                      const QString &workingDirectory)
{
    Q_UNUSED(workingDirectory)

    clear();
    m_controller.m_contextFileData = diffFileList;
    showDiff();
}

QString UnifiedDiffEditorWidget::showChunk(const ChunkData &chunkData,
                                           bool lastChunk,
                                           int *blockNumber,
                                           int *charNumber,
                                           QMap<int, QList<DiffSelection> > *selections)
{
    if (chunkData.contextChunk)
        return QString();

    QString diffText;
    int leftLineCount = 0;
    int rightLineCount = 0;
    int blockCount = 0;
    int charCount = 0;
    QList<TextLineData> leftBuffer, rightBuffer;

    (*selections)[*blockNumber].append(DiffSelection(&m_controller.m_chunkLineFormat));

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
            if (leftBuffer.count()) {
                for (int j = 0; j < leftBuffer.count(); j++) {
                    const TextLineData &lineData = leftBuffer.at(j);
                    const QString line = DiffUtils::makePatchLine(
                                QLatin1Char('-'),
                                lineData.text,
                                lastChunk,
                                i == chunkData.rows.count()
                                && j == leftBuffer.count() - 1);

                    const int blockDelta = line.count(QLatin1Char('\n')); // no new line
                                                     // could have been added
                    for (int k = 0; k < blockDelta; k++)
                        (*selections)[*blockNumber + blockCount + 1 + k].append(&m_controller.m_leftLineFormat);

                    for (auto it = lineData.changedPositions.cbegin(),
                              end = lineData.changedPositions.cend(); it != end; ++it) {
                        const int startPos = it.key() < 0
                                ? 1 : it.key() + 1;
                        const int endPos = it.value() < 0
                                ? it.value() : it.value() + 1;
                        (*selections)[*blockNumber + blockCount + 1].append(
                                    DiffSelection(startPos, endPos, &m_controller.m_leftCharFormat));
                    }

                    if (!line.isEmpty()) {
                        setLeftLineNumber(*blockNumber + blockCount + 1,
                                          chunkData.leftStartingLineNumber
                                          + leftLineCount + 1);
                        blockCount += blockDelta;
                        ++leftLineCount;
                    }

                    diffText += line;

                    charCount += line.count();
                }
                leftBuffer.clear();
            }
            if (rightBuffer.count()) {
                for (int j = 0; j < rightBuffer.count(); j++) {
                    const TextLineData &lineData = rightBuffer.at(j);
                    const QString line = DiffUtils::makePatchLine(
                                QLatin1Char('+'),
                                lineData.text,
                                lastChunk,
                                i == chunkData.rows.count()
                                && j == rightBuffer.count() - 1);

                    const int blockDelta = line.count(QLatin1Char('\n')); // no new line
                                                     // could have been added

                    for (int k = 0; k < blockDelta; k++)
                        (*selections)[*blockNumber + blockCount + 1 + k].append(&m_controller.m_rightLineFormat);

                    for (auto it = lineData.changedPositions.cbegin(),
                              end = lineData.changedPositions.cend(); it != end; ++it) {
                        const int startPos = it.key() < 0
                                ? 1 : it.key() + 1;
                        const int endPos = it.value() < 0
                                ? it.value() : it.value() + 1;
                        (*selections)[*blockNumber + blockCount + 1].append
                                (DiffSelection(startPos, endPos, &m_controller.m_rightCharFormat));
                    }

                    if (!line.isEmpty()) {
                        setRightLineNumber(*blockNumber + blockCount + 1,
                                           chunkData.rightStartingLineNumber
                                           + rightLineCount + 1);
                        blockCount += blockDelta;
                        ++rightLineCount;
                    }

                    diffText += line;

                    charCount += line.count();
                }
                rightBuffer.clear();
            }
            if (i < chunkData.rows.count()) {
                const QString line = DiffUtils::makePatchLine(QLatin1Char(' '),
                                          rowData.rightLine.text,
                                          lastChunk,
                                          i == chunkData.rows.count() - 1);

                if (!line.isEmpty()) {
                    setLeftLineNumber(*blockNumber + blockCount + 1,
                                      chunkData.leftStartingLineNumber
                                      + leftLineCount + 1);
                    setRightLineNumber(*blockNumber + blockCount + 1,
                                       chunkData.rightStartingLineNumber
                                       + rightLineCount + 1);
                    blockCount += line.count(QLatin1Char('\n'));
                    ++leftLineCount;
                    ++rightLineCount;
                }

                diffText += line;

                charCount += line.count();
            }
        } else {
            if (rowData.leftLine.textLineType == TextLineData::TextLine)
                leftBuffer.append(rowData.leftLine);
            if (rowData.rightLine.textLineType == TextLineData::TextLine)
                rightBuffer.append(rowData.rightLine);
        }
    }

    const QString chunkLine = QLatin1String("@@ -")
            + QString::number(chunkData.leftStartingLineNumber + 1)
            + QLatin1Char(',')
            + QString::number(leftLineCount)
            + QLatin1String(" +")
            + QString::number(chunkData.rightStartingLineNumber+ 1)
            + QLatin1Char(',')
            + QString::number(rightLineCount)
            + QLatin1String(" @@")
            + chunkData.contextInfo
            + QLatin1Char('\n');

    diffText.prepend(chunkLine);

    *blockNumber += blockCount + 1; // +1 for chunk line
    *charNumber += charCount + chunkLine.count();
    return diffText;
}


static void setFoldingIndent(const QTextBlock &block, int indent)
{
    if (TextEditor::TextBlockUserData *userData = TextEditor::TextDocumentLayout::userData(block))
         userData->setFoldingIndent(indent);
}

void UnifiedDiffEditorWidget::showDiff()
{
    QString diffText;

    int blockNumber = 0;
    int charNumber = 0;

    // 'foldingIndent' is populated with <block number> and folding indentation
    // value where 1 indicates start of new file and 2 indicates a diff chunk.
    // Remaining lines (diff contents) are assigned 3.
    QHash<int, int> foldingIndent;

    QMap<int, QList<DiffSelection> > selections;

    for (const FileData &fileData : m_controller.m_contextFileData) {
        const QString leftFileInfo = QLatin1String("--- ")
                + fileData.leftFileInfo.fileName + QLatin1Char('\n');
        const QString rightFileInfo = QLatin1String("+++ ")
                + fileData.rightFileInfo.fileName + QLatin1Char('\n');
        setFileInfo(blockNumber, fileData.leftFileInfo, fileData.rightFileInfo);
        selections[blockNumber].append(DiffSelection(&m_controller.m_fileLineFormat));
        foldingIndent.insert(blockNumber, 1);
        blockNumber++;
        foldingIndent.insert(blockNumber, 1);
        selections[blockNumber].append(DiffSelection(&m_controller.m_fileLineFormat));
        blockNumber++;

        diffText += leftFileInfo;
        diffText += rightFileInfo;
        charNumber += leftFileInfo.count() + rightFileInfo.count();

        if (fileData.binaryFiles) {
            foldingIndent.insert(blockNumber, 2);
            selections[blockNumber].append(DiffSelection(&m_controller.m_chunkLineFormat));
            blockNumber++;
            const QString binaryLine = QLatin1String("Binary files ")
                    + fileData.leftFileInfo.fileName
                    + QLatin1String(" and ")
                    + fileData.rightFileInfo.fileName
                    + QLatin1String(" differ\n");
            diffText += binaryLine;
            charNumber += binaryLine.count();
        } else {
            for (int j = 0; j < fileData.chunks.count(); j++) {
                const int oldBlockNumber = blockNumber;
                foldingIndent.insert(blockNumber, 2);
                diffText += showChunk(fileData.chunks.at(j),
                                      (j == fileData.chunks.count() - 1)
                                      && fileData.lastChunkAtTheEndOfFile,
                                      &blockNumber,
                                      &charNumber,
                                      &selections);
                if (!fileData.chunks.at(j).contextChunk)
                    setChunkIndex(oldBlockNumber, blockNumber - oldBlockNumber, j);
            }
        }

    }

    if (diffText.isEmpty()) {
        setPlainText(tr("No difference."));
        return;
    }

    diffText.replace(QLatin1Char('\r'), QLatin1Char(' '));
    const bool oldIgnore = m_controller.m_ignoreCurrentIndexChange;
    m_controller.m_ignoreCurrentIndexChange = true;
    setPlainText(diffText);

    QTextBlock block = document()->firstBlock();
    for (int b = 0; block.isValid(); block = block.next(), ++b)
        setFoldingIndent(block, foldingIndent.value(b, 3));

    m_controller.m_ignoreCurrentIndexChange = oldIgnore;

    setSelections(selections);
}

int UnifiedDiffEditorWidget::blockNumberForFileIndex(int fileIndex) const
{
    if (fileIndex < 0 || fileIndex >= m_fileInfo.count())
        return -1;

    return (m_fileInfo.constBegin() + fileIndex).key();
}

int UnifiedDiffEditorWidget::fileIndexForBlockNumber(int blockNumber) const
{
    int i = -1;
    for (auto it = m_fileInfo.cbegin(), end = m_fileInfo.cend(); it != end; ++it, ++i) {
        if (it.key() > blockNumber)
            break;
    }

    return i;
}

int UnifiedDiffEditorWidget::chunkIndexForBlockNumber(int blockNumber) const
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

void UnifiedDiffEditorWidget::jumpToOriginalFile(const QTextCursor &cursor)
{
    if (m_fileInfo.isEmpty())
        return;

    const int blockNumber = cursor.blockNumber();
    const int fileIndex = fileIndexForBlockNumber(blockNumber);
    if (fileIndex < 0)
        return;

    const FileData fileData = m_controller.m_contextFileData.at(fileIndex);
    const QString leftFileName = fileData.leftFileInfo.fileName;
    const QString rightFileName = fileData.rightFileInfo.fileName;

    const int columnNumber = cursor.positionInBlock() - 1; // -1 for the first character in line

    const int rightLineNumber = m_rightLineNumbers.value(blockNumber, -1);
    if (rightLineNumber >= 0) {
        m_controller.jumpToOriginalFile(rightFileName, rightLineNumber, columnNumber);
        return;
    }

    const int leftLineNumber = m_leftLineNumbers.value(blockNumber, -1);
    if (leftLineNumber >= 0) {
        if (leftFileName == rightFileName) {
            for (const ChunkData &chunkData : fileData.chunks) {

                int newLeftLineNumber = chunkData.leftStartingLineNumber;
                int newRightLineNumber = chunkData.rightStartingLineNumber;

                for (const RowData &rowData : chunkData.rows) {
                    if (rowData.leftLine.textLineType == TextLineData::TextLine)
                        newLeftLineNumber++;
                    if (rowData.rightLine.textLineType == TextLineData::TextLine)
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
    if (m_controller.m_ignoreCurrentIndexChange)
        return;

    const bool oldIgnore = m_controller.m_ignoreCurrentIndexChange;
    m_controller.m_ignoreCurrentIndexChange = true;
    const int blockNumber = blockNumberForFileIndex(diffFileIndex);

    QTextBlock block = document()->findBlockByNumber(blockNumber);
    QTextCursor cursor = textCursor();
    cursor.setPosition(block.position());
    setTextCursor(cursor);
    verticalScrollBar()->setValue(blockNumber);
    m_controller.m_ignoreCurrentIndexChange = oldIgnore;
}

} // namespace Internal
} // namespace DiffEditor
