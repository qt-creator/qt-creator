// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "diffutils.h"

#include <utils/guard.h>

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

    Utils::Guard m_ignoreChanges;
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
