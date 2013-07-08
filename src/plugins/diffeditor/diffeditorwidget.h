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
class FontSettings;
}

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
class QSplitter;
class QSyntaxHighlighter;
class QTextCharFormat;
QT_END_NAMESPACE



namespace DiffEditor {

class DiffViewEditorWidget;
struct TextLineData;
struct ChunkData;
struct FileData;

class DIFFEDITOR_EXPORT DiffEditorWidget : public QWidget
{
    Q_OBJECT
public:
    struct DiffFileInfo {
        DiffFileInfo() {}
        DiffFileInfo(const QString &file) : fileName(file) {}
        DiffFileInfo(const QString &file, const QString &type) : fileName(file), typeInfo(type) {}
        QString fileName;
        QString typeInfo;
    };

    struct DiffFilesContents {
        DiffFileInfo leftFileInfo;
        QString leftText;
        DiffFileInfo rightFileInfo;
        QString rightText;
    };

    DiffEditorWidget(QWidget *parent = 0);
    ~DiffEditorWidget();

    void clear();
    void clear(const QString &message);
    void setDiff(const QList<DiffFilesContents> &diffFileList, const QString &workingDirectory = QString());
    QTextCodec *codec() const;

#ifdef WITH_TESTS
    void testAssemblyRows();
#endif // WITH_TESTS

public slots:
    void setContextLinesNumber(int lines);
    void setIgnoreWhitespaces(bool ignore);
    void setHorizontalScrollBarSynchronization(bool on);
    void navigateToDiffFile(int diffFileIndex);

signals:
    void navigatedToDiffFile(int diffFileIndex);

protected:
    TextEditor::SnippetEditorWidget *leftEditor() const;
    TextEditor::SnippetEditorWidget *rightEditor() const;

private slots:
    void setFontSettings(const TextEditor::FontSettings &fontSettings);
    void slotLeftJumpToOriginalFileRequested(int diffFileIndex, int lineNumber, int columnNumber);
    void slotRightJumpToOriginalFileRequested(int diffFileIndex, int lineNumber, int columnNumber);
    void leftVSliderChanged();
    void rightVSliderChanged();
    void leftHSliderChanged();
    void rightHSliderChanged();
    void leftCursorPositionChanged();
    void rightCursorPositionChanged();
    void leftDocumentSizeChanged();
    void rightDocumentSizeChanged();

private:
    struct DiffList {
        DiffFileInfo leftFileInfo;
        DiffFileInfo rightFileInfo;
        QList<Diff> diffList;
    };

    void setDiff(const QList<DiffList> &diffList);
    bool isWhitespace(const QChar &c) const;
    bool isWhitespace(const Diff &diff) const;
    bool isEqual(const QList<Diff> &diffList, int diffNumber) const;
    QList<QTextEdit::ExtraSelection> colorPositions(const QTextCharFormat &format,
            QTextCursor &cursor,
            const QMap<int, int> &positions) const;
    void colorDiff(const QList<FileData> &fileDataList);
    QList<TextLineData> assemblyRows(const QStringList &lines,
                                                       const QMap<int, int> &lineSpans,
                                                       const QMap<int, int> &changedPositions,
                                                       QMap<int, int> *outputChangedPositions) const;
    ChunkData calculateOriginalData(const QList<Diff> &diffList) const;
    FileData calculateContextData(const ChunkData &originalData) const;
    void showDiff();
    void synchronizeFoldings(DiffViewEditorWidget *source, DiffViewEditorWidget *destination);
    void jumpToOriginalFile(const QString &fileName, int lineNumber, int columnNumber);

    DiffViewEditorWidget *m_leftEditor;
    DiffViewEditorWidget *m_rightEditor;
    QSplitter *m_splitter;

    QList<DiffList> m_diffList; // list of original outputs from differ
    QList<ChunkData> m_originalChunkData; // one big chunk for every file, ignoreWhitespaces taken into account
    QList<FileData> m_contextFileData; // ultimate data to be shown, contextLinesNumber taken into account
    QString m_workingDirectory;
    int m_contextLinesNumber;
    bool m_ignoreWhitespaces;
    bool m_syncScrollBars;

    bool m_foldingBlocker;

    QTextCharFormat m_fileLineFormat;
    QTextCharFormat m_chunkLineFormat;
    QTextCharFormat m_leftLineFormat;
    QTextCharFormat m_leftCharFormat;
    QTextCharFormat m_rightLineFormat;
    QTextCharFormat m_rightCharFormat;
};

} // namespace DiffEditor

#endif // DIFFEDITORWIDGET_H
