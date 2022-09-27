// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "selectabletexteditorwidget.h"
#include "diffeditorwidgetcontroller.h"

#include <QFutureWatcher>

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

class UnifiedDiffOutput
{
public:
    QString diffText;
    // 'foldingIndent' is populated with <block number> and folding indentation
    // value where 1 indicates start of new file and 2 indicates a diff chunk.
    // Remaining lines (diff contents) are assigned 3.
    QHash<int, int> foldingIndent;
    DiffSelections selections;
};

class UnifiedDiffData
{
public:
    UnifiedDiffOutput setDiff(QFutureInterface<void> &fi, int progressMin, int progressMax,
                              const DiffEditorInput &input);

    // block number, visual line number, chunk row number
    using LineNumbers = QMap<int, QPair<int, int>>;
    std::array<LineNumbers, SideCount> m_lineNumbers{};
    std::array<int, SideCount> m_lineNumberDigits{1, 1};

    // block number, visual line number.
    QMap<int, QPair<DiffFileInfo, DiffFileInfo>> m_fileInfo;
    // start block number, block count of a chunk, chunk index inside a file.
    QMap<int, QPair<int, int>> m_chunkInfo;

private:
    void setLineNumber(DiffSide side, int blockNumber, int lineNumber, int rowNumberInChunk);
    void setFileInfo(int blockNumber, const DiffFileInfo &leftInfo, const DiffFileInfo &rightInfo);
    void setChunkIndex(int startBlockNumber, int blockCount, int chunkIndex);
    QString setChunk(const DiffEditorInput &input, const ChunkData &chunkData, bool lastChunk,
                     int *blockNumber, int *charNumber,
                     DiffSelections *selections);
};

class UnifiedDiffEditorWidget final : public SelectableTextEditorWidget
{
    Q_OBJECT
public:
    UnifiedDiffEditorWidget(QWidget *parent = nullptr);
    ~UnifiedDiffEditorWidget();

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

    void showDiff();
    int blockNumberForFileIndex(int fileIndex) const;
    int fileIndexForBlockNumber(int blockNumber) const;
    int chunkIndexForBlockNumber(int blockNumber) const;
    void jumpToOriginalFile(const QTextCursor &cursor);
    void addContextMenuActions(QMenu *menu,
                               int fileIndex,
                               int chunkIndex,
                               const ChunkSelection &selection);

    UnifiedDiffData m_data;
    DiffEditorWidgetController m_controller;
    QByteArray m_state;

    struct ShowResult
    {
        QSharedPointer<TextEditor::TextDocument> textDocument;
        UnifiedDiffData diffData;
        QHash<int, int> foldingIndent;
        DiffSelections selections;
    };

    std::unique_ptr<QFutureWatcher<ShowResult>> m_watcher;
};

} // namespace Internal
} // namespace DiffEditor
