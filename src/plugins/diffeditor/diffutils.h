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

#ifndef DIFFUTILS_H
#define DIFFUTILS_H

#include "diffeditor_global.h"

#include <QString>
#include <QMap>

namespace TextEditor { class FontSettings; }

namespace DiffEditor {

class Diff;

class DIFFEDITOR_EXPORT DiffFileInfo {
public:
    DiffFileInfo() {}
    DiffFileInfo(const QString &file) : fileName(file) {}
    DiffFileInfo(const QString &file, const QString &type)
        : fileName(file), typeInfo(type) {}
    QString fileName;
    QString typeInfo;
};

class DIFFEDITOR_EXPORT TextLineData {
public:
    enum TextLineType {
        TextLine,
        Separator,
        Invalid
    };
    TextLineData() : textLineType(Invalid) {}
    TextLineData(const QString &txt) : textLineType(TextLine), text(txt) {}
    TextLineData(TextLineType t) : textLineType(t) {}
    TextLineType textLineType;
    QString text;
    /*
     * <start position, end position>
     * <-1, n> means this is a continuation from the previous line
     * <n, -1> means this will be continued in the next line
     * <-1, -1> the whole line is a continuation (from the previous line to the next line)
     */
    QMap<int, int> changedPositions; // counting from the beginning of the line
};

class DIFFEDITOR_EXPORT RowData {
public:
    RowData() : equal(false) {}
    RowData(const TextLineData &l)
        : leftLine(l), rightLine(l), equal(true) {}
    RowData(const TextLineData &l, const TextLineData &r)
        : leftLine(l), rightLine(r), equal(false) {}
    TextLineData leftLine;
    TextLineData rightLine;
    bool equal;
};

class DIFFEDITOR_EXPORT ChunkData {
public:
    ChunkData() : contextChunk(false),
        leftStartingLineNumber(0), rightStartingLineNumber(0) {}
    QList<RowData> rows;
    bool contextChunk;
    int leftStartingLineNumber;
    int rightStartingLineNumber;
    QString contextInfo;
};

class DIFFEDITOR_EXPORT FileData {
public:
    enum FileOperation {
        ChangeFile,
        NewFile,
        DeleteFile,
        CopyFile,
        RenameFile
    };

    FileData()
        : fileOperation(ChangeFile),
          binaryFiles(false),
          lastChunkAtTheEndOfFile(false),
          contextChunksIncluded(false) {}
    FileData(const ChunkData &chunkData)
        : fileOperation(ChangeFile),
          binaryFiles(false),
          lastChunkAtTheEndOfFile(false),
          contextChunksIncluded(false) { chunks.append(chunkData); }
    QList<ChunkData> chunks;
    DiffFileInfo leftFileInfo;
    DiffFileInfo rightFileInfo;
    FileOperation fileOperation;
    bool binaryFiles;
    bool lastChunkAtTheEndOfFile;
    bool contextChunksIncluded;
};

class DIFFEDITOR_EXPORT DiffUtils {
public:
    enum PatchFormattingFlags {
        AddLevel = 0x1, // Add 'a/' , '/b' for git am
        GitFormat = AddLevel | 0x2, // Add line 'diff ..' as git does
    };

    static ChunkData calculateOriginalData(const QList<Diff> &leftDiffList,
                                           const QList<Diff> &rightDiffList);
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
    static QString makePatch(const QList<FileData> &fileDataList,
                             unsigned formatFlags = 0);
    static QList<FileData> readPatch(const QString &patch,
                                     bool *ok = 0);
};

} // namespace DiffEditor

#endif // DIFFUTILS_H
