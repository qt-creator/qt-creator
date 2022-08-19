// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "selectabletexteditorwidget.h"
#include "diffeditorwidgetcontroller.h"

namespace Core { class IContext; }

namespace TextEditor {
class DisplaySettings;
class FontSettings;
}

namespace DiffEditor {

class ChunkData;
class FileData;
class ChunkSelection;

namespace Internal {

class DiffEditorDocument;

class UnifiedDiffEditorWidget final : public SelectableTextEditorWidget
{
    Q_OBJECT
public:
    UnifiedDiffEditorWidget(QWidget *parent = nullptr);

    void setDocument(DiffEditorDocument *document);
    DiffEditorDocument *diffDocument() const;

    void setDiff(const QList<FileData> &diffFileList);
    void setCurrentDiffFileIndex(int diffFileIndex);

    void saveState();

    using TextEditor::TextEditorWidget::restoreState;
    void restoreState();

    void clear(const QString &message = QString());
    void setDisplaySettings(const TextEditor::DisplaySettings &ds) override;

signals:
    void currentDiffFileIndexChanged(int index);

protected:
    void mouseDoubleClickEvent(QMouseEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
    void contextMenuEvent(QContextMenuEvent *e) override;
    QString lineNumber(int blockNumber) const override;
    int lineNumberDigits() const override;

private:
    void setFontSettings(const TextEditor::FontSettings &fontSettings);

    void slotCursorPositionChangedInEditor();

    void setLeftLineNumber(int blockNumber, int lineNumber, int rowNumberInChunk);
    void setRightLineNumber(int blockNumber, int lineNumber, int rowNumberInChunk);
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
                               int fileIndex,
                               int chunkIndex,
                               const ChunkSelection &selection);

    // block number, visual line number, chunk row number
    QMap<int, QPair<int, int> > m_leftLineNumbers;
    QMap<int, QPair<int, int> > m_rightLineNumbers;

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
