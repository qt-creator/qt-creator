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

#pragma once

#include "selectabletexteditorwidget.h"
#include "diffeditorwidgetcontroller.h"

namespace TextEditor {
class DisplaySettings;
class FontSettings;
}

QT_BEGIN_NAMESPACE
class QSplitter;
class QTextCharFormat;
QT_END_NAMESPACE

namespace DiffEditor {

class ChunkData;
class FileData;

namespace Internal {

class DiffEditorDocument;

class UnifiedDiffEditorWidget : public SelectableTextEditorWidget
{
    Q_OBJECT
public:
    UnifiedDiffEditorWidget(QWidget *parent = 0);

    void setDocument(DiffEditorDocument *document);
    DiffEditorDocument *diffDocument() const;

    void setDiff(const QList<FileData> &diffFileList,
                 const QString &workingDirectory);
    void setCurrentDiffFileIndex(int diffFileIndex);

    void saveState();
    void restoreState();

    void clear(const QString &message = QString());
    void setDisplaySettings(const TextEditor::DisplaySettings &ds) override;

signals:
    void currentDiffFileIndexChanged(int index);

protected:
    void mouseDoubleClickEvent(QMouseEvent *e) override;
    void contextMenuEvent(QContextMenuEvent *e) override;
    QString lineNumber(int blockNumber) const override;
    int lineNumberDigits() const override;

private:
    void setFontSettings(const TextEditor::FontSettings &fontSettings);

    void slotCursorPositionChangedInEditor();

    void setLeftLineNumber(int blockNumber, int lineNumber);
    void setRightLineNumber(int blockNumber, int lineNumber);
    void setFileInfo(int blockNumber,
                     const DiffFileInfo &leftFileInfo,
                     const DiffFileInfo &rightFileInfo);
    void setChunkIndex(int startBlockNumber, int blockCount, int chunkIndex);
    void showDiff();
    QString showChunk(const ChunkData &chunkData,
                      bool lastChunk,
                      int *blockNumber,
                      int *charNumber,
                      QMap<int, QList<DiffSelection> > *selections);
    int blockNumberForFileIndex(int fileIndex) const;
    int fileIndexForBlockNumber(int blockNumber) const;
    int chunkIndexForBlockNumber(int blockNumber) const;
    void jumpToOriginalFile(const QTextCursor &cursor);
    void addContextMenuActions(QMenu *menu,
                               int diffFileIndex,
                               int chunkIndex);

    // block number, visual line number.
    QMap<int, int> m_leftLineNumbers;
    QMap<int, int> m_rightLineNumbers;

    DiffEditorWidgetController m_controller;

    int m_leftLineNumberDigits = 1;
    int m_rightLineNumberDigits = 1;
    // block number, visual line number.
    QMap<int, QPair<DiffFileInfo, DiffFileInfo> > m_fileInfo;
    // start block number, block count of a chunk, chunk index inside a file.
    QMap<int, QPair<int, int> > m_chunkInfo;

    QByteArray m_state;
};

} // namespace Internal
} // namespace DiffEditor
