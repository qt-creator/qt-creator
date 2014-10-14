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
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef SIDEBYSIDEDIFFEDITORWIDGET_H
#define SIDEBYSIDEDIFFEDITORWIDGET_H

#include "diffeditor_global.h"
#include "differ.h"
#include "diffeditorcontroller.h"
#include <QWidget>
#include <QTextCharFormat>

namespace TextEditor { class FontSettings; }

QT_BEGIN_NAMESPACE
class QSplitter;
class QMenu;
QT_END_NAMESPACE


namespace DiffEditor {
class DiffEditorGuiController;
class SideDiffEditorWidget;
class ChunkData;
class FileData;

class DIFFEDITOR_EXPORT SideBySideDiffEditorWidget : public QWidget
{
    Q_OBJECT
public:
    SideBySideDiffEditorWidget(QWidget *parent = 0);
    ~SideBySideDiffEditorWidget();

    void setDiffEditorGuiController(DiffEditorGuiController *controller);
    DiffEditorGuiController *diffEditorGuiController() const;

private slots:
    void clear(const QString &message = QString());
    void clearAll(const QString &message = QString());
    void setDiff(const QList<FileData> &diffFileList,
                 const QString &workingDirectory);

    void setCurrentDiffFileIndex(int diffFileIndex);

    void setFontSettings(const TextEditor::FontSettings &fontSettings);
    void slotLeftJumpToOriginalFileRequested(int diffFileIndex,
                                             int lineNumber, int columnNumber);
    void slotRightJumpToOriginalFileRequested(int diffFileIndex,
                                              int lineNumber, int columnNumber);
    void slotLeftContextMenuRequested(QMenu *menu, int diffFileIndex,
                                      int chunkIndex);
    void slotRightContextMenuRequested(QMenu *menu, int diffFileIndex,
                                       int chunkIndex);
    void slotSendChunkToCodePaster();
    void slotApplyChunk();
    void slotRevertChunk();
    void leftVSliderChanged();
    void rightVSliderChanged();
    void leftHSliderChanged();
    void rightHSliderChanged();
    void leftCursorPositionChanged();
    void rightCursorPositionChanged();
//    void leftDocumentSizeChanged();
//    void rightDocumentSizeChanged();

private:
    void showDiff();
//    void synchronizeFoldings(SideDiffEditorWidget *source, SideDiffEditorWidget *destination);
    void jumpToOriginalFile(const QString &fileName,
                            int lineNumber, int columnNumber);
    void patch(bool revert);

    DiffEditorGuiController *m_guiController;
    DiffEditorController *m_controller;
    SideDiffEditorWidget *m_leftEditor;
    SideDiffEditorWidget *m_rightEditor;
    QSplitter *m_splitter;

    QList<FileData> m_contextFileData; // ultimate data to be shown, contextLinesNumber taken into account

    bool m_ignoreCurrentIndexChange;
    bool m_foldingBlocker;
    int m_contextMenuFileIndex;
    int m_contextMenuChunkIndex;

    QTextCharFormat m_spanLineFormat;
    QTextCharFormat m_fileLineFormat;
    QTextCharFormat m_chunkLineFormat;
    QTextCharFormat m_leftLineFormat;
    QTextCharFormat m_leftCharFormat;
    QTextCharFormat m_rightLineFormat;
    QTextCharFormat m_rightCharFormat;
};

} // namespace DiffEditor

#endif // SIDEBYSIDEDIFFEDITORWIDGET_H
