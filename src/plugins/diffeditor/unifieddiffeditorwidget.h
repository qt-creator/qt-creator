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

#ifndef UNIFIEDDIFFEDITORWIDGET_H
#define UNIFIEDDIFFEDITORWIDGET_H

#include "diffutils.h"
#include "selectabletexteditorwidget.h"

namespace TextEditor {
class DisplaySettings;
class FontSettings;
}

QT_BEGIN_NAMESPACE
class QSplitter;
class QTextCharFormat;
QT_END_NAMESPACE

namespace DiffEditor {

class ChunkData;
class FileData;

namespace Internal {

class DiffEditorDocument;

class UnifiedDiffEditorWidget : public SelectableTextEditorWidget
{
    Q_OBJECT
public:
    UnifiedDiffEditorWidget(QWidget *parent = 0);

    void setDocument(DiffEditorDocument *document);

    void setDiff(const QList<FileData> &diffFileList,
                 const QString &workingDirectory);
    void setCurrentDiffFileIndex(int diffFileIndex);

    void saveState();
    void restoreState();

    void clear(const QString &message = QString());

signals:
    void currentDiffFileIndexChanged(int index);

public slots:
    void setDisplaySettings(const TextEditor::DisplaySettings &ds) override;

protected:
    void mouseDoubleClickEvent(QMouseEvent *e) override;
    void contextMenuEvent(QContextMenuEvent *e) override;
    QString lineNumber(int blockNumber) const override;
    int lineNumberDigits() const override;

private slots:
    void setFontSettings(const TextEditor::FontSettings &fontSettings);

    void slotCursorPositionChangedInEditor();

    void slotSendChunkToCodePaster();
    void slotApplyChunk();
    void slotRevertChunk();

private:
    void setLeftLineNumber(int blockNumber, int lineNumber);
    void setRightLineNumber(int blockNumber, int lineNumber);
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
    void jumpToOriginalFile(const QString &fileName,
                            int lineNumber,
                            int columnNumber);
    void addContextMenuActions(QMenu *menu,
                               int diffFileIndex,
                               int chunkIndex);
    void patch(bool revert);

    DiffEditorDocument *m_document;

    // block number, visual line number.
    QMap<int, int> m_leftLineNumbers;
    QMap<int, int> m_rightLineNumbers;
    bool m_ignoreCurrentIndexChange;
    int m_contextMenuFileIndex;
    int m_contextMenuChunkIndex;

    int m_leftLineNumberDigits;
    int m_rightLineNumberDigits;
    // block number, visual line number.
    QMap<int, QPair<DiffFileInfo, DiffFileInfo> > m_fileInfo;
    // start block number, block count of a chunk, chunk index inside a file.
    QMap<int, QPair<int, int> > m_chunkInfo;

    QList<FileData> m_contextFileData; // ultimate data to be shown
                                       // contextLineCount taken into account

    QTextCharFormat m_fileLineFormat;
    QTextCharFormat m_chunkLineFormat;
    QTextCharFormat m_leftLineFormat;
    QTextCharFormat m_rightLineFormat;
    QTextCharFormat m_leftCharFormat;
    QTextCharFormat m_rightCharFormat;
    QByteArray m_state;
};

} // namespace Internal
} // namespace DiffEditor

#endif // UNIFIEDDIFFEDITORWIDGET_H
