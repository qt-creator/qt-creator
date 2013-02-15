/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef DIFFEDITORWIDGET_H
#define DIFFEDITORWIDGET_H

#include "diffeditor_global.h"
#include "differ.h"

#include <QTextEdit>

namespace TextEditor {
class BaseTextEditorWidget;
class SnippetEditorWidget;
}

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
class QSplitter;
class QSyntaxHighlighter;
class QTextCharFormat;
QT_END_NAMESPACE



namespace DIFFEditor {

class DiffViewEditorWidget;

struct TextLineData {
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

struct RowData {
    RowData() : equal(true) {}
    RowData(const TextLineData &l)
        : leftLine(l), rightLine(l), equal(true) {}
    RowData(const TextLineData &l, const TextLineData &r, bool e = false)
        : leftLine(l), rightLine(r), equal(e) {}
    RowData(const QString &txt)
        : text(txt), equal(true) {}
    TextLineData leftLine;
    TextLineData rightLine;
    QString text; // file of context description
    bool equal; // true if left and right lines are equal, taking whitespaces into account (or both invalid)
};

struct ChunkData {
    ChunkData() : skippedLinesBefore(0), leftOffset(0), rightOffset(0) {}
    QList<RowData> rows;
    int skippedLinesBefore; // info for text
    int leftOffset;
    int rightOffset;
    // <absolute position in the file, absolute position in the file>
    QMap<int, int> changedLeftPositions; // counting from the beginning of the chunk
    QMap<int, int> changedRightPositions; // counting from the beginning of the chunk
    QString text;
};

struct FileData {
    FileData() {}
    FileData(const ChunkData &chunkData) { chunks.append(chunkData); }
    QList<ChunkData> chunks;
    QList<int> chunkOffset;
    QString text;
};

struct DiffData {
    QList<FileData> files;
    QString text;
};

class DIFFEDITOR_EXPORT DiffEditorWidget : public QWidget
{
    Q_OBJECT
public:
    DiffEditorWidget(QWidget *parent = 0);
    ~DiffEditorWidget();

    void setDiff(const QString &leftText, const QString &rightText);
    void setDiff(const QList<Diff> &diffList);
    QTextCodec *codec() const;

public slots:
    void setContextLinesNumber(int lines);
    void setIgnoreWhitespaces(bool ignore);

protected:
    TextEditor::SnippetEditorWidget *leftEditor() const;
    TextEditor::SnippetEditorWidget *rightEditor() const;

private slots:
    void leftSliderChanged();
    void rightSliderChanged();

private:
    bool isWhitespace(const QChar &c) const;
    bool isWhitespace(const Diff &diff) const;
    bool isEqual(const QList<Diff> &diffList, int diffNumber) const;
    QList<QTextEdit::ExtraSelection> colorPositions(const QTextCharFormat &format,
            QTextCursor &cursor,
            const QMap<int, int> &positions) const;
    void colorDiff(const FileData &fileData);
    ChunkData calculateOriginalData(const QList<Diff> &diffList) const;
    FileData calculateContextData(const ChunkData &originalData) const;
    void showDiff();

    DiffViewEditorWidget *m_leftEditor;
    DiffViewEditorWidget *m_rightEditor;
    QSplitter *m_splitter;

    QList<Diff> m_diffList;
    int m_contextLinesNumber;
    bool m_ignoreWhitespaces;

    ChunkData m_originalChunkData;
    FileData m_contextFileData;
    int m_leftSafePosHack;
    int m_rightSafePosHack;
};

} // namespace DIFFEditor

#endif // DIFFEDITORWIDGET_H
