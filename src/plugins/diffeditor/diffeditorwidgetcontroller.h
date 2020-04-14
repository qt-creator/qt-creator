/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "diffutils.h"

#include <QObject>
#include <QTextCharFormat>
#include <QTimer>

QT_FORWARD_DECLARE_CLASS(QMenu)

namespace Core { class IDocument; }
namespace TextEditor { class FontSettings; }
namespace Utils { class ProgressIndicator; }

namespace DiffEditor {

class ChunkSelection;

namespace Internal {

class DiffEditorDocument;

class DiffEditorWidgetController : public QObject
{
    Q_OBJECT
public:
    explicit DiffEditorWidgetController(QWidget *diffEditorWidget);

    void setDocument(DiffEditorDocument *document);
    DiffEditorDocument *document() const;

    void jumpToOriginalFile(const QString &fileName, int lineNumber,
                            int columnNumber);
    void setFontSettings(const TextEditor::FontSettings &fontSettings);
    void addCodePasterAction(QMenu *menu, int fileIndex, int chunkIndex);
    void addApplyAction(QMenu *menu, int fileIndex, int chunkIndex);
    void addRevertAction(QMenu *menu, int fileIndex, int chunkIndex);
    void addExtraActions(QMenu *menu, int fileIndex, int chunkIndex, const ChunkSelection &selection);
    void updateCannotDecodeInfo();

    ChunkData chunkData(int fileIndex, int chunkIndex) const;

    bool m_ignoreCurrentIndexChange = false;
    QList<FileData> m_contextFileData; // ultimate data to be shown
                                       // contextLineCount taken into account
    QTextCharFormat m_fileLineFormat;
    QTextCharFormat m_chunkLineFormat;
    QTextCharFormat m_leftLineFormat;
    QTextCharFormat m_rightLineFormat;
    QTextCharFormat m_leftCharFormat;
    QTextCharFormat m_rightCharFormat;

private:
    void patch(bool revert, int fileIndex, int chunkIndex);
    void sendChunkToCodePaster(int fileIndex, int chunkIndex);
    bool chunkExists(int fileIndex, int chunkIndex) const;
    bool fileNamesAreDifferent(int fileIndex) const;

    void scheduleShowProgress();
    void showProgress();
    void hideProgress();
    void onDocumentReloadFinished();

    QWidget *m_diffEditorWidget = nullptr;

    DiffEditorDocument *m_document = nullptr;

    Utils::ProgressIndicator *m_progressIndicator = nullptr;
    QTimer m_timer;
};

} // namespace Internal
} // namespace DiffEditor
