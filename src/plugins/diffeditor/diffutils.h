// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "diffeditor_global.h"
#include "diffenums.h"

#include <utils/algorithm.h>

#include <QMap>
#include <QString>

#include <array>

QT_BEGIN_NAMESPACE
template <class T>
class QPromise;
QT_END_NAMESPACE

namespace Utils { class Diff; }

namespace DiffEditor {

class DIFFEDITOR_EXPORT DiffFileInfo {
public:
    enum PatchBehaviour {
        PatchFile,
        PatchEditor
    };

    DiffFileInfo() = default;
    DiffFileInfo(const QString &file, const QString &type = {})
        : fileName(file), typeInfo(type) {}
    QString fileName;
    QString typeInfo;
    PatchBehaviour patchBehaviour = PatchFile;
};

using DiffFileInfoArray = std::array<DiffFileInfo, SideCount>;

class DiffChunkInfo {
public:
    int chunkIndexForBlockNumber(int blockNumber) const;
    int chunkRowForBlockNumber(int blockNumber) const;
    int chunkRowsCountForBlockNumber(int blockNumber) const;

    void setChunkIndex(int startBlockNumber, int blockCount, int chunkIndex) {
        m_chunkInfo.insert(startBlockNumber, {blockCount, chunkIndex});
    }

private:
    // start block number, block count of a chunk, chunk index inside a file.
    QMap<int, QPair<int, int>> m_chunkInfo;
};

class DIFFEDITOR_EXPORT TextLineData {
public:
    enum TextLineType {
        TextLine,
        Separator,
        Invalid
    };
    TextLineData() = default;
    TextLineData(const QString &txt) : text(txt), textLineType(TextLine) {}
    TextLineData(TextLineType t) : textLineType(t) {}
    QString text;
    /*
     * <start position, end position>
     * <-1, n> means this is a continuation from the previous line
     * <n, -1> means this will be continued in the next line
     * <-1, -1> the whole line is a continuation (from the previous line to the next line)
     */
    QMap<int, int> changedPositions; // counting from the beginning of the line
    TextLineType textLineType = Invalid;
};

class DIFFEDITOR_EXPORT RowData {
public:
    RowData() = default;
    RowData(const TextLineData &l)
        : line({l, l}), equal(true) {}
    RowData(const TextLineData &l, const TextLineData &r)
        : line({l, r}) {}
    std::array<TextLineData, SideCount> line{};
    bool equal = false;
};

class DIFFEDITOR_EXPORT ChunkData {
public:
    QList<RowData> rows;
    QString contextInfo;
    std::array<int, SideCount> startingLineNumber{};
    bool contextChunk = false;
};

class DIFFEDITOR_EXPORT ChunkSelection {
public:
    ChunkSelection() = default;
    ChunkSelection(const QList<int> &left, const QList<int> &right)
        : selection({left, right}) {}
    bool isNull() const { return Utils::allOf(selection, &QList<int>::isEmpty); }
    int selectedRowsCount() const;
    std::array<QList<int>, SideCount> selection{};
};

class DIFFEDITOR_EXPORT FileData {
public:
    enum FileOperation {
        ChangeFile,
        ChangeMode,
        NewFile,
        DeleteFile,
        CopyFile,
        RenameFile
    };

    FileData() = default;
    FileData(const ChunkData &chunkData) { chunks.append(chunkData); }
    QList<ChunkData> chunks;
    DiffFileInfoArray fileInfo{};
    FileOperation fileOperation = ChangeFile;
    bool binaryFiles = false;
    bool lastChunkAtTheEndOfFile = false;
    bool contextChunksIncluded = false;
};

class DIFFEDITOR_EXPORT DiffUtils {
public:
    static ChunkData calculateOriginalData(const QList<Utils::Diff> &leftDiffList,
                                           const QList<Utils::Diff> &rightDiffList);
    static FileData calculateContextData(const ChunkData &originalData,
                                         int contextLineCount,
                                         int joinChunkThreshold = 1);
    static QString makePatchLine(const QChar &startLineCharacter,
                                 const QString &textLine,
                                 bool lastChunk,
                                 bool lastLine);
    static QString makePatch(const ChunkData &chunkData,
                             bool lastChunk = false);
    static QString makePatch(const ChunkData &chunkData,
                             const QString &leftFileName,
                             const QString &rightFileName,
                             bool lastChunk = false);
    static QString makePatch(const QList<FileData> &fileDataList);
    static std::optional<QList<FileData>> readPatch(const QString &patch);
    static void readPatchWithPromise(QPromise<QList<FileData>> &promise, const QString &patch);
};

} // namespace DiffEditor
