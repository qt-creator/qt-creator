// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "diffeditorwidgetcontroller.h"
#include "selectabletexteditorwidget.h"

namespace Core { class IContext; }

namespace TextEditor { class FontSettings; }

namespace Utils {
template <typename R>
class Async;
}

namespace DiffEditor {

class ChunkData;
class FileData;
class ChunkSelection;

namespace Internal {

class DiffEditorDocument;
class UnifiedDiffOutput;

class UnifiedDiffData
{
public:
    DiffChunkInfo m_chunkInfo;
    // block number, visual line number.
    QMap<int, DiffFileInfoArray> m_fileInfo;
    // block number, visual line number, chunk row number
    using LineNumbers = QMap<int, QPair<int, int>>;
    std::array<LineNumbers, SideCount> m_lineNumbers{};
    std::array<int, SideCount> m_lineNumberDigits{1, 1};

    int blockNumberForFileIndex(int fileIndex) const;
    int fileIndexForBlockNumber(int blockNumber) const;

    QString setChunk(const DiffEditorInput &input, const ChunkData &chunkData,
                     bool lastChunk, int *blockNumber, DiffSelections *selections);
private:
    void setLineNumber(DiffSide side, int blockNumber, int lineNumber, int rowNumberInChunk);
};

class UnifiedDiffOutput
{
public:
    UnifiedDiffData diffData;
    QString diffText;
    // 'foldingIndent' is populated with <block number> and folding indentation
    // value where 1 indicates start of new file and 2 indicates a diff chunk.
    // Remaining lines (diff contents) are assigned 3.
    QHash<int, int> foldingIndent;
    DiffSelections selections;
};

class UnifiedShowResult
{
public:
    QSharedPointer<TextEditor::TextDocument> textDocument;
    UnifiedDiffData diffData;
    DiffSelections selections;
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

    void clear(const QString &message = {});

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
    void jumpToOriginalFile(const QTextCursor &cursor);
    void addContextMenuActions(QMenu *menu, int fileIndex, int chunkIndex,
                               const ChunkSelection &selection);

    UnifiedDiffData m_data;
    DiffEditorWidgetController m_controller;
    QByteArray m_state;

    std::unique_ptr<Utils::Async<UnifiedShowResult>> m_asyncTask;
};

} // namespace Internal
} // namespace DiffEditor
