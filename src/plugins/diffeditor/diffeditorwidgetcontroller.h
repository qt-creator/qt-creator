// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "diffutils.h"

#include <coreplugin/patchtool.h>
#include <utils/guard.h>

#include <QObject>
#include <QTextCharFormat>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QMenu;
QT_END_NAMESPACE

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
    void addPatchAction(QMenu *menu, int fileIndex, int chunkIndex, Core::PatchAction patchAction);
    void addExtraActions(QMenu *menu, int fileIndex, int chunkIndex, const ChunkSelection &selection);
    void updateCannotDecodeInfo();
    void setBusyShowing(bool busy);
    void setCurrentDiffFileIndex(int index) { m_currentDiffFileIndex = index; }
    int currentDiffFileIndex() const { return m_currentDiffFileIndex; }

    ChunkData chunkData(int fileIndex, int chunkIndex) const;

    Utils::Guard m_ignoreChanges;
    QList<FileData> m_contextFileData; // ultimate data to be shown
                                       // contextLineCount taken into account
    QTextCharFormat m_fileLineFormat;
    QTextCharFormat m_chunkLineFormat;
    QTextCharFormat m_spanLineFormat;
    std::array<QTextCharFormat, SideCount> m_lineFormat{};
    std::array<QTextCharFormat, SideCount> m_charFormat{};

private:
    bool isInProgress() const;
    void toggleProgress(bool wasInProgress);

    void patch(Core::PatchAction patchAction, int fileIndex, int chunkIndex);
    void sendChunkToCodePaster(int fileIndex, int chunkIndex);
    bool chunkExists(int fileIndex, int chunkIndex) const;
    bool fileNamesAreDifferent(int fileIndex) const;

    void scheduleShowProgress();
    void showProgress();
    void hideProgress();
    void onDocumentReloadFinished();

    QWidget *m_diffEditorWidget = nullptr;

    DiffEditorDocument *m_document = nullptr;

    bool m_isBusyShowing = false;
    int m_currentDiffFileIndex = -1;
    Utils::ProgressIndicator *m_progressIndicator = nullptr;
    QTimer m_timer;
};

class DiffEditorInput
{
public:
    DiffEditorInput(DiffEditorWidgetController *controller);
    QList<FileData> m_contextFileData;
    QTextCharFormat *m_fileLineFormat = nullptr;
    QTextCharFormat *m_chunkLineFormat = nullptr;
    QTextCharFormat *m_spanLineFormat = nullptr;
    std::array<QTextCharFormat *, SideCount> m_lineFormat{};
    std::array<QTextCharFormat *, SideCount> m_charFormat{};
};

} // namespace Internal
} // namespace DiffEditor
