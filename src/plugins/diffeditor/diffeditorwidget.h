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

#ifndef DIFFEDITORWIDGET_H
#define DIFFEDITORWIDGET_H

#include "diffeditor_global.h"
#include "differ.h"
#include "diffeditorcontroller.h"

#include <QTextEdit>

namespace TextEditor {
class BaseTextEditorWidget;
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
class TextLineData;
class ChunkData;
class FileData;

class DIFFEDITOR_EXPORT DiffEditorWidget : public QWidget
{
    Q_OBJECT
public:
    DiffEditorWidget(QWidget *parent = 0);
    ~DiffEditorWidget();

    void setDiffEditorController(DiffEditorController *controller);
    DiffEditorController *diffEditorController() const;

    QTextCodec *codec() const;

#ifdef WITH_TESTS
    void testAssemblyRows();
#endif // WITH_TESTS

private slots:
    void clear(const QString &message = QString());
    void setDiff(const QList<DiffEditorController::DiffFilesContents> &diffFileList, const QString &workingDirectory);

    void setContextLinesNumber(int lines);
    void setIgnoreWhitespaces(bool ignore);
    void setCurrentDiffFileIndex(int diffFileIndex);

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
    class DiffList {
    public:
        DiffEditorController::DiffFileInfo leftFileInfo;
        DiffEditorController::DiffFileInfo rightFileInfo;
        QList<Diff> diffList;
    };

    void setDiff(const QList<DiffList> &diffList);
    void handleWhitespaces(const QList<Diff> &input,
                           QList<Diff> *leftOutput,
                           QList<Diff> *rightOutput) const;
    QList<QTextEdit::ExtraSelection> colorPositions(const QTextCharFormat &format,
            QTextCursor &cursor,
            const QMap<int, int> &positions) const;
    void colorDiff(const QList<FileData> &fileDataList);
    FileData calculateContextData(const ChunkData &originalData) const;
    void showDiff();
    void synchronizeFoldings(DiffViewEditorWidget *source, DiffViewEditorWidget *destination);
    void jumpToOriginalFile(const QString &fileName, int lineNumber, int columnNumber);

    DiffEditorController *m_controller;
    DiffViewEditorWidget *m_leftEditor;
    DiffViewEditorWidget *m_rightEditor;
    QSplitter *m_splitter;

    QList<DiffList> m_diffList; // list of original outputs from differ
    QList<ChunkData> m_originalChunkData; // one big chunk for every file, ignoreWhitespace taken into account
    QList<FileData> m_contextFileData; // ultimate data to be shown, contextLinesNumber taken into account

    bool m_foldingBlocker;
    QString m_source;

    QTextCharFormat m_fileLineFormat;
    QTextCharFormat m_chunkLineFormat;
    QTextCharFormat m_leftLineFormat;
    QTextCharFormat m_leftCharFormat;
    QTextCharFormat m_rightLineFormat;
    QTextCharFormat m_rightCharFormat;
};

} // namespace DiffEditor

#endif // DIFFEDITORWIDGET_H
