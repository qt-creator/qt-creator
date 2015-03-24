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

#include "unifieddiffeditorwidget.h"
#include "diffeditorcontroller.h"
#include "diffutils.h"
#include "diffeditorconstants.h"
#include "diffeditordocument.h"

#include <QPlainTextEdit>
#include <QMenu>
#include <QPlainTextDocumentLayout>
#include <QTextBlock>
#include <QTextCodec>
#include <QPainter>
#include <QDir>
#include <QMessageBox>

#include <texteditor/textdocument.h>
#include <texteditor/textdocumentlayout.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/displaysettings.h>
#include <texteditor/highlighterutils.h>

#include <coreplugin/patchtool.h>
#include <coreplugin/editormanager/editormanager.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/tooltip/tooltip.h>

//static const int FILE_LEVEL = 1;
//static const int CHUNK_LEVEL = 2;

using namespace Core;
using namespace TextEditor;

namespace DiffEditor {
namespace Internal {

UnifiedDiffEditorWidget::UnifiedDiffEditorWidget(QWidget *parent)
    : SelectableTextEditorWidget("DiffEditor.UnifiedDiffEditor", parent)
    , m_document(0)
    , m_ignoreCurrentIndexChange(false)
    , m_contextMenuFileIndex(-1)
    , m_contextMenuChunkIndex(-1)
    , m_leftLineNumberDigits(1)
    , m_rightLineNumberDigits(1)
{
    DisplaySettings settings = displaySettings();
    settings.m_textWrapping = false;
    settings.m_displayLineNumbers = true;
    settings.m_highlightCurrentLine = false;
    settings.m_displayFoldingMarkers = true;
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
}

void UnifiedDiffEditorWidget::setDocument(DiffEditorDocument *document)
{
    m_document = document;
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

    const bool oldIgnore = m_ignoreCurrentIndexChange;
    m_ignoreCurrentIndexChange = true;
    SelectableTextEditorWidget::restoreState(m_state);
    m_state.clear();
    m_ignoreCurrentIndexChange = oldIgnore;
}

void UnifiedDiffEditorWidget::setDisplaySettings(const DisplaySettings &ds)
{
    DisplaySettings settings = displaySettings();
    settings.m_visualizeWhitespace = ds.m_visualizeWhitespace;
    SelectableTextEditorWidget::setDisplaySettings(settings);
}

void UnifiedDiffEditorWidget::setFontSettings(const FontSettings &fontSettings)
{
    m_fileLineFormat  = fontSettings.toTextCharFormat(C_DIFF_FILE_LINE);
    m_chunkLineFormat = fontSettings.toTextCharFormat(C_DIFF_CONTEXT_LINE);
    m_leftLineFormat  = fontSettings.toTextCharFormat(C_DIFF_SOURCE_LINE);
    m_leftCharFormat  = fontSettings.toTextCharFormat(C_DIFF_SOURCE_CHAR);
    m_rightLineFormat = fontSettings.toTextCharFormat(C_DIFF_DEST_LINE);
    m_rightCharFormat = fontSettings.toTextCharFormat(C_DIFF_DEST_CHAR);
}

void UnifiedDiffEditorWidget::slotCursorPositionChangedInEditor()
{
    if (m_ignoreCurrentIndexChange)
        return;

    const bool oldIgnore = m_ignoreCurrentIndexChange;
    m_ignoreCurrentIndexChange = true;
    emit currentDiffFileIndexChanged(fileIndexForBlockNumber(textCursor().blockNumber()));
    m_ignoreCurrentIndexChange = oldIgnore;
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
    if (!m_document || !m_document->controller())
        return;

    menu->addSeparator();
    menu->addSeparator();
    QAction *sendChunkToCodePasterAction =
            menu->addAction(tr("Send Chunk to CodePaster..."));
    connect(sendChunkToCodePasterAction, &QAction::triggered,
            this, &UnifiedDiffEditorWidget::slotSendChunkToCodePaster);
    QAction *applyAction = menu->addAction(tr("Apply Chunk..."));
    connect(applyAction, &QAction::triggered, this, &UnifiedDiffEditorWidget::slotApplyChunk);
    QAction *revertAction = menu->addAction(tr("Revert Chunk..."));
    connect(revertAction, &QAction::triggered, this, &UnifiedDiffEditorWidget::slotRevertChunk);

    m_contextMenuFileIndex = diffFileIndex;
    m_contextMenuChunkIndex = chunkIndex;

    applyAction->setEnabled(false);
    revertAction->setEnabled(false);

    if (m_contextMenuFileIndex < 0 || m_contextMenuChunkIndex < 0)
        return;

    if (m_contextMenuFileIndex >= m_contextFileData.count())
        return;

    const FileData fileData = m_contextFileData.at(m_contextMenuFileIndex);
    if (m_contextMenuChunkIndex >= fileData.chunks.count())
        return;

    m_document->chunkActionsRequested(menu, diffFileIndex, chunkIndex);

    revertAction->setEnabled(true);

    if (fileData.leftFileInfo.fileName == fileData.rightFileInfo.fileName)
        return;

    applyAction->setEnabled(true);
}

void UnifiedDiffEditorWidget::slotSendChunkToCodePaster()
{
    if (!m_document)
        return;

    const QString patch = m_document->makePatch(m_contextMenuFileIndex, m_contextMenuChunkIndex, false);

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

void UnifiedDiffEditorWidget::slotApplyChunk()
{
    patch(false);
}

void UnifiedDiffEditorWidget::slotRevertChunk()
{
    patch(true);
}

void UnifiedDiffEditorWidget::patch(bool revert)
{
    if (!m_document)
        return;

    const QString title = revert ? tr("Revert Chunk") : tr("Apply Chunk");
    const QString question = revert
            ? tr("Would you like to revert the chunk?")
            : tr("Would you like to apply the chunk?");
    if (QMessageBox::No == QMessageBox::question(this, title, question,
                                                 QMessageBox::Yes
                                                 | QMessageBox::No)) {
        return;
    }

    const int strip = m_document->baseDirectory().isEmpty() ? -1 : 0;

    const FileData fileData = m_contextFileData.at(m_contextMenuFileIndex);
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

void UnifiedDiffEditorWidget::clear(const QString &message)
{
    m_leftLineNumberDigits = 1;
    m_rightLineNumberDigits = 1;
    m_leftLineNumbers.clear();
    m_rightLineNumbers.clear();
    m_fileInfo.clear();
    m_chunkInfo.clear();
    setSelections(QMap<int, QList<DiffSelection> >());

    const bool oldIgnore = m_ignoreCurrentIndexChange;
    m_ignoreCurrentIndexChange = true;
    SelectableTextEditorWidget::clear();
    setDiff(QList<FileData>(), QString());
    setPlainText(message);
    m_ignoreCurrentIndexChange = oldIgnore;
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

    m_contextFileData = diffFileList;

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

    (*selections)[*blockNumber].append(DiffSelection(&m_chunkLineFormat));

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
                        (*selections)[*blockNumber + blockCount + 1 + k].append(&m_leftLineFormat);
                    QMapIterator<int, int> itPos(lineData.changedPositions);
                    while (itPos.hasNext()) {
                        itPos.next();
                        const int startPos = itPos.key() < 0
                                ? 1 : itPos.key() + 1;
                        const int endPos = itPos.value() < 0
                                ? itPos.value() : itPos.value() + 1;
                        (*selections)[*blockNumber + blockCount + 1].append(
                                    DiffSelection(startPos, endPos, &m_leftCharFormat));
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
                        (*selections)[*blockNumber + blockCount + 1 + k].append(&m_rightLineFormat);
                    QMapIterator<int, int> itPos(lineData.changedPositions);
                    while (itPos.hasNext()) {
                        itPos.next();
                        const int startPos = itPos.key() < 0
                                ? 1 : itPos.key() + 1;
                        const int endPos = itPos.value() < 0
                                ? itPos.value() : itPos.value() + 1;
                        (*selections)[*blockNumber + blockCount + 1].append
                                (DiffSelection(startPos, endPos, &m_rightCharFormat));
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

void UnifiedDiffEditorWidget::showDiff()
{
    QString diffText;

    int blockNumber = 0;
    int charNumber = 0;

    QMap<int, QList<DiffSelection> > selections;

    for (int i = 0; i < m_contextFileData.count(); i++) {
        const FileData &fileData = m_contextFileData.at(i);
        const QString leftFileInfo = QLatin1String("--- ")
                + fileData.leftFileInfo.fileName + QLatin1Char('\n');
        const QString rightFileInfo = QLatin1String("+++ ")
                + fileData.rightFileInfo.fileName + QLatin1Char('\n');
        setFileInfo(blockNumber, fileData.leftFileInfo, fileData.rightFileInfo);
        selections[blockNumber].append(DiffSelection(&m_fileLineFormat));
        blockNumber++;
        selections[blockNumber].append(DiffSelection(&m_fileLineFormat));
        blockNumber++;

        diffText += leftFileInfo;
        diffText += rightFileInfo;
        charNumber += leftFileInfo.count() + rightFileInfo.count();

        if (fileData.binaryFiles) {
            selections[blockNumber].append(DiffSelection(&m_chunkLineFormat));
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
    const bool oldIgnore = m_ignoreCurrentIndexChange;
    m_ignoreCurrentIndexChange = true;
    setPlainText(diffText);
    m_ignoreCurrentIndexChange = oldIgnore;

    setSelections(selections);
}

int UnifiedDiffEditorWidget::blockNumberForFileIndex(int fileIndex) const
{
    if (fileIndex < 0 || fileIndex >= m_fileInfo.count())
        return -1;

    QMap<int, QPair<DiffFileInfo, DiffFileInfo> >::const_iterator it
            = m_fileInfo.constBegin();
    for (int i = 0; i < fileIndex; i++)
        ++it;

    return it.key();
}

int UnifiedDiffEditorWidget::fileIndexForBlockNumber(int blockNumber) const
{
    QMap<int, QPair<DiffFileInfo, DiffFileInfo> >::const_iterator it
            = m_fileInfo.constBegin();
    QMap<int, QPair<DiffFileInfo, DiffFileInfo> >::const_iterator itEnd
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

int UnifiedDiffEditorWidget::chunkIndexForBlockNumber(int blockNumber) const
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

void UnifiedDiffEditorWidget::jumpToOriginalFile(const QTextCursor &cursor)
{
    if (m_fileInfo.isEmpty())
        return;

    const int blockNumber = cursor.blockNumber();
    const int fileIndex = fileIndexForBlockNumber(blockNumber);
    if (fileIndex < 0)
        return;

    const FileData fileData = m_contextFileData.at(fileIndex);
    const QString leftFileName = fileData.leftFileInfo.fileName;
    const QString rightFileName = fileData.rightFileInfo.fileName;

    const int columnNumber = cursor.positionInBlock() - 1; // -1 for the first character in line

    const int rightLineNumber = m_rightLineNumbers.value(blockNumber, -1);
    if (rightLineNumber >= 0) {
        jumpToOriginalFile(rightFileName, rightLineNumber, columnNumber);
        return;
    }

    const int leftLineNumber = m_leftLineNumbers.value(blockNumber, -1);
    if (leftLineNumber >= 0) {
        if (leftFileName == rightFileName) {
            for (int i = 0; i < fileData.chunks.count(); i++) {
                const ChunkData chunkData = fileData.chunks.at(i);

                int newLeftLineNumber = chunkData.leftStartingLineNumber;
                int newRightLineNumber = chunkData.rightStartingLineNumber;

                for (int j = 0; j < chunkData.rows.count(); j++) {
                    const RowData rowData = chunkData.rows.at(j);
                    if (rowData.leftLine.textLineType == TextLineData::TextLine)
                        newLeftLineNumber++;
                    if (rowData.rightLine.textLineType == TextLineData::TextLine)
                        newRightLineNumber++;
                    if (newLeftLineNumber == leftLineNumber) {
                        jumpToOriginalFile(leftFileName, newRightLineNumber, 0);
                        return;
                    }
                }
            }
        } else {
            jumpToOriginalFile(leftFileName, leftLineNumber, columnNumber);
        }
        return;
    }
}

void UnifiedDiffEditorWidget::jumpToOriginalFile(const QString &fileName,
                                                 int lineNumber,
                                                 int columnNumber)
{
    if (!m_document)
        return;

    const QDir dir(m_document->baseDirectory());
    const QString absoluteFileName = dir.absoluteFilePath(fileName);
    QFileInfo fi(absoluteFileName);
    if (fi.exists() && !fi.isDir())
        EditorManager::openEditorAt(absoluteFileName, lineNumber, columnNumber);
}

void UnifiedDiffEditorWidget::setCurrentDiffFileIndex(int diffFileIndex)
{
    if (m_ignoreCurrentIndexChange)
        return;

    const bool oldIgnore = m_ignoreCurrentIndexChange;
    m_ignoreCurrentIndexChange = true;
    const int blockNumber = blockNumberForFileIndex(diffFileIndex);

    QTextBlock block = document()->findBlockByNumber(blockNumber);
    QTextCursor cursor = textCursor();
    cursor.setPosition(block.position());
    setTextCursor(cursor);
    centerCursor();
    m_ignoreCurrentIndexChange = oldIgnore;
}

} // namespace Internal
} // namespace DiffEditor
