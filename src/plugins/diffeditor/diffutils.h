/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DIFFUTILS_H
#define DIFFUTILS_H

#include <QString>
#include <QMap>
#include <QTextEdit>

#include "diffeditorcontroller.h"
#include "texteditor/texteditorconstants.h"

namespace TextEditor { class FontSettings; }

namespace DiffEditor {

class Diff;

namespace Internal {

class TextLineData {
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
};

class RowData {
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

class ChunkData {
public:
    ChunkData() : contextChunk(false) {}
    QList<RowData> rows;
    bool contextChunk;
    // start position, end position, TextLineData::Separator lines not taken into account
    QMap<int, int> changedLeftPositions; // counting from the beginning of the chunk
    QMap<int, int> changedRightPositions; // counting from the beginning of the chunk
};

class FileData {
public:
    FileData() {}
    FileData(const ChunkData &chunkData) { chunks.append(chunkData); }
    QList<ChunkData> chunks;
    DiffEditorController::DiffFileInfo leftFileInfo;
    DiffEditorController::DiffFileInfo rightFileInfo;
};

ChunkData calculateOriginalData(const QList<Diff> &leftDiffList,
                                const QList<Diff> &rightDiffList);
FileData calculateContextData(const ChunkData &originalData,
                              int contextLinesNumber);
QList<QTextEdit::ExtraSelection> colorPositions(const QTextCharFormat &format,
        QTextCursor &cursor,
        const QMap<int, int> &positions);
QTextCharFormat fullWidthFormatForTextStyle(const TextEditor::FontSettings &fontSettings,
                                            TextEditor::TextStyle textStyle);
} // namespace Internal
} // namespace DiffEditor

#endif // DIFFUTILS_H
